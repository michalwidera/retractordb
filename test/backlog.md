# Backlog silnika

Lista zmian w silniku (`src/`), które trzeba wdrożyć w przyszłości, ale które świadomie nie weszły do sesji, w której się pojawiły. Źródłem wpisu jest zwykle praca nad czymś innym: przy naprawie testu, przeglądzie kodu albo przebiegu ablacji wychodzi pomysł, brakująca reguła lub brakujące sprawdzenie. Zgodnie z `CLAUDE.md` („Surgical edits") taka rzecz nie jest robiona przy okazji - zostaje zapisana tutaj, a decyzja o realizacji należy do człowieka i jest osobnym zadaniem.

Plik jest pamięcią projektu, nie trackerem zgłoszeń. Wpis ma pozwolić podjąć temat po tygodniach bez odtwarzania kontekstu: skąd się wziął, jak wygląda kod dziś, jak go zrealizować i po czym poznać, że jest gotowe.

## Co tu trafia

- Reguły i przebiegi kompilatora, zmiany ewaluatora, runtime'u i klientów (`xqry`, `xtrdb`), odłożone świadomie.
- Brakujące sprawdzenia, które zamieniłyby awarię w wykonaniu na błąd kompilacji.

Nie trafiają tu: defekty psujące wynik lub wywracające proces - te zgłasza się od razu człowiekowi; zmiany dokumentacji; stan kampanii badawczych.

## Format wpisu

Indeksy `B-NNN` są kolejne i nigdy nie są używane ponownie. Wpisu się nie usuwa: po realizacji albo odrzuceniu zmienia się tylko jego status, dopisując commit albo powód. Nowe wpisy dopisuje się na końcu.

```markdown
## B-NNN - tytuł

- **Data:** RRRR-MM-DD
- **Status:** pomysł | przyjęte | w realizacji | zrobione (commit) | odrzucone (powód)
- **Źródło:** skąd wpis się wziął - sesja, test, przebieg
- **Stan obecny:** jak to wygląda w kodzie dziś, z plikami i funkcjami
- **Pomysł na realizację:** konkretne kroki
- **Do rozstrzygnięcia:** ryzyka, skutki uboczne, alternatywy
- **Gotowe gdy:** kryteria sprawdzalne testem
```

---

## B-001 - Reguła C (`x+0 -> x`) pod przełącznikiem `aggressive_expr_optimization`

- **Data:** 2026-09-18
- **Status:** pomysł - do decyzji
- **Źródło:** `it_dot_labels-compile` i `it_dot_labels-dot` oblewały build ablacyjny all-off. Szablon `core[0]+$` dawał dla `$`=0 wyrażenie `core[0]+0`, które znika z planu tylko przy `RDB_OPT_SIMPLIFY_EXPRESSIONS=ON`, więc listing zależał od przełącznika wydajnościowego. Test naprawiono po stronie testu (`SELECT w[$] STREAM ch[2] FROM w`), więc ten wpis nie jest potrzebny, żeby test był zielony. Zostaje jako propozycja zmiany w optymalizatorze.
- **Stan obecny:** reguła C już istnieje. `dropNeutralOperand()` w `src/retractor/lib/exprSimplify.cpp` usuwa `E+0`, `E-0`, `E*1` i `E/1` wyłącznie dla typów o arytmetyce dokładnej i tylko wtedy, gdy stała ma tę samą reprezentację co podwyrażenie (inaczej skasowałaby promocję typu, np. `bajt + 0`). Działa razem z regułami A (zwijanie stałych) i B (reasocjacja ogona stałych) w `simplifyFieldExpressions()`, wołanym w `compiler.cpp` pod `#if RDB_OPT_SIMPLIFY_EXPRESSIONS`, czyli za przełącznikiem ablacyjnym. Za `aggressive_expr_optimization` (domyślnie OFF) stoi dziś wyłącznie reguła D (`a*a -> a^2`). Testy reguły C: `test/UnitTest/test_exprSimplify.cpp` - `drops_neutral_operands`, `drops_neutral_operand_written_on_the_left`, `keeps_neutral_operand_of_a_wider_type`.
- **Pomysł na realizację:**
  1. W `exprSimplify.cpp` objąć jedyne wywołanie `dropNeutralOperand()` (koniec funkcji upraszczającej ogon stałych) warunkiem `#if aggressive_expr_optimization`, tak jak regułę D.
  2. Zaktualizować opis reguł w `exprSimplify.hpp` oraz komentarz i opis `option(aggressive_expr_optimization ...)` w głównym `CMakeLists.txt` (dziś „rule D").
  3. Testy reguły C w `test_exprSimplify.cpp` objąć tym samym warunkiem, a dla domyślnego buildu dodać asercję, że `E+0` zostaje w planie.
  4. Przejrzeć wzorce testów integracyjnych, w których element neutralny znika dziś z listingu.
- **Do rozstrzygnięcia:**
  - Zmienia plany domyślnego buildu: każde `E+0` i `E*1` zostaje w planie i jest liczone w każdym slocie. To regres wydajności domyślnej konfiguracji w zamian za niezależność listingu.
  - Zmienia znaczenie przełącznika: `aggressive_expr_optimization` jest dziś zarezerwowany dla przepisań, które ruszają korpus H9 (`validate_corpus.py::require_main_r3_zero`). Reguła C jest zwykłym uproszczeniem wydajnościowym i pasuje do `RDB_OPT_SIMPLIFY_EXPRESSIONS`.
  - Nie usuwa zależności listingu od konfiguracji, tylko przenosi ją na przełącznik, którego macierz ablacji w CI nie sprawdza.
  - Alternatywa bez zmiany silnika: zasada dla testów - test, który nie dotyczy optymalizatora, nie zawiera wyrażeń, które optymalizator może przepisać (tak naprawiono `it_dot_labels`).
- **Gotowe gdy:** przy domyślnej konfiguracji reguła C nie działa, a przy `aggressive_expr_optimization=ON` działa; `ctest` zielony w Debug i Release; `ninja test_gate` zielona; pełna seria w buildzie all-off zielona (zmiana dotyka optymalizatora).
