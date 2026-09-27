#!/bin/bash
# `xqry --reset` do planu, w ktorym strumien VOLATILE `x` o tej samej nazwie ma WEZSZY rekord, i odczyt
# jego historii zaraz po wymianie. Rekordy `y` maja byc takie same jak w swiezym procesie (pattern.txt),
# a serwer ma dozyc `xqry -k`.
#
# Do 2026-09-27 kubelek memoryStore przezywal wymiane planu: nowy `x` dziedziczyl stare rekordy (64 B)
# i writeCount. Pierscien nowego planu byl przesuniety o odziedziczony licznik, okno `y` czytalo stare
# rekordy do bufora 4 B (valgrind: Invalid write w memoryFile::read), a odczyt poza rozmiarem kubelka
# konczyl serwer FatalError-em - po planie, ktory kanal `reset` przyjal z "OK".
#
# Uzycie: run.sh <xtrdb> [opakowanie...] - opakowanie (valgrind) poprzedza xretractor.
set -e
. "$(dirname "$0")/../serverlib.sh"

xtrdb="$1"
shift
rm -rf temp && mkdir -p temp

# Wlasny start zamiast server_start: pod valgrindem serwer przejmuje blokade po kilku sekundach,
# a server_start czeka najwyzej 10 s. Sciezka blokady i sprzatanie zostaja z serverlib.sh.
"$@" xretractor planA.rql --noanykey </dev/null >server.txt 2>&1 &
_server_pid=$!
_server_started=$_server_pid
i=0
until grep -qx "PID: $_server_pid" "$SERVER_LOCK" 2>/dev/null; do
  if ! kill -0 "$_server_pid" 2>/dev/null; then
    echo "serwer nie wstal"
    cat server.txt
    exit 1
  fi
  if [ "$i" -ge 600 ]; then
    echo "serwer nie przejal blokady $SERVER_LOCK w ciagu 60 s"
    exit 1
  fi
  sleep 0.1
  i=$((i + 1))
done

# Szesc rekordow `x` planu A: kubelek ma stare rekordy, a odziedziczony licznik lezy daleko od
# wielokrotnosci pierscienia planu B (200), wiec odczyt `y` trafia w stare sloty deterministycznie.
xqry -s x -m 6 >out_x.txt

xqry --reset planB.rql
# Wymiana jest asynchroniczna: `--reset` konczy sie na PRZYJECIU planu.
i=0
until xqry --bus 2>/dev/null | grep -qE '\| y *$'; do
  if [ "$i" -ge 300 ]; then
    echo "strumien y nie pojawil sie w planie w ciagu 30 s"
    xqry --bus || true
    cat server.txt
    exit 1
  fi
  sleep 0.1
  i=$((i + 1))
done
# Kod wyjscia `xqry -s` mowi tylko, ze serwer zamilkl - przyczyne (FatalError) niesie log serwera,
# wiec o wyniku rozstrzygaja wiersze, a przy ich braku wypisujemy log.
xqry -s y -m 8 >out_y.txt || true
rows=$(wc -l <out_y.txt)
if [ "$rows" -lt 8 ]; then
  echo "strumien y oddal $rows z 8 wierszy po wymianie planu"
  cat server.txt
  exit 1
fi

xqry -k
if ! server_wait_exit; then
  cat server.txt
  exit 1
fi

"$xtrdb" noprompt <term.script >out.txt
bash ../compare.sh --ignore-eol pattern.txt out.txt
