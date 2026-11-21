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

Plan to ciąg węzłów `PlanNode`. Każdy węzeł ma interwał, początek i mapę odczytu
`reads n`, czyli listę par (węzeł wejściowy, indeks rekordu), które czyta rekord `n`.
Węzeł czyta tylko węzły o mniejszym indeksie (`Topological`). Źródło ma ogon podany
z zewnątrz. Chwila emisji rekordu `n` to `emitTime`, a ogon `w` wystarcza węzłowi `v`
(`localOK`), gdy każdy odczyt każdego określonego rekordu trafia w rekord zapisany
najpóźniej w chwili emisji czytającego. Reguły ogona (`TailRules`) to równania, które
rozwiązuje `compiler::computeStartupLatency`: źródło ma ogon podany, a węzeł pochodny
najmniejszy wystarczający przy ogonach swoich wejść.

# Początki

Gdy każdy węzeł spełnia lokalną regułę początku (`OriginRules`, twierdzenia
`*Origin_le_iff`), każda semantyka określoności, w której rekord węzła pochodnego jest
określony dokładnie wtedy, gdy określone są wszystkie czytane rekordy, pokrywa się
z sufiksem od początku:

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
      constructor
      · intro h ij hij
        exact (ih ij.1 (htop v n ij hij) ij.2).mp (h ij hij)
      · intro h ij hij
        exact (ih ij.1 (htop v n ij hij) ij.2).mpr (h ij hij)
    · exact (hD v n).1 hs
```

# Ogony

Ogony z reguł są dopuszczalne: każdy odczyt określonego rekordu trafia w rekord
określony i już zapisany.

```anchor plan_rules_feasible -showProofStates (module := Profs.PlanComposition)
theorem plan_rules_feasible (P : ℕ → PlanNode) (Ws W : ℕ → ℕ)
    (hO : OriginRules P) (hW : TailRules P Ws W) :
    ∀ v, (P v).source = false → ∀ n, (P v).origin ≤ n → ∀ ij ∈ (P v).reads n,
      (P ij.1).origin ≤ ij.2 ∧ emitTime P W ij.1 ij.2 ≤ emitTime P W v n := by
  intro v hs n hn ij hij
  exact ⟨(hO v hs n).mp hn ij hij, ((hW v).2 hs).1 n hn ij hij⟩
```

Są też najmniejsze. Żadne przypisanie dopuszczalne nie ma krótszego ogona w żadnym
węźle, więc dokładność reguły lokalnej przenosi się na cały plan. Dowód idzie
indukcją po porządku topologicznym. Węzeł przy dłuższych ogonach wejść nie może mieć
krótszego ogona (`localOK_mono`), więc najmniejszy ogon przy ogonach z reguł nie
przekracza ogona żadnego przypisania dopuszczalnego. Stąd też jedyność rozwiązania
równań (`plan_rules_unique`).

```anchor plan_rules_least -showProofStates (module := Profs.PlanComposition)
theorem plan_rules_least (P : ℕ → PlanNode) (Ws W W' : ℕ → ℕ)
    (htop : Topological P) (hpos : ∀ i, 0 ≤ (P i).interval)
    (hW : TailRules P Ws W) (hF : Feasible P Ws W') :
    ∀ v, W v ≤ W' v := by
  intro v
  induction v using Nat.strong_induction_on with
  | _ v ih =>
    cases hs : (P v).source
    · exact ((hW v).2 hs).2 _ (localOK_mono P W W' v (W' v) htop hpos ih ((hF v).2 hs))
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
    (htop : Topological P) (hpos : ∀ i, 0 ≤ (P i).interval)
    (hle : ∀ v, Ws v ≤ Ws' v) (hW : TailRules P Ws W) (hW' : TailRules P Ws' W') :
    ∀ v, W v ≤ W' v := by
  apply plan_rules_least P Ws W W' htop hpos hW
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

# Zakres

Dowód nie obejmuje zgodności kodu C++ z modelem wykonania (zastępuje ją test),
wartości rekordów poza ich określonością, planów z `VAR` i regułami `DO SET`,
instancji ad hoc dołączanych do biegnącej osi, przeładowania planu, wypierania
historii, spóźnionych slotów w trybie czasu rzeczywistego ani rozrzutu czasu
`DEVICE`.
