import VersoManual

open Verso.Genre Manual
open Verso.Code.External

set_option verso.exampleProject "."

#doc (Manual) "Wniosek: dokładna odwracalność na liczbach wymiernych" =>
%%%
file := "exact-invertibility"
%%%

Publikacja (wniosek `Exact invertibility on rationals`) i polska
dokumentacja (wniosek po Twierdzeniu 2) mówią, że dla strumieni o wymiernych
odstępach operatory `Θ` i `∼Θ` odtwarzają składowe przeplotu `φ(A,B)`
dokładnie: żadna krotka nie ginie, nie powtarza się ani nie zmienia
kolejności. Para `(φ; Θ, ∼Θ)` zachowuje się więc jak mnożenie i dzielenie,
a para `(Σ; δ)` jak dodawanie i odejmowanie.

`deinterleaveLeft` (`Θ`) czyta strumień przepleciony w pozycjach
`originalIndex`, a `deinterleaveRight` (`∼Θ`) w pozycjach `residueIndex`.
Z Twierdzenia 2 wynika, że złożenie obu selektorów jest bijekcją
z sumy rozłącznej indeksów obu składowych na indeksy strumienia
przeplecionego. Różnowartościowość oznacza, że nic się nie powtarza, a
surjekcja, że nic nie ginie.

```anchor slots_bijective -showProofStates (module := Profs.ExactInvertibility)
theorem selectors_bijective (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    Function.Bijective (Sum.elim (originalIndex a b) (residueIndex a b)) := by
  constructor
  · rintro (n | n) (m | m) h
    · exact congrArg Sum.inl (original_index_injective a b ha hb h)
    · exact absurd h.symm (selectors_disjoint a b m n ha hb)
    · exact absurd h (selectors_disjoint a b n m ha hb)
    · exact congrArg Sum.inr (residue_index_injective a b ha hb h)
  · intro j
    rcases every_slot_selected a b j ha hb with ⟨n, hn⟩ | ⟨n, hn⟩
    · exact ⟨.inr n, hn⟩
    · exact ⟨.inl n, hn⟩
```

Odczyt przez `Θ` i `∼Θ` zwraca całe pierwotne strumienie, element po
elemencie i w pierwotnej kolejności:

```anchor interleave_inverse -showProofStates (module := Profs.ExactInvertibility)
theorem deinterleave_inverts_interleave (a b : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} (A : ℕ → α) (B : ℕ → β) :
    deinterleaveLeft a b (interleaveAt a b A B) = (fun n => .inl (A n)) ∧
    deinterleaveRight a b (interleaveAt a b A B) = (fun n => .inr (B n)) := by
  constructor
  · funext n
    exact recover_original a b n ha hb A B
  · funext n
    exact recover_residue a b n ha hb A B
```

Dla pary `(Σ; δ)` model przyjmuje różnicę z `debs/main-debs.tex`:
`C − Δ_t` wybiera `c` o indeksie `⌈n*Δ_t/Δ_C⌉` i wymaga `Δ_t ≥ Δ_C`
(`rateDiff`). Suma ma odstęp `min(Δ_a, Δ_b)`, więc oba odstępy składowych
spełniają ten warunek. Lemat `resample_roundtrip` pokazuje, że wybór
indeksu sufitem w `δ` i podłogą w `Σ` daje z powrotem `n`. Dlatego
`Σ(A,B) − Δ_a` z rzutem na atrybuty `A` odtwarza `A`, a `Σ(A,B) − Δ_b` z
rzutem na atrybuty `B` odtwarza `B`, niezależnie od tego, która składowa
jest szybsza:

```anchor sum_inverse -showProofStates (module := Profs.ExactInvertibility)
theorem diff_recovers_sum {α β : Type*} (A : RegularStream α) (B : RegularStream β)
    (hA : 0 < A.period) (hB : 0 < B.period) :
    (rateDiff (streamSum A B) A.period).period = A.period ∧
    (∀ n, ((rateDiff (streamSum A B) A.period).value n).1 = A.value n) ∧
    (rateDiff (streamSum A B) B.period).period = B.period ∧
    (∀ n, ((rateDiff (streamSum A B) B.period).value n).2 = B.value n) := by
  have hmin : 0 < min A.period B.period := lt_min hA hB
  refine ⟨rfl, fun n => ?_, rfl, fun n => ?_⟩
  · simp only [rateDiff, streamSum]
    rw [resample_roundtrip A.period _ hmin (min_le_left _ _) n]
  · simp only [rateDiff, streamSum]
    rw [resample_roundtrip B.period _ hmin (min_le_right _ _) n]
```

Sama różnica zwraca pełną krotkę sumy. Do odtworzenia składowej potrzebny
jest jeszcze rzut `π` na jej atrybuty.

Źródło: `Profs/ExactInvertibility.lean`. Formalizacja dotyczy indeksów i
wartości krotek. „Bit w bit” w publikacji dotyczy reprezentacji w silniku
(deskryptory, mapy `NULL`, materializacja), której ten model nie obejmuje.
