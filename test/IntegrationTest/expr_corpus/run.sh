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
      (cd temp && printf 'open %s\nlist 1000\nquit\n' "$stream" | run_timeout 10 xtrdb -n 2>&1 | grep -F '{' || true)
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
