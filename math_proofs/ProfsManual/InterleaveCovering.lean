import VersoManual

open Verso.Genre Manual
open Verso.Code.External

set_option verso.exampleProject "."

#doc (Manual) "Twierdzenie 1: przeplot jest pokryciem sekwencyjnym" =>
%%%
file := "interleave-covering"
%%%

Publikacja (twierdzenie `Interleave is a sequential covering`) i polska
dokumentacja (Twierdzenie 1) mówią, że przeplot wybiera każdy element
strumienia `A` i każdy element strumienia `B` dokładnie raz, w kolejności,
bez luk i bez powtórzeń.

`interleaveAt a b A B n` to równanie przeplotu dla `z = b/(a+b)`, gdzie
`a` i `b` to odstępy strumieni `A` i `B`. Formalizacja bierze za strumienie
identyczności `id`. Wartość `.inl m` oznacza wtedy wybór elementu `A` o
indeksie `m`, a `.inr m` wybór elementu `B` o indeksie `m`. Wynik
przenosi się na dowolne strumienie przez `Sum.map`:

```anchor interleave_map -showProofStates (module := Profs.InterleaveCovering)
theorem interleaveAt_map (a b : ℕ) {α β : Type*} (A : ℕ → α) (B : ℕ → β) (n : ℕ) :
    interleaveAt a b A B n = Sum.map A B (interleaveAt a b id id n) := by
  dsimp only [interleaveAt]
  split_ifs <;> rfl
```

Dowód idzie tą samą drogą co w publikacji. Indeks wyboru z `A`
(`indexA n = floor(n*z)`) i indeks wyboru z `B` (`indexB n = n - floor(n*z)`)
zaczynają od zera i w każdym kroku rosną o 0 albo 1 (`indexA_step`,
`indexB_step`). Wybór z `A` w kroku `n` z indeksem `m` zachodzi dokładnie
wtedy, gdy `indexA` rośnie w tym kroku z `m` do `m+1`, a dla `B` tak samo
z `indexB` (`selects_A_iff`, `selects_B_iff`). Obie funkcje są
nieograniczone, bo po `m+1` pełnych okresach `a+b` mają wartości `(m+1)*b`
i `(m+1)*a`. Wspólny lemat `covering_of_step` wyprowadza z tych
własności istnienie, jednoznaczność i kolejność wyborów.

Pierwsze dwa człony twierdzenia mówią, że każdy indeks `m` każdej
składowej zostaje wybrany w dokładnie jednym kroku. Dwa ostatnie mówią, że
późniejszy krok wybiera większy indeks. Razem oznacza to, że wybory z każdej
składowej przechodzą przez indeksy `0, 1, 2, ...` po kolei.

```anchor covering -showProofStates (module := Profs.InterleaveCovering)
theorem interleave_sequential_covering (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    (∀ m, ∃! n, interleaveAt a b id id n = .inl m) ∧
    (∀ m, ∃! n, interleaveAt a b id id n = .inr m) ∧
    (∀ n n' m m', n < n' → interleaveAt a b id id n = .inl m →
      interleaveAt a b id id n' = .inl m' → m < m') ∧
    (∀ n n' m m', n < n' → interleaveAt a b id id n = .inr m →
      interleaveAt a b id id n' = .inr m' → m < m') := by
  obtain ⟨onceA, orderA⟩ :=
    covering_of_step (fun n m => interleaveAt a b id id n = .inl m) (indexA a b)
      (by simp [indexA]) (indexA_step a b · ha)
      (fun m => ⟨(m + 1) * (a + b), by rw [indexA_multiple a b _ ha]; nlinarith⟩)
      (selects_A_iff a b · · ha)
  obtain ⟨onceB, orderB⟩ :=
    covering_of_step (fun n m => interleaveAt a b id id n = .inr m) (indexB a b)
      (by simp [indexB, indexA]) (indexB_step a b · ha)
      (fun m => ⟨(m + 1) * (a + b), by rw [indexB_multiple a b _ ha]; nlinarith⟩)
      (selects_B_iff a b · · ha)
  exact ⟨onceA, onceB, orderA, orderB⟩
```

Źródło: `Profs/InterleaveCovering.lean`. Dowód nie korzysta z ciągów
Beatty'ego. Służą one dopiero do analizy rozplotu w Twierdzeniu 2.
