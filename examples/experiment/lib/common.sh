#!/bin/bash
# Wspolne funkcje dla start_supervisor.sh i worker/run_study.sh.
# Zrodlo tego pliku, nie uruchamiac bezposrednio.

log()  { printf '[%(%Y-%m-%d %H:%M:%S)T] %s\n' -1 "$*" >&2; }
die()  { log "BLAD: $*"; exit 1; }

# ssh do workera z rozsadnymi timeoutami; propaguje kod wyjscia polecenia.
#
# Nieinteraktywna sesja SSH NIE sourcuje ~/.bashrc (typowy guard "if not
# interactive, return" na jego poczatku) -- PATH do ~/.local/bin (ninja
# install) i aktywacja Python venv (conan) z .bashrc nigdy by sie nie
# zastosowaly. Odtwarzamy je tu jawnie zamiast na tym polegac.
ssh_worker() {
  local host="$1"; shift
  ssh -o BatchMode=yes -o ConnectTimeout=8 -o ServerAliveInterval=5 \
      -o ServerAliveCountMax=3 "$host" \
      "export PATH=\"\$HOME/.local/bin:\$PATH\"; [ -f \"\$HOME/.venv/bin/activate\" ] && source \"\$HOME/.venv/bin/activate\"; $*"
}

# Czeka az worker odpowie po SSH (po reboot). Timeout w sekundach.
#
# Sonda MUSI byc opakowana w 'timeout': ConnectTimeout bounduje wylacznie
# zestawienie TCP, a sesja nawiazana w oknie wczesnego rozruchu potrafi zawisnac
# po autoryzacji i nie wrocic nigdy. Bez tego cialo petli sie nie wykonuje, licznik
# nie rosnie i zadeklarowany timeout NIE wystrzeliwuje -- nadzorca stoi cicho
# w nieskonczonosc (zaobserwowane na czterech restartach z czterech, JOURNAL.md
# 2026-07-21). Z tego samego powodu liczymy czas zegarem, a nie sumowaniem sleepow:
# nieudana iteracja trwa tyle, ile sonda plus sleep, wiec licznik przyrostowy
# zawyzalby faktyczny timeout.
wait_for_worker() {
  local host="$1" timeout_s="${2:-600}"
  local start deadline
  start=$(date +%s)
  deadline=$((start + timeout_s))
  log "Czekam az $host wroci po restarcie (timeout ${timeout_s}s)..."
  while ! timeout 15 ssh -o BatchMode=yes -o ConnectTimeout=5 \
        -o ServerAliveInterval=5 -o ServerAliveCountMax=3 "$host" true 2>/dev/null; do
    [ "$(date +%s)" -ge "$deadline" ] && die "$host nie odpowiedzial po ${timeout_s}s od restartu"
    sleep 10
  done
  log "$host odpowiada po $(( $(date +%s) - start ))s."
  # Daj czas na dojscie sshd/uslug do stabilnego stanu po boot.
  sleep 15
}
