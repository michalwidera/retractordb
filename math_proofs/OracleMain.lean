import Profs

/- Wyrocznia dla testow silnika RetractorDB. Tablice sa liczone definicjami, o ktorych
   mowia dowody w Profs, i wypisywane jako inicjalizatory C++. Plik wynikowy tworzy
   gen-oracle.sh; nie edytowac go recznie. -/

open Profs

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

def emitTable (decl : String) (rows : List String) : IO Unit := do
  IO.println s!"inline constexpr {decl}[] = \{"
  for row in rows do
    IO.println row
  IO.println "};"
  IO.println ""

def main : IO Unit := do
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
  emitTable "InterleaveRow kInterleaveRows" interleaveRows
  emitTable "SubtractRow kSubtractRows" subtractRows
  emitTable "TailRow kTailRows" tailRows
  emitTable "AffineReadRow kAffineReadRows" affineReadRows
  emitTable "AffineTailRow kAffineTailRows" affineTailRows
  emitTable "AgseOriginRow kAgseOriginRows" agseOriginRows
