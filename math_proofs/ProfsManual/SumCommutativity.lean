import VersoManual

open Verso.Genre Manual
open Verso.Code.External

set_option verso.exampleProject "."

#doc (Manual) "Stwierdzenie: przemienność sumy strumieni" =>
%%%
file := "sum-commutativity"
%%%

Suma strumieni pobiera próbki obu argumentów z odstępu wyjściowego
równego minimum ich odstępów. Wynikiem jest para atrybutów; po zamianie
argumentów zmienia się tylko kolejność pól w tej parze. `sample_self`
potwierdza, że szybszy strumień jest próbkowany bez zmiany indeksu.

Formalizacja dotyczy wartości i odstępu. Taki sam porządek pól,
nazwy deskryptora oraz mapy `NULL` wymagają dodatkowych założeń o
schemacie i reprezentacji krotek.

```anchor sum_comm -showProofStates (module := Profs.SumCommutativity)
theorem sum_commutativity {α β : Type*} (A : RegularStream α) (B : RegularStream β) :
    (streamSum A B).period = (streamSum B A).period ∧
    ∀ n, (streamSum A B).value n = ((streamSum B A).value n).swap := by
  constructor
  · simp [streamSum, min_comm]
  · intro n
    simp [streamSum, min_comm]
```
