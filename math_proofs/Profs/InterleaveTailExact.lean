import Profs.CausalShift

namespace Profs

theorem progress_period (a b n : ℕ) (ha : 0 < a) (hb : 0 < b) :
    progress a b (n + (a + b)) = progress a b n + b := by
  have hs : 0 < a + b := by omega
  have heq : (n + (a + b)) * b = n * b + (a + b) * b := by ring
  simp only [progress, heq]
  exact Nat.add_mul_div_left (n * b) b hs

theorem selectsA_period (a b n : ℕ) (ha : 0 < a) (hb : 0 < b) :
    selectsA a b (n + (a + b)) = selectsA a b n := by
  have h1 := progress_period a b n ha hb
  have h2 := progress_period a b (n + 1) ha hb
  have heq : n + (a + b) + 1 = n + 1 + (a + b) := by omega
  unfold selectsA
  rw [heq, h1, h2]
  simp

theorem phaseRequirement_period (j W n s t : ℕ) (ratio : ℚ) (h : (s : ℚ) * ratio = t) :
    phaseRequirement (j + s) W (n + t) ratio = phaseRequirement j W n ratio := by
  have hq : ((j + s + 1 + W : ℕ) : ℚ) * ratio = ((j + 1 + W : ℕ) : ℚ) * ratio + t := by
    rw [← h]; push_cast; ring
  unfold phaseRequirement
  rw [hq, Int.ceil_add_natCast]
  omega

/- Wymaganie fazy powtarza sie co a+b slotow wyjscia. -/
-- ANCHOR: tail_period
theorem tailRequirement_period (a b WA WB n : ℕ) (ha : 0 < a) (hb : 0 < b) :
    selectedRequirement a b WA WB (selectsA a b) (selectedIndex a b) (n + (a + b)) =
      selectedRequirement a b WA WB (selectsA a b) (selectedIndex a b) n := by
  have hsel := selectsA_period a b n ha hb
  have hp := progress_period a b n ha hb
  have hle := progress_le_slot a b n ha hb
  have haq : (a : ℚ) ≠ 0 := by exact_mod_cast ha.ne'
  have hbq : (b : ℚ) ≠ 0 := by exact_mod_cast hb.ne'
  unfold selectedRequirement selectedIndex
  rw [hsel]
  by_cases hA : selectsA a b n = true
  · simp only [hA, ↓reduceIte, hp]
    exact phaseRequirement_period _ WA n b (a + b) (ratioA a b)
      (by unfold ratioA; field_simp)
  · have hB : selectsA a b n = false := Bool.eq_false_of_not_eq_true hA
    simp only [hB, Bool.false_eq_true, ↓reduceIte, hp]
    have hidx : n + (a + b) - (progress a b n + b) = n - progress a b n + a := by omega
    rw [hidx]
    exact phaseRequirement_period _ WB n a (a + b) (ratioB a b)
      (by unfold ratioB; field_simp)
-- ANCHOR_END: tail_period

theorem tailRequirement_add_mul (a b WA WB n q : ℕ) (ha : 0 < a) (hb : 0 < b) :
    selectedRequirement a b WA WB (selectsA a b) (selectedIndex a b) (n + (a + b) * q) =
      selectedRequirement a b WA WB (selectsA a b) (selectedIndex a b) n := by
  induction q with
  | zero => simp
  | succ q ih =>
      rw [Nat.mul_succ, ← Nat.add_assoc, tailRequirement_period a b WA WB _ ha hb, ih]

theorem tailRequirement_mod (a b WA WB n : ℕ) (ha : 0 < a) (hb : 0 < b) :
    selectedRequirement a b WA WB (selectsA a b) (selectedIndex a b) n =
      selectedRequirement a b WA WB (selectsA a b) (selectedIndex a b) (n % (a + b)) := by
  have h := tailRequirement_add_mul a b WA WB (n % (a + b)) (n / (a + b)) ha hb
  rwa [Nat.mod_add_div] at h

theorem phaseMax_le_iff (phases : List ℕ) (f : ℕ → ℤ) (W : ℤ) :
    phaseMax phases f ≤ W ↔ 0 ≤ W ∧ ∀ n ∈ phases, f n ≤ W := by
  induction phases with
  | nil => simp [phaseMax]
  | cons m ms ih =>
      simp only [phaseMax, List.foldr_cons] at ih ⊢
      rw [max_le_iff, ih, List.forall_mem_cons]
      tauto

/- Zdarzeniowy warunek dostepnosci w czasie wymiernym: rekord wejscia j o ogonie W_src
   jest okreslony w chwili (j+1+W_src)*Delta_src, a slot n wyjscia konczy sie
   w chwili (n+1+W)*Delta_out. -/
def interleaveReady (a b WA WB W : ℕ) (tick : ℚ) (n : ℕ) : Prop :=
  if selectsA a b n then
    ((selectedIndex a b n + 1 + WA : ℕ) : ℚ) * ((a : ℚ) * tick) ≤
      ((n + 1 + W : ℕ) : ℚ) * (((a : ℚ) * b / (a + b)) * tick)
  else
    ((selectedIndex a b n + 1 + WB : ℕ) : ℚ) * ((b : ℚ) * tick) ≤
      ((n + 1 + W : ℕ) : ℚ) * (((a : ℚ) * b / (a + b)) * tick)

theorem ready_iff_requirement (j Ws n W : ℕ) (ratio c d : ℚ) (hc : 0 < c)
    (hd : d = ratio * c) :
    ((j + 1 + Ws : ℕ) : ℚ) * d ≤ ((n + 1 + W : ℕ) : ℚ) * c ↔
      phaseRequirement j Ws n ratio ≤ W := by
  rw [hd, ← mul_assoc, mul_le_mul_iff_of_pos_right hc]
  have h : ⌈((j + 1 + Ws : ℕ) : ℚ) * ratio⌉ ≤ ((n + 1 + W : ℕ) : ℤ) ↔
      ((j + 1 + Ws : ℕ) : ℚ) * ratio ≤ (((n + 1 + W : ℕ) : ℤ) : ℚ) := Int.ceil_le
  rw [Int.cast_natCast] at h
  unfold phaseRequirement
  constructor
  · intro hx
    have := h.mpr hx
    omega
  · intro hw
    exact h.mp (by omega)

theorem interleaveReady_iff (a b WA WB W n : ℕ) (ha : 0 < a) (hb : 0 < b)
    (tick : ℚ) (htick : 0 < tick) :
    interleaveReady a b WA WB W tick n ↔
      selectedRequirement a b WA WB (selectsA a b) (selectedIndex a b) n ≤ W := by
  have haq : (0 : ℚ) < a := by exact_mod_cast ha
  have hbq : (0 : ℚ) < b := by exact_mod_cast hb
  have hc : 0 < (a : ℚ) * b / (a + b) * tick := by positivity
  unfold interleaveReady selectedRequirement
  by_cases hA : selectsA a b n = true
  · simp only [hA, ↓reduceIte]
    apply ready_iff_requirement _ _ _ _ _ _ _ hc
    unfold ratioA
    push_cast
    field_simp
  · have hB : selectsA a b n = false := Bool.eq_false_of_not_eq_true hA
    simp only [hB, Bool.false_eq_true, ↓reduceIte]
    apply ready_iff_requirement _ _ _ _ _ _ _ hc
    unfold ratioB
    push_cast
    field_simp

/- Ogon jest dokladna granica dostepnosci: spelnia warunek dla kazdego rekordu
   od dowolnego progu N0 i kazdy ogon spelniajacy warunek jest od niego nie mniejszy. -/
-- ANCHOR: tail_exact
theorem exactInterleaveTail_exact (a b WA WB W N0 : ℕ) (ha : 0 < a) (hb : 0 < b)
    (tick : ℚ) (htick : 0 < tick) :
    (exactInterleaveTail a b WA WB).toNat ≤ W ↔
      ∀ n, N0 ≤ n → interleaveReady a b WA WB W tick n := by
  have hs : 0 < a + b := by omega
  simp only [interleaveReady_iff a b WA WB W _ ha hb tick htick]
  constructor
  · intro hW n _
    have hle : exactInterleaveTail a b WA WB ≤ W := by omega
    unfold exactInterleaveTail at hle
    obtain ⟨_, hall⟩ := (phaseMax_le_iff _ _ _).mp hle
    rw [tailRequirement_mod a b WA WB n ha hb]
    exact hall _ (List.mem_range.mpr (Nat.mod_lt _ hs))
  · intro h
    have hle : exactInterleaveTail a b WA WB ≤ W := by
      unfold exactInterleaveTail
      refine (phaseMax_le_iff _ _ _).mpr ⟨by omega, ?_⟩
      intro m _
      have hN : N0 ≤ m + (a + b) * N0 := by nlinarith
      have hm := h (m + (a + b) * N0) hN
      rwa [tailRequirement_add_mul a b WA WB m N0 ha hb] at hm
    omega
-- ANCHOR_END: tail_exact

-- ANCHOR: causal_tail_exact
theorem causalInterleave_tail_exact (a b : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} {tick : ℚ} (htick : 0 < tick)
    (A : CausalStream α ((a : ℚ) * tick)) (B : CausalStream β ((b : ℚ) * tick)) (W : ℕ) :
    (causalInterleave a b ha hb A B).tail ≤ W ↔
      ∀ n, (causalInterleave a b ha hb A B).origin ≤ n →
        interleaveReady a b A.tail B.tail W tick n :=
  exactInterleaveTail_exact a b A.tail B.tail W _ ha hb tick htick
-- ANCHOR_END: causal_tail_exact

end Profs
