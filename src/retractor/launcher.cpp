#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <sstream>
#include <vector>

#include <fmt/ranges.h>                    // fmt::join — łączenie listy ścieżek konfiguracyjnych
#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>

#include "config.h"  // Add an automatically generated configuration file
#include "lib/appConfig.hpp"
#include "lib/bus.hpp"
#include "lib/compiler.hpp"
#include "lib/executor_rt.hpp"
#include "lib/executorsm.hpp"
#include "lib/lockManager.hpp"
#include "lib/persistentCounter.hpp"
#include "lib/presenter.hpp"
#include "lib/qTree.hpp"
#include "lib/serverName.hpp"
#include "lib/serviceControl.hpp"
#include "rdb/probe.hpp"  // baner buildu z sondami pomiarowymi
#include "uxSysTermTools.hpp"

/// @brief Główny plik uruchamiający program, odpowiedzialny za parsowanie argumentów, obsługę sygnałów i koordynację działania programu.
///
/// program xretractor powinien:
/// - być budowany jako pojedynczy plik wykonywalny, który można uruchomić z linii poleceń.
/// - Przyjmować jako opcjonalny argument plik z zapytaniami RQL; brak pliku uruchamia tryb bezczynny (idle).
/// - Parsować ten plik i kompilować zapytania do postaci wewnętrznej reprezentacji (qTree).
/// - Uruchamiać wykonanie zapytań, zarządzać ich cyklem życia i obsługiwać wyniki.
/// - Obsługiwać sygnały systemowe (np. SIGINT, SIGTERM), aby umożliwić bezpieczne zatrzymanie programu.
/// - Zapewniać opcje konfiguracyjne, takie jak tryb tylko kompilacji, ciche działanie, generowanie diagramów itp.
/// - Logować istotne informacje o działaniu programu, błędach i wynikach do pliku logów (poza trybem usługowym)
/// - Być odpornym na błędy, zapewniając odpowiednie komunikaty o błędach i obsługę wyjątków.
///
/// Niezależność od programu nadzorującego (supervisor):
/// - xretractor jest samodzielnym procesem i NIE wymaga do działania żadnego programu nadrzędnego
///   typu supervisor/orchestrator; musi w pełni funkcjonować uruchomiony bezpośrednio z linii poleceń.
/// - Nie zakłada obecności kanału sterującego nadzorcy ani jego API (REST/gRPC); własny cykl życia
///   (start, praca, zatrzymanie) realizuje samodzielnie poprzez argumenty CLI i sygnały systemowe.
/// - Cała koordynacja stanu odbywa się przez własne mechanizmy procesu: blokadę pojedynczej instancji
///   (FlockServiceGuard), kanał IPC do klientów (xqry) oraz sygnały — bez zależności od procesu nadzorcy.
/// - Ewentualny supervisor pełni wyłącznie rolę zewnętrznego zarządcy plików/uruchomień i może zostać
///   przebudowany lub usunięty bez wpływu na zdolność xretractor do samodzielnej pracy.
///
/// Praca jako usługa systemd:
/// - Działać w trybie pierwszoplanowym (foreground), bez samodzielnej demonizacji — zgodnie z `Type=simple`.
/// - Umożliwiać start bez pliku .rql (tryb idle), tak aby jednostka systemd mogła wstać przy starcie systemu
///   i pozostać aktywna, zanim zostaną zdefiniowane jakiekolwiek zapytania (bez pętli restartów/crash-loop).
/// - Reagować na SIGTERM bezpiecznym, kontrolowanym zatrzymaniem (graceful shutdown) w skończonym czasie,
///   współpracując z `KillSignal=SIGTERM` i `TimeoutStopSec` menedżera systemd.
/// - W trybie usługi nie oczekiwać na klawisz/terminal (opcja --noanykey), działając bez TTY.
/// - Zwracać kody wyjścia zgodne z konwencją POSIX, aby systemd mógł poprawnie ocenić stan i politykę Restart.
/// - Udostępniać kontrolę stanu działania (opcja --status) na potrzeby zewnętrznego monitoringu/healthcheck.
///
/// Logowanie w trybie usługi systemowej:
/// - W trybie usługi kierować logi na standardowe wyjście procesu (stdout/stderr), tak aby były przechwytywane
///   przez journald i dostępne przez `journalctl -u`; nie pisać logów do pliku w katalogu tymczasowym (/tmp).
/// - Nie duplikować znacznika czasu w komunikacie — czas nadaje journald; format usługowy ma być zwięzły
///   (poziom + treść), bez własnego timestampu.
/// - Nie emitować kodów ANSI/kolorów w trybie usługowym, gdy wyjście nie jest terminalem (brak TTY) — log do journala musi być
///   czystym tekstem.
/// - Nie wykonywać własnej rotacji ani retencji plików logów — pozostawić to menedżerowi journald.
/// - Zapewniać natychmiastowy zrzut (flush) po każdej linii, aby wpisy pojawiały się w dzienniku na bieżąco.
/// - Mapować poziom logowania na priorytety syslog (prefiks `<0>`..`<7>` wg sd-daemon),
///   aby journald poprawnie klasyfikował wagę komunikatów w trybie usługowym.
/// - Umożliwiać włączenie trybu usługowego logowania zarówno flagą CLI (--service), jak i zmienną
///   środowiskową XRETRACTOR_SERVICE — dla wygody konfiguracji jednostki systemd przez Environment=.
///
/// Interfejs komunikacji i funkcjonalność:
/// - Wymuszać pojedynczą instancję programu w systemie poprzez blokadę plikową (FlockServiceGuard); kolejna próba startu
///   jest sygnalizowana błędem braku dostępnej blokady (no_lock_available).
/// - W przypadku rozpoznania funkcjonowania innej instancji programu, należy rozpoznać czy ta instancja działa jako serwis,
///   czy jako osobny proces, jeśli działa jako serwis zaraportować informację o tym fakcie i
///   przekompilować zapytania (sprawdzić poprawność) i przekazać zapytanie do tej instancji poprzez restart serwisu z zapytaniem (zachowując konfigurację serwisu).
/// - Udostępniać dane wynikowe strumieni klientom (xqry) przez współdzieloną pamięć / IPC (Boost.Interprocess)
///   obsługiwane w osobnym wątku komunikacyjnym, niezależnym od wątku przetwarzania danych.
/// - Umożliwiać sterowanie startem przetwarzania z poziomu klienta (opcja --xqrywait: wstrzymanie pętli do
///   nadejścia pierwszego zapytania kanałem IPC) oraz zewnętrzne zatrzymanie instancji.
/// - Wspierać tryb wsadowej kompilacji bez uruchamiania przetwarzania (--onlycompile) z generowaniem artefaktów
///   diagnostycznych (dot/csv/diagram) jako odrębną, nieusługową ścieżką użycia.
/// - Oferować ograniczenie liczby iteracji pętli (--llimitqry) na potrzeby testów i pracy deterministycznej.
/// - Opcjonalnie wspierać szeregowanie czasu rzeczywistego (--realtime: SCHED_FIFO, mlockall, sen do bezwzględnego
///   punktu czasu) dla deterministycznych interwałów przetwarzania.
/// - Oferować tryb bez taktowania zegarem ściennym (--no-clock) dla przebiegów offline: pełna semantyka
///   interwałów planu, ale bez czekania na zegar. Wyklucza się z --realtime.
/// - Oferować tryb liczenia do końca wejścia (--until-eof): źródła deklarowane czytane bez zawijania,
///   a przebieg kończony w slocie, w którym pierwsze z nich wyczerpie dane. Zdejmuje dobieranie --llimitqry
///   do długości serii danych.
///
/// Konfiguracja:
/// - System w trybie usługowym wspiera pliki konfiguracjne podobnie jak inne usługi systemu linux (np. sshd).
/// - W plikach konfiguracyjnych można zdefiniować katalogi w których będą przechowywane artefakty (storage) a także inne ustawienia.
/// - Pliki konfiguracyjne są opcjonalne, a ich brak nie powinien uniemożliwiać startu programu; w przypadku braku konfiguracji
///   program powinien działać z domyślnymi ustawieniami, a brak konfiguracji traktować jako stan poprawny (nie błąd).
/// - Jeśli ustawiono storage.dir, katalog musi istnieć i mieć uprawnienia zapisu; w przeciwnym razie start programu
///   jest przerywany z błędem konfiguracji.
/// - Konfigurację wspiera bibliteka toml++.

using namespace boost;

using boost::lexical_cast;

extern std::tuple<std::string, std::string, std::string> parserRQLString(qTree &coreInstance, const std::string &sInputFile);
extern std::vector<std::string> readLogicalLines(std::ifstream &file);

extern std::atomic<int> iLoopLimitCnt;

extern std::vector<std::pair<std::string, std::string>> processedLines;

static void handleSignal(int signum) {
  switch (signum) {
    case SIGINT:
      SPDLOG_WARN("Received SIGINT, initiating shutdown...");
      break;
    case SIGTERM:
      SPDLOG_WARN("Received SIGTERM, initiating shutdown...");
      break;
    case SIGHUP:
      SPDLOG_WARN("Received SIGHUP, initiating shutdown...");
      break;
    default:
      SPDLOG_WARN("Received unknown signal: {}", signum);
      break;
  }

  // This will cause the main loop to exit
  iLoopLimitCnt = executorsm::stop_now;
}

void dropArtifactFile(const std::filesystem::path &artifact_filename) {
  if (std::filesystem::exists(artifact_filename)) {
    std::error_code ec;
    std::filesystem::remove(artifact_filename, ec);
    if (ec) {
      SPDLOG_WARN("Failed to remove file {}: {}", artifact_filename.string(), ec.message());
    }
  }
}

static std::vector<std::string> claimedStreamNames(const qTree &plan) {
  std::vector<std::string> retVal;
  for (const auto &q : plan)
    if (!q.isCompilerDirective()) retVal.push_back(q.id);
  return retVal;
}

// Normalizacja sciezki publikowanej w slocie magistrali. absolute() PRZED weakly_canonical():
// plik licznika przy pierwszym starcie jeszcze nie istnieje, a weakly_canonical nad
// nieistniejaca sciezka wzgledna zwraca ja bez zmiany — czyli bez katalogu roboczego,
// o ktory w tej normalizacji chodzi.
static std::string absolutePathOf(const std::string &path) {
  if (path.empty()) return {};
  std::error_code ec;
  const auto absolute = std::filesystem::absolute(std::filesystem::path(path), ec);
  if (ec) return path;
  const auto canonical = std::filesystem::weakly_canonical(absolute, ec);
  return ec ? absolute.string() : canonical.string();
}

static std::string normalizedRotationCounterPath(const qTree &plan) {
  for (const auto &q : plan)
    if (q.id == ":ROTATION" && !q.filename.empty()) return absolutePathOf(q.filename);
  return {};
}

static std::string ownerLabel(std::string_view instance) {
  return instance.empty() ? "the unnamed instance" : "instance '" + std::string(instance) + "'";
}

static void validateConfiguredStorageDir(const AppConfig &cfg) {
  if (cfg.storageDir.empty()) return;

  const std::filesystem::path storageDir(cfg.storageDir);
  std::error_code ec;

  if (!std::filesystem::exists(storageDir, ec)) {
    throw std::invalid_argument("Configuration error: storage.dir does not exist: " + storageDir.string());
  }
  if (ec) {
    throw std::invalid_argument("Configuration error: cannot access storage.dir '" + storageDir.string() + "': " + ec.message());
  }
  if (!std::filesystem::is_directory(storageDir, ec)) {
    throw std::invalid_argument("Configuration error: storage.dir is not a directory: " + storageDir.string());
  }
  if (ec) {
    throw std::invalid_argument("Configuration error: cannot inspect storage.dir '" + storageDir.string() +
                                "': " + ec.message());
  }

  const std::filesystem::path probeFile = storageDir / (".xretractor_write_probe_" + std::to_string(std::rand()) + ".tmp");
  {
    std::ofstream out(probeFile, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
      throw std::invalid_argument("Configuration error: storage.dir is not writable: " + storageDir.string());
    }
    out << "probe";
  }
  std::filesystem::remove(probeFile, ec);
}

static void printOptimizerBuildInfo() {
#if RDB_OPT_DEDUP_SUBSTRATES
  std::println("RDB_OPT_DEDUP_SUBSTRATES=ON");
#else
  std::println("RDB_OPT_DEDUP_SUBSTRATES=OFF");
#endif
#if RDB_OPT_SHARE_EQUIVALENT_SELECTS
  std::println("RDB_OPT_SHARE_EQUIVALENT_SELECTS=ON");
#else
  std::println("RDB_OPT_SHARE_EQUIVALENT_SELECTS=OFF");
#endif
#if RDB_OPT_COMMUTATIVE_ADD
  std::println("RDB_OPT_COMMUTATIVE_ADD=ON");
#else
  std::println("RDB_OPT_COMMUTATIVE_ADD=OFF");
#endif
#if RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES
  std::println("RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=ON");
#else
  std::println("RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=OFF");
#endif
  std::println("RDB_BENCH_PROBE={}", rdb::probe::enabled ? "ON" : "OFF");
#if RDB_OPT_SIMPLIFY_EXPRESSIONS
  std::println("RDB_OPT_SIMPLIFY_EXPRESSIONS=ON");
#else
  std::println("RDB_OPT_SIMPLIFY_EXPRESSIONS=OFF");
#endif
}

int main(int argc, char *argv[]) {
  qTree coreInstance;
  compiler cm(coreInstance);

  fixArgcv(argc, argv);

  namespace po = boost::program_options;

  // Wczesny skan argumentów: tryb logowania usługowego musi być znany przed konfiguracją logera.
  // Tryb usługi można włączyć flagą (-j/--service) albo zmienną środowiskową XRETRACTOR_SERVICE
  // (dowolna wartość poza pustą i "0") — wygodne dla jednostki systemd przez Environment=.
  bool serviceLog{false};
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--service") == 0) serviceLog = true;
  }
  if (const char *env = std::getenv("XRETRACTOR_SERVICE"); env != nullptr && env[0] != '\0' && strcmp(env, "0") != 0)
    serviceLog = true;

  const auto tempLocation = setupLoggerMain(std::string(argv[0]), false /* dual */, serviceLog);

  // Kompilacja z włączoną sondą pomiarową. Ostrzeżenie trafia do logu, a w trybie
  // usługowym (-j) do journald — operator usługi widzi, że to build benchmarkowy, nie produkcyjny.
  if constexpr (rdb::probe::enabled)
    SPDLOG_WARN("[warning: probe benchmark build] measurement probe compiled in (RDB_BENCH_PROBE) — NOT for production.");

  // Nazwa instancji i sciezka konfiguracji musza byc znane przed zbudowaniem straznika blokady.
  // Ten sam parser Boosta obsluguje wszystkie formy, ktore zaakceptuje pozniejsze parsowanie
  // pelnego CLI: `--name alfa`, `--name=alfa` i sklejone `-nalfa`. allow_unregistered zostawia
  // pozostale opcje i argument pozycyjny dla pelnego parsera nizej.
  po::options_description earlyDesc;
  earlyDesc.add_options()("name,n", po::value<std::string>())("autoname", "")("config,g", po::value<std::string>());
  po::variables_map earlyVm;
  try {
    po::store(po::command_line_parser(argc, argv).options(earlyDesc).allow_unregistered().run(), earlyVm);
    po::notify(earlyVm);
  } catch (const po::error &e) {
    std::println(std::cerr, "{}: {}", argv[0], e.what());
    return system::errc::invalid_argument;
  }

  const std::optional<std::string> earlyConfigPath =
      earlyVm.contains("config") ? std::optional<std::string>(earlyVm["config"].as<std::string>()) : std::nullopt;
  std::string earlyServerName = earlyVm.contains("name") ? earlyVm["name"].as<std::string>() : std::string{};
  // --autoname to osobna flaga, a nie --name o opcjonalnej wartosci: przy opcjonalnej wartosci
  // `xretractor --name plik.rql` bylo nierozroznialne od nazwy instancji podanej wprost, bo
  // plik zapytan jest argumentem pozycyjnym. Osobna flaga nie ma tej dwuznacznosci.
  const bool wantsAutoName = earlyVm.contains("autoname");
  if (wantsAutoName && !earlyServerName.empty()) {
    std::println(std::cerr, "{}: --autoname and --name are mutually exclusive", argv[0]);
    return system::errc::invalid_argument;
  }

  // Konfiguracja musi byc znana przed rozstrzygnieciem nazwy, bo klucz [server] autoname
  // wspoldecyduje o losowaniu. Sama sciezka konfiguracji zalezy tylko od --config, wiec
  // przesuniecie tego ladowania przed blok nazwy nie tworzy cyklu.
  const AppConfig earlyAppCfg = [&]() -> AppConfig {
    try {
      return loadAppConfig(earlyConfigPath);
    } catch (...) {
      return {};  // błąd zostanie powtórzony i zgłoszony niżej z właściwym komunikatem
    }
  }();

  // Klucz konfiguracyjny dziala tylko wtedy, gdy operator nie rozstrzygnal nazwy sam:
  // jawne --name wygrywa po cichu, tak samo jak dyrektywa :STORAGE z RQL wygrywa nad
  // storage.dir. --autoname i autoname=true nie sa konfliktem, tylko dwiema drogami do
  // tego samego skutku.
  if (wantsAutoName || (earlyServerName.empty() && earlyAppCfg.serverAutoName)) {
    earlyServerName = servername::generate();
    // Nazwa musi trafic na standardowe wyjscie, nie tylko do logu: bez niej operator nie ma
    // jak wskazac tej instancji w `xqry --server`. Opróznienie bufora jest tu konieczne, a nie
    // ostrozne: stdout przekierowany do pliku jest buforowany blokowo, wiec bez flush nazwa
    // pojawia sie dopiero przy koncu procesu — czyli wtedy, gdy nie jest juz do niczego potrzebna.
    std::println("Instance name: {}", earlyServerName);
    std::fflush(stdout);
  }
  if (!earlyServerName.empty() && !servername::isValid(earlyServerName)) {
    std::println(std::cerr, "{}: invalid instance name '{}': expected [a-z][a-z0-9_-]{{0,{}}}", argv[0], earlyServerName,
                 servername::kMaxLength - 1);
    return system::errc::invalid_argument;
  }

  po::variables_map vm;
  po::options_description desc("Available options");

  bool onlyCompile{false};
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--onlycompile") == 0) onlyCompile = true;
  }

  // Bez --name zostaje tozsamosc historyczna (jeden serwer na maszyne, ta sama nazwa blokady
  // i te same obiekty IPC co dotad). Nazwa wlacza rezim wieloserwerowy i jest opcjonalna
  // wlasnie po to, zeby dotychczasowe uzycie nie zmienilo sie ani o jeden plik.
  const std::string executableName = std::filesystem::path(argv[0]).filename().string();
  const std::string serviceName    = executableName + "_service" + (earlyServerName.empty() ? "" : "." + earlyServerName);
  FlockServiceGuard guard(serviceName);
  guard.setLockDir(earlyAppCfg.lockDir);

  int loopLimitVar{executorsm::inifitie_loop};
  AppConfig appCfg = earlyAppCfg;
  try {
    std::string sInputFile;
    std::string sDiagram;
    std::string sConfig;
    std::string sServerName;
    if (onlyCompile) {
      desc.add_options()                                                             //
          ("help,h", "show help options")                                            //
          ("build-info,b", "show optimizer build configuration")                     //
          ("onlycompile,c", "compile only mode")                                     // linking inheritance from launcher
          ("queryfile,q", po::value<std::string>(&sInputFile), "query set file")     //
          ("quiet,r", "no output on screen, skip presenter")                         //
          ("dot,d", "create dot output")                                             //
          ("csv,m", "create csv output")                                             // c->m
          ("fields,f", "show fields in dot file")                                    //
          ("tags,t", "show tags in dot file")                                        //
          ("streamprogs,s", "show stream programs in dot file")                      //
          ("rules,u", "show rules in dot file")                                      //
          ("hideruleprog,i", "hide rule program in rules (-u) output")               //
          ("transparent,p", "make dot background transparent")                       //
          ("diagram,w", po::value<std::string>(&sDiagram), "create diagram output")  //
          ;
    } else {
      desc.add_options()                                                          //
          ("help,h", "Show program options")                                      //
          ("build-info,b", "show optimizer build configuration")                  //
          ("onlycompile,c", "compile only mode")                                  // linking inheritance from launcher
          ("queryfile,q", po::value<std::string>(&sInputFile), "query set file")  //
          ("quiet,r", "no output on screen, skip presenter")                      //
          ("status,s", "check service status")                                    //
          ("verbose,v", "verbose mode (show stream params)")                      //
          ("xqrywait,x", "wait with processing for first query")                  //
          ("name,n", po::value<std::string>(&sServerName),
           "instance name; own IPC area and lock (default: single-instance mode)")                    //
          ("autoname", "generate a docker-style instance name and print it")                          //
          ("noanykey,k", "do not wait for any key to terminate")                                      //
          ("service,j", "service mode: log to stderr (journald), no log file")                        //
          ("realtime,t", "enable real-time scheduling (SCHED_FIFO, mlockall, absolute wakeup)")       //
          ("no-clock,f", "offline mode: compute slots without waiting for the wall clock")            //
          ("until-eof,u", "stop when a declared source runs out of input (forces one-shot sources)")  //
          ("config,g", po::value<std::string>(&sConfig), "config file (TOML); overrides search")      //
          ("llimitqry,m", po::value<int>(&loopLimitVar)->default_value(executorsm::inifitie_loop),    //
           "loop iteration limit, 0 - no limit")                                                      //
          ;
    }
    po::positional_options_description p;  // Assume that infile is the first option
    p.add("queryfile", -1);
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

    po::notify(vm);

    // Wczesny i pelny parser maja dawac jedna tozsamosc. Niezgodnosc oznaczalaby, ze blokada
    // i IPC dostaly inna nazwe niz pozostala czesc programu. Przy --autoname nikt nie podal
    // --name, wiec vm nie ma tego klucza i porownanie sie nie wykonuje: nazwa zostala
    // wygenerowana wyzej i zadne pozniejsze parsowanie jej nie zna.
    if (vm.contains("name") && vm["name"].as<std::string>() != earlyServerName) {
      throw std::logic_error("early and full --name parsing produced different instance names");
    }

    // Introspekcja binarki (jak --version): tylko odczyt flag kompilacji, obsługiwana przed
    // wczytaniem i walidacją konfiguracji — na hoście z niepoprawnym storage.dir zapytanie
    // "czym jest ta binarka" musi nadal dać czysty wynik na stdout.
    if (vm.contains("build-info")) {
      printOptimizerBuildInfo();
      return system::errc::success;
    }

    appCfg = loadAppConfig(vm.contains("config") ? std::optional<std::string>(sConfig) : std::nullopt);
    if (appCfg.loadedFrom.empty())
      SPDLOG_INFO("No configuration file found; using built-in defaults.");
    else
      SPDLOG_INFO("Configuration loaded from: {}", fmt::join(appCfg.loadedFrom, ", "));
    validateConfiguredStorageDir(appCfg);

    iLoopLimitCnt = loopLimitVar;  // std::atomic assignment

    if (vm.contains("status")) {
      std::println("Checking service status.");
      bool isRunning = guard.isAnotherInstanceRunning();
      std::println("{}: {}", serviceName, isRunning ? "Running" : "Stopped");
      return isRunning ? system::errc::no_lock_available : system::errc::success;
    }

    if (vm.contains("help")) {
      if constexpr (rdb::probe::enabled) std::println("[warning: probe benchmark build]\n");
      std::println("{} - compiler & data processing tool.\n", argv[0]);
      std::print("Usage: {}", argv[0]);
      if (onlyCompile) std::print(" -c");
      std::println(" queryfile [option]\n");
      std::cout << desc;
      std::println("{}", config_line);
      std::println("Log: {}", tempLocation);
      if (vm.contains("realtime")) rtCheckAndPrint();
      std::println("{}", warranty);
      return system::errc::success;
    }

    // --realtime zaostrza szeregowanie do bezwzględnych pobudek, --no-clock je usuwa. Żądania są
    // sprzeczne i nie ma sensownego rozstrzygnięcia w żadną stronę, więc zamiast cichego pierwszeństwa
    // jednej z opcji zgłaszamy błąd argumentów.
    if (vm.contains("realtime") && vm.contains("no-clock")) {
      std::println("{}: fatal error: --realtime and --no-clock are mutually exclusive", argv[0]);
      return EPERM;
    }

    // Brak pliku z zapytaniami: w trybie --onlycompile to błąd (nie ma czego kompilować),
    // w trybie usługowym oznacza start bezczynny (idle) — pomijamy parsowanie i kompilację.
    if (!vm.contains("queryfile")) {
      if (onlyCompile) {
        std::println("{}: fatal error: no input file", argv[0]);
        return EPERM;  // ERROR defined in errno-base.h
      }
      SPDLOG_INFO("No query file provided; starting in idle (service) mode.");
    } else {
      if (!std::filesystem::exists(sInputFile)) {
        std::println("{}: fatal error: file {} does not exist.", argv[0], sInputFile);
        return EPERM;  // ERROR defined in errno-base.h
      }

      // Zapamiętaj plik zapytań w blokadzie: jeśli ta instancja zostanie serwisem, inna instancja
      // odczyta QUERYFILE i będzie wiedziała, który plik nadpisać przed restartem serwisu (E3).
      guard.setServiceQueryFile(std::filesystem::absolute(sInputFile).string());

      std::ifstream file(sInputFile);
      if (!file.is_open()) {
        std::cerr << "Error: Unable to open file!" << '\n';
        return system::errc::protocol_error;
      }

      std::string parseOut = "Empty file.";
      for (const auto &stmt : readLogicalLines(file)) {
        auto [status, first_keyword, stream_name] = parserRQLString(coreInstance, stmt);
        parseOut                                  = status;
        if (status != "OK") break;
        processedLines.emplace_back(stream_name, stmt);
      }

      file.close();

      if (parseOut != "OK") {
        std::cerr << "Input file:" << sInputFile << '\n'  //
                  << "Parse result:" << parseOut << '\n';
        return system::errc::protocol_error;
      }

      //
      // Compile part
      //
      if (coreInstance.empty()) throw std::out_of_range("No queries to process found");

      std::string response;

      response = cm.compile();

      if (response != "OK") {
        std::cerr << "Input file:" << sInputFile << '\n'  //
                  << "Check result:" << response << '\n';
        return system::errc::protocol_error;
      }

      if (onlyCompile) {
        if (!vm.contains("quiet")) {
          presenter dm(coreInstance);
          return dm.run(vm);
        }
        return system::errc::success;
      }

      // Odsiew przed dostarczeniem planu do dzialajacego serwisu obejmuje wszystkie fizyczne
      // zasoby publikowane w slocie: nazwy strumieni i licznik rotacji. Instancje docelowa
      // pomijamy, bo restart zastapi jej dotychczasowy plan. Zwykly start nie polega juz na tej
      // migawce: ponizej atomowo rości slot PRZED skasowaniem pierwszego artefaktu.
      {
        const std::vector<std::string> plannedStreams = claimedStreamNames(coreInstance);
        const std::string counterPath                 = normalizedRotationCounterPath(coreInstance);
        const bus::Bus xrdbbus(bus::kSegmentName, false);
        const std::vector<bus::InstanceInfo> instances = xrdbbus.instances();
        if (const auto owner = bus::findForeignOwner(instances, earlyServerName, plannedStreams)) {
          const std::string ownerName = ownerLabel(owner->instance);
          std::cerr << "xretractor: stream '" << owner->stream << "' is already served by " << ownerName << " (pid "
                    << owner->pid << "); nothing was changed\n";
          SPDLOG_ERROR("Refused before any change: stream '{}' is already served by {} (pid {}).", owner->stream, ownerName,
                       owner->pid);
          return system::errc::device_or_resource_busy;
        }
        if (const auto owner = bus::findForeignCounterOwner(instances, earlyServerName, counterPath)) {
          const std::string ownerName = ownerLabel(owner->instance);
          std::cerr << "xretractor: rotation counter file '" << owner->path << "' is already used by " << ownerName << " (pid "
                    << owner->pid << "); nothing was changed\n";
          SPDLOG_ERROR("Refused before any change: rotation counter file '{}' is already used by {} (pid {}).", owner->path,
                       ownerName, owner->pid);
          return system::errc::device_or_resource_busy;
        }
      }

      // E3: jeśli działa już inna instancja będąca serwisem systemd, nie startujemy drugiej —
      // dostarczamy zwalidowany (skompilowany powyżej) zestaw zapytań, nadpisując plik zapytań
      // serwisu i zlecając restart. Serwis załaduje nowy zestaw, zachowując konfigurację jednostki.
      // Podwójna kompilacja (tu lokalnie + w serwisie po restarcie) jest zamierzona.
      if (guard.isAnotherInstanceRunning()) {
        const FlockServiceGuard::PeerInfo peer = guard.readPeerInfo();
        if (peer.kind == FlockServiceGuard::PeerInfo::Kind::Service && !peer.unit.empty()) {
          const std::string target = peer.queryFile.empty() ? appCfg.serviceQueryFile : peer.queryFile;
          SPDLOG_INFO("Detected running service unit '{}'; delivering compiled query set to {}.", peer.unit, target);
          if (!servicecontrol::deliverQueryFile(sInputFile, target)) {
            std::cerr << "Failed to write query file to service: " << target << '\n';
            return system::errc::io_error;
          }
          const bool userScope = peer.scope == FlockServiceGuard::PeerInfo::Scope::User;
          const int rc         = servicecontrol::restartService(userScope, peer.unit);
          if (rc != 0) {
            std::cerr << "Failed to restart service unit '" << peer.unit << "' (systemctl rc=" << rc << ").";
            if (!userScope) std::cerr << " A system unit restart requires privileges (run with sudo).";
            std::cerr << '\n';
            return system::errc::operation_not_permitted;
          }
          std::println("Query compiled OK and sent to running service '{}' (restart requested).", peer.unit);
          return system::errc::success;
        }
        // Inna instancja to zwykły proces (lub nierozpoznana) — transakcja startowa poniżej
        // zgłosi brak dostępnej blokady (no_lock_available); nie próbujemy restartu.
      }
    }

    // Domyślny katalog storage z opcjonalnego pliku konfiguracyjnego (toml++).
    // Stosowany tylko gdy zestaw RQL nie podał własnej dyrektywy :STORAGE (RQL ma
    // pierwszeństwo) i gdy istnieją realne zapytania — w trybie idle coreInstance jest
    // pusty, dataModel nie powstaje, więc domyślny storage nie ma tam zastosowania.
    if (!appCfg.storageDir.empty() && !coreInstance.empty() &&
        std::ranges::none_of(coreInstance, [](const auto &it) { return it.id == ":STORAGE"; })) {
      query storageDirective;
      storageDirective.id       = ":STORAGE";
      storageDirective.filename = appCfg.storageDir;
      coreInstance.push_back(storageDirective);
      SPDLOG_INFO("Default storage directory from config: {}", appCfg.storageDir);
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << "\n";
    return system::errc::interrupted;
  }

  // Od tego miejsca zaczyna sie transakcja startowa zwyklej instancji. Najpierw blokada
  // tozsamosci, potem atomowe roszczenie magistrali, dopiero potem kasowanie artefaktow.
  // Przegrany rownolegly start nie dochodzi dzieki temu do zadnej czynnosci destrukcyjnej.
  if (!guard.acquireLock()) {
    // Odmowa startu musi byc widoczna tam, gdzie widac pozostale odmowy z tej transakcji
    // (konflikt strumienia, licznika, magistrali) — czyli na stderr, nie tylko w logu. Skrypt,
    // ktory startuje serwer w tle i po chwili odpytuje go klientem, nie ma innego sposobu, zeby
    // zauwazyc, ze jego serwer nie wstal: bez komunikatu pracuje dalej na cudzej instancji.
    const FlockServiceGuard::PeerInfo peer = guard.readPeerInfo();
    std::cerr << "xretractor: " << ownerLabel(earlyServerName) << " is already running";
    if (peer.pid != 0) std::cerr << " (pid " << peer.pid << ")";
    if (!peer.queryFile.empty()) std::cerr << ", queries: " << peer.queryFile;
    std::cerr << "\nxretractor: use --name <name> to run a second, independent instance\n";
    SPDLOG_ERROR("Cannot acquire service lock, {} is already running (pid {}).", ownerLabel(earlyServerName), peer.pid);
    return system::errc::no_lock_available;
  }

  bus::Bus xrdbbus;
  const std::vector<std::string> claimedStreams = claimedStreamNames(coreInstance);
  const std::string counterPath                 = normalizedRotationCounterPath(coreInstance);
  const SystemdIdentity systemd                 = detectSystemdIdentity();
  // Sciezka BEZWZGLEDNA, tak samo jak w pliku blokady (setServiceQueryFile wyzej). Slot czyta
  // operator z innego katalogu roboczego niz serwer, wiec `xqry --servers` z pozycja wzgledna
  // wskazywalby plik, ktorego pod ta nazwa u niego nie ma.
  const std::string queryFile = vm.contains("queryfile") ? absolutePathOf(vm["queryfile"].as<std::string>()) : std::string{};
  // Tryb pracy jest wlasnoscia URUCHOMIENIA, nie planu: dwa serwery na tym samym pliku zapytan
  // moga liczyc raz z zegarem, raz offline. Operator widzi wiec w `xqry --servers` to, co
  // wybrala linia polecen, a nie to, co da sie odczytac z .rql.
  //
  // Serwisem jest zarowno instancja z --service (log do journald), jak i ta wykryta jako
  // jednostka systemd: dla patrzacego na tabele to jeden fakt -- "tego nie zabijaj recznie".
  const std::uint32_t runModes = (vm.contains("realtime") ? bus::mode::kRealTime : 0U) |
                                 (vm.contains("no-clock") ? bus::mode::kNoClock : 0U) |
                                 (vm.contains("until-eof") ? bus::mode::kUntilEof : 0U) |
                                 (loopLimitVar != executorsm::inifitie_loop ? bus::mode::kLoopLimit : 0U) |
                                 (vm.contains("xqrywait") ? bus::mode::kXqryWait : 0U) |
                                 (vm.contains("service") || systemd.unit.has_value() ? bus::mode::kService : 0U);
  const bus::ClaimResult claimed = xrdbbus.claim({.name        = earlyServerName,
                                                  .queryFile   = queryFile,
                                                  .unit        = systemd.unit.value_or(std::string{}),
                                                  .counterPath = counterPath,
                                                  .modes       = runModes,
                                                  .streams     = claimedStreams});

  switch (claimed.status) {
    case bus::ClaimStatus::Claimed:
      break;
    case bus::ClaimStatus::Conflict: {
      const std::string owner = ownerLabel(claimed.ownerName);
      std::cerr << "xretractor: stream '" << claimed.stream << "' is already served by " << owner << " (pid " << claimed.ownerPid
                << ")\n";
      SPDLOG_ERROR("Stream '{}' is already served by {} (pid {}).", claimed.stream, owner, claimed.ownerPid);
      return system::errc::device_or_resource_busy;
    }
    case bus::ClaimStatus::CounterConflict: {
      const std::string owner = ownerLabel(claimed.ownerName);
      std::cerr << "xretractor: rotation counter file '" << claimed.detail << "' is already used by " << owner << " (pid "
                << claimed.ownerPid << ")\n";
      SPDLOG_ERROR("Rotation counter file '{}' is already used by {} (pid {}).", claimed.detail, owner, claimed.ownerPid);
      return system::errc::device_or_resource_busy;
    }
    case bus::ClaimStatus::TooLarge:
    case bus::ClaimStatus::NoFreeSlot:
      std::cerr << "xretractor: cannot register on the xrdbbus bus: " << claimed.detail << '\n';
      SPDLOG_ERROR("Cannot register on the xrdbbus bus: {}", claimed.detail);
      return system::errc::device_or_resource_busy;
    case bus::ClaimStatus::Unavailable:
      // Utrzymujemy dotychczasowa decyzje fail-open. Blokada instancji nadal chroni jej IPC,
      // ale przy niedostepnej magistrali rozlacznosc zasobow miedzy nazwami nie jest wymuszana.
      SPDLOG_WARN("xrdbbus unavailable ({}); stream name uniqueness is NOT enforced.", claimed.detail);
      break;
  }

  signal(SIGINT, handleSignal);   // Ctrl+C
  signal(SIGTERM, handleSignal);  // Terminate
  signal(SIGHUP, handleSignal);   // Hangup

  bool rotation_enabled = std::ranges::any_of(coreInstance, [](const auto &it) { return it.id == ":ROTATION"; });

  if (!rotation_enabled) {
    std::string storage_location;

    for (const auto &it : coreInstance)
      if (it.id == ":STORAGE") {
        storage_location = it.filename;
      }

    // Nazwa zwracana przez parser jest nazwa Z ZAPISU, a ta nie musi byc nazwa zapytania
    // w planie: generator `STREAM cell[24]` daje jedna linie RQL i 24 strumienie `cell$0`..
    // `cell$23`, a samego `cell` w planie nie ma. Rodziny bierzemy z kompilatora, bo to
    // jedyne pewne zrodlo — patrz compiler::generatedStreams().
    const auto &generatedStreams = cm.generatedStreams();
    for (const auto &[stream_id, query_text] : processedLines) {
      if (stream_id.empty()) continue;

      const auto family = generatedStreams.find(stream_id);
      const std::vector<std::string> definedStreams =
          (family != generatedStreams.end()) ? family->second : std::vector<std::string>{stream_id};

      for (const auto &defined_id : definedStreams) {
        if (coreInstance[defined_id].isDeclaration()) continue;
        if (coreInstance[defined_id].isCompilerDirective()) continue;
        dropArtifactFile(std::filesystem::path(storage_location) / defined_id);
        dropArtifactFile(std::filesystem::path(storage_location) / (defined_id + ".desc"));
        dropArtifactFile(std::filesystem::path(storage_location) / (defined_id + ".meta"));
      }
    }
  }

  executorsm exec;
  return exec.run(coreInstance, guard, xrdbbus, cm, vm, appCfg, earlyServerName);
}
