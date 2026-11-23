import Profs

/- Wyrocznia dla testow silnika RetractorDB. Tablice sa liczone definicjami, o ktorych
   mowia dowody w Profs, i wypisywane jako inicjalizatory C++. Plik wynikowy tworzy
   gen-oracle.sh; nie edytowac go recznie. -/

open Profs
open Profs.Cpp

def interleaveRows : List String := Id.run do
  let mut rows := []
  for a in List.range' 1 12 do
    for b in List.range' 1 12 do
      for n in List.range 48 do
        let (fromB, pos) :=
          match interleaveAt a b (id : ℕ → ℕ) (id : ℕ → ℕ) n with
          | .inl p => ("false", p)
          | .inr p => ("true", p)
        let left := deinterleaveLeft a b (id : ℕ → ℕ) n
        let right := deinterleaveRight a b (id : ℕ → ℕ) n
        rows := rows ++ [s!"    \{{a}, {b}, {n}, {fromB}, {pos}, {left}, {right}},"]
  return rows

/- Okres zrodla i docelowy interwal roznicy sa w cwiartkach: period/4, target/4. -/
def subtractRows : List String := Id.run do
  let mut rows := []
  for period in List.range' 1 16 do
    for target in List.range' period (17 - period) do
      for n in List.range 21 do
        let source : RegularStream ℕ := ⟨(period : ℚ) / 4, id⟩
        let index := (rateDiff source ((target : ℚ) / 4)).value n
        rows := rows ++ [s!"    \{{period}, {target}, {n}, {index}},"]
  return rows

def tailRows : List String := Id.run do
  let mut rows := []
  for a in List.range' 1 12 do
    for b in List.range' 1 12 do
      for wa in List.range 5 do
        for wb in List.range 5 do
          let tail := (exactInterleaveTail a b wa wb).toNat
          rows := rows ++ [s!"    \{{a}, {b}, {wa}, {wb}, {tail}},"]
  return rows

/- Mapy odczytu operatorow afinicznych: c = 0 (Add, Mod, AGSE), c = q-1 (Subtract), c = p-1 (Div). -/
def affineReadRows : List String := Id.run do
  let mut rows := []
  for p in List.range' 1 8 do
    for q in List.range' 1 8 do
      for c in [0, q - 1, p - 1].eraseDups do
        for n in List.range 24 do
          rows := rows ++ [s!"    \{{p}, {q}, {c}, {n}, {affineRead p q c n}},"]
  return rows

def affineTailRows : List String := Id.run do
  let mut rows := []
  for p in List.range' 1 8 do
    for q in List.range' 1 8 do
      for c in List.range 12 do
        for ws in List.range 4 do
          rows := rows ++ [s!"    \{{p}, {q}, {c}, {ws}, {affineTail p q c ws}},"]
  return rows

def agseOriginRows : List String := Id.run do
  let mut rows := []
  for f in List.range' 1 6 do
    for h in List.range' 1 6 do
      for w in List.range' 1 6 do
        for os in List.range 4 do
          rows := rows ++ [s!"    \{{f}, {h}, {w}, {os}, {agseOrigin f h w os}},"]
  return rows

/- ===== Przepisania funkcji SOperations.hpp (#634, krok A4, decyzja D5) =====

   Tablice licza wprost przepisania z Profs/EngineRules.lean; ut_proofOracle porownuje je
   z funkcjami C++. Siatka ma dwa obszary: male wartosci oraz punkty przy INT_MAX, INT64_MAX
   i granicy __int128. Wiersz z flaga defined = false to punkt, w ktorym przepisanie daje none,
   czyli niezdefiniowane zachowanie w C++: test nie wola tam funkcji C++, tylko sprawdza, ze
   punkt lezy poza kontraktem. W tablicach bez tej flagi none przerywa generowanie. -/

def intMax : ℤ := 2 ^ 31 - 1

/- Literal int64 dla C++: INT64_MIN nie ma postaci literalu. -/
def cI64 (x : ℤ) : String := if x = -2 ^ 63 then "(-9223372036854775807 - 1)" else toString x

/- Literal __int128 przez pomocnika I128(hi, lo) z proofOracle.hpp: x = hi*2^64 + lo. -/
def cI128 (x : ℤ) : String := s!"I128({x / 2 ^ 64}, {x % 2 ^ 64}u)"

def cBool (b : Bool) : String := if b then "true" else "false"

def cRat (r : RatInt) : String := s!"{r.num}, {r.den}"

def defined? (x : Option ℤ) : String × String :=
  match x with
  | some v => ("true", cI64 v)
  | none => ("false", "0")

def need {α : Type} (what : String) : Option α → Except String α
  | some v => .ok v
  | none => .error s!"przepisanie dalo none w punkcie objetym kontraktem: {what}"

/- Interwaly: male, spotykane w planach i graniczne. Wszystkie dodatnie i znormalizowane
   jak boost::rational<int>; 2^31 - 1 jest liczba pierwsza. -/
def ratGrid : List RatInt :=
  [⟨1, 1⟩, ⟨1, 2⟩, ⟨2, 3⟩, ⟨7, 3⟩, ⟨1, 360⟩, ⟨1, 250⟩, ⟨1, 48000⟩, ⟨60, 1⟩,
   ⟨intMax, 1⟩, ⟨1, intMax⟩, ⟨intMax - 1, intMax⟩, ⟨2000000002, 5⟩]

/- Mniejsza siatka dla funkcji trzech interwalow. -/
def ratGridSmall : List RatInt :=
  [⟨1, 1⟩, ⟨2, 3⟩, ⟨7, 3⟩, ⟨1, 48000⟩, ⟨60, 1⟩, ⟨intMax, 1⟩, ⟨1, intMax⟩]

/- Indeksy rekordow z [0, INT_MAX]; 59 652 324 to pierwszy indeks przepelnienia z #636. -/
def indexGrid : List ℤ := [0, 1, 2, 3, 7, 59652324, intMax - 1, intMax]

/- Ogony z [0, INT_MAX]; 307698 to przyklad z #635. -/
def tailGrid : List ℤ := [0, 1, 2, 3, 307698, 2 ^ 30, intMax]

def int64Grid : List ℤ :=
  [-2 ^ 63, -(2 ^ 63 - 1), -2 ^ 62, -7, -3, -1, 0, 1, 3, 7, 2 ^ 62, 2 ^ 63 - 1]

def cppDivisionRows : List String := Id.run do
  let mut rows := #[]
  for a in int64Grid do
    for b in int64Grid do
      let (fd, fv) := defined? (floorDivCpp a b)
      let (rd, rv) := defined? (roundedAwayFromZeroCpp a b)
      rows := rows.push s!"    \{{cI64 a}, {cI64 b}, {fd}, {fv}, {rd}, {rv}},"
  return rows.toList

def cppSaturatedIndexRows : Except String (List String) := do
  let mut rows := #[]
  for x in [0, 1, 2 ^ 31, 2 ^ 63 - 2, 2 ^ 63 - 1, 2 ^ 63, 2 ^ 93, 2 ^ 127 - 1] do
    let v ← need s!"saturatedIndex {x}" (saturatedIndexCpp x)
    rows := rows.push s!"    \{{cI128 x}, {cI64 v}},"
  return rows.toList

def cppSaturatedTailRows : List String := Id.run do
  let mut rows := #[]
  for n in [0, 1, 2, 5, 2 ^ 62, 2 ^ 93, 2 ^ 125, 2 ^ 127 - 2, 2 ^ 127 - 1] do
    for d in [1, 2, 3, 7, 2 ^ 62, 2 ^ 93, 2 ^ 127 - 1] do
      let (sd, sv) := defined? (saturatedTailCpp n d)
      rows := rows.push s!"    \{{cI128 n}, {cI128 d}, {sd}, {sv}},"
  return rows.toList

def cppReadRows : Except String (List String) := do
  let mut rows := #[]
  for a in ratGrid do
    for b in ratGrid do
      for i in indexGrid do
        let pt := s!"({cRat a}), ({cRat b}), {i}"
        let dv ← need s!"Div {pt}" (divCpp a b i)
        let mv ← need s!"Mod {pt}" (modCpp a b i)
        let sv ← need s!"Subtract {pt}" (subtractCpp a b i)
        let av ← need s!"Add {pt}" (addCpp a b i)
        let lv ← need s!"hashLeftCount {pt}" (hashLeftCountCpp a b i)
        let (fromB, pos) ← need s!"Hash {pt}" (hashCpp a b i)
        rows := rows.push
          s!"    \{{cRat a}, {cRat b}, {i}, {cI64 dv}, {cI64 mv}, {cI64 sv}, {cI64 av}, {cI64 lv}, {cBool fromB}, {pos}},"
  return rows.toList

def cppPairTailRows : Except String (List String) := do
  let mut rows := #[]
  for s in ratGrid do
    for t in ratGrid do
      for w in tailGrid do
        let pt := s!"({cRat s}), ({cRat t}), {w}"
        let av ← need s!"AddStartupLatency {pt}" (addStartupLatencyCpp s t w)
        let sv ← need s!"SubtractStartupLatency {pt}" (subtractStartupLatencyCpp s t w)
        let nv ← need s!"NThetaStartupLatency {pt}" (nthetaStartupLatencyCpp s t w)
        rows := rows.push s!"    \{{cRat s}, {cRat t}, {w}, {cI64 av}, {cI64 sv}, {cI64 nv}},"
  return rows.toList

def cppHashTailRows : Except String (List String) := do
  let mut rows := #[]
  for a in ratGrid do
    for b in ratGrid do
      for wa in [0, 1, intMax] do
        for wb in [0, 1, intMax] do
          let v ← need s!"HashStartupLatency ({cRat a}), ({cRat b}), {wa}, {wb}" (hashStartupLatencyCpp a b wa wb)
          rows := rows.push s!"    \{{cRat a}, {cRat b}, {wa}, {wb}, {cI64 v}},"
  return rows.toList

def cppThetaTailRows : Except String (List String) := do
  let mut rows := #[]
  for s in ratGridSmall do
    for t in ratGridSmall do
      for o in ratGridSmall do
        for w in [0, 1, intMax] do
          let v ← need s!"ThetaStartupLatency ({cRat s}), ({cRat t}), ({cRat o}), {w}"
            (thetaStartupLatencyCpp s t o w)
          rows := rows.push s!"    \{{cRat s}, {cRat t}, {cRat o}, {w}, {cI64 v}},"
  return rows.toList

/- Kres fazowy dowolnego znaku (PhaseStartupLatency przyjmuje ujemny licznik). -/
def phaseBoundGrid : List RatInt :=
  [⟨-intMax, 1⟩, ⟨-3, 2⟩, ⟨-1, 1⟩, ⟨0, 1⟩, ⟨1, 2⟩, ⟨5, 3⟩, ⟨intMax - 1, intMax⟩, ⟨intMax, 1⟩]

def cppPhaseTailRows : Except String (List String) := do
  let mut rows := #[]
  for k in phaseBoundGrid do
    for r in ratGrid do
      for w in tailGrid do
        let v ← need s!"PhaseStartupLatency ({cRat k}), ({cRat r}), {w}" (phaseStartupLatencyCpp k r w)
        rows := rows.push s!"    \{{cRat k}, {cRat r}, {w}, {cI64 v}},"
  return rows.toList

def widthGrid : List ℤ := [1, 2, 3, 7, 65536, intMax]

def cppAgseRows : Except String (List String) := do
  let mut rows := #[]
  for f in widthGrid do
    for h in widthGrid do
      for w in tailGrid do
        let v ← need s!"AgseStartupLatency {f}, {h}, {w}" (agseStartupLatencyCpp f h w)
        rows := rows.push s!"    \{{f}, {h}, {w}, {cI64 v}},"
  return rows.toList

/- Dlugosc okna dowolnego znaku, z 0 i INT_MIN. -/
def cppAgseOriginRows : Except String (List String) := do
  let mut rows := #[]
  for f in widthGrid do
    for h in widthGrid do
      for len in [-2 ^ 31, -3, -1, 0, 1, 3, intMax] do
        for os in [0, 1, 5, intMax] do
          let v ← need s!"AgseLogicalOrigin {f}, {h}, {len}, {os}" (agseLogicalOriginCpp f h len os)
          rows := rows.push s!"    \{{f}, {h}, {len}, {os}, {cI64 v}},"
  return rows.toList

def emitTable (decl : String) (rows : List String) : IO Unit := do
  IO.println s!"inline constexpr {decl}[] = \{"
  for row in rows do
    IO.println row
  IO.println "};"
  IO.println ""

def main : IO Unit := do
  IO.println "#include <cstdint>"
  IO.println ""
  IO.println "// interleaveAt a b id id n oraz deinterleaveLeft/Right a b id n."
  IO.println "struct InterleaveRow {"
  IO.println "  int a, b, n;"
  IO.println "  bool fromB;"
  IO.println "  int pos, left, right;"
  IO.println "};"
  IO.println ""
  IO.println "// (rateDiff <period/4, id> (target/4)).value n."
  IO.println "struct SubtractRow {"
  IO.println "  int period, target, n, index;"
  IO.println "};"
  IO.println ""
  IO.println "// (exactInterleaveTail a b WA WB).toNat."
  IO.println "struct TailRow {"
  IO.println "  int a, b, wa, wb, tail;"
  IO.println "};"
  IO.println ""
  IO.println "// affineRead p q c n = (n*p + c)/q."
  IO.println "struct AffineReadRow {"
  IO.println "  int p, q, c, n, read;"
  IO.println "};"
  IO.println ""
  IO.println "// affineTail p q c Ws."
  IO.println "struct AffineTailRow {"
  IO.println "  int p, q, c, ws, tail;"
  IO.println "};"
  IO.println ""
  IO.println "// agseOrigin F h w Os."
  IO.println "struct AgseOriginRow {"
  IO.println "  int f, h, w, os, origin;"
  IO.println "};"
  IO.println ""
  IO.println "// Przepisania funkcji SOperations.hpp z Profs/EngineRules.lean (#634, D5)."
  IO.println "constexpr __int128 I128(std::int64_t hi, std::uint64_t lo) {"
  IO.println "  return (static_cast<__int128>(hi) * (static_cast<__int128>(1) << 64)) + lo;"
  IO.println "}"
  IO.println ""
  IO.println "// floorDivCpp a b, roundedAwayFromZeroCpp a b; defined = false to none (UB w C++)."
  IO.println "struct CppDivisionRow {"
  IO.println "  std::int64_t a, b;"
  IO.println "  bool floorDefined;"
  IO.println "  std::int64_t floorDiv;"
  IO.println "  bool roundedDefined;"
  IO.println "  std::int64_t rounded;"
  IO.println "};"
  IO.println ""
  IO.println "// saturatedIndexCpp index."
  IO.println "struct CppSaturatedIndexRow {"
  IO.println "  __int128 index;"
  IO.println "  std::int64_t result;"
  IO.println "};"
  IO.println ""
  IO.println "// saturatedTailCpp numerator denominator; defined = false to none (UB w C++)."
  IO.println "struct CppSaturatedTailRow {"
  IO.println "  __int128 numerator, denominator;"
  IO.println "  bool defined;"
  IO.println "  std::int64_t tail;"
  IO.println "};"
  IO.println ""
  IO.println "// divCpp, modCpp, subtractCpp, addCpp, hashLeftCountCpp i hashCpp dla (a, b, i)."
  IO.println "struct CppReadRow {"
  IO.println "  int a1, a2, b1, b2, i;"
  IO.println "  std::int64_t div, mod, subtract, add, leftCount;"
  IO.println "  bool fromB;"
  IO.println "  int pos;"
  IO.println "};"
  IO.println ""
  IO.println "// addStartupLatencyCpp, subtractStartupLatencyCpp i nthetaStartupLatencyCpp dla (s, t, W)."
  IO.println "struct CppPairTailRow {"
  IO.println "  int s1, s2, t1, t2, w;"
  IO.println "  std::int64_t add, subtract, ntheta;"
  IO.println "};"
  IO.println ""
  IO.println "// hashStartupLatencyCpp a b WA WB."
  IO.println "struct CppHashTailRow {"
  IO.println "  int a1, a2, b1, b2, wa, wb;"
  IO.println "  std::int64_t tail;"
  IO.println "};"
  IO.println ""
  IO.println "// thetaStartupLatencyCpp s t o W."
  IO.println "struct CppThetaTailRow {"
  IO.println "  int s1, s2, t1, t2, o1, o2, w;"
  IO.println "  std::int64_t tail;"
  IO.println "};"
  IO.println ""
  IO.println "// phaseStartupLatencyCpp k r W."
  IO.println "struct CppPhaseTailRow {"
  IO.println "  int k1, k2, r1, r2, w;"
  IO.println "  std::int64_t tail;"
  IO.println "};"
  IO.println ""
  IO.println "// agseStartupLatencyCpp F h W."
  IO.println "struct CppAgseRow {"
  IO.println "  int f, h, w;"
  IO.println "  std::int64_t tail;"
  IO.println "};"
  IO.println ""
  IO.println "// agseLogicalOriginCpp F h length Os."
  IO.println "struct CppAgseOriginRow {"
  IO.println "  int f, h, length, os;"
  IO.println "  std::int64_t origin;"
  IO.println "};"
  IO.println ""
  emitTable "InterleaveRow kInterleaveRows" interleaveRows
  emitTable "SubtractRow kSubtractRows" subtractRows
  emitTable "TailRow kTailRows" tailRows
  emitTable "AffineReadRow kAffineReadRows" affineReadRows
  emitTable "AffineTailRow kAffineTailRows" affineTailRows
  emitTable "AgseOriginRow kAgseOriginRows" agseOriginRows
  emitTable "CppDivisionRow kCppDivisionRows" cppDivisionRows
  emitTable "CppSaturatedIndexRow kCppSaturatedIndexRows" (← IO.ofExcept cppSaturatedIndexRows)
  emitTable "CppSaturatedTailRow kCppSaturatedTailRows" cppSaturatedTailRows
  emitTable "CppReadRow kCppReadRows" (← IO.ofExcept cppReadRows)
  emitTable "CppPairTailRow kCppPairTailRows" (← IO.ofExcept cppPairTailRows)
  emitTable "CppHashTailRow kCppHashTailRows" (← IO.ofExcept cppHashTailRows)
  emitTable "CppThetaTailRow kCppThetaTailRows" (← IO.ofExcept cppThetaTailRows)
  emitTable "CppPhaseTailRow kCppPhaseTailRows" (← IO.ofExcept cppPhaseTailRows)
  emitTable "CppAgseRow kCppAgseRows" (← IO.ofExcept cppAgseRows)
  emitTable "CppAgseOriginRow kCppAgseOriginRows" (← IO.ofExcept cppAgseOriginRows)
