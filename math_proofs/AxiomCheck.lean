import Profs

/- Sprawdzenie aksjomatow (#634, krok A4): kazde twierdzenie z modulow Profs.* moze zalezec
   najwyzej od propext, Classical.choice i Quot.sound - tych samych, ktore wypisuje
   #print axioms dla zwyklego dowodu w Mathlib. Inny aksjomat (sorryAx, wlasne `axiom`,
   Lean.ofReduceBool z native_decide) jest bledem pliku, wiec render.sh konczy sie wtedy
   komunikatem DOWODY: BLAD. Uruchamia go render.sh przez `lake env lean AxiomCheck.lean`.

   Zero sprawdzonych twierdzen tez jest bledem: oznacza, ze filtr modulow przestal cokolwiek
   wybierac, a nie, ze dowody sa czyste. -/

open Lean Elab Command

#eval show CommandElabM Unit from do
  let env ← getEnv
  let allowed : List Name := [``propext, ``Classical.choice, ``Quot.sound]
  let mut checked : Nat := 0
  let mut bad : Array String := #[]
  for (name, ci) in env.constants.map₁.toList do
    let .thmInfo _ := ci | continue
    let some idx := env.getModuleIdxFor? name | continue
    let mod := env.header.moduleNames[idx.toNat]!
    unless (`Profs).isPrefixOf mod do continue
    if name.isInternal then continue
    checked := checked + 1
    let axioms ← liftCoreM <| collectAxioms name
    let extra := axioms.filter (fun a => !allowed.contains a)
    unless extra.isEmpty do bad := bad.push s!"{mod}: {name} zalezy od {extra}"
  if checked = 0 then
    logError "AKSJOMATY: nie znaleziono zadnego twierdzenia w modulach Profs.*"
  for b in bad do
    logError s!"AKSJOMATY: {b}"
  if bad.isEmpty && checked > 0 then
    logInfo s!"AKSJOMATY: OK - {checked} twierdzen Profs.* zalezy najwyzej od propext, Classical.choice i Quot.sound"
