import Mathlib.NumberTheory.Rayleigh
import Mathlib.Tactic

namespace Profs

/- The positive integers a and b encode the ratio of two rational intervals. -/
noncomputable def leftSlope (a b : ℕ) : ℝ := ((a + b : ℕ) : ℝ) / a
noncomputable def rightSlope (a b : ℕ) : ℝ := ((a + b : ℕ) : ℝ) / b

theorem slopes_conjugate (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    (leftSlope a b).HolderConjugate (rightSlope a b) := by
  apply Real.holderConjugate_iff.mpr
  have ha' : (0 : ℝ) < a := by exact_mod_cast ha
  have hb' : (0 : ℝ) < b := by exact_mod_cast hb
  constructor
  · dsimp [leftSlope]
    rw [one_lt_div ha']
    push_cast
    linarith
  · dsimp [leftSlope, rightSlope]
    push_cast
    field_simp

end Profs
