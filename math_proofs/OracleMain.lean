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
  emitTable "InterleaveRow kInterleaveRows" interleaveRows
  emitTable "SubtractRow kSubtractRows" subtractRows
  emitTable "TailRow kTailRows" tailRows
