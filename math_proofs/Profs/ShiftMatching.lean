import Profs.EventOrder

namespace Profs

def progress (a b n : ℕ) : ℕ := n * b / (a + b)

-- ANCHOR: not_commutative
theorem interleave_starts_with_right (a b : ℕ) (ha : 0 < a) (hb : 0 < b) {α β : Type*}
    (A : ℕ → α) (B : ℕ → β) :
    interleaveAt a b A B 0 = .inr (B 0) ∧ interleaveAt b a B A 0 = .inr (A 0) := by
  have h1 : b / (a + b) = 0 := Nat.div_eq_of_lt (by omega)
  have h2 : a / (b + a) = 0 := Nat.div_eq_of_lt (by omega)
  simp [interleaveAt, h1, h2]
-- ANCHOR_END: not_commutative

theorem progress_shift (a b i k n : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) :
    progress a b (n + i + k) = progress a b n + i := by
  have hs : 0 < a + b := by omega
  have heq : (n + i + k) * b = n * b + (a + b) * i := by
    nlinarith [hmatch]
  simp only [progress, heq]
  exact Nat.add_mul_div_left (n * b) i hs

theorem progress_le_slot (a b n : ℕ) (ha : 0 < a) (hb : 0 < b) :
    progress a b n ≤ n := by
  have hs : 0 < a + b := by omega
  have hprod : n * b ≤ n * (a + b) := by
    exact Nat.mul_le_mul_left n (by omega)
  calc
    progress a b n ≤ n * (a + b) / (a + b) := Nat.div_le_div_right hprod
    _ = n := by simpa [Nat.mul_comm] using Nat.mul_div_cancel_left n hs

-- ANCHOR: shift_values
theorem shift_matching_values (a b i k : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) {α β : Type*} (A : ℕ → α) (B : ℕ → β)
    (n : ℕ) :
    interleaveAt a b (fun j => A (j + i)) (fun j => B (j + k)) n =
      interleaveAt a b A B (n + i + k) := by
  have hp := progress_shift a b i k n ha hb hmatch
  have hp1 := progress_shift a b i k (n + 1) ha hb hmatch
  have hpn := progress_le_slot a b n ha hb
  have hshiftNext : n + i + k + 1 = (n + 1) + i + k := by omega
  simp only [interleaveAt]
  change (if progress a b n = progress a b (n + 1) then
      Sum.inr (B (n - progress a b n + k))
    else Sum.inl (A (progress a b n + i))) =
    (if progress a b (n + i + k) = progress a b (n + i + k + 1) then
      Sum.inr (B (n + i + k - progress a b (n + i + k)))
    else Sum.inl (A (progress a b (n + i + k))))
  rw [hshiftNext, hp, hp1]
  have hindex : n + i + k - (progress a b n + i) = n - progress a b n + k := by
    omega
  simp [hindex]
-- ANCHOR_END: shift_values

def phaseRequirement (j W n : ℕ) (ratio : ℚ) : ℤ :=
  ⌈(((j + 1 + W : ℕ) : ℚ) * ratio)⌉ - 1 - n

theorem phase_requirement_shift (j W t n L : ℕ) (ratio : ℚ)
    (hratio : 0 ≤ ratio) (hmatch : (t : ℚ) * ratio = L) :
    phaseRequirement j W n ratio - L ≤
      phaseRequirement j (W - t) n ratio := by
  have hnat : (j + 1 + W : ℤ) - t ≤ (j + 1 + (W - t) : ℕ) := by
    omega
  have hq : (((j + 1 + W : ℕ) : ℚ) - t) ≤
      ((j + 1 + (W - t) : ℕ) : ℚ) := by
    exact_mod_cast hnat
  have hmul : (((j + 1 + W : ℕ) : ℚ) * ratio - L) ≤
      ((j + 1 + (W - t) : ℕ) : ℚ) * ratio := by
    calc
      ((j + 1 + W : ℕ) : ℚ) * ratio - L =
          (((j + 1 + W : ℕ) : ℚ) - t) * ratio := by rw [← hmatch]; ring
      _ ≤ ((j + 1 + (W - t) : ℕ) : ℚ) * ratio :=
        mul_le_mul_of_nonneg_right hq hratio
  have hceil := Int.ceil_mono hmul
  rw [Int.ceil_sub_natCast] at hceil
  unfold phaseRequirement
  omega

def phaseMax (phases : List ℕ) (requirement : ℕ → ℤ) : ℤ :=
  phases.foldr (fun n acc => max (requirement n) acc) 0

theorem phaseMax_shift (phases : List ℕ) (f g : ℕ → ℤ) (L : ℕ)
    (h : ∀ n, f n - L ≤ g n) :
    max 0 (phaseMax phases f - L) ≤ phaseMax phases g := by
  induction phases with
  | nil => simp [phaseMax]
  | cons n ns ih =>
      simp only [phaseMax] at ih
      simp only [phaseMax, List.foldr_cons]
      have hn := h n
      omega

def ratioA (a b : ℕ) : ℚ := ((a + b : ℕ) : ℚ) / b
def ratioB (a b : ℕ) : ℚ := ((a + b : ℕ) : ℚ) / a

theorem ratioA_match (a b i k : ℕ) (hb : 0 < b)
    (hmatch : i * a = k * b) :
    (i : ℚ) * ratioA a b = (i + k : ℕ) := by
  have hb' : (b : ℚ) ≠ 0 := by exact_mod_cast (Nat.ne_of_gt hb)
  have hmatch' : (i : ℚ) * a = k * b := by exact_mod_cast hmatch
  dsimp [ratioA]
  push_cast
  field_simp
  nlinarith [hmatch']

theorem ratioB_match (a b i k : ℕ) (ha : 0 < a)
    (hmatch : i * a = k * b) :
    (k : ℚ) * ratioB a b = (i + k : ℕ) := by
  have ha' : (a : ℚ) ≠ 0 := by exact_mod_cast (Nat.ne_of_gt ha)
  have hmatch' : (i : ℚ) * a = k * b := by exact_mod_cast hmatch
  dsimp [ratioB]
  push_cast
  field_simp
  nlinarith [hmatch']

def selectedRequirement (a b WA WB : ℕ) (fromA : ℕ → Bool)
    (index : ℕ → ℕ) (n : ℕ) : ℤ :=
  if fromA n then phaseRequirement (index n) WA n (ratioA a b)
  else phaseRequirement (index n) WB n (ratioB a b)

theorem shift_matching_tail (a b i k WA WB : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) (phases : List ℕ)
    (fromA : ℕ → Bool) (index : ℕ → ℕ) :
    max 0 (phaseMax phases (selectedRequirement a b WA WB fromA index) - (i + k)) ≤
      phaseMax phases (selectedRequirement a b (WA - i) (WB - k) fromA index) := by
  apply phaseMax_shift
  intro n
  by_cases hsource : fromA n = true
  · simp only [selectedRequirement, hsource, ↓reduceIte]
    exact phase_requirement_shift (index n) WA i n (i + k) (ratioA a b)
      (by unfold ratioA; positivity) (ratioA_match a b i k hb hmatch)
  · have hfalse : fromA n = false := Bool.eq_false_of_not_eq_true hsource
    simp only [selectedRequirement, hfalse, Bool.false_eq_true, ↓reduceIte]
    exact phase_requirement_shift (index n) WB k n (i + k) (ratioB a b)
      (by unfold ratioB; positivity) (ratioB_match a b i k ha hmatch)

def selectsA (a b n : ℕ) : Bool :=
  decide (progress a b n ≠ progress a b (n + 1))

def selectedIndex (a b n : ℕ) : ℕ :=
  if selectsA a b n then progress a b n else n - progress a b n

def exactInterleaveTail (a b WA WB : ℕ) : ℤ :=
  phaseMax (List.range (a + b))
    (selectedRequirement a b WA WB (selectsA a b) (selectedIndex a b))

-- ANCHOR: shift_tail
theorem exact_tail_rewrite (a b i k WA WB : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) :
    max 0 (exactInterleaveTail a b WA WB - (i + k)) ≤
      exactInterleaveTail a b (WA - i) (WB - k) := by
  exact shift_matching_tail a b i k WA WB ha hb hmatch
    (List.range (a + b)) (selectsA a b) (selectedIndex a b)
-- ANCHOR_END: shift_tail

-- ANCHOR: tail_strict
theorem tail_can_be_strict :
    exactInterleaveTail 1 2 (0 - 2) (0 - 1) = 2 ∧
    max 0 (exactInterleaveTail 1 2 0 0 - (2 + 1 : ℕ)) = 0 := by
  decide +kernel
-- ANCHOR_END: tail_strict

end Profs
