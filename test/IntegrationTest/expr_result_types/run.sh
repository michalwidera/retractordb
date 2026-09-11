#!/bin/bash
# Kontrakt typu wyniku wyrazenia — pozycja 16 w paper-arXiv/usecases/requested.md.
#
# Do 2026-09-11 kompilator wyprowadzal typ pola z czterech regul lokalnych, z ktorych zadna
# nie widziala calego programu ONP. Kazde z piaciu pol `dst` wychodzilo wtedy jako INTEGER —
# a wartosci, ktore silnik do nich zapisywal, calkowite nie byly.
#
# Test sprawdza CZTERY rzeczy naraz, bo kazda z osobna przepuszcza inny blad: nazwy typow
# w `.desc`, rozmiar rekordu i rozklad bajtow na pola, wartosci payloadu (w tym niecalkowite)
# oraz NULL.
set -e
. "$(dirname "$0")/../serverlib.sh"
mkdir -p temp

# --- wartosci: klient czyta to, co silnik naprawde policzyl --------------------------------
server_start query.rql -k -x
xqry -s dst -k -m 3 > dst.txt
server_wait_exit

server_start query.rql -k -x
xqry -s copy -k -m 3 > copy.txt
server_wait_exit

# --- artefakty: przebieg OFFLINE, zeby liczba rekordow byla deterministyczna ---------------
# Serwer czekajacy na klienta liczy sloty do chwili zatrzymania, wiec liczba rekordow
# w artefakcie zalezy od czasu. `--until-eof --no-clock` konczy prace na wyczerpaniu zrodla:
# trzy wiersze `data.txt` to dokladnie trzy rekordy.
xretractor query.rql -k -u -f

# --- 1. Nazwy typow w deskryptorze ---------------------------------------------------------
# Czysty odczyt zachowuje typ producenta; `*3` nad DOUBLE zostaje DOUBLE; `to_float(...)*2`
# jest FLOAT, mimo ze konwersja NIE stoi na koncu programu; `to_double(k)/4` jest DOUBLE;
# `null2zero` przepuszcza typ argumentu.
grep -F 'DOUBLE dst_0' temp/dst.desc
grep -F 'DOUBLE dst_1' temp/dst.desc
grep -F 'FLOAT dst_2' temp/dst.desc
grep -F 'DOUBLE dst_3' temp/dst.desc
grep -F 'DOUBLE dst_4' temp/dst.desc

# `SELECT *` kopiuje ksztalt producenta slot po slocie, a nie wpisuje INTEGER na sztywno.
grep -F 'DOUBLE src_0' temp/copy.desc
grep -F 'INTEGER src_1' temp/copy.desc

# --- 2. Rozmiar rekordu i rozklad bajtow ---------------------------------------------------
# Sama nazwa typu w `.desc` to za malo: DOUBLE zajmuje 8 B zamiast 4 B, wiec zly typ przesuwa
# offsety WSZYSTKICH kolejnych pol rekordu. 8+8+4+8+8 = 36 B.
xtrdb -n -s temp/dst > map_dst.txt
grep -E 'DOUBLE +dst_0 +8 B' map_dst.txt
grep -E 'DOUBLE +dst_1 +8 B' map_dst.txt
grep -E 'FLOAT +dst_2 +4 B' map_dst.txt
grep -E 'DOUBLE +dst_3 +8 B' map_dst.txt
grep -E 'DOUBLE +dst_4 +8 B' map_dst.txt
grep -E 'Record size: +36 B' map_dst.txt
grep -E 'Records: 3' map_dst.txt

xtrdb -n -s temp/copy > map_copy.txt
grep -E 'DOUBLE +src_0 +8 B' map_copy.txt
grep -E 'INTEGER +src_1 +4 B' map_copy.txt
grep -E 'Record size: +12 B' map_copy.txt

# --- 3. Wartosci payloadu, w tym niecalkowite ----------------------------------------------
# 2,5 i -1,25 nie sa calkowite: pole INTEGER pokazaloby tu 2 i -1, a 7,5 wyszloby jako 7.
# 5/4 = 1,25 jest kontrola dla `to_double`: dzielenie calkowite dalo by 1.
# Klient konczy wiersz CR+LF (kompatybilnosc terminalowa), stad [[:space:]]* na koncu wzorca.
grep -E '^2\.5 7\.5 5 1\.25 2\.5[[:space:]]*$' dst.txt
grep -E '^-1\.25 -3\.75 5 1\.25 -1\.25[[:space:]]*$' dst.txt

# --- 4. NULL -------------------------------------------------------------------------------
# Wiersz drugi ma NULL w polu DOUBLE. NULL propaguje przez odczyt i przez mnozenie,
# a `null2zero` zamienia go na zero TEGO SAMEGO pola.
grep -E '^null null 5 1\.25 0[[:space:]]*$' dst.txt
# Metadane odnotowuja rekord CZESCIOWO pusty, a nie rekord zerowy.
grep -F 'some nulls' map_dst.txt

# Kopia calego schematu niesie ten sam NULL.
grep -E '^null 5[[:space:]]*$' copy.txt
