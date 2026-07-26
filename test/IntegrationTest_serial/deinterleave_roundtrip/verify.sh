#!/usr/bin/env bash
# Weryfikacja bitowa tozsamosci okreznej przeplotu/rozplotu.
# Wzorce wyprowadzone z definicji formalnych (nie z implementacji):
#   c  : z = 2/3 -> okres B,A,A; B[k]=1001+k, A[k]=1+k
#   a2 : odzyskuje sa DOKLADNIE, od a_0, bez rekordu-zastepnika
#   b2 : odzyskuje sb DOKLADNIE, od b_0
#
# Zasada brzegu strumienia: opoznienie operatora jest ogonem (query::startupLatency),
# nie rekordem. Theta jest o slot nieprzyczynowa, ale placu na dane nie rezerwuje sie
# ani zerem, ani NULL-em - strumien po prostu nie emituje, dopoki nie ma czego wydac.
# Dlatego a2 zaczyna sie od a_0, a nie od zastepnika.
#
# Dlugosc przebiegu nie jest czescia twierdzenia: liczba rekordow zalezy od budzetu
# -m, wiec kazdy strumien porownujemy z prefiksem jego wzorca o dlugosci faktycznej.
set -e
rm -f ./*.meta ./*.desc ./*.shadow c a2 b2
xretractor query.rql -r -k -m 48

dump() { od -An -v -tu4 "$1" | xargs; }
records() { echo $(($(stat -c %s "$1") / 4)); }

# Prefiks wzorca o dlugosci n rekordow.
head_n() { echo "$1" | tr ' ' '\n' | head -n "$2" | xargs; }

exp_c=$(for i in $(seq 0 199); do
  if [ $((i % 3)) -eq 0 ]; then echo $((1001 + i / 3)); else echo $((1 + 2 * i / 3)); fi
done | xargs)
# Tozsamosc okrezna: rozplot ma oddac dokladnie strumienie zrodlowe.
exp_a2=$(xargs <datafile1.txt)
exp_b2=$(xargs <datafile2.txt)

check() {
  local name=$1 expected=$2
  local n
  n=$(records "$name")
  [ "$n" -gt 0 ] || {
    echo "$name is empty"
    exit 1
  }
  [ "$(dump "$name")" = "$(head_n "$expected" "$n")" ] || {
    echo "$name mismatch"
    echo "expected: $(head_n "$expected" "$n")"
    echo "actual:   $(dump "$name")"
    exit 1
  }
}

check c "$exp_c"
check a2 "$exp_a2"
check b2 "$exp_b2"
echo OK
