"""Uruchamia podany program ze stdin podpietym do pseudoterminala, w ktorym CZEKA jeden bajt.

Dokladnie ta sytuacja zdarza sie na CI: krok biegnie na terminalu, a bajt w jego buforze
wejscia wyglada dla `_kbhit()` jak klawisz operatora. Harness nie sprawdza niczego sam --
jedynie odtwarza warunek; werdykt nalezy do verify.sh.
"""

import os
import pty
import subprocess
import sys
import time

master, slave = pty.openpty()
try:
    os.write(master, b"\n")
    time.sleep(0.05)  # bajt ma byc w buforze ZANIM proces zajrzy na terminal
    proc = subprocess.Popen(sys.argv[1:], stdin=slave, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    sys.exit(proc.wait())
finally:
    os.close(slave)
    os.close(master)
