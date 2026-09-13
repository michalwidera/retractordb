import Profs.BeattyModel

namespace Profs

open scoped symmDiff

noncomputable def residuePosition (a b : ℕ) (n : ℤ) : ℤ :=
  beattySeq (leftSlope a b) n

noncomputable def recoveredPosition (a b : ℕ) (n : ℤ) : ℤ :=
  beattySeq' (rightSlope a b) n

def residueSet (a b : ℕ) : Set ℤ :=
  {j | ∃ n : ℤ, 0 < n ∧ residuePosition a b n = j}

def recoveredSet (a b : ℕ) : Set ℤ :=
  {j | ∃ n : ℤ, 0 < n ∧ recoveredPosition a b n = j}

-- ANCHOR: partition
theorem positive_positions_partition (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    residueSet a b ∆ recoveredSet a b = {j : ℤ | 0 < j} := by
  exact beattySeq_symmDiff_beattySeq'_pos (slopes_conjugate a b ha hb)
-- ANCHOR_END: partition

theorem zero_is_residue (a b : ℕ) : residuePosition a b 0 = 0 := by
  simp [residuePosition, beattySeq]

theorem residue_formula (a b : ℕ) (ha : 0 < a) (n : ℤ) :
    residuePosition a b n = n + ⌊(n : ℝ) * b / a⌋ := by
  have ha' : (a : ℝ) ≠ 0 := by exact_mod_cast (Nat.ne_of_gt ha)
  unfold residuePosition beattySeq leftSlope
  have hrew : (n : ℝ) * (((a + b : ℕ) : ℝ) / a) =
      (n : ℝ) + (n : ℝ) * b / a := by
    push_cast
    field_simp
  rw [hrew, Int.floor_intCast_add]

theorem recovered_formula (a b : ℕ) (hb : 0 < b) (n : ℤ) :
    recoveredPosition a b (n + 1) = n + ⌈((n + 1 : ℤ) : ℝ) * a / b⌉ := by
  have hb' : (b : ℝ) ≠ 0 := by exact_mod_cast (Nat.ne_of_gt hb)
  unfold recoveredPosition beattySeq' rightSlope
  have hrew : ((n + 1 : ℤ) : ℝ) * (((a + b : ℕ) : ℝ) / b) =
      ((n + 1 : ℤ) : ℝ) + ((n + 1 : ℤ) : ℝ) * a / b := by
    push_cast
    field_simp
    ring
  rw [hrew, Int.ceil_intCast_add]
  omega

private theorem floor_strictMono (r : ℝ) (hr : 1 < r) :
    StrictMono (fun n : ℤ => ⌊(n : ℝ) * r⌋) := by
  intro n m hnm
  have hnm' : (n : ℝ) + 1 ≤ m := by exact_mod_cast hnm
  have hprod : (n : ℝ) * r + 1 ≤ (m : ℝ) * r := by
    calc
      (n : ℝ) * r + 1 ≤ (n : ℝ) * r + r := by linarith
      _ = ((n : ℝ) + 1) * r := by ring
      _ ≤ (m : ℝ) * r := mul_le_mul_of_nonneg_right hnm' (by linarith)
  have hfloor := Int.floor_mono hprod
  rw [Int.floor_add_one] at hfloor
  change ⌊(n : ℝ) * r⌋ < ⌊(m : ℝ) * r⌋
  omega

private theorem ceil_strictMono (r : ℝ) (hr : 1 < r) :
    StrictMono (fun n : ℤ => ⌈(n : ℝ) * r⌉ - 1) := by
  intro n m hnm
  have hnm' : (n : ℝ) + 1 ≤ m := by exact_mod_cast hnm
  have hprod : (n : ℝ) * r + 1 ≤ (m : ℝ) * r := by
    calc
      (n : ℝ) * r + 1 ≤ (n : ℝ) * r + r := by linarith
      _ = ((n : ℝ) + 1) * r := by ring
      _ ≤ (m : ℝ) * r := mul_le_mul_of_nonneg_right hnm' (by linarith)
  have hceil := Int.ceil_mono hprod
  rw [Int.ceil_add_one] at hceil
  change ⌈(n : ℝ) * r⌉ - 1 < ⌈(m : ℝ) * r⌉ - 1
  omega

theorem residue_position_injective (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    Function.Injective (residuePosition a b) := by
  exact (floor_strictMono _ (slopes_conjugate a b ha hb).lt).injective

theorem recovered_position_injective (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    Function.Injective (recoveredPosition a b) := by
  exact (ceil_strictMono _ (slopes_conjugate a b ha hb).symm.lt).injective

end Profs
