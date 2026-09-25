#include "rdb/descriptor.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>
#include <ranges>
#include <sstream>
#include <utility>

#include "rdb/exceptions.hpp"

#include <magic_enum/magic_enum.hpp>

extern std::string parserDESCString(rdb::Descriptor &desc, std::string_view inlet);

namespace rdb {

constexpr auto GetFieldType(const std::string_view name) {
  if (name == "NULL") return std::optional<rdb::descFld>(rdb::NULLTYPE);
  return magic_enum::enum_cast<rdb::descFld>(name);
}

constexpr auto GetFieldType(const rdb::descFld index) {
  return index == rdb::NULLTYPE ? std::string_view("NULL") : magic_enum::enum_name(index);
}

constexpr auto isConfigurationField(const rdb::descFld index) {
  return index == rdb::TYPE ||       //
         index == rdb::REF ||        //
         index == rdb::RETENTION ||  //
         index == rdb::RETMEMORY;
}

/// Szerokosc POJEDYNCZEGO slotu plaskiego rekordu.
///
/// Dla pola nietablicowego jest to rozmiar calego pola, czyli dokladnie to, co liczy
/// fieldSize(). Numeryczne `T[N]` zajmuje N slotow po `rlen` bajtow; `STRING[N]` pozostaje
/// JEDNYM slotem o dlugosci N - ten sam podzial, ktorego uzywa rebuildFieldMappings()
/// przy wyznaczaniu offsetow.
constexpr int flatSlotSize(const rField &field) {
  if (isConfigurationField(field.rtype)) return 0;
  if (field.rtype == rdb::NULLTYPE) return 0;
  if (field.rtype != rdb::STRING && field.rarray > 1) return field.rlen;
  return field.rlen * field.rarray;
}

Descriptor::Descriptor(std::initializer_list<rField> fields) : std::vector<rField>(fields) {}

Descriptor::Descriptor(const std::string &fieldName, int length, int elementCount, rdb::descFld type) {  //
  emplace_back(fieldName, length, elementCount, type);                                                   //
}

void Descriptor::rebuildFieldMappings() const {
  if (!fieldMappingsDirty_) return;

  flatToDescriptorIndexMap_.clear();
  fieldByteOffsets_.clear();

  flattenedFieldCount_ = 0;
  dataSizeBytes_       = 0;
  int offset{0};
  for (size_t descriptorFieldIdx = 0; descriptorFieldIdx < size(); ++descriptorFieldIdx) {
    const auto &field = (*this)[descriptorFieldIdx];
    dataSizeBytes_ += fieldSize(field);  // semantyka identyczna z dawnym getSizeInBytes (pola konfiguracyjne i NULLTYPE = 0)
    if (isConfigurationField(field.rtype)) continue;

    const int flatCount = rdb::flatElementCount(field);
    for (int arrayIndex = 0; arrayIndex < flatCount; ++arrayIndex) {
      flatToDescriptorIndexMap_.emplace_back(static_cast<int>(descriptorFieldIdx), arrayIndex);
      fieldByteOffsets_.push_back(offset);
      offset += (field.rtype == rdb::STRING) ? fieldSize(field) : field.rlen;
      ++flattenedFieldCount_;
    }
  }
  fieldMappingsDirty_ = false;
}

// flatIndexToDescriptorPosition / flatElementCount / byteOffsetAtFlatIndex sa
// teraz inline w descriptor.hpp (P2, speed_improvement) - hot-path bez wywolan
// cross-TU. Tu zostaje tylko zimna sciezka bledu byteOffsetAtFlatIndex.
void Descriptor::flatIndexOutOfRange(const int flatIndex) const {
  rebuildFieldMappings();
  throw LogicError(fmt::format("descriptor: flatIndex {} out of range [0,{})", flatIndex, flattenedFieldCount_));
}

std::vector<rField> Descriptor::dataFields() {
  rebuildFieldMappings();
  std::vector<rField> ret;
  ret.reserve(flattenedFieldCount_);
  for (const auto &i : (*this)) {
    if (isConfigurationField(i.rtype)) continue;
    ret.push_back(i);
  }
  return ret;
}

void Descriptor::append(std::initializer_list<rField> fields) {
  insert(end(), fields.begin(), fields.end());
  fieldMappingsDirty_ = true;
}

Descriptor operator+(const Descriptor &lhs, const Descriptor &rhs) {
  Descriptor ret(lhs);
  ret += rhs;
  return ret;
}

Descriptor &Descriptor::operator+=(const Descriptor &rhs) {
  if (this != &rhs) {
    insert(end(), rhs.begin(), rhs.end());  // TODO: add rename of duplicates here.
  } else {
    throw LogicError("descriptor: cannot merge descriptor with itself");
    // can't do safe: data | data
    // due one name policy
  }

  fieldMappingsDirty_ = true;
  return *this;
}

// this      rhs
// 4,INT  == 1,BYTE   1
// 1,BYTE == 4,INT    0
// 4,INT  == 4,INT    1
//
// Zgodnosc idzie po SLOTACH PLASKICH, nie po wpisach deskryptora. `INTEGER[3]` i trzy pola
// `INTEGER` opisuja ten sam rekord - te same bajty pod tymi samymi offsetami - ale maja
// odpowiednio jeden i trzy wpisy. Liczac wpisy, para taka wychodzila NIEZGODNA i
// payload::operator= konczylo sie bledem krytycznym; dotykalo to kazdego przypisania miedzy
// tymi dwoma zapisami rekordu, w tym przeplotu `#` nad polem tablicowym.
//
// Dla deskryptora BEZ numerycznego pola tablicowego wynik jest identyczny jak poprzednio:
// pole nietablicowe zajmuje dokladnie jeden slot o szerokosci fieldSize().
bool Descriptor::operator==(const Descriptor &rhs) const {
  if (flatElementCount() != rhs.flatElementCount()) return false;

  const int slots = flatElementCount();
  for (int slot = 0; slot < slots; ++slot) {
    const auto lhsPosition = flatIndexToDescriptorPosition(slot);
    const auto rhsPosition = rhs.flatIndexToDescriptorPosition(slot);
    if (!lhsPosition || !rhsPosition) return false;
    const auto &lhsField = (*this)[lhsPosition->first];
    const auto &rhsField = rhs[rhsPosition->first];

    if (flatSlotSize(lhsField) < flatSlotSize(rhsField) || lhsField.rtype < rhsField.rtype) return false;
  }
  return true;
}

void Descriptor::removeConfigurationFields() {
  Descriptor rhs(*this);
  clear();
  std::ranges::copy_if(rhs,                        //
                       std::back_inserter(*this),  //
                       [](const rField &i) {       //
                         return !isConfigurationField(i.rtype);
                       });

  fieldMappingsDirty_ = true;
}

void Descriptor::composeHashDescriptorFrom(const std::string &fieldNamePrefix, Descriptor lhs, Descriptor rhs) {
  lhs.removeConfigurationFields();
  rhs.removeConfigurationFields();
  // Zgodnosc idzie po slotach PLASKICH, nie po liczbie wpisow: `INTEGER[3]` i trzy pola
  // `INTEGER` opisuja ten sam rekord, a `#` laczy rekordy, nie deklaracje pol. Liczac wpisy,
  // strona tablicowa dawala deskryptor o szerokosci 1 zamiast 3 i przeplot zawieszal sie na
  // rekordzie wezszym niz plaski uklad zrodla. Ten sam warunek ta sama miara sprawdza
  // compiler::buildOutputSchema().
  if (lhs.flatElementCount() != rhs.flatElementCount()) {
    throw LogicError(fmt::format("descriptor: hash composition requires equal-width descriptors: lhs={} rhs={}",
                                 lhs.flatElementCount(), rhs.flatElementCount()));
  }

  clear();
  const int width = lhs.flatElementCount();
  for (int i = 0; i < width; ++i) {
    const auto lhsPosition = lhs.flatIndexToDescriptorPosition(i);
    const auto rhsPosition = rhs.flatIndexToDescriptorPosition(i);
    if (!lhsPosition || !rhsPosition) throw LogicError("descriptor: invalid flat field position");
    const auto &lhsField = lhs[lhsPosition->first];
    const auto &rhsField = rhs[rhsPosition->first];
    auto maxRtype        = std::max(lhsField.rtype, rhsField.rtype);
    auto maxRlen         = std::max(lhsField.rlen, rhsField.rlen);
    push_back(rField(fieldNamePrefix + "_" + std::to_string(i), maxRlen, 1, maxRtype));
  }

  fieldMappingsDirty_ = true;
}

int Descriptor::fieldSize(const rdb::rField &field) const {
  if (isConfigurationField(field.rtype)) return 0;
  if (field.rtype == rdb::NULLTYPE) return 0;
  return field.rlen * field.rarray;
}

size_t Descriptor::getSizeInBytes() const {
  // Wolane przez payload::span() przy kazdym dostepie do pola -- suma jest
  // cache'owana w rebuildFieldMappings zamiast liczona za kazdym razem.
  rebuildFieldMappings();
  return dataSizeBytes_;
}

rdb::retention_t Descriptor::retention() {
  rdb::retention_t retval{.segments = 0, .capacity = 0};

  auto it = std::ranges::find_if(*this,                                                         //
                                 [](const auto &item) { return item.rtype == rdb::RETENTION; }  //
  );

  if (it != end()) retval = std::pair<int, int>((*it).rlen, (*it).rarray);

  return retval;
}

std::pair<std::string, size_t> Descriptor::storagePolicy() {
  int retval{0};

  auto it1 = std::ranges::find_if(*this,                                                         //
                                  [](const auto &item) { return item.rtype == rdb::RETMEMORY; }  //
  );

  if (it1 != end()) retval = (*it1).rlen;

  std::string retvalType;
  auto it2 = std::ranges::find_if(*this,                                                    //
                                  [](const auto &item) { return item.rtype == rdb::TYPE; }  //
  );

  if (it2 != end()) retvalType = (*it2).rname;

  return std::make_pair(retvalType, retval);
}

size_t Descriptor::fieldIndex(const std::string_view fieldName) {
  auto it = std::ranges::find_if(*this,                                                               //
                                 [fieldName](const auto &item) { return item.rname == fieldName; });  //

  if (it != end()) return std::distance(begin(), it);
  // Nazwa pola W KOMUNIKACIE, bo ta funkcja jest wystawiona wprost do Pythona
  // (Descriptor.field_index) i wolana z tekstem od uzytkownika. "field not found" bez
  // podania, ktorego, zmusza do zgadywania przy kazdej literowce.
  throw LogicError(fmt::format("descriptor: no field named '{}'", fieldName));
}

int Descriptor::fieldSize(const std::string_view fieldName) { return fieldSize((*this)[fieldIndex(fieldName)]); }

size_t Descriptor::fieldByteOffset(const std::string_view fieldName) {
  auto offset{0};
  for (auto const &field : *this) {
    if (fieldName == field.rname) return offset;
    offset += fieldSize(field);
  }
  throw LogicError(fmt::format("descriptor: no field named '{}' (byte offset lookup)", fieldName));
}

std::string_view Descriptor::fieldTypeName(const std::string_view fieldName) {  //
  return GetFieldType(((*this)[fieldIndex(fieldName)]).rtype);                  //
}

std::pair<rdb::descFld, int> Descriptor::widestFieldType() {
  rdb::descFld retVal{rdb::BYTE};
  auto size{1};
  for (auto const &field : *this) {
    if (isConfigurationField(field.rtype)) continue;
    // Liczby tablicowe sa splaszczane do osobnych elementow, wiec szerokoscia
    // pojedynczej wartosci jest rlen, a nie rozmiar calego pola rlen*rarray.
    // STRING pozostaje jednym elementem plaskim i zachowuje pelna szerokosc pola.
    const int elementSize = (field.rtype == rdb::STRING) ? fieldSize(field) : field.rlen;
    if (retVal < field.rtype) {
      retVal = field.rtype;
      size   = elementSize;
    } else if (retVal == field.rtype) {
      size = std::max(size, elementSize);
    }
  }
  return std::make_pair(retVal, size);
}

namespace {
/// Slot xalloc jest przydzielany RAZ na proces i jest niezmienny - to indeks, nie stan. Sama
/// wartosc flagi zyje w iword() konkretnego strumienia.
int singleLineSlot() {
  static const int slot = std::ios_base::xalloc();
  return slot;
}
}  // namespace

bool isSingleLineOutput(std::ostream &os) { return os.iword(singleLineSlot()) != 0; }

void setSingleLineOutput(std::ostream &os, const bool enabled) { os.iword(singleLineSlot()) = enabled ? 1 : 0; }

std::ostream &singleLineFormat(std::ostream &os) {
  setSingleLineOutput(os, true);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Descriptor &rhs) {
  os << "{";
  for (auto const &r : rhs) {
    if (r.rtype == rdb::RETENTION)
      if (r.rlen == 0 && r.rarray == 0) continue;  // skip retention 0,0
    if (r.rtype == rdb::RETMEMORY)
      if (r.rlen == 0) continue;  // skip retention memory 0
    if (!isSingleLineOutput(os))
      os << "\t";
    else
      os << " ";
    os << GetFieldType(r.rtype) << " ";

    switch (r.rtype) {
      case rdb::REF:
        os << "\"" << r.rname << "\"";
        break;
      case rdb::TYPE:
        os << r.rname;
        break;
      case rdb::RETENTION:
        // retention {segment} {capacity}
        os << r.rarray << " " << r.rlen;
        break;
      case rdb::RETMEMORY:
        // retention memory {capacity}
        os << r.rlen;
        break;
      default:
        os << r.rname;
    }

    if (r.rarray > 1 && (r.rtype != rdb::RETENTION))
      os << "[" << r.rarray << "]";
    else if (r.rtype == rdb::STRING)
      os << "[" << r.rlen << "]";
    if (!isSingleLineOutput(os)) os << '\n';
  }
  if (rhs.empty())
    os << "Empty";
  else if (isSingleLineOutput(os))
    os << " ";
  os << "}";
  setSingleLineOutput(os, false);

  return os;
}

/// Wczytaj deskryptor ze strumienia. Bledny tekst zapala failbit; NIE konczy procesu.
///
/// Do fazy 1 bledny deskryptor konczyl sie przez FatalError, czyli std::exit. Ekstraktor
/// strumieniowy nie jest jednak miejscem, w ktorym zapada decyzja o przerwaniu programu -
/// zglasza niepowodzenie stanem strumienia, a co z nim zrobic, wie wolajacy:
/// loadDescriptorFile rzuca CorruptDescriptor, xtrdb wypisuje komunikat i czyta dalej.
std::istream &operator>>(std::istream &is, Descriptor &rhs) {
  // Strumien juz uszkodzony: nie ma czego czytac i nie wolno zmieniac jego stanu -
  // w szczegolnosci nie wolno zgasic failbita ustawionego przez nieudane otwarcie pliku.
  if (!is.good()) return is;

  std::stringstream strstream;
  std::string str;
  while (is >> str)
    strstream << " " << str;

  // Petla powyzej konczy sie WYLACZNIE niepowodzeniem ekstrakcji, wiec po odczytaniu
  // calego (poprawnego) tekstu failbit jest zapalony tak samo jak po bledzie. Gasimy go,
  // zeby po powrocie znaczyl dokladnie jedno: deskryptor sie nie sparsowal. eofbit i
  // badbit zostaja nietkniete.
  is.clear(is.rdstate() & ~std::ios::failbit);

  // Parsujemy do obiektu tymczasowego, a nie wprost do rhs. Listener uzupelnia deskryptor
  // polami w miare schodzenia po drzewie, wiec przerwany parse zostawilby rhs w stanie
  // czesciowym - a wolajacy, ktory sprawdzi failbit dopiero po powrocie, mialby juz wtedy
  // podmieniony wlasny deskryptor.
  Descriptor parsed;
  const auto result = parserDESCString(parsed, strstream.str());
  if (result != "OK") {
    SPDLOG_ERROR("Descriptor parse failed: {}", result);
    is.setstate(std::ios::failbit);
    return is;
  }

  rhs = std::move(parsed);
  return is;
}

}  // namespace rdb
