# rec205_h10 — ogon startowy jako składnik budżetu łącza

Aparatura do samodzielnego powtórzenia pomiaru odciążenia łącza dla potoku EKG
z redukcją tempa. Katalog jest samowystarczalny: dane wejściowe bierze przez
ścieżki względne z `../rec205`, więc rekordu 205 (5,2 MB) nie duplikujemy.

## Po co osobny katalog

`../rec205/rec205-qrs.rql` **nie redukuje tempa**: wejście to 2 wartości na
próbkę przy 360 Hz, wyjście `qrs_out` to 3 wartości na próbkę przy tym samym
360 Hz. Ten potok *zwiększa* ruch wychodzący o połowę. Wariant tutaj kończy
się rozplotem `&`, czyli realną decymacją 10×.

Katalog jest osobny, bo przebieg nadpisuje artefakty strumieni o nazwach
wspólnych z `rec205-qrs.rql`, a tamte są w repozytorium śledzone.

## Co jest sprawdzane

Iloraz bajtów surowe/wysyłane dąży do 10, a różnica między ilorazem zmierzonym
a 10 jest w całości **ubytkiem ogona startowego** strumienia wysyłanego.

Ogon jest funkcją planu, nie przebiegu — więc ubytek musi być **tą samą liczbą
rekordów** niezależnie od długości przebiegu. Gdyby rósł z długością, nie byłby
ogonem, tylko wyciekiem. To jest H10 (`plan-derived startup boundaries`)
pokazane jako składnik budżetu łącza, a nie jako warunek poprawności.

Bramka `experiment.sh`:

| | warunek |
|---|---|
| (a) | ubytek ogona identyczny we wszystkich przebiegach |
| (b) | iloraz maleje monotonicznie ku 10 |
| (c) | odchylenie ilorazu od 10 skaluje się jak 1/N |

**Dlaczego (c) nie jest progiem bezwzględnym.** Pierwsza wersja tej bramki
żądała „iloraz w najdłuższym przebiegu poniżej 10,05" i **padła na własnym
domyślnym zestawie**: przy stałym ogonie odchylenie maleje jak 1/N, więc każdy
próg bezwzględny jest osiągalny dopiero powyżej pewnej długości przebiegu.
Domyślny zestaw kończy się na 300 s i daje 10,0558 — próg był nieosiągalny
z konstrukcji, co jest defektem aparatury, nie wynikiem silnika.

Zastąpiony niezmiennikiem niezależnym od długości przebiegu: iloczyn
`(iloraz − 10) × sekundy` ma być **stały**. Dla ogona `t` slotów źródła dąży on
do `10·t/360`; przy `t = 590` daje to 16,4. Zmierzone: 16,74…16,92, rozrzut
1,08%. Wyciek rosnący z długością przebiegu rozsadziłby ten iloczyn
natychmiast, a próg bezwzględny by go przepuścił przy dostatecznie długim
przebiegu — dlatego ta postać jest ostrzejsza, a nie łagodniejsza.

## Wymagania

1. **Binarki.** Skrypty szukają `xretractor` i `xtrdb` kolejno w
   `build/Release/src`, `build/Debug/src`, a na końcu w `PATH`.
   Jeśli brak: `scripts/buildrdb.sh release` z korzenia repozytorium.
2. **Dane.** `../rec205/rec205`, `../rec205/bp_coef.txt`, `../rec205/d_coef.txt`.
   Jeśli brak: `examples/ecg/build.sh` (pobiera rekord 205 MIT-BIH i przelicza
   go na format rdb). Oba skrypty sprawdzają te warunki i wypisują tę wskazówkę.

Przebiegi są **tempowane do czasu rzeczywistego**: 180 000 slotów to ~8 minut
zegarowych. Domyślny zestaw `experiment.sh` trwa ~10 minut.

## Uruchomienie

```bash
cd examples/ecg/rec205_h10

./experiment.sh              # pełny eksperyment z bramką: 100 s, 200 s, 300 s
./experiment.sh 36000 180000 # własne długości przebiegów (w slotach źródła)

./measure.sh                 # pojedynczy pomiar, 500 s sygnału
./measure.sh 36000           # pojedynczy pomiar, krótszy
```

`experiment.sh` zapisuje `results.tsv` (jeden wiersz na przebieg) i wypisuje
werdykt `PASS`/`FAIL`. Wszystkie produkty przebiegu są w `.gitignore`.

## Konstrukcja potoku

Ten sam Pan-Tompkins co `rec205-qrs.rql`, zakończony:

```
SELECT mwi[0]*5, (mwi[0]-mwi_thr[0]*2)*5
  STREAM qrs_feat FROM mwi+mwi_thr VOLATILE
SELECT * STREAM qrs_ship FROM qrs_feat&1/324
```

Argument rozplotu to **delta strumienia usuwanego**. Dla decymacji 10× z `1/360`
jest to `1/324`, ponieważ `(1/36 · 1/324)/(1/36 + 1/324) = 1/360`. Odstępu nie
trzeba deklarować — kompilator wyprowadza `1/36` z planu; można to zobaczyć bez
uruchamiania przebiegu:

```bash
xretractor -q rec205-offload.rql -c -m | head -3
```

**Umiejscowienie decymacji jest merytoryczne, nie kosmetyczne.** Rozplot dobiera
podciąg zapisanych krotek i **nie filtruje**, więc stoi **za** scaleniem w oknie
ruchomym z kroku 4, które ogranicza pasmo obwiedni do ok. 12 Hz wobec Nyquista
18 Hz strumienia wysyłanego. Postawiony przed krokiem 4 aliasowałby — i to jest
dokładnie zastrzeżenie, które artykuł stawia w ograniczeniach („a thinned
constituent can alias").

Do przebiegu dołożony jest strumień odniesienia `ecg_raw` (oba kanały, pełne
tempo), żeby porównanie było na **jednym uruchomieniu**, a nie zestawieniem
dwóch osobnych przebiegów.

## Wartości odniesienia

Silnik `master:6dec187`, Release, GCC 15.2.0. Liczby rekordów i szerokość
rekordu odczytane z `xtrdb -s`, nie z rozmiaru pliku dzielonego przez założoną
szerokość.

| przebieg | `ecg_raw` | `qrs_ship` | iloraz | ubytek ogona |
|---|---:|---:|---:|---:|
| 36 000 slotów (100 s) | 35 999 rek. / 287 992 B | 3 540 rek. / 28 320 B | 10,1692 | 59 rek. |
| 72 000 slotów (200 s) | 71 999 rek. / 575 992 B | 7 140 rek. / 57 120 B | 10,0839 | 59 rek. |
| 108 000 slotów (300 s) | 107 999 rek. / 863 992 B | 10 740 rek. / 85 920 B | 10,0558 | 59 rek. |
| 180 000 slotów (500 s) | 179 999 rek. / 1 439 992 B | 17 940 rek. / 143 520 B | 10,0334 | **59 rek.** |

Trzy pierwsze wiersze to domyślny zestaw `experiment.sh`; czwarty zmierzono
osobno. We wszystkich czterech zachodzi dokładnie
`ship_rec = floor(raw_rec / 10) − 59`.

Stan ustalony: 8 B × 360/s = **2880 B/s** wobec 8 B × 36/s = **287 B/s**, czyli
dokładnie **10×**, redukcja **90,0%**.

Ubytek ogona jest **stały (59 rekordów)** w obu przebiegach — to jest właśnie
sprawdzana teza. Wartości bezwzględne mogą się różnić, jeśli zmieni się rachunek
ogona w silniku; stałość ubytku zmienić się nie może.

## Zakres

Pomiar dotyczy **objętości przesyłanej dla jednego potoku na jednym rekordzie**.
Nie jest twierdzeniem o budżetach łączy w ogólności i nie mierzy ani zajętości
pamięci, ani energii, ani bajtów faktycznie utrwalonych.
