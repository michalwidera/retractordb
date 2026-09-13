import VersoManual

open Verso.Genre Manual
open Verso.Code.External

set_option verso.exampleProject "."

#doc (Manual) "Twierdzenie 2: rozplot spełnia warunki Fraenkela" =>
%%%
file := "deinterleave"
%%%

To twierdzenie odpowiada Twierdzeniu 2 w polskiej dokumentacji i dowodowi
`De-interleave satisfies Fraenkel's conditions` w długiej publikacji.
Przy dodatnich całkowitych `a` i `b` pozycje reszty są wartościami
`floor(n*(a+b)/a)`, a pozycje odzyskiwanego składnika są wartościami
`ceil(n*(a+b)/b)-1`. Są to dokładnie wzory z dokumentacji po wyciągnięciu
całkowitej części `n` i przeindeksowaniu drugiej sekwencji o jeden.

Pierwszy krok to podział dodatnich pozycji strumienia przeplecionego
między dwa ciągi Beatty'ego: resztę z funkcją podłogi i odzyskiwaną
składową z funkcją sufitu. Symetryczna różnica obu zbiorów pozycji jest
równa zbiorowi dodatnich liczb całkowitych, więc zbiory są rozłączne i
razem pokrywają wszystkie dodatnie pozycje. Moduł pomocniczy
`Profs/BeattyPartition.lean` dowodzi też, że obie funkcje pozycji są
ściśle rosnące, a pozycja zero należy do reszty.

```anchor partition -showProofStates (module := Profs.BeattyPartition)
theorem positive_positions_partition (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    residueSet a b ∆ recoveredSet a b = {j : ℤ | 0 < j} := by
  exact beattySeq_symmDiff_beattySeq'_pos (slopes_conjugate a b ha hb)
```

Następnie lematy
`progress_residue` i `progress_original` ustalają, którą gałąź
wybiera dokładnie równanie przeplotu `interleaveAt` dla obu wzorów
indeksów. Poniższe twierdzenia mówią, że odczyty zwracają pierwotne
krotki z obu składowych, we właściwej kolejności.

Lematy `residue_index_eq_position` i `original_index_eq_position`
utożsamiają bezpośrednie indeksy selektorów z sekwencjami Beatty.
Indeksy każdej składowej są różne, a zbiory obu składowych są rozłączne.

```anchor recover_residue -showProofStates (module := Profs.Deinterleave)
theorem recover_residue (a b n : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} (A : ℕ → α) (B : ℕ → β) :
    interleaveAt a b A B (residueIndex a b n) = .inr (B n) := by
  obtain ⟨h0, h1⟩ := progress_residue a b n ha hb
  simp only [interleaveAt, h0, h1, ↓reduceIte]
  simp [residueIndex]
```

```anchor recover_original -showProofStates (module := Profs.Deinterleave)
theorem recover_original (a b n : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} (A : ℕ → α) (B : ℕ → β) :
    interleaveAt a b A B (originalIndex a b n) = .inl (A n) := by
  obtain ⟨h0, h1⟩ := progress_original a b n ha hb
  simp [interleaveAt, h0, h1]
```

Każda pozycja przeplotu pochodzi z jednej ze składowych. Dla pozycji
zerowej wynika to wprost z definicji, a dla dodatnich z podziału
pozycji Beatty i powyższej zgodności indeksów.

```anchor all_slots -showProofStates (module := Profs.Deinterleave)
theorem every_slot_selected (a b j : ℕ) (ha : 0 < a) (hb : 0 < b) :
    (∃ n, residueIndex a b n = j) ∨
      (∃ n, originalIndex a b n = j) := by
  by_cases hj : j = 0
  · left
    refine ⟨0, ?_⟩
    simp [residueIndex, hj]
  have hjpos : (0 : ℤ) < j := by omega
  have hpart : (j : ℤ) ∈ residueSet a b ∆ recoveredSet a b := by
    rw [positive_positions_partition a b ha hb]
    exact hjpos
  rcases (Set.mem_symmDiff.mp hpart) with ⟨hres, _⟩ | ⟨hrec, _⟩
  · obtain ⟨m, hm, hposition⟩ := hres
    left
    refine ⟨m.toNat, ?_⟩
    have hmcast : ((m.toNat : ℕ) : ℤ) = m := Int.toNat_of_nonneg (le_of_lt hm)
    have hindex := residue_index_eq_position a b m.toNat ha
    rw [hmcast, hposition] at hindex
    exact_mod_cast hindex
  · obtain ⟨m, hm, hposition⟩ := hrec
    right
    let n : ℕ := (m - 1).toNat
    refine ⟨n, ?_⟩
    have hncast : ((n : ℕ) : ℤ) + 1 = m := by
      dsimp [n]
      omega
    have hindex := original_index_eq_position a b n hb
    rw [hncast, hposition] at hindex
    exact_mod_cast hindex
```

Dowód podziału używa uogólnionego twierdzenia Rayleigha z Mathlib,
które działa również dla parametrów wymiernych. Nie formalizujemy
całego historycznego twierdzenia Fraenkela ani pięciu jego warunków.
Nie twierdzimy też, że formalizacja obejmuje bajty, deskryptory lub
politykę materializacji działającego silnika. Te elementy wymagają
oddzielnego modelu wykonania.
