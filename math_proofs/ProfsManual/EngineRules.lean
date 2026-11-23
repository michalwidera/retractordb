import VersoManual

open Verso.Genre Manual
open Verso.Code.External

set_option verso.exampleProject "."

#doc (Manual) "Zgodność funkcji SOperations.hpp z modelem" =>
%%%
file := "engine-rules"
%%%

Poprzednie rozdziały dowodzą dokładności początków i ogonów w modelu: `affineRead`,
`affineTail`, `phaseStartupLatency`, `agseOrigin` i `exactInterleaveTail`. Ten rozdział
zamyka krok od kodu C++ do tych wzorów dla funkcji z `src/include/SOperations.hpp`
(zgłoszenie #634, warstwa A). Każda funkcja ma w `Profs/EngineRules.lean` przepisanie
`<nazwa>Cpp` w arytmetyce maszynowej, a twierdzenie mówi, że przy jawnych warunkach
przepisanie daje wartość modelu. Przepisanie odtwarza kod krok po kroku: ta sama
kolejność działań, te same typy, a przy każdym kroku komentarz z odpowiadającym
fragmentem C++. Jego wierność wobec kodu sprawdzają człowiek i test opisany na końcu
rozdziału.

# Arytmetyka sprawdzana

`Profs/CppArith.lean` modeluje `int`, `std::int64_t` i `__int128` jako arytmetykę
sprawdzaną, nie zawijaną. Operacja na typie ze znakiem o szerokości `w` bitów to funkcja
`ℤ → ℤ → Option ℤ`, która daje `none` dla wyniku spoza `[-2^(w-1), 2^(w-1))` i dla dzielenia
przez zero, czyli dokładnie tam, gdzie C++ ma niezdefiniowane zachowanie. Dzielenie i reszta
obcinają w stronę zera, jak w C++ od wersji 11 (`Int.tdiv`, `Int.tmod`). Typy `Int32`
i `Int64` z rdzenia Lean liczą modulo `2^w`, więc opisywałyby zachowanie, którego C++ dla
typów ze znakiem nie gwarantuje.

Wyrażenie mieszane, na przykład `int` razy `__int128`, liczy się w szerokości szerszego
argumentu. Rzutowanie zawężające daje `none` poza zakresem; kod silnika rzutuje wyłącznie
wartości już sprawdzone. `boost::rational<int>` jest parą `(num, den)` z niezmiennikiem
Boost: `den > 0`, `gcd(num, den) = 1` i obie składowe w zakresie `int`. Po poprawkach #635,
#636 i #644 funkcje nie liczą na `rational`, tylko czytają licznik, mianownik i porównują
pary operatorem `==`. `std::gcd` jest modelowane jako `Int.gcd`, bez przepisywania pętli
Euklidesa.

Ponieważ każde twierdzenie `<nazwa>Cpp_eq` mówi, że przepisanie daje `some`, jest ono
zarazem dowodem braku niezdefiniowanego zachowania przy swoich hipotezach. Przykład:
wspólne zakończenie reguł ogona, które nasyca wynik na `INT64_MAX`.

```anchor saturated_tail_cpp (module := Profs.EngineRules)
def saturatedTailCpp (numerator denominator : ℤ) : Option ℤ := do
  -- numerator + denominator
  let s ← addC wInt128 numerator denominator
  -- ... - 1
  let s1 ← subC wInt128 s 1
  -- (...) / denominator
  let qt ← tdivC wInt128 s1 denominator
  -- const __int128 tail = (...) - 1;
  let tail ← subC wInt128 qt 1
  -- if (tail < 0) return 0;
  if tail < 0 then some 0
  -- return tail > limit ? limit : static_cast<std::int64_t>(tail);
  else if tail > int64Max then some int64Max
  else castC wInt64 tail
```

Twierdzenie `saturatedTailCpp_eq` daje `min (max 0 (⌈N/D⌉ - 1)) INT64_MAX` przy `N ≥ 0`,
`D > 0` i `N + D` w zakresie `__int128`. Ostatni warunek jest potrzebny, bo C++ liczy
`(numerator + denominator) - 1`; każda reguła, która kończy się na `saturatedTail`,
rozładowuje go w swoim dowodzie.

# Parametry modelu

Parametry modelu są nieskróconymi iloczynami krzyżowymi interwałów: dla `r = r1/r2`
i `s = s1/s2` jest `cross r s = r1*s2`. `affineRead`, `affineTail` i `affineTail_exact`
działają dla dowolnych `p, q > 0`, więc parametry biorą się wprost z kodu. Mapa odczytu `Θ`:

```anchor div_cpp (module := Profs.EngineRules)
def divCpp (a b : RatInt) (i : ℤ) : Option ℤ := do
  -- const __int128 numerator = (__int128{i} + 1) * deltaA.numerator() * deltaB.denominator();
  let i1 ← addC wInt128 i 1
  let t1 ← mulC wInt128 i1 a.num
  let numerator ← mulC wInt128 t1 b.den
  -- const __int128 denominator = __int128{deltaA.denominator()} * deltaB.numerator();
  let denominator ← mulC wInt128 a.den b.num
  -- numerator + denominator - 1
  let s ← addC wInt128 numerator denominator
  let s1 ← subC wInt128 s 1
  -- (...) / denominator
  let c ← tdivC wInt128 s1 denominator
  -- i + (...)   (int + __int128 -> __int128)
  let idx ← addC wInt128 i c
  -- return saturatedIndex(...);
  saturatedIndexCpp idx
```

`divCpp_eq` mówi, że dla dodatnich znormalizowanych interwałów i `0 ≤ i ≤ INT_MAX` wynik to
`min (affineRead (u+v) v (u+v-1) i) INT64_MAX` przy `u = cross a b` i `v = cross b a`;
z `thetaRead_eq` jest to `i + ⌈(i+1)·u/v⌉`. Reguła ogona różnicy liczy na nieskróconej
parze i dlatego nie potrzebuje założenia `Coprime` z `subtractTail_exact`: `g = gcd(p, q)`
dzieli `q`, więc `(q-1) - (q-1) mod g = q - g`.

```anchor subtract_startup_latency_cpp (module := Profs.EngineRules)
def subtractStartupLatencyCpp (s t : RatInt) (W : ℤ) : Option ℤ := do
  -- const std::int64_t sourceScaled = std::int64_t{deltaTarget.denominator()} * deltaSource.numerator();
  let sourceScaled ← mulC wInt64 t.den s.num
  -- const std::int64_t targetScaled = std::int64_t{deltaTarget.numerator()} * deltaSource.denominator();
  let targetScaled ← mulC wInt64 t.num s.den
  -- const std::int64_t h = std::gcd(targetScaled, sourceScaled);
  let h ← gcd64 targetScaled sourceScaled
  -- (__int128{sourceLatency} + 2) * sourceScaled
  let a ← addC wInt128 W 2
  let b ← mulC wInt128 a sourceScaled
  -- const __int128 numerator = (...) - h;
  let numerator ← subC wInt128 b h
  -- return saturatedTail(numerator, targetScaled);
  saturatedTailCpp numerator targetScaled
```

# Twierdzenia

Hipotezy wspólne: interwały znormalizowane i dodatnie, indeks rekordu z `[0, INT_MAX]`,
ogony z `[0, 2^31)`. To założenia kompilatora planu i `assert()` z kodu; w buildzie Release
asercji nie ma, więc wymusza je kompilator, nie funkcja.

- Funkcje pomocnicze: `floorDivCpp_eq` (`Int.fdiv`), `roundedAwayFromZeroCpp_eq`
  i `roundedAwayFromZeroCpp_natCast` (zaokrąglenie od zera, dla nieujemnych argumentów sufit),
  `saturatedIndexCpp_eq` i `saturatedTailCpp_eq` (nasycenie jako `min` z `INT64_MAX`).
- Mapy odczytu: `divCpp_eq`, `modCpp_eq`, `subtractCpp_eq` i `addCpp_eq` dają `affineRead`
  z parametrami `cross`; `hashLeftCountCpp_eq`, `hashCpp_eq` i `hashCpp_eq_reduced` dają
  `progress`, `selectsA` i `selectedIndex` przeplotu, także po skróceniu pary.
- Reguły ogona: `addStartupLatencyCpp_eq`, `subtractStartupLatencyCpp_eq` i
  `nthetaStartupLatencyCpp_eq` dają `affineTail`; `phaseStartupLatencyCpp_eq` i
  `thetaStartupLatencyCpp_eq` dają `phaseStartupLatency`, a `thetaStartupLatencyCpp_eq_affine`
  daje `affineTail` mapy `Div` przy wejściu będącym przeplotem (`s = t·o/(t+o)`, `theta_ratio`);
  `hashStartupLatencyCpp_eq` daje `exactInterleaveTail` pary skróconej. Wszystkie z nasyceniem
  na `INT64_MAX`.
- Okno AGSE: `agseStartupLatencyCpp_eq` (`affineTail h F 0 W`) i `agseLogicalOriginCpp_eq`
  (`agseOrigin` przy `length ≠ 0`), bez nasycenia, bo wynik jest mniejszy niż `2^62`.
- Wnioski `_exact`: wynik kodu nie przekracza ogona `W'` z zakresu `int` wtedy i tylko wtedy,
  gdy od każdego progu każdy rekord czytany przez mapę odczytu operatora jest gotowy. Przy
  takich `W'` nasycenie nie ma znaczenia.

Każde z tych twierdzeń zależy najwyżej od aksjomatów `propext`, `Classical.choice`
i `Quot.sound`; sprawdza to `AxiomCheck.lean` dla wszystkich twierdzeń `Profs.*` przy każdym
uruchomieniu `render.sh`.

# Związek z silnikiem

Przepisania są obliczalne, więc `OracleMain.lean` liczy nimi tablice `kCpp*`
w `test/UnitTest/proofOracle.hpp`, a testy `ProofOracle.<nazwa>Cpp_table` porównują je
z funkcjami C++. Siatka obejmuje małe wartości i punkty graniczne: interwały `INT_MAX/1`,
`1/INT_MAX`, `(INT_MAX-1)/INT_MAX` i `2000000002/5`, indeksy do `INT_MAX`, ogony do `INT_MAX`,
argumenty `int64` od `INT64_MIN` do `INT64_MAX` i liczniki `__int128` do `2^127 - 1`. Punkty,
w których przepisanie daje `none`, są w tablicy oznaczone; test nie wywołuje dla nich funkcji
C++ i sprawdza, że leżą dokładnie poza kontraktem funkcji.

Granice tego wyniku:

- Wierność przepisania wobec kodu C++ i nagłówka Boost sprawdza człowiek oraz test na siatce;
  luka się zwęża, ale nie znika.
- `std::gcd` jest funkcją matematyczną; pętla Euklidesa nie jest przepisana.
- Poza zakresem są `compiler::computeStartupLatency()` (wybór reguły dla węzła, `toSlots`),
  `computeLogicalOrigin()` i emisja w `dataModel::processRows()`. W szczególności to, że
  `Θ` i `~Θ` są wołane z interwałem wejścia równym interwałowi przeplotu, należy do walidacji
  planu (warstwa B), nie do tego rozdziału.
