#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>  // support for basic file logging
#include <spdlog/spdlog.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>

#include "config.h"  // Add an automatically generated configuration file
#include "lib/appConfig.hpp"
#include "lib/compiler.hpp"
#include "lib/executor_rt.hpp"
#include "lib/executorsm.hpp"
#include "lib/lockManager.hpp"
#include "lib/persistentCounter.hpp"
#include "lib/presenter.hpp"
#include "lib/qTree.hpp"
#include "lib/serviceControl.hpp"
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
/// - Logować istotne informacje o działaniu programu, błędach i wynikach do pliku logów.
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
/// - Nie emitować kodów ANSI/kolorów, gdy wyjście nie jest terminalem (brak TTY) — log do journala musi być
///   czystym tekstem.
/// - Nie wykonywać własnej rotacji ani retencji plików logów — pozostawić to menedżerowi journald.
/// - Zapewniać natychmiastowy zrzut (flush) po każdej linii, aby wpisy pojawiały się w dzienniku na bieżąco.
/// - Opcjonalnie mapować poziom logowania na priorytety syslog (prefiks `<0>`..`<7>` wg sd-daemon),
///   aby journald poprawnie klasyfikował wagę komunikatów.
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

int main(int argc, char *argv[]) {
  qTree coreInstance;
  compiler cm(coreInstance);

  fixArgcv(argc, argv);

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

#ifdef RDB_BENCH_PROBE
  // Kompilacja z włączoną sondą pomiarową (E1/E3). Ostrzeżenie trafia do logu, a w trybie
  // usługowym (-j) do journald — operator usługi widzi, że to build benchmarkowy, nie produkcyjny.
  SPDLOG_WARN("[warning: probe benchmark build] measurement probe compiled in (RDB_BENCH_PROBE) — NOT for production.");
#endif

  // Wczesny skan argumentów: ścieżka --config musi być znana przed konstruowaniem FlockServiceGuard,
  // aby lock dir z config trafił do guard przed acquireLock().
  std::optional<std::string> earlyConfigPath;
  for (int i = 0; i < argc - 1; ++i) {
    if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--config") == 0) {
      earlyConfigPath = argv[i + 1];
      break;
    }
  }
  const AppConfig earlyAppCfg = [&]() -> AppConfig {
    try {
      return loadAppConfig(earlyConfigPath);
    } catch (...) {
      return {};  // błąd zostanie powtórzony i zgłoszony niżej z właściwym komunikatem
    }
  }();

  namespace po = boost::program_options;
  po::variables_map vm;
  po::options_description desc("Available options");

  bool onlyCompile{false};
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--onlycompile") == 0) onlyCompile = true;
  }

  const std::string serviceName = std::string(argv[0]) + "_service";
  FlockServiceGuard guard(serviceName);
  guard.setLockDir(earlyAppCfg.lockDir);

  int loopLimitVar{executorsm::inifitie_loop};
  AppConfig appCfg = earlyAppCfg;
  try {
    std::string sInputFile;
    std::string sDiagram;
    std::string sConfig;
    if (onlyCompile) {
      desc.add_options()                                                             //
          ("help,h", "show help options")                                            //
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
          ("onlycompile,c", "compile only mode")                                  // linking inheritance from launcher
          ("queryfile,q", po::value<std::string>(&sInputFile), "query set file")  //
          ("quiet,r", "no output on screen, skip presenter")                      //
          ("status,s", "check service status")                                    //
          ("verbose,v", "verbose mode (show stream params)")                      //
          ("xqrywait,x", "wait with processing for first query")                  //
          ("noanykey,k", "do not wait for any key to terminate")                  //
          ("service,j", "service mode: log to stderr (journald), no log file")    //
          ("realtime,t", "enable real-time scheduling (SCHED_FIFO, mlockall, absolute wakeup)")     //
          ("config,g", po::value<std::string>(&sConfig), "config file (TOML); overrides search")    //
          ("llimitqry,m", po::value<int>(&loopLimitVar)->default_value(executorsm::inifitie_loop),  //
           "loop iteration limit, 0 - no limit")                                                    //
          ;
    }
    po::positional_options_description p;  // Assume that infile is the first option
    p.add("queryfile", -1);
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

    po::notify(vm);

    appCfg = loadAppConfig(vm.contains("config") ? std::optional<std::string>(sConfig) : std::nullopt);
    validateConfiguredStorageDir(appCfg);

    iLoopLimitCnt = loopLimitVar;  // std::atomic assignment

    if (vm.contains("status")) {
      std::cout << "Checking service status." << '\n';
      bool isRunning = guard.isAnotherInstanceRunning();
      std::cout << serviceName << ": " << (isRunning ? "Running" : "Stopped") << '\n';
      return isRunning ? system::errc::no_lock_available : system::errc::success;
    }

    if (vm.contains("help")) {
#ifdef RDB_BENCH_PROBE
      std::cout << "[warning: probe benchmark build]" << '\n' << '\n';
#endif
      std::cout << argv[0] << " - compiler & data processing tool." << '\n' << '\n';
      std::cout << "Usage: " << argv[0];
      if (onlyCompile) std::cout << " -c";
      std::cout << " queryfile [option]" << '\n' << '\n';
      std::cout << desc;
      if (!onlyCompile)
        std::cout << "Config search: /etc/retractor/retractor.toml, "
                     "$XDG_CONFIG_HOME (or ~/.config)/retractor/retractor.toml (optional)"
                  << '\n';
      std::cout << config_line << '\n';
      std::cout << "Log: " << tempLocation << '\n';
      if (vm.contains("realtime")) rtCheckAndPrint();
      std::cout << warranty << '\n';
      return system::errc::success;
    }

    // Brak pliku z zapytaniami: w trybie --onlycompile to błąd (nie ma czego kompilować),
    // w trybie usługowym oznacza start bezczynny (idle) — pomijamy parsowanie i kompilację.
    if (!vm.contains("queryfile")) {
      if (onlyCompile) {
        std::cout << argv[0] << ": fatal error: no input file" << '\n';
        return EPERM;  // ERROR defined in errno-base.h
      }
      SPDLOG_INFO("No query file provided; starting in idle (service) mode.");
    } else {
      if (!std::filesystem::exists(sInputFile)) {
        std::cout << argv[0] << ": fatal error: file " << sInputFile << " does not exist." << '\n';
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
          std::cout << "Query compiled OK and sent to running service '" << peer.unit << "' (restart requested)." << '\n';
          return system::errc::success;
        }
        // Inna instancja to zwykły proces (lub nierozpoznana) — dalsza ścieżka (exec.run) zgłosi
        // brak dostępnej blokady (no_lock_available); nie próbujemy restartu.
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

    for (const auto &[stream_id, query_text] : processedLines) {
      if (stream_id.empty()) continue;
      if (coreInstance[stream_id].isDeclaration()) continue;
      if (coreInstance[stream_id].isCompilerDirective()) continue;
      dropArtifactFile(std::filesystem::path(storage_location) / stream_id);
      dropArtifactFile(std::filesystem::path(storage_location) / (stream_id + ".desc"));
      dropArtifactFile(std::filesystem::path(storage_location) / (stream_id + ".meta"));
    }
  }

  executorsm exec;
  return exec.run(coreInstance, guard, cm, vm, appCfg);
}
