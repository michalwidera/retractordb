#!/usr/bin/env bash
# Stabilnosc odtworzenia: dwa przebiegi tej samej binarki na tych samych danych
# musza dac identyczne artefakty. Porownanie obejmuje dane, deskryptory (.desc),
# metadane (.meta) i cienie (.shadow).
#
# Z porownania .meta wylaczony jest WYLACZNIE 8-bajtowy naglowek — ta sama granica,
# ktora stosuje kampania K18 i pozostale testy bajtowe w tym katalogu (tail -c +9).
# Do 2026-09-02 naglowek niosl znacznik utworzenia i bylo to konieczne; dzis jest
# zarezerwowany (zera), wiec pominiecie jest juz tylko zaszloscia.
#
# Test jest napisany tak, zeby NIE MOGL przejsc pusty: sprawdza liczbe
# porownanych plikow, identycznosc zbioru nazw i niezerowa dlugosc kazdego
# strumienia danych. Bez tych trzech kontroli przebieg, ktory nie wyprodukowal
# niczego, przechodzilby jako "brak roznic".
set -eu

readonly kBudget=60
readonly kStreams="proj win red shifted merged inter recovered_a recovered_b slower"
readonly kMinimumFiles=36

rm -rf temp run1
mkdir -p temp

# Flaga -f (--no-clock) zdejmuje czekanie na zegar scienny; os czasu planu,
# wyrownanie slotow i ogon zostaja bez zmian, wiec artefakt jest bajtowo ten sam.
# Rownosc obu sciezek pilnuje it_noclock_offline. UWAGA: w trybie -c litera -f
# znaczy 'fields' w wyjsciu DOT — do wywolan kompilacyjnych jej NIE dodawac.
xretractor query.rql -r -k -m "$kBudget" -f > run1.out 2> run1.err
mv temp run1

mkdir -p temp
xretractor query.rql -r -k -m "$kBudget" -f > run2.out 2> run2.err

# --- kontrole niepustosci -----------------------------------------------------------
#
# Kazdy strumien planu musi wydac rekordy w OBU przebiegach. Strumien pusty
# oznaczalby, ze budzet przebiegu nie pokrywa jego origin i ogona, a wtedy
# porownanie nie mowi nic o tej klasie operatora.
for stream in $kStreams; do
  for run in run1 temp; do
    [ -s "$run/$stream" ] || {
      echo "replay: $run/$stream nie zawiera rekordow — budzet przebiegu nie pokrywa tego strumienia"
      exit 1
    }
  done
done

# --- kontrola zbioru artefaktow -----------------------------------------------------
#
# Rozny zbior nazw to takze niestabilnosc, nawet gdyby wspolne pliki byly zgodne.
diff <(cd run1 && ls -1 | sort) <(cd temp && ls -1 | sort) || {
  echo "replay: przebiegi wyprodukowaly rozny zbior artefaktow"
  exit 1
}

# --- porownanie bajtowe -------------------------------------------------------------
compared=0
for path in run1/*; do
  name=$(basename "$path")
  case "$name" in
    *.meta)
      # Naglowek .meta to 8 bajtow zarezerwowanych — jedyna wielkosc, ktorej
      # rownosci nie wymagamy (patrz nota na poczatku pliku).
      cmp <(tail -c +9 "run1/$name") <(tail -c +9 "temp/$name") || {
        echo "replay: rozna mapa null w $name"
        exit 1
      }
      ;;
    *)
      cmp "run1/$name" "temp/$name" || {
        echo "replay: rozna zawartosc $name"
        exit 1
      }
      ;;
  esac
  compared=$((compared + 1))
done

# --- kontrola pokrycia --------------------------------------------------------------
#
# Gdyby plan przestal utrwalac czesc strumieni, petla wyzej porownalaby mniej plikow
# i milczaco przeszla. Prog jest ustawiony na liczbe artefaktow planu, nie na zero.
[ "$compared" -ge "$kMinimumFiles" ] || {
  echo "replay: porownano tylko $compared plikow, oczekiwano co najmniej $kMinimumFiles"
  exit 1
}

echo "OK ($compared plikow)"
