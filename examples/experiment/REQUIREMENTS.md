Eksperyment powinien:
- być procesem prowadzonym na fizycznym sprzęcie z zainstalowanym oprogramowaniem RetractorDB, systemem pracującym pod kontrolą Linux skonfigurowanym w trybie czasu rzeczywistego.
- mieć zdefinowany cel badawczy: zebranie informacji wymaganych dla celów publikacji naukowej (paper-arXiv/debs/main-debs.tex - Performance Evaluation)
- składać się z szeregu badań. Każde badanie powinno być poprzedzone zapisaniem stanu maszyny do pliku, wykonaniem badań - zebraniem ich wyników w pliku markdown, ponownym zapisaniem stanu po zrealizowaniu badania. Wysłaniem zebranych danych do repozytorium i zatwierdzeniu. Przeprowadzeniem procesu restartu maszyny workera przez nadzorcę i przygotowaniem do kolejnego badania.
- Zapis danych realizowany jest na branchu. Dane powinny być zapisywane do repozytorium w jednym commit. Za każdym badaniem wysyłany jest commit --ammed modyfikujący ostatni kommit i wysyłany git push --force-with-lease. Po zakończeniu eksperymentu powinien powstać pojedyńczy commit zawierający pliki/zapisy poszczególnych procesów badawczych.
- być prowadzony na dwóch maszynach. Obie maszyny mają pobrane aktualne repozytorium z wydzielonym branchem. Branch nie powinien zawierać zmian funkcjonalnych w kodzie - do brancha będa zapisywane jedynie wyniki eskperymentu.
- zapisać za każdym razem przed uruchomieniem pojedyńczego procesu badawczego stan, konfigurację (informacje o jądrze, systemie, dystrybucji, jego konfiguracji i zasobach), datę wykonania oraz rodzaj przeprowadzonego badania.
- być uruchamiany ze skryptu na maszynie nadzorcy znajdującego się katalogu examples/experminet/start_superivsor.sh
- w ramach procesu nadzorcy wydawać polecenia maszynie workera. 
- w trakcie realizacji procesu na maszynie workera uruchomic ciąg poleceń realizujący wybrany algorytm przetwarzania danych oraz zebrać dane z wbudowanego w oprogramowanie czujników.
- Zbierać informację o temperaturze, obciążeniu procesora, wykorzystaniu pamięci systemu w celu określenia dostępnych w czasie realizacji zasobów i potencjalnych przyczyn zaburzeń realizacji procesu xretractor w rygorze czasu rzeczywistego.
- Być podzielony na szereg (początkowo np.3) badań zwiększających szybkość napływu danych i rejestrujących wyniki opóźnień end-2-end oraz temperatury, obciążenia procesora i wykorzystania pamięci w trakcie realizacji badanego algorytmu.
- Analizować wybrany algorytm (opisany w pracy main-debs.tex/detekcja qry) - dla różnych szybkości napływu. Celem nie jest poprawność realizacji algorytmu ale określenie jakiej szybkości dane na wybranym sprzęcie workera można zrealzować.
- Zwiększąć w ramach kolejnych badań szybkość napływu danych i rejestrować kolejne wyniki.
- Dostarczyć informacji: jak system zachowuje się wraz ze wzrostem szybkości napływu danych, czy zmianie ulegają opóźnienia w dostarczaniu wyników (probe), czy wzrasta temperatura urządzenia (czas realizacji eksperymentu), czy po zakończeniu pracy systemu - system wraca do stanu przed eksperymentem (czy system zwalania zasoby lub czy fragmentacja pamięci systemowej w trakcie pracy wzrasta)
- W trakcie eksperymentu zebrać informację czy dołączenie dołączonych klientów (xqry) (np. od 1 do 10) ma wpływ i jaki na na funkcjonowanie procesu xretractor i system. (Określenie wpływu na proces działający w systemie)
- Ograniczyć komunikację z kartą sd w trakcie eksperymentu. Analizowany Algorytm powinien przetwarzać dane w pamięci (VOLATILE) i nie tworzyć artefaktów. Dane źródłowe dla xretractor powinny znajdować się w katalogu /tmp znajdującym się w pamięci.
- ograniczyć pobieranie i wysłanie danych w trakcie realizacji na kartę SD. Dołączane procesy xqry powinny wysyłać dane netcat np. na loopback lub /dev/null a xretractor powinien czytać dane źródłowe z /tmp aby nie dokonywać pomiaru szybkości działania karty sd na której znajduje się system i repozytorium. (Eksperyment w którym będziemy testować system wyposażony w dysk twardy SSD w takim systemie jest zaplanowany) 
- wykorzystywać sondę ecg/e1_stats.py do realziacji eksperymentu.
- Każdy eksperyment kończy się zatwierdzonym commit w repozytorium zrealizowanym na maszynie workera (maszyna ma zainstalowane klucze ssh), po czym dane są analizowane i podejmowana jest decyzja o modyfikacji eksperymentu (np. tego pliku) w celu wyzacznia kolejnych szczegółowych celów badawczych i/lub skierowaniu badań w określonym celu.
- zakończyć proces powtarzania kolejnych eksperymentów po osiagnięciu celu badawczego: Określeniu zbioru informacji - jak szybkie dane możemy analizować na wybranej platformie sprzętowej bez straty dokładności, ilu klientów możemy podłączyć w trakcie realizacji procesu, jakie opóźnienie wnosi proces xretractor poprzez realizację badanego algorytmu end-2-end. 
- być udokumentowany, zapisany i zarejestrowany w sposób dający możliwość odtworzenia (przynajmniej teoretycznie) procesu badawczego we własnych warunkach na własnym sprzęcie (inny host nadzorcy/workera, inne parametry badań).
- przed rozpoczęciem każdej kampanii zweryfikować, że branch `master` na maszynie nadzorcy i na maszynie workera jest zsynchronizowany z `origin/master` (brak rozjazdu, czyste working tree) - dopiero wtedy wolno utworzyć z niego branch eksperymentu. WYJĄTEK (dopisany 2026-07-18, śledztwo zdarzeń ~40 ms; poluzowanie tego punktu oraz pkt 6): branch `experiment/40ms` MOŻE zawierać zmiany funkcjonalne (skrypty Fazy 0, a w kolejnych fazach w razie potrzeby instrumentację silnika) i worker buduje binarki z brancha eksperymentu, nie z mastera. Jeśli przyczyna zdarzeń zostanie jednoznacznie ustalona, branch będzie squashowany i dołączony do mastera; w przeciwnym razie pozostaje niezmergowany jako udokumentowana ślepa uliczka.
- móc składać się z wielu kampanii badawczych (np. skalowanie szybkości napływu danych - pkt 12-14, oraz liczba dołączonych klientów xqry - pkt 16). Każda kampania zapisuje swoje wyniki do osobnego katalogu, ale wszystkie kampanie tego samego eksperymentu są zatwierdzane (commit --amend + push --force-with-lease, pkt 5) na jednym, wspólnym branchu eksperymentalnym - nie tworzymy osobnego brancha per kampania.
- każdy katalog kampanii (osobny katalog wyników, patrz punkt wyżej) ma dołączony plik README.md opisujący cel tej konkretnej kampanii i rodzaj prowadzonego w jej ramach badania. Plik ten jest generowany automatycznie przez proces nadzorcy PRZED rozpoczęciem pierwszego badania danej kampanii (nie dopisywany ręcznie po fakcie) - to uzupełnienie pkt 7 (który dotyczy stanu pojedynczego badania) o kontekst na poziomie całej kampanii.
- w przypadku przerwania eksperymentu i dostarczenia nowego kodu do analizy, poprzednie wyniki powinny zostać przeniesione do katalogu z kolejnym numerem rotacji - a nowe wyniki z nowego eksperymentu powinny trafić do pustego, nowego rezultatu. Celem jest zachowanie kontekstu.
- proces badawczy musi być dokumentowany na bieżąco w dzienniku badawczym (examples/experiment/JOURNAL.md): każdy krok, wynik (sukces i porażka), postawiona hipoteza, jej weryfikacja lub obalenie, podjęta decyzja i plan kolejnych działań. Wpisy są datowane i dopisywane chronologicznie (nie przepisujemy historii - błędne hipotezy zostają w dzienniku jako część drogi badawczej). Dziennik jest częścią infrastruktury eksperymentu i żyje na gałęzi master; commit wpisu powinien towarzyszyć zmianie, której dotyczy.

@note Branch repozytorium na maszynie nadzorcy znajduje się w katalogu /home/michal/github/retractordb
@note Branch repozytoriumna maszynie workera znajduje się w katalogu /home/michal/retractordb
@note Oba branche wskazują na ten sam korzeń w drzewie repozytorium.
@note Przed rozpoczęciem eksperymenu na branchu nie ma żadnych zmian i zawiera on to co znajduje się w głęzi master. Identyfikatorem tej gałęzi (master) - jest podpisywany eksperyment
@note Pkt 17 zakłada, że `/tmp` jest w pamięci - na sprzęcie workera to nieprawda: zweryfikowano (Pi 400), że `/tmp` to zwykły ext4 na karcie SD (`/dev/mmcblk0p2`), nie tmpfs. Katalogiem faktycznie gwarantującym RAM jest `/dev/shm` - tego (nie `/tmp`) używają skrypty.
@note po zakończeniu eksperymentów z algorytmem adetekcji qrs i profilowaniem systemu - zrób krótki test określający jak szybki napływ danych jest w stanie przetworzyć prosty filtr FIR (dla kontrastu badawczego) - użyj testu retractordb/test/IntegrationTest_parallel/dsp do zbudowania przykładu.

---

## Plan badawczy — kampanie baseline (rozszerzenie dla celów publikacji)

Dopisane 2026-07-17, po ukończeniu kampanii rate/clients/fir-contrast
(JOURNAL.md, dni 0–2). Cel: dostarczyć dane do dwóch pozostałych
podsekcji Performance Evaluation w main-debs.tex — 7.5 Baselines
(sec:eval-baselines) i 7.6 Exactness and Replay Stability
(sec:eval-exact). Obowiązuje pełna metodyka z pkt 1–27 (migawki stanu,
dziennik, rotacja, wspólny branch eksperymentu).

### Kampania baseline-numpy (priorytet 1)

- Cel: zmiennoprzecinkowy punkt odniesienia dla 7.5(i) — koszt dokładnej
  semantyki wymiernej — oraz dane wejściowe dla 7.6 (błąd resamplingu
  float zestawiony z tożsamością okrężną przeplotu/rozplotu).
- Implementacja: potok Pan–Tompkinsa o etapach identycznych z
  rec205-detect.rql, w float64, NumPy/SciPy w venv na workerze
  (koła aarch64 dostępne dla Ubuntu 24.04; wersje pakietów przypięte
  i zapisane w results.md).
- Dwa tryby pomiaru, raportowane OSOBNO (nie wolno ich uśredniać ani
  porównywać wprost — mierzą co innego):
  1. per-slot: pętla próbka-po-próbce (okna na collections.deque),
     pomiar na zegarze monotonicznym per interwał — odpowiednik metryki
     E1; zawiera narzut interpretera CPython (odnotować przy
     interpretacji);
  2. batch: wektoryzowany scipy.signal.lfilter na całym nagraniu —
     przepustowość batch, bez semantyki slotowej.
- Dyscyplina środowiska identyczna z kampaniami QRS: governor
  performance, taskset -c 3 (rdzeń izolowany), SCHED_FIFO 50 przez
  os.sched_setscheduler, 20 000 próbek, dane w /dev/shm, sampler
  metrics.csv; wyniki do results_YYYYMMDD/baseline-numpy/ wg konwencji
  kampanii (results.md + surowe CSV + migawki stanu).
- Hipoteza do weryfikacji: koszt semantyki wymiernej jest pomijalny
  (profil callgrind z dnia 1: boost::rational <0,4% instrukcji);
  ewentualne różnice zdominuje model wykonania (interpreter vs
  skompilowany silnik), nie arytmetyka.
- Część dla 7.6: (a) dwukrotny przebieg silnika na identycznym wejściu
  → równość artefaktów co do bitu; (b) round-trip
  interleave/de-interleave w silniku (tożsamość, wniosek cor:exact)
  vs scipy.signal.resample/resample_poly (skumulowany błąd float,
  raportowany jako norma błędu w funkcji długości nagrania).

### Kampania baseline-flink (priorytet 2, expected fail)

- Cel: próba punktu odniesienia DSMS głównego nurtu dla 7.5(ii).
  UWAGA: sama próba instalacji i uruchomienia jest wynikiem badawczym —
  także negatywnym — i podlega regule dziennika (pkt 27); porażkę
  dokumentujemy z przyczynami, nie ukrywamy.
- Plan próby: OpenJDK 17/21 aarch64 z repozytorium Ubuntu; Flink w
  trybie local/MiniCluster (jeden JVM), heap ≤2 GB, checkpointing
  wyłączony; równoważny dataflow Pan–Tompkinsa; pomiar per rekord.
- Zastrzeżenia (dlaczego spodziewamy się niepowodzenia lub wyniku
  nieporównywalnego):
  1. RAM: Pi 400 ma 4 GB; JobManager + TaskManager + heap + metaspace
     obok systemu i klientów grozi OOM lub swapowaniem na kartę SD
     (co samo w sobie unieważnia pomiar);
  2. jitter JVM: GC i kompilacja JIT zaburzają rozkłady per slot przy
     360 Hz — brak odpowiednika sondy E1/E2E, ogony nieporównywalne;
  3. model wykonania: Flink nie ma semantyki slotowej 1/Δ — bufory
     sieciowe i event-time wnoszą opóźnienia rzędu ms niezależne od
     obliczeń; porównanie łatwo zakwestionować jako strawman;
  4. izolacja rdzenia: pozostawienie całego JVM na 3 rdzeniach to
     konfiguracja nietypowa dla Flinka (wynik nie będzie reprezentować
     „Flinka w warunkach zalecanych").
- Kryterium stopu: jeśli instalacja, uruchomienie lub stabilny przebieg
  20 000 próbek nie powiedzie się w budżecie 1 dnia roboczego —
  zamykamy próbę wpisem w dzienniku (wynik negatywny + przyczyny),
  a w 7.5(ii) artykułu opisujemy powód braku porównania.
- Opcja zapasowa (jeśli Flink odpadnie): baseline „engine-double" —
  wariant silnika z double zamiast arytmetyki wymiernej (przełącznik
  kompilacji), izolujący koszt semantyki przy wszystkich pozostałych
  czynnikach stałych; tańszy pomiarowo i trudniejszy do zakwestionowania
  niż porównanie międzysystemowe.