# Korpus VAR uc10-uc16 (#538)

Podstawa: [szkice w #531](https://github.com/michalwidera/retractordb/issues/531#issuecomment-6254884794),
decyzje D1-D9, analiza R1-R7 i rekomendacje S1-S6. To publiczny korpus testow,
oddzielny od `paper-arXiv/usecases` i pracy DEBS. Plany zachowuja obliczenia
ze szkicow, w tym celowe ograniczenie uc16; nie sa rozwinieciem korpusu uc01-uc08.

| Rodzina | Obliczenie | Skalowanie wzgledem szkicu |
|---------|------------|--------------------------|
| uc10 | Histereza sprezarki, cykle, zerowanie godzinne, DUMP | Godzina 3600 -> 40 s; progi 50/20/6 i DUMP -60..0 bez zmiany |
| uc11 | Automat turbiny, przerwana cisza, DUMP burzy | Cisza 600 -> 40 rekordow; DUMP -30..30 bez zmiany |
| uc12 | EMA SpO2 zamrazana podczas desaturacji, licznik i indeks | Bez zmiany taktow, progow i wspolczynnikow |
| uc13 | IIR STA/LTA, zamrozona LTA, histereza i DUMP na zboczu | Bez zmiany: 100 Hz, historia 500 i przyszlosc 1000 rekordow |
| uc14 | Bilans parkingu od startu i korekta z kamer | Doba 86400 -> 40 s; pojemnosc 400 bez zmiany |
| uc15 | Zatrzask awarii, kwitowanie, licznik sztuk i zmiana | Zmiana 28800 -> 10 s; PLC 1/10 s, panel 1 s bez zmiany |
| uc16 | Dwa zapisy TOK w slocie, ostatni wygrywa | Bez zmiany; wynik D3, nie intuicyjny token bucket |

`generate.py` ma osobne stale ziarno dla kazdej rodziny. Szum wypelnia tlo,
a zdarzenia sterujace sa wymuszone. `oracle.py` wymaga wyzwolenia kazdej reguly
SET i DUMP oraz sprawdza konflikty: uc10 dokladnie w slotach 120 i 160,
uc14 co 40 slotow. uc15 obejmuje kwitowanie jednoczesne z pojawieniem sie awarii:
panel widzi poprzedni FAULT_NOW, a pozniejsza w tekscie regula ack wygrywa
z fault_set. Przy kwitowaniu podczas trwajacej awarii stary FAULT_NOW blokuje ack.

```bash
python3 generate.py           # odtwarza *.txt i expected.jsonl, bez silnika
python3 generate.py --check   # kontrola zapisanych danych i wzorcow
ctest --test-dir ../../../build/Debug -R '^it_var_corpus-' -LE valgrind --output-on-failure
```

Kazdy `expected.jsonl` zawiera rekordy `schema` (strumien i typy), `record`
(strumien, indeks od zera, wartosci i maska NULL), `dump` (nazwa pliku i zakres
polotwarty) i `events` (liczba wyzwolen kazdej reguly i sloty konfliktow).
Wzorzec pochodzi z wyroczni, nigdy z uruchomienia silnika. Test najpierw odtwarza
go w pamieci, potem porownuje wszystkie binarne rekordy i metadane z modelem.
FLOAT jest liczony z zaokragleniem do float32 po kazdej operacji; porownanie
wartosci jest dokladne. Bajty pod polem NULL nie zastepuja maski NULL.

Wyrocznia liczy od pierwszego rekordu. Wszystkie odczyty slotu uzywaja starego
stanu (D1), zapisy sa zatwierdzane razem, a konflikty wygrywa ostatnia regula
w tekscie (D3). Granica EOF to koniec pierwszego wyczerpanego zrodla.
Zrzuty sa sprawdzane co do nazwy, liczby rekordow, kolejnosci i wartosci.
Plany bez RETENTION zastepuja poprzedni zrzut tej samej reguly, wiec plik koncowy
odpowiada ostatniemu wyzwoleniu. Wszystkie wyzwolenia maja pelna historie
i wystarczajaca liczbe rekordow do dokonczenia przyszlej czesci DUMP.

`run.sh` liczy kazda rodzine dwa razy, sprawdza oba przebiegi wyrocznia i porownuje
wyjsciowe dane, `.desc`, `.meta` oraz DUMP bajtowo przez `compare.sh`.
Test restartu uc10 uruchamia dwa osobne procesy tego samego planu na jednym
magazynie: drugi startuje na `.desc`, `.meta` i danych pierwszego. Po wlaczeniu
sprezarki restart gubi zatrzask (D6), mimo utrzymywania temperatury w pasmie
histerezy. Stan wraca dopiero od rekordu po kolejnym przekroczeniu progu. Wyrocznia
porownuje ten wynik takze z kontynuacja zachowujaca stan. Silnik zaczyna przy starcie
strumienie pochodne od zera (zachowanie zaobserwowane 2026-10-20, takze bez `-u`),
wiec po restarcie magazyn zawiera wylacznie rekordy drugiego przebiegu; test to
sprawdza, a kopia `restart/before` zachowuje wynik pierwszego.

`BASE` w uc12 oraz `STA` i `LTA` w uc13 maja jawny typ `: FLOAT` (#546). To ten sam
typ, ktory dalby sam literal dziesietny, wiec wzorce sa wspolne z zapisem bez typu.
Zmienne calkowite i stale `LET` zostaja bez adnotacji.

Rejestracja lokalna korzysta ze wspolnego makra CTest i blokady przestrzeni nazw.
Testy dostaja silnik jako `$<TARGET_FILE:xretractor>`, a nie z `PATH`: przebieg pod
Valgrindem i floor ablacji sprawdzaja wtedy binarke swojego katalogu budowy,
niezaleznie od kopii w `~/.local/bin`.
Nazwy `-vg-` uruchamiaja caly scenariusz przez IT_VALGRIND, w tym restart.
Sprawdzenie wartosci pozostaje aktywne przy wszystkich RDB_OPT_* = OFF.
Samotest wyroczni obejmuje uszkodzone dane, maski NULL, deskryptory, brakujace
i dodatkowe rekordy/strumienie/zrzuty oraz recznie policzone granice slotow.

Pomiar uc13 stosuje przepis `comparative-measurement`: aktualny Release-Probe,
jeden katalog, ta sama nazwa programu i planu, pusty magazyn przed kazdym
przebiegiem, Ir tylko w `processRows`, czasy `compute_ns` z sondy. Warianty
uruchamiaja sie naprzemiennie AB/BA; kazdy ma piec przebiegow po 20000 slotow,
z pominieciem pierwszych 1000 slotow w statystyce czasu. Przed pomiarem pelne
wyjscie obu wariantow jest sprawdzane niezaleznym modelem ich obliczen.
Ir po rozgrzewce to roznica prefiksow 3000 i 1000 slotow liczonych od startu,
czyli koszt 2000 kolejnych slotow; surowe liczniki obu prefiksow sa zachowane.
Budzet `-m` obejmuje poczatkowy slot zerowy, ktorego nie ma w CSV: skrypt podaje
N+1 dla N slotow roboczych i sprawdza, ze CSV zawiera dokladnie N wierszy.

```bash
cmake --build ../../../build/Release-Probe --target xretractor
python3 measure_uc13.py ../../../build/Release-Probe/src/retractor/xretractor /tmp/uc13-538-measure
```

Katalog wynikowy musi byc nowy. Zawiera CSV, callgrind, logi i `summary.json`
z rewizja zrodel i SHA-256 programu; nie wchodzi do repozytorium.
`window.rql` liczy AVG w oknach 10/1000 zamiast EMA i nie zamraza LTA, zachowujac
histereze na TRIG i DUMP. To porownanie kosztu dwoch wariantow detektora o roznych
wynikach. Nie dowodzi przewagi ani regresji wydajnosci silnika; taki werdykt nadal
wymaga uc01-uc08. Takie ujecie pomiaru zostalo zaakceptowane przy planie #538.
Okno 1000 rekordow ma 999 rekordow opoznienia startowego: dla 3600 rekordow
wejscia powstaje 2601 rekordow detektora (VAR: 3600). Rozgrzewka usuwa ten
transjent z porownania kosztow; nie usuwa roznic wartosci ani zdarzen DUMP.
