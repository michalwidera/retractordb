#include ".antlr/DESCParser.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>

#include ".antlr/DESCBaseListener.h"
#include ".antlr/DESCLexer.h"
#include "antlr4-runtime/antlr4-runtime.h"
#include "rdb/descriptor.hpp"

using namespace antlrcpp;
using namespace antlr4;

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

 public:
  ParserDESCListener(rdb::Descriptor &desc) : desc(desc) {};

  void enterDesc(DESCParser::DescContext *ctx) override {
    // std::cerr << "enterDesc" << std::endl;
  }

  void exitDesc(DESCParser::DescContext *ctx) override {
    // std::cerr << "exitDesc:" << desc << std::endl;
  }

  void exitByteID(DESCParser::ByteIDContext *ctx) override {
    int count = 1;
    if (ctx->arr != nullptr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(uint8_t), count, rdb::BYTE)});
  }

  void exitIntegerID(DESCParser::IntegerIDContext *ctx) override {
    int count = 1;
    if (ctx->arr != nullptr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(int), count, rdb::INTEGER)});
  }

  void exitUnsignedID(DESCParser::UnsignedIDContext *ctx) override {
    int count = 1;
    if (ctx->arr != nullptr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(unsigned), count, rdb::UINT)});
  }

  void exitFloatID(DESCParser::FloatIDContext *ctx) override {
    int count = 1;
    if (ctx->arr != nullptr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(float), count, rdb::FLOAT)});
  }

  void exitDoubleID(DESCParser::DoubleIDContext *ctx) override {
    int count = 1;
    if (ctx->arr != nullptr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(double), count, rdb::DOUBLE)});
  }

  void exitRationalID(DESCParser::RationalIDContext *ctx) override {
    int count = 1;
    if (ctx->arr != nullptr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(boost::rational<int>), count, rdb::RATIONAL)});
  }

  void exitStringID(DESCParser::StringIDContext *ctx) override {
    int count = std::stoi(ctx->strsize->getText());
    desc.append({rdb::rField(ctx->name->getText(), sizeof(char), count, rdb::STRING)});
  }

  void exitRefID(DESCParser::RefIDContext *ctx) override { desc.append({rdb::rField(ctx->file->getText(), 0, 0, rdb::REF)}); }

  void exitTypeID(DESCParser::TypeIDContext *ctx) override { desc.append({rdb::rField(ctx->type->getText(), 0, 0, rdb::TYPE)}); }

  void exitRetentionID(DESCParser::RetentionIDContext *ctx) override {
    // retention {capacity} !{segments} <- in grammar.
    desc.append({rdb::rField("", std::stoi(ctx->segment->getText()), std::stoi(ctx->capacity->getText()), rdb::RETENTION)});
  }

  void exitRetentionMemoryID(DESCParser::RetentionMemoryIDContext *ctx) override {
    desc.append({rdb::rField("", std::stoi(ctx->capacity->getText()), 0, rdb::RETMEMORY)});
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
    return e.message;
  }
  return "OK";
}
