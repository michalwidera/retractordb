#include "exprSimplify.hpp"

#include <algorithm>  // std::max, std::ranges::transform
#include <cctype>     // std::tolower
#include <exception>
#include <iterator>
#include <set>
#include <utility>  // std::move, std::pair
#include <variant>
#include <vector>

#include <boost/rational.hpp>

#include "expressionEvaluator.hpp"

namespace {

/// Indeks wariantu descFldVT i enum descFld mają tę samą kolejność. Na tej tożsamości opiera
/// się już normalize() w expressionEvaluator (castFldVT po a.index()); tutaj jest tylko
/// nazwana wprost.
rdb::descFld typeOfConstant(const rdb::descFldVT &value) { return static_cast<rdb::descFld>(value.index()); }

/// Typy o arytmetyce DOKŁADNEJ i łącznej: całkowite (dodawanie i mnożenie pozostają łączne
/// także w arytmetyce modulo 2^n) oraz wymierne.
///
/// FLOAT i DOUBLE są poza zbiorem świadomie: reasocjacja zmienia tam liczbę zaokrągleń.
/// Dla float x = 2^24 mamy (x+1)+1 == x, ale x+2 == x+2 — przepisanie zmieniłoby wynik.
bool isExact(rdb::descFld type) {
  return type == rdb::BYTE || type == rdb::INTEGER || type == rdb::UINT || type == rdb::RATIONAL;
}

bool isNumeric(rdb::descFld type) { return type <= rdb::DOUBLE; }

bool isConstantEqualTo(const rdb::descFldVT &value, int reference) {
  return std::visit(Overload{[reference](uint8_t v) { return v == reference; },                          //
                             [reference](int v) { return v == reference; },                              //
                             [reference](unsigned v) { return v == static_cast<unsigned>(reference); },  //
                             [reference](boost::rational<int> v) { return v == boost::rational<int>(reference); },
                             [](const auto &) { return false; }},
                    value);
}

/// Nazwa funkcji złożona do małych liter — gramatyka dopuszcza `Sqrt` i `sqrt`, a ewaluator
/// dopasowuje nazwy właśnie po złożeniu wielkości liter.
std::string lowercased(std::string text) {
  std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

/// Typ wyniku wywołania funkcji. Nieznana nazwa daje nullopt, co blokuje reguły B i C —
/// odmowa uproszczenia jest zawsze bezpieczna, zgadywanie typu nie jest.
std::optional<rdb::descFld> typeOfCall(const token &call, std::optional<rdb::descFld> argumentType) {
  // Funkcje matematyczne zachowują typ argumentu (callFun liczy w double i rzutuje z powrotem).
  static const std::set<std::string> typePreserving{"floor", "ceil", "sqrt", "round", "sin",
                                                    "cos",   "tan",  "log",  "log2",  "trunc"};
  const auto name = lowercased(call.getStr_());
  if (name == "to_integer") return rdb::INTEGER;
  if (name == "to_float") return rdb::FLOAT;
  if (name == "to_double") return rdb::DOUBLE;
  if (name == "to_string") return rdb::STRING;
  if (name == "isnull") return rdb::INTEGER;
  // Funkcje dopisane 2026-08-30. Bez tych czterech wierszy typ wychodzil nullopt, czyli
  // „nie wiadomo" — odpowiedz bezpieczna, ale blokujaca reguly B i C w kazdym wyrazeniu,
  // ktore ich uzywa. Zadna z nich nie liczy w double przez callFun, wiec nie naleza do
  // typePreserving mimo ze `Abs` zachowuje typ argumentu.
  if (name == "abs") return argumentType;
  // IsZero/IsNonZero/Length zwracaja INTEGER NIEZALEZNIE od typu argumentu — predykat 0/1
  // i dlugosc napisu sa liczbami calkowitymi, a nie wartoscia w typie wejscia.
  if (name == "iszero" || name == "isnonzero" || name == "length") return rdb::INTEGER;
  if (typePreserving.contains(name)) return argumentType;
  return std::nullopt;
}

/// Wynik operatora arytmetycznego — odwzorowanie normalize(): wygrywa wyższy indeks wariantu.
std::optional<rdb::descFld> arithmeticResultType(std::optional<rdb::descFld> left, std::optional<rdb::descFld> right) {
  if (!left.has_value() || !right.has_value()) return std::nullopt;
  return std::max(*left, *right);
}

/// Liczy program pozbawiony odwołań do payloadu PRODUKCYJNYM ewaluatorem. To jedyne miejsce,
/// w którym powstaje wartość zwiniętej stałej — kompilator nie ma własnej kopii arytmetyki,
/// więc zwijanie nie może się rozjechać z wykonaniem.
///
/// nullopt oznacza „zostaw program w spokoju": albo ewaluator rzucił (np. `'a'-'b'`, nieznana
/// funkcja) i błąd ma polecieć w wykonaniu jak dotąd, albo wynik jest wartością, której nie da
/// się z powrotem wstawić do programu jako literał.
std::optional<rdb::descFldVT> foldConstants(const std::list<token> &program) {
  expressionEvaluator evaluator;
  try {
    auto value      = evaluator.eval(program, nullptr);
    const auto type = typeOfConstant(value);
    if (type == rdb::NULLTYPE || type == rdb::INTPAIR || type == rdb::IDXPAIR) return std::nullopt;
    return value;
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

/// Ogon postaci `(E op c)` albo `(c op E)` — materiał dla reguł B i C.
struct constantTail {
  command_id op;                         ///< operacja węzła: ADD, SUBTRACT albo MULTIPLY
  rdb::descFldVT constant;               ///< stały operand
  bool constantOnLeft;                   ///< true dla `(c op E)`, false dla `(E op c)`
  std::list<token> base;                 ///< program podwyrażenia E
  std::optional<rdb::descFld> baseType;  ///< typ statyczny E, o ile znany
};

#if aggressive_expr_optimization
/// Podwyrażenie zwinięte już do postaci `E ^ k` — materiał dla reguły D.
struct powerForm {
  std::list<token> base;  ///< program podwyrażenia E
  int exponent;           ///< wykładnik, zawsze >= 2
};
#endif

/// Węzeł drzewa wyrażenia odtworzonego ze strumienia ONP.
struct node {
  std::list<token> program;
  std::optional<rdb::descFldVT> constant;  ///< wartość, gdy CAŁE podwyrażenie jest stałe
  std::optional<rdb::descFld> type;
  std::optional<constantTail> tail;
#if aggressive_expr_optimization
  std::optional<powerForm> asPower;  ///< ustawione tylko dla węzłów zbudowanych regułą D
#endif
};

node constantNode(rdb::descFldVT value) {
  node result;
  result.program.emplace_back(PUSH_VAL, value);
  result.type     = typeOfConstant(value);
  result.constant = std::move(value);
  return result;
}

/// Operator, którym trzeba zwinąć obie stałe, żeby `(E op1 c1) op2 c2` dało `E op1 (c1 ? c2)`.
///
/// Dla ogona z odejmowaniem po prawej stronie operator się odwraca: `E-c1-c2 == E-(c1+c2)`,
/// `E-c1+c2 == E-(c1-c2)`. Gdy stała stoi po lewej, odwrócenia nie ma: `c1-E-c2 == (c1-c2)-E`.
std::optional<command_id> foldOperator(command_id tailOp, command_id op, bool constantOnLeft) {
  if (tailOp == MULTIPLY) return op == MULTIPLY ? std::optional<command_id>{MULTIPLY} : std::nullopt;
  if (tailOp != ADD && tailOp != SUBTRACT) return std::nullopt;
  if (op != ADD && op != SUBTRACT) return std::nullopt;
  if (tailOp == SUBTRACT && !constantOnLeft) return op == ADD ? SUBTRACT : ADD;
  return op;
}

/// Reguła B — łączy dwie stałe rozdzielone podwyrażeniem.
std::optional<node> reassociate(const node &left, const rdb::descFldVT &constant, command_id op) {
  if (!left.tail.has_value()) return std::nullopt;
  const auto &tail = *left.tail;
  if (!tail.baseType.has_value()) return std::nullopt;

  const auto tailConstantType = typeOfConstant(tail.constant);
  const auto constantType     = typeOfConstant(constant);

  const bool exactArithmetic = isExact(*tail.baseType) && isExact(tailConstantType) && isExact(constantType);
  // Konkatenacja jest łączna, ale NIE przemienna, więc przepisujemy wyłącznie ogon po prawej:
  // `E+'a'+'b'` to `E+'ab'`, ale `'a'+E+'b'` już nie zwija się do `'ab'+E`.
  const bool stringConcat = *tail.baseType == rdb::STRING && tailConstantType == rdb::STRING && constantType == rdb::STRING &&
                            !tail.constantOnLeft && tail.op == ADD && op == ADD;
  if (!exactArithmetic && !stringConcat) return std::nullopt;

  const auto folding = foldOperator(tail.op, op, tail.constantOnLeft);
  if (!folding.has_value()) return std::nullopt;

  const std::list<token> foldProgram{token(PUSH_VAL, tail.constant), token(PUSH_VAL, constant), token(*folding)};
  auto value = foldConstants(foldProgram);
  if (!value.has_value()) return std::nullopt;

  node result;
  if (tail.constantOnLeft) {
    result.program.emplace_back(PUSH_VAL, *value);
    result.program.insert(result.program.end(), tail.base.begin(), tail.base.end());
  } else {
    result.program = tail.base;
    result.program.emplace_back(PUSH_VAL, *value);
  }
  result.program.emplace_back(tail.op);
  result.type = arithmeticResultType(tail.baseType, typeOfConstant(*value));
  // Wynik znów ma kształt ogona, więc łańcuch `E+1+1+1` zwija się w jednym przebiegu.
  result.tail = constantTail{tail.op, *value, tail.constantOnLeft, tail.base, tail.baseType};
  return result;
}

/// Reguła C — usuwa element neutralny.
std::optional<node> dropNeutralOperand(const node &left, const rdb::descFldVT &constant, command_id op) {
  if (!left.type.has_value() || !isExact(*left.type)) return std::nullopt;
  // Stała musi mieć TĘ SAMĄ reprezentację co podwyrażenie. Inaczej usunięcie operatora
  // skasowałoby promocję typu: `bajt + 0` daje int, a samo `bajt` zostaje bajtem i dalsza
  // arytmetyka zaczyna zawijać modulo 256.
  if (typeOfConstant(constant) != *left.type) return std::nullopt;

  const bool neutral = ((op == ADD || op == SUBTRACT) && isConstantEqualTo(constant, 0)) ||
                       ((op == MULTIPLY || op == DIVIDE) && isConstantEqualTo(constant, 1));
  if (!neutral) return std::nullopt;
  return left;
}

#if aggressive_expr_optimization
/// Równość SKŁADNIOWA dwóch programów ONP. `token` nie ma operator==, a dokładanie go do
/// jego publicznego API tylko dla tego porównania byłoby zmianą szerszą niż potrzeba.
bool sameProgram(const std::list<token> &left, const std::list<token> &right) {
  return std::ranges::equal(left, right, [](const token &a, const token &b) {
    return a.getCommandID() == b.getCommandID() && a.getVT() == b.getVT();
  });
}

/// Węzeł `E ^ exponent` o znanej postaci potęgowej, gotowy na kolejny krok łańcucha.
node powerNode(std::list<token> base, int exponent, rdb::descFld baseType) {
  node result;
  result.program = base;
  result.program.emplace_back(PUSH_VAL, exponent);
  result.program.emplace_back(POWER);
  // Wykładnik jest literałem INTEGER, więc normalize() podniesie do niego podstawę —
  // stąd typ wyniku jest maksimum z obu, a nie samym typem podstawy. Dla BYTE daje to
  // INTEGER, czyli dokładnie to, co daje `bajt * bajt` (promocja do int w operator*).
  result.type    = std::max(baseType, rdb::INTEGER);
  result.asPower = powerForm{std::move(base), exponent};
  return result;
}

/// Reguła D — powtórzony czynnik zwija się do potęgi: `a*a` to `a^2`, `a*a*a` to `a^3`.
///
/// Wygrana jest w liczbie ODCZYTÓW pola, nie w liczbie mnożeń: `a*a` czyta payload dwa
/// razy, `a^2` raz. Mnożeń jest tyle samo.
///
/// Warunkiem jest DOKŁADNA arytmetyka czynnika. Dla FLOAT i DOUBLE przepisanie byłoby
/// niepoprawne — `x*x` to jedno mnożenie IEEE, a `x^2` liczy się przez std::pow, który nie
/// ma gwarancji poprawnego zaokrąglenia. Dla typów dokładnych różnicy nie ma z definicji:
/// exactPower() w expressionEvaluator liczy je tym samym operator*, którego użyłby MULTIPLY,
/// więc zawinięcie modulo 2^n i promocja BYTE do int zostają zachowane. Na tej równości
/// stoi też niezmiennik ablacyjny — przy RDB_OPT_SIMPLIFY_EXPRESSIONS=OFF zostaje `a*a`
/// i musi policzyć dokładnie to samo.
std::optional<node> foldRepeatedFactor(const node &left, const node &right) {
  // Stałe należą do reguły A; `2*2` ma się zwinąć do 4, a nie do `2^2`.
  if (left.constant.has_value() || right.constant.has_value()) return std::nullopt;
  if (!right.type.has_value() || !isExact(*right.type)) return std::nullopt;

  // `a * a` — pierwszy krok łańcucha.
  if (sameProgram(left.program, right.program)) return powerNode(right.program, 2, *right.type);

  // `a^k * a` — kolejny krok. Lewostronna łączność `*` daje wyłącznie to ustawienie stron.
  if (left.asPower.has_value() && sameProgram(left.asPower->base, right.program))
    return powerNode(right.program, left.asPower->exponent + 1, *right.type);

  return std::nullopt;
}
#endif

std::optional<node> rewriteWithConstantOnRight(const node &left, const rdb::descFldVT &constant, command_id op) {
  if (auto rewritten = reassociate(left, constant, op)) return rewritten;
  return dropNeutralOperand(left, constant, op);
}

}  // namespace

std::size_t simplifyExpression(std::list<token> &program, const fieldTypeLookup &typeOfField) {
  if (program.empty()) return 0;

  std::vector<node> stack;
  std::size_t rewrites = 0;

  auto pop = [&stack]() -> std::optional<node> {
    if (stack.empty()) return std::nullopt;
    node result = std::move(stack.back());
    stack.pop_back();
    return result;
  };

  for (const auto &tk : program) {
    const command_id cmd = tk.getCommandID();
    switch (cmd) {
      case PUSH_VAL:
        stack.push_back(constantNode(tk.getVT()));
        break;

      case PUSH_ID:
      case PUSH_ID2: {
        // PUSH_ID niesie parę (nazwa strumienia, indeks pola) — stąd typ. PUSH_ID2 trzyma
        // odwołanie tekstowe i zostaje bez typu; to wyłącza reguły B i C, ale nie zwijanie.
        node leaf;
        leaf.program.push_back(tk);
        if (const auto *reference = std::get_if<std::pair<std::string, int>>(&tk.getVT()))
          leaf.type = typeOfField(reference->first, reference->second);
        stack.push_back(std::move(leaf));
      } break;

      case NEGATE:
      case NOT:
      case CALL:
      case CALL2: {
        auto operand = pop();
        if (!operand.has_value()) return 0;

        node result;
        result.program = std::move(operand->program);
        result.program.push_back(tk);

        if (operand->constant.has_value()) {
          if (auto value = foldConstants(result.program)) {
            stack.push_back(constantNode(std::move(*value)));
            ++rewrites;
            break;
          }
        }
        // NOT normalizuje wartość do 1/0 w typie operandu, ale przez logicResultAsType,
        // a nie przez normalize — nie wchodzi w rachunek typów reguł B i C.
        if (cmd == NEGATE)
          result.type = operand->type;
        else if (cmd == CALL || cmd == CALL2)
          result.type = typeOfCall(tk, operand->type);
        stack.push_back(std::move(result));
      } break;

      case ADD:
      case SUBTRACT:
      case MULTIPLY:
      case DIVIDE:
      // POWER wchodzi tu WYLACZNIE po regule A (zwijanie stalych) i po to, zeby wyrazenie
      // z `^` nie wypadalo na `default: return 0`, blokujac uproszczenia w calym polu.
      // Regul B i C nie dotyka: potegowanie nie jest ani laczne, ani przemienne, wiec
      // `E^c1^c2` nie zwija sie do `E^(c1?c2)`. Trzy warunki nizej pilnuja tego same z
      // siebie — foldOperator() nie zna POWER, dropNeutralOperand() nie zna POWER, a
      // galaz „stala po lewej" wpuszcza tylko ADD i MULTIPLY.
      case POWER:
      case CMP_EQUAL:
      case CMP_NOT_EQUAL:
      case CMP_LT:
      case CMP_GT:
      case CMP_LE:
      case CMP_GE:
      case AND:
      case OR: {
        auto right = pop();
        auto left  = pop();
        if (!right.has_value() || !left.has_value()) return 0;

        std::list<token> combined = left->program;
        combined.insert(combined.end(), right->program.begin(), right->program.end());
        combined.push_back(tk);

        // A — całe podwyrażenie jest stałe.
        if (left->constant.has_value() && right->constant.has_value()) {
          if (auto value = foldConstants(combined)) {
            stack.push_back(constantNode(std::move(*value)));
            ++rewrites;
            break;
          }
        }

#if aggressive_expr_optimization
        // D — powtórzony czynnik jako potęga.
        if (cmd == MULTIPLY) {
          if (auto rewritten = foldRepeatedFactor(*left, *right)) {
            stack.push_back(std::move(*rewritten));
            ++rewrites;
            break;
          }
        }
#endif

        // B i C — stała po prawej.
        if (right->constant.has_value() && !left->constant.has_value()) {
          if (auto rewritten = rewriteWithConstantOnRight(*left, *right->constant, cmd)) {
            stack.push_back(std::move(*rewritten));
            ++rewrites;
            break;
          }
        }

        // Stała po lewej — te same reguły po zamianie stron. Wolno tylko dla przemiennych
        // operatorów i wyłącznie na liczbach: dla łańcuchów `'a'+x` i `x+'a'` to dwa różne
        // wyniki, a mieszany `'a'+liczba` promuje się do konkatenacji.
        if (left->constant.has_value() && !right->constant.has_value() && (cmd == ADD || cmd == MULTIPLY) &&
            right->type.has_value() && isNumeric(*right->type) && isNumeric(typeOfConstant(*left->constant))) {
          if (auto rewritten = rewriteWithConstantOnRight(*right, *left->constant, cmd)) {
            stack.push_back(std::move(*rewritten));
            ++rewrites;
            break;
          }
        }

        node result;
        result.program = std::move(combined);
        if (cmd == ADD || cmd == SUBTRACT || cmd == MULTIPLY || cmd == DIVIDE || cmd == POWER) {
          result.type = arithmeticResultType(left->type, right->type);
          if (cmd != DIVIDE && cmd != POWER && right->constant.has_value() && !left->constant.has_value())
            result.tail = constantTail{cmd, *right->constant, false, std::move(left->program), left->type};
          else if (cmd != DIVIDE && cmd != POWER && left->constant.has_value() && !right->constant.has_value())
            result.tail = constantTail{cmd, *left->constant, true, std::move(right->program), right->type};
        }
        stack.push_back(std::move(result));
      } break;

      default:
        // Token spoza zestawu ewaluatora (PUSH_STREAM, COUNT, PUSH_IDX...) — nie znamy jego
        // arytmetyki stosu, więc nie ruszamy programu w ogóle.
        return 0;
    }
  }

  if (stack.size() != 1 || rewrites == 0) return 0;
  program = std::move(stack.front().program);
  return rewrites;
}

namespace {

/// Wartość na stosie wnioskowania typu wyniku: napis o znanej szerokości albo liczba.
/// Liczba wchodząca w konkatenację ma szerokość 0 — tak samo, jak liczyła ją reguła
/// sprzed 2026-08-30, więc szerokości pól w istniejących planach się nie zmieniają.
struct inferredValue {
  bool isString;
  int width;
};

}  // namespace

std::optional<int> inferStringWidth(const std::list<token> &program, const fieldShapeLookup &shapeOfField) {
  if (program.empty()) return std::nullopt;

  std::vector<inferredValue> stack;

  auto pop = [&stack]() -> std::optional<inferredValue> {
    if (stack.empty()) return std::nullopt;
    const inferredValue result = stack.back();
    stack.pop_back();
    return result;
  };

  for (const auto &tk : program) {
    const command_id cmd = tk.getCommandID();
    switch (cmd) {
      case PUSH_VAL:
        if (const auto *text = std::get_if<std::string>(&tk.getVT()))
          stack.push_back({true, static_cast<int>(text->length())});
        else
          stack.push_back({false, 0});
        break;

      // Odwołanie do pola. Kształt zna wyłącznie PUSH_ID, bo tylko on niesie parę
      // (nazwa strumienia, indeks płaski). Pozostałe postaci są przedrozwiązaniowe:
      // PUSH_ID1/PUSH_ID3 trzymają sam tekst, a PUSH_ID2 wprawdzie parę, ale jej pierwszy
      // element to odwołanie `strumień[offset]`, a nie nazwa. Wchodzą więc jako liczba —
      // i jest to dokładnie stan wiedzy parsera, którego domyślnym typem pola jest INTEGER.
      case PUSH_ID:
      case PUSH_ID1:
      case PUSH_ID2:
      case PUSH_ID3:
      case PUSH_ID4:
      case PUSH_ID5:
      case PUSH_IDX: {
        std::optional<fieldShape> shape;
        if (cmd == PUSH_ID)
          if (const auto *reference = std::get_if<std::pair<std::string, int>>(&tk.getVT()))
            shape = shapeOfField(reference->first, reference->second);
        if (shape.has_value() && shape->type == rdb::STRING)
          stack.push_back({true, shape->width});
        else
          stack.push_back({false, 0});
      } break;

      case CALL:
      case CALL2: {
        if (!pop().has_value()) return std::nullopt;
        if (lowercased(tk.getStr_()) != "to_string") {
          stack.push_back({false, 0});
          break;
        }
        // Szerokość zadeklarowana `to_string(expr : N)` siedzi w tokenie jako IDXPAIR —
        // jest deklaracją pola, a nie wartością na stosie (patrz rqlFunctions.hpp).
        int width = kToStringDefaultWidth;
        if (cmd == CALL2)
          if (const auto *declared = std::get_if<std::pair<std::string, int>>(&tk.getVT())) width = declared->second;
        stack.push_back({true, width});
      } break;

      case NEGATE:
      case NOT:
        if (!pop().has_value()) return std::nullopt;
        stack.push_back({false, 0});
        break;

      // Konkatenacja: `operator+` ewaluatora normalizuje operandy do wyższego indeksu
      // wariantu, a STRING stoi wyżej niż każdy typ liczbowy — więc `'a'+1` daje napis.
      case ADD: {
        auto right = pop();
        auto left  = pop();
        if (!right.has_value() || !left.has_value()) return std::nullopt;
        if (left->isString || right->isString)
          stack.push_back({true, left->width + right->width});
        else
          stack.push_back({false, 0});
      } break;

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
      case OR: {
        auto right = pop();
        auto left  = pop();
        if (!right.has_value() || !left.has_value()) return std::nullopt;
        stack.push_back({false, 0});
      } break;

      default:
        // Token spoza zestawu ewaluatora (PUSH_STREAM, COUNT, PUSH_TSCAN...) — nie znamy
        // jego arytmetyki stosu, więc odmawiamy odpowiedzi zamiast zgadywać.
        return std::nullopt;
    }
  }

  if (stack.size() != 1 || !stack.front().isString) return std::nullopt;
  return stack.front().width > 0 ? stack.front().width : kToStringDefaultWidth;
}
