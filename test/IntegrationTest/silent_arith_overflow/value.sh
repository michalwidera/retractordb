#!/bin/bash
# WARTOSC: przepelnienie INTEGER i RATIONAL daje NULL w polu, a wartosc na granicy liczy sie dokladnie.
#
# Przebieg OFFLINE (`-f -u`): dwa wiersze danych to dwa rekordy, niezaleznie od zegara.
# Wartosci czytamy z ARTEFAKTOW przez `xtrdb list`, ktore wypisuje NULL jako `null`.
# Do 2026-09-14 kazde `null` ponizej bylo zawinieta liczba (np. 410065408/1, -1589934592).
set -e
mkdir -p temp
rm -f temp/*

xretractor query.rql -f -k -u

check() {
  local stream=$1 records=$2 expected=$3
  local lines
  # `xtrdb` otwiera artefakt z katalogu biezacego; spoza `temp` czeka w nieskonczonosc na wejscie.
  lines=$(cd temp && printf 'open %s\nlist 10\nquit\n' "$stream" | timeout 10 xtrdb -n 2>&1 | grep -F '{' || true)
  local count
  count=$(printf '%s\n' "$lines" | grep -c -F '{' || true)
  test "$count" = "$records" || {
    echo "REGRESJA: '$stream' ma $count rekordow, oczekiwane $records"
    printf '%s\n' "$lines"
    exit 1
  }
  while IFS= read -r line; do
    test "$line" = "$expected" || {
      echo "REGRESJA: zle wartosci '$stream'"
      echo "oczekiwane: $expected"
      echo "jest:       $line"
      exit 1
    }
  done <<< "$lines"
}

# Kontrola: suma 7 + 2 bez przepelnienia.
check t 2 '{ t_0:9/1 }'

# RATIONAL: 9*2e8 i 9+2147483638 miesza sie w int32, 9*1e9 i 9+2147483639 juz nie.
# 9/1e9/2 = 9/2000000000 miesci sie; 9/1e9/7 = 9/7000000000 ma mianownik poza int32.
check v 2 '{ v_0:1800000000/1 v_1:null v_2:2147483647/1 v_3:null v_4:9/2000000000 v_5:null }'

# INTEGER: 7*3e8 i 7+2147483640 na granicy INT_MAX, o krok dalej NULL.
check iv 2 '{ iv_0:2100000000 iv_1:null iv_2:2147483647 iv_3:null }'

# INTEGER: -2147483646-2 to INT_MIN, -2147483646-3 juz nie.
check ilo 2 '{ ilo_0:-2147483648 ilo_1:null }'

# Reduktory po polach rekordu: 2e9 + 2e9 przepelnia sume, wiec i srednia.
check bs 2 '{ sum:null }'
check ba 2 '{ avg:null }'

# Agregaty okna po dwoch rekordach: suma i srednia NULL, MAX nie liczy arytmetyki i zostaje dokladny.
check bw 1 '{ bw_0:null bw_1:null bw_2:2000000000/1 }'
