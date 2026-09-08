#!/bin/bash
# Porownanie wyniku testu ze wzorcem, z diagnostyka na porazce.
#
# Zastepuje `cmake -E compare_files`, ktore przy roznicy mowi tylko tyle, ze pliki sie
# roznia. Dokladnie tej informacji zabraklo, gdy it_agse_array oblal na CI 2026-09-04:
# z artefaktu dalo sie wyliczyc, ze test wykonal caly lancuch i przegral na porownaniu,
# ale nie bylo wiadomo NA CZYM. Tutaj roznica idzie na standardowe wyjscie, wiec do
# postawienia diagnozy wystarcza log kroku CI, bez pobierania artefaktow.
#
# Uzycie:
#   bash ../compare.sh [--ignore-eol] <wzorzec> <wynik>
#
# --ignore-eol znaczy to samo, co w `cmake -E compare_files`: pomijane sa roznice
# w zakonczeniu linii. Obejmuje to DWIE rzeczy, nie jedna — CR na koncu linii ORAZ brak
# znaku nowej linii na koncu pliku. Drugie ujawnil od razu it_rotation_test, ktorego
# count.pattern ma jeden bajt "4" bez konca linii, a wynik "4\n": samo
# `diff --strip-trailing-cr` uznaje to za roznice, a compare_files nie. Normalizujemy
# wiec oba pliki i dopiero je porownujemy. Bez tej flagi porownanie jest bajtowe.
set -u

readonly kDiffLineBudget=200

ignoreEol=0
if [ "${1:-}" = "--ignore-eol" ]; then
  ignoreEol=1
  shift
fi

if [ $# -ne 2 ]; then
  echo "compare.sh: uzycie: [--ignore-eol] <wzorzec> <wynik>" >&2
  exit 2
fi

pattern="$1"
actual="$2"
diffLeft="$1"
diffRight="$2"

describe() {
  local f="$1"
  if [ -e "$f" ]; then
    printf '  %-30s %9s bajtow, %6s linii\n' "$f" "$(wc -c <"$f")" "$(wc -l <"$f")"
  else
    printf '  %-30s BRAK PLIKU\n' "$f"
  fi
}

report() {
  echo "=== ROZNICA: $pattern (wzorzec) vs $actual (wynik) ==="
  echo "katalog roboczy: $(pwd)"
  describe "$pattern"
  describe "$actual"
  if [ ! -e "$pattern" ] || [ ! -e "$actual" ]; then
    echo "=== koniec (brak pliku, nie ma czego porownac) ==="
    return
  fi
  echo "--- roznica (- wzorzec, + wynik), do $kDiffLineBudget linii ---"
  # Bez `set -e` w tej sciezce: `diff` z definicji konczy sie niezerowo.
  local out
  out=$(diff -u --label "$pattern" --label "$actual" "$diffLeft" "$diffRight" 2>&1)
  local lines
  lines=$(printf '%s\n' "$out" | wc -l)
  printf '%s\n' "$out" | head -n "$kDiffLineBudget"
  if [ "$lines" -gt "$kDiffLineBudget" ]; then
    echo "--- (obciete: $lines linii roznicy, pokazano $kDiffLineBudget) ---"
  fi
  echo "=== koniec roznicy ==="
}

if [ ! -e "$pattern" ] || [ ! -e "$actual" ]; then
  report
  exit 1
fi

# Rozstrzygniecie i raport pracuja na TYCH SAMYCH danych, zeby nie mogly sie rozjechac:
# porownanie, ktore przechodzi, a potem raport pokazujacy roznice (albo odwrotnie),
# bylby gorszy od braku diagnostyki. Przy --ignore-eol obie strony widza wersje
# znormalizowana: `sub(/\r$/, "")` zdejmuje CR, a `print` dokleja brakujacy koniec
# ostatniej linii.
if [ "$ignoreEol" -eq 1 ]; then
  normalizedPattern=$(mktemp)
  normalizedActual=$(mktemp)
  trap 'rm -f "$normalizedPattern" "$normalizedActual"' EXIT
  awk '{ sub(/\r$/, ""); print }' "$pattern" >"$normalizedPattern"
  awk '{ sub(/\r$/, ""); print }' "$actual" >"$normalizedActual"
  cmp -s "$normalizedPattern" "$normalizedActual" && exit 0
  diffLeft="$normalizedPattern"
  diffRight="$normalizedActual"
else
  cmp -s "$pattern" "$actual" && exit 0
  diffLeft="$pattern"
  diffRight="$actual"
fi

report
exit 1
