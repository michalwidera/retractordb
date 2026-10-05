#include "RQLParser.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>

// please note that the order of includes is important here

#include ".antlr/RQLBaseListener.h"
#include ".antlr/RQLLexer.h"
#include ".antlr/RQLParser.h"
#include "antlr4-runtime/antlr4-runtime.h"
#include "constants.hpp"
#include "exprSimplify.hpp"
#include "fatalError.hpp"
#include "qTree.hpp"
#include "rdb/accessorFactory.hpp"
#include "rdb/convertTypes.hpp"
#include "rdb/sizeLimits.hpp"
#include "rqlFunctions.hpp"

using namespace antlrcpp;
using namespace antlr4;

namespace {
constexpr size_t kAgseWindowSignChildIndex = 5;

/// Blad skladni RQL: przerywa parsowanie, zamiast konczyc proces.
///
/// Do 2026-09-05 oba listenery bledow wolaly exit(EPERM). W procesie serwera oznaczalo to
/// smierc xretractora przy KAZDYM blednym zapytaniu ad-hoc - `xqry -a "ml"` wystarczalo.
///
/// Sam powrot z listenera nie zalatwia sprawy: ANTLR wchodzi wtedy w odzyskiwanie i wola
/// dalej callbacki ParserListenera na kalekich kontekstach, gdzie np. ctx->ID() jest nullem.
/// Rzut wychodzi z prog() przez generowany kod, bo ten lapie wylacznie RecognitionException.
struct RQLSyntaxError {
  std::string message;
};

/// Gorne ograniczenie dlugosci komunikatu wracajacego do klienta.
///
/// Komunikat idzie do odpowiedzi serwera, a ta do slotu o stalym rozmiarze
/// (ipc::kResponseSlotDataSize). Lista `expecting {...}` przy blednym poczatku instrukcji
/// wylicza kilkadziesiat tokenow; odpowiedz dluzsza od slotu klient dostaje jako blad
/// "response too large" zamiast diagnostyki.
constexpr size_t kMaxSyntaxErrorMessage = 300;

/// Nazwa agregatu zlozona do malych liter. Lekser dopuszcza dwie pisownie ('MIN'|'min'),
/// wiec ASCII wystarcza.
std::string lowercased(std::string text) {
  std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

/// Zamrozona regula przestarzalego `DECLARE ... FILE` (#346). To ta sama regula, ktora do tej
/// zmiany wybierala akcesor w query::descriptorStorage(), rozszerzona wylacznie o rozdzial
/// pliku binarnego i urzadzenia. Wiersze po kolei: `.txt` w DOWOLNYM miejscu sciezki i bez
/// wzgledu na wielkosc liter - TEXTFILE; poczatek `/dev/` - DEVICE; kazda inna - BINFILE.
///
/// NIE ZMIENIAC. Seria pomiarowa artykulu i korpus H9 uruchamiaja te same teksty planow po obu
/// stronach tej granicy, a regula daje dla nich dokladnie dawne zachowanie. Nowe plany pisza
/// jawne slowo; usuniecie tej formy to #349.
sourceKind resolveDeprecatedFile(const std::string &path) {
  if (lowercased(path).find(".txt") != std::string::npos) return sourceKind::textFile;
  if (path.starts_with("/dev/")) return sourceKind::device;
  return sourceKind::binFile;
}

/// Zdejmuje ParserListenera i rzuca RQLSyntaxError.
///
/// Zdjecie listenera jest warunkiem KONIECZNYM, nie porzadkami: samo rozwijanie stosu
/// przechodzi przez `finally` generowanego kodu (antlrcpp::FinalAction), ktore wola
/// exitRule(), a to wola exitDeclare()/exitSelect()/... na kontekscie zatrzymanym w polowie
/// budowy. Pierwsza wersja tej naprawy padala tam w exitDeclare() na `ctx->ID()` rownym
/// nullptr - czyli segfaultem zamiast exit(EPERM), bez zadnej poprawy.
/// Po removeParseListeners() petla triggerExitRuleEvent() chodzi po pustej liscie.
///
/// Listener bledow leksera dostaje ten sam parser, bo blad leksera rozwija stos przez
/// dokladnie te same `finally` - token pobiera sie w srodku reguly parsera.
[[noreturn]] void abortParse(antlr4::Parser &parser, size_t firstLine, size_t line, size_t charPositionInLine,
                             const std::string &msg, Token *offendingSymbol, std::string_view sourceFile) {
  // Lekser i parser licza wiersze wewnatrz PRZEKAZANEGO tekstu, a ten bywa pojedyncza
  // instrukcja wyjeta z pliku planu przez readLogicalLines. firstLine przesuwa numer z
  // powrotem na wiersz pliku - bez tego kazda odmowa wskazywala wiersz 1, niezaleznie od
  // tego, w ktorym miejscu planu stoi blad.
  const size_t sourceLine = firstLine + line - 1;

  // Tekst obrazajacego tokenu, a nie jego adres. Listener leksera podaje tu nullptr, bo blad
  // powstaje, zanim token zostanie zbudowany.
  const std::string offendingText = (offendingSymbol != nullptr) ? offendingSymbol->getText() : std::string("<unknown>");

  // Komunikat MUSI byc jednowierszowy: wraca do klienta jako wartosc ptree w formacie `info`,
  // ktory znaki nowej linii escape'uje - wielowierszowiec dojechalby jako jeden ciag z
  // widocznymi `\n`.
  std::string message = "line " + std::to_string(sourceLine) + ":" + std::to_string(charPositionInLine) + " " + msg;
  std::ranges::replace_if(message, [](char c) { return c == '\n' || c == '\r'; }, ' ');
  if (message.size() > kMaxSyntaxErrorMessage) message.resize(kMaxSyntaxErrorMessage);

  // Wydruk na stderr ZOSTAJE obok statusu: w trybie uslugowym stderr to journald, czyli
  // jedyny slad po stronie serwera. Tak samo robi sciezka semantyczna (reportSemanticError).
  // Tu idzie msg nieprzyciety - ograniczenie dotyczy wylacznie drogi przez pamiec dzielona.
  std::cerr << "Syntax error @Rql" << '\n';
  std::cerr << "line:" << sourceLine << ":" << charPositionInLine << " at " << offendingText << '\n';
  std::cerr << "msg:" << msg << '\n';
  if (sourceFile.empty())
    SPDLOG_ERROR("Parser: {}", message);
  else
    SPDLOG_ERROR("Parser: {}: {}", sourceFile, message);

  parser.removeParseListeners();
  throw RQLSyntaxError{std::move(message)};
}

/// Wartosc literalu liczbowego albo nullopt, gdy nie miesci sie w typie T.
///
/// Wyjatek NIE MOZE wyjsc z metody exit* listenera: te biegna z destruktora
/// antlrcpp::FinalAction w generowanym parserze, ktory jest noexcept, wiec kazdy rzut konczy
/// sie tam std::terminate. Do 2026-09-25 `xqry -a` z literalem 99999999999 konczyl tak
/// dzialajacy serwer (#306). Tokeny DECIMAL i FLOAT nie maja ograniczenia dlugosci, wiec
/// std::out_of_range jest osiagalny z tekstu RQL; std::invalid_argument - nie, lekser
/// przepuszcza tu wylacznie cyfry i kropke.
///
/// std::sto*, a nie std::from_chars: wariant zmiennoprzecinkowy from_chars pojawil sie w libc++
/// dopiero w LLVM 20 i jest objety adnotacja dostepnosci biblioteki systemowej, a port macOS
/// celuje w 14.4. Przy okazji kazdy literal w zakresie jest przyjmowany dokladnie jak dotad.
template <typename T>
std::optional<T> parseLiteral(const std::string &text) {
  try {
    if constexpr (std::is_same_v<T, float>)
      return std::stof(text);
    else if constexpr (std::is_same_v<T, double>)
      return std::stod(text);
    else
      return std::stoi(text);
  } catch (const std::out_of_range &) {
    return std::nullopt;
  }
}
}  // namespace

// https://stackoverflow.com/questions/44515370/how-to-override-error-reporting-in-c-target-of-antlr4

class LexerErrorListener : public BaseErrorListener {
 public:
  LexerErrorListener(antlr4::Parser &parser, size_t firstLine, std::string_view sourceFile)
      : parser_(parser),
        firstLine_(firstLine),
        sourceFile_(sourceFile) {}
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    abortParse(parser_, firstLine_, line, charPositionInLine, msg, offendingSymbol, sourceFile_);
  }

 private:
  antlr4::Parser &parser_;
  size_t firstLine_;
  std::string_view sourceFile_;
};

class ParserErrorListener : public BaseErrorListener {
 public:
  ParserErrorListener(antlr4::Parser &parser, size_t firstLine, std::string_view sourceFile)
      : parser_(parser),
        firstLine_(firstLine),
        sourceFile_(sourceFile) {}
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    abortParse(parser_, firstLine_, line, charPositionInLine, msg, offendingSymbol, sourceFile_);
  }

 private:
  antlr4::Parser &parser_;
  size_t firstLine_;
  std::string_view sourceFile_;
};

/* Iterator - each new field gets new fieldCount number */
int fieldCount = 0;

class ParserListener : public RQLBaseListener {
  qTree &coreInstance;

  /* Wiersz pliku planu, na ktorym stoi parsowana porcja - jak w ParserErrorListener */
  size_t firstLine_;

  /* Helper variable required for rational numbers processing */
  boost::rational<int> rationalResult;

  /// Wartosc klauzuli TIMEOUT biezacej deklaracji (#347), osobno od rationalResult: ta sama
  /// deklaracja ma dwa rational_se, a exitDeclare czyta rationalResult jako interwal. Puste, gdy
  /// klauzuli nie ma albo literal zostal odrzucony; exitDeclare zuzywa wartosc i ja kasuje.
  std::optional<boost::rational<int>> timeoutResult;

  /* Helper variable required to build query or declaration */
  query qry;

  /* sequence of tokens - same variable for stream and field program*/
  std::list<token> program;

  /* Type of field */
  rdb::descFld fType = rdb::BYTE;

  /* Filed type */
  int fTypeSize = 1;

  /* Type of field - eq.1-atomic, >1 - array */
  int fTypeSizeArray = 1;

  /** Rule command support */
  std::list<token> ruleCondition;
  long int dump_left;
  long int dump_right;
  size_t dump_retention;
  std::string systemCommand;
  rule::actionType actionType;

  void recpToken(command_id id) { program.emplace_back(id); };

  template <typename T>
  void recpToken(command_id id, T arg1) {
    program.push_back(token(id, arg1));
  };

  /// Pierwszy blad semantyczny calego przebiegu - czyli taki, ktorego gramatyka nie lapie,
  /// a ktory mimo to unieważnia zapytanie (regula na nieistniejacym strumieniu, na deklaracji,
  /// powtorzona nazwa reguly). Do 2026-09-05 kazdy z tych przypadkow konczyl sie abort() albo
  /// cisza; w procesie serwera pierwsze znaczylo smierc xretractora z powodu bledu w cudzym
  /// zapytaniu ad-hoc, drugie - odpowiedz "OK" na polecenie, ktore nie zrobilo nic.
  /// Rozstrzyga blad pierwszy: dalsze sa juz tylko jego nastepstwami.
  std::string semanticError_;

  void reportSemanticError(const std::string &message) {
    std::cerr << "Error: " << message << '\n';
    SPDLOG_ERROR("Parser: {}", message);
    if (semanticError_.empty()) semanticError_ = message;
  }

  void reportOutOfRange(const std::string &text) { reportSemanticError("numeric literal " + text + " is out of range"); }

  /// Jedyne wejscie do rationalResult z trzech postaci `rational_se`. Wszyscy czterej odbiorcy
  /// - interwal DECLARE, argument `&` i `%`, cel `-` - wymagaja liczby dodatniej, a gramatyka
  /// dopuszcza zero (`0`, `0.0`, `0/5`). Do 2026-09-26 zero przechodzilo parser: DECLARE padal
  /// na "Circular dependency" albo FatalError w qTree::getAvailableTimeIntervals, `&`/`%`/`-`
  /// na FatalError w kompilatorze - w kanale ad-hoc smierc dzialajacego serwera (#308).
  /// Przy odmowie rationalResult zostaje bez zmian, jak po bledzie z #306.
  void acceptInterval(const boost::rational<int> &value, const std::string &text) {
    if (value == 0) {
      reportSemanticError("interval " + text + " must be greater than zero");
      return;
    }
    rationalResult = value;
  }

  /// Czy ten rational_se jest wartoscia klauzuli TIMEOUT deklaracji, a nie interwalem (#347).
  ///
  /// Listener jest podpiety przez addParseListener, wiec biegnie W TRAKCIE parsowania: przy wyjsciu
  /// z rational_se pole etykiety timeout_value rodzica nie jest jeszcze przypisane. Token TIMEOUT jest
  /// juz natomiast dzieckiem deklaracji, bo gramatyka dopasowuje go przed liczba - a przy rational_se
  /// interwalu, ktory stoi wczesniej, jeszcze go nie ma.
  static bool isTimeoutValue(antlr4::tree::ParseTree *rational) {
    auto *declare = dynamic_cast<RQLParser::DeclareContext *>(rational->parent);
    return declare != nullptr && declare->TIMEOUT() != nullptr;
  }

  /// Rozdzial wartosci rational_se: TIMEOUT dopuszcza zero i idzie do timeoutResult, kazdy inny
  /// odbiorca przez acceptInterval() do rationalResult.
  void acceptRational(antlr4::tree::ParseTree *rational, const boost::rational<int> &value, const std::string &text) {
    if (isTimeoutValue(rational))
      timeoutResult = value;
    else
      acceptInterval(value, text);
  }

  /// Literal liczbowy; spoza zakresu - blad planu i wartosc zastepcza 0.
  ///
  /// Zero jest wypelnieniem, nie wynikiem: plan z bledem semantycznym jest odrzucany w calosci,
  /// ale listener idzie dalej przez kolejne reguly, wiec `program` i reszta stanu musza
  /// zachowac taki ksztalt, jaki mialyby przy poprawnej liczbie.
  template <typename T>
  T literal(const std::string &text) {
    if (const auto value = parseLiteral<T>(text)) return *value;
    reportOutOfRange(text);
    return T{};
  }

  /// Literal wymiaru z przedzialu [min, max]; min to 0 albo 1.
  ///
  /// Gramatyka bierze tu DECIMAL, wiec wartosc ujemna nie istnieje. Zero przechodzilo parser tam,
  /// gdzie nic nie znaczy: krok 0 konczyl proces FatalError-em w kompilatorze, okno 0 przy tworzeniu
  /// magazynu, pojemnosc 0 przy pierwszym zapisie - w kanale ad-hoc i `--reset` smierc serwera (#308).
  /// Gorna granica (rdb/sizeLimits.hpp) zamyka te sama droge od drugiej strony: wartosc, ktora miesci
  /// sie w int, a jest absurdalna jako rozmiar, konczyla sie std::bad_alloc, OOM killerem albo
  /// przepelnieniem int dalej w silniku (A2 M11).
  /// Literal spoza zakresu int ma juz swoj komunikat; zastepcze 0 nie dostaje drugiego.
  int boundedLiteral(const std::string &what, const std::string &text, int min, int max = std::numeric_limits<int>::max()) {
    const auto value = parseLiteral<int>(text);
    if (!value) {
      reportOutOfRange(text);
      return 0;
    }
    if (*value < min)
      reportSemanticError(what + " " + text + " must be greater than zero");
    else if (*value > max)
      reportSemanticError(what + " " + text + " exceeds the limit " + std::to_string(max));
    return *value;
  }

  /// Dopina regule do strumienia wskazanego przez ON. Zwraca pusty napis albo powod odmowy;
  /// przy odmowie plan pozostaje nietkniety, wiec wolajacy odrzuca calosc bez sladu po regule.
  std::string buildRule(const std::string &stream_name, const std::string &rule_name) {
    query *target = nullptr;
    for (auto &i : coreInstance)
      if (i.id == stream_name) {
        target = &i;
        break;
      }

    if (target == nullptr)
      return "Rule '" + rule_name + "' refers to stream '" + stream_name + "', but no such stream is defined";
    if (target->isDeclaration())
      return "Rule '" + rule_name + "' cannot be attached to declaration stream '" + stream_name + "'";
    for (const auto &existing : target->lRules)
      if (existing.name == rule_name) {
        std::string message = "Rule '";
        message += rule_name;
        message += "' is already defined on stream '";
        message += stream_name;
        message += '\'';
        return message;
      }

    rule ruleConstruct(rule_name, ruleCondition);
    switch (actionType) {
      case rule::DUMP:
        // Zakres pusty odrzucamy juz tutaj, bo dalej czeka na niego FatalError w
        // compiler::computeRequiredCapacities() - a w sciezce ad-hoc FatalError to smierc
        // serwera. Rownosc granic nie opisuje zadnego zrzutu, wiec nic sie nie traci.
        if (dump_left >= dump_right)
          return "Rule '" + rule_name + "': dump range [" + std::to_string(dump_left) + ".." + std::to_string(dump_right) +
                 "] is empty, left bound must be less than right bound";
        ruleConstruct.action         = rule::DUMP;
        ruleConstruct.dumpRange      = std::make_pair(dump_left, dump_right);
        ruleConstruct.dump_retention = dump_retention;
        break;
      case rule::SYSTEM:
        ruleConstruct.action        = rule::SYSTEM;
        ruleConstruct.systemCommand = systemCommand;
        break;
      default:
        return "Rule '" + rule_name + "' on stream '" + stream_name + "' has an unknown action";
    }

    target->lRules.push_back(std::move(ruleConstruct));
    return {};
  }

 public:
  ParserListener(qTree &coreInstance, size_t firstLine) : coreInstance(coreInstance), firstLine_(firstLine) {};

  [[nodiscard]] const std::string &semanticError() const { return semanticError_; }

  void enterProg(RQLParser::ProgContext *ctx) override {}

  void exitFieldID(RQLParser::FieldIDContext *ctx) override { recpToken(PUSH_ID3, ctx->getText()); }
  void exitFieldIDUnderline(RQLParser::FieldIDUnderlineContext *ctx) override { recpToken(PUSH_IDX, ctx->getText()); }
  void exitFieldIDColumnName(RQLParser::FieldIDColumnNameContext *ctx) override { recpToken(PUSH_ID1, ctx->getText()); }

  /// `strumien[k]` - indeks jest literalem DECIMAL, rownie nieograniczonym jak kazdy inny.
  /// Do 2026-09-27 kompilator czytal go przez atoi: `core0[4294967296]` liczylo sie po cichu
  /// jako `core0[0]`, a `core0[4294967295]` dawalo PUSH_ID z indeksem -1, ktory ad-hoc
  /// przechodzil z "OK" i konczyl serwer przy pierwszym rekordzie (#306, A2 M10).
  void exitFieldIDTable(RQLParser::FieldIDTableContext *ctx) override {
    if (!parseLiteral<int>(ctx->column_index->getText())) reportOutOfRange(ctx->column_index->getText());
    recpToken(PUSH_ID2, ctx->getText());
  }

  /// `cells[$]`, `cells[23-$]` - indeks z numerem instancji generatora.
  ///
  /// Wystawia DOKLADNIE ten sam token co `cells[3]`: rozny jest wylacznie tekst, ktory
  /// compiler::expandStreamGenerators() zwija do postaci literalowej zanim zobaczy go
  /// ktorykolwiek dalszy przebieg. Osobny opcode byl tu zbedny.
  void exitFieldIDGenerated(RQLParser::FieldIDGeneratedContext *ctx) override { recpToken(PUSH_ID2, ctx->getText()); }

  void exitExpPlus(RQLParser::ExpPlusContext *ctx) override { recpToken(ADD); }
  void exitExpMinus(RQLParser::ExpMinusContext *ctx) override { recpToken(SUBTRACT); }
  void exitExpPow(RQLParser::ExpPowContext *ctx) override { recpToken(POWER); }
  void exitExpMult(RQLParser::ExpMultContext *ctx) override { recpToken(MULTIPLY); }
  void exitExpDiv(RQLParser::ExpDivContext *ctx) override { recpToken(DIVIDE); }
  void exitExpAnd(RQLParser::ExpAndContext *ctx) override { recpToken(AND); }
  void exitExpOr(RQLParser::ExpOrContext *ctx) override { recpToken(OR); }
  void exitExpEq(RQLParser::ExpEqContext *ctx) override { recpToken(CMP_EQUAL); }
  void exitExpNq(RQLParser::ExpNqContext *ctx) override { recpToken(CMP_NOT_EQUAL); }
  void exitExpGr(RQLParser::ExpGrContext *ctx) override { recpToken(CMP_GT); }
  void exitExpLs(RQLParser::ExpLsContext *ctx) override { recpToken(CMP_LT); }
  void exitExpGe(RQLParser::ExpGeContext *ctx) override { recpToken(CMP_GE); }
  void exitExpLe(RQLParser::ExpLeContext *ctx) override { recpToken(CMP_LE); }
  void exitExpNot(RQLParser::ExpNotContext *ctx) override { recpToken(NOT); }

  /// `-a`, `+a`, `~a` nad dowolnym `term` - operand dolozyl juz swoje tokeny, tu dochodzi operator.
  /// `+` jest tozsamoscia i tokenu nie ma. Literal ujemny (`-2`) tu nie trafia, bo jest
  /// prymitywem `ExpDec`/`ExpFloat`.
  void exitUnary_op_expression(RQLParser::Unary_op_expressionContext *ctx) override {
    if (ctx->BIT_NOT() != nullptr)
      recpToken(BIT_NOT);
    else if (ctx->MINUS() != nullptr)
      recpToken(NEGATE);
  }

  /// `$` poza nawiasami kwadratowymi - numer instancji jako wartosc.
  ///
  /// Wartosci jeszcze nie znamy (jest nia numer instancji, ktory powstanie dopiero przy
  /// ekspansji), wiec token jest tymczasowy: expandStreamGenerators() zamienia go na
  /// PUSH_VAL. PUSH_GENIDX, ktory przezyl ten przebieg, jest bledem kompilacji - znaczy
  /// `$` uzyte poza generatorem.
  void exitExpGenIndex(RQLParser::ExpGenIndexContext *ctx) override { recpToken(PUSH_GENIDX); }

  /// Poczatki podprogramow argumentow okna - pozycja w `program` w chwili wejscia w regule.
  ///
  /// Stos, a nie pojedyncza zmienna, bo gramatyka dopuszcza `MIN(MAX(a[0]:2):3)`: znacznik
  /// wewnetrznego okna musi zdjac sie przed zewnetrznym. Samo zagniezdzenie odrzuca dopiero
  /// kompilator, ktory jako jedyny widzi, ze historia zrodla nie zawiera wynikow okna.
  ///
  /// Znaczniki sa wazne w obrebie JEDNEGO wyrazenia - `program` czysci exitExpression().
  std::vector<size_t> windowArgMarks;

  void enterWindow_agg(RQLParser::Window_aggContext *ctx) override { windowArgMarks.push_back(program.size()); }

  /// `MIN(cells[0] : 10)` - agregat okna REKORDOWEGO w liscie SELECT.
  ///
  /// Argument jest WYRAZENIEM, wiec jego tokeny doklada juz podregula `expression_factor`;
  /// ten listener dopisuje operator, ktory niesie DWIE liczby: szerokosc okna i pozycje
  /// pierwszego tokenu argumentu. Bez tej drugiej nie da sie odroznic argumentu od tego, co
  /// stalo w programie wczesniej - `x + MIN(y[0]:2)` i `MIN(x+y[0]:2)` roznia sie wylacznie
  /// nia. compiler::resolveWindowAggregates() zabiera oba i zostawia jeden bezargumentowy
  /// token z indeksem grupy okna.
  ///
  /// Okno jest zawsze PRZESUWNE co rekord - powod przy regule `window_agg` w RQL.g4.
  /// Szerokosc spoza 1..kMaxHistoryReach odrzucamy tutaj (A2 M11): kazdy rekord okna to rekord
  /// historii zrodla, wiec literal w zakresie int, ale absurdalny, stawal sie pojemnoscia bufora.
  void exitWindow_agg(RQLParser::Window_aggContext *ctx) override {
    const int width = boundedLiteral("record window width", ctx->width->getText(), 1, rdb::limits::kMaxHistoryReach);
    if (windowArgMarks.empty()) FatalError("RQLParser::exitWindow_agg: no argument mark for '{}'", ctx->getText());
    const auto argStart = static_cast<int>(windowArgMarks.back());
    windowArgMarks.pop_back();
    const auto shape = std::make_pair(width, argStart);

    const auto name = lowercased(ctx->children[0]->getText());
    if (name == "min")
      recpToken(WINDOW_MIN, shape);
    else if (name == "max")
      recpToken(WINDOW_MAX, shape);
    else if (name == "avg")
      recpToken(WINDOW_AVG, shape);
    else if (name == "sumc")
      recpToken(WINDOW_SUM, shape);
    else
      FatalError("RQLParser::exitWindow_agg: unknown aggregate '{}'", ctx->children[0]->getText());
  }

  void exitExpFloat(RQLParser::ExpFloatContext *ctx) override { recpToken(PUSH_VAL, literal<float>(ctx->getText())); }
  void exitExpDec(RQLParser::ExpDecContext *ctx) override { recpToken(PUSH_VAL, literal<int>(ctx->getText())); }
  void exitExpString(RQLParser::ExpStringContext *ctx) override {
    auto text = ctx->getText();
    // Strip surrounding single quotes
    if (text.size() >= 2) {
      text.erase(text.size() - 1);
      text.erase(0, 1);
    }
    program.emplace_back(PUSH_VAL, rdb::descFldVT(text));
  }
  //  void exitExpRational(RQLParser::ExpRationalContext *ctx) { program.push_back(token(PUSH_VAL, rationalResult)); }

  void exitSExpHash(RQLParser::SExpHashContext *ctx) override { recpToken(STREAM_HASH); }

  void exitSExpAnd(RQLParser::SExpAndContext *ctx) override {
    recpToken(PUSH_VAL, rationalResult);
    recpToken(STREAM_DEHASH_DIV);
  }

  void exitSExpMod(RQLParser::SExpModContext *ctx) override {
    recpToken(PUSH_VAL, rationalResult);
    recpToken(STREAM_DEHASH_MOD);
  }

  void exitStreamMin(RQLParser::StreamMinContext *ctx) override { recpToken(STREAM_MIN); }
  void exitStreamMax(RQLParser::StreamMaxContext *ctx) override { recpToken(STREAM_MAX); }
  void exitStreamAvg(RQLParser::StreamAvgContext *ctx) override { recpToken(STREAM_AVG); }
  void exitStreamSum(RQLParser::StreamSumContext *ctx) override { recpToken(STREAM_SUM); }

  // Notacja przyrostkowa `strumien.avg` jest wygaszana na rzecz AVG(strumien) - patrz
  // exitStream_fn_call(). Ostrzezenie stoi TUTAJ, a nie w exitStreamMin/Max/Avg/Sum,
  // bo reguly `agregator` uzywa takze `term : agregator # ExpAgg`, gdzie `avg` jest
  // odwolaniem do POLA wyniku reduktora, a nie operatorem strumieniowym. Ostrzezenie
  // w tamtym miejscu krzyczaloby na poprawny zapis SELECT-a.
  void exitSExpAgregate_proforma(RQLParser::SExpAgregate_proformaContext *ctx) override {
    auto functionName = ctx->agregator()->getText();
    std::ranges::transform(functionName, functionName.begin(), ::toupper);
    SPDLOG_WARN("RQL: notation '{}' is deprecated; use the functional form {}({})", ctx->getText(), functionName,
                ctx->stream_expression()->getText());
  }

  /// AVG/MIN/MAX/SUMC w postaci funkcyjnej nad WYRAZENIEM strumieniowym.
  ///
  /// Nie wnosi nic do wykonania: dokleja ten sam token reduktora, ktory dokladalaby notacja
  /// przyrostkowa. Roznica jest w zasiegu - postac funkcyjna domyka argument wlasnymi
  /// nawiasami, wiec bierze cale wyrazenie niezaleznie od drabiny priorytetow, podczas gdy
  /// `.agg` siega tylko po operand poziomu postfiksowego. Do 2026-08-29 `.agg` przyjmowalo
  /// wylacznie stream_factor i okno trzeba bylo materializowac osobnym zapytaniem:
  ///
  ///     SELECT * STREAM w FROM sq@(125,1000)
  ///     SELECT * STREAM s FROM w.sumc
  ///
  /// Postac funkcyjna bierze cale stream_expression, wiec ta sama para to jedno zapytanie
  /// `FROM SUMC(sq@(125,1000))`. Program klauzuli FROM wychodzi identyczny po sklejeniu
  /// - [PUSH_STREAM sq, STREAM_AGSE(125,1000), STREAM_SUM] - a rozbija go z powrotem na dwa
  /// wezly compiler::extractIntermediateStreams(). DAG jest ten sam; znika tylko koniecznosc
  /// nazwania okna w RQL.
  void exitStream_fn_call(RQLParser::Stream_fn_callContext *ctx) override {
    if (ctx->MIN() != nullptr)
      recpToken(STREAM_MIN);
    else if (ctx->MAX() != nullptr)
      recpToken(STREAM_MAX);
    else if (ctx->AVG() != nullptr)
      recpToken(STREAM_AVG);
    else if (ctx->SUMC() != nullptr)
      recpToken(STREAM_SUM);
    else
      FatalError("RQLParser::exitStream_fn_call: unknown stream function '{}'", ctx->getText());
  }
  void exitSExpPlus(RQLParser::SExpPlusContext *ctx) override { recpToken(STREAM_ADD); }
  void exitSExpMinus(RQLParser::SExpMinusContext *ctx) override { recpToken(STREAM_SUBTRACT, rationalResult); }

  void exitSExpAgse(RQLParser::SExpAgseContext *ctx) override {
    int window{0};
    int step{0};
    // Minus przed szerokoscia jest legalny (kompilator bierze abs), wiec zerem jest tez `-0`.
    const int windowAbs = boundedLiteral("AGSE window", ctx->window->getText(), 1, rdb::limits::kMaxHistoryReach);
    window              = (ctx->children[kAgseWindowSignChildIndex]->getText() == "-") ? -windowAbs : windowAbs;
    step                = boundedLiteral("AGSE step", ctx->step->getText(), 1, rdb::limits::kMaxHistoryReach);

    program.emplace_back(STREAM_AGSE, std::make_pair(step, window));
  }

  /// Nazwa funkcji jest w gramatyce zwyklym ID, wiec autor moze ja napisac dowolna
  /// wielkoscia liter. Do tokena idzie postac KANONICZNA z rqlFunctions.hpp, a nie ta
  /// napisana w zapytaniu - uzasadnienie przy definicji tabeli.
  ///
  /// Nazwy NIEZNANEJ nie odrzucamy tutaj: `compiler::checkFunctionCalls()` raportuje ja przez
  /// `Check result:` razem z pozostalymi kontrolami planu. Nieznana nazwa jedzie wiec dalej
  /// w postaci doslownej, zeby komunikat pokazal to, co napisal autor.
  void exitFunction_call(RQLParser::Function_callContext *ctx) override {
    const std::string written = ctx->fn->getText();
    const auto known          = rdb::findRqlFunction(written);
    const std::string name    = known ? std::string(known->canonical) : written;

    // Szerokosc `to_string(x : N)` to dlugosc pola STRING, wiec ma granice pola (A2 M11).
    if (ctx->DECIMAL() != nullptr)
      recpToken(CALL2, std::make_pair(
                           name, boundedLiteral(name + " width", ctx->DECIMAL()->getText(), 1, rdb::limits::kMaxFieldLength)));
    else
      recpToken(CALL, name);
  }

  // page 119 - The Definitive ANTL4 Reference Guide
  void exitDeclare(RQLParser::DeclareContext *ctx) override {
    qry.filename = ctx->file_name->getText();
    // This removes ''
    qry.filename.erase(qry.filename.size() - 1);
    qry.filename.erase(0, 1);
    qry.isDeprecatedFile = (ctx->kind->getType() == RQLParser::FILE);
    switch (ctx->kind->getType()) {
      case RQLParser::BINFILE:
        qry.kind = sourceKind::binFile;
        break;
      case RQLParser::TEXTFILE:
        qry.kind = sourceKind::textFile;
        break;
      case RQLParser::DEVICE:
        qry.kind = sourceKind::device;
        break;
      default:
        qry.kind = resolveDeprecatedFile(qry.filename);
    }
    const std::string keyword(qry.isDeprecatedFile ? "FILE" : sourceKindKeyword(qry.kind));
    qry.declarationLine = firstLine_ + ctx->getStart()->getLine() - 1;
    // Blad planu juz tutaj. Bez tego pusta nazwa przechodzila parser i kompilacje, a zatrzymywal
    // ja dopiero FatalError w rdb::StoragePaths przy rejestracji w modelu - w sciezce ad-hoc juz
    // po imporcie do zywego planu, czyli smierc dzialajacego serwera.
    if (qry.filename.empty())
      reportSemanticError(keyword + " of stream " + ctx->ID()->getText() + " requires a non-empty file name");
    qry.id           = ctx->ID()->getText();
    qry.rInterval    = rationalResult;
    qry.isDisposable = (ctx->DISPOSABLE() != nullptr);
    qry.isOneShot    = (ctx->ONESHOT() != nullptr);
    qry.isHold       = (ctx->HOLD() != nullptr);
    // DEVICE to zrodlo zywe (#346): HOLD nie zatrzymuje producenta, tylko gromadzi zaleglosc,
    // a DISPOSABLE kasowalby sciezke urzadzenia albo FIFO. ONESHOT jest dozwolony od #347:
    // wyczerpaniem DEVICE jest pierwszy EOF po otrzymaniu danych (binaryDeviceRO::fill).
    if (qry.kind == sourceKind::device) {
      const std::string what =
          qry.isDeprecatedFile ? "FILE '" + qry.filename + "' resolves as DEVICE, which" : std::string("DEVICE");
      for (const auto &[present, option] : {std::pair{qry.isDisposable, "DISPOSABLE"}, std::pair{qry.isHold, "HOLD"}})
        if (present)
          reportSemanticError("DECLARE " + qry.id + ": " + what + " does not take " + option +
                              "; DISPOSABLE and HOLD apply to BINFILE and TEXTFILE");
    }
    // TIMEOUT (#347) ma sens tylko przy zrodle zywym. Forma przestarzala nie przyjmuje klauzul
    // wcale - takze wtedy, gdy regula ze sciezki wybrala DEVICE - wiec komunikat podpowiada jawne
    // slowo. Wartosc ujemna nie ma znaczenia "czekaj bez konca": takiego terminu nie ma.
    if (ctx->TIMEOUT() != nullptr) {
      const std::string text = ctx->timeout_value->getText();
      // Brak wartosci znaczy, ze przebieg rational_se juz zglosil literal (zakres, zerowy mianownik).
      if (timeoutResult) {
        if (ctx->timeout_sign != nullptr)
          reportSemanticError("DECLARE " + qry.id + ": TIMEOUT -" + text + " must not be negative");
        else if (boost::rational_cast<double>(*timeoutResult) > rdb::limits::kMaxDeviceTimeoutSeconds)
          reportSemanticError("DECLARE " + qry.id + ": TIMEOUT " + text + " exceeds the limit " +
                              std::to_string(static_cast<long long>(rdb::limits::kMaxDeviceTimeoutSeconds)) + " s");
        else
          qry.timeoutSeconds = *timeoutResult;
      }
      if (qry.isDeprecatedFile)
        reportSemanticError("DECLARE " + qry.id + ": deprecated FILE does not take TIMEOUT; declare the source with DEVICE '" +
                            qry.filename + "'");
      else if (qry.kind != sourceKind::device)
        reportSemanticError("DECLARE " + qry.id + ": " + keyword + " does not take TIMEOUT; TIMEOUT applies to DEVICE");
    }
    // Ta sama odmowa co w exitSelect: klient bral kazdy rekord deklaracji o tej nazwie
    // za sygnal zamkniecia serwera i konczyl sie "no data in stream".
    if (qry.id == constants::Reserved_id_oob)
      reportSemanticError(std::string(constants::Reserved_id_oob) + " is reserved stream name");
    coreInstance.push_back(qry);
    qry.reset();
    timeoutResult.reset();
    fieldCount = 0;
  }

  // https://www.programiz.com/cpp-programming/string-float-conversion
  // https://www.geeksforgeeks.org/converting-strings-numbers-cc/

  /// Zakres liczy sie tu wzgledem `rational<int>`, nie `double`. Rationalize() nie odmawia:
  /// wartosc od 2^31 w gore i niezerowa ponizej jej rozdzielczosci (1e-6) oddaje jako 0/1,
  /// wiec `3000000000.0` i `0.00000001` dawaly interwal zerowy, a plan padal dopiero
  /// w kompilatorze na mylacym "Circular dependency in stream definitions". Jawne `0.0` zostaje
  /// poza ta kontrola - to zerowy interwal, a nie literal spoza zakresu; odrzuca je acceptInterval().
  void exitRationalAsFloat(RQLParser::RationalAsFloatContext *ctx) override {
    const std::string text = ctx->FLOAT()->getText();
    const auto value       = parseLiteral<double>(text);
    const auto rational    = value ? Rationalize(*value) : boost::rational<int>{};
    if (!value || (rational == 0 && *value != 0)) {
      reportOutOfRange(text);
      return;
    }
    acceptRational(ctx, rational, text);
  }

  void exitRationalAsDecimal(RQLParser::RationalAsDecimalContext *ctx) override {
    const std::string text = ctx->DECIMAL()->getText();
    const auto value       = parseLiteral<int>(text);
    // Bez wartosci zastepczej z literal(): zastepcze 0 dolozyloby na stderr drugi, falszywy
    // komunikat o zerowym interwale.
    if (!value) {
      reportOutOfRange(text);
      return;
    }
    acceptRational(ctx, *value, text);
  }

  void exitFraction(RQLParser::FractionContext *ctx) override {
    const std::string nomText = ctx->children[0]->getText();
    const std::string denText = ctx->children[2]->getText();
    const auto nom            = parseLiteral<int>(nomText);
    const auto den            = parseLiteral<int>(denText);
    // Bez wartosci zastepczej z literal(): zastepcze 0 w mianowniku dolozyloby na stderr drugi,
    // falszywy komunikat o zerowym mianowniku.
    if (!nom || !den) {
      reportOutOfRange(nom ? denText : nomText);
      return;
    }
    // Blad planu, nie FatalError: `xqry -a` z `1/0` konczyl dzialajacy serwer. rationalResult
    // zostaje bez zmian - plan z bledem semantycznym jest odrzucany w calosci, a konstruktor
    // boost::rational z zerowym mianownikiem rzuca.
    if (*den == 0) {
      reportSemanticError("fraction " + ctx->getText() + " has a zero denominator");
      return;
    }
    // Ulamek stoi pod alternatywa RationalAsFraction_proforma - to ona jest dzieckiem deklaracji.
    acceptRational(ctx->parent, boost::rational<int>(*nom, *den), ctx->getText());
  }

  void exitSelect(RQLParser::SelectContext *ctx) override {
    // Kazda instancja generatora to kopia zapytania i wlasny strumien, wiec rozmiar ma granice planu (A2 M11).
    qry.generatorSize = (ctx->gen_size != nullptr) ? boundedLiteral("stream generator size", ctx->gen_size->getText(), 1,
                                                                    static_cast<int>(rdb::limits::kMaxPlanStreams))
                                                   : query::notAGenerator;

    // this loop creates field names in streamName + "_" + counter++
    //
    // Dla generatora prefiks doklada compiler::expandStreamGenerators(), bo nazwa pola ma
    // pochodzic od nazwy INSTANCJI (`cell$0_0`), a nie od nazwy szablonu (`cell_0`). Tylko
    // wtedy plan z generatora jest nie do odroznienia od recznie rozpisanych SELECT-ow.
    if (qry.generatorSize == query::notAGenerator) {
      for (auto &i : qry.lSchema) {
        if ((i.field_.rname).starts_with("_")) (i.field_.rname) = ctx->stream_name->getText() + i.field_.rname;
      }
    }

    qry.id = ctx->stream_name->getText();

    // Blad planu, nie abort(): `xqry -a` z ta nazwa konczyl dzialajacy serwer SIGABRT-em.
    if (qry.id == constants::Reserved_id_oob)
      reportSemanticError(std::string(constants::Reserved_id_oob) + " is reserved stream name");

    qry.lProgram = program;
    // Domyslnosc jest w planie, bo plik moze byc parsowany po jednej instrukcji.
    // Jawna polityka SELECT wygrywa; DECLARE nie dziedziczy tego ustawienia.
    const bool inheritVolatile = coreInstance.exists(":DEFAULT") && ctx->PERSISTENT() == nullptr && ctx->STORAGE() == nullptr;
    // Samo `RETENTION n` (juz w policy.second) jest rozmiarem pierscienia, jak przy `STORAGE MEMORY`.
    // Do 2026-09-27 VOLATILE nadpisywal je jedynka, wiec RETENTION ginelo bez slowa (decyzja P4).
    if (ctx->VOLATILE() != nullptr || inheritVolatile) {
      qry.policy = std::make_pair("MEMORY", std::max<size_t>(qry.policy.second, 1));
    }

    if (ctx->FILE() != nullptr) {
      qry.filename = ctx->file_name->getText();

      // This removes ''
      qry.filename.erase(qry.filename.size() - 1);
      qry.filename.erase(0, 1);

      // Blad planu, nie FatalError - ta sama przyczyna co w exitCoption.
      if (qry.filename.empty())
        reportSemanticError("FILE of stream " + ctx->stream_name->getText() + " requires a non-empty file name");
    }

    if (ctx->STORAGE() != nullptr) {
      qry.storage_policy = ctx->type_name->getText();
      std::ranges::transform(qry.storage_policy, qry.storage_policy.begin(), ::toupper);  // to upper case
      // Gramatyka przyjmuje tu takze slowa, ktore profilem nie sa (patrz select_statement w RQL.g4) -
      // po to, zeby odmowa nazwala strumien i profile, zamiast bledu skladni o dyrektywie STORAGE.
      // Rozstrzyga typ tokenu, nie tekst: `Direct` jako ID nie moze stac sie profilem przez toupper,
      // bo profile maja tylko dwie pisownie, jak kazde slowo kluczowe.
      const auto tokenType = ctx->type_name->getType();
      if (tokenType != RQLParser::TYPE_PROFILE && tokenType != RQLParser::DEFAULT) {
        const bool sourceKind = tokenType == RQLParser::DEVICE || tokenType == RQLParser::BINFILE ||
                                tokenType == RQLParser::TEXTFILE || qry.storage_policy == "TEXTSOURCE";
        reportSemanticError("STORAGE " + ctx->type_name->getText() + " of stream " + ctx->stream_name->getText() +
                            " is not a storage profile" + (sourceKind ? " but a source kind of DECLARE" : "") +
                            "; use DEFAULT, MEMORY, DIRECT, POSIX, POSIXSHD or GENERIC");
      }
    }
    if (ctx->PERSISTENT() != nullptr && qry.storage_policy == "MEMORY")
      reportSemanticError("PERSISTENT conflicts with STORAGE MEMORY");
    // `STORAGE MEMORY` to pierscien w RAM, jak VOLATILE. Do 2026-09-27 polityka zostawala
    // ("DEFAULT", n): bez RETENTION deskryptor nie dostawal RETMEMORY, wiec memoryFile nie mial granicy
    // i rosl o rekord na takt do wyczerpania pamieci; z `RETENTION n` dostawal TYPE DEFAULT i dane szly
    // na DYSK. Pojemnosc to co najmniej 1 - reszte, jak dla VOLATILE, dobiera kompilator.
    if (qry.storage_policy == "MEMORY" && qry.policy.first != "MEMORY")
      qry.policy = std::make_pair("MEMORY", std::max<size_t>(qry.policy.second, 1));
    // Samo `RETENTION n` to rozmiar pierscienia MEMORY. Magazyn plikowy liczy retencje w segmentach,
    // a do 2026-09-27 zostawala tu polityka ("DEFAULT", n): deskryptor dostawal TYPE DEFAULT zamiast
    // STORAGE z planu i zadnej retencji - plik rosl bez granicy, `STORAGE DIRECT` konczyl jako DEFAULT
    // z .shadow (decyzja D7).
    if (qry.policy.first != "MEMORY" && qry.policy.second != 0) {
      const std::string capacity = std::to_string(qry.policy.second);
      const std::string hint =
          (qry.storage_policy == "DEFAULT" || qry.storage_policy == "DIRECT")
              ? qry.storage_policy + " storage on disk keeps segments: write RETENTION " + capacity + " <segments>"
              : qry.storage_policy + " storage has no retention: use STORAGE DEFAULT or DIRECT with RETENTION " + capacity +
                    " <segments>";
      reportSemanticError("RETENTION " + capacity + " on stream " + qry.id + " sets only the size of a MEMORY ring; " + hint);
    }
    // Lustro powyzszego: segmenty na dysku przy magazynie w pamieci. memoryFile czyta tylko RETMEMORY,
    // wiec `RETENTION c s` bylo tu ignorowane bez slowa (decyzja P4).
    if (qry.policy.first == "MEMORY" && !qry.retention.noRetention()) {
      const std::string capacity = std::to_string(qry.retention.capacity);
      std::string owner          = "STORAGE MEMORY";
      std::string onDisk         = "use STORAGE DEFAULT or DIRECT";
      if (ctx->VOLATILE() != nullptr) {
        owner  = "VOLATILE";
        onDisk = "drop VOLATILE";
      } else if (inheritVolatile) {
        owner  = "DEFAULT VOLATILE";
        onDisk = "add PERSISTENT";
      }
      reportSemanticError("RETENTION " + capacity + " " + std::to_string(qry.retention.segments) + " on stream " + qry.id +
                          " sets segments on disk, but " + owner + " keeps the stream in memory: write RETENTION " + capacity +
                          " for a MEMORY ring, or " + onDisk + " to keep segments on disk");
    }

    coreInstance.push_back(qry);
    program.clear();
    qry.reset();
    qry.storage_policy = "DEFAULT";
    fieldCount         = 0;
  }

  /// Pojemnosc 0 odrzucamy w obu postaciach. Z segmentami padala przy pierwszym zapisie
  /// (groupFile::write), a `RETENTION 0` i `RETENTION 0 0` nie robily nic, bez slowa (#308).
  /// Segmenty 0 zostaja legalne - znacza "bez limitu segmentow".
  void exitRetention(RQLParser::RetentionContext *ctx) override {
    if (ctx->segments != nullptr) {
      // retention {capacity} !{segments}
      qry.retention = std::pair<int, int>(         //
          literal<int>(ctx->segments->getText()),  //
          boundedLiteral("RETENTION capacity", ctx->capacity->getText(), 1));
    } else {
      // retention {capacity} - note: segments is optional but capacity is required
      qry.policy.second = boundedLiteral("RETENTION capacity", ctx->capacity->getText(), 1);
    }
  }

  void exitRulez(RQLParser::RulezContext *ctx) override {
    const std::string stream_name(ctx->stream_name->getText());
    const std::string rule_name(ctx->name->getText());

    if (const std::string error = buildRule(stream_name, rule_name); !error.empty()) reportSemanticError(error);

    program.clear();
    dump_left      = 0;
    dump_right     = 0;
    dump_retention = 0;
    systemCommand.clear();
    ruleCondition.clear();
    actionType = rule::UNKNOWN_ACTION;
    qry.reset();
    fieldCount = 0;
  }

  /// Czy tuz PRZED podana liczba stoi w zapisie znak minus.
  ///
  /// Znak jest w gramatyce osobnym, OPCJONALNYM dzieckiem (`'-'? DECIMAL`), wiec numery pozycji
  /// przesuwaja sie razem z jego obecnoscia: `DUMP -5 TO 5` ma piecioro dzieci, `DUMP 5 TO 5` -
  /// czworo. Odczyt ze stalej pozycji children[4] wychodzil w tym drugim przypadku poza wektor:
  /// w Debug konczylo sie to asercja biblioteki standardowej, w Release odczytem spoza zakresu,
  /// a z kanalu ad-hoc - smiercia serwera po `DO DUMP 5 TO 5`. Dlatego pytamy o sasiada samej
  /// liczby, zamiast liczyc pozycje z gory.
  static bool negatedBefore(RQLParser::DumppartContext *ctx, const antlr4::Token *number) {
    for (size_t i = 1; i < ctx->children.size(); ++i) {
      const auto *terminal = dynamic_cast<antlr4::tree::TerminalNode *>(ctx->children[i]);
      if (terminal == nullptr || terminal->getSymbol() != number) continue;
      return ctx->children[i - 1]->getText() == "-";
    }
    return false;
  }

  /// Granice zakresu siegaja historii strumienia, a RETENTION to liczba jednoczesnych zadan zrzutu,
  /// z ktorych kazde trzyma otwarty deskryptor pliku - stad granice z rdb/sizeLimits.hpp (A2 M11).
  void exitDumppart(RQLParser::DumppartContext *ctx) override {
    actionType = rule::DUMP;
    dump_left  = boundedLiteral("DUMP range bound", ctx->step_back->getText(), 0, rdb::limits::kMaxHistoryReach);
    if (negatedBefore(ctx, ctx->step_back)) dump_left = -dump_left;
    dump_right = boundedLiteral("DUMP range bound", ctx->step_forward->getText(), 0, rdb::limits::kMaxHistoryReach);
    if (negatedBefore(ctx, ctx->step_forward)) dump_right = -dump_right;

    if (ctx->rule_retnetion != nullptr)
      dump_retention = boundedLiteral("DUMP RETENTION", ctx->rule_retnetion->getText(), 0, rdb::limits::kMaxDumpRetention);
    else
      dump_retention = 0;  // Default: no retention
  }

  void exitSystempart(RQLParser::SystempartContext *ctx) override {
    actionType    = rule::SYSTEM;
    systemCommand = ctx->syscmd->getText();
    // This removes ''
    systemCommand.erase(systemCommand.size() - 1);
    systemCommand.erase(0, 1);
  }

  void exitDefaultOption(RQLParser::DefaultOptionContext *ctx) override {
    if (coreInstance.exists(":DEFAULT")) {
      reportSemanticError("DEFAULT VOLATILE may occur only once");
      return;
    }
    if (std::ranges::any_of(coreInstance, [](const query &q) { return !q.isCompilerDirective(); })) {
      reportSemanticError("DEFAULT VOLATILE must precede DECLARE, SELECT and RULE");
      return;
    }
    query option;
    option.id       = ":DEFAULT";
    option.filename = "VOLATILE";
    coreInstance.push_back(option);
  }

  void exitCoption(RQLParser::CoptionContext *ctx) override {
    qry.id = ":" + ctx->directive->getText();
    std::ranges::transform(qry.id, qry.id.begin(), ::toupper);  // to upper case
    qry.filename = ctx->value->getText();

    // This removes ''
    qry.filename.erase(qry.filename.size() - 1);
    qry.filename.erase(0, 1);

    // Blad semantyczny, nie FatalError: tym samym parserem idzie kanal ad-hoc i `xqry --reset`,
    // czyli tekst obcy wykonywany w procesie DZIALAJACEGO serwera. FatalError konczyl tam cala
    // instancje - `xqry -a "STORAGE ''"` wystarczalo. Stan listenera sprzatamy tak samo w obu
    // galeziach, bo parser po bledzie semantycznym idzie dalej przez kolejne instrukcje.
    std::string upperValue(qry.filename);
    std::ranges::transform(upperValue, upperValue.begin(), ::toupper);
    if (qry.filename.empty()) {
      reportSemanticError("directive " + qry.id.substr(1) + " requires a non-empty value");
    } else if (qry.id == ":SUBSTRAT" && !rdb::isWritableType(upperValue)) {
      // Do #346 wartosc szla bez kontroli: `SUBSTRAT 'device'` dawal posrednim wezlom akcesor
      // tylko do odczytu i FatalError przy pierwszym zapisie, `SUBSTRAT 'foo'` - FatalError
      // nieznanego typu przy budowie modelu. `-c` przepuszczal oba. Wielkosc liter jak
      // w compiler::extractIntermediateStreams(), ktory sklada wartosc do wielkich liter.
      reportSemanticError("SUBSTRAT '" + qry.filename +
                          "' is not a storage profile; use DEFAULT, MEMORY, DIRECT, POSIX, POSIXSHD or GENERIC");
    } else {
      // Add / at the end of path, if not present in case of STORAGE
      if (qry.id == ":STORAGE" && qry.filename[qry.filename.size() - 1] != '/') qry.filename.push_back('/');

      coreInstance.push_back(qry);
    }
    program.clear();
    qry.reset();
    fieldCount = 0;
  }

  /// Przesuniecie o N rekordow to N rekordow historii zrodla (A2 M11).
  void exitSExpTimeMove(RQLParser::SExpTimeMoveContext *ctx) override {
    recpToken(STREAM_TIMEMOVE, boundedLiteral("time shift", ctx->DECIMAL()->getText(), 0, rdb::limits::kMaxHistoryReach));
  }

  /// Nazwa strumienia. Pozostale alternatywy `stream_factor` - `( e )` i wywolanie
  /// reduktora - nie wnosza wlasnego tokenu: ich tresc dolozyly juz wezly nizej.
  ///
  /// Rozroznienie idzie po ctx->ID(), a nie po liczbie dzieci: od chwili, gdy prymitywem
  /// stalo sie takze `stream_fn_call`, JEDNO dziecko maja dwie alternatywy, a `MIN(a)`
  /// wchodzilo tedy z ctx->ID() rownym nullptr.
  void exitStream_factor(RQLParser::Stream_factorContext *ctx) override {
    if (ctx->ID() == nullptr) return;
    // `cell[3]` i `cell[$]` musza wejsc z nawiasem: samo ctx->ID() zgubiloby indeks, a to on
    // wskazuje instancje rodziny. Nazwe fizyczna (`cell$3`) podstawia expandStreamGenerators().
    if (ctx->gen_index() != nullptr)
      program.emplace_back(PUSH_STREAM, ctx->getText());
    else
      program.emplace_back(PUSH_STREAM, ctx->ID()->getText());
  }

  void exitSelectListFullscan(RQLParser::SelectListFullscanContext *ctx) override {
    recpToken(PUSH_TSCAN, ctx->getText());
    qry.lSchema.emplace_back(rdb::rField(/*Field_*/ "_" + boost::lexical_cast<std::string>(fieldCount++), 4, 1, rdb::INTEGER),
                             program);
    program.clear();
  }

  void exitLogicExpression(RQLParser::LogicExpressionContext *ctx) override {
    ruleCondition = program;
    program.clear();
  }

  void exitExpression(RQLParser::ExpressionContext *ctx) override {
    // SENTINEL, nie rozstrzygniecie. Publiczny ksztalt pola ustala compiler::inferFieldShapes()
    // z calego programu ONP, kiedy odwolania do pol i agregaty okienne sa juz rozwiazane.
    // Parser zna wtedy wylacznie wlasne tokeny: schematow obcych strumieni na tym etapie nie
    // ma i miec nie moze, wiec `SELECT source[0]` jest dla niego nieodroznialne od `SELECT 1`.
    //
    // Do 2026-09-11 stalo tu jeszcze rozpoznawanie konwersji po OSTATNIM tokenie programu
    // (`to_float` i `to_double`). Regula trafiala w `to_double(k)` i chybiala we wszystkim,
    // co po konwersji jeszcze cokolwiek liczy: `to_float('2.5') * 2` konczy sie tokenem
    // MULTIPLY, wiec pole wychodzilo `INTEGER` mimo wartosci zmiennoprzecinkowej
    // (pozycja 16 w paper-arXiv/debs/done/requested.md, granica 2). Wnioskowanie po ostatnim tokenie
    // nie daje sie na to naprawic - zastepuje je przejscie po calym programie.
    auto outType = rdb::INTEGER;
    int outLen   = 4;
    int outArr   = 1;

    // Napis zostaje TUTAJ, bo jego szerokosc bierze sie z literalow i deklaracji `to_string`,
    // czyli z rzeczy, ktore parser widzi w calosci. Jest to nadal tylko wartosc poczatkowa:
    // inferFieldShapes() liczy ja ponownie tym samym zestawem regul, juz ze znajomoscia pol
    // zrodlowych (`SELECT txt` nad polem `STRING[8]`).
    const auto stringWidth =
        inferStringWidth(program, [](const std::string &, int) -> std::optional<fieldShape> { return std::nullopt; });

    if (stringWidth.has_value()) {
      outType = rdb::STRING;
      outLen  = 1;
      outArr  = *stringWidth;
    }
    qry.lSchema.emplace_back(
        rdb::rField(/*Field_*/ "_" + boost::lexical_cast<std::string>(fieldCount++), outLen, outArr, outType), program);
    program.clear();
  }

  void exitTypeString(RQLParser::TypeStringContext *ctx) override {
    fType     = rdb::STRING;
    fTypeSize = sizeof(uint8_t);
  }

  void exitTypeByte(RQLParser::TypeByteContext *ctx) override {
    fType     = rdb::BYTE;
    fTypeSize = sizeof(uint8_t);
  }
  void exitTypeInt(RQLParser::TypeIntContext *ctx) override {
    fType     = rdb::INTEGER;
    fTypeSize = sizeof(int);
  }
  void exitTypeUnsigned(RQLParser::TypeUnsignedContext *ctx) override {
    fType     = rdb::UINT;
    fTypeSize = sizeof(unsigned);
  }
  void exitTypeFloat(RQLParser::TypeFloatContext *ctx) override {
    fType     = rdb::FLOAT;
    fTypeSize = sizeof(float);
  }
  void exitTypeDouble(RQLParser::TypeDoubleContext *ctx) override {
    fType     = rdb::DOUBLE;
    fTypeSize = sizeof(double);
  }

  void exitSingleDeclaration(RQLParser::SingleDeclarationContext *ctx) override {
    auto fTypeSizeArray = 1;  // Default:1
    // Ta sama granica obowiazuje w gramatyce DESC - .desc czyta takze serwer (A2 M11).
    if (ctx->type_size != nullptr)
      fTypeSizeArray = boundedLiteral("field size", ctx->type_size->getText(), 1, rdb::limits::kMaxFieldLength);
    std::list<token> emptyProgram;
    qry.lSchema.emplace_back(rdb::rField(ctx->ID()->getText(), fTypeSize, fTypeSizeArray, fType), emptyProgram);
    fType = rdb::BYTE;
  }
};

std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet,
                                                                  std::vector<std::string> &statementKeywords, size_t firstLine,
                                                                  std::string_view sourceFile) {
  statementKeywords.clear();
  ANTLRInputStream input(inlet);
  // Create a lexer which scans the input stream
  // to create a token stream.
  RQLLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  // Create a parser which parses the token stream
  // to create a parse tree.
  RQLParser parser(&tokens);
  // Oba listenery bledow potrzebuja parsera (abortParse), wiec powstaja po nim - i przed nim
  // sa niszczone, czyli w chwili, gdy nikt juz do nich nie siega.
  LexerErrorListener lexerErrorListener(parser, firstLine, sourceFile);
  lexer.removeErrorListeners();
  lexer.addErrorListener(&lexerErrorListener);
  ParserErrorListener parserErrorListener(parser, firstLine, sourceFile);
  ParserListener parserListener(coreInstance, firstLine);
  parser.removeParseListeners();
  parser.removeErrorListeners();
  parser.addErrorListener(&parserErrorListener);
  parser.addParseListener(&parserListener);

  // Powod wczesnego powrotu, a nie ogladania drzewa po bledzie: patrz RQLSyntaxError.
  // Komunikat wypisal juz listener, a coreInstance moze zostac czesciowo zmieniony -
  // wolajacy odrzuca wtedy caly plan (launcher) albo cala kopie planu (executorsm::getAdHoc).
  tree::ParseTree *tree = nullptr;
  try {
    tree = parser.prog();
  } catch (const RQLSyntaxError &e) {
    // Tresc bledu wraca ta sama droga co blad semantyczny - statusem. Bez tego operator
    // dostawal samo "Fail", a zdanie nazywajace przyczyne zostawalo na stderr PROCESU
    // SERWERA, czyli w journalu maszyny, gdzie autora zapytania nie ma.
    // Slowo kluczowe pozostaje "UNRECOGNIZED": opiera sie na tym executorsm::getAdHoc,
    // ktory kontroluje status PRZED slowem kluczowym.
    return {e.message, "UNRECOGNIZED", ""};
  }

  for (const auto *child : tree->children) {
    if (child->children.empty()) continue;  // EOF
    std::string keyword = child->children[0]->getText();
    std::ranges::transform(keyword, keyword.begin(), ::toupper);
    statementKeywords.push_back(std::move(keyword));
  }

  const std::string firsttoken = statementKeywords.empty() ? "UNRECOGNIZED" : statementKeywords.front();

  std::string streamName;  // tree->children[1]->children[0]->getText();
  if (!tree->children.empty()) {
    if (auto *selectCtx = dynamic_cast<RQLParser::SelectContext *>(tree->children[0])) {
      streamName = selectCtx->stream_name->getText();
    } else if (auto *declareCtx = dynamic_cast<RQLParser::DeclareContext *>(tree->children[0])) {
      streamName = declareCtx->stream_name->getText();
    } else if (auto *ruleCtx = dynamic_cast<RQLParser::RulezContext *>(tree->children[0])) {
      streamName = ruleCtx->stream_name->getText();
    }
  }
  // Blad semantyczny wraca ta sama droga co skladniowy - wolajacy (launcher albo
  // executorsm::getAdHoc) ma jedno miejsce, w ktorym odrzuca plan lub kopie planu.
  // Nazwa strumienia i slowo kluczowe ida z nim, zeby komunikat wskazywal instrukcje.
  if (!parserListener.semanticError().empty()) return {parserListener.semanticError(), firsttoken, streamName};

  return {"OK", firsttoken, streamName};
}

std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet,
                                                                  std::vector<std::string> &statementKeywords) {
  return parserRQLString(coreInstance, inlet, statementKeywords, 1);
}

std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet) {
  std::vector<std::string> ignoredKeywords;
  return parserRQLString(coreInstance, inlet, ignoredKeywords, 1);
}

/// Wiersze logiczne pliku RQL: komentarze usuniete, kontynuacje `\\` sklejone.
///
/// Komentarz `#` jest obslugiwany TUTAJ, a nie w lekserze, i zajmuje CALY wiersz. Lekser
/// zna `#` wylacznie jako operator przeplotu, wiec `FROM a # b` jest przeplotem niezaleznie
/// od spacji - do 2026-08-29 regula leksera `'# '` zjadala taki zapis do `FROM a` i plan
/// kompilowal sie po cichu bez `b`. Komentarz konczacy wiersz zapisuje sie `//`.
///
/// Warunek patrzy na pierwszy NIEBIALY znak, bo wcieta linia komentarza szla dotad do
/// leksera i lapala ja wlasnie usunieta regula.
///
/// Z kazda instrukcja wraca numer wiersza PLIKU, na ktorym sie zaczyna. Bez tego numeru
/// blad skladni wskazywal wiersz liczony wewnatrz pojedynczej instrukcji, czyli praktycznie
/// zawsze 1 - pozycja, ktorej w pliku planu nie da sie odnalezc. Kotwica jest pierwszym
/// wierszem instrukcji, bo kontynuacje `\\` sa sklejane w jeden wiersz logiczny.
std::vector<std::pair<std::string, size_t>> readLogicalLines(std::istream &file) {
  std::vector<std::pair<std::string, size_t>> result;
  std::string line;
  std::string accumulated;
  size_t physicalLine         = 0;
  size_t accumulatedFirstLine = 0;  // 0 znaczy: instrukcja jeszcze sie nie zaczela
  while (std::getline(file, line)) {
    ++physicalLine;
    const auto firstVisible = line.find_first_not_of(" \t\r");
    if (firstVisible == std::string::npos || line[firstVisible] == '#') continue;
    if (accumulatedFirstLine == 0) accumulatedFirstLine = physicalLine;
    if (line.back() == '\\') {
      accumulated += line.substr(0, line.size() - 1) + ' ';
      continue;
    }
    accumulated += line;
    result.emplace_back(std::move(accumulated), accumulatedFirstLine);
    accumulated          = {};
    accumulatedFirstLine = 0;
  }
  return result;
}

std::string parserRQLFile_4Test(qTree &coreInstance, const std::string &sInputFile) {
  std::ifstream file(sInputFile);
  if (!file.is_open()) {
    SPDLOG_ERROR("Error: Unable to open file!");
    return "Unable to open file.";
  }

  std::string status = "Empty file.";
  std::vector<std::string> statementKeywords;
  for (const auto &[stmt, firstLine] : readLogicalLines(file)) {
    auto [result, first_keyword, stream_name] = parserRQLString(coreInstance, stmt, statementKeywords, firstLine, sInputFile);
    status                                    = result;
    if (status != "OK") {
      SPDLOG_ERROR("Error: Parsing failed on {}.\n{}", first_keyword, stmt);
      return status;
    }
  }

  return status;
}
