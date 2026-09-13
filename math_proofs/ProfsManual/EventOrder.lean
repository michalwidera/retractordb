import VersoManual

open Verso.Genre Manual
open Verso.Code.External

set_option verso.exampleProject "."

#doc (Manual) "Stwierdzenie: zaburzenie kolejności zdarzeń" =>
%%%
file := "event-order"
%%%

Kontrprzykład z publikacji używa dwóch strumieni o odstępach 3 i 2
sekundy. W przeplocie pozycja 6 zawiera piąty rekord strumienia
dwusekundowego, którego czas zdarzenia wynosi 10 sekund. Następna
pozycja 7 zawiera trzeci rekord strumienia trzysekundowego z czasem
zdarzenia 9 sekund. Indeks wyjściowy rośnie, a czas zdarzenia maleje.

Najpierw Lean sprawdza cały prefiks strumienia
`Tau = φ(Epsilon, Alfa) = {1,2,a,3,b,4,5,c,6,d,...}` z publikacji. `.inl m`
to rekord Epsilon o indeksie `m` (`a`, `b`, `c`, ...), a `.inr m` to rekord
Alfa o indeksie `m` (`1`, `2`, `3`, ...):

```anchor tau_prefix -showProofStates (module := Profs.EventOrder)
theorem tau_prefix :
    (List.range 10).map (interleaveAt 3 2 id id) =
      [.inr 0, .inr 1, .inl 0, .inr 2, .inl 1, .inr 3, .inr 4, .inl 2, .inr 5, .inl 3] := by
  decide
```

`interleaveAt` implementuje dokładnie wybór gałęzi przez porównanie
dwóch podłóg z równania przeplotu. Dowód jest skończonym obliczeniem,
które Lean sprawdza przez `decide`.

```anchor event_order -showProofStates (module := Profs.EventOrder)
theorem event_order_counterexample :
    interleaveAt 3 2 threeSecondEvents twoSecondEvents 6 = .inr 10 ∧
    interleaveAt 3 2 threeSecondEvents twoSecondEvents 7 = .inl 9 ∧
    6 < 7 ∧ 9 < 10 := by
  decide
```
