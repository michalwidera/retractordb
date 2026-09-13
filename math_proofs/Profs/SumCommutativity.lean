import Mathlib.Data.Rat.Floor
import Mathlib.Tactic

namespace Profs

structure RegularStream (α : Type*) where
  period : ℚ
  value : ℕ → α

def sampleIndex (sourcePeriod outputPeriod : ℚ) (n : ℕ) : ℕ :=
  ⌊(n : ℚ) * outputPeriod / sourcePeriod⌋₊

theorem sample_self (d : ℚ) (hd : 0 < d) (n : ℕ) :
    sampleIndex d d n = n := by
  unfold sampleIndex
  rw [mul_div_cancel_right₀ (n : ℚ) (ne_of_gt hd)]
  exact Nat.floor_natCast n

def streamSum {α β : Type*} (A : RegularStream α) (B : RegularStream β) :
    RegularStream (α × β) where
  period := min A.period B.period
  value n :=
    (A.value (sampleIndex A.period (min A.period B.period) n),
     B.value (sampleIndex B.period (min A.period B.period) n))

-- ANCHOR: sum_comm
theorem sum_commutativity {α β : Type*} (A : RegularStream α) (B : RegularStream β) :
    (streamSum A B).period = (streamSum B A).period ∧
    ∀ n, (streamSum A B).value n = ((streamSum B A).value n).swap := by
  constructor
  · simp [streamSum, min_comm]
  · intro n
    simp [streamSum, min_comm]
-- ANCHOR_END: sum_comm

theorem sum_fast_left {α β : Type*} (A : RegularStream α) (B : RegularStream β)
    (h : A.period ≤ B.period) (n : ℕ) :
    (streamSum A B).period = A.period ∧
    (streamSum A B).value n =
      (A.value (sampleIndex A.period A.period n),
       B.value (sampleIndex B.period A.period n)) := by
  simp [streamSum, min_eq_left h]

end Profs
