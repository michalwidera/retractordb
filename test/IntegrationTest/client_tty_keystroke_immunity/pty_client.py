"""Uruchamia klienta ze stdin podpietym do pseudoterminala, w ktorym CZEKA jeden bajt.

Dokladnie ta sytuacja zdarza sie na CI: krok biegnie na terminalu, CTest przekazuje go
testom, a bajt w buforze wejscia wyglada dla `_kbhit()` jak klawisz operatora. Odpowiednik
pty_run.py z ../tty_keystroke_immunity, ktory robi to samo dla silnika; roznica jest jedna --
stdout klienta jest PRZEKIEROWANY do pliku podanego w argv[1], bo to jego tresc jest dowodem.
Stderr zostaje odziedziczony, wiec diagnostyka klienta trafia do logu ctest.

Harness nie sprawdza niczego sam -- jedynie odtwarza warunek; werdykt nalezy do run.sh.
"""

import os
import pty
import subprocess
import sys
import time

outPath = sys.argv[1]
master, slave = pty.openpty()
try:
    os.write(master, b"\n")
    time.sleep(0.05)  # bajt ma byc w buforze ZANIM proces zajrzy na terminal
    with open(outPath, "wb") as out:
        proc = subprocess.Popen(sys.argv[2:], stdin=slave, stdout=out)
        sys.exit(proc.wait())
finally:
    os.close(slave)
    os.close(master)
