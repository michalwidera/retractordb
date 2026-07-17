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
wait_for_worker() {
  local host="$1" timeout_s="${2:-600}" waited=0
  log "Czekam az $host wroci po restarcie (timeout ${timeout_s}s)..."
  while ! ssh -o BatchMode=yes -o ConnectTimeout=5 "$host" true 2>/dev/null; do
    sleep 10
    waited=$((waited + 10))
    [ "$waited" -ge "$timeout_s" ] && die "$host nie odpowiedzial po ${timeout_s}s od restartu"
  done
  log "$host odpowiada po ${waited}s."
  # Daj czas na dojscie sshd/uslug do stabilnego stanu po boot.
  sleep 15
}
