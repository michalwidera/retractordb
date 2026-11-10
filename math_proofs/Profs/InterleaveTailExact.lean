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

/- Zrodla deklarowane maja ogon 0. Faza 0 przeplotu wybiera b_0 (interleave_starts_with_right),
   a jej wymaganie ceil((a+b)/a) - 1 jest dodatnie, wiec ogon przeplotu dwoch zrodel
   deklarowanych wynosi co najmniej 1. -/
-- ANCHOR: declared_tail_pos
theorem exactInterleaveTail_declared_pos (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    1 ≤ exactInterleaveTail a b 0 0 := by
  have hs : 0 < a + b := by omega
  have h1 : b / (a + b) = 0 := Nat.div_eq_of_lt (by omega)
  have hsel : selectsA a b 0 = false := by simp [selectsA, progress, h1]
  have hidx : selectedIndex a b 0 = 0 := by simp [selectedIndex, hsel]
  have hratio : (1 : ℚ) < ratioB a b := by
    have haq : (0 : ℚ) < a := by exact_mod_cast ha
    have hbq : (0 : ℚ) < b := by exact_mod_cast hb
    unfold ratioB
    rw [one_lt_div haq]
    push_cast
    linarith
  have hceil : 1 < ⌈ratioB a b⌉ := Int.lt_ceil.mpr (by exact_mod_cast hratio)
  have hreq : 1 ≤ selectedRequirement a b 0 0 (selectsA a b) (selectedIndex a b) 0 := by
    simp only [selectedRequirement, hsel, hidx, Bool.false_eq_true, ↓reduceIte, phaseRequirement]
    norm_num
    omega
  by_contra hlt
  have hle : exactInterleaveTail a b 0 0 ≤ 0 := by omega
  unfold exactInterleaveTail at hle
  obtain ⟨_, hall⟩ := (phaseMax_le_iff _ _ _).mp hle
  have h0 := hall 0 (List.mem_range.mpr hs)
  omega
-- ANCHOR_END: declared_tail_pos

/- Nad zrodlami deklarowanymi strona zablokowana niesie caly ogon przeplotu W, a sfaktoryzowana
   tylko nadwyzke max(0, W - L). Przy rownym poczatku cisza (origin + ogon) rozni sie wiec
   dokladnie o min(W, L), a przy kazdym niezerowym przesunieciu przepisanie jest ostro wczesniejsze. -/
-- ANCHOR: causal_r1_declared
theorem causal_shift_matching_declared (a b i k : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) {α β : Type*} {tick : ℚ}
    (A : CausalStream α ((a : ℚ) * tick)) (B : CausalStream β ((b : ℚ) * tick))
    (hA : A.tail = 0) (hB : B.tail = 0) :
    let W := (exactInterleaveTail a b 0 0).toNat
    let lhs := causalInterleave a b ha hb (causalShift i A) (causalShift k B)
    let rhs := causalShift (i + k) (causalInterleave a b ha hb A B)
    lhs.tail = W ∧ rhs.tail = W - (i + k) ∧
      lhs.origin + lhs.tail = rhs.origin + rhs.tail + min W (i + k) ∧
      (0 < i + k → rhs.origin + rhs.tail < lhs.origin + lhs.tail) := by
  have ho := interleaveOrigin_shift a b i k A.origin B.origin ha hb hmatch
  have hW := exactInterleaveTail_declared_pos a b ha hb
  dsimp only [causalInterleave, causalShift]
  rw [hA, hB, Nat.zero_sub, Nat.zero_sub, ho]
  omega
-- ANCHOR_END: causal_r1_declared

/- Postac zamknieta dokladnego ogona przeplotu (#616). Dla wzglednie pierwszych a, b ogon jest
   wiekszym z dwoch kresow: ceil(W_A*(a+b)/b), osiaganego w ostatniej fazie okresu (rekord b-1
   lewej skladowej), i ceil(((1+W_B)*(a+b) + a - 1)/a) - 1, osiaganego w fazie czytajacej rekord m
   prawej skladowej z m*(a+b) = -1 (mod a). Tylko drugi kres wymaga wzglednej pierwszosci.
   Oba sa zapisane dzieleniem calkowitym w N, tak jak liczy je HashStartupLatency(). -/
theorem le_phaseMax (phases : List ℕ) (f : ℕ → ℤ) (n : ℕ) (hn : n ∈ phases) :
    f n ≤ phaseMax phases f :=
  ((phaseMax_le_iff phases f _).mp le_rfl).2 n hn

theorem phaseRequirement_le_natCast (j W n M L d : ℕ) (hd : 0 < d) :
    phaseRequirement j W n ((L : ℚ) / d) ≤ M ↔ (j + 1 + W) * L ≤ (M + 1 + n) * d := by
  have hdq : (0 : ℚ) < d := by exact_mod_cast hd
  unfold phaseRequirement
  have h1 : ⌈((j + 1 + W : ℕ) : ℚ) * ((L : ℚ) / d)⌉ - 1 - n ≤ (M : ℤ) ↔
      ⌈((j + 1 + W : ℕ) : ℚ) * ((L : ℚ) / d)⌉ ≤ ((M + 1 + n : ℕ) : ℤ) := by
    push_cast
    omega
  rw [h1, Int.ceil_le, Int.cast_natCast, mul_div_assoc', div_le_iff₀ hdq]
  constructor <;> intro h <;> exact_mod_cast h

theorem natCast_le_phaseRequirement (j W n M L d : ℕ) (hd : 0 < d) :
    (M : ℤ) ≤ phaseRequirement j W n ((L : ℚ) / d) ↔ (M + n) * d < (j + 1 + W) * L := by
  have hdq : (0 : ℚ) < d := by exact_mod_cast hd
  unfold phaseRequirement
  have h1 : (M : ℤ) ≤ ⌈((j + 1 + W : ℕ) : ℚ) * ((L : ℚ) / d)⌉ - 1 - n ↔
      ((M + n : ℕ) : ℤ) < ⌈((j + 1 + W : ℕ) : ℚ) * ((L : ℚ) / d)⌉ := by
    push_cast
    omega
  rw [h1, Int.lt_ceil, Int.cast_natCast, mul_div_assoc', lt_div_iff₀ hdq]
  constructor <;> intro h <;> exact_mod_cast h

theorem progress_succ_le (a b n : ℕ) (ha : 0 < a) (hb : 0 < b) :
    progress a b (n + 1) ≤ progress a b n + 1 := by
  have hs : 0 < a + b := by omega
  unfold progress
  calc (n + 1) * b / (a + b) ≤ (n * b + (a + b)) / (a + b) :=
        Nat.div_le_div_right (by nlinarith)
    _ = n * b / (a + b) + 1 := Nat.add_div_right _ hs

theorem progress_mono_succ (a b n : ℕ) :
    progress a b n ≤ progress a b (n + 1) :=
  Nat.div_le_div_right (by nlinarith)

theorem exactInterleaveTail_closed (a b WA WB : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hab : Nat.Coprime a b) :
    exactInterleaveTail a b WA WB =
      (max ((WA * (a + b) + b - 1) / b) (((1 + WB) * (a + b) + a - 2) / a) : ℕ) := by
  have hs : 0 < a + b := by omega
  set A := (WA * (a + b) + b - 1) / b with hAdef
  set B := ((1 + WB) * (a + b) + a - 2) / a with hBdef
  -- Wlasnosci dzielenia, z ktorych biora sie oba kresy.
  have hA1 : A * b ≤ WA * (a + b) + b - 1 := Nat.div_mul_le_self _ _
  have hA2 : WA * (a + b) + b - 1 < A * b + b := Nat.lt_div_mul_add hb
  have hB1 : B * a ≤ (1 + WB) * (a + b) + a - 2 := Nat.div_mul_le_self _ _
  have hB2 : (1 + WB) * (a + b) + a - 2 < B * a + a := Nat.lt_div_mul_add ha
  have hY : 2 ≤ (1 + WB) * (a + b) := by nlinarith
  apply le_antisymm
  · -- Gorne ograniczenie: kazda faza okresu spelnia wymaganie przy W = max A B.
    unfold exactInterleaveTail
    refine (phaseMax_le_iff _ _ _).mpr ⟨by positivity, ?_⟩
    intro n _
    unfold selectedRequirement selectedIndex
    by_cases hsel : selectsA a b n = true
    · simp only [hsel, ↓reduceIte]
      have hne : progress a b n ≠ progress a b (n + 1) := by
        simpa [selectsA] using hsel
      have hstep : progress a b (n + 1) = progress a b n + 1 := by
        have := progress_succ_le a b n ha hb
        have := progress_mono_succ a b n
        omega
      have hfloor : progress a b (n + 1) * (a + b) ≤ (n + 1) * b := Nat.div_mul_le_self _ _
      rw [hstep] at hfloor
      unfold ratioA
      rw [phaseRequirement_le_natCast _ _ _ _ _ _ hb]
      have hM : A * b ≤ max A B * b := Nat.mul_le_mul_right _ (le_max_left _ _)
      have hWA : WA * (a + b) ≤ A * b := by omega
      nlinarith
    · have hsel' : selectsA a b n = false := Bool.eq_false_of_not_eq_true hsel
      simp only [hsel', Bool.false_eq_true, ↓reduceIte]
      have heq : progress a b n = progress a b (n + 1) := by
        simpa [selectsA] using hsel'
      have hle := progress_le_slot a b n ha hb
      have hceil : (n + 1) * b < progress a b (n + 1) * (a + b) + (a + b) :=
        Nat.lt_div_mul_add hs
      rw [← heq] at hceil
      unfold ratioB
      rw [phaseRequirement_le_natCast _ _ _ _ _ _ ha]
      have hM : B * a ≤ max A B * a := Nat.mul_le_mul_right _ (le_max_right _ _)
      have hWB : (1 + WB) * (a + b) ≤ B * a + 1 := by omega
      generalize progress a b n = p at hceil hle ⊢
      obtain ⟨k, rfl⟩ : ∃ k, n = p + k := ⟨n - p, by omega⟩
      have hidx : p + k - p = k := by omega
      rw [hidx]
      nlinarith
  · rw [Nat.cast_max]
    apply max_le
    · -- Kres A: ostatnia faza okresu, n = a+b-1, czyta rekord b-1 lewej skladowej.
      obtain ⟨b', rfl⟩ : ∃ b', b = b' + 1 := ⟨b - 1, by omega⟩
      obtain ⟨a', rfl⟩ : ∃ a', a = a' + 1 := ⟨a - 1, by omega⟩
      have hp0 : progress (a' + 1) (b' + 1) (a' + b' + 1) = b' := by
        unfold progress
        apply Nat.div_eq_of_lt_le <;> nlinarith
      have hp1 : progress (a' + 1) (b' + 1) (a' + b' + 1 + 1) = b' + 1 := by
        unfold progress
        rw [show a' + b' + 1 + 1 = a' + 1 + (b' + 1) by omega]
        exact Nat.mul_div_cancel_left _ hs
      have hsel : selectsA (a' + 1) (b' + 1) (a' + b' + 1) = true := by
        simp [selectsA, hp0, hp1]
      have hmem : a' + b' + 1 ∈ List.range (a' + 1 + (b' + 1)) := List.mem_range.mpr (by omega)
      have h := le_phaseMax _ (selectedRequirement (a' + 1) (b' + 1) WA WB (selectsA (a' + 1) (b' + 1))
        (selectedIndex (a' + 1) (b' + 1))) _ hmem
      unfold exactInterleaveTail
      refine le_trans ?_ h
      unfold selectedRequirement selectedIndex
      simp only [hsel, ↓reduceIte, hp0]
      unfold ratioA
      rw [natCast_le_phaseRequirement _ _ _ _ _ _ hb]
      have hA1' : A * (b' + 1) ≤ WA * (a' + 1 + (b' + 1)) + b' := by omega
      nlinarith
    · -- Kres B: faza, w ktorej czytany rekord m prawej skladowej spelnia m*(a+b) = -1 (mod a).
      clear hA1 hA2 hB2
      obtain ⟨m, hm_lt, hm⟩ := Nat.exists_mul_mod_eq_of_coprime (a - 1) hab.symm ha.ne'
      obtain ⟨a', rfl⟩ : ∃ a', a = a' + 1 := ⟨a - 1, by omega⟩
      simp only [Nat.add_sub_cancel] at hm hB1
      rw [Nat.mod_eq_of_lt (by omega : a' < a' + 1)] at hm
      have hmL : m * (a' + 1 + b) % (a' + 1) = a' := by
        rw [show m * (a' + 1 + b) = (a' + 1) * m + b * m by ring, Nat.mul_add_mod]
        exact hm
      obtain ⟨c, hdiv⟩ : ∃ c, (a' + 1) * c + a' = m * (a' + 1 + b) :=
        ⟨_, by have := Nat.div_add_mod (m * (a' + 1 + b)) (a' + 1); rwa [hmL] at this⟩
      have hcm : m ≤ c := by
        by_contra hlt
        have h1 : (a' + 1) * (c + 1) ≤ (a' + 1) * m := Nat.mul_le_mul_left _ (by omega)
        have h3 : 0 ≤ m * b := Nat.zero_le _
        linarith
      have hcL : c < a' + 1 + b := by
        by_contra hge
        have h1 : (a' + 1) * (a' + 1 + b) ≤ (a' + 1) * c := Nat.mul_le_mul_left _ (by omega)
        have h2 : m * (a' + 1 + b) ≤ a' * (a' + 1 + b) := Nat.mul_le_mul_right _ (by omega)
        linarith
      obtain ⟨k, rfl⟩ : ∃ k, c = k + m := ⟨c - m, by omega⟩
      have hkb : (a' + 1) * k + a' = m * b := by linarith
      have hcb : (k + m) * b = k * (a' + 1 + b) + a' := by linarith
      have hpc : progress (a' + 1) b (k + m) = k := by
        unfold progress
        apply Nat.div_eq_of_lt_le
        · rw [hcb]; omega
        · rw [hcb]; linarith
      have hpc1 : progress (a' + 1) b (k + m + 1) = k := by
        unfold progress
        have hcb1 : (k + m + 1) * b = k * (a' + 1 + b) + a' + b := by rw [Nat.add_mul, hcb, one_mul]
        apply Nat.div_eq_of_lt_le
        · rw [hcb1]; omega
        · rw [hcb1]; linarith
      have hsel : selectsA (a' + 1) b (k + m) = false := by
        simp [selectsA, hpc, hpc1]
      have hmem : k + m ∈ List.range (a' + 1 + b) := List.mem_range.mpr hcL
      have h := le_phaseMax _ (selectedRequirement (a' + 1) b WA WB (selectsA (a' + 1) b)
        (selectedIndex (a' + 1) b)) _ hmem
      unfold exactInterleaveTail
      refine le_trans ?_ h
      unfold selectedRequirement selectedIndex
      simp only [hsel, Bool.false_eq_true, ↓reduceIte, hpc]
      rw [show k + m - k = m by omega]
      unfold ratioB
      rw [natCast_le_phaseRequirement _ _ _ _ _ _ ha]
      have hB' : B * (a' + 1) + 1 ≤ (1 + WB) * (a' + 1 + b) + a' := by omega
      linarith

end Profs
