import Mathlib.Algebra.Order.Floor.Semifield
import Mathlib.Data.Rat.Floor
import Mathlib.Tactic

namespace Profs

/- Granice rozruchowe operatorow o afinicznej mapie odczytu (Tabela 1 publikacji DEBS):
   suma strumieni, roznica, rozploty Theta i ~Theta, okno AGSE i projekcja.

   Rekord n wyjscia czyta rekord affineRead p q c n wejscia. Interwaly wynosza
   Delta_out = p*tick i Delta_src = q*tick, wiec r = Delta_out/Delta_src = p/q.
   Mapy silnika z SOperations.hpp maja te postac:
     Add      floor(n*Delta/Delta_s)          p/q = Delta/Delta_s,            c = 0
     Subtract ceil(n*Delta/Delta_s)           p/q = Delta/Delta_s skrocone,   c = q - 1
     Div      n + ceil((n+1)*u/v)             p = u + v, q = v, u/v = Delta/Delta_b skrocone,
                                              c = u + v - 1
     Mod      n + floor(n*u/v)                p = u + v, q = v, u/v = Delta/Delta_a, c = 0
     AGSE     floor(n*h/F)                    p = h, q = F, c = 0
   Model dostepnosci jest ten sam co w interleaveReady: rekord j wejscia o ogonie Ws jest okreslony
   w chwili (j+1+Ws)*Delta_src, a slot n wyjscia konczy sie w chwili (n+1+W)*Delta_out. -/

-- ANCHOR: affine_read
def affineRead (p q c n : ℕ) : ℕ := (n * p + c) / q
-- ANCHOR_END: affine_read

def affineReady (p q c Ws W : ℕ) (tick : ℚ) (n : ℕ) : Prop :=
  ((affineRead p q c n + 1 + Ws : ℕ) : ℚ) * ((q : ℚ) * tick) ≤
    ((n + 1 + W : ℕ) : ℚ) * ((p : ℚ) * tick)

/- Postac zamknieta Phi(kappa, p/q, Ws) = max(0, ceil((kappa+1+Ws)/r) - 1) przy
   kappa = (c - c mod g)/q, g = gcd(p, q). Licznik jest dodatni, wiec max(0, .) jest zbedne. -/
-- ANCHOR: affine_tail
def affineTail (p q c Ws : ℕ) : ℕ :=
  (c - c % Nat.gcd p q + q * (1 + Ws) + p - 1) / p - 1
-- ANCHOR_END: affine_tail

theorem affineReady_iff_nat (p q c Ws W n : ℕ) (tick : ℚ) (htick : 0 < tick) :
    affineReady p q c Ws W tick n ↔
      q * (affineRead p q c n + 1 + Ws) ≤ p * (n + 1 + W) := by
  unfold affineReady
  constructor
  · intro h
    have h' : (((q * (affineRead p q c n + 1 + Ws) : ℕ) : ℚ)) * tick ≤
        ((p * (n + 1 + W) : ℕ) : ℚ) * tick := by
      push_cast at h ⊢
      nlinarith
    exact_mod_cast le_of_mul_le_mul_right h' htick
  · intro h
    have h' : ((q * (affineRead p q c n + 1 + Ws) : ℕ) : ℚ) ≤ ((p * (n + 1 + W) : ℕ) : ℚ) := by
      exact_mod_cast h
    push_cast at h' ⊢
    nlinarith

/- Reszta rho_n = (n*p + c) mod q rozstrzyga gotowosc: czesc liniowa n*p skraca sie obustronnie. -/
theorem affineReady_iff_residue (p q c Ws W n : ℕ) (tick : ℚ) (htick : 0 < tick) :
    affineReady p q c Ws W tick n ↔
      c + q * (1 + Ws) ≤ (n * p + c) % q + p * (1 + W) := by
  rw [affineReady_iff_nat p q c Ws W n tick htick]
  have hdm := Nat.div_add_mod (n * p + c) q
  unfold affineRead
  have e1 : q * ((n * p + c) / q + 1 + Ws) = q * ((n * p + c) / q) + q * (1 + Ws) := by ring
  have e2 : p * (n + 1 + W) = n * p + p * (1 + W) := by ring
  rw [e1, e2]
  generalize q * ((n * p + c) / q) = A at *
  generalize (n * p + c) % q = R at *
  generalize n * p = B at *
  omega

/- Reszta nigdy nie spada ponizej c mod g: g dzieli q i n*p. -/
theorem residue_ge (p q c n : ℕ) :
    c % Nat.gcd p q ≤ (n * p + c) % q := by
  set g := Nat.gcd p q
  have hgq : g ∣ q := Nat.gcd_dvd_right p q
  have hgp : g ∣ n * p := Dvd.dvd.mul_left (Nat.gcd_dvd_left p q) n
  have h1 : (n * p + c) % q % g = (n * p + c) % g := Nat.mod_mod_of_dvd _ hgq
  have h2 : (n * p + c) % g = c % g := by
    obtain ⟨t, ht⟩ := hgp
    rw [ht, Nat.mul_add_mod]
  have h3 : (n * p + c) % q % g ≤ (n * p + c) % q := Nat.mod_le _ _
  omega

/- Reszta c mod g jest osiagana: z Bezouta g = p*x + q*y wybieramy n = -x*(c/g) mod q. -/
theorem residue_attained (p q c : ℕ) (hq : 0 < q) :
    ∃ n, (n * p + c) % q = c % Nat.gcd p q := by
  set g := Nat.gcd p q
  have hg : (0 : ℤ) < g := by exact_mod_cast Nat.gcd_pos_of_pos_right p hq
  have hgle : g ≤ q := Nat.le_of_dvd hq (Nat.gcd_dvd_right p q)
  have hbez : (g : ℤ) = p * Nat.gcdA p q + q * Nat.gcdB p q := Nat.gcd_eq_gcd_ab p q
  set t : ℤ := ((c / g : ℕ) : ℤ)
  set x : ℤ := Nat.gcdA p q
  set y : ℤ := Nat.gcdB p q
  have hqz : (0 : ℤ) < q := by exact_mod_cast hq
  set m : ℤ := (-(x * t)) % q
  have hm0 : 0 ≤ m := Int.emod_nonneg _ (ne_of_gt hqz)
  refine ⟨m.toNat, ?_⟩
  have hc : (c : ℤ) = g * t + ((c % g : ℕ) : ℤ) := by
    have := Nat.div_add_mod c g
    simp only [t]
    push_cast
    exact_mod_cast this.symm
  have hlt : ((c % g : ℕ) : ℤ) < q := by
    have : c % g < g := Nat.mod_lt _ (by exact_mod_cast hg)
    exact_mod_cast lt_of_lt_of_le this hgle
  have hnn : (0 : ℤ) ≤ ((c % g : ℕ) : ℤ) := by positivity
  have key : ((m.toNat * p + c : ℕ) : ℤ) % q = ((c % g : ℕ) : ℤ) := by
    have hmt : ((m.toNat : ℕ) : ℤ) = m := Int.toNat_of_nonneg hm0
    have hL : ((m.toNat * p + c : ℕ) : ℤ) = m * p + (c : ℤ) := by push_cast; rw [hmt]
    rw [hL, hc]
    -- m*p + g*t + r = m*p + (p*x + q*y)*t + r = (m + x*t)*p + q*(y*t) + r, a m + x*t = 0 mod q
    have hmx : (m + x * t) % q = 0 := by
      simp only [m]
      rw [Int.add_emod, Int.emod_emod_of_dvd _ (dvd_refl _), ← Int.add_emod]
      simp
    obtain ⟨k, hk⟩ := Int.dvd_of_emod_eq_zero hmx
    have : m * p + (g * t + ((c % g : ℕ) : ℤ)) = q * (k * p + y * t) + ((c % g : ℕ) : ℤ) := by
      have hm' : m = q * k - x * t := by linarith
      rw [hm', hbez]
      ring
    rw [this, Int.mul_add_emod_self_left]
    exact Int.emod_eq_of_lt hnn hlt
  have := key
  exact_mod_cast this

/- Okresowosc: reszta powtarza sie co q slotow, wiec jest osiagana od dowolnego progu. -/
theorem residue_attained_from (p q c N0 : ℕ) (hq : 0 < q) :
    ∃ n, N0 ≤ n ∧ (n * p + c) % q = c % Nat.gcd p q := by
  obtain ⟨m, hm⟩ := residue_attained p q c hq
  refine ⟨m + q * N0, ?_, ?_⟩
  · nlinarith
  · have : (m + q * N0) * p + c = (m * p + c) + q * (N0 * p) := by ring
    rw [this, Nat.add_mul_mod_self_left, hm]

theorem ceil_pred_le_iff (a p W : ℕ) (hp : 0 < p) :
    (a + p - 1) / p - 1 ≤ W ↔ a ≤ p * (1 + W) := by
  rw [Nat.sub_le_iff_le_add, ← Nat.lt_succ_iff, Nat.div_lt_iff_lt_mul hp]
  have e : (W + 1).succ * p = p * (1 + W) + p := by rw [Nat.succ_eq_add_one]; ring
  rw [e]
  generalize p * (1 + W) = M
  omega

/- Ogon jest dokladna granica dostepnosci od dowolnego progu N0. -/
-- ANCHOR: affine_tail_exact
theorem affineTail_exact (p q c Ws W N0 : ℕ) (hp : 0 < p) (hq : 0 < q)
    (tick : ℚ) (htick : 0 < tick) :
    affineTail p q c Ws ≤ W ↔ ∀ n, N0 ≤ n → affineReady p q c Ws W tick n := by
  simp only [affineReady_iff_residue p q c Ws W _ tick htick]
  unfold affineTail
  rw [ceil_pred_le_iff _ p W hp]
  have hcg : c % Nat.gcd p q ≤ c := Nat.mod_le _ _
  constructor
  · intro h n _
    have := residue_ge p q c n
    omega
  · intro h
    obtain ⟨n, hn, hres⟩ := residue_attained_from p q c N0 hq
    have := h n hn
    omega
-- ANCHOR_END: affine_tail_exact

/- Sufit ilorazu liczb naturalnych jako dzielenie calkowite (ceilR i roundedAwayFromZero silnika
   dla argumentow dodatnich). -/
theorem natCeil_div (m n : ℕ) (hn : 0 < n) : ⌈(m : ℚ) / n⌉₊ = (m + n - 1) / n := by
  have hnq : (0 : ℚ) < n := by exact_mod_cast hn
  apply le_antisymm
  · rw [Nat.ceil_le, div_le_iff₀ hnq]
    have h := Nat.div_add_mod (m + n - 1) n
    have h2 := Nat.mod_lt (m + n - 1) hn
    have : m ≤ (m + n - 1) / n * n := by
      rw [mul_comm]
      generalize n * ((m + n - 1) / n) = A at *
      omega
    exact_mod_cast this
  · have hc := Nat.le_ceil ((m : ℚ) / n)
    rw [div_le_iff₀ hnq] at hc
    have hc' : m ≤ ⌈(m : ℚ) / n⌉₊ * n := by exact_mod_cast hc
    apply Nat.lt_succ_iff.mp
    rw [Nat.div_lt_iff_lt_mul hn]
    have e : (⌈(m : ℚ) / n⌉₊).succ * n = ⌈(m : ℚ) / n⌉₊ * n + n := by
      rw [Nat.succ_eq_add_one]; ring
    rw [e]
    generalize ⌈(m : ℚ) / n⌉₊ * n = K at *
    omega

/- Lustro PhaseStartupLatency(phaseBound, ratio, sourceLatency) z SOperations.hpp:
   max(0, ceil((phaseBound + sourceLatency + 1)/ratio) - 1). Nat.ceil obcina do zera jak max. -/
-- ANCHOR: phase_startup_latency
noncomputable def phaseStartupLatency (kappa ratio : ℚ) (Ws : ℕ) : ℕ :=
  ⌈(kappa + Ws + 1) / ratio⌉₊ - 1
-- ANCHOR_END: phase_startup_latency

theorem phaseStartupLatency_eq_engine (kappa ratio : ℚ) (Ws : ℕ) :
    (phaseStartupLatency kappa ratio Ws : ℤ) = max 0 (⌈(kappa + Ws + 1) / ratio⌉ - 1) := by
  unfold phaseStartupLatency
  rw [← Int.ceil_toNat]
  generalize ⌈(kappa + Ws + 1) / ratio⌉ = k
  omega

/- Postac zamknieta affineTail to PhaseStartupLatency z kappa = (c - c mod g)/q i r = p/q. -/
-- ANCHOR: affine_tail_phase
theorem affineTail_eq_phase (p q c Ws : ℕ) (hp : 0 < p) (hq : 0 < q) :
    affineTail p q c Ws =
      phaseStartupLatency (((c - c % Nat.gcd p q : ℕ) : ℚ) / q) ((p : ℚ) / q) Ws := by
  unfold affineTail phaseStartupLatency
  have hpq : (0 : ℚ) < p := by exact_mod_cast hp
  have hqq : (0 : ℚ) < q := by exact_mod_cast hq
  have e : (((c - c % Nat.gcd p q : ℕ) : ℚ) / q + Ws + 1) / ((p : ℚ) / q) =
      ((c - c % Nat.gcd p q + q * (1 + Ws) : ℕ) : ℚ) / p := by
    push_cast
    field_simp
    ring
  rw [e, natCeil_div _ _ hp]
-- ANCHOR_END: affine_tail_phase

/- ===== Operatory silnika ===== -/

/- Suma strumieni: skladowa czyta floor(n*Delta/Delta_s) (Add), c = 0; ogon wzgledem skladowej
   ceil((1+Ws)*Delta_s/Delta) - 1 (AddStartupLatency), ogon wezla to maksimum po skladowych. -/
theorem addRead_eq (p q n : ℕ) : affineRead p q 0 n = ⌊(n : ℚ) * ((p : ℚ) / q)⌋₊ := by
  unfold affineRead
  rw [← mul_div_assoc, show (n : ℚ) * p = ((n * p : ℕ) : ℚ) by push_cast; ring,
    Nat.floor_div_eq_div]
  simp

theorem addTail_eq (p q Ws : ℕ) (hp : 0 < p) :
    affineTail p q 0 Ws = ⌈((1 + Ws : ℕ) : ℚ) * ((q : ℚ) / p)⌉₊ - 1 := by
  unfold affineTail
  rw [← mul_div_assoc, show ((1 + Ws : ℕ) : ℚ) * q = ((q * (1 + Ws) : ℕ) : ℚ) by push_cast; ring,
    natCeil_div _ _ hp]
  simp

-- ANCHOR: sum_tail_exact
theorem sumTail_exact (p1 q1 W1 p2 q2 W2 W N0 : ℕ) (hp1 : 0 < p1) (hq1 : 0 < q1)
    (hp2 : 0 < p2) (hq2 : 0 < q2) (t1 t2 : ℚ) (ht1 : 0 < t1) (ht2 : 0 < t2) :
    max (affineTail p1 q1 0 W1) (affineTail p2 q2 0 W2) ≤ W ↔
      ∀ n, N0 ≤ n → affineReady p1 q1 0 W1 W t1 n ∧ affineReady p2 q2 0 W2 W t2 n := by
  rw [max_le_iff, affineTail_exact p1 q1 0 W1 W N0 hp1 hq1 t1 ht1,
    affineTail_exact p2 q2 0 W2 W N0 hp2 hq2 t2 ht2]
  constructor
  · exact fun ⟨h1, h2⟩ n hn => ⟨h1 n hn, h2 n hn⟩
  · exact fun h => ⟨fun n hn => (h n hn).1, fun n hn => (h n hn).2⟩
-- ANCHOR_END: sum_tail_exact

/- Roznica C-Delta: czyta ceil(n*p/q) (Subtract) przy p/q = Delta/Delta_s po skroceniu, czyli
   c = q - 1; ogon PhaseStartupLatency((q-1)/q, p/q, Ws) (SubtractStartupLatency). -/
theorem subtractRead_eq (p q n : ℕ) (hq : 0 < q) :
    affineRead p q (q - 1) n = ⌈(n : ℚ) * ((p : ℚ) / q)⌉₊ := by
  unfold affineRead
  rw [← mul_div_assoc, show (n : ℚ) * p = ((n * p : ℕ) : ℚ) by push_cast; ring, natCeil_div _ _ hq]
  congr 1
  omega

-- ANCHOR: subtract_tail_exact
theorem subtractTail_exact (p q Ws W N0 : ℕ) (hp : 0 < p) (hq : 0 < q) (hcop : Nat.Coprime p q)
    (tick : ℚ) (htick : 0 < tick) :
    phaseStartupLatency (((q - 1 : ℕ) : ℚ) / q) ((p : ℚ) / q) Ws ≤ W ↔
      ∀ n, N0 ≤ n → affineReady p q (q - 1) Ws W tick n := by
  have h := affineTail_eq_phase p q (q - 1) Ws hp hq
  rw [Nat.Coprime.gcd_eq_one hcop, Nat.mod_one, Nat.sub_zero] at h
  rw [← h]
  exact affineTail_exact p q (q - 1) Ws W N0 hp hq tick htick
-- ANCHOR_END: subtract_tail_exact

/- Rozplot Theta (Div): czyta n + ceil((n+1)*u/v) przy u/v = Delta/Delta_b po skroceniu.
   Wejscie jest przeplotem, Delta_s = Delta*Delta_b/(Delta + Delta_b), wiec r = (u+v)/v,
   a mapa ma postac afiniczna z p = u + v, q = v, c = u + v - 1 (ThetaStartupLatency). -/
theorem thetaRead_eq (u v n : ℕ) (hv : 0 < v) :
    affineRead (u + v) v (u + v - 1) n = n + ⌈((n + 1 : ℕ) : ℚ) * ((u : ℚ) / v)⌉₊ := by
  unfold affineRead
  rw [← mul_div_assoc, show ((n + 1 : ℕ) : ℚ) * u = (((n + 1) * u : ℕ) : ℚ) by push_cast; ring,
    natCeil_div _ _ hv]
  have e : n * (u + v) + (u + v - 1) = ((n + 1) * u + v - 1) + v * n := by
    have : 1 ≤ u + v := by omega
    zify [this, show 1 ≤ (n + 1) * u + v by nlinarith]
    ring
  rw [e, Nat.add_mul_div_left _ _ hv]
  ring

theorem theta_ratio (d db : ℚ) (u v : ℕ) (hd : 0 < d) (hdb : 0 < db) (hv : 0 < v)
    (huv : d / db = (u : ℚ) / v) :
    d / (d * db / (d + db)) = ((u + v : ℕ) : ℚ) / v := by
  have hvq : (0 : ℚ) < v := by exact_mod_cast hv
  have hu : (u : ℚ) = d / db * v := by rw [huv]; field_simp
  push_cast
  rw [hu]
  field_simp

-- ANCHOR: theta_tail_exact
theorem thetaTail_exact (u v Ws W N0 : ℕ) (hv : 0 < v) (hcop : Nat.Coprime u v)
    (tick : ℚ) (htick : 0 < tick) :
    phaseStartupLatency (((u + v - 1 : ℕ) : ℚ) / v) (((u + v : ℕ) : ℚ) / v) Ws ≤ W ↔
      ∀ n, N0 ≤ n → affineReady (u + v) v (u + v - 1) Ws W tick n := by
  have hp : 0 < u + v := by omega
  have hg : Nat.gcd (u + v) v = 1 := by
    rw [Nat.gcd_comm, Nat.gcd_add_self_right, Nat.gcd_comm]; exact hcop
  have h := affineTail_eq_phase (u + v) v (u + v - 1) Ws hp hv
  rw [hg, Nat.mod_one, Nat.sub_zero] at h
  rw [← h]
  exact affineTail_exact (u + v) v (u + v - 1) Ws W N0 hp hv tick htick
-- ANCHOR_END: theta_tail_exact

/- Rozplot ~Theta (Mod): czyta n + floor(n*u/v) przy u/v = Delta/Delta_a; p = u + v, q = v,
   c = 0, ogon PhaseStartupLatency(0, r, Ws) (NThetaStartupLatency). Skracanie nie jest potrzebne. -/
theorem nthetaRead_eq (u v n : ℕ) (hv : 0 < v) :
    affineRead (u + v) v 0 n = n + ⌊(n : ℚ) * ((u : ℚ) / v)⌋₊ := by
  unfold affineRead
  rw [← mul_div_assoc, show (n : ℚ) * u = ((n * u : ℕ) : ℚ) by push_cast; ring,
    Nat.floor_div_eq_div]
  have e : n * (u + v) + 0 = n * u + v * n := by ring
  rw [e, Nat.add_mul_div_left _ _ hv]
  ring

-- ANCHOR: ntheta_tail_exact
theorem nthetaTail_exact (u v Ws W N0 : ℕ) (hv : 0 < v) (tick : ℚ) (htick : 0 < tick) :
    phaseStartupLatency 0 (((u + v : ℕ) : ℚ) / v) Ws ≤ W ↔
      ∀ n, N0 ≤ n → affineReady (u + v) v 0 Ws W tick n := by
  have hp : 0 < u + v := by omega
  have h := affineTail_eq_phase (u + v) v 0 Ws hp hv
  simp only [Nat.zero_mod, Nat.sub_zero, Nat.cast_zero, zero_div] at h
  rw [← h]
  exact affineTail_exact (u + v) v 0 Ws W N0 hp hv tick htick
-- ANCHOR_END: ntheta_tail_exact

/- Okno AGSE @(h, w) nad zrodlem o F polach: rekord n obejmuje splaszczone pozycje
   n*h-(w-1) ... n*h, najnowsze pole n*h lezy w rekordzie floor(n*h/F) = affineRead h F 0 n,
   Delta_out = Delta_s*h/F. Starsze pola leza w rekordach wczesniejszych, wiec okno jest gotowe
   dokladnie wtedy, gdy gotowe jest jego najnowsze pole. -/
theorem agse_window_ready (F h Ws n : ℕ) (ds T : ℚ) (hds : 0 < ds) :
    (∀ j, j ≤ n * h → ((j / F + 1 + Ws : ℕ) : ℚ) * ds ≤ T) ↔
      ((affineRead h F 0 n + 1 + Ws : ℕ) : ℚ) * ds ≤ T := by
  unfold affineRead
  simp only [Nat.add_zero]
  constructor
  · intro hall
    exact hall _ le_rfl
  · intro hlast j hj
    have hmono : j / F ≤ n * h / F := Nat.div_le_div_right hj
    have : ((j / F + 1 + Ws : ℕ) : ℚ) ≤ ((n * h / F + 1 + Ws : ℕ) : ℚ) := by
      exact_mod_cast (by omega : j / F + 1 + Ws ≤ n * h / F + 1 + Ws)
    exact le_trans (mul_le_mul_of_nonneg_right this hds.le) hlast

-- ANCHOR: agse_tail_exact
theorem agseTail_exact (F h Ws W N0 : ℕ) (hF : 0 < F) (hh : 0 < h)
    (tick : ℚ) (htick : 0 < tick) :
    ⌈((1 + Ws : ℕ) : ℚ) * ((F : ℚ) / h)⌉₊ - 1 ≤ W ↔
      ∀ n, N0 ≤ n → affineReady h F 0 Ws W tick n := by
  rw [← addTail_eq h F Ws hh]
  exact affineTail_exact h F 0 Ws W N0 hh hF tick htick
-- ANCHOR_END: agse_tail_exact

/- Projekcja i redukcje krotek czytaja rekord n, czyli p = q = 1, c = 0: ogon W_S. -/
-- ANCHOR: projection_tail_exact
theorem projectionTail_exact (Ws W N0 : ℕ) (tick : ℚ) (htick : 0 < tick) :
    Ws ≤ W ↔ ∀ n, N0 ≤ n → affineReady 1 1 0 Ws W tick n := by
  have h : affineTail 1 1 0 Ws = Ws := by unfold affineTail; simp
  have := affineTail_exact 1 1 0 Ws W N0 one_pos one_pos tick htick
  rwa [h] at this
-- ANCHOR_END: projection_tail_exact

/- ===== Poczatki logiczne ===== -/

/- Rekord n wyjscia jest okreslony, gdy okreslony jest czytany rekord wejscia, czyli
   O_S <= affineRead n. Mapa jest niemalejaca, wiec rekordy okreslone tworza ciagly sufiks,
   a poczatek to pierwszy indeks osiagajacy prog (firstIndexReaching w compiler.cpp). -/
theorem affineRead_mono (p q c : ℕ) : Monotone (affineRead p q c) := by
  intro m n hmn
  unfold affineRead
  exact Nat.div_le_div_right (by nlinarith)

theorem affineRead_reaches (p q c o : ℕ) (hp : 0 < p) (hq : 0 < q) :
    ∃ n, o ≤ affineRead p q c n := by
  refine ⟨o * q, ?_⟩
  unfold affineRead
  rw [Nat.le_div_iff_mul_le hq]
  have : o * q ≤ o * q * p := Nat.le_mul_of_pos_right _ hp
  omega

noncomputable def affineOrigin (p q c o : ℕ) (hp : 0 < p) (hq : 0 < q) : ℕ :=
  Nat.find (affineRead_reaches p q c o hp hq)

-- ANCHOR: affine_origin
theorem affineOrigin_le_iff (p q c o n : ℕ) (hp : 0 < p) (hq : 0 < q) :
    affineOrigin p q c o hp hq ≤ n ↔ o ≤ affineRead p q c n := by
  constructor
  · intro hn
    exact le_trans (Nat.find_spec (affineRead_reaches p q c o hp hq)) (affineRead_mono p q c hn)
  · exact Nat.find_min' (affineRead_reaches p q c o hp hq)
-- ANCHOR_END: affine_origin

/- Suma: poczatek wezla to maksimum progow skladowych. -/
-- ANCHOR: sum_origin
theorem sumOrigin_le_iff (p1 q1 o1 p2 q2 o2 n : ℕ) (hp1 : 0 < p1) (hq1 : 0 < q1)
    (hp2 : 0 < p2) (hq2 : 0 < q2) :
    max (affineOrigin p1 q1 0 o1 hp1 hq1) (affineOrigin p2 q2 0 o2 hp2 hq2) ≤ n ↔
      o1 ≤ affineRead p1 q1 0 n ∧ o2 ≤ affineRead p2 q2 0 n := by
  rw [max_le_iff, affineOrigin_le_iff, affineOrigin_le_iff]
-- ANCHOR_END: sum_origin

/- Projekcja przenosi poczatek bez zmian. -/
theorem projectionOrigin_eq (o : ℕ) : affineOrigin 1 1 0 o one_pos one_pos = o := by
  apply le_antisymm
  · rw [affineOrigin_le_iff]; unfold affineRead; simp
  · have h := (affineOrigin_le_iff 1 1 0 o (affineOrigin 1 1 0 o one_pos one_pos)
      one_pos one_pos).mp le_rfl
    unfold affineRead at h; simpa using h

/- AGSE: najmniejsze n, przy ktorym cale okno n*h-(w-1) ... n*h lezy w istniejacej czesci
   zrodla, czyli od pozycji O_S*F (AgseLogicalOrigin, w = |length| >= 1). -/
def agseOrigin (F h w Os : ℕ) : ℕ := (Os * F + (w - 1) + h - 1) / h

-- ANCHOR: agse_origin
theorem agseOrigin_le_iff (F h w Os n : ℕ) (hh : 0 < h) :
    agseOrigin F h w Os ≤ n ↔ Os * F + (w - 1) ≤ n * h := by
  unfold agseOrigin
  rw [← Nat.lt_succ_iff, Nat.div_lt_iff_lt_mul hh]
  have e : n.succ * h = n * h + h := by rw [Nat.succ_eq_add_one]; ring
  rw [e]
  generalize n * h = K
  omega
-- ANCHOR_END: agse_origin

theorem agseOrigin_eq_ceil (F h w Os : ℕ) (hh : 0 < h) :
    agseOrigin F h w Os = ⌈((Os * F + (w - 1) : ℕ) : ℚ) / h⌉₊ := by
  unfold agseOrigin
  rw [natCeil_div _ _ hh]

/- ===== Okno rekordowe w liscie SELECT ===== -/

/- MIN/MAX/AVG/SUMC(e : N): rekord n obejmuje rekordy zrodla n-(N-1) ... n, czyli to AGSE
   z F = h = 1 i w = N (pozycje n*h-(w-1) ... n*h, rekord floor(j/F) = j). Przy kilku
   szerokosciach w jednej liscie rozstrzyga najwieksza N (windowWidthOf w compiler.cpp).
   Prog zapisany dodawaniem: O_S <= n-(N-1) na liczbach naturalnych zachodziloby takze
   dla n < N-1, gdy okno siega przed indeks 0. -/
-- ANCHOR: record_window_origin
theorem recordWindowOrigin_le_iff (N Os n : ℕ) :
    agseOrigin 1 1 N Os ≤ n ↔ Os + (N - 1) ≤ n := by
  simpa using agseOrigin_le_iff 1 1 N Os n one_pos
-- ANCHOR_END: record_window_origin

/- Ogon okna rekordowego to ogon AGSE przy F = h = 1: ceil((1 + W_S)*1) - 1 = W_S. -/
-- ANCHOR: record_window_tail_exact
theorem recordWindowTail_exact (Ws W N0 : ℕ) (tick : ℚ) (htick : 0 < tick) :
    Ws ≤ W ↔ ∀ n, N0 ≤ n → affineReady 1 1 0 Ws W tick n := by
  have h : ⌈((1 + Ws : ℕ) : ℚ) * ((1 : ℕ) / (1 : ℕ) : ℚ)⌉₊ - 1 = Ws := by
    rw [Nat.cast_one, div_one, mul_one, Nat.ceil_natCast]; omega
  have := agseTail_exact 1 1 Ws W N0 one_pos one_pos tick htick
  rwa [h] at this
-- ANCHOR_END: record_window_tail_exact

end Profs
