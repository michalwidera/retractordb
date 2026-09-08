#!/bin/bash
# Klient `xqry -s` nie ma prawa zgubic PIERWSZEGO wiersza strumienia.
#
# Bramka --xqrywait (-x) wstrzymuje przetwarzanie do pierwszej komendy, ale komenda,
# ktora ja zdejmuje, i komenda, ktora zaklada kolejke odpowiedzi klienta, to byly dwie
# rozne komendy. `xqry -s` wysylal najpierw 'get' (walidacja nazwy strumienia), a
# subskrybowal dopiero pozniejszym 'show' -- a kolejke odpowiedzi tworzy serwer wlasnie
# w handlerze 'show'. Miedzy jedna a druga serwer juz liczyl i emitowal do nikogo.
# Przy takcie 1/8 s kazde 125 ms tej przerwy to jeden wiersz przepadly bezpowrotnie.
# Lokalnie przerwa byla rzedu milisekund, na obciazonym kontenerze CI przekroczyla takt:
# 2026-09-08 it_null_divide_by_zero dostal wiersze 2-4 (null, 20, null) zamiast 1-3.
#
# Naprawa ma dwa czlony i kazdy z nich lamie sie osobno, wiec kazdy ma tu wlasna czesc:
#
#  A. Klient subskrybuje PIERWSZA komenda -- 'show' idzie przed 'get' (qry.cpp).
#  B. Bramke zdejmuje komenda OBSLUZONA, nie odebrana (ipcServer.cpp). Sam reorder nie
#     wystarcza: sygnal wysylany na odbiorze wpuszczal petle przetwarzania, zanim handler
#     'show' zdazyl zarejestrowac subskrybenta.
#
# Zaden z tych wyscigow nie da sie zamowic zegarem -- oba rozstrzygaja sie na zajeciu
# muteksu -- wiec kazda czesc rozciaga swoje okno hakiem diagnostycznym, ta sama droga co
# RDB_FAULT_PLAN_SWAP_DELAY w it_service_reset_double. Sonda czasowa bylaby tu bezwartosciowa:
# mierzylaby, czy okno zdazylo sie zamknac, a nie czy go nie ma.
#
# UWAGA na to, czego ten test NIE bada. Bramke zdejmuje nadal KAZDA komenda, takze cudza
# ('hello' z sasiedniego `xqry -l`) -- to zapisany kontrakt bramki, ktorego pilnuje
# it_xqrywait_gate. Serwer, ktoremu bramke zdjal kto inny, liczy bez subskrybenta i jego
# wiersze przepadaja tak samo jak przedtem. Naprawa dotyczy przypadku, w ktorym klient
# jest pierwszy -- czyli wszystkich czternastu testow `-x` + ONESHOT w tym drzewie.
set -e
. "$(dirname "$0")/../serverlib.sh"

# 400 ms to ponad trzy takty 1/8 s: okno jest wtedy szersze od calego wyjscia, wiec
# roznica miedzy naprawa a jej brakiem nie zalezy od obciazenia maszyny.
export RDB_FAULT_GET_AWAIT_EPOCH_SWAP=400

rm -rf temp
mkdir -p temp
server_start query.rql -k -x
xqry -s dst -k -m 3 > out_client_first.txt
server_wait_exit
unset RDB_FAULT_GET_AWAIT_EPOCH_SWAP
bash ../compare.sh --ignore-eol first_row.pattern out_client_first.txt

export RDB_FAULT_SHOW_DELAY=400

rm -rf temp
mkdir -p temp
server_start query.rql -k -x
xqry -s dst -k -m 3 > out_gate_latch.txt
server_wait_exit
unset RDB_FAULT_SHOW_DELAY
bash ../compare.sh --ignore-eol first_row.pattern out_gate_latch.txt
