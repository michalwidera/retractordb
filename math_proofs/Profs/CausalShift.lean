import Profs.ShiftMatching
import Profs.InterleaveCovering

namespace Profs

/- Wspolny prog obu monotonicznych map, jak w computeLogicalOrigin(). -/
def originReady (a b OA OB n : ℕ) : Prop :=
  OA ≤ indexA a b n ∧ OB ≤ indexB a b n

instance (a b OA OB n : ℕ) : Decidable (originReady a b OA OB n) :=
  inferInstanceAs (Decidable (OA ≤ indexA a b n ∧ OB ≤ indexB a b n))

theorem originReady_exists (a b OA OB : ℕ) (ha : 0 < a) (hb : 0 < b) :
    ∃ n, originReady a b OA OB n := by
  refine ⟨(OA + OB + 1) * (a + b), ?_⟩
  unfold originReady
  rw [indexA_multiple a b _ ha, indexB_multiple a b _ ha]
  constructor <;> nlinarith

noncomputable def interleaveOrigin (a b OA OB : ℕ) (ha : 0 < a) (hb : 0 < b) : ℕ :=
  Nat.find (originReady_exists a b OA OB ha hb)

theorem interleaveOrigin_le_iff (a b OA OB n : ℕ) (ha : 0 < a) (hb : 0 < b) :
    interleaveOrigin a b OA OB ha hb ≤ n ↔ originReady a b OA OB n := by
  constructor
  · intro hn
    have h := Nat.find_spec (originReady_exists a b OA OB ha hb)
    have hA : Monotone (indexA a b) :=
      monotone_nat_of_le_succ (fun n => (indexA_step a b n ha).1)
    have hB : Monotone (indexB a b) :=
      monotone_nat_of_le_succ (fun n => (indexB_step a b n ha).1)
    exact ⟨le_trans h.1 (hA hn), le_trans h.2 (hB hn)⟩
  · exact Nat.find_min' (originReady_exists a b OA OB ha hb)

theorem originReady_shift (a b i k OA OB n : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) :
    originReady a b (OA + i) (OB + k) n ↔
      i + k ≤ n ∧ originReady a b OA OB (n - (i + k)) := by
  have hs : 0 < a + b := by omega
  have hbound : originReady a b (OA + i) (OB + k) n → i + k ≤ n := by
    intro h
    have hi : i ≤ n * b / (a + b) := le_trans (Nat.le_add_left i OA) h.1
    have hmul := (Nat.le_div_iff_mul_le hs).mp hi
    nlinarith
  have hmaps (hn : i + k ≤ n) :
      indexA a b n = indexA a b (n - (i + k)) + i ∧
      indexB a b n = indexB a b (n - (i + k)) + k := by
    have h := progress_shift a b i k (n - (i + k)) ha hb hmatch
    have heq : n - (i + k) + i + k = n := by omega
    rw [heq] at h
    change indexA a b n = indexA a b (n - (i + k)) + i at h
    have hp := indexA_le a b (n - (i + k)) ha
    refine ⟨h, ?_⟩
    unfold indexB
    omega
  constructor
  · intro h
    have hn := hbound h
    obtain ⟨hA, hB⟩ := hmaps hn
    exact ⟨hn, by unfold originReady at *; omega⟩
  · rintro ⟨hn, h⟩
    obtain ⟨hA, hB⟩ := hmaps hn
    unfold originReady at *
    omega

-- ANCHOR: causal_origin
theorem interleaveOrigin_shift (a b i k OA OB : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) :
    interleaveOrigin a b (OA + i) (OB + k) ha hb =
      interleaveOrigin a b OA OB ha hb + (i + k) := by
  apply Nat.le_antisymm
  · apply (interleaveOrigin_le_iff a b _ _ _ ha hb).mpr
    apply (originReady_shift a b i k OA OB _ ha hb hmatch).mpr
    refine ⟨by omega, ?_⟩
    simpa [interleaveOrigin] using Nat.find_spec (originReady_exists a b OA OB ha hb)
  · have h := Nat.find_spec (originReady_exists a b (OA + i) (OB + k) ha hb)
    obtain ⟨hn, hready⟩ := (originReady_shift a b i k OA OB _ ha hb hmatch).mp h
    have hmin := (interleaveOrigin_le_iff a b OA OB _ ha hb).mpr hready
    change interleaveOrigin a b OA OB ha hb ≤
      interleaveOrigin a b (OA + i) (OB + k) ha hb - (i + k) at hmin
    change i + k ≤ interleaveOrigin a b (OA + i) (OB + k) ha hb at hn
    omega
-- ANCHOR_END: causal_origin

/- Interwal jest parametrem typu; wartosci przed origin sa nieobserwowalne. -/
structure CausalStream (α : Type*) (period : ℚ) where
  origin : ℕ
  tail : ℕ
  value : ℕ → α

def CausalStream.interval {α : Type*} {d : ℚ} (_S : CausalStream α d) : ℚ := d

def CausalStream.read {α : Type*} {d : ℚ} (S : CausalStream α d) (n : ℕ) : Option α :=
  if S.origin ≤ n then some (S.value n) else none

def CausalStream.available {α : Type*} {d : ℚ} (S : CausalStream α d) (n slot : ℕ) : Prop :=
  S.origin ≤ n ∧ n + 1 + S.tail ≤ slot

-- ANCHOR: causal_shift
def causalShift {α : Type*} {d : ℚ} (m : ℕ) (S : CausalStream α d) : CausalStream α d where
  origin := S.origin + m
  tail := S.tail - m
  value n := S.value (n - m)

theorem causalShift_read {α : Type*} {d : ℚ} (m n : ℕ) (S : CausalStream α d) :
    (causalShift m S).read n = if m ≤ n then S.read (n - m) else none := by
  by_cases hm : m ≤ n
  · have hiff : S.origin + m ≤ n ↔ S.origin ≤ n - m := by omega
    simp [CausalStream.read, causalShift, hm, hiff]
  · have hnot : ¬ S.origin + m ≤ n := by omega
    simp [CausalStream.read, causalShift, hm, hnot]
-- ANCHOR_END: causal_shift

/- Dokladnie najmniejszy ogon, ktory pokrywa dostepnosc starszego rekordu. -/
theorem causalShift_tail_exact {α : Type*} {d : ℚ} (m w : ℕ) (S : CausalStream α d) :
    (causalShift m S).tail ≤ w ↔
      ∀ n, (causalShift m S).origin ≤ n → n - m + 1 + S.tail ≤ n + 1 + w := by
  dsimp [causalShift]
  constructor
  · intro hw n hn
    omega
  · intro h
    have hn := h (S.origin + m) (le_refl _)
    omega

theorem causalShift_available_source {α : Type*} {d : ℚ} (m n slot : ℕ)
    (S : CausalStream α d) (h : (causalShift m S).available n slot) :
    S.available (n - m) slot := by
  dsimp [CausalStream.available, causalShift] at *
  omega

noncomputable def causalInterleave (a b : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} {tick : ℚ}
    (A : CausalStream α ((a : ℚ) * tick)) (B : CausalStream β ((b : ℚ) * tick)) :
    CausalStream (Sum α β) (((a : ℚ) * b / (a + b)) * tick) where
  origin := interleaveOrigin a b A.origin B.origin ha hb
  tail := (exactInterleaveTail a b A.tail B.tail).toNat
  value := interleaveAt a b A.value B.value

theorem causalInterleave_interval (a b : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} {tick : ℚ} (htick : 0 < tick)
    (A : CausalStream α ((a : ℚ) * tick)) (B : CausalStream β ((b : ℚ) * tick)) :
    (causalInterleave a b ha hb A B).interval =
      A.interval * B.interval / (A.interval + B.interval) ∧
      0 < (causalInterleave a b ha hb A B).interval := by
  dsimp [CausalStream.interval]
  have hs : (a : ℚ) + b ≠ 0 := ne_of_gt (by positivity)
  have ht : tick ≠ 0 := ne_of_gt htick
  constructor
  · rw [← add_mul]
    field_simp
  · positivity

theorem causalInterleave_inputs_defined (a b n : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} {tick : ℚ}
    (A : CausalStream α ((a : ℚ) * tick)) (B : CausalStream β ((b : ℚ) * tick))
    (hn : (causalInterleave a b ha hb A B).origin ≤ n) :
    A.origin ≤ indexA a b n ∧ B.origin ≤ indexB a b n := by
  exact (interleaveOrigin_le_iff a b A.origin B.origin n ha hb).mp hn

theorem delayed_interleave_values (a b i k n : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) (hn : i + k ≤ n)
    {α β : Type*} (A : ℕ → α) (B : ℕ → β) :
    interleaveAt a b (fun j => A (j - i)) (fun j => B (j - k)) n =
      interleaveAt a b A B (n - (i + k)) := by
  have h := shift_matching_values a b i k ha hb hmatch
    (fun j => A (j - i)) (fun j => B (j - k)) (n - (i + k))
  have heq : n - (i + k) + i + k = n := by omega
  simpa only [Nat.add_sub_cancel, heq] using h.symm

-- ANCHOR: causal_values
theorem causal_shift_matching_read (a b i k : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) {α β : Type*} {tick : ℚ}
    (A : CausalStream α ((a : ℚ) * tick)) (B : CausalStream β ((b : ℚ) * tick)) (n : ℕ) :
    (causalInterleave a b ha hb (causalShift i A) (causalShift k B)).read n =
      (causalShift (i + k) (causalInterleave a b ha hb A B)).read n := by
  have ho : (causalInterleave a b ha hb (causalShift i A) (causalShift k B)).origin =
      (causalShift (i + k) (causalInterleave a b ha hb A B)).origin :=
    interleaveOrigin_shift a b i k A.origin B.origin ha hb hmatch
  by_cases hn : (causalShift (i + k) (causalInterleave a b ha hb A B)).origin ≤ n
  · have hl : (causalInterleave a b ha hb (causalShift i A) (causalShift k B)).origin ≤ n := by
      rwa [ho]
    simp only [CausalStream.read, if_pos hn, if_pos hl]
    apply congrArg some
    exact delayed_interleave_values a b i k n ha hb hmatch
      (by dsimp [causalShift] at hn; omega) A.value B.value
  · have hl : ¬ (causalInterleave a b ha hb (causalShift i A) (causalShift k B)).origin ≤ n := by
      rwa [ho]
    simp only [CausalStream.read, if_neg hn, if_neg hl]
-- ANCHOR_END: causal_values

theorem causal_shift_matching_tail (a b i k WA WB : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) :
    (exactInterleaveTail a b WA WB).toNat - (i + k) ≤
      (exactInterleaveTail a b (WA - i) (WB - k)).toNat := by
  have h := exact_tail_rewrite a b i k WA WB ha hb hmatch
  omega

-- ANCHOR: causal_r1
theorem causal_shift_matching (a b i k : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) {α β : Type*} {tick : ℚ}
    (A : CausalStream α ((a : ℚ) * tick)) (B : CausalStream β ((b : ℚ) * tick)) :
    let lhs := causalInterleave a b ha hb (causalShift i A) (causalShift k B)
    let rhs := causalShift (i + k) (causalInterleave a b ha hb A B)
    lhs.interval = rhs.interval ∧ lhs.origin = rhs.origin ∧
      (∀ n, lhs.read n = rhs.read n) ∧ rhs.tail ≤ lhs.tail ∧
      (∀ n slot, lhs.available n slot → rhs.available n slot) := by
  dsimp only
  have ho := interleaveOrigin_shift a b i k A.origin B.origin ha hb hmatch
  have ht := causal_shift_matching_tail a b i k A.tail B.tail ha hb hmatch
  refine ⟨rfl, ho, causal_shift_matching_read a b i k ha hb hmatch A B, ht, ?_⟩
  intro n slot h
  dsimp [CausalStream.available, causalInterleave, causalShift] at *
  omega
-- ANCHOR_END: causal_r1

end Profs
