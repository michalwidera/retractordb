#!/usr/bin/env bash
#
# Sciezka zrzutu DO DUMP jest przewidywalna z tekstu planu, wiec ktos moze ja zajac przed
# silnikiem. Pod nazwy obu zrzutow podstawiamy dowiazania do plikow wartowniczych POZA katalogiem
# magazynu: symboliczne (zatrzymaloby je juz O_NOFOLLOW) i twarde (O_NOFOLLOW go nie zatrzymuje,
# O_TRUNC obcialby wspoldzielony i-wezel). Zrzut ma utworzyc wlasny plik i wartownikow nie dotknac.
#
# Na stdout: zawartosc obu zrzutow (4 B na rekord INTEGER), porownywana z pattern.txt.
set -e
export LC_ALL=C

rm -rf temp outside-*.txt
mkdir temp
printf 'wartownik symlink\n' > outside-symlink.txt
printf 'wartownik hardlink\n' > outside-hardlink.txt
cp outside-symlink.txt outside-symlink.orig
cp outside-hardlink.txt outside-hardlink.orig
ln -s ../outside-symlink.txt temp/str1_viasymlink_dump.tmp
ln outside-hardlink.txt temp/str1_viahardlink_dump.tmp

xretractor query.rql -m 10 -f > /dev/null

cmp outside-symlink.orig outside-symlink.txt
cmp outside-hardlink.orig outside-hardlink.txt
for f in temp/str1_viasymlink_dump.tmp temp/str1_viahardlink_dump.tmp; do
  if [ -L "$f" ] || [ ! -f "$f" ]; then
    echo "$f nie jest zwyklym plikiem" >&2
    exit 1
  fi
done
if [ temp/str1_viahardlink_dump.tmp -ef outside-hardlink.txt ]; then
  echo "zrzut nadal wspoldzieli i-wezel z outside-hardlink.txt" >&2
  exit 1
fi

for f in temp/str1_viasymlink_dump.tmp temp/str1_viahardlink_dump.tmp; do
  printf '%s ' "${f##*/}"
  od -An -td4 -w4 -v "$f" | tr -d ' ' | paste -sd' ' -
done
