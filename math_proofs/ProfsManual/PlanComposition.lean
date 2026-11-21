import VersoManual

open Verso.Genre Manual
open Verso.Code.External

set_option verso.exampleProject "."

#doc (Manual) "Złożenie granic rozruchowych w planie" =>
%%%
file := "plan-composition"
%%%

Twierdzenia o pojedynczych operatorach (rozdział o granicach rozruchowych Tabeli 1
i rozdział o R1) przyjmują ogon wejścia `W_S` jako parametr: rekord `j` wejścia jest
określony w chwili `(j+1+W_S)*Δ_S`. Ten rozdział dowodzi, że ten sam model obowiązuje
wyjście każdego węzła, więc reguły lokalne dają dokładne początki i najmniejsze
dopuszczalne ogony dla całego planu (`Profs/PlanComposition.lean`).

# Model wykonania

Model opiera się na trzech własnościach silnika, które nie są tu dowodzone. Ich
zgodność z kodem sprawdza test harmonogramu w `test/UnitTest/test_proofOracle.cpp`:

- węzeł o interwale `Δ` liczy się w chwilach `k*Δ`, `k ≥ 1` (`TimeLine` w `CRSMath.cpp`);
- węzeł planu startowego milczy przez `O + W` swoich slotów, a potem zapisuje jeden
  rekord na slot. Jego fizyczny rekord 0 ma indeks logiczny `O`, więc rekord `n`
  powstaje dokładnie w chwili `(n+1+W)*Δ`, nigdy wcześniej i bez względu na gotowość
  wejść (`dataModel::processRows`);
- w slocie węzły liczą się w porządku topologicznym (`compiler::compile` kończy się
  `topologicalSort`), więc rekord zapisany w chwili `t` konsument widzi w tym samym slocie.

Plan to ciąg węzłów `PlanNode`. Każdy węzeł ma interwał, początek i dwie mapy.
`reads n` to lista par (węzeł wejściowy, indeks rekordu), które rekord `n` czyta, i od
niej zależy ogon. `deps n` to rekordy, od których zależy określoność rekordu `n`. Zawiera
`reads n` (`ReadsInDeps`) i bywa szersza: przeplot czyta jeden rekord, ale jego początek
to wspólny próg obu wejść (`originReady`). Indeksy są całkowite, bo przesunięcie i okno
sięgają przed rekord 0, a taki rekord nie istnieje. Węzeł zależy tylko od węzłów
o mniejszym indeksie (`Topological`). Źródło ma ogon podany z zewnątrz.

Chwila emisji rekordu `n` to `emitTime`. Ogon `w` wystarcza węzłowi `v` (`localOK`),
gdy każdy odczyt każdego określonego rekordu trafia w rekord zapisany najpóźniej
w chwili emisji czytającego. Reguły ogona (`TailRules`) to równania, które rozwiązuje
`compiler::computeStartupLatency`: źródło ma ogon podany, a węzeł pochodny najmniejszy
wystarczający przy ogonach swoich wejść.

# Początki

Gdy każdy węzeł spełnia lokalną regułę początku (`OriginRule`, twierdzenia
`*Origin_le_iff`), każda semantyka określoności, w której rekord węzła pochodnego jest
określony dokładnie wtedy, gdy istnieją i są określone wszystkie rekordy z `deps`,
pokrywa się z sufiksem od początku:

```anchor plan_defined_iff_origin -showProofStates (module := Profs.PlanComposition)
theorem plan_defined_iff_origin (P : ℕ → PlanNode) (D : ℕ → ℕ → Prop)
    (htop : Topological P) (hO : OriginRules P) (hD : DefinedSemantics P D) :
    ∀ v n, D v n ↔ (P v).origin ≤ n := by
  intro v
  induction v using Nat.strong_induction_on with
  | _ v ih =>
    intro n
    cases hs : (P v).source
    · rw [(hD v n).2 hs, hO v hs n]
      apply forall₂_congr
      intro ij hij
      rw [ih ij.1 (htop v n ij hij) ij.2.toNat]
      omega
    · exact (hD v n).1 hs
```

# Ogony

Ogony z reguł są dopuszczalne: każdy odczyt określonego rekordu trafia w rekord
określony i już zapisany.

```anchor plan_rules_feasible -showProofStates (module := Profs.PlanComposition)
theorem plan_rules_feasible (P : ℕ → PlanNode) (Ws W : ℕ → ℕ)
    (hsub : ReadsInDeps P) (hO : OriginRules P) (hW : TailRules P Ws W) :
    ∀ v, (P v).source = false → ∀ n, (P v).origin ≤ n → ∀ ij ∈ (P v).reads n,
      ((P ij.1).origin : ℤ) ≤ ij.2 ∧ emitTime P W ij.1 ij.2.toNat ≤ emitTime P W v n := by
  intro v hs n hn ij hij
  exact ⟨(hO v hs n).mp hn ij (hsub v n ij hij), ((hW v).2 hs).1 n hn ij hij⟩
```

Są też najmniejsze. Żadne przypisanie dopuszczalne nie ma krótszego ogona w żadnym
węźle, więc dokładność reguły lokalnej przenosi się na cały plan. Dowód idzie
indukcją po porządku topologicznym. Węzeł przy dłuższych ogonach wejść nie może mieć
krótszego ogona (`localOK_mono`), więc najmniejszy ogon przy ogonach z reguł nie
przekracza ogona żadnego przypisania dopuszczalnego. Stąd też jedyność rozwiązania
równań (`plan_rules_unique`).

```anchor plan_rules_least -showProofStates (module := Profs.PlanComposition)
theorem plan_rules_least (P : ℕ → PlanNode) (Ws W W' : ℕ → ℕ)
    (htop : Topological P) (hsub : ReadsInDeps P) (hpos : ∀ i, 0 ≤ (P i).interval)
    (hW : TailRules P Ws W) (hF : Feasible P Ws W') :
    ∀ v, W v ≤ W' v := by
  intro v
  induction v using Nat.strong_induction_on with
  | _ v ih =>
    cases hs : (P v).source
    · exact ((hW v).2 hs).2 _ (localOK_mono P W W' v (W' v) htop hsub hpos ih ((hF v).2 hs))
    · rw [(hW v).1 hs]
      exact (hF v).1 hs
```

Krótszy ogon źródła nie wydłuża żadnego ogona dalej w planie. Węzeł, który
przepisanie (na przykład R1) zastępuje innym o tym samym interwale, początku
i rekordach, jest dla reszty planu źródłem, bo dalsze węzły widzą tylko jego
interwał, początek i ogon. Przepisanie, które nie wydłuża ogona w swoim węźle
(`causal_shift_matching`), nie wydłuża więc ogona żadnego wyjścia publicznego.

```anchor plan_tail_mono -showProofStates (module := Profs.PlanComposition)
theorem plan_tail_mono (P : ℕ → PlanNode) (Ws Ws' W W' : ℕ → ℕ)
    (htop : Topological P) (hsub : ReadsInDeps P) (hpos : ∀ i, 0 ≤ (P i).interval)
    (hle : ∀ v, Ws v ≤ Ws' v) (hW : TailRules P Ws W) (hW' : TailRules P Ws' W') :
    ∀ v, W v ≤ W' v := by
  apply plan_rules_least P Ws W W' htop hsub hpos hW
  intro v
  exact ⟨fun hs => le_trans (hle v) ((hW' v).1 hs).ge, fun hs => ((hW' v).2 hs).1⟩
```

Twierdzenie dokładności operatora ma postać `r ≤ W ↔` warunek gotowości od dowolnego
progu. Z progiem równym początkowi węzła daje ono równanie reguły `W_v = r`:

```anchor least_tail_of_exact -showProofStates (module := Profs.PlanComposition)
theorem leastTail_of_exact (P : ℕ → PlanNode) (W : ℕ → ℕ) (v r : ℕ)
    (h : ∀ w, r ≤ w ↔ localOK P W v w) : IsLeastTail P W v r :=
  ⟨(h r).mp le_rfl, fun w' hw' => (h w').mpr hw'⟩
```

# Operatory Tabeli 1

Plik `Profs/PlanInstances.lean` zapisuje każdy wiersz Tabeli 1 jako węzeł modelu. Dla
każdego operatora podaje mapę odczytu i dowodzi, że warunek `localOK` jest warunkiem
gotowości z twierdzenia tego operatora. Z twierdzenia dokładności wynika stąd równanie
reguły ogona z tą samą postacią reguły, a z twierdzenia o początku reguła `OriginRule`.
Postaci reguł z silnikiem wiążą testy `ProofOracle` tamtych twierdzeń.

Ogony: projekcja i redukcje krotek (`projection_leastTail`), suma (`sum_leastTail`),
różnica `δ` (`subtract_leastTail`), rozploty `Θ` i `~Θ` (`theta_leastTail`,
`ntheta_leastTail`), okno AGSE (`agse_leastTail`), okno rekordowe w liście SELECT
(`recordWindow_leastTail`), przesunięcie i przeplot. Okno czyta rekordy `(n*h-k)/F`
dla `k < w` (`windowReads`), a o ogonie decyduje najnowszy z nich, bo starsze są
zapisane wcześniej. Przesunięcie czyta rekord `n-m` i ma ogon `max(0, W_S - m)`:

```anchor shift_least_tail -showProofStates (module := Profs.PlanInstances)
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
```

Przeplot czyta jeden rekord, z `A` albo z `B` według `selectsA`, więc jego warunek
lokalny to `interleaveReady`, a ogon to `exactInterleaveTail`:

```anchor interleave_least_tail -showProofStates (module := Profs.PlanInstances)
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
```

Początki: operatory afiniczne (`affine_originRule`, w tym `δ`, `Θ` i `~Θ`), projekcja
(`projection_originRule`), suma (`sum_originRule`), okna AGSE i rekordowe
(`window_originRule`), przesunięcie (`shift_originRule`) i przeplot. Okno jest określone,
gdy jego najstarsza pozycja leży w określonej części źródła:

```anchor window_origin_rule -showProofStates (module := Profs.PlanInstances)
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
```

Przeplot zależy od obu wejść (`interleaveDeps`), choć czyta jedno
(`interleave_readsInDeps`):

```anchor interleave_origin_rule -showProofStates (module := Profs.PlanInstances)
theorem interleave_originRule (P : ℕ → PlanNode) (v iA iB a b : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hO : (P v).origin = interleaveOrigin a b (P iA).origin (P iB).origin ha hb)
    (hd : ∀ n, (P v).deps n = interleaveDeps iA iB a b n) : OriginRule P v := by
  intro n
  rw [hO, interleaveOrigin_le_iff, hd n]
  simp [interleaveDeps, originReady]
```

# Zakres

Dowód nie obejmuje zgodności kodu C++ z modelem wykonania (zastępuje ją test),
wartości rekordów poza ich określonością, planów z `VAR` i regułami `DO SET`,
instancji ad hoc dołączanych do biegnącej osi, przeładowania planu, wypierania
historii, spóźnionych slotów w trybie czasu rzeczywistego ani rozrzutu czasu
`DEVICE`.
