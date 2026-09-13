import Profs.Deinterleave
import Profs.SumCommutativity

namespace Profs

/- Theta: odczyt lewej składowej, ~Theta: odczyt reszty (prawej składowej). -/
def deinterleaveLeft (a b : ℕ) {γ : Type*} (C : ℕ → γ) (n : ℕ) : γ :=
  C (originalIndex a b n)

def deinterleaveRight (a b : ℕ) {γ : Type*} (C : ℕ → γ) (n : ℕ) : γ :=
  C (residueIndex a b n)

-- ANCHOR: slots_bijective
theorem selectors_bijective (a b : ℕ) (ha : 0 < a) (hb : 0 < b) :
    Function.Bijective (Sum.elim (originalIndex a b) (residueIndex a b)) := by
  constructor
  · rintro (n | n) (m | m) h
    · exact congrArg Sum.inl (original_index_injective a b ha hb h)
    · exact absurd h.symm (selectors_disjoint a b m n ha hb)
    · exact absurd h (selectors_disjoint a b n m ha hb)
    · exact congrArg Sum.inr (residue_index_injective a b ha hb h)
  · intro j
    rcases every_slot_selected a b j ha hb with ⟨n, hn⟩ | ⟨n, hn⟩
    · exact ⟨.inr n, hn⟩
    · exact ⟨.inl n, hn⟩
-- ANCHOR_END: slots_bijective

-- ANCHOR: interleave_inverse
theorem deinterleave_inverts_interleave (a b : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} (A : ℕ → α) (B : ℕ → β) :
    deinterleaveLeft a b (interleaveAt a b A B) = (fun n => .inl (A n)) ∧
    deinterleaveRight a b (interleaveAt a b A B) = (fun n => .inr (B n)) := by
  constructor
  · funext n
    exact recover_original a b n ha hb A B
  · funext n
    exact recover_residue a b n ha hb A B
-- ANCHOR_END: interleave_inverse

/- delta w wersji DEBS: C - Δt wybiera c_{⌈n*Δt/ΔC⌉} i wymaga Δt ≥ ΔC. -/
def rateDiff {γ : Type*} (C : RegularStream γ) (target : ℚ) : RegularStream γ where
  period := target
  value n := C.value ⌈(n : ℚ) * target / C.period⌉₊

/- Próbkowanie w dół przez sufit i z powrotem przez podłogę zwraca indeks. -/
theorem resample_roundtrip (s o : ℚ) (ho : 0 < o) (hos : o ≤ s) (n : ℕ) :
    sampleIndex s o ⌈(n : ℚ) * s / o⌉₊ = n := by
  have hs : 0 < s := lt_of_lt_of_le ho hos
  have hx : (0 : ℚ) ≤ (n : ℚ) * s / o := by positivity
  set m := ⌈(n : ℚ) * s / o⌉₊
  have hlo : (n : ℚ) * s / o ≤ m := Nat.le_ceil _
  have hhi : (m : ℚ) < (n : ℚ) * s / o + 1 := Nat.ceil_lt_add_one hx
  unfold sampleIndex
  rw [Nat.floor_eq_iff (by positivity)]
  constructor
  · rw [le_div_iff₀ hs]
    calc
      (n : ℚ) * s = (n : ℚ) * s / o * o := by field_simp
      _ ≤ (m : ℚ) * o := mul_le_mul_of_nonneg_right hlo ho.le
  · rw [div_lt_iff₀ hs]
    calc
      (m : ℚ) * o < ((n : ℚ) * s / o + 1) * o := mul_lt_mul_of_pos_right hhi ho
      _ = (n : ℚ) * s + o := by field_simp
      _ ≤ ((n : ℚ) + 1) * s := by nlinarith

-- ANCHOR: sum_inverse
theorem diff_recovers_sum {α β : Type*} (A : RegularStream α) (B : RegularStream β)
    (hA : 0 < A.period) (hB : 0 < B.period) :
    (rateDiff (streamSum A B) A.period).period = A.period ∧
    (∀ n, ((rateDiff (streamSum A B) A.period).value n).1 = A.value n) ∧
    (rateDiff (streamSum A B) B.period).period = B.period ∧
    (∀ n, ((rateDiff (streamSum A B) B.period).value n).2 = B.value n) := by
  have hmin : 0 < min A.period B.period := lt_min hA hB
  refine ⟨rfl, fun n => ?_, rfl, fun n => ?_⟩
  · simp only [rateDiff, streamSum]
    rw [resample_roundtrip A.period _ hmin (min_le_left _ _) n]
  · simp only [rateDiff, streamSum]
    rw [resample_roundtrip B.period _ hmin (min_le_right _ _) n]
-- ANCHOR_END: sum_inverse

end Profs
