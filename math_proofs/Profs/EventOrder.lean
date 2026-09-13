import Profs.BeattyModel

namespace Profs

def interleaveAt (a b : ℕ) {α β : Type*}
    (A : ℕ → α) (B : ℕ → β) (n : ℕ) : Sum α β :=
  let p := n * b / (a + b)
  if p = (n + 1) * b / (a + b) then
    .inr (B (n - p))
  else
    .inl (A p)

def threeSecondEvents (n : ℕ) : ℕ := (n + 1) * 3
def twoSecondEvents (n : ℕ) : ℕ := (n + 1) * 2

/- Tau = phi(Epsilon, Alfa) = {1,2,a,3,b,4,5,c,6,d,...}: inl m to (m+1)-ty
   rekord Epsilon (a, b, c, ...), inr m to (m+1)-ty rekord Alfa (1, 2, 3, ...). -/
-- ANCHOR: tau_prefix
theorem tau_prefix :
    (List.range 10).map (interleaveAt 3 2 id id) =
      [.inr 0, .inr 1, .inl 0, .inr 2, .inl 1, .inr 3, .inr 4, .inl 2, .inr 5, .inl 3] := by
  decide
-- ANCHOR_END: tau_prefix

-- ANCHOR: event_order
theorem event_order_counterexample :
    interleaveAt 3 2 threeSecondEvents twoSecondEvents 6 = .inr 10 ∧
    interleaveAt 3 2 threeSecondEvents twoSecondEvents 7 = .inl 9 ∧
    6 < 7 ∧ 9 < 10 := by
  decide
-- ANCHOR_END: event_order

end Profs
