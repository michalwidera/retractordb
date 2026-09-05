#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
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
#include "rdb/convertTypes.hpp"
#include "rqlFunctions.hpp"

using namespace antlrcpp;
using namespace antlr4;

namespace {
constexpr size_t kAgseWindowSignChildIndex = 5;

/// Blad skladni RQL: przerywa parsowanie, zamiast konczyc proces.
///
/// Do 2026-09-05 oba listenery bledow wolaly exit(EPERM). W procesie serwera oznaczalo to
/// smierc xretractora przy KAZDYM blednym zapytaniu ad-hoc — `xqry -a "ml"` wystarczalo.
///
/// Sam powrot z listenera nie zalatwia sprawy: ANTLR wchodzi wtedy w odzyskiwanie i wola
/// dalej callbacki ParserListenera na kalekich kontekstach, gdzie np. ctx->ID() jest nullem.
/// Rzut wychodzi z prog() przez generowany kod, bo ten lapie wylacznie RecognitionException.
struct RQLSyntaxError {
  std::string message;
};

/// Gorne ograniczenie dlugosci komunikatu wracajacego do klienta.
///
/// Komunikat idzie do odpowiedzi serwera jako IPCString we WSPOLNYM segmencie 64 kB
/// (ipc::kShmemSegmentSize). Lista `expecting {...}` przy blednym poczatku instrukcji
/// wylicza kilkadziesiat tokenow; wyczerpanie segmentu konczy sie bad_alloc-iem lapanym
/// POZA petla odbioru, czyli smiercia watku komunikacyjnego z dala od przyczyny.
constexpr size_t kMaxSyntaxErrorMessage = 300;

/// Nazwa agregatu zlozona do malych liter. Lekser dopuszcza dwie pisownie ('MIN'|'min'),
/// wiec ASCII wystarcza.
std::string lowercased(std::string text) {
  std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

/// Zdejmuje ParserListenera i rzuca RQLSyntaxError.
///
/// Zdjecie listenera jest warunkiem KONIECZNYM, nie porzadkami: samo rozwijanie stosu
/// przechodzi przez `finally` generowanego kodu (antlrcpp::FinalAction), ktore wola
/// exitRule(), a to wola exitDeclare()/exitSelect()/... na kontekscie zatrzymanym w polowie
/// budowy. Pierwsza wersja tej naprawy padala tam w exitDeclare() na `ctx->ID()` rownym
/// nullptr — czyli segfaultem zamiast exit(EPERM), bez zadnej poprawy.
/// Po removeParseListeners() petla triggerExitRuleEvent() chodzi po pustej liscie.
///
/// Listener bledow leksera dostaje ten sam parser, bo blad leksera rozwija stos przez
/// dokladnie te same `finally` — token pobiera sie w srodku reguly parsera.
[[noreturn]] void abortParse(antlr4::Parser &parser, size_t firstLine, size_t line, size_t charPositionInLine,
                             const std::string &msg, Token *offendingSymbol) {
  // Lekser i parser licza wiersze wewnatrz PRZEKAZANEGO tekstu, a ten bywa pojedyncza
  // instrukcja wyjeta z pliku planu przez readLogicalLines. firstLine przesuwa numer z
  // powrotem na wiersz pliku — bez tego kazda odmowa wskazywala wiersz 1, niezaleznie od
  // tego, w ktorym miejscu planu stoi blad.
  const size_t sourceLine = firstLine + line - 1;

  // Tekst obrazajacego tokenu, a nie jego adres. Listener leksera podaje tu nullptr, bo blad
  // powstaje, zanim token zostanie zbudowany.
  const std::string offendingText = (offendingSymbol != nullptr) ? offendingSymbol->getText() : std::string("<unknown>");

  // Komunikat MUSI byc jednowierszowy: wraca do klienta jako wartosc ptree w formacie `info`,
  // ktory znaki nowej linii escape'uje — wielowierszowiec dojechalby jako jeden ciag z
  // widocznymi `\n`.
  std::string message = "line " + std::to_string(sourceLine) + ":" + std::to_string(charPositionInLine) + " " + msg;
  std::ranges::replace_if(message, [](char c) { return c == '\n' || c == '\r'; }, ' ');
  if (message.size() > kMaxSyntaxErrorMessage) message.resize(kMaxSyntaxErrorMessage);

  // Wydruk na stderr ZOSTAJE obok statusu: w trybie uslugowym stderr to journald, czyli
  // jedyny slad po stronie serwera. Tak samo robi sciezka semantyczna (reportSemanticError).
  // Tu idzie msg nieprzyciety — ograniczenie dotyczy wylacznie drogi przez pamiec dzielona.
  std::cerr << "Syntax error @Rql" << '\n';
  std::cerr << "line:" << sourceLine << ":" << charPositionInLine << " at " << offendingText << '\n';
  std::cerr << "msg:" << msg << '\n';
  SPDLOG_ERROR("Parser: {}", message);

  parser.removeParseListeners();
  throw RQLSyntaxError{std::move(message)};
}
}  // namespace

// https://stackoverflow.com/questions/44515370/how-to-override-error-reporting-in-c-target-of-antlr4

class LexerErrorListener : public BaseErrorListener {
 public:
  LexerErrorListener(antlr4::Parser &parser, size_t firstLine) : parser_(parser), firstLine_(firstLine) {}
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    abortParse(parser_, firstLine_, line, charPositionInLine, msg, offendingSymbol);
  }

 private:
  antlr4::Parser &parser_;
  size_t firstLine_;
};

class ParserErrorListener : public BaseErrorListener {
 public:
  ParserErrorListener(antlr4::Parser &parser, size_t firstLine) : parser_(parser), firstLine_(firstLine) {}
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    abortParse(parser_, firstLine_, line, charPositionInLine, msg, offendingSymbol);
  }

 private:
  antlr4::Parser &parser_;
  size_t firstLine_;
};

/* Iterator - each new field gets new fieldCount number */
int fieldCount = 0;

class ParserListener : public RQLBaseListener {
  qTree &coreInstance;

  /* Helper variable required for rational numbers processing */
  boost::rational<int> rationalResult;

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

  /// Pierwszy blad semantyczny calego przebiegu — czyli taki, ktorego gramatyka nie lapie,
  /// a ktory mimo to unieważnia zapytanie (regula na nieistniejacym strumieniu, na deklaracji,
  /// powtorzona nazwa reguly). Do 2026-09-05 kazdy z tych przypadkow konczyl sie abort() albo
  /// cisza; w procesie serwera pierwsze znaczylo smierc xretractora z powodu bledu w cudzym
  /// zapytaniu ad-hoc, drugie — odpowiedz "OK" na polecenie, ktore nie zrobilo nic.
  /// Rozstrzyga blad pierwszy: dalsze sa juz tylko jego nastepstwami.
  std::string semanticError_;

  void reportSemanticError(const std::string &message) {
    std::cerr << "Error: " << message << '\n';
    SPDLOG_ERROR("Parser: {}", message);
    if (semanticError_.empty()) semanticError_ = message;
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
      if (existing.name == rule_name) return "Rule '" + rule_name + "' is already defined on stream '" + stream_name + "'";

    rule ruleConstruct(rule_name, ruleCondition);
    switch (actionType) {
      case rule::DUMP:
        // Zakres pusty odrzucamy juz tutaj, bo dalej czeka na niego FatalError w
        // compiler::computeRequiredCapacities() — a w sciezce ad-hoc FatalError to smierc
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
  ParserListener(qTree &coreInstance) : coreInstance(coreInstance) {};

  [[nodiscard]] const std::string &semanticError() const { return semanticError_; }

  void enterProg(RQLParser::ProgContext *ctx) override {}

  void exitFieldID(RQLParser::FieldIDContext *ctx) override { recpToken(PUSH_ID3, ctx->getText()); }
  void exitFieldIDUnderline(RQLParser::FieldIDUnderlineContext *ctx) override { recpToken(PUSH_IDX, ctx->getText()); }
  void exitFieldIDColumnName(RQLParser::FieldIDColumnNameContext *ctx) override { recpToken(PUSH_ID1, ctx->getText()); }
  void exitFieldIDTable(RQLParser::FieldIDTableContext *ctx) override { recpToken(PUSH_ID2, ctx->getText()); }

  /// `cells[$]`, `cells[23-$]` — indeks z numerem instancji generatora.
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

  /// `$` poza nawiasami kwadratowymi — numer instancji jako wartosc.
  ///
  /// Wartosci jeszcze nie znamy (jest nia numer instancji, ktory powstanie dopiero przy
  /// ekspansji), wiec token jest tymczasowy: expandStreamGenerators() zamienia go na
  /// PUSH_VAL. PUSH_GENIDX, ktory przezyl ten przebieg, jest bledem kompilacji — znaczy
  /// `$` uzyte poza generatorem.
  void exitExpGenIndex(RQLParser::ExpGenIndexContext *ctx) override { recpToken(PUSH_GENIDX); }

  /// Poczatki podprogramow argumentow okna — pozycja w `program` w chwili wejscia w regule.
  ///
  /// Stos, a nie pojedyncza zmienna, bo gramatyka dopuszcza `MIN(MAX(a[0]:2):3)`: znacznik
  /// wewnetrznego okna musi zdjac sie przed zewnetrznym. Samo zagniezdzenie odrzuca dopiero
  /// kompilator, ktory jako jedyny widzi, ze historia zrodla nie zawiera wynikow okna.
  ///
  /// Znaczniki sa wazne w obrebie JEDNEGO wyrazenia — `program` czysci exitExpression().
  std::vector<size_t> windowArgMarks;

  void enterWindow_agg(RQLParser::Window_aggContext *ctx) override { windowArgMarks.push_back(program.size()); }

  /// `MIN(cells[0] : 10)` — agregat okna REKORDOWEGO w liscie SELECT.
  ///
  /// Argument jest WYRAZENIEM, wiec jego tokeny doklada juz podregula `expression_factor`;
  /// ten listener dopisuje operator, ktory niesie DWIE liczby: szerokosc okna i pozycje
  /// pierwszego tokenu argumentu. Bez tej drugiej nie da sie odroznic argumentu od tego, co
  /// stalo w programie wczesniej — `x + MIN(y[0]:2)` i `MIN(x+y[0]:2)` roznia sie wylacznie
  /// nia. compiler::resolveWindowAggregates() zabiera oba i zostawia jeden bezargumentowy
  /// token z indeksem grupy okna.
  ///
  /// Okno jest zawsze PRZESUWNE co rekord — powod przy regule `window_agg` w RQL.g4.
  /// Szerokosc NIE jest tu sprawdzana: listener parsera nie ma lagodnego kanalu bledu
  /// (zostaje FatalError), a szerokosc niedodatnia jest bledem PLANU, ktory kompilator
  /// raportuje przez `Check result:` razem z pozostalymi kontrolami.
  void exitWindow_agg(RQLParser::Window_aggContext *ctx) override {
    const int width = std::stoi(ctx->width->getText());
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

  void exitExpFloat(RQLParser::ExpFloatContext *ctx) override { recpToken(PUSH_VAL, std::stof(ctx->getText())); }
  void exitExpDec(RQLParser::ExpDecContext *ctx) override { recpToken(PUSH_VAL, std::stoi(ctx->getText())); }
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

  // Notacja przyrostkowa `strumien.avg` jest wygaszana na rzecz AVG(strumien) — patrz
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
  /// przyrostkowa. Roznica jest w zasiegu — postac funkcyjna domyka argument wlasnymi
  /// nawiasami, wiec bierze cale wyrazenie niezaleznie od drabiny priorytetow, podczas gdy
  /// `.agg` siega tylko po operand poziomu postfiksowego. Do 2026-08-29 `.agg` przyjmowalo
  /// wylacznie stream_factor i okno trzeba bylo materializowac osobnym zapytaniem:
  ///
  ///     SELECT * STREAM w FROM sq@(125,1000)
  ///     SELECT * STREAM s FROM w.sumc
  ///
  /// Postac funkcyjna bierze cale stream_expression, wiec ta sama para to jedno zapytanie
  /// `FROM SUMC(sq@(125,1000))`. Program klauzuli FROM wychodzi identyczny po sklejeniu
  /// — [PUSH_STREAM sq, STREAM_AGSE(125,1000), STREAM_SUM] — a rozbija go z powrotem na dwa
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
    if (ctx->children[kAgseWindowSignChildIndex]->getText() == "-")
      window = -std::stoi(ctx->window->getText());
    else
      window = std::stoi(ctx->window->getText());
    step = std::stoi(ctx->step->getText());

    program.emplace_back(STREAM_AGSE, std::make_pair(step, window));
  }

  /// Nazwa funkcji jest w gramatyce zwyklym ID, wiec autor moze ja napisac dowolna
  /// wielkoscia liter. Do tokena idzie postac KANONICZNA z rqlFunctions.hpp, a nie ta
  /// napisana w zapytaniu — uzasadnienie przy definicji tabeli.
  ///
  /// Nazwy NIEZNANEJ nie odrzucamy tutaj. Listener parsera nie ma kanalu na lagodny
  /// blad (zostaje FatalError), a `compiler::checkFunctionCalls()` raportuje ja przez
  /// `Check result:` razem z pozostalymi kontrolami planu. Nieznana nazwa jedzie wiec
  /// dalej w postaci doslownej, zeby komunikat pokazal to, co napisal autor.
  void exitFunction_call(RQLParser::Function_callContext *ctx) override {
    const std::string written = ctx->fn->getText();
    const auto known          = rdb::findRqlFunction(written);
    const std::string name    = known ? std::string(known->canonical) : written;

    if (ctx->DECIMAL() != nullptr)
      recpToken(CALL2, std::make_pair(name, std::stoi(ctx->DECIMAL()->getText())));
    else
      recpToken(CALL, name);
  }

  // page 119 - The Definitive ANTL4 Reference Guide
  void exitDeclare(RQLParser::DeclareContext *ctx) override {
    qry.filename = ctx->file_name->getText();
    // This removes ''
    qry.filename.erase(qry.filename.size() - 1);
    qry.filename.erase(0, 1);
    qry.id           = ctx->ID()->getText();
    qry.rInterval    = rationalResult;
    qry.isDisposable = (ctx->DISPOSABLE() != nullptr);
    qry.isOneShot    = (ctx->ONESHOT() != nullptr);
    qry.isHold       = (ctx->HOLD() != nullptr);
    coreInstance.push_back(qry);
    qry.reset();
    fieldCount = 0;
  }

  // https://www.programiz.com/cpp-programming/string-float-conversion
  // https://www.geeksforgeeks.org/converting-strings-numbers-cc/

  void exitRationalAsFloat(RQLParser::RationalAsFloatContext *ctx) override {
    rationalResult = Rationalize(std::stod(ctx->FLOAT()->getText()));
  }

  void exitRationalAsDecimal(RQLParser::RationalAsDecimalContext *ctx) override {
    rationalResult = std::stoi(ctx->DECIMAL()->getText());
  }

  void exitFraction(RQLParser::FractionContext *ctx) override {
    const int nom = std::stoi(ctx->children[0]->getText());
    const int den = std::stoi(ctx->children[2]->getText());
    if (den == 0) FatalError("RQLParser::exitFraction: denominator is zero");
    rationalResult = boost::rational<int>(nom, den);
  }

  void exitSelect(RQLParser::SelectContext *ctx) override {
    qry.generatorSize = (ctx->gen_size != nullptr) ? std::stoi(ctx->gen_size->getText()) : query::notAGenerator;

    // this loop creates field names in streamName + "_" + counter++
    //
    // Dla generatora prefiks doklada compiler::expandStreamGenerators(), bo nazwa pola ma
    // pochodzic od nazwy INSTANCJI (`cell$0_0`), a nie od nazwy szablonu (`cell_0`). Tylko
    // wtedy plan z generatora jest nie do odroznienia od recznie rozpisanych SELECT-ow.
    if (qry.generatorSize == query::notAGenerator) {
      for (auto &i : qry.lSchema) {
        if ((i.field_.rname).starts_with("_")) (i.field_.rname) = ctx->ID()->getText() + i.field_.rname;
      }
    }

    qry.id = ctx->ID()->getText();

    if (qry.id == constants::Reserved_id_oob) {
      std::cerr << "Error: " << constants::Reserved_id_oob << " is reserved stream name." << '\n';
      SPDLOG_ERROR("{} is reserved stream name.", constants::Reserved_id_oob);
      abort();
    }

    qry.lProgram = program;
    if (ctx->VOLATILE() != nullptr) {
      qry.policy = std::make_pair("MEMORY", 1);
    }

    if (ctx->FILE() != nullptr) {
      qry.filename = ctx->file_name->getText();

      // This removes ''
      qry.filename.erase(qry.filename.size() - 1);
      qry.filename.erase(0, 1);

      if (qry.filename.empty()) FatalError("RQLParser: directive filename must not be empty");
    }

    if (ctx->STORAGE() != nullptr) {
      qry.storage_policy = ctx->type_name->getText();
      std::ranges::transform(qry.storage_policy, qry.storage_policy.begin(), ::toupper);  // to upper case
    }

    coreInstance.push_back(qry);
    program.clear();
    qry.reset();
    fieldCount = 0;
  }

  void exitRetention(RQLParser::RetentionContext *ctx) override {
    if (ctx->segments != nullptr) {
      // retention {capacity} !{segments}
      qry.retention = std::pair<int, int>(      //
          std::stoi(ctx->segments->getText()),  //
          std::stoi(ctx->capacity->getText()));
    } else {
      // retention {capacity} - note: segments is optional but capacity is required
      qry.policy.second = std::stoi(ctx->capacity->getText());
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
  /// przesuwaja sie razem z jego obecnoscia: `DUMP -5 TO 5` ma piecioro dzieci, `DUMP 5 TO 5` —
  /// czworo. Odczyt ze stalej pozycji children[4] wychodzil w tym drugim przypadku poza wektor:
  /// w Debug konczylo sie to asercja biblioteki standardowej, w Release odczytem spoza zakresu,
  /// a z kanalu ad-hoc — smiercia serwera po `DO DUMP 5 TO 5`. Dlatego pytamy o sasiada samej
  /// liczby, zamiast liczyc pozycje z gory.
  static bool negatedBefore(RQLParser::DumppartContext *ctx, const antlr4::Token *number) {
    for (size_t i = 1; i < ctx->children.size(); ++i) {
      const auto *terminal = dynamic_cast<antlr4::tree::TerminalNode *>(ctx->children[i]);
      if (terminal == nullptr || terminal->getSymbol() != number) continue;
      return ctx->children[i - 1]->getText() == "-";
    }
    return false;
  }

  void exitDumppart(RQLParser::DumppartContext *ctx) override {
    actionType = rule::DUMP;
    dump_left  = std::stoi(ctx->step_back->getText());
    if (negatedBefore(ctx, ctx->step_back)) dump_left = -dump_left;
    dump_right = std::stoi(ctx->step_forward->getText());
    if (negatedBefore(ctx, ctx->step_forward)) dump_right = -dump_right;

    if (ctx->rule_retnetion != nullptr)
      dump_retention = std::stoi(ctx->rule_retnetion->getText());
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

  void exitCoption(RQLParser::CoptionContext *ctx) override {
    qry.id = ":" + ctx->directive->getText();
    std::ranges::transform(qry.id, qry.id.begin(), ::toupper);  // to upper case
    qry.filename = ctx->value->getText();

    // This removes ''
    qry.filename.erase(qry.filename.size() - 1);
    qry.filename.erase(0, 1);

    if (qry.filename.empty()) FatalError("RQLParser: directive filename must not be empty");

    // Add / at the end of path, if not present in case of STORAGE
    if (qry.id == ":STORAGE" && qry.filename[qry.filename.size() - 1] != '/') qry.filename.push_back('/');

    coreInstance.push_back(qry);
    program.clear();
    qry.reset();
    fieldCount = 0;
  }

  void exitSExpTimeMove(RQLParser::SExpTimeMoveContext *ctx) override {
    recpToken(STREAM_TIMEMOVE, std::stoi(ctx->DECIMAL()->getText()));
  }

  /// Nazwa strumienia. Pozostale alternatywy `stream_factor` — `( e )` i wywolanie
  /// reduktora — nie wnosza wlasnego tokenu: ich tresc dolozyly juz wezly nizej.
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
    auto outType = rdb::INTEGER;
    int outLen   = 4;
    int outArr   = 1;

    // Napis rozstrzyga wynik CALEGO wyrazenia, a nie pierwszy napotkany literal — inaczej
    // `to_integer('42')+k` ladowalo w polu STRING (pozycja 12 w usecases/requested.md).
    // Ksztaltow pol obcych strumieni na etapie parsowania nie ma i miec nie moze, wiec
    // odwolanie do pola wchodzi tu jako liczba; przypadek `SELECT txt` nad polem STRING
    // domyka compiler::inferStringFieldTypes(), gdy schematy sa juz rozwiazane.
    const auto stringWidth =
        inferStringWidth(program, [](const std::string &, int) -> std::optional<fieldShape> { return std::nullopt; });

    if (stringWidth.has_value()) {
      outType = rdb::STRING;
      outLen  = 1;
      outArr  = *stringWidth;
    } else if (!program.empty()) {
      auto &last = program.back();
      if (last.getCommandID() == CALL) {
        auto fn = last.getStr_();
        if (fn == "to_float") {
          outType = rdb::FLOAT;
          outLen  = 4;
        } else if (fn == "to_double") {
          outType = rdb::DOUBLE;
          outLen  = static_cast<int>(sizeof(double));
        }
      }
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
    if (ctx->type_size != nullptr) fTypeSizeArray = std::stoi(ctx->type_size->getText());
    std::list<token> emptyProgram;
    qry.lSchema.emplace_back(rdb::rField(ctx->ID()->getText(), fTypeSize, fTypeSizeArray, fType), emptyProgram);
    fType = rdb::BYTE;
  }
};

/// Parsuje JEDNA porcje tekstu RQL. `firstLine` to numer wiersza, na ktorym ta porcja stoi
/// w pliku zrodlowym — wolajacy, ktory tnie plik na instrukcje (parsePlanText,
/// parserRQLFile_4Test), podaje tu pozycje instrukcji, reszta zostawia 1.
std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &inlet,
                                                                  std::vector<std::string> &statementKeywords,
                                                                  size_t firstLine) {
  statementKeywords.clear();
  ANTLRInputStream input(inlet);
  // Create a lexer which scans the input stream
  // to create a token stream.
  RQLLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  // Create a parser which parses the token stream
  // to create a parse tree.
  RQLParser parser(&tokens);
  // Oba listenery bledow potrzebuja parsera (abortParse), wiec powstaja po nim — i przed nim
  // sa niszczone, czyli w chwili, gdy nikt juz do nich nie siega.
  LexerErrorListener lexerErrorListener(parser, firstLine);
  lexer.removeErrorListeners();
  lexer.addErrorListener(&lexerErrorListener);
  ParserErrorListener parserErrorListener(parser, firstLine);
  ParserListener parserListener(coreInstance);
  parser.removeParseListeners();
  parser.removeErrorListeners();
  parser.addErrorListener(&parserErrorListener);
  parser.addParseListener(&parserListener);

  // Powod wczesnego powrotu, a nie ogladania drzewa po bledzie: patrz RQLSyntaxError.
  // Komunikat wypisal juz listener, a coreInstance moze zostac czesciowo zmieniony —
  // wolajacy odrzuca wtedy caly plan (launcher) albo cala kopie planu (executorsm::getAdHoc).
  tree::ParseTree *tree = nullptr;
  try {
    tree = parser.prog();
  } catch (const RQLSyntaxError &e) {
    // Tresc bledu wraca ta sama droga co blad semantyczny — statusem. Bez tego operator
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
      streamName = selectCtx->ID()->getText();
    } else if (auto *declareCtx = dynamic_cast<RQLParser::DeclareContext *>(tree->children[0])) {
      streamName = declareCtx->stream_name->getText();
    } else if (auto *ruleCtx = dynamic_cast<RQLParser::RulezContext *>(tree->children[0])) {
      streamName = ruleCtx->stream_name->getText();
    }
  }
  // Blad semantyczny wraca ta sama droga co skladniowy — wolajacy (launcher albo
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
/// od spacji — do 2026-08-29 regula leksera `'# '` zjadala taki zapis do `FROM a` i plan
/// kompilowal sie po cichu bez `b`. Komentarz konczacy wiersz zapisuje sie `//`.
///
/// Warunek patrzy na pierwszy NIEBIALY znak, bo wcieta linia komentarza szla dotad do
/// leksera i lapala ja wlasnie usunieta regula.
///
/// Z kazda instrukcja wraca numer wiersza PLIKU, na ktorym sie zaczyna. Bez tego numeru
/// blad skladni wskazywal wiersz liczony wewnatrz pojedynczej instrukcji, czyli praktycznie
/// zawsze 1 — pozycja, ktorej w pliku planu nie da sie odnalezc. Kotwica jest pierwszym
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
    auto [result, first_keyword, stream_name] = parserRQLString(coreInstance, stmt, statementKeywords, firstLine);
    status                                    = result;
    if (status != "OK") {
      SPDLOG_ERROR("Error: Parsing failed on {}.\n{}", first_keyword, stmt);
      return status;
    }
  }

  return status;
}
