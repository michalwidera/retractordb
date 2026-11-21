import Mathlib.Tactic

namespace Profs

/- Zlozenie granic rozruchowych w planie.

   Twierdzenia o pojedynczych operatorach (StartupBoundaries, CausalShift, InterleaveTailExact)
   przyjmuja ogon wejscia W_S jako parametr: rekord j wejscia jest okreslony w chwili
   (j+1+W_S)*Delta_S. Ten plik dowodzi, ze ten sam model obowiazuje wyjscie kazdego wezla i ze
   reguly lokalne daja dla calego planu dokladne poczatki i najmniejsze dopuszczalne ogony.

   Model wykonania silnika (dataModel::processRows, TimeLine w CRSMath.cpp):
     * wezel o interwale Delta liczy sie w chwilach k*Delta, k >= 1;
     * wezel planu startowego milczy przez O + W swoich slotow, potem zapisuje jeden rekord na
       slot, a jego fizyczny rekord 0 ma indeks logiczny O - rekord n powstaje wiec dokladnie
       w chwili (n+1+W)*Delta, nigdy wczesniej, bez wzgledu na gotowosc wejsc;
     * w slocie wezly licza sie w porzadku topologicznym (compiler::compile konczy sie
       topologicalSort), wiec rekord zapisany w chwili t konsument widzi w tym samym slocie.
   Te trzy fakty sa wlasnoscia kodu C++, a nie tego pliku: tutaj chwila emisji jest definicja
   (emitTime), a zgodnosc silnika z nia sprawdza test harmonogramu w test_proofOracle.cpp. -/

/- Wezel planu: interwal, poczatek logiczny i dwie mapy. reads n to lista par (wezel wejsciowy,
   indeks rekordu), ktore rekord n czyta; od nich zalezy ogon. deps n to rekordy, od ktorych
   zalezy okreslonosc rekordu n; zawiera reads n i bywa szersza - przeplot czyta jeden rekord,
   ale jego poczatek to wspolny prog obu wejsc (originReady w CausalShift.lean). Indeksy sa
   calkowite, bo przesuniecie i okno siegaja przed rekord 0, a taki rekord nie istnieje.
   Zrodlo (deklaracja) ma ogon podany z zewnatrz i jego map model nie ogranicza. -/
structure PlanNode where
  interval : ℚ
  origin : ℕ
  source : Bool
  deps : ℕ → List (ℕ × ℤ)
  reads : ℕ → List (ℕ × ℤ)

/- Porzadek topologiczny: wezel zalezy tylko od wezlow o mniejszym indeksie. -/
def Topological (P : ℕ → PlanNode) : Prop :=
  ∀ v n ij, ij ∈ (P v).deps n → ij.1 < v

def ReadsInDeps (P : ℕ → PlanNode) : Prop :=
  ∀ v n ij, ij ∈ (P v).reads n → ij ∈ (P v).deps n

/- Chwila, w ktorej wezel v o ogonie W v zapisuje rekord n. -/
def emitTime (P : ℕ → PlanNode) (W : ℕ → ℕ) (v n : ℕ) : ℚ :=
  ((n + 1 + W v : ℕ) : ℚ) * (P v).interval

/- Ogon w jest dla wezla v wystarczajacy przy ogonach wejsc W: kazdy odczyt kazdego
   okreslonego rekordu trafia w rekord zapisany najpozniej w chwili jego wlasnej emisji.
   Odczyt okreslonego rekordu ma indeks nieujemny (plan_rules_feasible), wiec toNat
   niczego tu nie obcina. -/
def localOK (P : ℕ → PlanNode) (W : ℕ → ℕ) (v w : ℕ) : Prop :=
  ∀ n, (P v).origin ≤ n → ∀ ij ∈ (P v).reads n,
    emitTime P W ij.1 ij.2.toNat ≤ ((n + 1 + w : ℕ) : ℚ) * (P v).interval

def IsLeastTail (P : ℕ → PlanNode) (W : ℕ → ℕ) (v w : ℕ) : Prop :=
  localOK P W v w ∧ ∀ w', localOK P W v w' → w ≤ w'

/- Reguly ogona: zrodlo ma ogon podany, wezel pochodny najmniejszy wystarczajacy przy ogonach
   swoich wejsc - te rownania rozwiazuje compiler::computeStartupLatency. -/
def TailRules (P : ℕ → PlanNode) (Ws W : ℕ → ℕ) : Prop :=
  ∀ v, ((P v).source = true → W v = Ws v) ∧ ((P v).source = false → IsLeastTail P W v (W v))

/- Przypisanie dopuszczalne: zrodla nie szybsze niz podano, kazdy wezel pochodny ma ogon
   wystarczajacy przy ogonach z tego samego przypisania. -/
def Feasible (P : ℕ → PlanNode) (Ws W : ℕ → ℕ) : Prop :=
  ∀ v, ((P v).source = true → Ws v ≤ W v) ∧ ((P v).source = false → localOK P W v (W v))

/- Regula poczatku wezla: rekord n ma definicje dokladnie wtedy, gdy kazdy rekord z deps n
   lezy w okreslonej czesci swojego wejscia (lokalne twierdzenia *Origin_le_iff). -/
def OriginRule (P : ℕ → PlanNode) (v : ℕ) : Prop :=
  ∀ n, (P v).origin ≤ n ↔ ∀ ij ∈ (P v).deps n, ((P ij.1).origin : ℤ) ≤ ij.2

def OriginRules (P : ℕ → PlanNode) : Prop :=
  ∀ v, (P v).source = false → OriginRule P v

/- Semantyka okreslonosci: zrodlo ma rekordy od swojego poczatku, rekord wezla pochodnego
   jest okreslony, gdy istnieja i sa okreslone wszystkie rekordy, od ktorych zalezy. -/
def DefinedSemantics (P : ℕ → PlanNode) (D : ℕ → ℕ → Prop) : Prop :=
  ∀ v n, ((P v).source = true → (D v n ↔ (P v).origin ≤ n)) ∧
    ((P v).source = false → (D v n ↔ ∀ ij ∈ (P v).deps n, 0 ≤ ij.2 ∧ D ij.1 ij.2.toNat))

/- ===== Poczatki ===== -/

-- ANCHOR: plan_defined_iff_origin
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
-- ANCHOR_END: plan_defined_iff_origin

/- ===== Ogony ===== -/

/- Kazdy odczyt okreslonego rekordu trafia w rekord okreslony i juz zapisany. -/
-- ANCHOR: plan_rules_feasible
theorem plan_rules_feasible (P : ℕ → PlanNode) (Ws W : ℕ → ℕ)
    (hsub : ReadsInDeps P) (hO : OriginRules P) (hW : TailRules P Ws W) :
    ∀ v, (P v).source = false → ∀ n, (P v).origin ≤ n → ∀ ij ∈ (P v).reads n,
      ((P ij.1).origin : ℤ) ≤ ij.2 ∧ emitTime P W ij.1 ij.2.toNat ≤ emitTime P W v n := by
  intro v hs n hn ij hij
  exact ⟨(hO v hs n).mp hn ij (hsub v n ij hij), ((hW v).2 hs).1 n hn ij hij⟩
-- ANCHOR_END: plan_rules_feasible

/- Dluzsze ogony wejsc nie ulatwiaja odczytu. -/
theorem localOK_mono (P : ℕ → PlanNode) (W W' : ℕ → ℕ) (v w : ℕ)
    (htop : Topological P) (hsub : ReadsInDeps P) (hpos : ∀ i, 0 ≤ (P i).interval)
    (hle : ∀ i, i < v → W i ≤ W' i) (h : localOK P W' v w) : localOK P W v w := by
  intro n hn ij hij
  refine le_trans ?_ (h n hn ij hij)
  unfold emitTime
  apply mul_le_mul_of_nonneg_right _ (hpos ij.1)
  exact_mod_cast (by have := hle ij.1 (htop v n ij (hsub v n ij hij)); omega :
    ij.2.toNat + 1 + W ij.1 ≤ ij.2.toNat + 1 + W' ij.1)

theorem TailRules.feasible {P : ℕ → PlanNode} {Ws W : ℕ → ℕ} (hW : TailRules P Ws W) :
    Feasible P Ws W := by
  intro v
  exact ⟨fun hs => ((hW v).1 hs).ge, fun hs => ((hW v).2 hs).1⟩

/- Ogony z regul sa najmniejsze: zadne przypisanie dopuszczalne nie ma krotszego ogona
   w zadnym wezle. Dokladnosc reguly lokalnej przenosi sie wiec na caly plan. -/
-- ANCHOR: plan_rules_least
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
-- ANCHOR_END: plan_rules_least

theorem plan_rules_unique (P : ℕ → PlanNode) (Ws W1 W2 : ℕ → ℕ)
    (htop : Topological P) (hsub : ReadsInDeps P) (hpos : ∀ i, 0 ≤ (P i).interval)
    (h1 : TailRules P Ws W1) (h2 : TailRules P Ws W2) : W1 = W2 := by
  funext v
  exact le_antisymm (plan_rules_least P Ws W1 W2 htop hsub hpos h1 h2.feasible v)
    (plan_rules_least P Ws W2 W1 htop hsub hpos h2 h1.feasible v)

/- Krotszy ogon zrodla nie wydluza zadnego ogona dalej w planie. Wezel, ktory przepisanie
   (np. R1) zastepuje innym o tym samym poczatku i tych samych rekordach, jest dla reszty planu
   zrodlem: dalsze wezly widza tylko jego interwal, poczatek i ogon. -/
-- ANCHOR: plan_tail_mono
theorem plan_tail_mono (P : ℕ → PlanNode) (Ws Ws' W W' : ℕ → ℕ)
    (htop : Topological P) (hsub : ReadsInDeps P) (hpos : ∀ i, 0 ≤ (P i).interval)
    (hle : ∀ v, Ws v ≤ Ws' v) (hW : TailRules P Ws W) (hW' : TailRules P Ws' W') :
    ∀ v, W v ≤ W' v := by
  apply plan_rules_least P Ws W W' htop hsub hpos hW
  intro v
  exact ⟨fun hs => le_trans (hle v) ((hW' v).1 hs).ge, fun hs => ((hW' v).2 hs).1⟩
-- ANCHOR_END: plan_tail_mono

/- Pomost do Tabeli 1: twierdzenie dokladnosci operatora w postaci r <= W <-> warunek lokalny
   daje rownanie reguly W_v = r. -/
-- ANCHOR: least_tail_of_exact
theorem leastTail_of_exact (P : ℕ → PlanNode) (W : ℕ → ℕ) (v r : ℕ)
    (h : ∀ w, r ≤ w ↔ localOK P W v w) : IsLeastTail P W v r :=
  ⟨(h r).mp le_rfl, fun w' hw' => (h w').mpr hw'⟩
-- ANCHOR_END: least_tail_of_exact

end Profs
