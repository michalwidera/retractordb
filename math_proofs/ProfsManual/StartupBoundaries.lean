import VersoManual

open Verso.Genre Manual
open Verso.Code.External

set_option verso.exampleProject "."

#doc (Manual) "Granice rozruchowe Tabeli 1" =>
%%%
file := "startup-boundaries"
%%%

Tabela 1 publikacji DEBS (`Static startup-boundary calculus`) podaje dla każdego
rodzaju węzła planu dwie granice: początek logiczny `O`, czyli indeks pierwszego
rekordu, który w ogóle ma definicję, i ogon rozruchowy `W`, czyli liczbę początkowych
slotów własnego interwału, w których wynik jeszcze nie jest określony. Ten rozdział
dowodzi obu granic dla operatorów, których rekord `n` czyta jeden rekord wejścia
przez afiniczną mapę odczytu: sumy strumieni, różnicy `δ`, rozplotów `Θ` i `~Θ`,
okna AGSE i projekcji. Przesunięcie i przeplot mają osobny model w
`Profs/CausalShift.lean` i `Profs/InterleaveTailExact.lean` (rozdział o R1).

Rekord `n` wyjścia czyta rekord `(n*p + c)/q` wejścia. Interwał wyjścia wynosi
`p*tick`, a interwał wejścia `q*tick`, więc `r = Δ/Δ_S = p/q`. Mapy silnika z
`SOperations.hpp` mają tę postać przy następujących stałych: `Add` (`c = 0`),
`Subtract` (`c = q - 1` po skróceniu `p/q`), `Div` (`p = u + v`, `q = v`,
`c = u + v - 1`), `Mod` (`p = u + v`, `q = v`, `c = 0`) i okno AGSE (`p = h`,
`q = F`, `c = 0`). Zgodność map dowodzą lematy `addRead_eq`, `subtractRead_eq`,
`thetaRead_eq` i `nthetaRead_eq`.

```anchor affine_read (module := Profs.StartupBoundaries)
def affineRead (p q c n : ℕ) : ℕ := (n * p + c) / q
```

Model dostępności jest ten sam co dla przeplotu: rekord `j` wejścia o ogonie
`W_S` jest określony w chwili `(j + 1 + W_S)*Δ_S`, a slot `n` wyjścia o ogonie
`W` kończy się w chwili `(n + 1 + W)*Δ`. Ogon w postaci zamkniętej liczy
`affineTail`, gdzie `g = gcd(p, q)`:

```anchor affine_tail (module := Profs.StartupBoundaries)
def affineTail (p q c Ws : ℕ) : ℕ :=
  (c - c % Nat.gcd p q + q * (1 + Ws) + p - 1) / p - 1
```

Główne twierdzenie mówi, że to dokładna granica: `affineTail` nie przekracza `W`
wtedy i tylko wtedy, gdy każdy rekord od dowolnego progu `N0` jest dostępny w
swoim slocie. Część liniowa `n*p` skraca się po obu stronach warunku, więc
gotowość rozstrzyga reszta `(n*p + c) mod q`. Reszta nigdy nie spada poniżej
`c mod g` i osiąga tę wartość w każdym okresie `q` slotów (tożsamość Bézouta), więc
najgorszy przypadek występuje za każdym progiem.

```anchor affine_tail_exact -showProofStates (module := Profs.StartupBoundaries)
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
```

Silnik liczy ten ogon funkcją `PhaseStartupLatency(κ, r, W_S) =
max(0, ceil((κ + W_S + 1)/r) - 1)`. Jej lustrem w Lean jest `phaseStartupLatency`,
gdzie `Nat.ceil` obcina do zera tak jak `max` (`phaseStartupLatency_eq_engine`). Postać zamknięta `affineTail` jest
dokładnie tą funkcją przy `κ = (c - c mod g)/q`:

```anchor phase_startup_latency (module := Profs.StartupBoundaries)
noncomputable def phaseStartupLatency (kappa ratio : ℚ) (Ws : ℕ) : ℕ :=
  ⌈(kappa + Ws + 1) / ratio⌉₊ - 1
```

```anchor affine_tail_phase -showProofStates (module := Profs.StartupBoundaries)
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
```

# Operatory

Suma strumieni czyta składową przez `floor(n*Δ/Δ_s)`, więc `c = 0`. Ogon
względem jednej składowej to `ceil((1 + W_s)*Δ_s/Δ) - 1` (`AddStartupLatency`),
a ogon węzła to maksimum po składowych:

```anchor sum_tail_exact -showProofStates (module := Profs.StartupBoundaries)
theorem sumTail_exact (p1 q1 W1 p2 q2 W2 W N0 : ℕ) (hp1 : 0 < p1) (hq1 : 0 < q1)
    (hp2 : 0 < p2) (hq2 : 0 < q2) (t1 t2 : ℚ) (ht1 : 0 < t1) (ht2 : 0 < t2) :
    max (affineTail p1 q1 0 W1) (affineTail p2 q2 0 W2) ≤ W ↔
      ∀ n, N0 ≤ n → affineReady p1 q1 0 W1 W t1 n ∧ affineReady p2 q2 0 W2 W t2 n := by
  rw [max_le_iff, affineTail_exact p1 q1 0 W1 W N0 hp1 hq1 t1 ht1,
    affineTail_exact p2 q2 0 W2 W N0 hp2 hq2 t2 ht2]
  constructor
  · exact fun ⟨h1, h2⟩ n hn => ⟨h1 n hn, h2 n hn⟩
  · exact fun h => ⟨fun n hn => (h n hn).1, fun n hn => (h n hn).2⟩
```

Różnica `δ` czyta `ceil(n*p/q)` przy `p/q` skróconym, czyli `c = q - 1`, i ma ogon
`Φ((q-1)/q, p/q, W_S)` (`SubtractStartupLatency`):

```anchor subtract_tail_exact -showProofStates (module := Profs.StartupBoundaries)
theorem subtractTail_exact (p q Ws W N0 : ℕ) (hp : 0 < p) (hq : 0 < q) (hcop : Nat.Coprime p q)
    (tick : ℚ) (htick : 0 < tick) :
    phaseStartupLatency (((q - 1 : ℕ) : ℚ) / q) ((p : ℚ) / q) Ws ≤ W ↔
      ∀ n, N0 ≤ n → affineReady p q (q - 1) Ws W tick n := by
  have h := affineTail_eq_phase p q (q - 1) Ws hp hq
  rw [Nat.Coprime.gcd_eq_one hcop, Nat.mod_one, Nat.sub_zero] at h
  rw [← h]
  exact affineTail_exact p q (q - 1) Ws W N0 hp hq tick htick
```

Rozplot `Θ` czyta pozycję `n + ceil((n+1)*u/v)` przeplotu przy `u/v = Δ/Δ_b` po
skróceniu. Model zakłada, że wejście jest przeplotem, czyli
`Δ_S = Δ*Δ_b/(Δ + Δ_b)`. Wtedy `r = (u+v)/v` (`theta_ratio`), a ogon wynosi
`Φ((u+v-1)/v, r, W_S)` (`ThetaStartupLatency`):

```anchor theta_tail_exact -showProofStates (module := Profs.StartupBoundaries)
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
```

Rozplot `~Θ` czyta `n + floor(n*u/v)`, więc `c = 0`, ogon wynosi `Φ(0, r, W_S)`
(`NThetaStartupLatency`), a skracanie nie jest potrzebne:

```anchor ntheta_tail_exact -showProofStates (module := Profs.StartupBoundaries)
theorem nthetaTail_exact (u v Ws W N0 : ℕ) (hv : 0 < v) (tick : ℚ) (htick : 0 < tick) :
    phaseStartupLatency 0 (((u + v : ℕ) : ℚ) / v) Ws ≤ W ↔
      ∀ n, N0 ≤ n → affineReady (u + v) v 0 Ws W tick n := by
  have hp : 0 < u + v := by omega
  have h := affineTail_eq_phase (u + v) v 0 Ws hp hv
  simp only [Nat.zero_mod, Nat.sub_zero, Nat.cast_zero, zero_div] at h
  rw [← h]
  exact affineTail_exact (u + v) v 0 Ws W N0 hp hv tick htick
```

Okno AGSE `@(h, w)` nad źródłem o `F` polach obejmuje spłaszczone pozycje
`n*h - (w-1)` ... `n*h`. Najnowsze pole leży w rekordzie `floor(n*h/F)`, a
starsze pola w rekordach wcześniejszych, więc okno jest gotowe dokładnie wtedy,
gdy gotowe jest jego najnowsze pole (`agse_window_ready`). Ogon to
`ceil((1 + W_S)*F/h) - 1` (`AgseStartupLatency`):

```anchor agse_tail_exact -showProofStates (module := Profs.StartupBoundaries)
theorem agseTail_exact (F h Ws W N0 : ℕ) (hF : 0 < F) (hh : 0 < h)
    (tick : ℚ) (htick : 0 < tick) :
    ⌈((1 + Ws : ℕ) : ℚ) * ((F : ℚ) / h)⌉₊ - 1 ≤ W ↔
      ∀ n, N0 ≤ n → affineReady h F 0 Ws W tick n := by
  rw [← addTail_eq h F Ws hh]
  exact affineTail_exact h F 0 Ws W N0 hh hF tick htick
```

Projekcja i redukcje krotek czytają rekord `n` (`p = q = 1`, `c = 0`), więc ich
ogon to ogon źródła (`projectionTail_exact`), a początek przechodzi bez zmian
(`projectionOrigin_eq`). Okno rekordowe w liście SELECT (`MIN(e : N)`) to AGSE z
`F = h = 1` i `w = N`, co daje początek `O_S + N - 1` i ogon `W_S`.

```anchor projection_tail_exact -showProofStates (module := Profs.StartupBoundaries)
theorem projectionTail_exact (Ws W N0 : ℕ) (tick : ℚ) (htick : 0 < tick) :
    Ws ≤ W ↔ ∀ n, N0 ≤ n → affineReady 1 1 0 Ws W tick n := by
  have h : affineTail 1 1 0 Ws = Ws := by unfold affineTail; simp
  have := affineTail_exact 1 1 0 Ws W N0 one_pos one_pos tick htick
  rwa [h] at this
```

# Początki logiczne

Rekord `n` wyjścia ma definicję, gdy ma ją czytany rekord wejścia, czyli gdy
`O_S ≤ affineRead n`. Mapa jest niemalejąca, więc rekordy z definicją tworzą
ciągły sufiks, a początek to pierwszy indeks, który osiąga próg. To liczy
`firstIndexReaching` w `compiler::computeLogicalOrigin` dla `δ`, `Θ` i `~Θ`:

```anchor affine_origin -showProofStates (module := Profs.StartupBoundaries)
theorem affineOrigin_le_iff (p q c o n : ℕ) (hp : 0 < p) (hq : 0 < q) :
    affineOrigin p q c o hp hq ≤ n ↔ o ≤ affineRead p q c n := by
  constructor
  · intro hn
    exact le_trans (Nat.find_spec (affineRead_reaches p q c o hp hq)) (affineRead_mono p q c hn)
  · exact Nat.find_min' (affineRead_reaches p q c o hp hq)
```

Początek sumy to maksimum progów składowych:

```anchor sum_origin -showProofStates (module := Profs.StartupBoundaries)
theorem sumOrigin_le_iff (p1 q1 o1 p2 q2 o2 n : ℕ) (hp1 : 0 < p1) (hq1 : 0 < q1)
    (hp2 : 0 < p2) (hq2 : 0 < q2) :
    max (affineOrigin p1 q1 0 o1 hp1 hq1) (affineOrigin p2 q2 0 o2 hp2 hq2) ≤ n ↔
      o1 ≤ affineRead p1 q1 0 n ∧ o2 ≤ affineRead p2 q2 0 n := by
  rw [max_le_iff, affineOrigin_le_iff, affineOrigin_le_iff]
```

Początek okna AGSE to najmniejsze `n`, dla którego całe okno leży w istniejącej
części źródła, czyli od pozycji `O_S*F` (`AgseLogicalOrigin`, `w = |length|`).
`agseOrigin_eq_ceil` zapisuje go jako sufit, w postaci z silnika:

```anchor agse_origin -showProofStates (module := Profs.StartupBoundaries)
theorem agseOrigin_le_iff (F h w Os n : ℕ) (hh : 0 < h) :
    agseOrigin F h w Os ≤ n ↔ Os * F + (w - 1) ≤ n * h := by
  unfold agseOrigin
  rw [← Nat.lt_succ_iff, Nat.div_lt_iff_lt_mul hh]
  have e : n.succ * h = n * h + h := by rw [Nat.succ_eq_add_one]; ring
  rw [e]
  generalize n * h = K
  omega
```

# Związek z silnikiem

Testy `ProofOracle` w `test/UnitTest/test_proofOracle.cpp` porównują funkcje
silnika z tablicami liczonymi przez `affineRead`, `affineTail` i `agseOrigin`
(`math_proofs/gen-oracle.sh`). Sprawdzają też warunek dostępności poza zakresem
tablic. Początki i ogony sumy i projekcji testy odczytują z planów skompilowanych
z RQL. Mapa odczytu AGSE `floor(n*h/F)` jest liczona w `dataModel`, a nie w
`SOperations.hpp`, więc test porównuje z modelem tylko ogon i początek okna.
Dowód nie obejmuje bajtów, deskryptorów ani harmonogramu działającego silnika.
