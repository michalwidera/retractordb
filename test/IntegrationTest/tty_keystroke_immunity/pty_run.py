"""Uruchamia podany program ze stdin podpietym do pseudoterminala, w ktorym CZEKA jeden bajt.

Dokladnie ta sytuacja zdarza sie na CI: krok biegnie na terminalu, a bajt w jego buforze
wejscia wyglada dla `_kbhit()` jak klawisz operatora. Harness nie sprawdza niczego sam --
jedynie odtwarza warunek; werdykt nalezy do verify.sh.
"""

import argparse
import os
import pty
import subprocess
import sys
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--timeout", type=float)
parser.add_argument("command", nargs=argparse.REMAINDER)
args = parser.parse_args()
if not args.command:
    parser.error("podaj program do uruchomienia")

master, slave = pty.openpty()
try:
    os.write(master, b"\n")
    time.sleep(0.05)  # bajt ma byc w buforze ZANIM proces zajrzy na terminal
    proc = subprocess.Popen(args.command, stdin=slave, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        status = proc.wait(timeout=args.timeout)
    except subprocess.TimeoutExpired:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        # Takze poprawnie obsluzony SIGTERM oznacza przekroczenie limitu, jak w GNU timeout.
        status = 124
    sys.exit(status)
finally:
    os.close(slave)
    os.close(master)
