import Profs.PlanInstances

namespace Profs

/- Zlozenie wartosci (Val) w planie.

   plan_tail_mono przenosi na reszte planu tylko ogon (Lat). Ten plik dowodzi, ze
   przepisanie, ktore zachowuje na swojej granicy interwal, poczatek i rekordy i nie wydluza
   ogona, zachowuje to samo w kazdym wezle dalej w planie, a wiec na wyjsciach publicznych
   (Def. 4.2: Val rowne, Lat nie dluzszy). R1 spelnia warunek granicy (r1_frontier).

   Model wartosci: E v n f to rekord n wezla v liczony z funkcji f, ktora podaje rekordy wejsc
   po parach (wezel, indeks). Rekord jest jednego, abstrakcyjnego typu alpha, wiec mapa
   wartosci pustych jest jego czescia. Rekordy przed poczatkiem sa nieobserwowalne (Def. 4.2
   obejmuje tylko r_n dla n >= O_U) i model ich nie ogranicza.

   Zalozenie o silniku, nie wynik dowodu: EvalLocal - rekord n zalezy tylko od rekordow
   z reads n. To formalny odpowiednik zdania Tw. 4.3, ze we fragmencie RQL bez VAR wartosci
   dalej w planie zaleza tylko od interwalow, indeksow i rekordow wejsc. VAR i DO SET niosa
   stan miedzy rekordami i to zalozenie lamia.

   Poza modelem: schemat sigma_U, luki G_U (puste dla wyjsc, ktorych dotyczy R1) i kontrakt
   materializacji M_U; sam przebieg R1 w compiler.cpp (dowod porownuje dwa plany); R2,
   instancje ad hoc, przeladowanie planu, wypieranie historii, spoznione sloty RT, rozrzut
   DEVICE. -/

/- Rekord n wezla v zalezy tylko od rekordow, ktore czyta. -/
def EvalLocal {α : Type*} (P : ℕ → PlanNode) (E : ℕ → ℕ → (ℕ × ℤ → α) → α) : Prop :=
  ∀ v n (f g : ℕ × ℤ → α), (∀ ij ∈ (P v).reads n, f ij = g ij) → E v n f = E v n g

/- Semantyka wartosci: zrodlo ma od swojego poczatku rekordy src, rekord wezla pochodnego
   od jego poczatku liczy E z rekordow wejsc. -/
def ValueSemantics {α : Type*} (P : ℕ → PlanNode) (E : ℕ → ℕ → (ℕ × ℤ → α) → α)
    (src X : ℕ → ℕ → α) : Prop :=
  ∀ v n, (P v).origin ≤ n →
    ((P v).source = true → X v n = src v n) ∧
    ((P v).source = false → X v n = E v n (fun ij => X ij.1 ij.2.toNat))

/- Wezel u ma w P' te sama czesc wartosciowa co w P (interwal, poczatek, rekordy od
   poczatku) i nie dluzszy ogon - rownanie (5) Def. 4.2 w jednym wezle. -/
def Matches {α : Type*} (P P' : ℕ → PlanNode) (X X' : ℕ → ℕ → α) (W W' : ℕ → ℕ) (u : ℕ) :
    Prop :=
  (P' u).interval = (P u).interval ∧ (P' u).origin = (P u).origin ∧
    (∀ n, (P u).origin ≤ n → X u n = X' u n) ∧ W' u ≤ W u

theorem IsLeastTail.unique {P : ℕ → PlanNode} {W : ℕ → ℕ} {v w1 w2 : ℕ}
    (h1 : IsLeastTail P W v w1) (h2 : IsLeastTail P W v w2) : w1 = w2 :=
  le_antisymm (h1.2 _ h2.1) (h2.2 _ h1.1)

/- ===== Podstawienie ===== -/

/- Dwa plany na wspolnym zbiorze indeksow. S to wezly niezmienione (ten sam wezel i ta sama
   funkcja wartosci), F to granica przepisania. Wezly przepisanego fragmentu nie naleza do S
   i nic sie o nich nie zaklada, wiec plany moga sie w nim dowolnie roznic. Gdy kazdy odczyt
   wezla z S trafia do S albo do F, a granica pasuje, pasuje kazdy wezel z S. -/
-- ANCHOR: plan_substitution
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
-- ANCHOR_END: plan_substitution

/- ===== R1 ===== -/

theorem interleaveAt_congr (a b n : ℕ) {α β : Type*} (A A' : ℕ → α) (B B' : ℕ → β)
    (hA : A (indexA a b n) = A' (indexA a b n)) (hB : B (indexB a b n) = B' (indexB a b n)) :
    interleaveAt a b A B n = interleaveAt a b A' B' n := by
  unfold indexB indexA at *
  simp only [interleaveAt, hA, hB]

/- Dluzsze ogony wejsc nie skracaja ogona przeplotu. -/
theorem exactInterleaveTail_mono (a b WA WB WA' WB' : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hA : WA ≤ WA') (hB : WB ≤ WB') :
    (exactInterleaveTail a b WA WB).toNat ≤ (exactInterleaveTail a b WA' WB').toNat := by
  have h := (exactInterleaveTail_exact a b WA' WB' _ 0 ha hb 1 one_pos).mp le_rfl
  apply (exactInterleaveTail_exact a b WA WB _ 0 ha hb 1 one_pos).mpr
  intro n hn
  have hn' := h n hn
  unfold interleaveReady at *
  split_ifs at * <;> refine le_trans ?_ hn' <;> gcongr

/- Ksztalt obu stron R1 w modelu: w P wezel r to (A>i)#(B>k) przez wezly sA i sB, w P' to
   (A#B)>(i+k) przez wezel c. Interwaly A i B to a*tick i b*tick, a i*a = k*b. Wartosci
   przesuniecia i przeplotu sa podane wprost: przesuniecie czyta rekord n-m, przeplot to
   interleaveAt (rownanie (1)) nad rekordami wejsc. -/
structure R1Shape {α : Type*} (P P' : ℕ → PlanNode) (E E' : ℕ → ℕ → (ℕ × ℤ → α) → α)
    (nA nB sA sB r c a b i k : ℕ) (tick : ℚ) : Prop where
  ha : 0 < a
  hb : 0 < b
  htick : 0 < tick
  hmatch : i * a = k * b
  ΔA : (P nA).interval = a * tick
  ΔB : (P nB).interval = b * tick
  -- lewa strona
  srcSA : (P sA).source = false
  srcSB : (P sB).source = false
  srcR : (P r).source = false
  ΔsA : (P sA).interval = (P nA).interval
  ΔsB : (P sB).interval = (P nB).interval
  OsA : (P sA).origin = (P nA).origin + i
  OsB : (P sB).origin = (P nB).origin + k
  RsA : ∀ n, (P sA).reads n = [(nA, (n : ℤ) - i)]
  RsB : ∀ n, (P sB).reads n = [(nB, (n : ℤ) - k)]
  EsA : ∀ n f, E sA n f = f (nA, (n : ℤ) - i)
  EsB : ∀ n f, E sB n f = f (nB, (n : ℤ) - k)
  Δr : (P r).interval = ((a : ℚ) * b / (a + b)) * tick
  Or : (P r).origin = interleaveOrigin a b (P sA).origin (P sB).origin ha hb
  Rr : ∀ n, (P r).reads n = interleaveReads sA sB a b n
  Er : ∀ n f, E r n f =
    Sum.elim id id (interleaveAt a b (fun j => f (sA, (j : ℤ))) (fun j => f (sB, (j : ℤ))) n)
  -- prawa strona
  srcC : (P' c).source = false
  srcR' : (P' r).source = false
  Δc : (P' c).interval = ((a : ℚ) * b / (a + b)) * tick
  Oc : (P' c).origin = interleaveOrigin a b (P' nA).origin (P' nB).origin ha hb
  Rc : ∀ n, (P' c).reads n = interleaveReads nA nB a b n
  Ec : ∀ n f, E' c n f =
    Sum.elim id id (interleaveAt a b (fun j => f (nA, (j : ℤ))) (fun j => f (nB, (j : ℤ))) n)
  Δr' : (P' r).interval = (P' c).interval
  Or' : (P' r).origin = (P' c).origin + (i + k)
  Rr' : ∀ n, (P' r).reads n = [(c, (n : ℤ) - ((i + k : ℕ) : ℤ))]
  Er' : ∀ n f, E' r n f = f (c, (n : ℤ) - ((i + k : ℕ) : ℤ))

/- R1 spelnia warunek granicy: gdy wejscia A i B pasuja, wezel r ma w obu planach ten sam
   interwal, poczatek i rekordy, a jego ogon po prawej stronie nie jest dluzszy. To
   causal_shift_matching w jezyku modelu planu. -/
-- ANCHOR: r1_frontier
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
-- ANCHOR_END: r1_frontier

/- Teza: R1 jest dopuszczalne (rownanie (5) Def. 4.2) na kazdym wezle dalej w planie. S to
   wezly niezmienione, ktore czytaja tylko siebie nawzajem i wezel r; wejscia A i B musza
   pasowac (np. sa tymi samymi zrodlami z tymi samymi danymi). Na kazdym wezle z S, w tym na
   kazdym wyjsciu publicznym, interwal, poczatek i rekordy sa rowne, a ogon nie dluzszy. -/
-- ANCHOR: r1_plan_admissible
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
-- ANCHOR_END: r1_plan_admissible

end Profs
