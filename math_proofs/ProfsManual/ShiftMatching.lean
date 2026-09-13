import VersoManual

open Verso.Genre Manual
open Verso.Code.External

set_option verso.exampleProject "."

#doc (Manual) "Twierdzenie R1: przesunięcie dopasowane do tempa" =>
%%%
file := "shift-matching"
%%%

Publikacje (twierdzenie `Interleave shift-matching`, w wersji DEBS
`Interleave shift-matching rewrite`) i polska dokumentacja (Twierdzenie R1)
zaczynają od uwagi, że przeplot nie jest przemienny. Dla `n = 0` zawsze
zachodzi gałąź równości, więc `φ(A,B)` zaczyna się od `b₀`, a `φ(B,A)` od
`a₀`:

```anchor not_commutative -showProofStates (module := Profs.ShiftMatching)
theorem interleave_starts_with_right (a b : ℕ) (ha : 0 < a) (hb : 0 < b) {α β : Type*}
    (A : ℕ → α) (B : ℕ → β) :
    interleaveAt a b A B 0 = .inr (B 0) ∧ interleaveAt b a B A 0 = .inr (A 0) := by
  have h1 : b / (a + b) = 0 := Nat.div_eq_of_lt (by omega)
  have h2 : a / (b + a) = 0 := Nat.div_eq_of_lt (by omega)
  simp [interleaveAt, h1, h2]
```

Reguła R1 ma warunek `i*a = k*b`: oba wejścia muszą być przesunięte
o ten sam czas. Wówczas pozycja wyniku przesuwa się o `L = i+k`.
`progress_shift` pokazuje, że licznik wyborów pierwszej składowej
zwiększa się o `i`, a liczbę wyborów drugiej składowej zwiększa `k`.
Dlatego wybór gałęzi i treść krotki pozostają takie same. Jest to
pomocnicza tożsamość dla indeksów `j+i` i `j+k`; poniżej wiążemy ją
z przesunięciem przyczynowym, które czyta starszy indeks `n-m`.

```anchor shift_values -showProofStates (module := Profs.ShiftMatching)
theorem shift_matching_values (a b i k : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) {α β : Type*} (A : ℕ → α) (B : ℕ → β)
    (n : ℕ) :
    interleaveAt a b (fun j => A (j + i)) (fun j => B (j + k)) n =
      interleaveAt a b A B (n + i + k) := by
  have hp := progress_shift a b i k n ha hb hmatch
  have hp1 := progress_shift a b i k (n + 1) ha hb hmatch
  have hpn := progress_le_slot a b n ha hb
  have hshiftNext : n + i + k + 1 = (n + 1) + i + k := by omega
  simp only [interleaveAt]
  change (if progress a b n = progress a b (n + 1) then
      Sum.inr (B (n - progress a b n + k))
    else Sum.inl (A (progress a b n + i))) =
    (if progress a b (n + i + k) = progress a b (n + i + k + 1) then
      Sum.inr (B (n + i + k - progress a b (n + i + k)))
    else Sum.inl (A (progress a b (n + i + k))))
  rw [hshiftNext, hp, hp1]
  have hindex : n + i + k - (progress a b n + i) = n - progress a b n + k := by
    omega
  simp [hindex]
```

Model przyczynowy jest w `Profs/CausalShift.lean`. `CausalStream α Δ`
przechowuje początek logiczny `origin`, nieujemny ogon `tail` i funkcję
wartości; interwał `Δ` jest parametrem typu. Dla dodatniej jednostki czasu
`tick` wejścia mają interwały `a*tick` i `b*tick`, a wyjście
`a*b/(a+b)*tick`. Lemat `causalInterleave_interval` sprawdza zgodność
z harmonicznym wzorem na interwał i dodatniość wyniku.

`read n` zwraca `none` przed początkiem logicznym i `some (value n)` od
tego początku. `none` oznacza brak rekordu, a nie pole `NULL` w istniejącej
krotce. Wartości funkcji pomocniczej przed początkiem nie są obserwowalne.
Przesunięcie zwiększa początek o `m`, obcina ogon `tail-m` od dołu zerem
i czyta wartość pod indeksem `n-m`. `causalShift_read` dowodzi zgodności
odczytów, łącznie z brakiem prefiksu:

```anchor causal_shift -showProofStates (module := Profs.CausalShift)
def causalShift {α : Type*} {d : ℚ} (m : ℕ) (S : CausalStream α d) : CausalStream α d where
  origin := S.origin + m
  tail := S.tail - m
  value n := S.value (n - m)

theorem causalShift_read {α : Type*} {d : ℚ} (m n : ℕ) (S : CausalStream α d) :
    (causalShift m S).read n = if m ≤ n then S.read (n - m) else none := by
  by_cases hm : m ≤ n
  · have hiff : S.origin + m ≤ n ↔ S.origin ≤ n - m := by omega
    simp [CausalStream.read, causalShift, hm, hiff]
  · have hnot : ¬ S.origin + m ≤ n := by omega
    simp [CausalStream.read, causalShift, hm, hnot]
```

Początek przeplotu `interleaveOrigin` jest najmniejszym `n`, dla którego
obie mapy osiągają początki wejść: `originReady` wymaga
`O_A ≤ floor(n*b/(a+b))` oraz `O_B ≤ n-floor(n*b/(a+b))`. To wspólny próg
monotonicznych map stosowany przez `compiler::computeLogicalOrigin()`.
`interleaveOrigin_le_iff` dowodzi charakterystyki tego progu, a
`causalInterleave_inputs_defined` gwarantuje, że od początku wyjścia obie
pozycje odnoszą się do istniejących rekordów wejść.

`originReady_shift` pokazuje, że warunek dla przesuniętych wejść zachodzi
wtedy i tylko wtedy, gdy `i+k ≤ n` i warunek pierwotny zachodzi dla
`n-(i+k)`. Wynika stąd równość początków dla dowolnych `O_A,O_B`, także
zerowych, i dla zerowego przesunięcia:

```anchor causal_origin -showProofStates (module := Profs.CausalShift)
theorem interleaveOrigin_shift (a b i k OA OB : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) :
    interleaveOrigin a b (OA + i) (OB + k) ha hb =
      interleaveOrigin a b OA OB ha hb + (i + k) := by
  apply Nat.le_antisymm
  · apply (interleaveOrigin_le_iff a b _ _ _ ha hb).mpr
    apply (originReady_shift a b i k OA OB _ ha hb hmatch).mpr
    refine ⟨by omega, ?_⟩
    simpa [interleaveOrigin] using Nat.find_spec (originReady_exists a b OA OB ha hb)
  · have h := Nat.find_spec (originReady_exists a b (OA + i) (OB + k) ha hb)
    obtain ⟨hn, hready⟩ := (originReady_shift a b i k OA OB _ ha hb hmatch).mp h
    have hmin := (interleaveOrigin_le_iff a b OA OB _ ha hb).mpr hready
    change interleaveOrigin a b OA OB ha hb ≤
      interleaveOrigin a b (OA + i) (OB + k) ha hb - (i + k) at hmin
    change i + k ≤ interleaveOrigin a b (OA + i) (OB + k) ha hb at hn
    omega
```

Kolejna część dotyczy ogona startowego. `phaseRequirement` zapisuje
warunek dostępności jednej krotki z sufitem, a `exactInterleaveTail`
bierze maksimum po pełnym okresie `a+b`. Po przesunięciu wejść ich
ogony są obcięte od dołu zerem. Dla każdej fazy wymaganie po lewej
stronie jest co najmniej wymaganiem pierwotnym minus `L`; maksimum
zachowuje tę nierówność. Strona sfaktoryzowana ma więc ogon nie większy.

```anchor shift_tail -showProofStates (module := Profs.ShiftMatching)
theorem exact_tail_rewrite (a b i k WA WB : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) :
    max 0 (exactInterleaveTail a b WA WB - (i + k)) ≤
      exactInterleaveTail a b (WA - i) (WB - k) := by
  exact shift_matching_tail a b i k WA WB ha hb hmatch
    (List.range (a + b)) (selectsA a b) (selectedIndex a b)
```

Równość ogonów nie jest tezą. W formalnym przykładzie dla odstępów
`1:2`, przesunięć `2:1` i zerowych ogonów obu wejść, ogon strony
niesfaktoryzowanej wynosi 2, a sfaktoryzowanej 0. Lean sprawdza te liczby
obliczeniem w jądrze (`decide +kernel`):

```anchor tail_strict -showProofStates (module := Profs.ShiftMatching)
theorem tail_can_be_strict :
    exactInterleaveTail 1 2 (0 - 2) (0 - 1) = 2 ∧
    max 0 (exactInterleaveTail 1 2 0 0 - (2 + 1 : ℕ)) = 0 := by
  decide +kernel
```

Maksimum po jednym okresie `a+b` jest pełnym ogonem dlatego, że wymaganie
fazy jest okresowe. Po `a+b` slotach wyjścia licznik `progress` rośnie
o `b`, więc wybór składowej się nie zmienia, a indeks rekordu wejścia rośnie
o `b` (składowa A) albo o `a` (składowa B). Sufit rośnie wtedy o
`b*(a+b)/b = a*(a+b)/a = a+b`, dokładnie o tyle, o ile rośnie odejmowany
indeks slotu. Lemat `phaseRequirement_period` wykonuje ten rachunek dla
jednej fazy, a `tailRequirement_period` składa oba przypadki:

```anchor tail_period -showProofStates (module := Profs.InterleaveTailExact)
theorem tailRequirement_period (a b WA WB n : ℕ) (ha : 0 < a) (hb : 0 < b) :
    selectedRequirement a b WA WB (selectsA a b) (selectedIndex a b) (n + (a + b)) =
      selectedRequirement a b WA WB (selectsA a b) (selectedIndex a b) n := by
  have hsel := selectsA_period a b n ha hb
  have hp := progress_period a b n ha hb
  have hle := progress_le_slot a b n ha hb
  have haq : (a : ℚ) ≠ 0 := by exact_mod_cast ha.ne'
  have hbq : (b : ℚ) ≠ 0 := by exact_mod_cast hb.ne'
  unfold selectedRequirement selectedIndex
  rw [hsel]
  by_cases hA : selectsA a b n = true
  · simp only [hA, ↓reduceIte, hp]
    exact phaseRequirement_period _ WA n b (a + b) (ratioA a b)
      (by unfold ratioA; field_simp)
  · have hB : selectsA a b n = false := Bool.eq_false_of_not_eq_true hA
    simp only [hB, Bool.false_eq_true, ↓reduceIte, hp]
    have hidx : n + (a + b) - (progress a b n + b) = n - progress a b n + a := by omega
    rw [hidx]
    exact phaseRequirement_period _ WB n a (a + b) (ratioB a b)
      (by unfold ratioB; field_simp)
```

Dokładność ogona nie może być wyrażona przez `phaseRequirement`, bo wtedy
byłaby tautologią. `interleaveReady a b WA WB W tick n` zapisuje warunek
zdarzeniowy wprost w czasie wymiernym: rekord wejścia o indeksie
`j = selectedIndex a b n` i ogonie `W_src` jest określony w chwili
`(j+1+W_src)*Δ_src`, a slot `n` wyjścia o ogonie `W` kończy się w chwili
`(n+1+W)*Δ_out`, gdzie `Δ_A = a*tick`, `Δ_B = b*tick` i
`Δ_out = a*b/(a+b)*tick`. `interleaveReady_iff` pokazuje, że dla `tick > 0`
ten warunek jest równoważny `selectedRequirement … n ≤ W`; sufit pojawia się
dopiero tu, bo prawa strona po podzieleniu przez `Δ_out` jest liczbą
całkowitą.

Z okresowości wynika twierdzenie o dokładności. Dla dowolnego progu `N₀`
ogon `toNat` jest nie większy od `W` wtedy i tylko wtedy, gdy każdy rekord
`n ≥ N₀` jest dostępny z ogonem `W`. Kierunek w prawo mówi, że ogon
wystarcza dla wszystkich rekordów, nie tylko w pierwszym okresie (każde `n`
sprowadza się do fazy `n % (a+b)`). Kierunek w lewo mówi, że ogon jest
minimalny: każda faza z pierwszego okresu powtarza się jako
`m + (a+b)*N₀ ≥ N₀`, więc żaden mniejszy ogon nie spełnia warunku nawet
wtedy, gdy pominąć skończony prefiks rekordów:

```anchor tail_exact -showProofStates (module := Profs.InterleaveTailExact)
theorem exactInterleaveTail_exact (a b WA WB W N0 : ℕ) (ha : 0 < a) (hb : 0 < b)
    (tick : ℚ) (htick : 0 < tick) :
    (exactInterleaveTail a b WA WB).toNat ≤ W ↔
      ∀ n, N0 ≤ n → interleaveReady a b WA WB W tick n := by
  have hs : 0 < a + b := by omega
  simp only [interleaveReady_iff a b WA WB W _ ha hb tick htick]
  constructor
  · intro hW n _
    have hle : exactInterleaveTail a b WA WB ≤ W := by omega
    unfold exactInterleaveTail at hle
    obtain ⟨_, hall⟩ := (phaseMax_le_iff _ _ _).mp hle
    rw [tailRequirement_mod a b WA WB n ha hb]
    exact hall _ (List.mem_range.mpr (Nat.mod_lt _ hs))
  · intro h
    have hle : exactInterleaveTail a b WA WB ≤ W := by
      unfold exactInterleaveTail
      refine (phaseMax_le_iff _ _ _).mpr ⟨by omega, ?_⟩
      intro m _
      have hN : N0 ≤ m + (a + b) * N0 := by nlinarith
      have hm := h (m + (a + b) * N0) hN
      rwa [tailRequirement_add_mul a b WA WB m N0 ha hb] at hm
    omega
```

Dla `N₀` równego początkowi logicznemu otrzymujemy wersję przyczynową:
ogon `causalInterleave` jest najmniejszym ogonem, przy którym każdy
istniejący rekord wyjścia jest emitowany nie wcześniej, niż określony jest
wybrany rekord wejścia:

```anchor causal_tail_exact -showProofStates (module := Profs.InterleaveTailExact)
theorem causalInterleave_tail_exact (a b : ℕ) (ha : 0 < a) (hb : 0 < b)
    {α β : Type*} {tick : ℚ} (htick : 0 < tick)
    (A : CausalStream α ((a : ℚ) * tick)) (B : CausalStream β ((b : ℚ) * tick)) (W : ℕ) :
    (causalInterleave a b ha hb A B).tail ≤ W ↔
      ∀ n, (causalInterleave a b ha hb A B).origin ≤ n →
        interleaveReady a b A.tail B.tail W tick n :=
  exactInterleaveTail_exact a b A.tail B.tail W _ ha hb tick htick
```

Teza łączna używa już operatora przyczynowego `causalShift`.
`causal_shift_matching_read` przenosi pomocniczą tożsamość indeksową na
odczyt starszych rekordów i porównuje `Option` pod każdym indeksem,
również przed początkiem. `available n slot` oznacza, że rekord istnieje
oraz `n+1+tail ≤ slot`; dla dodatniego wspólnego interwału slot wyznacza
moment dostępności. `causalShift_available_source` sprawdza też, że
przesunięcie nie udostępnia rekordu wcześniej niż jego źródło.
`causalShift_tail_exact` dowodzi, że `max(0,tail-m)` jest najmniejszym
ogonem pokrywającym dostępność starszych rekordów we wszystkich
istniejących slotach wyjścia.

Poniższe twierdzenie łączy równość interwałów, początków i odczytów
z nierównością ogonów oraz implikacją dostępności: jeśli rekord strony
niesfaktoryzowanej jest dostępny w danym slocie, ten sam rekord strony
sfaktoryzowanej jest już dostępny.

```anchor causal_r1 -showProofStates (module := Profs.CausalShift)
theorem causal_shift_matching (a b i k : ℕ) (ha : 0 < a) (hb : 0 < b)
    (hmatch : i * a = k * b) {α β : Type*} {tick : ℚ}
    (A : CausalStream α ((a : ℚ) * tick)) (B : CausalStream β ((b : ℚ) * tick)) :
    let lhs := causalInterleave a b ha hb (causalShift i A) (causalShift k B)
    let rhs := causalShift (i + k) (causalInterleave a b ha hb A B)
    lhs.interval = rhs.interval ∧ lhs.origin = rhs.origin ∧
      (∀ n, lhs.read n = rhs.read n) ∧ rhs.tail ≤ lhs.tail ∧
      (∀ n slot, lhs.available n slot → rhs.available n slot) := by
  dsimp only
  have ho := interleaveOrigin_shift a b i k A.origin B.origin ha hb hmatch
  have ht := causal_shift_matching_tail a b i k A.tail B.tail ha hb hmatch
  refine ⟨rfl, ho, causal_shift_matching_read a b i k ha hb hmatch A B, ht, ?_⟩
  intro n slot h
  dsimp [CausalStream.available, causalInterleave, causalShift] at *
  omega
```

# Zgodność z kodem silnika

Twierdzenie `causal_shift_matching` dotyczy obiektów modelu. Uzasadnienie
poprawności reguły R1 w silniku wymaga dwóch kroków, które nie są
sprawdzane przez Lean, lecz przez porównanie kodu z definicjami:

* każda wielkość, którą kompilator i `dataModel` liczą dla węzłów `>N`
  i `#`, jest równa odpowiadającej jej definicji modelu;
* przepisanie wykonywane przez `compiler::factorMatchedHashTimeMoves()`
  zamienia dokładnie stronę `lhs` twierdzenia na stronę `rhs`.

Odwołania dotyczą repozytorium `retractordb` w wersji `36625de`. Jeśli
któryś z wymienionych fragmentów kodu się zmieni, ten rozdział trzeba
porównać z nim ponownie.

## Interwały wymierne a parametry naturalne

Silnik przechowuje interwały jako `boost::rational<int>`. Dla dodatnich
`Δ_A`, `Δ_B` niech `Δ_A/Δ_B = p/q` będzie ułamkiem nieskracalnym
i `tick = Δ_A/p = Δ_B/q`. Wtedy `Δ_A = a*tick`, `Δ_B = b*tick` dla `a = p`,
`b = q` oraz `tick > 0`, czyli dokładnie założenia modelu. Wszystkie
wyrażenia kodu, które niżej porównujemy, zależą od interwałów wyłącznie
przez ten stosunek:

* `zet = Δ_B/(Δ_A+Δ_B) = b/(a+b)` w `Hash()` i w `computeLogicalOrigin()`;
* `Δ_A/Δ_out = (a+b)/b = ratioA a b` oraz `Δ_B/Δ_out = (a+b)/a = ratioB a b`,
  bo `compiler::resolveStreamIntervals()` nadaje węzłowi `#` interwał
  `Δ_A*Δ_B/(Δ_A+Δ_B)`, zgodny z `causalInterleave_interval`;
* okres przeglądu `period = p+q = a+b` w `HashStartupLatency()`;
* warunek reguły `Δ_A*i = Δ_B*k`, równoważny `i*a = k*b`, czyli `hmatch`.

Węzeł `>N` dziedziczy interwał producenta (`resolveStreamIntervals()`,
gałąź `STREAM_TIMEMOVE`), tak jak `causalShift` zachowuje parametr `Δ`.

## Przesunięcie `>N`

* Początek: `computeLogicalOrigin()`, gałąź `STREAM_TIMEMOVE`, ustawia
  `o1 + N`, czyli `causalShift.origin`.
* Ogon: `computeStartupLatency()`, gałąź `STREAM_TIMEMOVE`, ustawia
  `max(0, w1 - N)`, czyli odejmowanie `S.tail - m` w liczbach naturalnych.
* Treść: `dataModel::constructInputPayload()` pobiera
  `fetchForward(src, n - N)` dla indeksu logicznego `n`, a
  `queryInputsAvailable()` sprawdza ten sam indeks. To `value n := S.value (n - m)`.
  Silnik liczy tylko dla `n ≥ o1 + N`, więc `n - N ≥ o1` i różnica
  w liczbach całkowitych jest równa odejmowaniu w `ℕ`. Model obcina je
  tylko przed początkiem, gdzie `read` zwraca `none`.

## Przeplot `#`

* Wybór składowej i pozycja: `Hash()` w `SOperations.hpp` porównuje
  `floor(zet*n)` z `floor(zet*(n+1))`. Przy równości zwraca pozycję
  `n - floor(zet*n)` drugiej składowej, w przeciwnym razie `floor(zet*n)`
  pierwszej. To `interleaveAt` z `p = n*b/(a+b)`. Zarówno
  `constructInputPayload()`, jak i `queryInputsAvailable()` wywołują `Hash()`
  z pierwszym argumentem programu jako `A`; kolejność ma znaczenie, bo
  przeplot nie jest przemienny (`interleave_starts_with_right`).
* Początek: gałąź `STREAM_HASH` w `computeLogicalOrigin()` bierze większy
  z progów `firstIndexReaching` dla map `floor(zet*n)` i `n - floor(zet*n)`.
  `firstIndexReaching` zwraca najmniejsze `n`, dla którego niemalejąca mapa
  osiąga próg (podwajanie, potem bisekcja; próg `≤ 0` daje `0`). Obie mapy
  są niemalejące, więc zbiór `n` spełniających każdy warunek jest półprostą,
  a najmniejszy element ich przecięcia jest większym z dwóch progów.
  `interleaveOrigin_le_iff` charakteryzuje ten sam najmniejszy element,
  więc kod liczy `interleaveOrigin`.
* Ogon: gałąź przeglądu w `HashStartupLatency()` dla każdej fazy
  `index < period` wyznacza pozycję i składową przez `Hash()` i liczy
  `ceil((position+1+W_src)*Δ_src/Δ_out) - 1 - index`. Sufit jest liczony
  w 64 bitach wzorem `(x+y-1)/y`, poprawnym dla dodatnich `x`, `y`. Wynik
  startuje od `0` i bierze maksimum. To `selectedRequirement` z
  `phaseRequirement`, złożone przez `phaseMax` (fałd z wartością
  początkową `0`) i obcięte `toNat`, czyli ogon `causalInterleave`.
  Na mocy `causalInterleave_tail_exact` ta gałąź zwraca najmniejszy ogon
  spełniający zdarzeniowy warunek dostępności dla wszystkich istniejących
  rekordów.

## Emisja a dostępność

`dataModel::processRows()` milczy przez `logicalOrigin + startupLatency`
slotów własnego interwału, a pierwszy wyemitowany rekord nosi indeks
logiczny `logicalOrigin` (`logicalIndexBase` ustawiony na początek
logiczny przy starcie planu). Rekord o indeksie `n` jest więc emitowany
w slocie kończącym się w chwili `(n+1+tail)*Δ`. To dokładnie najmniejszy
`slot`, dla którego zachodzi `available n slot`. Implikacja dostępności
z twierdzenia mówi zatem, że po przepisaniu silnik emituje każdy rekord
nie później niż przed przepisaniem.

## Przepisanie R1

`compiler::factorMatchedHashTimeMoves()` dopasowuje zapytanie o programie
`PUSH X, PUSH Y, STREAM_HASH`, w którym `X` i `Y` są substratami
o programach `PUSH A, STREAM_TIMEMOVE i` oraz `PUSH B, STREAM_TIMEMOVE k`,
z `i, k ≥ 0`, dodatnimi interwałami źródeł i `Δ_A*i = Δ_B*k` (w
`rational<int64_t>`). To strona `lhs`. Wynik:

* substrat `PUSH A, PUSH B, STREAM_HASH` z tą samą kolejnością składowych,
  nowy albo ponownie użyty tylko wtedy, gdy istniejący węzeł ma identyczny
  program, interwał i zgodny schemat;
* dopasowane zapytanie dostaje program `PUSH hash, STREAM_TIMEMOVE (i+k)`,
  a przy `i+k = 0` samo `PUSH hash`, co odpowiada `causalShift 0`,
  czyli identyczności na początku, ogonie i odczytach.

To strona `rhs`. Pozostałe warunki reguły (jednoznaczność nazw, zakres
`int` dla `i+k`, przekierowanie odwołań w schemacie, usuwanie substratów
bez konsumentów) ograniczają tylko to, kiedy reguła zadziała, i chronią
innych konsumentów. Nie zmieniają obliczanych wielkości.

Z powyższych odpowiedniości i z `causal_shift_matching` wynika, że
przepisany węzeł ma ten sam interwał, ten sam początek logiczny i tę samą
treść rekordu pod każdym indeksem logicznym, a ogon nie większy. W języku
`def:observable`: `Val` jest równe, a `Lat` nie rośnie.

## Gałąź powyżej progu przeglądu

Dla `period > kHashPhaseScanLimit` `HashStartupLatency()` zwraca
`F(W_A, W_B) = max(C_A(W_A), C_B(W_B) + own)`, gdzie
`C_A(w) = ceil(w*(a+b)/b)`, `C_B(w) = ceil(w*(a+b)/a)` (zero dla `w ≤ 0`),
a `own = ceil((a+b-1)/a) ≥ 1` zależy tylko od stosunku interwałów. Obie
strony reguły mają te same interwały składowych, więc obie trafiają do tej
samej gałęzi. Poniższy argument jest ręczny i nie ma go w Lean.

Dla `W_A ≥ i` mamy `C_A(W_A - i) = C_A(W_A) - (i+k)`, bo
`i*(a+b)/b = i + i*a/b = i+k` jest liczbą całkowitą. Dla `W_A < i` mamy
`W_A ≤ i-1`, więc `C_A(W_A) ≤ (i+k) - floor((a+b)/b) < i+k` i
`C_A(0) = 0 ≥ C_A(W_A) - (i+k)`. W obu przypadkach
`C_A(max(0, W_A - i)) ≥ C_A(W_A) - (i+k)`; analogicznie
`C_B(max(0, W_B - k)) ≥ C_B(W_B) - (i+k)`, bo `k*(a+b)/a = k+i`. Stąd
`F(max(0,W_A-i), max(0,W_B-k)) ≥ F(W_A, W_B) - (i+k)`, a ponieważ lewa
strona jest nie mniejsza niż `own ≥ 1`, jest też nie mniejsza niż
`max(0, F(W_A, W_B) - (i+k))`, czyli ogon strony `rhs`. Nierówność ogonów
zachodzi więc także w tej gałęzi. Czy `F` ogranicza granicę zdarzeniową
z góry, to osobna własność tej gałęzi, której ten rozdział nie dowodzi.

## Poza zakresem

* Odpowiedniości kodu i modelu są sprawdzone przez porównanie, nie
  mechanicznie; formalny dowód zgodności implementacji C++ nie istnieje.
* Deskryptory, mapy `NULL`, luki, materializacja i pojemności historii
  (`computeRequiredCapacities`) nie są modelowane.
* Instancja ad hoc dołączająca do biegnącej osi wyznacza `logicalIndexBase`
  z bieżącego slotu; model opisuje plan startujący od slotu zero.
* Konsumenci przepisanego węzła widzą ten sam początek i te same odczyty,
  ale mniejszy ogon wejścia. To, że ich ogony też nie rosną, wymaga
  monotoniczności reguł ogona pozostałych operatorów względem ogonów
  wejść; ten rozdział jej nie dowodzi.
* Mapy w `firstIndexReaching` liczą w `rational<int>`; dla bardzo dużych
  `n` możliwe jest przepełnienie, przed którym chroni tylko limit
  `kOriginSearchLimit`.
