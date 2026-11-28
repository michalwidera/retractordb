#!/usr/bin/env bash
# Straznik wariantu -vg- (makro it_add_vg_test w CMakeLists.txt tego katalogu).
#
# Uzycie: vg_check.sh <katalog logow> <komenda...>
#
# Uruchamia komende scenariusza z podkladkami .vgshim na poczatku PATH, tak ze
# kazdy xretractor, xqry i xtrdb biegnie pod valgrindem i pisze log do
# <katalog logow>/<program>.<pid>.log, a potem ocenia te logi. Sam kod wyjscia
# scenariusza nie wystarcza: skrypt, ktory wola program z `|| true`, sprawdza
# tylko jego wyjscie albo trzyma serwer w tle, przepuscilby blad pamieci po
# cichu. Test pada, gdy:
#   - scenariusz zakonczyl sie bledem (jego kod wyjscia przechodzi dalej);
#   - nie powstal zaden log - wariant nie uruchomil pod valgrindem nic, wiec
#     niczego nie sprawdzil;
#   - log nie ma podsumowania "ERROR SUMMARY" - proces zabity sygnalem albo
#     zastapiony przez exec, wiec kontrola wyciekow w ogole sie nie odbyla;
#   - podsumowanie liczy bledy (z --leak-check=full wycieki "definitely" i
#     "possibly" tez sa liczone jako bledy).
set -u

logdir=$1
shift
rm -rf "$logdir"
mkdir -p "$logdir"
export RDB_VG_LOG_DIR=$logdir

"$@"
rc=$?

shopt -s nullglob
logs=("$logdir"/*.log)
bad=0
if [ ${#logs[@]} -eq 0 ]; then
  echo "vg_check: brak logow valgrinda w $logdir - wariant niczego nie uruchomil pod valgrindem"
  bad=1
fi
for log in "${logs[@]}"; do
  if ! grep -q "ERROR SUMMARY:" "$log"; then
    echo "vg_check: $log bez podsumowania (proces zabity albo exec) - kontrola pamieci sie nie odbyla"
    grep -m1 "Command:" "$log"
    bad=1
  elif grep -q "ERROR SUMMARY: [1-9]" "$log"; then
    echo "vg_check: bledy pamieci w $log"
    cat "$log"
    bad=1
  fi
done
echo "vg_check: logow ${#logs[@]}, kod scenariusza $rc"

if [ "$rc" -ne 0 ]; then
  exit "$rc"
fi
exit "$bad"
