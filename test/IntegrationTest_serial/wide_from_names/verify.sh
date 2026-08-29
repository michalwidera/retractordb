#!/usr/bin/env bash
set -eu

# Sufit NAME_MAX na nazwie substratu — wytworzenie i redukcja szerokiej klauzuli FROM.
#
# Nazwa substratu jest zarazem nazwa pliku, a rosnie LINIOWO z arnoscia FROM: kazdy poziom
# doklada operator, podkreslenie i nazwe operandu. Przy 14 skladnikach nazwa czytelna
# przekracza NAME_MAX i plan staje sie niezapisywalny — obejsciem bylo reczne rozbicie na
# zapytania pomocnicze. compiler::composeStreamName() zastepuje ja skrotem po przekroczeniu
# progu 200 bajtow.
#
# Ten plan celowo stoi OKRAKIEM na progu: wezel 12-skladnikowy ma 192 bajty i zostaje
# czytelny, 13-skladnikowy przekracza prog i dostaje skrot. Jeden przebieg pokrywa wiec obie
# galezie, a granica jest przypieta liczbowo.

rm -f ./*.desc ./*.meta ./*.shadow ./STREAM_ADD_* ./STREAM_AGSE_* wide reduced out_compile.txt

xretractor query.rql -c > out_compile.txt

# Galaz czytelna. 12 skladnikow, 192 bajty — ponizej progu, wiec nazwa jak dotad.
readable='STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_STREAM_ADD_str01_str02_str03_str04_str05_str06_str07_str08_str09_str10_str11_str12'
[ "${#readable}" = 192 ]
grep -F "${readable}(1/1)" out_compile.txt

# Galaz skrotu. 13 skladnikow — nazwa czytelna mialaby 209 bajtow.
#
# Wartosc skrotu jest PRZYPIETA, nie tylko jego ksztalt: skrot trafia na dysk jako nazwa
# pliku, wiec musi byc stabilny miedzy wersjami biblioteki standardowej i miedzy platformami.
# Stad wlasne FNV-1a w compiler.cpp zamiast std::hash. Zmiana tej wartosci jest zmiana
# tozsamosci artefaktu i ma byc widoczna w tescie, a nie dopiero na dysku uzytkownika.
digest='STREAM_ADD_x92741c15f69ba93f'
grep -F "${digest}(1/1)" out_compile.txt

# Redukcja planu: wezel skrotu ma byc FAKTYCZNIE uzyty, a nie osierocony obok drugiej kopii.
grep -F ":- PUSH_STREAM(${digest})" out_compile.txt
[ "$(grep -c "^${digest}(" out_compile.txt)" = 1 ]

# Reduktor nad dlugim artefaktem. `wide` nie przekracza progu, wiec jego wezel okna
# zostaje czytelny — sprawdzamy, ze prog nie zadziala tam, gdzie nie musi.
grep -F 'STREAM_AGSE_1_3_wide(1/14)' out_compile.txt
grep -F ':- PUSH_STREAM(STREAM_AGSE_1_3_wide)' out_compile.txt
grep -F ':- STREAM_SUM' out_compile.txt

xretractor query.rql -r -k -m 60

# Wytworzenie artefaktu. SUBSTRAT 'default', wiec wezel skrotu ma na dysku komplet:
# dane, deskryptor, metadane i cien. Cien bywa pusty (brak luk do odnotowania), wiec
# niepustosci zadamy tylko tam, gdzie tresc jest obowiazkowa.
for suffix in '' .desc .meta; do
  [ -s "${digest}${suffix}" ] || {
    echo "brak artefaktu ${digest}${suffix}"
    exit 1
  }
done
[ -f "${digest}.shadow" ] || {
  echo "brak artefaktu ${digest}.shadow"
  exit 1
}

# Wlasciwa teza: ZADNA nazwa pliku nie przekracza NAME_MAX. Bez skrotu wezel 13-skladnikowy
# dawalby 209 bajtow nazwy plus sufiks — czyli plan, ktorego nie da sie zapisac.
too_long=$(find . -maxdepth 1 -type f -printf '%f\n' | awk 'length($0) > 255')
[ -z "$too_long" ] || {
  echo "nazwa pliku przekracza NAME_MAX:"
  echo "$too_long"
  exit 1
}

# Poprawnosc redukcji. Strumien str0k emituje stala k, wiec `wide` to rekord (1..14), a `@`
# czyta go jako plaski ciag wartosci v[i] = (i mod 14) + 1. SUMC nad oknem 3 daje wiec
# v[i] + v[i+1] + v[i+2] — z zawinieciem na granicy rekordu (…,39,28,17,6,…), ktore jest
# jedyna nietrywialna czescia wyroczni.
#
# Pole `sum` jest typu RATIONAL, czyli para (licznik, mianownik): bierzemy co drugie slowo
# i osobno zadamy mianownika 1.
record_count=$(($(stat -c %s reduced) / 8))
[ "$record_count" -gt 20 ]

actual=$(od -An -v -td4 reduced | awk '{ for (i = 1; i <= NF; i += 2) printf "%s ", $i }')
denominators=$(od -An -v -td4 reduced | awk '{ for (i = 2; i <= NF; i += 2) printf "%s ", $i }')
expected=$(awk -v n="$record_count" 'BEGIN {
  for (i = 0; i < n; i++) {
    total = 0
    for (j = 0; j < 3; j++) total += ((i + j) % 14) + 1
    printf "%s ", total
  }
}')
ones=$(awk -v n="$record_count" 'BEGIN { for (i = 0; i < n; i++) printf "1 " }')

[ "$actual" = "$expected" ] || {
  echo "reduced payload mismatch"
  echo "expected: $expected"
  echo "actual:   $actual"
  exit 1
}
[ "$denominators" = "$ones" ] || {
  echo "reduced denominators are not 1: $denominators"
  exit 1
}

echo OK
