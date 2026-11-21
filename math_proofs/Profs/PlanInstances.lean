import Profs.PlanComposition
import Profs.StartupBoundaries
import Profs.InterleaveTailExact

namespace Profs

/- Operatory Tabeli 1 jako wezly modelu z PlanComposition.lean. Dla kazdego wiersza: mapa
   odczytu, rownowaznosc warunku localOK z warunkiem gotowosci z twierdzenia operatora i stad
   rownanie reguly ogona (IsLeastTail) z ta sama postacia reguly, ktora twierdzenie podaje,
   oraz regula poczatku (OriginRule). Postaci regul z silnikiem wiaza testy ProofOracle
   istniejacych twierdzen (affineTail_table, sumTail_exact, agseOrigin_le_iff i inne). -/

/- ===== Pomocnicze ===== -/

/- Rekord n czyta jeden rekord read n wejscia i; interwaly p*tick i q*tick. -/
theorem single_localOK_iff (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i w : ℕ) (read : ℕ → ℕ)
    (p q : ℕ) (tick : ℚ) (hΔv : (P v).interval = p * tick) (hΔi : (P i).interval = q * tick)
    (hr : ∀ n, (P v).reads n = [(i, (read n : ℤ))]) :
    localOK P W v w ↔ ∀ n, (P v).origin ≤ n →
      ((read n + 1 + W i : ℕ) : ℚ) * ((q : ℚ) * tick) ≤
        ((n + 1 + w : ℕ) : ℚ) * ((p : ℚ) * tick) := by
  unfold localOK emitTime
  simp only [hr, List.mem_singleton, forall_eq, Int.toNat_natCast, hΔv, hΔi]

theorem affine_localOK_iff (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i w p q c : ℕ) (tick : ℚ)
    (hΔv : (P v).interval = p * tick) (hΔi : (P i).interval = q * tick)
    (hr : ∀ n, (P v).reads n = [(i, (affineRead p q c n : ℤ))]) :
    localOK P W v w ↔ ∀ n, (P v).origin ≤ n → affineReady p q c (W i) w tick n :=
  single_localOK_iff P W v i w (affineRead p q c) p q tick hΔv hΔi hr

/- Twierdzenie operatora od progu O_v daje rownanie reguly. -/
theorem affine_leastTail_of (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i p q c r : ℕ) (tick : ℚ)
    (hΔv : (P v).interval = p * tick) (hΔi : (P i).interval = q * tick)
    (hr : ∀ n, (P v).reads n = [(i, (affineRead p q c n : ℤ))])
    (hex : ∀ w, r ≤ w ↔ ∀ n, (P v).origin ≤ n → affineReady p q c (W i) w tick n) :
    IsLeastTail P W v r :=
  leastTail_of_exact P W v r fun w =>
    (hex w).trans (affine_localOK_iff P W v i w p q c tick hΔv hΔi hr).symm

/- Okno: wszystkie odczyty z jednego wejscia, najnowszy rekord top n. Starsze rekordy sa
   zapisane wczesniej, wiec o ogonie decyduje najnowszy (agse_window_ready). -/
theorem newest_localOK_iff (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i w : ℕ) (top : ℕ → ℕ)
    (hpos : 0 ≤ (P i).interval)
    (hall : ∀ n ij, ij ∈ (P v).reads n → ij.1 = i ∧ ij.2 ≤ top n)
    (hmem : ∀ n, (i, (top n : ℤ)) ∈ (P v).reads n) :
    localOK P W v w ↔ ∀ n, (P v).origin ≤ n →
      emitTime P W i (top n) ≤ ((n + 1 + w : ℕ) : ℚ) * (P v).interval := by
  constructor
  · intro h n hn
    simpa using h n hn _ (hmem n)
  · intro h n hn ij hij
    obtain ⟨h1, h2⟩ := hall n ij hij
    refine le_trans ?_ (h n hn)
    unfold emitTime
    rw [h1]
    apply mul_le_mul_of_nonneg_right _ hpos
    exact_mod_cast (by omega : ij.2.toNat + 1 + W i ≤ top n + 1 + W i)

/- ===== Ogony ===== -/

/- Projekcja i redukcje krotek: rekord n czyta rekord n, ten sam interwal, ogon W_S. -/
-- ANCHOR: projection_least_tail
theorem projection_leastTail (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i : ℕ)
    (hpos : 0 < (P i).interval) (hΔ : (P v).interval = (P i).interval)
    (hr : ∀ n, (P v).reads n = [(i, (n : ℤ))]) :
    IsLeastTail P W v (W i) := by
  refine affine_leastTail_of P W v i 1 1 0 (W i) (P i).interval (by rw [hΔ]; ring) (by ring)
    (fun n => by rw [hr n]; simp [affineRead]) fun w => ?_
  exact projectionTail_exact (W i) w (P v).origin (P i).interval hpos
-- ANCHOR_END: projection_least_tail

/- Suma: dwa odczyty floor(n*Delta/Delta_k), ogon to maksimum skladowych. -/
-- ANCHOR: sum_least_tail
theorem sum_leastTail (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i1 i2 p1 q1 p2 q2 : ℕ) (t1 t2 : ℚ)
    (hp1 : 0 < p1) (hq1 : 0 < q1) (hp2 : 0 < p2) (hq2 : 0 < q2) (ht1 : 0 < t1) (ht2 : 0 < t2)
    (hΔv1 : (P v).interval = p1 * t1) (hΔv2 : (P v).interval = p2 * t2)
    (hΔ1 : (P i1).interval = q1 * t1) (hΔ2 : (P i2).interval = q2 * t2)
    (hr : ∀ n, (P v).reads n =
      [(i1, (affineRead p1 q1 0 n : ℤ)), (i2, (affineRead p2 q2 0 n : ℤ))]) :
    IsLeastTail P W v (max (affineTail p1 q1 0 (W i1)) (affineTail p2 q2 0 (W i2))) := by
  refine leastTail_of_exact P W v _ fun w => ?_
  rw [sumTail_exact p1 q1 (W i1) p2 q2 (W i2) w (P v).origin hp1 hq1 hp2 hq2 t1 t2 ht1 ht2]
  unfold localOK emitTime affineReady
  simp only [hr, List.mem_cons, List.not_mem_nil, or_false, forall_eq_or_imp, forall_eq,
    Int.toNat_natCast]
  rw [← hΔ1, ← hΔ2]
  constructor
  · intro h n hn
    obtain ⟨h1, h2⟩ := h n hn
    exact ⟨hΔv1 ▸ h1, hΔv2 ▸ h2⟩
  · intro h n hn
    obtain ⟨h1, h2⟩ := h n hn
    exact ⟨hΔv1 ▸ h1, hΔv2 ▸ h2⟩
-- ANCHOR_END: sum_least_tail

/- Roznica C-Delta: odczyt ceil(n*p/q) przy p/q skroconym. -/
-- ANCHOR: subtract_least_tail
theorem subtract_leastTail (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i p q : ℕ) (tick : ℚ)
    (hp : 0 < p) (hq : 0 < q) (hcop : Nat.Coprime p q) (htick : 0 < tick)
    (hΔv : (P v).interval = p * tick) (hΔi : (P i).interval = q * tick)
    (hr : ∀ n, (P v).reads n = [(i, (affineRead p q (q - 1) n : ℤ))]) :
    IsLeastTail P W v (phaseStartupLatency (((q - 1 : ℕ) : ℚ) / q) ((p : ℚ) / q) (W i)) :=
  affine_leastTail_of P W v i p q (q - 1) _ tick hΔv hΔi hr fun w =>
    subtractTail_exact p q (W i) w (P v).origin hp hq hcop tick htick
-- ANCHOR_END: subtract_least_tail

/- Rozplot Theta: wejscie jest przeplotem, p = u + v, q = v, c = u + v - 1. -/
-- ANCHOR: theta_least_tail
theorem theta_leastTail (P : ℕ → PlanNode) (W : ℕ → ℕ) (x i u v : ℕ) (tick : ℚ)
    (hv : 0 < v) (hcop : Nat.Coprime u v) (htick : 0 < tick)
    (hΔx : (P x).interval = ((u + v : ℕ) : ℚ) * tick) (hΔi : (P i).interval = v * tick)
    (hr : ∀ n, (P x).reads n = [(i, (affineRead (u + v) v (u + v - 1) n : ℤ))]) :
    IsLeastTail P W x
      (phaseStartupLatency (((u + v - 1 : ℕ) : ℚ) / v) (((u + v : ℕ) : ℚ) / v) (W i)) :=
  affine_leastTail_of P W x i (u + v) v (u + v - 1) _ tick hΔx hΔi hr fun w =>
    thetaTail_exact u v (W i) w (P x).origin hv hcop tick htick
-- ANCHOR_END: theta_least_tail

/- Rozplot ~Theta: p = u + v, q = v, c = 0. -/
-- ANCHOR: ntheta_least_tail
theorem ntheta_leastTail (P : ℕ → PlanNode) (W : ℕ → ℕ) (x i u v : ℕ) (tick : ℚ)
    (hv : 0 < v) (htick : 0 < tick)
    (hΔx : (P x).interval = ((u + v : ℕ) : ℚ) * tick) (hΔi : (P i).interval = v * tick)
    (hr : ∀ n, (P x).reads n = [(i, (affineRead (u + v) v 0 n : ℤ))]) :
    IsLeastTail P W x (phaseStartupLatency 0 (((u + v : ℕ) : ℚ) / v) (W i)) :=
  affine_leastTail_of P W x i (u + v) v 0 _ tick hΔx hΔi hr fun w =>
    nthetaTail_exact u v (W i) w (P x).origin hv tick htick
-- ANCHOR_END: ntheta_least_tail

/- Okno AGSE @(h, w) nad zrodlem o F polach: rekord n czyta pozycje n*h-k, k < w, czyli
   rekordy (n*h-k)/F; pozycja ujemna nie istnieje. Okno rekordowe MIN/MAX/AVG/SUMC(e : N)
   to F = h = 1 i w = N. -/
def windowReads (i F h w n : ℕ) : List (ℕ × ℤ) :=
  (List.range w).map fun (k : ℕ) => (i, (((n * h : ℕ) : ℤ) - (k : ℤ)) / F)

theorem window_localOK_iff (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i F h w wt : ℕ) (tick : ℚ)
    (hF : 0 < F) (hw : 0 < w) (htick : 0 < tick)
    (hΔv : (P v).interval = h * tick) (hΔi : (P i).interval = F * tick)
    (hr : ∀ n, (P v).reads n = windowReads i F h w n) :
    localOK P W v wt ↔ ∀ n, (P v).origin ≤ n → affineReady h F 0 (W i) wt tick n := by
  have hFz : (0 : ℤ) < F := by exact_mod_cast hF
  rw [newest_localOK_iff P W v i wt (affineRead h F 0) (by rw [hΔi]; positivity)]
  · unfold emitTime affineReady
    rw [hΔv, hΔi]
  · intro n ij hij
    rw [hr n] at hij
    obtain ⟨k, -, rfl⟩ := List.mem_map.mp hij
    refine ⟨rfl, ?_⟩
    show (((n * h : ℕ) : ℤ) - k) / F ≤ ((affineRead h F 0 n : ℕ) : ℤ)
    unfold affineRead
    rw [Nat.add_zero, Int.natCast_div]
    exact Int.ediv_le_ediv hFz (by omega)
  · intro n
    rw [hr n]
    refine List.mem_map.mpr ⟨0, List.mem_range.mpr hw, ?_⟩
    unfold affineRead
    rw [Nat.add_zero, Int.natCast_div]
    simp

-- ANCHOR: agse_least_tail
theorem agse_leastTail (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i F h w : ℕ) (tick : ℚ)
    (hF : 0 < F) (hh : 0 < h) (hw : 0 < w) (htick : 0 < tick)
    (hΔv : (P v).interval = h * tick) (hΔi : (P i).interval = F * tick)
    (hr : ∀ n, (P v).reads n = windowReads i F h w n) :
    IsLeastTail P W v (⌈((1 + W i : ℕ) : ℚ) * ((F : ℚ) / h)⌉₊ - 1) :=
  leastTail_of_exact P W v _ fun wt =>
    (agseTail_exact F h (W i) wt (P v).origin hF hh tick htick).trans
      (window_localOK_iff P W v i F h w wt tick hF hw htick hΔv hΔi hr).symm
-- ANCHOR_END: agse_least_tail

-- ANCHOR: record_window_least_tail
theorem recordWindow_leastTail (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i N : ℕ)
    (hN : 0 < N) (hpos : 0 < (P i).interval) (hΔ : (P v).interval = (P i).interval)
    (hr : ∀ n, (P v).reads n = windowReads i 1 1 N n) :
    IsLeastTail P W v (W i) :=
  leastTail_of_exact P W v _ fun wt =>
    (recordWindowTail_exact (W i) wt (P v).origin (P i).interval hpos).trans
      (window_localOK_iff P W v i 1 1 N wt (P i).interval one_pos hN hpos
        (by rw [hΔ]; simp) (by simp) hr).symm
-- ANCHOR_END: record_window_least_tail

/- Przesuniecie >m: rekord n czyta rekord n-m, ten sam interwal, ogon max(0, W_S - m). -/
-- ANCHOR: shift_least_tail
theorem shift_leastTail (P : ℕ → PlanNode) (W : ℕ → ℕ) (v i m : ℕ)
    (hpos : 0 < (P i).interval) (hΔ : (P v).interval = (P i).interval)
    (hO : (P v).origin = (P i).origin + m)
    (hr : ∀ n, (P v).reads n = [(i, (n : ℤ) - m)]) :
    IsLeastTail P W v (W i - m) := by
  let S : CausalStream Unit (P i).interval := ⟨(P i).origin, W i, fun _ => ()⟩
  refine leastTail_of_exact P W v _ fun w => ?_
  have hc := causalShift_tail_exact m w S
  simp only [causalShift, S] at hc
  rw [hc]
  unfold localOK emitTime
  simp only [hr, List.mem_singleton, forall_eq, hΔ, hO]
  apply forall₂_congr
  intro n hn
  rw [mul_le_mul_iff_of_pos_right hpos, Nat.cast_le]
  omega
-- ANCHOR_END: shift_least_tail

/- Przeplot: rekord n czyta jeden rekord, z A albo z B wedlug selectsA. Poczatek zalezy od
   obu wejsc (interleaveDeps), wiec deps jest tu szersze od reads. -/
def interleaveReads (iA iB a b n : ℕ) : List (ℕ × ℤ) :=
  [if selectsA a b n then (iA, (selectedIndex a b n : ℤ)) else (iB, (selectedIndex a b n : ℤ))]

def interleaveDeps (iA iB a b n : ℕ) : List (ℕ × ℤ) :=
  [(iA, (indexA a b n : ℤ)), (iB, (indexB a b n : ℤ))]

theorem interleave_readsInDeps (iA iB a b n : ℕ) (ij : ℕ × ℤ)
    (h : ij ∈ interleaveReads iA iB a b n) : ij ∈ interleaveDeps iA iB a b n := by
  unfold interleaveReads at h
  rw [List.mem_singleton] at h
  unfold interleaveDeps selectedIndex at *
  cases hs : selectsA a b n <;> simp only [hs] at h <;> subst h <;> simp [indexB, indexA, progress]

-- ANCHOR: interleave_least_tail
theorem interleave_leastTail (P : ℕ → PlanNode) (W : ℕ → ℕ) (v iA iB a b : ℕ) (tick : ℚ)
    (ha : 0 < a) (hb : 0 < b) (htick : 0 < tick)
    (hΔv : (P v).interval = ((a : ℚ) * b / (a + b)) * tick)
    (hΔA : (P iA).interval = a * tick) (hΔB : (P iB).interval = b * tick)
    (hr : ∀ n, (P v).reads n = interleaveReads iA iB a b n) :
    IsLeastTail P W v (exactInterleaveTail a b (W iA) (W iB)).toNat := by
  refine leastTail_of_exact P W v _ fun w => ?_
  rw [exactInterleaveTail_exact a b (W iA) (W iB) w (P v).origin ha hb tick htick]
  unfold localOK emitTime
  simp only [hr, interleaveReads, List.mem_singleton, forall_eq]
  apply forall₂_congr
  intro n _
  unfold interleaveReady
  cases selectsA a b n <;> simp [hΔv, hΔA, hΔB]
-- ANCHOR_END: interleave_least_tail

/- ===== Poczatki ===== -/

-- ANCHOR: affine_origin_rule
theorem affine_originRule (P : ℕ → PlanNode) (v i p q c : ℕ) (hp : 0 < p) (hq : 0 < q)
    (hO : (P v).origin = affineOrigin p q c (P i).origin hp hq)
    (hd : ∀ n, (P v).deps n = [(i, (affineRead p q c n : ℤ))]) : OriginRule P v := by
  intro n
  rw [hO, affineOrigin_le_iff, hd n]
  simp
-- ANCHOR_END: affine_origin_rule

theorem projection_originRule (P : ℕ → PlanNode) (v i : ℕ) (hO : (P v).origin = (P i).origin)
    (hd : ∀ n, (P v).deps n = [(i, (n : ℤ))]) : OriginRule P v := by
  intro n
  rw [hO, hd n]
  simp

-- ANCHOR: sum_origin_rule
theorem sum_originRule (P : ℕ → PlanNode) (v i1 i2 p1 q1 p2 q2 : ℕ)
    (hp1 : 0 < p1) (hq1 : 0 < q1) (hp2 : 0 < p2) (hq2 : 0 < q2)
    (hO : (P v).origin = max (affineOrigin p1 q1 0 (P i1).origin hp1 hq1)
      (affineOrigin p2 q2 0 (P i2).origin hp2 hq2))
    (hd : ∀ n, (P v).deps n =
      [(i1, (affineRead p1 q1 0 n : ℤ)), (i2, (affineRead p2 q2 0 n : ℤ))]) :
    OriginRule P v := by
  intro n
  rw [hO, sumOrigin_le_iff, hd n]
  simp
-- ANCHOR_END: sum_origin_rule

-- ANCHOR: window_origin_rule
theorem window_originRule (P : ℕ → PlanNode) (v i F h w : ℕ) (hF : 0 < F) (hh : 0 < h)
    (hw : 0 < w) (hO : (P v).origin = agseOrigin F h w (P i).origin)
    (hd : ∀ n, (P v).deps n = windowReads i F h w n) : OriginRule P v := by
  have hFz : (0 : ℤ) < F := by exact_mod_cast hF
  intro n
  rw [hO, agseOrigin_le_iff F h w (P i).origin n hh, hd n]
  simp only [windowReads, List.mem_map, List.mem_range, forall_exists_index, and_imp,
    forall_apply_eq_imp_iff₂]
  generalize (P i).origin = Os
  constructor
  · intro hle k hk
    rw [Int.le_ediv_iff_mul_le hFz]
    have : ((Os * F + (w - 1) : ℕ) : ℤ) ≤ ((n * h : ℕ) : ℤ) := by exact_mod_cast hle
    push_cast at this ⊢
    omega
  · intro hall
    have hk := hall (w - 1) (by omega)
    rw [Int.le_ediv_iff_mul_le hFz] at hk
    have : ((Os * F + (w - 1) : ℕ) : ℤ) ≤ ((n * h : ℕ) : ℤ) := by
      push_cast [Nat.cast_sub hw] at hk ⊢
      omega
    exact_mod_cast this
-- ANCHOR_END: window_origin_rule

/- Przesuniecie >m: poczatek O_S + m. -/
-- ANCHOR: shift_origin_rule
theorem shift_originRule (P : ℕ → PlanNode) (v i m : ℕ)
    (hO : (P v).origin = (P i).origin + m)
    (hd : ∀ n, (P v).deps n = [(i, (n : ℤ) - m)]) : OriginRule P v := by
  intro n
  rw [hO, hd n]
  simp only [List.mem_singleton, forall_eq]
  omega
-- ANCHOR_END: shift_origin_rule

-- ANCHOR: interleave_origin_rule
theorem interleave_originRule (P : ℕ → PlanNode) (v iA iB a b : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hO : (P v).origin = interleaveOrigin a b (P iA).origin (P iB).origin ha hb)
    (hd : ∀ n, (P v).deps n = interleaveDeps iA iB a b n) : OriginRule P v := by
  intro n
  rw [hO, interleaveOrigin_le_iff, hd n]
  simp [interleaveDeps, originReady]
-- ANCHOR_END: interleave_origin_rule

end Profs
