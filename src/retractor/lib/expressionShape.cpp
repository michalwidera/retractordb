#include "expressionShape.hpp"

#include <cctype>  // std::tolower

#include <algorithm>  // std::ranges::transform, std::max
#include <set>
#include <string>
#include <string_view>
#include <utility>  // std::cmp_less
#include <variant>
#include <vector>

#include <boost/rational.hpp>

#include "exprSimplify.hpp"  // kToStringDefaultWidth
#include "rqlFunctions.hpp"  // findRqlFunction — kanoniczna pisownia w komunikacie bramki

namespace {

/// Nazwa funkcji zlozona do malych liter — parser zapisuje postac kanoniczna, ale ewaluator
/// i tak sklada wielkosc liter, wiec analizator robi to samo.
std::string lowercased(std::string text) {
  std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

/// Szerokosc napisu w bajtach. Wartosc LICZBOWA wnosi do konkatenacji ZERO — tak samo, jak
/// liczyla to regula sprzed 2026-08-30 (`inferStringWidth`), wiec szerokosci pol
/// `STRING` w istniejacych planach sie nie zmieniaja.
int stringBytes(const exprShape &shape) { return shape.rtype == rdb::STRING ? shape.rlen * shape.rarray : 0; }

/// Ksztalt pola `STRING` o zadanej szerokosci — zapis deskryptora to `rlen = 1`, `rarray = N`.
exprShape stringShape(int width) {
  return exprShape{.rtype = rdb::STRING, .rlen = static_cast<int>(sizeof(uint8_t)), .rarray = width};
}

/// Funkcje matematyczne liczone przez `callFun()`: rachunek idzie przez `double`, a wynik
/// wraca rzutem na typ ARGUMENTU. Stad `Ceil(DOUBLE)` jest `DOUBLE`, a `Sqrt(INTEGER)` —
/// `INTEGER`.
const std::set<std::string> &typePreservingFunctions() {
  static const std::set<std::string> names{"floor", "ceil", "sqrt", "round", "tan", "log", "log2", "trunc"};
  return names;
}

/// Funkcje o niewymiernej przeciwdziedzinie liczone przez `callRealFun()`: wynik jest
/// `DOUBLE` NIEZALEZNIE od typu argumentu, bez stratnego powrotu do `INTEGER` czy `RATIONAL`.
/// To wyjatek od reguly `typePreservingFunctions()` powyzej — patrz opis przy
/// `rejectedIrrationalOverExact()`, dlaczego `tan`, `log` i `log2` jeszcze tu nie stoja.
bool isRealValuedFunction(std::string_view name) { return name == "sin" || name == "cos" || name == "exp"; }

/// Operatory zdejmujace DWA operandy ze stosu ksztaltow.
bool isBinaryShapeOperator(command_id cmd) {
  switch (cmd) {
    case ADD:
    case SUBTRACT:
    case MULTIPLY:
    case DIVIDE:
    case POWER:
    case CMP_EQUAL:
    case CMP_NOT_EQUAL:
    case CMP_LT:
    case CMP_GT:
    case CMP_LE:
    case CMP_GE:
    case AND:
    case OR:
      return true;
    default:
      return false;
  }
}

/// Kombinacje funkcja+typ, ktorych kompilator ODRZUCA, zamiast wydac zla wartosc albo
/// wpuscic uzytkownika w niejednorodna regule. Pusty napis znaczy „wolno".
///
/// Bramka obejmuje SIEDEM funkcji o niewymiernej przeciwdziedzinie nad `RATIONAL`, ale nie
/// wszystkie stoja tutaj z tego samego powodu i te powody trzeba trzymac osobno — inaczej
/// ktos zdejmie bramke z trojki, ktora defektu nie ma, i nie zauwazy, ze reszcie zostaje.
///
/// `Sqrt` — CICHA ZLA WARTOSC
/// -------------------------
/// `callFun()` (expressionEvaluator.cpp) liczy funkcje matematyczne przez `double` i rzutuje
/// wynik Z POWROTEM NA TYP ARGUMENTU. Dla `RATIONAL` droga powrotna idzie przez `Rationalize`
/// z tolerancja 1e-6, bo `boost::rational<int>` nie ma jak zapisac liczby niewymiernej. Skutek
/// jest taki, ze `Sqrt(2/1)` daje `19601/13860` — wartosc dobra do ~1,8e-9, ale o OGROMNYM
/// mianowniku. `boost::rational<int>` trzyma licznik i mianownik w `int32` i NIE sprawdza
/// zakresu, wiec dwa dalsze mnozenia przepelniaja go po cichu:
///
///     Sqrt(x)                    -> 19601/13860              = 1,41421356
///     Sqrt(x)*Sqrt(x)            -> 384199201/192099600      = 2,0000000052
///     Sqrt(x)*Sqrt(x)*Sqrt(x)    -> 1610868913/-379267520    = -4,247   (oczekiwane +2,828)
///
/// Zly rzad wielkosci i ZLY ZNAK, bez wyjatku i bez NULL. To nie jest strata precyzji, tylko
/// cicha bledna wartosc, wiec bramka `-c` jest wlasciwym miejscem: program, ktory nie policzy
/// poprawnie, nie ma sie kompilowac (ta sama zasada co w compiler::checkFunctionCalls()).
///
/// `tan`, `log`, `log2` — TEN SAM DEFEKT, TA SAMA DROGA
/// ----------------------------------------------------
/// Nadal licza przez `callFun()` i wracaja rzutem na typ argumentu, wiec nad `RATIONAL`
/// rationalizuja tak samo jak `Sqrt` (zmierzone: `log(2/1)` daje `2731/3940`) i tak samo po
/// cichu przepelniaja sie w dalszym rachunku. Bramka zamyka to BEZ ruszania ich typu wyniku:
/// odrzucenie dotyczy WYLACZNIE pary z `RATIONAL`, a nad `INTEGER`, `FLOAT` i `DOUBLE` nic
/// sie nie zmienia, wiec ZADEN deskryptor nie zmienia typu pola i bramki H9/H10 tego nie widza.
///
/// `sin`, `cos`, `exp` — JEDNOLITOSC REGULY, NIE DEFEKT
/// ---------------------------------------------------
/// Te trzy funkcje NIE MAJA powyzszego defektu: ida przez `callRealFun()`, ktore konczy na
/// `DOUBLE` i nigdy nie wraca do `RATIONAL`, wiec `cos(m[0])` policzyloby sie z pelna
/// dokladnoscia `double`. Odrzucenie jest decyzja o KONTRAKCIE JEZYKA, podjeta swiadomie
/// (2026-09-12): uzytkownik nie ma pamietac, ktora funkcja o niewymiernej przeciwdziedzinie
/// przyjmuje wartosc wymierna, a ktora nie. Jedna regula brzmi „funkcja niewymierna nad
/// `RATIONAL` wymaga jawnego `to_double`" i obowiazuje dla wszystkich siedmiu.
///
/// Cena tej decyzji jest realna i zostala przyjeta: reduktory `MIN`/`MAX`/`AVG`/`SUMC` sa
/// z definicji `RATIONAL`, wiec `to_double` bedzie potrzebne w kazdym zapytaniu w rodzaju RMS.
///
/// CO ZOSTAJE OTWARTE po tej bramce
/// --------------------------------
/// Klasa CICHEJ ZLEJ WARTOSCI jest zamknieta: `RATIONAL` jest jedynym typem, ktorego droga
/// powrotna rationalizuje, i dla wszystkich siedmiu nazw jest teraz odrzucany. Otwarty
/// zostaje sam KONTRAKT TYPU `tan`/`log`/`log2`: nad `INTEGER` nadal wracaja na `INTEGER`,
/// czyli obcinaja czesc ulamkowa — strata JAWNA i udokumentowana, nie przepelnienie.
/// Doprowadzenie ich do `DOUBLE` jak `sin`/`cos`/`exp` zmienia typ pola w `.desc`, czyli jest
/// zmiana formatu artefaktu i musi przejsc przez bramki H9/H10 oraz korpus. To jest osobne
/// zadanie i NIE jest juz pilne, bo zla liczba nie ma juz ktoredy wyjsc.
///
/// Funkcje zaokraglajace `Floor`, `Ceil`, `round` i `trunc` nad `RATIONAL` sa bezpieczne
/// i zostaja przy typie argumentu: ich wynik jest calkowity, czyli ma mianownik 1 (zmierzone).
/// `Abs` liczy wprost na wariancie, wiec mianownika nie rusza w ogole.
bool hasIrrationalRange(std::string_view name) {
  static const std::set<std::string, std::less<>> names{"sqrt", "sin", "cos", "exp", "tan", "log", "log2"};
  return names.contains(name);
}

std::string rejectedIrrationalOverExact(std::string_view name, rdb::descFld argumentType) {
  if (argumentType != rdb::RATIONAL || !hasIrrationalRange(name)) return {};

  const auto canonical = rdb::findRqlFunction(name);
  const std::string shown{canonical ? canonical->canonical : name};

  return "'" + shown +
         "' over a RATIONAL value is not implemented: functions with an irrational range "
         "require an explicit conversion. Write " +
         shown + "(to_double(x)). Note that stream reducers MIN/MAX/AVG/SUMC produce RATIONAL.";
}

}  // namespace

int fieldLengthOfType(const rdb::descFld type) {
  switch (type) {
    case rdb::BYTE:
      return static_cast<int>(sizeof(uint8_t));
    case rdb::INTEGER:
      return static_cast<int>(sizeof(int));
    case rdb::UINT:
      return static_cast<int>(sizeof(unsigned));
    case rdb::RATIONAL:
      return static_cast<int>(sizeof(boost::rational<int>));
    case rdb::FLOAT:
      return static_cast<int>(sizeof(float));
    case rdb::DOUBLE:
      return static_cast<int>(sizeof(double));
    case rdb::STRING:
      return static_cast<int>(sizeof(uint8_t));
    default:
      return static_cast<int>(sizeof(int));
  }
}

exprShape numericShape(const rdb::descFld type) {
  return exprShape{.rtype = type, .rlen = fieldLengthOfType(type), .rarray = 1};
}

rdb::descFld normalizedOperandType(const rdb::descFld left, const rdb::descFld right) { return std::max(left, right); }

rdb::descFld arithmeticValueType(const rdb::descFld left, const rdb::descFld right) {
  const auto normalized = normalizedOperandType(left, right);
  // `uint8_t op uint8_t` promuje sie w C++ do `int`, a wariant zapamietuje wlasnie `int`.
  // Ta sama promocja obowiazuje potege typu dokladnego (`exactPower` mnozy tym samym
  // `operator*`), wiec `bajt ^ k` dla k >= 1 takze jest INTEGER.
  return normalized == rdb::BYTE ? rdb::INTEGER : normalized;
}

std::optional<rdb::descFld> functionResultType(const std::string_view name, const std::optional<rdb::descFld> argumentType) {
  const auto key = lowercased(std::string(name));

  if (key == "to_integer") return rdb::INTEGER;
  if (key == "to_float") return rdb::FLOAT;
  if (key == "to_double") return rdb::DOUBLE;
  if (key == "to_string") return rdb::STRING;

  // Predykaty i dlugosc napisu sa liczbami calkowitymi NIEZALEZNIE od typu argumentu —
  // patrz isnull(), isZeroValue() i stringLength() w expressionEvaluator.
  if (key == "isnull" || key == "iszero" || key == "isnonzero" || key == "length") return rdb::INTEGER;

  // `Abs` liczy wprost na wariancie (nie przez callFun, zeby nie gubic mianownika
  // RATIONAL), ale typu argumentu takze nie zmienia.
  if (key == "abs") return argumentType;

  // `null2zero` przepuszcza wartosc nie-NULL BEZ ZMIANY TYPU, wiec typem statycznym jest typ
  // argumentu. Dla wartosci NULL ewaluator klada na stos calkowite zero — patrz komentarz
  // przy tej galezi w inferExpressionShape().
  if (key == "null2zero") return argumentType;

  if (isRealValuedFunction(key)) return rdb::DOUBLE;
  if (typePreservingFunctions().contains(key)) return argumentType;

  return std::nullopt;
}

bool isExactType(const rdb::descFld type) {
  return type == rdb::BYTE || type == rdb::INTEGER || type == rdb::UINT || type == rdb::RATIONAL;
}

exprShapeResult inferExpressionShape(const std::list<token> &program, const exprFieldShapeFn &shapeOfField,
                                     const exprWindowShapeFn &shapeOfWindow) {
  if (program.empty()) return {.status = exprShapeStatus::unknown, .shape = {}};

  std::vector<exprShape> stack;
  stack.reserve(program.size());

  const auto unknown  = exprShapeResult{.status = exprShapeStatus::unknown, .shape = {}};
  const auto illTyped = exprShapeResult{.status = exprShapeStatus::illTyped, .shape = {}};

  for (const auto &tk : program) {
    const command_id cmd = tk.getCommandID();

    // Operandy zdejmujemy PRZED rozpoznaniem operacji — tak samo jak robi to petla
    // expressionEvaluator::eval(). Pusty stos znaczy program bez wartosci, a nie „nie wiadomo".
    exprShape left;
    exprShape right;
    const int operands = isBinaryShapeOperator(cmd) ? 2 : (cmd == CALL || cmd == CALL2 || cmd == NEGATE || cmd == NOT) ? 1 : 0;
    if (operands > 0) {
      if (std::cmp_less(stack.size(), operands)) return illTyped;
      right = stack.back();
      stack.pop_back();
      if (operands == 2) {
        left = stack.back();
        stack.pop_back();
      }
    }

    switch (cmd) {
      case PUSH_VAL: {
        if (const auto *text = std::get_if<std::string>(&tk.getVT())) {
          stack.push_back(stringShape(static_cast<int>(text->length())));
          break;
        }
        const auto index = static_cast<rdb::descFld>(tk.getVT().index());
        // Literal `null` nie istnieje w gramatyce, a pary INTPAIR/IDXPAIR nie sa wartosciami
        // wyrazenia — obie postaci znacza, ze program nie jest programem WARTOSCI.
        if (index > rdb::DOUBLE) return unknown;
        stack.push_back(numericShape(index));
      } break;

      case PUSH_ID: {
        const auto *reference = std::get_if<std::pair<std::string, int>>(&tk.getVT());
        if (reference == nullptr) return unknown;
        const auto shape = shapeOfField ? shapeOfField(reference->first, reference->second) : std::nullopt;
        if (!shape.has_value()) return unknown;
        stack.push_back(*shape);
      } break;

      // Postaci PRZEDROZWIAZANIOWE odwolania do pola. Po resolveFieldReferences() nie ma ich
      // w planie; przed nim nie niosa pary (strumien, slot), wiec ksztaltu nie da sie ustalic.
      case PUSH_ID1:
      case PUSH_ID2:
      case PUSH_ID3:
      case PUSH_ID4:
      case PUSH_ID5:
      case PUSH_IDX:
        return unknown;

      case WINDOW_MIN:
      case WINDOW_MAX:
      case WINDOW_AVG:
      case WINDOW_SUM: {
        // Po resolveWindowAggregates() token jest LISCIEM: niesie indeks grupy i kladzie
        // gotowy wynik okna. Przed tym przebiegiem niesie szerokosc okna i poprzedza go
        // PUSH_ID argumentu — obie liczby sa zwyklym `int`, wiec etapu po samym tokenie
        // rozpoznac nie sposob. Rozstrzyga wolajacy: bez tablicy grup nie ma odpowiedzi.
        const auto *groupIndex = std::get_if<int>(&tk.getVT());
        if (groupIndex == nullptr || !shapeOfWindow) return unknown;
        const auto shape = shapeOfWindow(*groupIndex);
        if (!shape.has_value()) return unknown;
        stack.push_back(*shape);
      } break;

      case ADD: {
        // Konkatenacja: normalize() podnosi operand liczbowy do STRING, bo STRING stoi
        // w wariancie wyzej niz kazdy typ liczbowy. Szerokoscia wyniku jest suma szerokosci,
        // przy czym liczba wnosi zero.
        if (left.rtype == rdb::STRING || right.rtype == rdb::STRING) {
          stack.push_back(stringShape(stringBytes(left) + stringBytes(right)));
          break;
        }
        if (left.rtype > rdb::DOUBLE || right.rtype > rdb::DOUBLE) return illTyped;
        stack.push_back(numericShape(arithmeticValueType(left.rtype, right.rtype)));
      } break;

      case SUBTRACT:
      case MULTIPLY:
      case DIVIDE:
      case POWER:
        // Operand tekstowy pod operatorem liczbowym jest bledem WYKONANIA i tak ma zostac:
        // ewaluator rzuca `Operator '<op>' not defined for string operands`. Analizator
        // odmawia ksztaltu, wiec pole zostaje przy sentinelu i moment zgloszenia bledu
        // nie zmienia sie ani o krok.
        if (left.rtype > rdb::DOUBLE || right.rtype > rdb::DOUBLE) return illTyped;
        stack.push_back(numericShape(arithmeticValueType(left.rtype, right.rtype)));
        break;

      case CMP_EQUAL:
      case CMP_NOT_EQUAL:
      case CMP_LT:
      case CMP_GT:
      case CMP_LE:
      case CMP_GE: {
        // Porownanie NIE promuje BYTE: `is_eq` zapisuje wprost `uint8_t(1)`/`uint8_t(0)`,
        // wiec wynikiem jest typ znormalizowany. Porownanie napisow daje `"1"`/`"0"`,
        // czyli napis o szerokosci jednego bajtu.
        const auto normalized = normalizedOperandType(left.rtype, right.rtype);
        if (normalized == rdb::STRING) {
          stack.push_back(stringShape(1));
          break;
        }
        if (normalized > rdb::DOUBLE) return illTyped;
        stack.push_back(numericShape(normalized));
      } break;

      case AND:
      case OR:
        // `is_logic_and`/`is_logic_or` oddaja wynik w typie operandu wskazanego przez
        // logicResultTypeRef(): LEWEGO, chyba ze lewy jest NULL — wtedy prawego. Typ zalezy
        // wiec od WARTOSCI i jednego statycznego nie ma. Operatory logiczne zyja w regule
        // `term_logic`, czyli w warunku RULE, a nie w liscie SELECT — deskryptora nie
        // opisuja i ta niejednoznacznosc nie dociera do artefaktu. Analizator podaje typ
        // lewego operandu i zaznacza to komentarzem, zamiast udawac, ze pytanie jest
        // rozstrzygniete.
        if (left.rtype > rdb::STRING || right.rtype > rdb::STRING) return illTyped;
        stack.push_back(left.rtype == rdb::STRING ? stringShape(1) : left);
        break;

      case NEGATE:
        // `neg()` ZACHOWUJE typ: dla `uint8_t` liczy `~a` i oddaje `uint8_t`, dla `unsigned`
        // takze `unsigned`. Promocji tu nie ma, inaczej niz przy operatorach dwuargumentowych.
        // Operand tekstowy jest bledem wykonania (`Operator 'negate' not defined for string`).
        if (right.rtype > rdb::DOUBLE) return illTyped;
        stack.push_back(right);
        break;

      case NOT:
        // `logic_not()` oddaje wynik przez logicResultAsType() w typie ARGUMENTU; dla napisu
        // jest to `"1"` albo `"0"`, czyli jeden bajt.
        if (right.rtype > rdb::STRING) return illTyped;
        stack.push_back(right.rtype == rdb::STRING ? stringShape(1) : right);
        break;

      case CALL:
      case CALL2: {
        const auto name = lowercased(tk.getStr_());
        if (name == "to_string") {
          // Szerokosc zadeklarowana `to_string(expr : N)` siedzi w tokenie jako IDXPAIR —
          // jest DEKLARACJA pola, a nie wartoscia na stosie (patrz rqlFunctions.hpp).
          int width = kToStringDefaultWidth;
          if (cmd == CALL2)
            if (const auto *declared = std::get_if<std::pair<std::string, int>>(&tk.getVT())) width = declared->second;
          stack.push_back(stringShape(width));
          break;
        }

        // `null2zero` jest JEDYNA funkcja, ktora przepuszcza napis bez zmiany: `isnull(b) ?
        // 0 : b`. Zachowuje wiec takze szerokosc pola zrodlowego.
        if (name == "null2zero") {
          stack.push_back(right);
          break;
        }

        // Kombinacja nieobslugiwana — zatrzymuje kompilacje, zamiast wydac zla wartosc.
        // Stoi PRZED functionResultType(), bo pytanie nie brzmi „jakiego typu jest wynik",
        // tylko „czy ten wynik w ogole da sie policzyc".
        if (auto refusal = rejectedIrrationalOverExact(name, right.rtype); !refusal.empty())
          return {.status = exprShapeStatus::rejected, .shape = {}, .reason = std::move(refusal)};

        if (isRealValuedFunction(name) && right.rtype > rdb::DOUBLE) return illTyped;

        const auto resultType = functionResultType(name, right.rtype);
        // Nazwa spoza tabeli nie dociera do planu: compiler::checkFunctionCalls() odrzuca ja
        // przez `Check result:`. Zostaje dla programow budowanych z pominieciem kompilatora.
        if (!resultType.has_value()) return unknown;
        // `Abs` i funkcje matematyczne zachowuja typ argumentu, ale nad napisem sa bledem
        // wykonania — programu bez wartosci nie typujemy. `isnull`, `IsZero`, `IsNonZero`
        // i `Length` daja INTEGER niezaleznie od argumentu i przechodza tedy bez zmian.
        if (*resultType == rdb::STRING) return illTyped;
        stack.push_back(numericShape(*resultType));
      } break;

      default:
        // Token spoza zestawu ewaluatora (PUSH_STREAM, PUSH_TSCAN, COUNT, STREAM_*...) —
        // arytmetyki stosu dla niego nie znamy, wiec odmawiamy odpowiedzi zamiast zgadywac.
        return unknown;
    }
  }

  if (stack.size() != 1) return illTyped;

  auto result = stack.front();
  // Napis o zerowej szerokosci (`SELECT ''`) dostaje szerokosc domyslna `to_string` — ta sama
  // regula, ktora stosowal inferStringWidth(), zeby nie powstalo pole `STRING[0]`.
  if (result.rtype == rdb::STRING && result.rlen * result.rarray <= 0) result = stringShape(kToStringDefaultWidth);

  return exprShapeResult{.status = exprShapeStatus::resolved, .shape = result};
}
