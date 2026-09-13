import Profs.EventOrder

namespace Profs

/- Indeks wyboru z A (floor(n*z)) i z B (n - floor(n*z)) dla z = b/(a+b). -/
def indexA (a b n : ℕ) : ℕ := n * b / (a + b)
def indexB (a b n : ℕ) : ℕ := n - indexA a b n

-- ANCHOR: interleave_map
theorem interleaveAt_map (a b : ℕ) {α β : Type*} (A : ℕ → α) (B : ℕ → β) (n : ℕ) :
    interleaveAt a b A B n = Sum.map A B (interleaveAt a b id id n) := by
  dsimp only [interleaveAt]
  split_ifs <;> rfl
-- ANCHOR_END: interleave_map

theorem interleaveAt_id (a b n : ℕ) :
    interleaveAt a b id id n =
      if indexA a b n = indexA a b (n + 1) then .inr (indexB a b n) else .inl (indexA a b n) :=
  rfl

theorem indexA_le (a b n : ℕ) (ha : 0 < a) : indexA a b n ≤ n := by
  have hs : 0 < a + b := by omega
  calc
    indexA a b n ≤ n * (a + b) / (a + b) :=
      Nat.div_le_div_right (Nat.mul_le_mul_left n (by omega))
    _ = n := Nat.mul_div_cancel n hs

/- Przyrost d_n indeksu A wynosi 0 albo 1. -/
theorem indexA_step (a b n : ℕ) (ha : 0 < a) :
    indexA a b n ≤ indexA a b (n + 1) ∧ indexA a b (n + 1) ≤ indexA a b n + 1 := by
  have hs : 0 < a + b := by omega
  constructor
  · exact Nat.div_le_div_right (Nat.mul_le_mul_right b (Nat.le_succ n))
  · calc
      indexA a b (n + 1) ≤ (n * b + (a + b)) / (a + b) := by
        unfold indexA
        exact Nat.div_le_div_right (by rw [Nat.succ_mul]; omega)
      _ = indexA a b n + 1 := Nat.add_div_right _ hs

theorem indexA_multiple (a b m : ℕ) (ha : 0 < a) :
    indexA a b (m * (a + b)) = m * b := by
  unfold indexA
  rw [Nat.mul_right_comm]
  exact Nat.mul_div_cancel _ (by omega)

/- Funkcja startująca od 0, rosnąca o 0 albo 1 i nieograniczona
   przyjmuje każdą wartość m w kroku, w którym rośnie z m do m+1. -/
theorem step_hits (f : ℕ → ℕ) (h0 : f 0 = 0)
    (hstep : ∀ n, f n ≤ f (n + 1) ∧ f (n + 1) ≤ f n + 1)
    (hunb : ∀ m, ∃ N, m < f N) (m : ℕ) :
    ∃ n, f n = m ∧ f (n + 1) = m + 1 := by
  classical
  let N := Nat.find (hunb m)
  have hN : m < f N := Nat.find_spec (hunb m)
  have hNpos : N ≠ 0 := by
    intro hz
    rw [hz, h0] at hN
    omega
  obtain ⟨n, hn⟩ := Nat.exists_eq_add_one_of_ne_zero hNpos
  have hprev : ¬ m < f n := Nat.find_min (hunb m) (by omega)
  have hs := hstep n
  rw [hn] at hN
  exact ⟨n, by omega, by omega⟩

theorem step_order (f : ℕ → ℕ) (hstep : ∀ n, f n ≤ f (n + 1))
    {n n' : ℕ} (hlt : n < n') (hinc : f (n + 1) = f n + 1) : f n < f n' := by
  have hmono : Monotone f := monotone_nat_of_le_succ hstep
  have := hmono (show n + 1 ≤ n' by omega)
  omega

/- Wybór z A w kroku n oznacza wzrost indeksu A z m do m+1. -/
theorem selects_A_iff (a b n m : ℕ) (ha : 0 < a) :
    interleaveAt a b id id n = .inl m ↔
      indexA a b n = m ∧ indexA a b (n + 1) = m + 1 := by
  have := indexA_step a b n ha
  rw [interleaveAt_id]
  split_ifs with h <;> simp <;> omega

theorem indexB_step (a b n : ℕ) (ha : 0 < a) :
    indexB a b n ≤ indexB a b (n + 1) ∧ indexB a b (n + 1) ≤ indexB a b n + 1 := by
  have := indexA_step a b n ha
  have := indexA_le a b (n + 1) ha
  unfold indexB
  omega

/- Wybór z B w kroku n oznacza wzrost indeksu B z m do m+1. -/
theorem selects_B_iff (a b n m : ℕ) (ha : 0 < a) :
    interleaveAt a b id id n = .inr m ↔
      indexB a b n = m ∧ indexB a b (n + 1) = m + 1 := by
  have := indexA_step a b n ha
  have := indexA_le a b n ha
  have := indexA_le a b (n + 1) ha
  rw [interleaveAt_id]
  unfold indexB
  split_ifs with h <;> simp <;> omega

theorem indexB_multiple (a b m : ℕ) (ha : 0 < a) :
    indexB a b (m * (a + b)) = m * a := by
  unfold indexB
  rw [indexA_multiple a b m ha, Nat.mul_add]
  omega

/- Wspólny szkielet dla obu składowych: indeks wyboru f rośnie o 0 albo 1,
   a wybór w kroku n z indeksem m to dokładnie wzrost f z m do m+1. -/
theorem covering_of_step (sel : ℕ → ℕ → Prop) (f : ℕ → ℕ) (h0 : f 0 = 0)
    (hstep : ∀ n, f n ≤ f (n + 1) ∧ f (n + 1) ≤ f n + 1)
    (hunb : ∀ m, ∃ N, m < f N)
    (hsel : ∀ n m, sel n m ↔ f n = m ∧ f (n + 1) = m + 1) :
    (∀ m, ∃! n, sel n m) ∧ (∀ n n' m m', n < n' → sel n m → sel n' m' → m < m') := by
  have order : ∀ n n' m m', n < n' → sel n m → sel n' m' → m < m' := by
    intro n n' m m' hlt hn hn'
    obtain ⟨hm, hinc⟩ := (hsel n m).mp hn
    obtain ⟨hm', -⟩ := (hsel n' m').mp hn'
    have := step_order f (fun n => (hstep n).1) hlt (by omega)
    omega
  refine ⟨fun m => ?_, order⟩
  obtain ⟨n, hn⟩ := step_hits f h0 hstep hunb m
  refine ⟨n, (hsel n m).mpr hn, fun n' hn' => ?_⟩
  rcases Nat.lt_trichotomy n' n with hlt | heq | hlt
  · exact absurd (order n' n m m hlt hn' ((hsel n m).mpr hn)) (lt_irrefl m)
  · exact heq
  · exact absurd (order n n' m m hlt ((hsel n m).mpr hn) hn') (lt_irrefl m)

-- ANCHOR: covering
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
-- ANCHOR_END: covering

end Profs
