#include ".antlr/DESCParser.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

#include ".antlr/DESCBaseListener.h"
#include ".antlr/DESCLexer.h"
#include "antlr4-runtime/antlr4-runtime.h"
#include "rdb/descriptor.hpp"
#include "rdb/sizeLimits.hpp"

using namespace antlrcpp;
using namespace antlr4;

namespace {
/// Wartosc tokenu DECIMAL albo nullopt, gdy nie miesci sie w int.
///
/// std::from_chars, a nie std::stoi: metody exit* listenera biegna z noexcept-owego destruktora
/// antlrcpp::FinalAction, wiec std::out_of_range ze std::stoi konczyl proces przez std::terminate -
/// takze proces serwera, ktory czyta .desc z katalogu magazynu (A2 M11; to samo w RQL: #306).
/// Wariant calkowity from_chars nie ma ograniczen dostepnosci libc++, ktore opisuje parseLiteral
/// w RQLParser.cpp - te dotycza wylacznie zmiennoprzecinkowego.
std::optional<int> decimalLiteral(const std::string &text) {
  int value{0};
  const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || end != text.data() + text.size()) return std::nullopt;
  return value;
}
}  // namespace

// https://stackoverflow.com/questions/44515370/how-to-override-error-reporting-in-c-target-of-antlr4

namespace {

/// Blad skladni deskryptora: przerywa parsowanie, zamiast konczyc proces.
///
/// Do fazy 1 oba listenery bledow wolaly exit(EPERM). Wystarczyl jeden uszkodzony plik
/// .desc, zeby zabic dowolny proces, ktory go otworzyl - lacznie z jadrem notatnika,
/// gdzie znika cala sesja uzytkownika.
///
/// Sam powrot z listenera nie zalatwia sprawy: ANTLR wchodzi wtedy w odzyskiwanie i wola
/// dalej callbacki ParserListenera na kalekich kontekstach, gdzie np. ctx->name jest
/// nullem. Rzut wychodzi z desc() przez generowany kod, bo ten lapie wylacznie
/// RecognitionException - dlatego ten typ NIE dziedziczy po niej.
struct DESCSyntaxError {
  std::string message;
};

/// Gorne ograniczenie dlugosci komunikatu. Ta sama wartosc i ten sam powod co w
/// RQLParser.cpp: komunikat potrafi trafic do odpowiedzi serwera w segmencie 64 kB,
/// a lista `expecting {...}` przy bledzie na poczatku deskryptora wylicza dziesiatki
/// tokenow.
constexpr size_t kMaxSyntaxErrorMessage = 300;

/// Zdejmuje ParserListenera i rzuca DESCSyntaxError.
///
/// Zdjecie listenera jest warunkiem KONIECZNYM, nie porzadkami: rozwijanie stosu
/// przechodzi przez `finally` generowanego kodu (antlrcpp::FinalAction), ktore wola
/// exitRule(), a to wola exitIntegerID()/exitStringID()/... na kontekscie zatrzymanym w
/// polowie budowy - czyli std::stoi() na ctx->arr rownym nullptr. Na sciezce RQL pierwsza
/// wersja tej samej naprawy padala tam segfaultem; patrz abortParse() w RQLParser.cpp.
/// Po removeParseListeners() petla triggerExitRuleEvent() chodzi po pustej liscie.
///
/// Listener bledow leksera dostaje ten sam parser, bo blad leksera rozwija stos przez
/// dokladnie te same `finally` - token pobiera sie w srodku reguly parsera.
[[noreturn]] void abortParse(antlr4::Parser &parser, size_t line, size_t charPositionInLine, const std::string &msg,
                             Token *offendingSymbol) {
  // Tekst obrazajacego tokenu, a nie jego adres. Listener leksera podaje tu nullptr, bo
  // blad powstaje, zanim token zostanie zbudowany. Poprzednia wersja wypisywala tu goly
  // wskaznik.
  const std::string offendingText = (offendingSymbol != nullptr) ? offendingSymbol->getText() : std::string("<unknown>");

  // Komunikat MUSI byc jednowierszowy - wraca wartoscia funkcji i bywa przekazywany dalej
  // jako tekst wyjatku.
  std::string message = "line " + std::to_string(line) + ":" + std::to_string(charPositionInLine) + " " + msg;
  std::ranges::replace_if(message, [](char c) { return c == '\n' || c == '\r'; }, ' ');
  if (message.size() > kMaxSyntaxErrorMessage) message.resize(kMaxSyntaxErrorMessage);

  // Wydruk na stderr ZOSTAJE obok statusu: w trybie uslugowym stderr to journald, czyli
  // jedyny slad po stronie serwera.
  std::cerr << "Syntax error @Descriptor" << '\n';
  std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingText << '\n';
  std::cerr << "msg:" << msg << '\n';

  parser.removeParseListeners();
  throw DESCSyntaxError{std::move(message)};
}

}  // namespace

class LexerErrorListenerDesc : public BaseErrorListener {
 public:
  explicit LexerErrorListenerDesc(antlr4::Parser &parser) : parser_(parser) {}
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    abortParse(parser_, line, charPositionInLine, msg, offendingSymbol);
  }

 private:
  antlr4::Parser &parser_;
};

class ParserErrorListenerDesc : public BaseErrorListener {
 public:
  explicit ParserErrorListenerDesc(antlr4::Parser &parser) : parser_(parser) {}
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    abortParse(parser_, line, charPositionInLine, msg, offendingSymbol);
  }

 private:
  antlr4::Parser &parser_;
};

class ParserDESCListener : public DESCBaseListener {
  rdb::Descriptor &desc;

  /// Suma bajtow pol danych. Limit rozmiaru pola nie ogranicza ich liczby, a Descriptor liczy offsety
  /// w int - kilka tysiecy pol `DOUBLE a[65536]` przepelnialo go po cichu.
  std::int64_t recordBytes_{0};

  /// Pierwszy blad wartosci w deskryptorze, lokalny dla jednego parsowania.
  std::string error_;

  void reportError(const std::string &message) {
    std::cerr << "Error @Descriptor: " << message << '\n';
    if (error_.empty()) error_ = message;
  }

  /// Liczba spoza int - blad deskryptora i wartosc zastepcza 1.
  int number(const antlr4::Token *token) {
    const std::string text = token->getText();
    if (const auto value = decimalLiteral(text)) return *value;
    reportError("numeric literal " + text + " is out of range");
    return 1;
  }

  /// Rozmiar pola `TYP a[N]` / `STRING a[N]` - ta sama granica co w RQL (rdb/sizeLimits.hpp).
  /// Przy bledzie wartosc zastepcza 1: deskryptor i tak jest odrzucany w calosci, a pole z absurdalnym
  /// rozmiarem nie powinno powstac nawet na chwile.
  int fieldSize(const antlr4::Token *token) {
    if (token == nullptr) return 1;
    const std::string text = token->getText();
    const auto value       = decimalLiteral(text);
    if (!value) {
      reportError("numeric literal " + text + " is out of range");
      return 1;
    }
    if (*value < 1) {
      reportError("field size " + text + " must be greater than zero");
      return 1;
    }
    if (*value > rdb::limits::kMaxFieldLength) {
      reportError("field size " + text + " exceeds the limit " + std::to_string(rdb::limits::kMaxFieldLength));
      return 1;
    }
    return *value;
  }

  void appendField(const std::string &name, int length, int count, rdb::descFld type) {
    const bool fitted = recordBytes_ <= std::numeric_limits<int>::max();
    recordBytes_ += std::int64_t{length} * count;
    if (fitted && recordBytes_ > std::numeric_limits<int>::max())
      reportError("record of " + std::to_string(recordBytes_) + " bytes exceeds the descriptor limit " +
                  std::to_string(std::numeric_limits<int>::max()));
    desc.append({rdb::rField(name, length, count, type)});
  }

 public:
  ParserDESCListener(rdb::Descriptor &desc) : desc(desc) {};

  [[nodiscard]] const std::string &error() const { return error_; }

  void enterDesc(DESCParser::DescContext *ctx) override {
    // std::cerr << "enterDesc" << std::endl;
  }

  void exitDesc(DESCParser::DescContext *ctx) override {
    // std::cerr << "exitDesc:" << desc << std::endl;
  }

  void exitByteID(DESCParser::ByteIDContext *ctx) override {
    const int count = fieldSize(ctx->arr);

    appendField(ctx->name->getText(), sizeof(uint8_t), count, rdb::BYTE);
  }

  void exitIntegerID(DESCParser::IntegerIDContext *ctx) override {
    const int count = fieldSize(ctx->arr);

    appendField(ctx->name->getText(), sizeof(int), count, rdb::INTEGER);
  }

  void exitUnsignedID(DESCParser::UnsignedIDContext *ctx) override {
    const int count = fieldSize(ctx->arr);

    appendField(ctx->name->getText(), sizeof(unsigned), count, rdb::UINT);
  }

  void exitFloatID(DESCParser::FloatIDContext *ctx) override {
    const int count = fieldSize(ctx->arr);

    appendField(ctx->name->getText(), sizeof(float), count, rdb::FLOAT);
  }

  void exitDoubleID(DESCParser::DoubleIDContext *ctx) override {
    const int count = fieldSize(ctx->arr);

    appendField(ctx->name->getText(), sizeof(double), count, rdb::DOUBLE);
  }

  void exitRationalID(DESCParser::RationalIDContext *ctx) override {
    const int count = fieldSize(ctx->arr);

    appendField(ctx->name->getText(), sizeof(boost::rational<int>), count, rdb::RATIONAL);
  }

  void exitStringID(DESCParser::StringIDContext *ctx) override {
    const int count = fieldSize(ctx->strsize);
    appendField(ctx->name->getText(), sizeof(char), count, rdb::STRING);
  }

  void exitRefID(DESCParser::RefIDContext *ctx) override { desc.append({rdb::rField(ctx->file->getText(), 0, 0, rdb::REF)}); }

  void exitTypeID(DESCParser::TypeIDContext *ctx) override { desc.append({rdb::rField(ctx->type->getText(), 0, 0, rdb::TYPE)}); }

  /// Pojemnosc 0 odrzucamy jak w RQL (#308): groupFile konczyl na niej proces przy pierwszym zapisie.
  /// Segmenty 0 zostaja legalne - znacza "bez limitu segmentow".
  void exitRetentionID(DESCParser::RetentionIDContext *ctx) override {
    // retention {capacity} !{segments} <- in grammar.
    const int capacity = number(ctx->capacity);
    if (capacity == 0) reportError("RETENTION capacity 0 must be greater than zero");
    desc.append({rdb::rField("", number(ctx->segment), capacity, rdb::RETENTION)});
  }

  /// RETMEMORY 0 to pierscien memoryFile bez granicy: kazdy zapis dokladal rekord. Zapis deskryptora
  /// pomija wartosc 0, wiec w pliku moze sie ona znalezc tylko recznie.
  void exitRetentionMemoryID(DESCParser::RetentionMemoryIDContext *ctx) override {
    const int capacity = number(ctx->capacity);
    if (capacity == 0) reportError("RETMEMORY capacity 0 must be greater than zero");
    desc.append({rdb::rField("", capacity, 0, rdb::RETMEMORY)});
  }
};

/// Sparsuj tekstowy deskryptor. Zwraca "OK" albo jednowierszowy komunikat bledu.
///
/// Status jest WARTOSCIA ZWRACANA, nie zmienna globalna. Do fazy 1 stan parsera trzymal
/// `statusDesc` o zasiegu zewnetrznym, ktory nikt nie zerowal przy wejsciu. Bylo to
/// nieszkodliwe WYLACZNIE dlatego, ze listener konczyl proces przez exit(EPERM), wiec
/// drugiego parsowania po bledzie nigdy nie bylo. W chwili, w ktorej exit znika, taki
/// globalny status robi sie lepki i kazde kolejne parsowanie w tym procesie widzi "Fail"
/// po cudzym bledzie. Dokladnie ta pulapka wywrocila wczesniej strone RQL; pinuje ja
/// TEST(descriptor, parse_failure_does_not_poison_the_next_parse).
///
/// @param desc deskryptor zapisywany przez listenera. Przy bledzie moze zostac czesciowo
///        uzupelniony - wolajacy ma go wtedy odrzucic, i tak robi operator>>.
std::string parserDESCString(rdb::Descriptor &desc, const std::string_view inlet) {
  ANTLRInputStream input(inlet);
  // Create a lexer which scans the input stream
  // to create a token stream.
  DESCLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  // Create a parser which parses the token stream
  // to create a parse tree.
  DESCParser parser(&tokens);
  // Oba listenery bledow potrzebuja parsera (abortParse), wiec powstaja PO nim - i przed
  // nim sa niszczone, czyli w chwili, gdy nikt juz do nich nie siega.
  LexerErrorListenerDesc lexerErrorListener(parser);
  lexer.removeErrorListeners();
  lexer.addErrorListener(&lexerErrorListener);
  ParserErrorListenerDesc parserErrorListener(parser);
  ParserDESCListener parserDescListener(desc);
  parser.removeParseListeners();
  parser.removeErrorListeners();
  parser.addErrorListener(&parserErrorListener);
  parser.addParseListener(&parserDescListener);

  // Powod wczesnego powrotu, a nie ogladania drzewa po bledzie: patrz DESCSyntaxError.
  try {
    (void)parser.desc();
  } catch (const DESCSyntaxError &e) {
    return "Fail: " + e.message;
  }
  // Blad wartosci idzie tym samym kanalem co wynik - operator>> zglasza go failbitem.
  if (!parserDescListener.error().empty()) return parserDescListener.error();
  return "OK";
}
