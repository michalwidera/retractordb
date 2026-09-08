#!/usr/bin/env bash
# Czesc WYNIKOWA (Val) tozsamosci (A > i) # (B > k) -> (A # B) > (i + k), gdy
# i*delta(A) == k*delta(B). Biegnie w KAZDEJ konfiguracji przelacznikow, takze
# przy RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=OFF — asercje ksztaltu planu, ktore
# bez tego przebiegu nie maja szans przejsc, stoja osobno w verify_shape.sh.
#
# Podzial powstal 2026-09-06. Do tego dnia jedno `cmp matched CC` pinowalo naraz
# Val i Lat, wiec caly test musial byc wylaczany w ablacji, a razem z nim
# znikalo jedyne miejsce e2e, w ktorym widac rozjazd ogona: przy R1 OFF
# `matched` deklaruje ogon 2, a `CC` ogon 0, wiec przy budzecie -m 48 `matched`
# konczy sie o dwa rekordy wczesniej. Kontrakt def:observable na to pozwala
# (Val rowne, Lat tylko nierosnace), wiec asercje sa dobrane tak, zeby te dwie
# rzeczy rozdzielic:
#   - wartosci obu stron sprawdza NIEZALEZNY oracle formulowy, a nie porownanie
#     dwoch zapisow silnika ze soba (decyzja D-A1: oba sa przedmiotem sporu),
#   - rownosc obu stron obowiazuje tylko na WSPOLNYM PREFIKSIE i wynika z
#     przylozenia oracle'a do obu, nie z osobnego `cmp` (powod nizej),
#   - dlugosci wiaze nierownosc, nie rownosc.
set -eu

rm -f ./*.meta ./*.desc matched CC out_compile.txt

xretractor query.rql -c > out_compile.txt

# Flaga -f (--no-clock) zdejmuje czekanie na zegar scienny; os czasu planu,
# wyrownanie slotow i ogon zostaja bez zmian, wiec artefakt jest bajtowo ten sam.
# Rownosc obu sciezek pilnuje it_noclock_offline. UWAGA: w trybie -c litera -f
# znaczy 'fields' w wyjsciu DOT — do wywolan kompilacyjnych jej NIE dodawac.
xretractor query.rql -r -k -m 48 -f

# Formula-derived payload for delta(A)=1/10 and delta(B)=1/5:
# A#B has the repeating order B,A,A. The equivalent shift of 2+1=3 output slots
# is carried by tail plus origin, so no placeholder records precede the data —
# the first stored record is interleave element 0: B[0],A[0],A[1],B[1],...
#
# Wzor jest funkcja NUMERU elementu przeplotu, a nie dlugosci przebiegu, wiec
# przyklada sie do kazdej ze stron z jej wlasna liczba rekordow — o to wlasnie
# chodzi w oracle'u niezaleznym od ogona.
check_payload() {
  local stream="$1"
  local actual expected
  actual=$(od -An -v -td4 "$stream" | xargs)
  expected=$(
    record_count=$(($(stat -c %s "$stream") / 4))
    for element in $(seq 0 $((record_count - 1))); do
      cycle=$((element / 3))
      case $((element % 3)) in
        0) echo $((100 * (cycle + 1))) ;;
        1) echo $((2 * cycle + 1)) ;;
        2) echo $((2 * cycle + 2)) ;;
      esac
    done | xargs
  )
  [ "$actual" = "$expected" ] || {
    echo "$stream payload mismatch"
    echo "expected: $expected"
    echo "actual:   $actual"
    exit 1
  }
}

check_payload matched
check_payload CC

matched_size=$(stat -c %s matched)
cc_size=$(stat -c %s CC)

# Prog przeciw zielonemu wynikowi na pustce: przy budzecie -m 48 najkrotsza
# znana konfiguracja (wszystkie przelaczniki OFF) daje 30 rekordow po 4 bajty.
[ "$matched_size" -ge 120 ] && [ "$cc_size" -ge 120 ] || {
  echo "przebieg za krotki: matched=$matched_size CC=$cc_size (min 120 B)"
  exit 1
}

# Val: rownosci obu stron na wspolnym prefiksie NIE pilnuje tu osobny `cmp` i
# jest to swiadome. Ten sam oracle przylozony do obu artefaktow juz ja wymusza:
# skoro kazda strona zgadza sie ze wzorem dla WLASNEJ liczby rekordow, ich
# wspolny prefiks jest bajtowo rowny z definicji. Sprawdzone mutacja —
# przekrecenie bajtu w CC oblewa `check_payload CC`, wiec `cmp` prefiksu nie
# mialby jak oblac, a asercja, ktora nie moze oblac, jest w tym drzewie
# defektem aparatury, nie zabezpieczeniem.

# Lat: strona sfaktoryzowana nie moze czekac DLUZEJ, czyli `matched` nie moze
# wydac WIECEJ rekordow niz `CC`. Przy R1 ON zachodzi rownosc, przy R1 OFF
# nierownosc ostra (30 wobec 32) i to jest przypadek przewidziany przez
# thm:shift-match, nie defekt.
[ "$matched_size" -le "$cc_size" ] || {
  echo "matched wydal wiecej rekordow niz CC: $matched_size > $cc_size"
  exit 1
}

echo OK
