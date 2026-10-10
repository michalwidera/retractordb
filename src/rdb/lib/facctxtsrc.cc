#include "rdb/facctxtsrc.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cerrno>
#include <charconv>  // from_chars
#include <cstring>   // memcpy
#include <format>
#include <memory>  // make_unique
#include <optional>
#include <ranges>
#include <sstream>
#include <type_traits>  // is_integral_v
#include "fatalError.hpp"
#include "rdb/accessorFactory.hpp"

namespace rdb {

std::optional<std::string> readTokenFromFstream(std::fstream &myFile, bool loopToBeginningIfEOF = true) {
  std::string token;

  auto readToken = [&]() -> bool { return static_cast<bool>(myFile >> token); };

  if (readToken()) return token;

  if (myFile.eof() && loopToBeginningIfEOF) {
    myFile.clear();
    myFile.seekg(0, std::ios::beg);
    if (readToken()) return token;
  }

  return std::nullopt;
}

bool isNullToken(const std::string &token) { return token == "NULL" || token == "Null" || token == "null"; }

// Token liczbowy jest poprawny, gdy zostal odczytany W CALOSCI i miesci sie w zakresie typu pola (#471).
// Token niepoprawny nie ma wartosci - wolajacy zapisuje NULL, tak jak przy przepelnieniu w obliczeniach.
// Do 2026-10-10 `abc` dawalo 0, `12x` - 12, `99999999999` w INTEGER - 2147483647, `-5` w UINT -
// 4294967291, a `300` w BYTE - 44.
template <typename T>
std::optional<T> parseAs(const std::string &token) {
  T var{0};
  // Typy calkowite: from_chars nie alokuje i nie buduje strumienia, a zakres sprawdza sam
  // (result_out_of_range, takze ujemna wartosc dla typu bez znaku). Nie przyjmuje wiodacego '+',
  // ktory zrodlo akceptowalo dotad - zdejmujemy go, o ile stoi za nim cyfra (`+-5` jest bledny).
  if constexpr (std::is_integral_v<T>) {
    const char *first = token.data();
    const char *last  = token.data() + token.size();
    if (token.size() > 1 && token[0] == '+' && token[1] >= '0' && token[1] <= '9') ++first;
    const auto [ptr, ec] = std::from_chars(first, last, var);
    if (ec != std::errc{} || ptr != last) return std::nullopt;
  } else {
    // FLOAT/DOUBLE zostaja na strumieniu: from_chars przyjmuje "inf"/"nan", a strumien je odrzuca.
    // failbit niesie tez przekroczenie zakresu (`1e999`), a niedomiar (`1e-999` -> 0) go nie ustawia.
    std::istringstream in(token);
    in >> var;
    if (in.fail() || in.peek() != std::char_traits<char>::eof()) return std::nullopt;
  }
  return var;
}

template <typename T>
bool setParsed(rdb::payload &payload, int index, const std::string &token) {
  const auto value = parseAs<T>(token);
  if (!value.has_value()) return false;
  payload.setItem(index, *value);
  return true;
}

// false = token niepoprawny; pola nie zapisuje, NULL ustawia wolajacy.
bool parseAndSetNumericItem(rdb::payload &payload, int index, rdb::descFld rtype, const std::string &token) {
  switch (rtype) {
    case rdb::INTEGER:
      return setParsed<int>(payload, index, token);
    case rdb::UINT:
      return setParsed<unsigned>(payload, index, token);
    case rdb::FLOAT:
      return setParsed<float>(payload, index, token);
    case rdb::DOUBLE:
      return setParsed<double>(payload, index, token);
    case rdb::BYTE:
      return setParsed<uint8_t>(payload, index, token);
    default:
      FatalError("facctxtsrc: unsupported field type: {}", static_cast<int>(rtype));
  }
}

textSourceRO::textSourceRO(const std::string_view fileName,    //
                           const rdb::Descriptor &descriptor,  //
                           bool loopToBeginningIfEOF)
    : filename_(std::string(fileName)),
      descriptor_(descriptor),
      recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),

      loopToBeginningIfEOF_(loopToBeginningIfEOF) {
  // TEXTFILE czyta wyłącznie plik zwykły (#346), sprawdzany przez stat() PRZED otwarciem - otwarcie FIFO
  // bez pisarza wisi w samym open(). Plik zwykły ma jednego czytelnika deskryptora, więc strumień jest
  // zawsze buforowany; do #346 bufor wyłączano tu dla urządzeń i FIFO, bo czytałby w przód poza krotkę.
  initializationError_ = sourceKindMismatch("TEXTSOURCE", filename_);
  if (initializationError_.empty()) {
    myFile_.open(filename_, std::ios::in);
    if ((myFile_.rdstate() & std::ifstream::failbit) != 0) {
      SPDLOG_WARN("Unable to open text source file: {}", filename_);
      myFile_.clear();
    }
  }

  payload_ = std::make_unique<rdb::payload>(descriptor_);
}

textSourceRO::~textSourceRO() { myFile_.close(); }

auto textSourceRO::name() -> std::string & { return filename_; }

ssize_t textSourceRO::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  auto markAllNullAndZero = [&](ssize_t status) {
    payload_->setNullBitset(std::vector<bool>(descriptor_.size(), true));
    std::fill(payload_->span().begin(), payload_->span().end(), 0);
    if (ptrData != nullptr) {
      std::memcpy(ptrData, payload_->span().data(), descriptor_.getSizeInBytes());
    }
    nullBitset = payload_->getNullBitset();
    readCount_++;
    return status;
  };

  if (position != 0) return markAllNullAndZero(EINVAL);

  if (recordSize_ == 0) return markAllNullAndZero(EINVAL);

  if (!myFile_.is_open()) return markAllNullAndZero(EBADF);

  if (!loopToBeginningIfEOF_) {
    if (myFile_.eof()) {
      // Ostatni rekord skonczyl sie dokladnie na koncu pliku: eofbit stoi juz przy wejsciu
      // do kolejnego odczytu, a wiec danych nie ma i przebieg z --until-eof ma sie zatrzymac.
      exhausted_ = true;
      return markAllNullAndZero(EXIT_SUCCESS);
    }
  }

  if (myFile_.fail()) return markAllNullAndZero(EIO);

  auto i = 0;
  for (const auto &item : descriptor_) {
    if (item.rtype == rdb::NULLTYPE) {
      // NULLTYPE[N] to N slotow, wiec - jak tablica liczbowa - N tokenow wiersza.
      for (auto j = 0; j < rdb::flatElementCount(item); j++) {
        auto token = readTokenFromFstream(myFile_, loopToBeginningIfEOF_);
        // Nieosiagalne (K1, #471): pola NULLTYPE nie da sie zadeklarowac dla zrodla - typu NULL nie ma
        // ani w DESC.g4, ani w DECLARE (fldType.hpp, flatElementCount), wiec deskryptor zrodla
        // zbudowany z planu nigdy go nie niesie (buduja go recznie tylko testy jednostkowe). Zly token
        // w takim polu nie jest wiec anomalia danych, tylko zlamanym niezmiennikiem.
        if (token.has_value() && !isNullToken(*token)) {
          FatalError("facctxtsrc: expected NULL token for NULL field, got: {}", *token);
        }
        payload_->setItem(i + j, std::nullopt);
      }
      i += rdb::flatElementCount(item);
      continue;
    }

    if (item.rlen != 0) {
      if (item.rtype == rdb::STRING) {
        myFile_ >> std::ws;
        // Powrót na początek musi nastąpić PRZED rozpoznaniem cudzysłowu. Inaczej na końcu pliku
        // peek() zwraca EOF, sterowanie wchodzi w ścieżkę tokenu, ta zawija plik i konsumuje jego
        // pierwszy napis, a skan poniżej sięga po następny - pierwszy rekord pliku wypadał wtedy
        // z cyklu przy każdym zawinięciu (dla "aa","bb","cc" ciąg odczytów był aa,bb,cc,bb,cc,...).
        if (myFile_.eof() && loopToBeginningIfEOF_) {
          myFile_.clear();
          myFile_.seekg(0, std::ios::beg);
          myFile_ >> std::ws;
        }
        auto strLen = item.rlen * item.rarray;

        // Napis BEZ cudzyslowu jest zwyklym tokenem rozdzielanym bialym znakiem - jego wartosc
        // idzie do pola tak samo, jak wartosc pola liczbowego. Do 2026-08-30 przeczytany token
        // byl tu wyrzucany, a sterowanie schodzilo do skanu cudzyslowu ponizej: ten konsumowal
        // reszte pliku, zawijal go i rozjezdzal caly rekord. Dla `DECLARE txt STRING[8], k INTEGER`
        // nad wierszem `42 7` pole txt wychodzilo puste, a k dostawalo 42 - czyli pierwszy token
        // wiersza (pozycja 12 w paper-arXiv/debs/done/requested.md).
        if (myFile_.peek() != '"') {
          auto token = readTokenFromFstream(myFile_, loopToBeginningIfEOF_);
          if (!token.has_value() || isNullToken(*token)) {
            payload_->setItem(i, std::nullopt);
            i++;
            continue;
          }
          auto plain = *token;
          plain.resize(strLen);
          payload_->setItem(i, plain);
          i++;
          continue;
        }

        std::string var;
        char c;
        bool found = false;
        while (myFile_.get(c) && c != '"')
          ;
        if (myFile_.eof() && loopToBeginningIfEOF_) {
          myFile_.clear();
          myFile_.seekg(0, std::ios::beg);
          while (myFile_.get(c) && c != '"')
            ;
        }
        if (!myFile_.eof()) {
          while (myFile_.get(c) && c != '"')
            var += c;
          found = true;
        }
        if (!found && loopToBeginningIfEOF_) {
          myFile_.clear();
          myFile_.seekg(0, std::ios::beg);
          while (myFile_.get(c) && c != '"')
            ;
          while (myFile_.get(c) && c != '"')
            var += c;
        }
        var.erase(std::ranges::remove(var, '"').begin(), var.end());
        var.resize(strLen);
        payload_->setItem(i, var);
      } else {
        bool anyNull = false;
        for (auto j = 0; j < item.rarray; j++) {
          auto token = readTokenFromFstream(myFile_, loopToBeginningIfEOF_);
          if (!token.has_value() || isNullToken(*token)) {
            anyNull = true;
            continue;
          }
          if (!parseAndSetNumericItem(*payload_, i + j, item.rtype, *token)) {
            // Zrodlo zawijane w petli czyta te same wiersze bez konca - jedno ostrzezenie na zrodlo.
            if (!badTokenReported_) {
              badTokenReported_ = true;
              SPDLOG_WARN(
                  "Text source {}: record {}, field {}{}: invalid numeric token '{}' read as NULL; further invalid tokens "
                  "in this source are not reported",
                  filename_, readCount_, item.rname, item.rarray > 1 ? std::format("[{}]", j) : std::string{}, *token);
            }
            anyNull = true;
          }
        }
        // Bit NULL jest jeden na wpis deskryptora, a zapis wartosci elementu go kasuje. Dopoki element
        // NULL byl zapisywany w petli, `NULL 2 3` dawalo pole okreslone z zerem w a[0], a `2 3 NULL` -
        // pole NULL. NULL na dowolnym elemencie oznacza NULL calego pola (payload::retargetNullBitsetFrom),
        // wiec pole NULL zapisujemy dopiero po wszystkich elementach.
        if (anyNull)
          for (auto j = 0; j < item.rarray; j++)
            payload_->setItem(i + j, std::nullopt);
      }

      // rdb::RATIONAL - deprecate ?
      // STRING zajmuje jedną pozycję płaską, a tablica liczbowa po jednej pozycji na element. Stałe
      // i++ ustawiało kolejne pole na pozycji wewnątrz poprzedniej tablicy: przy DECLARE a INTEGER[3],
      // b INTEGER wartość b lądowała w a[1], a własne pole b zostawało niezapisane.
      i += rdb::flatElementCount(item);
    }
  }

  // Plik zakonczony znakiem nowej linii nie ustawia eofbit na ostatnim tokenie, wiec bramka
  // na wejsciu do read() go nie zlapie - konczy sie dopiero ekstrakcja tokenu w TYM rekordzie.
  // failbit po petli oznacza rekord skladany z brakujacych tokenow, czyli pierwszy rekord za
  // koncem wejscia. Rekord kompletny zostawia czysty failbit, nawet jesli eofbit juz stoi.
  if (!loopToBeginningIfEOF_ && myFile_.fail()) exhausted_ = true;

  std::memcpy(ptrData, payload_->span().data(), descriptor_.getSizeInBytes());

  nullBitset = payload_->getNullBitset();
  readCount_++;

  return EXIT_SUCCESS;
}

size_t textSourceRO::count() { return readCount_; }

const std::vector<bool> &textSourceRO::lastNullBitset() const { return payload_->getNullBitset(); }

}  // namespace rdb
