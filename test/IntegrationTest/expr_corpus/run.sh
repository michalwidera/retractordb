#!/bin/bash
# Korpus wyrazen arytmetycznych (#289). Kazdy plan liczony offline jednym przejsciem przez dane,
# a wynik sprawdzany dwa razy: wobec wzorca `<plan>.pattern` (compare.sh - kazda zmiana wyniku)
# i wobec wyroczni niezaleznej oracle.py (czy wynik jest POPRAWNY wedlug modelu jezyka).
#
# Wypis `<plan>.out`: rekordy kazdego strumienia SELECT przez `xtrdb list`, typy pol z `.desc`
# i lista plikow zrzutu - reguly korpusu nie maja prawa odpalic.
set -e
. "$(dirname "$0")/../portable.sh"
export LC_ALL=C

status=0
for rql in *.rql; do
  plan=${rql%.rql}
  rm -rf temp
  mkdir temp
  # `-f -u`: offline do wyczerpania zrodel - N wierszy danych to N rekordow.
  xretractor "$rql" -k -f -u > /dev/null
  {
    for stream in $(sed -n 's/^SELECT .* STREAM \([A-Za-z_0-9]*\) FROM .*/\1/p' "$rql"); do
      echo "== $stream"
      # `xtrdb` otwiera artefakt z katalogu biezacego; spoza `temp` czeka na wejscie.
      # Znak NaN zalezy od platformy, nie od jezyka (#320): `sqrt(-x)` na x86-64 daje bity
      # fff8... i glibc pisze `-nan`, aarch64 daje 7ff8... i `nan`, libc macOS pisze `nan`
      # zawsze. Wyrocznia znak pomija; wzorzec trzyma postac bez znaku.
      # Zapis wykladniczy tez bywa platformowy: `os << double` to `%g` z precyzja 6, ktory zera
      # koncowe mantysy obcina, a biblioteka standardowa macOS (Apple clang 21, macOS 27) wypisala
      # jedna wartosc rehost_double jako `5.30770e+11` zamiast `5.3077e+11`. Liczba jest ta sama -
      # wyrocznia ja przyjmuje - rozni sie tylko zapis, wiec wzorzec trzyma postac kanoniczna. Na
      # Linuksie obie zamiany nie maja czego zmienic: poprawne `%g` takich zer nie zostawia.
      (cd temp && printf 'open %s\nlist 1000\nquit\n' "$stream" | run_timeout 10 xtrdb -n 2>&1 | grep -F '{' |
        sed -E -e 's/:-nan /:nan /g' -e 's/([0-9]\.[0-9]*[1-9])0+e/\1e/g' -e 's/([0-9])\.0+e/\1e/g' || true)
      echo "== desc $stream"
      cat "temp/$stream.desc"
      echo  # .desc nie konczy sie znakiem nowej linii
    done
    echo "== dumps"
    (cd temp && find . -name '*_dump*' | sort)
  } > "$plan.out"
  bash ../compare.sh --ignore-eol "$plan.pattern" "$plan.out" || status=1
  python3 oracle.py "$rql" "$plan.out" || status=1
done
exit $status
