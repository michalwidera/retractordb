#!/bin/bash
# Regresja: xqry z zadeklarowanym budzetem elementow (-m N) musi odebrac caly budzet
# niezaleznie od tego, czy na stdin siedzi terminal z bajtem w buforze.
#
# Bez tej odpornosci `_kbhit()` bral taki bajt za klawisz operatora i konczyl petle odczytu
# w pierwszym obrocie -- zanim watek producenta zdazyl raz sprobowac otworzyc swoja kolejke
# odpowiedzi. Klient wychodzil z zerem elementow i z werdyktem obciazajacym SERWER
# ("server did not create the client response queue"), czyli z diagnoza wskazujaca na
# druga strone IPC. Tak padl it_fncall_runtime_case na CI (2026-09-04); lokalnie nie
# odtwarzalo sie nigdy, bo bez terminala `_kbhit()` wychodzi natychmiast.
#
# Klientowy odpowiednik it_tty_keystroke_immunity, ktore pilnuje tej samej reguly w silniku.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp
rm -f out.txt

server_start query.rql -k -x

status=0
python3 pty_client.py out.txt xqry -s dst -k -m 2 || status=$?

# Werdykt PRZED czekaniem na serwer, a nie po nim. Serwer konczy sie tu komenda 'kill',
# ktora wysyla klient po odebraniu calego budzetu -- klient, ktory polegl wczesniej,
# nie wysyla jej wcale i `server_wait_exit` czekaloby az do limitu czasu testu. Awaria ma
# byc widoczna po sekundzie i z komunikatem, a nie po minucie jako "Timeout".
if [ "$status" -ne 0 ]; then
  echo "FAIL: xqry zakonczyl sie kodem $status; wyjscie:"
  cat out.txt
  exit 1
fi

# Komplet budzetu, a nie sam brak bledu: skrocony przebieg tez konczyl sie plikiem,
# tyle ze pustym.
for value in 11 22; do
  grep -Fq "$value" out.txt || {
    echo "FAIL: brak wartosci $value w wyjsciu klienta; wyjscie:"
    cat out.txt
    exit 1
  }
done

server_wait_exit
