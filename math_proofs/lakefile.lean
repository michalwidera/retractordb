import Lake

open Lake DSL

package profs where

require verso from git "https://github.com/leanprover/verso.git" @ "v4.33.0"
require mathlib from git "https://github.com/leanprover-community/mathlib4.git" @ "v4.33.0"

@[default_target]
lean_lib Profs where

lean_lib ProfsManual where

lean_exe profs_manual where
  root := `ManualMain
  supportInterpreter := true

lean_exe profs_oracle where
  root := `OracleMain
