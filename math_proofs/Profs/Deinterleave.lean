import Profs.EventOrder
import Profs.BeattyPartition
import Mathlib.Tactic

namespace Profs

open scoped symmDiff

def residueIndex (a b n : ℕ) : ℕ := n + n * b / a

theorem progress_residue (a b n : ℕ) (ha : 0 < a) (hb : 0 < b) :
    let s := residueIndex a b n
    s * b / (a + b) = n * b / a ∧
    (s + 1) * b / (a + b) = n * b / a := by
  let q := n * b / a
  have hs : 0 < a + b := by omega
  have hlow : q * a ≤ n * b := (Nat.le_div_iff_mul_le ha).mp (le_refl q)
  have hup : n * b < (q + 1) * a :=
    (Nat.div_lt_iff_lt_mul ha).mp (Nat.lt_succ_self q)
  have hlo0 : q * (a + b) ≤ (n + q) * b := by nlinarith
  have hup0 : (n + q) * b < (q + 1) * (a + b) := by nlinarith
  have hlo1 : q * (a + b) ≤ (n + q + 1) * b := by nlinarith
  have hup1 : (n + q + 1) * b < (q + 1) * (a + b) := by nlinarith
  have h0 : (n + q) * b / (a + b) = q := by
    apply Nat.le_antisymm
    · exact Nat.lt_succ_iff.mp ((Nat.div_lt_iff_lt_mul hs).mpr hup0)
    · exact (Nat.le_div_iff_mul_le hs).mpr hlo0
  have h1 : (n + q + 1) * b / (a + b) = q := by
    apply Nat.le_antisymm
    · exact Nat.lt_succ_iff.mp ((Nat.div_lt_iff_lt_mul hs).mpr hup1)
    · exact (Nat.le_div_iff_mul_le hs).mpr hlo1
  dsimp [residueIndex, q]
  exact ⟨h0, h1⟩

-- ANCHOR: recover_residue
theorem recover_residue (a b n : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} (A : ℕ → α) (B : ℕ → β) :
    interleaveAt a b A B (residueIndex a b n) = .inr (B n) := by
  obtain ⟨h0, h1⟩ := progress_residue a b n ha hb
  simp only [interleaveAt, h0, h1, ↓reduceIte]
  simp [residueIndex]
-- ANCHOR_END: recover_residue

def originalIndex (a b n : ℕ) : ℕ :=
  n + ⌈(((n + 1) * a : ℕ) : ℚ) / b⌉₊

theorem progress_original (a b n : ℕ) (ha : 0 < a) (hb : 0 < b) :
    let s := originalIndex a b n
    s * b / (a + b) = n ∧
    (s + 1) * b / (a + b) = n + 1 := by
  let x := (n + 1) * a
  let q := ⌈(x : ℚ) / b⌉₊
  have hs : 0 < a + b := by omega
  have hbQ : (0 : ℚ) < b := by exact_mod_cast hb
  have hrat : (0 : ℚ) ≤ (x : ℚ) / b := by positivity
  have hloQ : (x : ℚ) ≤ (q : ℚ) * b :=
    (div_le_iff₀ hbQ).mp (Nat.le_ceil ((x : ℚ) / b))
  have hhiQ : (q : ℚ) * b < (x : ℚ) + b := by
    have h := Nat.ceil_lt_add_one hrat
    have hm := mul_lt_mul_of_pos_right h hbQ
    calc
      (q : ℚ) * b < ((x : ℚ) / b + 1) * b := hm
      _ = (x : ℚ) + b := by field_simp
  have hlo : x ≤ q * b := by exact_mod_cast hloQ
  have hhi : q * b < x + b := by exact_mod_cast hhiQ
  change (n + 1) * a ≤ q * b at hlo
  change q * b < (n + 1) * a + b at hhi
  have hlo0 : n * (a + b) ≤ (n + q) * b := by nlinarith
  have hhi0 : (n + q) * b < (n + 1) * (a + b) := by nlinarith
  have hlo1 : (n + 1) * (a + b) ≤ (n + q + 1) * b := by nlinarith
  have hhi1 : (n + q + 1) * b < (n + 2) * (a + b) := by nlinarith
  have h0 : (n + q) * b / (a + b) = n := by
    apply Nat.le_antisymm
    · exact Nat.lt_succ_iff.mp ((Nat.div_lt_iff_lt_mul hs).mpr hhi0)
    · exact (Nat.le_div_iff_mul_le hs).mpr hlo0
  have h1 : (n + q + 1) * b / (a + b) = n + 1 := by
    apply Nat.le_antisymm
    · exact Nat.lt_succ_iff.mp ((Nat.div_lt_iff_lt_mul hs).mpr hhi1)
    · exact (Nat.le_div_iff_mul_le hs).mpr hlo1
  dsimp [originalIndex, q, x]
  exact ⟨h0, h1⟩

-- ANCHOR: recover_original
theorem recover_original (a b n : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} (A : ℕ → α) (B : ℕ → β) :
    interleaveAt a b A B (originalIndex a b n) = .inl (A n) := by
  obtain ⟨h0, h1⟩ := progress_original a b n ha hb
  simp [interleaveAt, h0, h1]
-- ANCHOR_END: recover_original

theorem residue_index_eq_position (a b n : ℕ) (ha : 0 < a) :
    (residueIndex a b n : ℤ) = residuePosition a b n := by
  rw [residue_formula a b ha]
  have hcast : (((n : ℤ) : ℝ) * b / a) = (((n * b : ℕ) : ℚ) / a : ℝ) := by
    push_cast
    ring
  have hfloor : ⌊(((n * b : ℕ) : ℚ) / a : ℝ)⌋ = (n * b / a : ℕ) := by
    exact_mod_cast Rat.floor_natCast_div_natCast (n * b) a
  rw [hcast, hfloor]
  simp [residueIndex]

theorem original_index_eq_position (a b n : ℕ) (hb : 0 < b) :
    (originalIndex a b n : ℤ) = recoveredPosition a b (n + 1) := by
  rw [recovered_formula a b hb]
  have hcast : ((((n : ℤ) + 1 : ℤ) : ℝ) * a / b) =
      (((((n + 1) * a : ℕ) : ℚ) / b) : ℝ) := by
    push_cast
    ring
  have hnonneg : (0 : ℚ) ≤ ((((n + 1) * a : ℕ) : ℚ) / b) := by positivity
  have hceil : ⌈(((((n + 1) * a : ℕ) : ℚ) / b) : ℝ)⌉ =
      (⌈((((n + 1) * a : ℕ) : ℚ) / b)⌉₊ : ℤ) := by
    exact_mod_cast (Int.natCast_ceil_eq_ceil hnonneg).symm
  rw [hcast, hceil]
  simp [originalIndex]

theorem residue_index_injective (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    Function.Injective (residueIndex a b) := by
  intro n m h
  have h' : (residuePosition a b n) = residuePosition a b m := by
    rw [← residue_index_eq_position a b n ha, ← residue_index_eq_position a b m ha]
    exact_mod_cast h
  exact_mod_cast (residue_position_injective a b ha hb h')

theorem original_index_injective (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    Function.Injective (originalIndex a b) := by
  intro n m h
  have h' : recoveredPosition a b (n + 1) = recoveredPosition a b (m + 1) := by
    rw [← original_index_eq_position a b n hb, ← original_index_eq_position a b m hb]
    exact_mod_cast h
  have := recovered_position_injective a b ha hb h'
  omega

theorem selectors_disjoint (a b n m : ℕ) (ha : 0 < a) (hb : 0 < b) :
    residueIndex a b n ≠ originalIndex a b m := by
  intro h
  have hB := recover_residue a b n ha hb (fun _ => ()) (fun _ => ())
  have hA := recover_original a b m ha hb (fun _ => ()) (fun _ => ())
  rw [h] at hB
  rw [hA] at hB
  cases hB

-- ANCHOR: all_slots
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
-- ANCHOR_END: all_slots

end Profs
