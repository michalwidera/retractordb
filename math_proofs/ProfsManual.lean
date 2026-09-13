import VersoManual
import ProfsManual.InterleaveCovering
import ProfsManual.Deinterleave
import ProfsManual.ExactInvertibility
import ProfsManual.EventOrder
import ProfsManual.SumCommutativity
import ProfsManual.ShiftMatching

open Verso.Genre Manual

#doc (Manual) "Dowody RetractorDB w Lean 4" =>

Formalizacje twierdzeń z publikacji i dokumentacji systemu RetractorDB.
Każdy rozdział odpowiada jednemu dowodowi i wskazuje sprawdzany plik Lean.
Wyniki dotyczą modelu matematycznego opisanego w rozdziałach; nie są
dowodem zgodności całej implementacji C++ z tym modelem.

{include 1 ProfsManual.InterleaveCovering}

{include 1 ProfsManual.Deinterleave}

{include 1 ProfsManual.ExactInvertibility}

{include 1 ProfsManual.EventOrder}

{include 1 ProfsManual.SumCommutativity}

{include 1 ProfsManual.ShiftMatching}
