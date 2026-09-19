#!/bin/bash
# KSZTALT: `-c` odrzuca odwolanie do strumienia spoza FROM i przyjmuje postac poprawna.
#
# Asercje dotycza wylacznie tego, CZY plan sie kompiluje, co mowi komunikat i jaki offset
# dostaje poprawne odwolanie. Wartosci liczy value.sh (regula przy issue202_hash_shift_e2e
# w CLAUDE.md).
set -e
. "$(dirname "$0")/../portable.sh"
mkdir -p temp

# Kompilacja ma SIE NIE UDAC, kanalem `Check result:` (protocol_error), bez planu.
#
# Kod bierzemy z nazwy stalej, a nie z liczby: silnik zwraca
# boost::system::errc::protocol_error, czyli EPROTO, a to jest 71 na Linuksie
# i 100 na macOS.
expected_rc=$(errno_value EPROTO)
for f in reject reject_core1; do
  rc=0
  xretractor $f.rql -c > $f.txt 2>&1 || rc=$?
  if [ "$rc" != "$expected_rc" ]; then
    echo "REGRESJA: $f.rql: oczekiwany kod $expected_rc, jest $rc; wyjscie:"
    cat $f.txt
    exit 1
  fi
  grep -F 'Check result:' $f.txt
  grep -F 'not in its FROM clause' $f.txt
  if grep -F 'PUSH_ID(result' $f.txt; then
    echo "REGRESJA: $f.rql: odrzucony plan zostal wydrukowany"
    exit 1
  fi
done

# Pierwsze bledne odwolanie w reject.rql to `core0[0]`; w reject_core1.rql - `core1[0]`,
# ktorego cichy offset 0 dawal zla wartosc.
grep -F "Stream 'result' refers to 'core0'" reject.txt
grep -F "Stream 'result' refers to 'core1'" reject_core1.txt

# Postac poprawna: `merged[2]` zostaje pozycja 2 bufora wejsciowego `result`.
xretractor query.rql -c > plan.txt
grep -F 'PUSH_ID(result[1])' plan.txt
grep -F 'PUSH_ID(result[2])' plan.txt
