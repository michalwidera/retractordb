#include "executorsm.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <ctime>  // kotwica osi czasu pętli: clock_gettime, timespec
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <print>
#include <string>
#include <thread>
#include <utility>

#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>
#include <boost/interprocess/exceptions.hpp>
#include <boost/system/error_code.hpp>

#include "bus.hpp"
#include "dataModel.hpp"
#include "executor_rt.hpp"
#include "executorsmState.hpp"
#include "fatalError.hpp"
#include "ipcServer.hpp"
#include "persistentCounter.hpp"
#include "planSource.hpp"
#include "rdb/probe.hpp"  // sondy E1/E2E, K6, E4
#include "serviceControl.hpp"
#include "shmBudget.hpp"
#include "uxSysTermTools.hpp"

// #include "antlr4-runtime/tree/ParseTree.h"

// extern antlr4::tree::ParseTree *pTree;

namespace IPC = boost::interprocess;

using namespace CRationalStreamMath;
// Stan wspolny wykonawcy — patrz executorsmState.hpp.
using namespace esm;

namespace {
constexpr std::chrono::milliseconds kIdleLoopSleep{100};
}  // namespace

void cleanup() {
  // Blad krytyczny w usludze systemd: plan, ktory zabil proces, nie moze wrocic przy
  // restarcie. Plik zapytan zostaje oprozniony, wiec jednostka wstaje w trybie bezczynnym
  // i czeka na kolejne `xqry --reset`. Bez tego Restart=on-failure zapetla start na tym
  // samym planie — a stan zerowy jest jedynym stanem, o ktorym wiadomo, ze wstanie.
  // Pierwsza czynnosc sprzatania: dalej zwalniamy blokade, po ktorej moze juz wystartowac
  // nastepna instancja i przeczytac ten plik.
  if (fatalErrorRaised.load(std::memory_order_acquire) && !serviceQueryFilePath.empty()) {
    if (servicecontrol::writeQueryFile("", serviceQueryFilePath))
      SPDLOG_CRITICAL("Fatal error: query file '{}' cleared; the service unit will restart with no plan.", serviceQueryFilePath);
    else
      SPDLOG_CRITICAL("Fatal error: could NOT clear query file '{}'; the service unit may restart into the same plan.",
                      serviceQueryFilePath);
  }
  {
    std::scoped_lock lock(core_mutex);
    if (iLoopLimitCnt != executorsm::stop_now) {
      SPDLOG_WARN("Cleanup: Setting iLoopLimitCnt to stop_now.");
      iLoopLimitCnt = executorsm::stop_now;
      std::cout << "Cleanup!" << '\n';
    }
  }
  cv.notify_all();
  ipcServer.shutdownFromExitHandler();
  // Slot magistrali przed blokada, w tej samej kolejnosci co reszta sprzatania: dopiero
  // zwolniona blokada wpuszcza kolejna instancje, a ta czyta magistrale.
  if (busPtr != nullptr) busPtr->release();
  // Blokada uslugi na koncu: po niej moze juz wystartowac kolejna instancja, wiec
  // zwalniamy ja dopiero, gdy IPC jest posprzatane. releaseLock() jest idempotentny,
  // wiec pozniejszy destruktor straznika na sciezce normalnej nie zrobi nic drugi raz.
  if (serviceGuardPtr != nullptr) serviceGuardPtr->releaseLock();
}

std::set<std::string> executorsm::getAwaitedStreamsSet(TimeLine &tl, qTree *coreInstancePtr) {
  if (coreInstancePtr == nullptr) FatalError("executorsm::getAwaitedStreamsSet: coreInstancePtr is null");
  std::set<std::string> retVal;
  for (const auto &it : *coreInstancePtr)
    if (tl.isThisDeltaAwaitCurrentTimeSlot(it.rInterval)) retVal.insert(it.id);

  return retVal;
}

int executorsm::run(qTree &coreInstance, FlockServiceGuard &guard, bus::Bus &xrdbbus, compiler &cm, vm_map &vm,
                    const AppConfig &cfg, std::string_view serverName, std::string_view systemdUnit) {
  executorsm::coreInstancePtr       = &coreInstance;
  executorsm::cmPtr                 = &cm;
  executorsm::cfgQueueBufferSeconds = cfg.ipcQueueBufferSeconds;
  executorsm::cfgMinQueueElements   = cfg.ipcMinQueueElements;
  executorsm::cfgRtPriority         = cfg.schedulingRtPriority;
  executorsm::cfgStorageDir         = cfg.storageDir;
  // Dyrektywy sa jeszcze w drzewie: dataModel usunie je dopiero przy budowie modelu.
  executorsm::activeStorageDir = planStorageDir(coreInstance, cfg.storageDir);
  dataModelExpected            = !coreInstance.empty();
  untilEofMode                 = vm.contains("until-eof");
  // Plik zapytan uslugi. Nadpisuje go przyjety plan i oprozniaja skutki bledu krytycznego,
  // wiec wskazuje go WYLACZNIE instancja bedaca jednostka systemd: plik `.rql` operatora,
  // ktory uruchomil xretractor z terminala, jest jego wlasnoscia, a nie stanem uslugi.
  if (!systemdUnit.empty()) {
    serviceQueryFilePath = guard.getServiceQueryFile().empty() ? cfg.serviceQueryFile : guard.getServiceQueryFile();
    SPDLOG_INFO("Service unit '{}': plan reloads are persisted to '{}'.", systemdUnit, serviceQueryFilePath);
  }

  // Zakres waznosci wskaznika na straznika — patrz komentarz przy serviceGuardPtr.
  // RAII, a nie zerowanie przy kazdym `return`, bo run() ma ich kilka.
  struct LockGuardScope {
    explicit LockGuardScope(FlockServiceGuard &g) { serviceGuardPtr = &g; }
    ~LockGuardScope() { serviceGuardPtr = nullptr; }
  } lockGuardScope(guard);

  struct BusScope {
    explicit BusScope(bus::Bus &b) { busPtr = &b; }
    ~BusScope() { busPtr = nullptr; }
  } busScope(xrdbbus);

  // Launcher musi wejsc tutaj z przejeta blokada i roszczeniem magistrali. To jest granica
  // transakcji startowej: oba zasoby zostaly zdobyte przed kasowaniem artefaktow i pozostaja
  // wazne do konca executora. Brak blokady oznacza blad kolejnosci wywolan, nie zwykla kolizje.
  if (!guard.isLockActive()) {
    SPDLOG_ERROR("Executor started without an active instance lock.");
    return system::errc::state_not_recoverable;
  }

  ipcServer.setServerName(serverName);
  if (!serverName.empty()) SPDLOG_INFO("Instance name: {}", serverName);

  // atexit dopiero po przejeciu blokady i slotu. Proces, ktory odpadl w launcherze, nie moze
  // miec handlera kasujacego IPC lub zwalniajacego cudze zasoby.
  std::atexit(cleanup);

  std::string percounterFilename{"{notinitialized}"};
  for (const auto &it : coreInstance)
    if (it.id == ":ROTATION") {
      percounterFilename = it.filename;
    }

  if (percounterFilename != "{notinitialized}") pCounterPtr = std::make_unique<PersistentCounter>(percounterFilename);

  auto retVal = system::errc::success;

  // Sending service in thread. Warstwa protokolu wchodzi do transportu przez te
  // cztery wywolania zwrotne -- IpcServer nie zna qTree, dataModel ani compilera.
  ipcServer.start({
      .onCommand = [](const ptree &pt) { return executorsm::commandProcessor(pt); },
      // Stan predykatu musi zmienic sie POD core_mutex. Watek glowny czeka na ipcReady
      // pod tym samym muteksem (ponizej), a cv.wait zwalnia go dopiero w chwili
      // zablokowania. Ustawienie flagi bez muteksu pozwalalo trafic w okno miedzy
      // sprawdzeniem predykatu a zasnieciem watku glownego -- powiadomienie przepadalo
      // i start wisial na zawsze, nie reagujac nawet na SIGTERM.
      .onReady =
          [] {
            {
              std::scoped_lock lock(core_mutex);
              executorsm::ipcReady = true;
            }
            cv.notify_all();
          },
      // Sciezka blizniacza do onReady i ta sama regula muteksu: predykat zmienia sie pod
      // core_mutex, inaczej powiadomienie trafia w okno miedzy sprawdzeniem a zasnieciem.
      .onFailure =
          [] {
            {
              std::scoped_lock lock(core_mutex);
              executorsm::ipcFailed = true;
            }
            cv.notify_all();
          },
      .onMessageReceived =
          [] {
            // Fakt "przyszla pierwsza komenda" zapisujemy BEZWARUNKOWO i w osobnym
            // zatrzasku. Poprzednia wersja podnosila bramke tylko wtedy, gdy widziala juz
            // iLoopLimitCnt == waitForXqry, a te flage watek glowny ustawia dopiero PO
            // zbudowaniu dataModel -- czyli dlugo po opublikowaniu blokady, na ktora czeka
            // klient. Komenda z tego okna gubila pobudke i serwer stal na bramce az do
            // nastepnej komendy (odtworzone 5/5 planem o 120 strumieniach).
            {
              std::scoped_lock lock(core_mutex);
              firstQueryReceived = true;
            }
            cv.notify_all();
          },
      .shouldStop = [] { return iLoopLimitCnt == executorsm::stop_now; },
  });

  {
    std::unique_lock<std::mutex> lock(core_mutex);
    cv.wait(lock, [] { return executorsm::ipcReady.load() || executorsm::ipcFailed.load(); });
  }

  // Bez zasobow IPC instancja nie ma jak przyjac ani jednej komendy: nie odpowie na `xqry`,
  // nie przyjmie planu i nie da sie jej zatrzymac inaczej niz sygnalem. Konczymy wiec od razu
  // i z komunikatem, zamiast liczyc dla nikogo. Najczestsza przyczyna jest mierzalna z wyprzedzeniem
  // -- `xretractor -c --shmbudget <plan>` pokazuje, ile miejsca w pamieci dzielonej potrzeba.
  if (executorsm::ipcFailed.load()) {
    const shmbudget::Space fs = shmbudget::space();
    std::println(std::cerr, "xretractor: cannot create IPC resources; fixed reservation needs {}{}",  //
                 shmbudget::humanBytes(shmbudget::fixedReservationBytes()),
                 fs.known ? std::format(", shared memory has {} free", shmbudget::humanBytes(fs.available)) : std::string{});
    SPDLOG_ERROR("Startup aborted: IPC resources unavailable.");
    ipcServer.stop();
    ipcServer.removeAllObjects();
    return system::errc::no_buffer_space;
  }

  // Blokade mamy od poczatku run(), ale jej TRESC publikujemy dopiero teraz. Linia
  // "PID: <pid>" w pliku blokady jest dla klientow i dla testow sygnalem "serwer gotowy"
  // (kontrakt server_start w test/IntegrationTest/serverlib.sh), wiec nie moze
  // pojawic sie, zanim segment i kolejka komend beda istniec.
  guard.publishLockInfo();

  try {
    // Zatrzymanie klawiszem nalezy wylacznie do przebiegu nieograniczonego -- tylko dla niego
    // drukowany jest ponizej komunikat "Press any key to stop". Przebieg z zadeklarowanym
    // budzetem slotow konczy sie po tym budzecie i po niczym innym, bo jego wynik ma byc
    // powtarzalny. Bez tego warunku bajt czekajacy na terminalu konczyl petle PRZED pierwszym
    // slotem: proces wychodzil kodem 0, deskryptor juz istnial (powstaje przed petla), a plik
    // danych zostawal pusty -- tak padl it_agse_array na CI (2026-09-04). Ta sama pulapka, co
    // opisana w qry.cpp dla xqry (issue_215): na CI stdin bywa terminalem z bajtem w buforze.
    // Ctrl+C (SIGINT) zatrzymuje przebieg bez zmian, obiema drogami.
    //
    // O ograniczeniu przebiegu decyduje WARTOSC licznika, nie obecnosc opcji: --llimitqry ma
    // default_value, wiec vm.contains("llimitqry") jest zawsze prawda. Licznik ograniczony
    // liczy w dol do stop_now i nigdy nie przyjmuje wartosci inifitie_loop.
    const bool boundedRun   = iLoopLimitCnt != executorsm::inifitie_loop;
    const bool ignoreanykey = vm.contains("noanykey") || boundedRun;

    // Petla EPOK planu. Jedna epoka to jeden plan: pusty (tryb bezczynny) albo policzalny
    // (dataModel + os czasu + petla slotow). Epoka konczy sie zatrzymaniem procesu, klawiszem,
    // wyczerpaniem budzetu — albo przyjeta komenda `reset`, i tylko wtedy zaczyna sie nastepna.
    // Bez tej petli tryb bezczynny byl slepym zaulkiem: instancja bez planu nie miala jak go
    // przyjac inaczej niz przez restart procesu.
    while (iLoopLimitCnt != executorsm::stop_now) {
      dataModelExpected = !coreInstancePtr->empty();

      if (coreInstancePtr->empty()) {
        //
        // Tryb bezczynny (idle): brak zapytań — nie budujemy dataModel ani TimeLine
        // (uniknięcie FatalError). Czekamy na zatrzymanie (SIGTERM / klawisz / limit iteracji),
        // utrzymując wątek komunikacyjny i blokadę usługi. pProc pozostaje null —
        // wątek komunikacyjny obsługuje to (komendy działają tylko gdy pProc != nullptr).
        //
        SPDLOG_INFO("Idle mode: no queries to process, waiting for a plan or a shutdown signal.");
        while (!_kbhit(ignoreanykey) && iLoopLimitCnt != executorsm::stop_now &&
               !planResetRequested.load(std::memory_order_acquire)) {
          if (iLoopLimitCnt != executorsm::inifitie_loop) {
            if (iLoopLimitCnt != executorsm::stop_now)
              iLoopLimitCnt--;
            else
              break;
          }
          if (!guard.isLockActive()) {
            SPDLOG_ERROR("CRITICAL ERROR: Lost service lock!");
            break;
          }
          std::this_thread::sleep_for(kIdleLoopSleep);
        }
      } else {
        // Tryb liczenia do konca wejscia: zrodla deklarowane czytamy bez zawijania, tak jakby kazda
        // deklaracja niosla ONESHOT. Bez tego pytanie "czy wejscie sie skonczylo" nie ma odpowiedzi —
        // zrodlo zawijane po koncu pliku wraca na jego poczatek i produkuje rekordy z danych, ktore
        // juz raz przeszly. Ustawienie musi nastapic PRZED konstrukcja dataModel, bo to ona tworzy
        // magazyny i przekazuje isOneShot do fabryki akcesorow.
        const bool until_eof_mode = untilEofMode;
        if (until_eof_mode)
          for (auto &q : *coreInstancePtr)
            if (q.isDeclaration()) q.isOneShot = true;

        dataModel proc(*coreInstancePtr);
        {
          // Publikacja pod obiema blokadami: blokada epoki wpuszcza handlery dopiero do
          // modelu gotowego, core_mutex niesie powiadomienie do czekajacych na cv.
          std::scoped_lock lock(plan_epoch_mutex, core_mutex);
          pProc = &proc;
        }
        cv.notify_all();

        // Czy bramke --xqrywait zdjelo zatrzymanie procesu, a nie komenda klienta. Osobna
        // zmienna, a nie odczyt iLoopLimitCnt nizej: `stop_now` to wartosc 1, czyli dokladnie
        // to, co w liczniku zostawia `-m 1`, wiec warunek na liczniku zmienialby zachowanie
        // przebiegu z budzetem jednego slotu — a ten z bramka nie ma nic wspolnego.
        bool gateStoppedProcess = false;

        if (vm.contains("xqrywait")) {
          if (vm.contains("verbose")) std::cout << "Waiting for first query to start process.\n";
          // Warunek na zatrzasku, a nie na liczniku petli. Licznik niesie budzet slotow
          // z --llimitqry, wiec uzycie go jako flagi bramki kasowalo ten budzet: po
          // podniesieniu bramki wracala wartosc inifitie_loop, a nie zadane N. Skutek byl
          // wprost mierzalny -- `xretractor -m 5` konczyl sie sam, `xretractor -x -m 5`
          // chodzil bez konca. Zatrzask ustawiony PRZED wejsciem tutaj przepuszcza od razu,
          // wiec komenda z okna startowego nie ginie.
          //
          // Czekanie jest TERMINOWE, bo bramke musi zdejmowac takze zatrzymanie procesu, a
          // sygnalu nie da sie tu uslyszec inaczej. handleSignal() ustawia wylacznie
          // iLoopLimitCnt (notify_all nie jest async-signal-safe), wiec czekanie bezterminowe
          // nie mialo kto przerwac: `xretractor -x` bez ani jednej komendy przezywal SIGTERM
          // i schodzil dopiero na SIGKILL -- systemd czekal na to caly TimeoutStopSec.
          // Rozszerzenie samego predykatu nic by nie dalo, potrzebna jest wlasnie pobudka
          // z zegara. Takt 100 ms jest ponizej kazdego rozsadnego limitu zatrzymania.
          std::unique_lock<std::mutex> scoped_lock(core_mutex);
          while (!firstQueryReceived.load() && iLoopLimitCnt != executorsm::stop_now)
            cv.wait_for(scoped_lock, kIdleLoopSleep);
          gateStoppedProcess = !firstQueryReceived.load();
          if (vm.contains("verbose") && !gateStoppedProcess) std::cout << "First query received, starting processing loop.\n";
        }

        if (vm.contains("verbose")) coreInstancePtr->dumpCore();

        std::set<boost::rational<int>> timeIntervals;
        std::uint64_t observedAdHocPlanRevision;
        {
          std::scoped_lock lock(core_mutex);
          timeIntervals             = coreInstancePtr->getAvailableTimeIntervals();
          observedAdHocPlanRevision = adHocPlanRevision.load(std::memory_order_relaxed);
        }
        TimeLine tl(timeIntervals);
        //
        // Main loop of data processing
        //
        // When this value is 0 - means we are waiting for key - other way watchdog
        //
        if (iLoopLimitCnt == executorsm::inifitie_loop && vm.contains("verbose")) std::cout << "Press any key to stop.\n";

        // Formatowanie wiersza jest warstwa protokolu, transport dostaje je jako callback.
        const IpcServer::RowFormatter formatRow = [this](const std::string &name) { return printRowValue(name); };

        // ZERO-step
        std::set<std::string> inSet;
        for (const auto &it : *coreInstancePtr)
          if (it.isDeclaration()) inSet.insert(it.id);
        // Zatrzymanie, ktore zdjelo bramke --xqrywait, nie ma prawa policzyc ani jednego kroku:
        // proces konczony sygnalem zapisalby wtedy rekord zerowy do magazynu, choc nikt o niego
        // nie prosil. Sama petla ponizej i tak nie wykona obrotu (warunek stop_now), a wyjscia
        // `break` w tym miejscu byc nie moze -- ominieloby zgaszenie pProc na koncu epoki i
        // zostawiloby watkowi komunikacyjnemu wskaznik na rozbierany dataModel.
        if (!gateStoppedProcess) {
          proc.processZeroStep();
          ipcServer.broadcast(inSet, formatRow);
        }
        // End of ZERO-step

        // Loop of data processing
        boost::rational<int> prev_interval(0);

        // Sonda E1/E2E: czas obliczeń slotu i latencja end-to-end (rdb/probe.hpp).
        // Uzbrajana dopiero zmienną RDB_BENCH_CSV; bez wkompilowanej sondy znika w całości.
        rdb::probe::slotProbe slotBench;

        struct timespec loop_anchor{};
        const bool rt_mode = vm.contains("realtime");
        // Tryb offline: oś czasu planu (interwały, wyrównanie slotów, ogon) pozostaje nietknięta —
        // znika wyłącznie czekanie na zegar ścienny, więc ciąg wyliczonych rekordów jest ten sam
        // co w przebiegu taktowanym. Wyklucza się z rt_mode; sprzeczność odrzuca launcher.
        const bool no_clock_mode = vm.contains("no-clock");
        if (rt_mode) {
          if (rtCheckAndPrint()) {
            rtActivate(cfgRtPriority);
            // Dopiero TERAZ znana jest maska wątku RT, więc dopiero teraz można z
            // niej wyliczyć rdzenie dla wątku komunikacyjnego. Bez tego przy
            // obciążeniu powyżej 100 % slotu wątek komunikacyjny nie dostaje CPU
            // i żaden klient nie zdąży się zarejestrować (issue_217, badanie W8).
            rtKeepThreadOffRtCpus(ipcServer.threadHandle());
          }
        }

        // Sonda E1/E2E otwierana PRZED kotwicą osi czasu: koszt otwarcia pliku nie może
        // obciążyć budżetu pierwszych slotów (transjent startowy ~20-47 ms w wake_lag --
        // sledztwo ~40 ms, JOURNAL.md 2026-07-18, Faza 3). Tak samo rtActivate
        // (mlockall/SCHED_FIFO) musi wykonać się przed kotwicą.
        slotBench.open();
        clock_gettime(CLOCK_MONOTONIC, &loop_anchor);
        slotBench.anchor(loop_anchor);

        while (!_kbhit(ignoreanykey) && iLoopLimitCnt != executorsm::stop_now &&
               !planResetRequested.load(std::memory_order_acquire)) {
          if (iLoopLimitCnt != executorsm::inifitie_loop) {
            if (iLoopLimitCnt != executorsm::stop_now)
              iLoopLimitCnt--;
            else
              break;
          }

          // Check if system service lock is still active
          if (!guard.isLockActive()) {
            SPDLOG_ERROR("CRITICAL ERROR: Lost service lock!");
            break;
          }

          // Szybka ścieżka wykonuje tylko odczyt atomowy. Pełny skan planu i
          // przebudowa osi następują wyłącznie po opublikowaniu importu ad hoc.
          const auto currentAdHocPlanRevision = adHocPlanRevision.load(std::memory_order_acquire);
          if (currentAdHocPlanRevision != observedAdHocPlanRevision) {
            std::scoped_lock lock(core_mutex);
            auto availableTimeIntervals = coreInstancePtr->getAvailableTimeIntervals();
            if (availableTimeIntervals != timeIntervals) {
              tl.updateTimeIntervals(availableTimeIntervals);
              timeIntervals = std::move(availableTimeIntervals);
            }
            // Import również publikuje rewizję pod core_mutex. Ponowny odczyt
            // pod blokadą obejmuje wszystkie importy zakończone przed tym skanem.
            observedAdHocPlanRevision = adHocPlanRevision.load(std::memory_order_relaxed);
          }

          //
          // Inner time is counted in miliseconds
          // probably can be increased in faster machines
          //
          const int msInSec                          = 1000;
          const boost::rational<int> currentTimeSlot = tl.getNextTimeSlot();
          boost::rational<int> interval(currentTimeSlot * msInSec /* sec->ms */);
          int period(rational_cast<int>(interval - prev_interval));  // miliseconds
          prev_interval = interval;

          //
          // Waiting given miliseconds time that is computed
          //
          if (rt_mode)
            rtAbsoluteSleep(loop_anchor, rational_cast<long>(interval));
          else if (!no_clock_mode)
            std::this_thread::sleep_for(std::chrono::milliseconds(period));

          slotBench.beginSlot(rational_cast<long>(interval));
          {
            // Kompilator ad hoc modyfikuje qTree pod tym samym muteksem. Bez blokady
            // iteracja getAwaitedStreamsSet mogłaby ścigać się z importem nowych węzłów.
            std::scoped_lock lock(core_mutex);
            inSet = getAwaitedStreamsSet(tl, coreInstancePtr);
          }
          {
            // Slot liczy sie pod blokada epoki, bo MUTUJE model: processRows przepisuje payloady,
            // a broadcast siega po nie przez getPayload (releaseOnHold/revRead). Handler komendy
            // czyta te same liczniki i te sama mape qSet, wiec bez tego wykluczenia `xqry -d`
            // czytalby stan w trakcie zmiany. Blokada jest brana PRZED beginCompute: czekanie na
            // komende w locie nie ma prawa wejsc do mierzonego rdzenia E1, a samo zajecie
            // nieobciazonego muteksu to kilkadziesiat nanosekund. endSlot zostaje w srodku, zeby
            // zwolnienie blokady wypadlo poza pomiarem.
            std::scoped_lock epoch(plan_epoch_mutex);
            slotBench.beginCompute();
            proc.processRows(inSet, currentTimeSlot);  // mierzony rdzeń obliczeń jednego interwału (E1)
            slotBench.endCompute();
            ipcServer.broadcast(inSet, formatRow);
            slotBench.endSlot();
          }

          // Deklaracje sa czytane na koncu slotu, a ich rekord konsumuje dopiero slot nastepny.
          // Wyjscie z petli w tym miejscu wypada wiec dokladnie przed pierwszym rekordem, ktory
          // powstalby z all-null wstawionego za koniec wejscia.
          if (until_eof_mode) {
            const auto exhausted = proc.exhaustedInputStream();
            if (!exhausted.empty()) {
              SPDLOG_INFO("End of input on declared stream '{}' — stopping (--until-eof).", exhausted);
              if (vm.contains("verbose")) std::cout << "End of input on stream '" << exhausted << "'. Stopping.\n";
              // Ta sama droga wyjscia co przy wyczerpaniu --llimitqry: stop_now zdejmuje czekanie
              // na klawisz ponizej petli, wiec przebieg wsadowy konczy sie sam.
              {
                std::scoped_lock lock(core_mutex);
                iLoopLimitCnt = executorsm::stop_now;
              }
              break;
            }
          }
          // End of loop while( ! _kbhit(ignoreanykey) )
        }

        // Raport liczników runtime (K6 materializacja, E4 praca na slot) po zakończeniu
        // mierzonej pętli, żeby zliczanie nie obciążało budżetu slotu.
        rdb::probe::reportRuntimeCounters();
        //
        // End of data processing loop
        //

        // Koniec epoki: model znika, zanim `proc` wyjdzie z zakresu. Kolejnosc jest wymogiem
        // poprawnosci — watek komunikacyjny siega po pProc bez wlasnej wiedzy o epokach, wiec
        // wskaznik musi zgasnac POD BLOKADA EPOKI, a nie tylko pod core_mutex. Sam core_mutex
        // zatrzymywal wylacznie komendy jeszcze nieprzebudzone; ta, ktora byla juz w srodku
        // handlera, czytala pProc na nowo i dostawala nulla albo zniszczony model.
        // plan_epoch_mutex czeka tu na handler w locie, a po jego zwolnieniu pProc jest juz
        // nullem, wiec destrukcja `proc` ponizej nie ma komu wyrwac obiektu spod rak.
        {
          std::scoped_lock lock(plan_epoch_mutex, core_mutex);
          pProc             = nullptr;
          dataModelExpected = false;
        }
        cv.notify_all();
        ipcServer.broadcastOutOfBusiness();
      }

      if (!planResetRequested.load(std::memory_order_acquire)) break;
      applyPendingPlan(guard, xrdbbus, cfg);
    }
    // Klawisz, ktory zakonczyl OSTATNIA epoke, zdejmujemy raz — epoka przerwana
    // przeladowaniem planu nie konczy sie klawiszem, wiec nie ma tam czego pobierac.
    if (iLoopLimitCnt != executorsm::stop_now) _getch();  // no wait ... feed key from kbhit
  } catch (IPC::interprocess_exception &ex) {
    std::cerr << ex.what() << '\n' << "IPC::interprocess exception" << '\n';
    retVal = system::errc::no_child_process;
  } catch (std::exception &e) {
    std::cerr << "IPC Fail." << '\n';
    std::cerr << e.what() << '\n';
    SPDLOG_ERROR("catch exception: {}", e.what());
    retVal = system::errc::interrupted;
  }
  {
    std::scoped_lock lock(core_mutex);
    iLoopLimitCnt = executorsm::stop_now;
  }
  cv.notify_all();
  ipcServer.broadcastOutOfBusiness();
  ipcServer.stop();
  ipcServer.removeAllObjects();
  return retVal;
}
