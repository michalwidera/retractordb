#!/bin/sh
# systemd wywoluje ExecStopPost po zakonczeniu procesow jednostki, takze przed
# restartem. EXIT_CODE, EXIT_STATUS i SERVICE_RESULT pochodza od managera, nie od
# xretractora. SIGABRT nie uruchamia atexit, a SIGKILL od OOM killera nie daje
# procesowi zadnej szansy: plan nie moze wracac po tych awariach (#475).
case "${SERVICE_RESULT:-}:${EXIT_CODE:-}:${EXIT_STATUS:-}" in
  oom-kill:*) reason='OOM kill' ;;
  *:killed:ABRT|*:dumped:ABRT) reason='SIGABRT' ;;
  *) exit 0 ;;
esac

if [ "$#" -ne 1 ] || [ -z "$1" ]; then
  echo "xretractor-stop-post: cannot clear plan after $reason: missing query file argument" >&2
  exit 1
fi
query_file=$1

# Plik obok planu pozwala na atomowe rename, tak samo jak writeQueryFile w silniku.
# Nie otwieramy starego inode do zapisu (moze byc dowiazaniem). noclobber tworzy plik
# wylacznie nowy (O_EXCL), z prawami wedlug umask, jak przy zapisie przez silnik.
tmp_file="${query_file}.abort.$$"
if ! (set -C && : > "$tmp_file") 2>/dev/null; then
  echo "xretractor-stop-post: cannot clear plan after $reason: $query_file" >&2
  exit 1
fi
trap 'rm -f -- "$tmp_file"' 0
trap 'exit 1' HUP INT TERM

# -T odmawia zamiany katalogu: zwykle mv przeniosloby plik DO katalogu i
# zglosilo sukces mimo nieoproznionego planu. Pomocnik jest tylko dla Linuksa.
if ! mv -fT -- "$tmp_file" "$query_file"; then
  echo "xretractor-stop-post: cannot clear plan after $reason: $query_file" >&2
  exit 1
fi
echo "xretractor-stop-post: cleared plan after $reason: $query_file" >&2
