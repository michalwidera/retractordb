# Eksperyment wydajnościowy RetractorDB

Infrastruktura do kampanii pomiarowej opisanej w `REQUIREMENTS.md` i w sekcji
*Performance Evaluation* `paper-arXiv/debs/main-debs.tex`. Dokument ten opisuje,
jak uruchomić kampanię od zera na własnym sprzęcie (nadzorca + worker) i co
dokładnie zostaje zarejestrowane, żeby wynik dało się odtworzyć.

## Architektura

- **Nadzorca** — maszyna, z której startuje się kampanię (`start_supervisor.sh`).
  W tym repo: `/home/michal/github/retractordb`.
- **Worker** — fizyczna maszyna pod Linuksem RT, na której faktycznie chodzi
  `xretractor`/`xqry` i zbierane są metryki. W tym repo: alias SSH `worker`,
  repo pod `/home/michal/retractordb` (patrz `REQUIREMENTS.md` @note).
- Obie maszyny mają wspólne pochodzenie (ten sam `origin`), branch eksperymentu
  jest tworzony z `master` **dopiero po** scommitowaniu tych skryptów do
  `master` (pkt 6/26 wymagań: branch eksperymentu nie zawiera zmian w kodzie).

## Wymagania wstępne (przed pierwszym uruchomieniem)

1. Skrypty z tego katalogu są scommitowane na `master` i wypchnięte do `origin`
   na obu maszynach (`git pull` na workerze).
2. `examples/ecg/build.sh` uruchomiony choć raz w repo, z którego korzysta
   worker — generuje `rec205`, `rec205.desc` (źródło dla algorytmu QRS).
3. Worker ma zainstalowane klucze SSH do `origin` (do `git push`) — pkt 20.
4. Worker ma **bezhasłowy `sudo reboot`**, bo `start_supervisor.sh` restartuje
   go fizycznie po każdym badaniu (pkt 4):
   ```
   # /etc/sudoers.d/rdb-experiment na workerze
   michal ALL=(root) NOPASSWD: /usr/sbin/reboot
   ```
5. `xretractor`/`xqry` na workerze zbudowane z sondą E1/E2E i zainstalowane:
   ```
   scripts/buildrdb.sh conan ninja probe && cd build/Release && ninja install
   ```
   (`start_supervisor.sh` robi to automatycznie, chyba że podasz `--skip-build`).
6. `nc` obecny, jeśli chcesz `--sink nc` (patrz niżej).

## Uruchomienie

Z katalogu głównego repo na nadzorcy:

```bash
# Kampania 1: rosnąca częstość napływu danych (natywne 360 Hz -> 10x), pkt 12-14
examples/experiment/start_supervisor.sh rate

# Kampania 2: rosnąca liczba klientów xqry (1..10) przy ustalonej częstości, pkt 16
# --rate-hz powinno być ustawione na max. stabilną częstość znalezioną w kampanii 1
examples/experiment/start_supervisor.sh clients --rate-hz 720
```

Obie kampanie to dwie różne osie badawcze (pkt 12-14: skalowanie częstości,
pkt 16: liczba klientów) — nie jedna siatka rate×clients, która byłaby
wielokrotnie dłuższa na fizycznym sprzęcie — ale zapisują się na **jednym,
wspólnym branchu eksperymentalnym** (domyślnie `experiment/YYYYMMDD`, bez
nazwy kampanii w nazwie brancha). Wyniki poszczególnych kampanii trafiają do
osobnych katalogów (`results/rate/...`, `results/clients/...`), ale
`commit --amend` + `push --force-with-lease` działają na tym samym branchu i
tym samym commicie niezależnie od tego, która kampania właśnie się wykonuje —
zgodnie z pkt 5 na końcu całego eksperymentu (obu kampanii) ma powstać jeden
commit. Żeby druga kampania trafiła na ten sam branch co pierwsza, podaj
jawnie ten sam `--branch`, jeśli uruchamiasz je różnego dnia:

```bash
examples/experiment/start_supervisor.sh rate    --branch experiment/20260716
examples/experiment/start_supervisor.sh clients --branch experiment/20260716 --rate-hz 720
```

Każde badanie z pliku `config/campaign_<nazwa>.csv` jest wykonywane po kolei;
po każdym (poza ostatnim) worker jest **fizycznie restartowany** i nadzorca
czeka, aż wróci po SSH, zanim zleci kolejne badanie.

Przed pierwszym badaniem danej kampanii `start_supervisor.sh` generuje na
workerze `examples/experiment/results/<kampania>/README.md` — opis celu tej
kampanii i rodzaju prowadzonego badania (nie plik dopisywany ręcznie po
fakcie). Kolejne uruchomienia tej samej kampanii nadpisują go tą samą treścią.

## Co zostaje zarejestrowane (odtwarzalność)

Dla każdego badania, w `examples/experiment/results/<kampania>/study_NN/`:

| Plik | Zawartość |
|---|---|
| `state_before.md` / `state_after.md` | `uname -a`, `/etc/os-release`, `lscpu`, `free -h`, `/proc/buddyinfo` (fragmentacja pamięci), `/proc/loadavg`, temperatury stref termicznych, data, parametry badania — pkt 7 i 15 |
| `e1_probe.csv` | surowe dane sondy `RDB_BENCH_CSV` (`iter,compute_ns,wake_lag_ns,e2e_ns`), skompilowanej z `-DRDB_BENCH_PROBE=ON` |
| `metrics.csv` | próbkowanie co 1s: `load1`, pamięć użyta/dostępna, temperatura — w trakcie realizacji badania (pkt 11) |
| `results.md` | streszczenie: wyjście `ecg/e1_stats.py` na `e1_probe.csv` + średnie z `metrics.csv` |

Wszystko trafia w jednym commicie na branch eksperymentu poprzez
`git commit --amend` + `git push --force-with-lease` po każdym badaniu (pkt 5)
— po zakończeniu całej kampanii branch ma **jeden** commit z wynikami
wszystkich badań tej kampanii.

## Odtworzenie na własnym sprzęcie

Żeby powtórzyć kampanię na innej parze maszyn:

1. Zmień `WORKER_HOST`/`WORKER_REPO` przez `--worker`/`--worker-repo`.
2. Dostosuj `config/campaign_rate.csv` / `config/campaign_clients.csv` — liczba
   i wartości badań nie są magiczne, to punkt startowy (pkt 12: "początkowo np. 10").
3. Wynik zależy od sprzętu workera — `state_before.md`/`state_after.md`
   dokumentują dokładnie, na czym mierzono (jądro, dystrybucja, CPU, pamięć),
   żeby porównanie między sprzętami było możliwe.
4. Realny tryb czasu rzeczywistego (`xretractor -t`, SCHED_FIFO) **nie jest
   włączony domyślnie** w `run_study.sh` (wymaga roota/capabilities) — to
   świadome uproszczenie; do w pełni miarodajnych liczb E2E (patrz komentarz
   w `ecg/e1_stats.py`) trzeba rozszerzyć `run_study.sh` o uruchomienie z
   `sudo ... -t` na docelowym sprzęcie.

## Znane ograniczenia / otwarte decyzje

Spisane przy przygotowaniu tych skryptów (zob. też dyskusja w commicie):

- Sonda E1/E2E (`executorsm.cpp`) nie zapisuje domyślnie nigdzie — pisze pod
  ścieżkę z `RDB_BENCH_CSV`; `run_study.sh` ustawia ją jawnie na plik w
  `/dev/shm/rdb-experiment/study_<id>/`.
- **Katalog roboczy to `/dev/shm`, nie `/tmp`.** Na Raspberry Pi 400 (worker
  użyty przy pierwszym przebiegu tej kampanii) `/tmp` okazał się zwykłym
  `ext4` na karcie SD (`/dev/mmcblk0p2`), a nie `tmpfs` — użycie samej nazwy
  `/tmp` nie gwarantuje pamięci RAM (pkt 17 zakłada, że gwarantuje).
  `/dev/shm` jest w praktyce niemal zawsze prawdziwym `tmpfs`; `run_study.sh`
  i tak sprawdza to (`[ -d /dev/shm ]`) i przerywa badanie, jeśli katalog nie
  istnieje, zamiast cicho pisać gdziekolwiek indziej.
- `bp_coef.txt`/`d_coef.txt` nie mają w repo towarzyszących `.desc` —
  `xretractor` tworzy je przy pierwszym uruchomieniu w bieżącym katalogu, więc
  `run_study.sh` **musi** odpalać proces z katalogu w `/dev/shm`, inaczej
  powstałby artefakt na karcie SD w working tree repo.
- Ujście danych klientów `xqry` domyślnie to `/dev/null` (`--sink null`);
  `--sink nc` przepuszcza je przez `nc -lk 127.0.0.1:<port>` na loopback,
  jeśli chcesz uwzględnić koszt stosu sieciowego w pomiarze.
- Kończenie kampanii (pkt 21) jest decyzją człowieka na podstawie
  `results.md` kolejnych badań — nie jest automatyzowane.
