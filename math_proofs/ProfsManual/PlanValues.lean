import VersoManual

open Verso.Genre Manual
open Verso.Code.External

set_option verso.exampleProject "."

-- Limit heartbeatow liczy sie dla calego polecenia #doc, a podswietlenie bloku rosnie z dlugoscia
-- dowodu i typow w podpowiedziach. Dowody tego rozdzialu sa dluzsze od pozostalych blokow i nie
-- mieszcza sie w domyslnych 200000.
set_option maxHeartbeats 4000000

#doc (Manual) "Złożenie wartości w planie" =>
%%%
file := "plan-values"
%%%

Twierdzenie `plan_tail_mono` przenosi na resztę planu tylko ogon (Lat). Plik
`Profs/PlanValues.lean` dowodzi, że przepisanie, które na swojej granicy zachowuje
interwał, początek i rekordy i nie wydłuża ogona, zachowuje to samo w każdym węźle
dalej w planie. Dotyczy to też wyjść publicznych, więc spełnione jest równanie (5)
Def. 4.2: Val równe, Lat nie dłuższy.

# Model wartości

Model wartości to osobny parametr `E`: `E v n f` jest rekordem `n` węzła `v` liczonym
z funkcji `f`, która podaje rekordy wejść po parach (węzeł, indeks). Rekord ma jeden,
abstrakcyjny typ, więc mapa wartości pustych jest jego częścią. Semantyka wartości
(`ValueSemantics`) daje źródłu jego rekordy od początku, a węzłowi pochodnemu rekordy
liczone przez `E` z rekordów wejść. Rekordy przed początkiem są nieobserwowalne i model
ich nie ogranicza.

Jedno założenie dotyczy silnika i nie jest wynikiem dowodu: `EvalLocal` mówi, że rekord
`n` zależy tylko od rekordów z `reads n`. To formalny odpowiednik zdania Tw. 4.3, że we
fragmencie RQL bez `VAR` wartości dalej w planie zależą tylko od interwałów, indeksów
i rekordów wejść.

# Podstawienie

Twierdzenie o podstawieniu porównuje dwa plany na wspólnym zbiorze indeksów. `S` to
węzły niezmienione: ten sam węzeł i ta sama funkcja wartości. `F` to granica
przepisania. O węzłach przepisanego fragmentu nic się nie zakłada, więc plany mogą się
w nim dowolnie różnić. Gdy każdy odczyt węzła z `S` trafia do `S` albo do `F`, a każdy
węzeł granicy pasuje (`Matches`: ten sam interwał, początek i rekordy od początku, ogon
nie dłuższy), pasuje każdy węzeł z `S`:

```anchor plan_substitution -showProofStates (module := Profs.PlanValues)
theorem plan_substitution {α : Type*} (P P' : ℕ → PlanNode)
    (E E' : ℕ → ℕ → (ℕ × ℤ → α) → α) (src src' X X' : ℕ → ℕ → α) (Ws Ws' W W' : ℕ → ℕ)
    (S F : ℕ → Prop)
    (htop : Topological P) (hsub : ReadsInDeps P) (hpos : ∀ i, 0 ≤ (P i).interval)
    (hO : OriginRules P) (hW : TailRules P Ws W) (hW' : TailRules P' Ws' W')
    (hX : ValueSemantics P E src X) (hX' : ValueSemantics P' E' src' X')
    (hloc : EvalLocal P E)
    (hsame : ∀ v, S v → P' v = P v ∧ E' v = E v)
    (hsrc : ∀ v, S v → (P v).source = true →
      Ws' v ≤ Ws v ∧ ∀ n, (P v).origin ≤ n → src v n = src' v n)
    (hF : ∀ u, F u → Matches P P' X X' W W' u)
    (hclos : ∀ v, S v → ∀ n ij, ij ∈ (P v).reads n → S ij.1 ∨ F ij.1) :
    ∀ v, S v → Matches P P' X X' W W' v := by
  intro v
  induction v using Nat.strong_induction_on with
  | _ v ih =>
    intro hv
    obtain ⟨hPv, hEv⟩ := hsame v hv
    have hin : ∀ n ij, ij ∈ (P v).reads n → Matches P P' X X' W W' ij.1 := by
      intro n ij hij
      rcases hclos v hv n ij hij with h | h
      · exact ih ij.1 (htop v n ij (hsub v n ij hij)) h
      · exact hF ij.1 h
    unfold Matches
    refine ⟨by rw [hPv], by rw [hPv], ?_, ?_⟩
    · intro n hn
      have hn' : (P' v).origin ≤ n := by rw [hPv]; exact hn
      cases hs : (P v).source
      · have hs' : (P' v).source = false := by rw [hPv]; exact hs
        rw [(hX v n hn).2 hs, (hX' v n hn').2 hs', hEv]
        apply hloc
        intro ij hij
        obtain ⟨-, -, hval, -⟩ := hin n ij hij
        have hle := (hO v hs n).mp hn ij (hsub v n ij hij)
        exact hval _ (by omega)
      · have hs' : (P' v).source = true := by rw [hPv]; exact hs
        rw [(hX v n hn).1 hs, (hX' v n hn').1 hs']
        exact (hsrc v hv hs).2 n hn
    · cases hs : (P v).source
      · have hs' : (P' v).source = false := by rw [hPv]; exact hs
        apply ((hW' v).2 hs').2
        intro n hn ij hij
        rw [hPv] at hn hij
        rw [hPv]
        obtain ⟨hint, -, -, htail⟩ := hin n ij hij
        refine le_trans ?_ (((hW v).2 hs).1 n hn ij hij)
        unfold emitTime
        rw [hint]
        apply mul_le_mul_of_nonneg_right _ (hpos ij.1)
        exact_mod_cast (by omega : ij.2.toNat + 1 + W' ij.1 ≤ ij.2.toNat + 1 + W ij.1)
      · have hs' : (P' v).source = true := by rw [hPv]; exact hs
        rw [(hW v).1 hs, (hW' v).1 hs']
        exact (hsrc v hv hs).1
```

# R1

R1 spełnia warunek granicy. `R1Shape` zapisuje obie strony w modelu: w `P` węzeł `r`
to `(A>i)#(B>k)` przez węzły `sA` i `sB`, w `P'` to `(A#B)>(i+k)` przez węzeł `c`.
Wartość przeplotu to `interleaveAt` (równanie (1)) nad rekordami wejść, a wartość
przesunięcia to rekord `n-m`. Początek daje `interleaveOrigin_shift`, rekordy
`delayed_interleave_values`, a ogon `causal_shift_matching_tail` z jednoznacznością
najmniejszego ogona i monotonicznością ogona przeplotu (`exactInterleaveTail_mono`):

```anchor r1_frontier (module := Profs.PlanValues)
theorem r1_frontier {α : Type*} (P P' : ℕ → PlanNode) (E E' : ℕ → ℕ → (ℕ × ℤ → α) → α)
    (src src' X X' : ℕ → ℕ → α) (Ws Ws' W W' : ℕ → ℕ) (nA nB sA sB r c a b i k : ℕ)
    (tick : ℚ) (h : R1Shape P P' E E' nA nB sA sB r c a b i k tick)
    (hX : ValueSemantics P E src X) (hX' : ValueSemantics P' E' src' X')
    (hW : TailRules P Ws W) (hW' : TailRules P' Ws' W')
    (hA : Matches P P' X X' W W' nA) (hB : Matches P P' X X' W W' nB) :
    Matches P P' X X' W W' r := by
  obtain ⟨hΔA', hOA', hvA, htA⟩ := hA
  obtain ⟨hΔB', hOB', hvB, htB⟩ := hB
  have hposA : 0 < (P nA).interval := by rw [h.ΔA]; exact mul_pos (by exact_mod_cast h.ha) h.htick
  have hposB : 0 < (P nB).interval := by rw [h.ΔB]; exact mul_pos (by exact_mod_cast h.hb) h.htick
  -- wspolny poczatek: O_r = O_c + (i + k) po obu stronach
  have hOr : (P r).origin =
      interleaveOrigin a b (P nA).origin (P nB).origin h.ha h.hb + (i + k) := by
    rw [h.Or, h.OsA, h.OsB]
    exact interleaveOrigin_shift a b i k _ _ h.ha h.hb h.hmatch
  have hOc : (P' c).origin = interleaveOrigin a b (P nA).origin (P nB).origin h.ha h.hb := by
    rw [h.Oc, hOA', hOB']
  unfold Matches
  refine ⟨by rw [h.Δr', h.Δc, h.Δr], by rw [h.Or', hOc, hOr], ?_, ?_⟩
  · intro n hn
    rw [hOr] at hn
    have hc : (P' c).origin ≤ n - (i + k) := by rw [hOc]; omega
    have hr' : (P' r).origin ≤ n := by rw [h.Or', hOc]; omega
    -- lewa strona: przeplot przesunietych wejsc
    rw [(hX r n (by rw [hOr]; exact hn)).2 h.srcR, h.Er]
    simp only [Int.toNat_natCast]
    have hready := (interleaveOrigin_le_iff a b _ _ n h.ha h.hb).mp
      (show interleaveOrigin a b (P sA).origin (P sB).origin h.ha h.hb ≤ n by
        rw [← h.Or, hOr]; exact hn)
    rw [h.OsA, h.OsB] at hready
    rw [interleaveAt_congr a b n _ (fun j => X nA (j - i)) _ (fun j => X nB (j - k))
      (by
        rw [(hX sA _ (by rw [h.OsA]; exact hready.1)).2 h.srcSA, h.EsA]
        congr 1
        omega)
      (by
        rw [(hX sB _ (by rw [h.OsB]; exact hready.2)).2 h.srcSB, h.EsB]
        congr 1
        omega)]
    rw [delayed_interleave_values a b i k n h.ha h.hb h.hmatch (by omega)]
    -- prawa strona: przesuniety przeplot
    rw [(hX' r n hr').2 h.srcR', h.Er']
    have hidx : ((n : ℤ) - ((i + k : ℕ) : ℤ)).toNat = n - (i + k) := by omega
    rw [hidx, (hX' c _ hc).2 h.srcC, h.Ec]
    simp only [Int.toNat_natCast]
    have hready' := (interleaveOrigin_le_iff a b _ _ _ h.ha h.hb).mp
      (show interleaveOrigin a b (P' nA).origin (P' nB).origin h.ha h.hb ≤ n - (i + k) by
        rw [← h.Oc]; exact hc)
    rw [hOA', hOB'] at hready'
    rw [interleaveAt_congr a b _ (X nA) (X' nA) (X nB) (X' nB)
      (hvA _ hready'.1) (hvB _ hready'.2)]
  · -- ogony z regul: W_r = T(W_A - i, W_B - k), W'_r = T(W'_A, W'_B) - (i + k)
    have hsA := (shift_leastTail P W sA nA i hposA h.ΔsA h.OsA h.RsA).unique
      ((hW sA).2 h.srcSA)
    have hsB := (shift_leastTail P W sB nB k hposB h.ΔsB h.OsB h.RsB).unique
      ((hW sB).2 h.srcSB)
    have hr := (interleave_leastTail P W r sA sB a b tick h.ha h.hb h.htick h.Δr
      (by rw [h.ΔsA, h.ΔA]) (by rw [h.ΔsB, h.ΔB]) h.Rr).unique ((hW r).2 h.srcR)
    have hc := (interleave_leastTail P' W' c nA nB a b tick h.ha h.hb h.htick h.Δc
      (by rw [hΔA', h.ΔA]) (by rw [hΔB', h.ΔB]) h.Rc).unique ((hW' c).2 h.srcC)
    have hposC : 0 < (P' c).interval := by
      rw [h.Δc]
      have : (0 : ℚ) < a := by exact_mod_cast h.ha
      have : (0 : ℚ) < b := by exact_mod_cast h.hb
      exact mul_pos (by positivity) h.htick
    have hr' := (shift_leastTail P' W' r c (i + k) hposC h.Δr' h.Or' h.Rr').unique
      ((hW' r).2 h.srcR')
    rw [← hr, ← hr', ← hc, ← hsA, ← hsB]
    have hmono := exactInterleaveTail_mono a b _ _ _ _ h.ha h.hb htA htB
    have hshift := causal_shift_matching_tail a b i k (W nA) (W nB) h.ha h.hb h.hmatch
    omega
```

Złożenie obu daje tezę: R1 jest dopuszczalne na każdym węźle z `S`, w tym na każdym
wyjściu publicznym, gdy wejścia `A` i `B` pasują, na przykład są tymi samymi źródłami
z tymi samymi danymi:

```anchor r1_plan_admissible -showProofStates (module := Profs.PlanValues)
theorem r1_plan_admissible {α : Type*} (P P' : ℕ → PlanNode)
    (E E' : ℕ → ℕ → (ℕ × ℤ → α) → α) (src src' X X' : ℕ → ℕ → α) (Ws Ws' W W' : ℕ → ℕ)
    (nA nB sA sB r c a b i k : ℕ) (tick : ℚ) (S : ℕ → Prop)
    (h : R1Shape P P' E E' nA nB sA sB r c a b i k tick)
    (htop : Topological P) (hsub : ReadsInDeps P) (hpos : ∀ i, 0 ≤ (P i).interval)
    (hO : OriginRules P) (hW : TailRules P Ws W) (hW' : TailRules P' Ws' W')
    (hX : ValueSemantics P E src X) (hX' : ValueSemantics P' E' src' X')
    (hloc : EvalLocal P E)
    (hA : Matches P P' X X' W W' nA) (hB : Matches P P' X X' W W' nB)
    (hsame : ∀ v, S v → P' v = P v ∧ E' v = E v)
    (hsrc : ∀ v, S v → (P v).source = true →
      Ws' v ≤ Ws v ∧ ∀ n, (P v).origin ≤ n → src v n = src' v n)
    (hclos : ∀ v, S v → ∀ n ij, ij ∈ (P v).reads n → S ij.1 ∨ ij.1 = r) :
    ∀ v, S v → Matches P P' X X' W W' v :=
  plan_substitution P P' E E' src src' X X' Ws Ws' W W' S (· = r) htop hsub hpos hO hW hW'
    hX hX' hloc hsame hsrc
    (fun _ hu => hu ▸ r1_frontier P P' E E' src src' X X' Ws Ws' W W' nA nB sA sB r c a b i k
      tick h hX hX' hW hW' hA hB)
    hclos
```

# Związek z silnikiem

Test `ProofOracle.r1_plan_admissible` w `test/UnitTest/test_proofOracle.cpp` wykonuje
slot po slocie plan z `plan_tail_mono`: za stronami R1 `(A>2)#(B>1)` i `(A#B)>3` stoi
ten sam łańcuch operatorów Tabeli 1. Każda para węzłów obu łańcuchów ma te same rekordy
pod tymi samymi indeksami logicznymi od początku, a prawa wydaje każdy z nich nie
później. Węzeł granicy dla trzech par tempa sprawdza `causal_shift_matching`.

# Zakres

Dowód nie obejmuje założenia `EvalLocal` o funkcjach wartości silnika (świadectwem dla
operatorów łańcucha jest test, nie dowód), schematu, luk i kontraktu materializacji
z Def. 4.2, samego przebiegu R1 w `compiler.cpp` (dowód porównuje dwa plany),
przepisania R2, planów z `VAR` i regułami `DO SET` (stan między rekordami łamie
`EvalLocal`), instancji ad hoc dołączanych do biegnącej osi, przeładowania planu,
wypierania historii, spóźnionych slotów w trybie czasu rzeczywistego ani rozrzutu
czasu `DEVICE`.
