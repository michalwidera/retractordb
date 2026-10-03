#include "rdb/payload.hpp"

// RDB_HAS_ADDR2LINE - wybor zaplecza sladu stosu i tresc komunikatu ponizej. Samo
// Boost.Stacktrace mieszka w stackTrace.cc (wyspa z wyjatkami): jego naglowki maja `throw`
// i `try` w funkcjach inline, wiec w tej jednostce, kompilowanej z -fno-exceptions, nie
// skompilowalyby sie (zaplecze domyslne Boosta z Conana na macOS).
#include "platformConfig.h"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <algorithm>  // std::min, std::copy, std::fill
#include <array>
#include <bit>
#include <boost/rational.hpp>

#include <cstdint>
#include <cstring>  // std::memcpy (for C-interop)
#include <iomanip>
#include <iostream>
#include <limits>
#include <ranges>
#include <sstream>
#include <type_traits>
#include <utility>
#include "rdb/error.hpp"

#include "rdb/convertTypes.hpp"
#include "rdb/stackTrace.hpp"

namespace rdb {

namespace {

constexpr int kHexByteWidth = 2;
constexpr int kHexWordWidth = 8;

int resolveFieldIndexOrAbort(const Descriptor &descriptor, const int positionFlat, const char *context) {
  const auto flatCount = descriptor.flatElementCount();
  if (positionFlat < 0 || positionFlat > flatCount - 1) {
    SPDLOG_ERROR("{} out of descriptor req:{} available len: {}", context, positionFlat, flatCount);
    if (std::string_view(context) == "Read") {
#if RDB_HAS_ADDR2LINE
      std::cerr << "Collecting stack trace (addr2line) - this may take more than 60 s, the process is not hung." << '\n';
#else
      std::cerr << "Collecting stack trace." << '\n';
#endif
      const std::string trace = rdb::currentStackTrace();
      SPDLOG_ERROR("Stack: {}", trace);
      std::cerr << trace << '\n';
    }
    // Pozycja plaska pochodzi z kompilatora planu albo ze strazy wiazania Pythona (IndexError
    // przed wywolaniem) - spoza zakresu znaczy blad w kodzie, a zapis dalej bylby zapisem poza
    // rekordem. Stad zatrzymanie, nie blad zwracany.
    rdb::fatal(fmt::format("payload: {} flat position {} out of range [0,{})", context, positionFlat, flatCount));
  }

  auto positionOpt = descriptor.flatIndexToDescriptorPosition(positionFlat);
  RDB_ASSERT(positionOpt.has_value(), "payload: {} conversion failed for flat position {}", context, positionFlat);
  const auto position = positionOpt->first;
  RDB_ASSERT(position >= 0 && std::cmp_less(position, descriptor.size()),
             "payload: {} converted index {} out of descriptor bounds", context, position);
  return position;
}

/// Wartosc std::any jako T. Typ niesiony przez any wynika z typu pola, ktory zna wolajacy, wiec
/// niezgodnosc to blad w kodzie. Forma z wskaznikiem zamiast std::any_cast<T>(any): ta rzuca
/// bad_any_cast, a w buildzie bez wyjatkow konczy proces bez slowa o przyczynie.
template <typename T>
const T &anyAs(const std::any &value) {
  const T *typed = std::any_cast<T>(&value);
  RDB_ASSERT(typed != nullptr, "payload: value of type {} where the field type expects another", value.type().name());
  return *typed;
}

void writeValue(std::ostream &os, const std::any &value, const descFld type, const bool hexFormat) {
  switch (type) {
    case rdb::NULLTYPE:
      os << "null";
      break;
    case rdb::STRING:
      os << anyAs<std::string>(value);
      break;
    case rdb::BYTE: {
      if (hexFormat) {
        os << std::setfill('0') << std::setw(kHexByteWidth);
      }
      os << static_cast<int>(anyAs<uint8_t>(value));
      break;
    }
    case rdb::INTEGER: {
      if (hexFormat) {
        os << std::setfill('0') << std::setw(kHexWordWidth);
      }
      os << anyAs<int>(value);
      break;
    }
    case rdb::UINT: {
      if (hexFormat) {
        os << std::setfill('0') << std::setw(kHexWordWidth);
      }
      os << anyAs<unsigned>(value);
      break;
    }
    case rdb::FLOAT:
      os << anyAs<float>(value);
      break;
    case rdb::DOUBLE:
      os << anyAs<double>(value);
      break;
    case rdb::RATIONAL:
      os << anyAs<boost::rational<int>>(value);
      break;
    case rdb::INTPAIR:
    case rdb::IDXPAIR:
      // Pary sa operandami tokenow planu, nie typami pol rekordu - gramatyka .desc ich nie zna (#267).
      // Tu i tak nie dochodza: operator<< czyta wartosc przez getItem, a ten konczy proces na tym
      // samym typie. Jawne przypadki zamiast milczacego pominiecia (-Wswitch).
      rdb::fatal("payload: INTPAIR/IDXPAIR are plan token operands, not record field types");
    case rdb::REF:
    case rdb::TYPE:
    case rdb::RETENTION:
    case rdb::RETMEMORY:
      // operator<< pomija pola konfiguracyjne, zanim zapyta o wartosc - tu nie dochodzi nic poza bledem w kodzie.
      rdb::fatal("payload: configuration fields (REF/TYPE/RETENTION) cannot be formatted");
  }
}

/// Ulamek w postaci tekstowej "n/d" - zamiennik operator>> z boost/rational.hpp, ktory w srodku
/// lapie bad_rational (try/catch) i dlatego nie kompiluje sie w rdzeniu bez wyjatkow. Semantyka ta
/// sama: tekst, ktory nie jest ulamkiem z poprawnym mianownikiem, ustawia failbit i zostawia @p out.
std::istream &readRationalText(std::istream &is, boost::rational<int> &out) {
  int numerator   = 0;
  int denominator = 1;
  char slash      = 0;
  if (!(is >> numerator)) return is;
  if (!is.get(slash) || slash != '/' || !(is >> std::noskipws >> denominator >> std::skipws)) {
    is.setstate(std::ios::failbit);
    return is;
  }
  // Mianownik zero, INT_MIN albo ujemny przy liczniku INT_MIN nie da sie znormalizowac - patrz
  // rationalOf w convertTypes.cc.
  if (denominator == 0 || denominator == std::numeric_limits<int>::min() ||
      (denominator < 0 && numerator == std::numeric_limits<int>::min())) {
    is.setstate(std::ios::failbit);
    return is;
  }
  out.assign(numerator, denominator);
  return is;
}

template <typename T>
void copyToMemory(std::istream &is, payload &rhs, const std::string_view fieldName, const int arrayOffset) {
  T data{};
  if constexpr (std::is_same_v<T, boost::rational<int>>)
    readRationalText(is, data);
  else
    is >> data;
  Descriptor desc(rhs.descriptor);
  auto dest = rhs.span().subspan(desc.fieldByteOffset(fieldName) + arrayOffset, sizeof(T));
  std::memcpy(dest.data(), &data, sizeof(T));
}

}  // namespace

// default constructor

payload::payload(const Descriptor &descriptor) : descriptor(descriptor) {
  payloadData_.assign(descriptor.getSizeInBytes(), 0);

  nullBitset_.resize(descriptor.size(), false);
}

// copy constructor

payload::payload(const payload &other) {
  descriptor = other.descriptor;
  payloadData_.assign(other.descriptor.getSizeInBytes(), 0);
  std::copy(other.span().begin(), other.span().end(), span().begin());
  nullBitset_ = other.nullBitset_;
}

// Move constructor

// Kolejnosc listy inicjalizacyjnej idzie po kolejnosci deklaracji skladnikow.
// hexFormat_ zostaje domyslny, dokladnie jak w konstruktorze kopiujacym.
payload::payload(payload &&other) noexcept
    : payloadData_(std::move(other.payloadData_)),
      nullBitset_(std::move(other.nullBitset_)),
      descriptor(std::move(other.descriptor)) {}

// Copy & assignment operator

payload &payload::operator=(const payload &other) {
  if (this == &other) return *this;  // assure not a self-assignment

  *this = other.descriptor;  // call operator=(const Descriptor
  std::copy(other.span().begin(), other.span().end(), span().begin());

  // Bit NULL jest per WPIS deskryptora, a zgodnosc ukladow - per SLOT PLASKI (patrz
  // Descriptor::operator==). Dwa zgodne zapisy tego samego rekordu moga miec rozna liczbe
  // wpisow: `INTEGER[3]` niesie JEDEN bit na trzy sloty, trzy pola `INTEGER` - trzy bity.
  // Kopia wprost dalaby wtedy bitset innej dlugosci niz deskryptor tego payloadu, czyli
  // odczyt poza zakresem przy pierwszym getItemVT/setItemVT.
  //
  // Rowna liczba wpisow to przypadek zwykly i idzie ta sama sciezka co dotad.
  if (nullBitset_.size() == other.nullBitset_.size()) {
    nullBitset_ = other.nullBitset_;
  } else {
    retargetNullBitsetFrom(other);
  }
  return *this;
}

// Move assignment operator

/// Przenoszenie trzyma TE SAMA regule zgodnosci deskryptorow co przypisanie kopiujace wyzej -
/// domyslna semantyka przenoszenia (kradziez calego stanu zrodla) zmienilaby zachowanie silnika.
/// Kradzione jest wylacznie przypisanie do celu o PUSTYM deskryptorze; cel, ktory ma juz
/// ksztalt, idzie dokladnie droga kopii, bo w tym przypadku nie ma czego ukrasc:
///
/// - Descriptor::operator== nie jest rownoscia, tylko warunkiem "cel miesci zrodlo" (odrzuca
///   wylacznie slot wezszy albo typ nizszy, patrz descriptor.cc). Zgodny cel moze byc wiec
///   SZERSZY od zrodla. span() liczy dlugosc z deskryptora CELU, nie z rozmiaru wektora, wiec
///   ukradziony (krotszy) bufor dawalby odczyt za koncem alokacji przy pierwszym getItemVT.
///   Do tego bajty celu poza span() zrodla zachowuja przy kopiowaniu swoja dotychczasowa tresc,
///   a kradziez podmienilaby je na tresc zrodla, czyli zmienilaby wartosci pol.
/// - Kradziez warunkowa (tylko przy rownych rozmiarach) jest bezpieczna, ale wymienia memcpy
///   rekordu na zwolnienie bufora celu i przejecie cudzego w kazdym slocie - to nie jest
///   szybsze. Ten sam rachunek dotyczy bitsetu NULL o rownej dlugosci.
/// - Zrodlo zostaje w calosci nietkniete, wiec nie powstaje obiekt czesciowo przeniesiony
///   (deskryptor z wpisami, a bitset pusty), po ktorym getItemVT czytalby poza zakresem.
///
/// noexcept: niezgodny deskryptor celu to zlamany niezmiennik (rdb::fatal), a nie rzut - silnik
/// nie zglasza bledow wyjatkami - wiec nic nie opuszcza tej funkcji inaczej niz powrotem.
/// Przypisanie do celu z ksztaltem nie alokuje (rozmiar bufora i bitsetu juz jest), wiec i
/// bad_alloc stad nie wychodzi. Kontenery biora przenoszenie tylko wtedy, gdy jest noexcept.
payload &payload::operator=(payload &&other) noexcept {
  if (this == &other) return *this;

  // Reguly zgodnosci nie ma tu drugiego raza: cel z ksztaltem obsluguje przypisanie kopiujace,
  // razem z warunkiem zgodnosci i z asercja na niezgodnym deskryptorze.
  if (!descriptor.empty()) return *this = static_cast<const payload &>(other);

  // Cel pusty - ta sama sciezka co operator=(const Descriptor&), tylko bez kopiowania czegokolwiek.
  descriptor   = std::move(other.descriptor);
  payloadData_ = std::move(other.payloadData_);
  nullBitset_  = std::move(other.nullBitset_);
  return *this;
}

/// Przeniesienie znacznikow NULL miedzy zgodnymi zapisami rekordu o roznej liczbie wpisow.
///
/// Slot plaski jest jedyna wspolna miara obu zapisow, wiec przez niego idzie odwzorowanie.
/// Zwijanie N pol skalarnych do jednego `T[N]` musi scalic N bitow w jeden: pole tablicowe
/// nie ma miejsca na wiecej, wiec NULL na dowolnym elemencie oznacza NULL calego pola.
/// W druga strone jeden bit rozklada sie na wszystkie N slotow.
void payload::retargetNullBitsetFrom(const payload &other) {
  nullBitset_.assign(descriptor.size(), false);

  const int slots = descriptor.flatElementCount();
  for (int slot = 0; slot < slots; ++slot) {
    const auto targetPosition = descriptor.flatIndexToDescriptorPosition(slot);
    const auto sourcePosition = other.descriptor.flatIndexToDescriptorPosition(slot);
    RDB_ASSERT(targetPosition.has_value() && sourcePosition.has_value(),
               "payload: flat slot {} missing while retargeting NULL flags", slot);
    if (other.nullBitset_[sourcePosition->first]) nullBitset_[targetPosition->first] = true;
  }
}

// Special init operator

payload &payload::operator=(const Descriptor &other) {
  // TODO: - create assignment operator, cover with test
  // * Plan: when descriptor is empty =Descriptor or =payload
  // * will create descriptor or descriptor with payload
  // * if non empty - this goes strange
  if (descriptor.empty()) {
    // default descriptor constructor (=default) has been used and descriptor is empty and ready to assign.
    descriptor = other;
    payloadData_.assign(other.getSizeInBytes(), 0);
    nullBitset_.assign(descriptor.size(), false);
  } else {
    // compare rlen and rtype only here; descriptor = other; <- would just change field names -
    // descriptor remains the same, payload remains the same.
    RDB_ASSERT(descriptor == other, "payload: descriptor not empty before assign - schema mismatch");
  }
  return *this;
}

// Math operation operators

payload payload::operator+(const payload &other) {
  rdb::Descriptor descSum(descriptor);
  descSum += other.descriptor;          // ! moving this into constructor fails
  descSum.removeConfigurationFields();  //
  payload result(descSum);              //
  std::copy(span().begin(), span().end(), result.span().begin());
  std::copy(other.span().begin(), other.span().end(), result.span().subspan(descriptor.getSizeInBytes()).begin());

  result.nullBitset_.clear();
  result.nullBitset_.reserve(result.descriptor.size());

  auto appendDataFieldNullFlags = [&result](const payload &src) {
    for (size_t i = 0; i < src.descriptor.size(); ++i) {
      auto type = src.descriptor[i].rtype;
      if (type == rdb::TYPE || type == rdb::REF || type == rdb::RETENTION || type == rdb::RETMEMORY) continue;
      result.nullBitset_.push_back(i < src.nullBitset_.size() ? src.nullBitset_[i] : false);
    }
  };

  appendDataFieldNullFlags(*this);
  appendDataFieldNullFlags(other);

  return result;
}

// Member Functions

void payload::setHex(bool hexFormatVal) { hexFormat_ = hexFormatVal; }

const std::vector<bool> &payload::getNullBitset() const { return nullBitset_; }

void payload::setNullBitset(const std::vector<bool> &nullBitset) {
  RDB_ASSERT(nullBitset.size() == descriptor.size(), "payload::setNullBitset: size mismatch: nullBitset={} descriptor={}",
             nullBitset.size(), descriptor.size());
  nullBitset_ = nullBitset;
}

std::span<uint8_t> payload::span() { return {payloadData_.data(), descriptor.getSizeInBytes()}; }

std::span<const uint8_t> payload::span() const { return {payloadData_.data(), descriptor.getSizeInBytes()}; }

template <typename T>
bool payload::setItemBy(const int positionFlat, const std::any &value) {
  const T *data = std::any_cast<T>(&value);  // forma bez rzutu - niezgodny typ obsluguje wolajacy
  if (data == nullptr) return false;
  auto position   = resolveFieldIndexOrAbort(descriptor, positionFlat, "Write");
  auto offsetFlat = descriptor.byteOffsetAtFlatIndex(positionFlat);
  auto dest       = span().subspan(offsetFlat, descriptor[position].rlen);
  std::memcpy(dest.data(), data, descriptor[position].rlen);
  return true;
}

void payload::setItem(const int positionFlat, std::optional<std::any> valueParam) {
  auto position = resolveFieldIndexOrAbort(descriptor, positionFlat, "Write");

  auto requestedType = descriptor[position].rtype;
  std::any value;
  if (!valueParam.has_value()) {
    nullBitset_[position] = true;
    auto fallbackValue    = nullFallbackValue(requestedType);
    std::visit([&value](const auto &v) { value = std::any(v); }, fallbackValue);
  } else {
    nullBitset_[position] = false;
    cast<std::any> castAny;
    value = castAny(valueParam.value(), requestedType);
    // Wartosc bez odpowiednika w typie pola (np. 1e30 do INTEGER) jest NULL-em, nie liczba -
    // patrz narrowFloatTo w convertTypes.cc. Pole NULLTYPE przechowuje monostate jako wartosc.
    if (requestedType != rdb::NULLTYPE && value.type() == typeid(std::monostate)) {
      nullBitset_[position] = true;
      std::visit([&value](const auto &v) { value = std::any(v); }, nullFallbackValue(requestedType));
    }
  }

  auto writeStringField = [&]() -> bool {
    const auto len          = descriptor[position].rlen * descriptor[position].rarray;
    const std::string *data = std::any_cast<std::string>(&value);
    if (data == nullptr) return false;
    auto lenr       = std::min(len, static_cast<int>(data->length()));
    auto destOffset = descriptor.byteOffsetAtFlatIndex(positionFlat);
    auto dest       = span().subspan(destOffset, len);
    RDB_ASSERT(destOffset + len <= descriptor.getSizeInBytes(),
               "payload::writeStringField: destOffset {} + len {} exceeds descriptor size {}", destOffset, len,
               descriptor.getSizeInBytes());
    std::ranges::fill(dest, 0);
    std::copy_n(data->c_str(), lenr, dest.begin());
    return true;
  };

  // Wartosc innego typu niz pole (np. napis bez konwersji do pola liczbowego) jest logowana i
  // pomijana - ta sama decyzja co dawny catch (std::bad_any_cast), tylko bez wyjatku: forma
  // std::any_cast ze wskaznikiem zwraca nullptr zamiast rzucac.
  bool written = true;
  switch (requestedType) {
    case rdb::NULLTYPE:
      break;
    case rdb::STRING:
      written = writeStringField();
      break;
    case rdb::BYTE:
      written = setItemBy<uint8_t>(positionFlat, value);
      break;
    case rdb::INTEGER:
      written = setItemBy<int>(positionFlat, value);
      break;
    case rdb::UINT:
      written = setItemBy<unsigned>(positionFlat, value);
      break;
    case rdb::DOUBLE:
      written = setItemBy<double>(positionFlat, value);
      break;
    case rdb::FLOAT:
      written = setItemBy<float>(positionFlat, value);
      break;
    case rdb::RATIONAL:
      written = setItemBy<boost::rational<int>>(positionFlat, value);
      break;
    case rdb::REF:
    case rdb::TYPE:
    case rdb::RETENTION:
    case rdb::RETMEMORY:
      break;
    default:
      rdb::fatal(fmt::format("payload::setItem: unsupported field type: {}", static_cast<int>(requestedType)));
  }
  if (!written) SPDLOG_ERROR("Error on payload::setItem");
}

template <typename T>
T getVal(std::span<const uint8_t> s, int offset) {
  T val;
  std::memcpy(&val, s.subspan(offset, sizeof(T)).data(), sizeof(T));
  return val;
}

// Uklad pola RATIONAL w rekordzie to para int32 (licznik, mianownik) - format ZEWNETRZNY, opisany w
// dokumentacji i przypiety testem rational_field_layout_is_two_int32_numerator_first. Zapis idzie
// przez setItemBy<boost::rational<int>>, czyli przez memcpy calego obiektu, wiec uklad skladowych
// Boosta JEST tym formatem. Aktualizacja Boosta, ktora go zmieni, ma byc bledem BUDOWY, a nie cicha
// zmiana tego, co lezy na dysku. Sam rozmiar tego nie zlapie: zamiana licznika z mianownikiem
// miejscami rozmiaru nie rusza, wiec druga asercja czyta bajty gotowej wartosci.
static_assert(sizeof(boost::rational<int>) == 2 * sizeof(std::int32_t) && std::is_trivially_copyable_v<boost::rational<int>>,
              "boost::rational<int> nie jest juz trywialnie kopiowalna para int32 - format pola RATIONAL sie zmienil");
static_assert(std::bit_cast<std::array<std::int32_t, 2>>(boost::rational<int>(3, 4)) == std::array<std::int32_t, 2>{3, 4},
              "boost::rational<int> trzyma skladowe w innej kolejnosci - format pola RATIONAL sie zmienil");

/// Bajty pola RATIONAL jako obiekt. Para int32 idzie przez KONSTRUKTOR, czyli przez normalize():
/// memcpy do gotowego obiektu nadpisuje jego reprezentacje z pominieciem niezmiennika klasy, wiec
/// rekord wyzerowany albo uszkodzony dawal 0/0 - stan, ktorego klasa zabrania. Taki ulamek dzielil
/// potem przez zerowy gcd w checkedArith: SIGFPE na x86-64, wyjatek na arm64.
///
/// Poprawny zapis ma zawsze mianownik DODATNI - to niezmiennik boost::rational i zarazem niezmiennik
/// formatu (test rational_field_is_stored_in_normalized_form) - wiec mianownik niedodatni oznacza
/// rekord uszkodzony i czytamy go jako NULL. Warunek obejmuje tez dwa wejscia, na ktorych zalamuje
/// sie sam konstruktor: den == INT_MIN rzuca bad_rational, a para (INT_MIN, -1) dzieli INT_MIN przez
/// -1 w gcd Boosta.
std::optional<boost::rational<int>> readRational(std::span<const uint8_t> s, int offset) {
  const auto raw = getVal<std::array<std::int32_t, 2>>(s, offset);
  if (raw[1] <= 0) return std::nullopt;
  return boost::rational<int>(raw[0], raw[1]);
}

std::optional<std::any> payload::getItem(const int positionFlat) const {
  // Goraca sciezka: zadnej kopii deskryptora -- metody mapowan sa const
  // (leniwy cache w Descriptor jest mutable), wiec czytamy wprost z pola.
  auto position = resolveFieldIndexOrAbort(descriptor, positionFlat, "Read");

  if (nullBitset_[position]) return std::nullopt;

  const auto requestedType = descriptor[position].rtype;
  const auto offsetFlat    = descriptor.byteOffsetAtFlatIndex(positionFlat);
  auto memory              = span();

  auto readStringField = [&]() -> std::string {
    auto len       = descriptor[position].rlen * descriptor[position].rarray;
    auto fieldSpan = memory.subspan(offsetFlat, len);
    auto descLen   = descriptor.getSizeInBytes();
    RDB_ASSERT(offsetFlat + static_cast<size_t>(len) <= descLen,
               "payload::readStringField: field offset {} + len {} exceeds descriptor size {}", offsetFlat, len, descLen);

    for (auto i = 0; i < len; i++) {
      if (fieldSpan[i] == 0) {
        len = i;
        break;
      }
    }
    return {fieldSpan.begin(), fieldSpan.begin() + len};
  };

  switch (requestedType) {
    case rdb::NULLTYPE:
      return std::any(std::monostate{});
    case rdb::STRING:
      return readStringField();
    case rdb::BYTE:
      return getVal<uint8_t>(memory, offsetFlat);
    case rdb::INTEGER:
      return getVal<int>(memory, offsetFlat);
    case rdb::UINT:
      return getVal<uint>(memory, offsetFlat);
    case rdb::DOUBLE:
      return getVal<double>(memory, offsetFlat);
    case rdb::FLOAT:
      return getVal<float>(memory, offsetFlat);
    case rdb::RATIONAL: {
      const auto value = readRational(memory, offsetFlat);
      if (!value.has_value()) return std::nullopt;
      return *value;
    }
    case rdb::REF:
    case rdb::TYPE:
    case rdb::RETENTION:
    case rdb::RETMEMORY:
      SPDLOG_ERROR("Configuration field type not supported in getItem: {}", static_cast<int>(requestedType));
      return std::nullopt;
    case rdb::INTPAIR:
    case rdb::IDXPAIR:
      break;  // nie typ pola rekordu (#267) - do rdb::fatal ponizej, jak kazdy typ spoza listy
  }

  rdb::fatal(fmt::format("payload::getItem: unsupported field type: {}", static_cast<int>(requestedType)));
}

// getItemVT / setItemVT (P1, speed_improvement): rownolegly interfejs wariantowy.
// Czyta/pisze descFldVT wprost do bajtow, bez posrednika std::any -> eliminuje
// konwersje any<->wariant, ktore dominuja czas processRows (cast<std::any>,
// visit_descFld<_,any>, _Manager_internal). Logika bajtowa identyczna z
// getItem/setItem; parytet potwierdzony testem round-trip w test_payload.cpp.
// (E0 addytywne: getItem/setItem nietkniete; unifikacja w E4.)
std::optional<rdb::descFldVT> payload::getItemVT(const int positionFlat) const {
  auto position = resolveFieldIndexOrAbort(descriptor, positionFlat, "Read");

  if (nullBitset_[position]) return std::nullopt;

  const auto requestedType = descriptor[position].rtype;
  const auto offsetFlat    = descriptor.byteOffsetAtFlatIndex(positionFlat);
  auto memory              = span();

  auto readStringField = [&]() -> std::string {
    auto len       = descriptor[position].rlen * descriptor[position].rarray;
    auto fieldSpan = memory.subspan(offsetFlat, len);
    auto descLen   = descriptor.getSizeInBytes();
    RDB_ASSERT(offsetFlat + static_cast<size_t>(len) <= descLen,
               "payload::getItemVT string: field offset {} + len {} exceeds descriptor size {}", offsetFlat, len, descLen);
    for (auto i = 0; i < len; i++) {
      if (fieldSpan[i] == 0) {
        len = i;
        break;
      }
    }
    return {fieldSpan.begin(), fieldSpan.begin() + len};
  };

  switch (requestedType) {
    case rdb::NULLTYPE:
      return rdb::descFldVT{std::monostate{}};
    case rdb::STRING:
      return rdb::descFldVT{readStringField()};
    case rdb::BYTE:
      return rdb::descFldVT{getVal<uint8_t>(memory, offsetFlat)};
    case rdb::INTEGER:
      return rdb::descFldVT{getVal<int>(memory, offsetFlat)};
    case rdb::UINT:
      return rdb::descFldVT{getVal<unsigned>(memory, offsetFlat)};
    case rdb::DOUBLE:
      return rdb::descFldVT{getVal<double>(memory, offsetFlat)};
    case rdb::FLOAT:
      return rdb::descFldVT{getVal<float>(memory, offsetFlat)};
    case rdb::RATIONAL: {
      const auto value = readRational(memory, offsetFlat);
      if (!value.has_value()) return std::nullopt;
      return rdb::descFldVT{*value};
    }
    case rdb::REF:
    case rdb::TYPE:
    case rdb::RETENTION:
    case rdb::RETMEMORY:
      SPDLOG_ERROR("Configuration field type not supported in getItemVT: {}", static_cast<int>(requestedType));
      return std::nullopt;
    case rdb::INTPAIR:
    case rdb::IDXPAIR:
      break;  // nie typ pola rekordu (#267) - do rdb::fatal ponizej, jak kazdy typ spoza listy
  }

  rdb::fatal(fmt::format("payload::getItemVT: unsupported field type: {}", static_cast<int>(requestedType)));
}

std::optional<int> payload::getIntegralItem(const int positionFlat) const {
  const auto position = resolveFieldIndexOrAbort(descriptor, positionFlat, "Read");

  if (nullBitset_[position]) return std::nullopt;

  const auto offsetFlat = descriptor.byteOffsetAtFlatIndex(positionFlat);
  switch (descriptor[position].rtype) {
    case rdb::BYTE:
      return getVal<uint8_t>(span(), offsetFlat);
    case rdb::INTEGER:
      return getVal<int>(span(), offsetFlat);
    default:
      // Wolajacy (indeksy tablic w ewaluatorze) prosi o pole calkowite tylko dla pola, ktore
      // kompilator sprawdzil jako BYTE/INTEGER.
      rdb::fatal(fmt::format("payload::getIntegralItem: field type {} is not BYTE/INTEGER",
                             static_cast<int>(descriptor[position].rtype)));
  }
}

void payload::setItemVT(const int positionFlat, std::optional<rdb::descFldVT> valueParam) {
  auto position = resolveFieldIndexOrAbort(descriptor, positionFlat, "Write");

  const auto requestedType = descriptor[position].rtype;

  rdb::descFldVT value;
  if (!valueParam.has_value()) {
    nullBitset_[position] = true;
    value                 = nullFallbackValue(requestedType);
  } else {
    nullBitset_[position] = false;
    cast<rdb::descFldVT> castVT;
    value = castVT(valueParam.value(), requestedType);  // alternatywa = requestedType albo monostate
    // Wartosc bez odpowiednika w typie pola (np. 1e30 do INTEGER) jest NULL-em, nie liczba -
    // patrz narrowFloatTo w convertTypes.cc. Pole NULLTYPE przechowuje monostate jako wartosc.
    if (requestedType != rdb::NULLTYPE && std::holds_alternative<std::monostate>(value)) {
      nullBitset_[position] = true;
      value                 = nullFallbackValue(requestedType);
    }
  }

  const auto offsetFlat = descriptor.byteOffsetAtFlatIndex(positionFlat);

  auto writeScalar = [&](const auto &data) {
    auto dest = span().subspan(offsetFlat, descriptor[position].rlen);
    std::memcpy(dest.data(), &data, descriptor[position].rlen);
  };

  switch (requestedType) {
    case rdb::NULLTYPE:
      break;
    case rdb::STRING: {
      const auto len  = descriptor[position].rlen * descriptor[position].rarray;
      const auto data = std::get<std::string>(value);
      auto lenr       = std::min(len, static_cast<int>(data.length()));
      auto dest       = span().subspan(offsetFlat, len);
      RDB_ASSERT(offsetFlat + len <= descriptor.getSizeInBytes(),
                 "payload::setItemVT string: destOffset {} + len {} exceeds descriptor size {}", offsetFlat, len,
                 descriptor.getSizeInBytes());
      std::ranges::fill(dest, 0);
      std::copy_n(data.c_str(), lenr, dest.begin());
    } break;
    case rdb::BYTE:
      writeScalar(std::get<uint8_t>(value));
      break;
    case rdb::INTEGER:
      writeScalar(std::get<int>(value));
      break;
    case rdb::UINT:
      writeScalar(std::get<unsigned>(value));
      break;
    case rdb::DOUBLE:
      writeScalar(std::get<double>(value));
      break;
    case rdb::FLOAT:
      writeScalar(std::get<float>(value));
      break;
    case rdb::RATIONAL:
      writeScalar(std::get<boost::rational<int>>(value));
      break;
    case rdb::REF:
    case rdb::TYPE:
    case rdb::RETENTION:
    case rdb::RETMEMORY:
      break;
    default:
      rdb::fatal(fmt::format("payload::setItemVT: unsupported field type: {}", static_cast<int>(requestedType)));
  }
}

// Friend operators

std::istream &operator>>(std::istream &is, payload &rhs) {
  std::string fieldName;
  is >> fieldName;
  if (is.eof()) return is;
  if (rhs.hexFormat_)
    is >> std::hex;
  else
    is >> std::dec;
  Descriptor desc(rhs.descriptor);
  if (!desc.hasField(fieldName)) {
    SPDLOG_ERROR("field {} not found", fieldName);
    return is;
  }

  const auto fieldIndex = desc.fieldIndex(fieldName);

  if (desc.fieldTypeName(fieldName) == "NULL") {
    rhs.setItem(static_cast<int>(fieldIndex), std::any(std::monostate{}));
    return is;
  }

  if (desc.fieldTypeName(fieldName) == "STRING") {
    std::string record;
    is >> record;
    auto fieldLen  = desc.fieldSize(fieldName);
    auto fieldSpan = rhs.span().subspan(desc.fieldByteOffset(fieldName), fieldLen);
    std::ranges::fill(fieldSpan, 0);
    std::copy_n(record.c_str(), std::min((size_t)fieldLen, record.size()), fieldSpan.begin());
    rhs.nullBitset_[fieldIndex] = false;
  } else
    for (auto i = 0; i < desc[desc.fieldIndex(fieldName)].rarray; i++) {
      if (desc.fieldTypeName(fieldName) == "BYTE") {
        int data;
        is >> data;
        auto data8 = static_cast<uint8_t>(data);
        auto dest  = rhs.span().subspan(desc.fieldByteOffset(fieldName) + (i * sizeof(uint8_t)), sizeof(uint8_t));
        std::memcpy(dest.data(), &data8, sizeof(uint8_t));
        rhs.nullBitset_[fieldIndex] = false;
      } else if (desc.fieldTypeName(fieldName) == "UINT")
        copyToMemory<uint>(is, rhs, fieldName, i * static_cast<int>(sizeof(unsigned))), rhs.nullBitset_[fieldIndex] = false;
      else if (desc.fieldTypeName(fieldName) == "INTEGER")
        copyToMemory<int>(is, rhs, fieldName, i * static_cast<int>(sizeof(int))), rhs.nullBitset_[fieldIndex] = false;
      else if (desc.fieldTypeName(fieldName) == "FLOAT")
        copyToMemory<float>(is, rhs, fieldName, i * static_cast<int>(sizeof(float))), rhs.nullBitset_[fieldIndex] = false;
      else if (desc.fieldTypeName(fieldName) == "DOUBLE")
        copyToMemory<double>(is, rhs, fieldName, i * static_cast<int>(sizeof(double))), rhs.nullBitset_[fieldIndex] = false;
      else if (desc.fieldTypeName(fieldName) == "RATIONAL")
        copyToMemory<boost::rational<int>>(is, rhs, fieldName, i * static_cast<int>(sizeof(boost::rational<int>))),
            rhs.nullBitset_[fieldIndex] = false;
      else if (desc.fieldTypeName(fieldName) == "REF")
        SPDLOG_ERROR("REF store not supported by this operator.");
      else if (desc.fieldTypeName(fieldName) == "TYPE")
        SPDLOG_ERROR("TYPE store not supported by this operator.");
      else if (desc.fieldTypeName(fieldName) == "RETENTION")
        SPDLOG_ERROR("RETENTION store not supported by this operator.");
      else if (desc.fieldTypeName(fieldName) == "RETMEMORY")
        SPDLOG_ERROR("RETMEMORY store not supported by this operator.");
      else
        SPDLOG_ERROR("field {} not found", fieldName);
    }
  return is;
}

std::ostream &operator<<(std::ostream &os, const payload &rhs) {
  if (rhs.hexFormat_)
    os << std::hex;
  else
    os << std::dec;
  os << "{";

  Descriptor desc(rhs.descriptor);
  int flatIndex = 0;
  for (size_t idx = 0; idx < rhs.descriptor.size(); ++idx) {
    const auto &r = rhs.descriptor[idx];
    if ((r.rtype == rdb::TYPE) ||       //
        (r.rtype == rdb::REF) ||        //
        (r.rtype == rdb::RETENTION) ||  //
        (r.rtype == rdb::RETMEMORY))    // skip these types
      continue;
    if (!isSingleLineOutput(os))
      os << "\t";
    else
      os << " ";
    os << r.rname;
    os << ":";
    const int flatCountForField = rdb::flatElementCount(r);
    const auto firstValue       = rhs.getItem(flatIndex);
    if (!firstValue.has_value()) {
      os << "null";
      flatIndex += flatCountForField;
    } else if (r.rtype == rdb::STRING || r.rtype == rdb::NULLTYPE) {
      writeValue(os, *firstValue, r.rtype, rhs.hexFormat_);
      flatIndex += flatCountForField;
    } else {
      for (int i = 0; i < flatCountForField; ++i) {
        const auto value = rhs.getItem(flatIndex + i);
        RDB_ASSERT(value.has_value(), "payload: non-null array field returned no value for flat element");
        writeValue(os, *value, r.rtype, rhs.hexFormat_);
        if (i < flatCountForField - 1) os << " ";
      }
      flatIndex += flatCountForField;
    }
    if (!isSingleLineOutput(os)) os << '\n';
  }
  if (rhs.descriptor.empty()) {
    os << "Empty";
    SPDLOG_ERROR("Empty descriptor on payload.");
  }
  if (isSingleLineOutput(os)) os << " ";
  os << "}";
  if (!isSingleLineOutput(os)) os << '\n';
  setSingleLineOutput(os, false);
  return os;
}

}  // namespace rdb
