#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "../retractor/lib/appConfig.hpp"
#include "config.h"  // Add an automatically generated configuration file
#include "constants.hpp"
#include "qry.hpp"
#include "retractor/lib/bus.hpp"
#include "retractor/lib/serverName.hpp"
#include "serverRouting.hpp"
#include "uxSysTermTools.hpp"

using namespace boost;
using boost::property_tree::ptree;

namespace IPC = boost::interprocess;

static bool waitForServer(int maxSeconds, int pollIntervalMs, std::string_view serverName) {
  const int safeSeconds      = std::max(1, maxSeconds);
  const int safePollInterval = std::max(1, pollIntervalMs);
  const int maxAttempts      = std::max(1, safeSeconds * 1000 / safePollInterval);
  for (int i = 0; i < maxAttempts; ++i) {
    try {
      const ipc::ServerNames names = ipc::names(serverName);
      IPC::managed_shared_memory seg(IPC::open_only, names.shmemSegment.c_str());
      IPC::message_queue mq(IPC::open_only, names.queryQueue.c_str());
      return true;
    } catch (...) {
      std::this_thread::sleep_for(std::chrono::milliseconds(safePollInterval));
    }
  }
  return false;
}

void cleanup() {
  spdlog::shutdown();  // flush logs on disk
}

/// Rozstrzyga instancję docelową dla komendy, która nie dostała jawnego `--server`.
///
/// Kolejność warunków odpowiada kolejności wysyłki w `main()`. To nie jest kosmetyka: gdyby
/// się rozjechały, routing rozstrzygałby według innej komendy niż ta, która faktycznie
/// poleci do serwera — np. `xqry -k -a "..."` zabija serwer, więc musi być rozstrzygany
/// jak `-k`, a nie jak zapytanie ad-hoc.
///
/// Przy dokładnie jednej żywej instancji zwracamy jej nazwę BEZ sprawdzania strumienia.
/// Diagnostyka "nie ma takiego strumienia" należy wtedy do serwera, dokładnie tak jak przed
/// etapem 2c — dzięki temu żaden istniejący test integracyjny nie wymaga poprawki.
static routing::Resolution resolveTarget(const boost::program_options::variables_map &vm,
                                         const std::vector<bus::InstanceInfo> &instances, int elemLimit,
                                         const std::string &stream, const std::string &detail, const std::string &adHoc) {
  if (instances.size() <= 1) return routing::forSingleTarget(instances);
  if (vm.contains("hello") || (vm.contains("kill") && elemLimit == 0) || vm.contains("dir"))
    return routing::forSingleTarget(instances);
  // Przeladowanie planu dotyczy CALEJ instancji, a nie strumienia — a instancja bezczynna
  // nie serwuje zadnego strumienia, wiec po strumieniu nie da sie jej wskazac. Przy wiecej
  // niz jednej zywej instancji `--reset` wymaga wiec jawnego `--server`.
  if (vm.contains("reset")) return routing::forSingleTarget(instances);
  if (vm.contains("adhoc") && !adHoc.empty()) return routing::forAdHoc(instances, adHoc);
  if (vm.contains("detail")) return routing::forStream(instances, detail);
  if (vm.contains("select") && stream != "none") return routing::forStream(instances, stream);
  return routing::forSingleTarget(instances);
}

int main(int argc, char *argv[]) {
  fixArgcv(argc, argv);
  const auto tempLocation = setupLoggerMain(std::string(argv[0]), true);
  /* const int result_atexit = */
  std::atexit(cleanup);

  try {
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    int elemLimit{0};
    std::string sInputStream;
    std::string sDetailStream;
    std::string sAdHoc;
    std::string sGnuplotDim;
    std::string sConfig;
    std::string sServerName;
    std::string sResetFile;
    std::tuple<int, int, int> gnuplotDim{0, 0, 0};
    desc.add_options()                                                                       //
        ("select,s", po::value<std::string>(&sInputStream), "show this stream")              //
        ("detail,t", po::value<std::string>(&sDetailStream), "show details of this stream")  //
        ("adhoc,a", po::value<std::string>(&sAdHoc), "adhoc query mode")                     //
        ("reset,q", po::value<std::string>(&sResetFile),
         "replace the whole plan of the target instance with this RQL file")                              //
        ("elimitqry,m", po::value<int>(&elemLimit)->default_value(0), "limit of elements, 0 - no limit")  //
        ("null,n", "if null row appear - skip it in output")                                              //
        ("hello,l", "diagnostic - hello db world")                                                        //
        ("kill,k", "kill xretractor server")                                                              //
        ("dir,d", "list of queries")                                                                      //
        ("yaml,y", "yaml output format for --dir, --detail and --bus")                                    //
        ("raw,r", "raw output mode (default)")                                                            //
        ("graphite,g", "graphite output mode")                                                            //
        ("influxdb,f", "influxDB output mode")                                                            //
        ("gnuplot,p", po::value<std::string>(&sGnuplotDim), "x,y - gnuplot output mode")                  //
        ("gnuplot-rtl,z", "gnuplot output: newest samples on the right (right-to-left scroll)")           //
        ("config,e", po::value<std::string>(&sConfig), "config file (TOML); overrides search")            //
        ("help,h", "produce help message")                                                                //
        ("needctrlc,c", "force ctl+c for stop this tool")                                                 //
        ("wait-server,w", "poll until xretractor server is available before executing command")           //
        ("server,x", po::value<std::string>(&sServerName),
         "target xretractor instance name (default: resolved from the bus)")  //
        ("bus,b", "list live xretractor instances and their streams");
    po::positional_options_description p;  // Assume that select is the first option
    p.add("select", -1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
    po::notify(vm);
    (void)setvbuf(stdout, nullptr, _IONBF, 0);

    const AppConfig appCfg = loadAppConfig(vm.contains("config") ? std::optional<std::string>(sConfig) : std::nullopt);

    // Przestrzen nazw uruchomienia (RDB_NAMESPACE) wskazuje instancje docelowa wprost, wiec
    // musi byc znana PRZED --wait-server: czekanie odpytuje obiekty IPC konkretnej instancji,
    // a bez nazwy czekaloby na obiekty instancji bezimiennej -- czyli w nieskonczonosc.
    // Jawny --server pozostaje nadrzedny.
    const std::string runNamespace = servername::environmentNamespace();
    if (!runNamespace.empty() && !servername::isValid(runNamespace)) {
      std::println(std::cerr, "xqry: invalid {} value '{}': expected [a-z][a-z0-9_-]{{0,{}}}", servername::kNamespaceEnv,
                   runNamespace, servername::kMaxLength - 1);
      return system::errc::invalid_argument;
    }
    if (!vm.contains("server") && !runNamespace.empty()) sServerName = runNamespace;

    // Format wyjścia rozbierany do zmiennych lokalnych, a nie wprost do obiektu `qry`:
    // instancja docelowa jest znana dopiero po odczycie magistrali, więc `qry` powstaje
    // niżej. Walidacja argumentów zostaje tam, gdzie była — przed jakimkolwiek IPC.
    formatMode outputFormatMode{formatMode::RAW};
    bool gnuplotRightToLeft{false};

    if (vm.count("graphite") + vm.count("raw") + vm.count("influxdb") + vm.count("gnuplot") > 1) {
      std::println("Only one output format could be selected.");
      return system::errc::invalid_argument;
    }
    if (vm.contains("graphite")) outputFormatMode = formatMode::GRAPHITE;
    if (vm.contains("raw")) outputFormatMode = formatMode::RAW;
    if (vm.contains("influxdb")) outputFormatMode = formatMode::INFLUXDB;
    if (vm.contains("gnuplot")) {
      outputFormatMode   = formatMode::GNUPLOT;
      gnuplotRightToLeft = vm.contains("gnuplot-rtl");
      std::stringstream ss(sGnuplotDim);

      auto delimetersCnt = std::count_if(sGnuplotDim.begin(), sGnuplotDim.end(), [](char c) { return c == ',' || c == ':'; });

      char c;
      int x    = 0;
      int ymax = 0;
      int ymin = 0;

      if (delimetersCnt == 1) {
        ss >> x >> c >> ymax;  // expected format is x,y or x:y
      } else if (delimetersCnt == 2) {
        ss >> x >> c >> ymin >> c >> ymax;  // expected format is x,ymin,ymax or x:ymin:ymax
      } else {
        std::print(std::cerr, "gnuplot mode need {{x,y}} or {{x,ymin,ymax}} parameters.");
        return system::errc::invalid_argument;
      }

      if (ss.fail() || !ss.eof()) {
        std::print(std::cerr, "gnuplot mode need {{x,y}} or {{x,ymin,ymax}} parameters.");
        return system::errc::invalid_argument;
      }
      if (x <= 0) {
        std::print("gnuplot mode need x > 0.");
        return system::errc::invalid_argument;
      }
      if (ymin >= ymax) {
        std::print("gnuplot mode need ymin < ymax.");
        return system::errc::invalid_argument;
      }
      gnuplotDim = std::make_tuple(x, ymin, ymax);
    }
    if (vm.contains("gnuplot-rtl") && !vm.contains("gnuplot")) {
      std::print(std::cerr, "--gnuplot-rtl requires --gnuplot/-p mode.");
      return system::errc::invalid_argument;
    }
    // Komendy wykluczaja sie wzajemnie, bo rozgalezienie nizej wybiera PIERWSZA pasujaca i
    // milczaco porzuca reszte. Ta cisza jest grozna, bo boost sklada wartosc z krotka opcja:
    // literowka `-yaml` to dla parsera `-y -a ml`, czyli zapytanie ad-hoc "ml" wyslane do
    // serwera zamiast zadanego detalu -- a bledne zapytanie ad-hoc konczy zycie serwera
    // (listener parsera RQL wola exit()). Odmowa zatrzymuje literowke po stronie klienta.
    //
    // `-k` do zbioru NIE nalezy: `-s <strumien> -k -m N` (ubij po budzecie elementow) i
    // `-k -a "..."` to celowe kombinacje, uzywane w testach integracyjnych.
    const auto commandCount = vm.count("select") + vm.count("detail") + vm.count("adhoc") + vm.count("dir") + vm.count("bus") +
                              vm.count("hello") + vm.count("reset");
    if (commandCount > 1) {
      std::println(std::cerr, "xqry: only one command at a time (--select, --detail, --adhoc, --reset, --dir, --bus, --hello)");
      return system::errc::invalid_argument;
    }
    // `-y` jest modyfikatorem formatu, nie komenda: sam z siebie nie wybiera niczego do
    // wypisania. Przy komendzie bez formy YAML (-s, -a, -l, -k) odmawiamy zamiast milczec,
    // bo flaga bez skutku wyglada dla operatora dokladnie tak jak flaga dzialajaca.
    if (vm.contains("yaml") && !vm.contains("dir") && !vm.contains("detail") && !vm.contains("bus")) {
      std::println(std::cerr, "xqry: --yaml/-y requires --dir/-d, --detail/-t or --bus/-b");
      return system::errc::invalid_argument;
    }
    if (vm.contains("wait-server") && !vm.contains("help")) {
      if (!waitForServer(appCfg.timingServerStartupWaitSeconds, appCfg.timingServerStartupPollIntervalMs, sServerName)) {
        SPDLOG_ERROR("server not available after {} seconds", appCfg.timingServerStartupWaitSeconds);
        return system::errc::no_child_process;
      }
    }
    if (vm.contains("help")) {
      std::println("{} - data query tool.\n", argv[0]);
      std::println("Usage: {} [option]\n", argv[0]);
      std::cout << desc;
      std::println("{}", config_line);
      std::println("Log: {}", tempLocation);
      std::println("{}", warranty);
      return system::errc::success;
    }

    // Migawka magistrali: czysty odczyt seqlockiem, bez muteksu i BEZ kontaktu z serwerami.
    // Klient nie zakłada segmentu (`createIfMissing = false`) — jego brak znaczy dokładnie
    // tyle, że żaden serwer nie wystartował.
    const std::vector<bus::InstanceInfo> liveInstances = [] {
      const bus::Bus xrdbbus(bus::segmentName(), /*createIfMissing=*/false);
      return xrdbbus.instances();
    }();

    if (vm.contains("bus")) {
      const std::vector<std::string> lines =
          vm.contains("yaml") ? routing::describeYaml(liveInstances) : routing::describe(liveInstances);
      for (const auto &line : lines)
        std::println("{}", line);
      // Komunikat o pustej magistrali wynika z samej magistrali, a nie z liczby wypisanych
      // wierszy: forma YAML wypisuje dokument `servers: []` takze wtedy, gdy nie ma czego opisac.
      if (liveInstances.empty()) std::println(std::cerr, "xqry: no live xretractor instance");
      return system::errc::success;
    }

    // Jawny `--server` wygrywa zawsze i pomija magistralę: operator, który wskazał instancję
    // palcem, ma dostać dokładnie ją, także wtedy gdy magistrala jest niedostępna. Tak samo
    // ustawiona przestrzeń nazw — jej instancja jest wskazana równie jednoznacznie.
    if (!vm.contains("server") && runNamespace.empty()) {
      const routing::Resolution resolved = resolveTarget(vm, liveInstances, elemLimit, sInputStream, sDetailStream, sAdHoc);
      switch (resolved.status) {
        case routing::Status::Resolved:
          sServerName = resolved.serverName;
          break;
        case routing::Status::StreamNotFound:
          std::println(std::cerr, "xqry: {}", resolved.detail);
          return system::errc::no_such_file_or_directory;
        case routing::Status::Ambiguous:
        case routing::Status::CrossServer:
          std::println(std::cerr, "xqry: {}", resolved.detail);
          return system::errc::invalid_argument;
      }
    }

    qry obj(appCfg.timingQueryNoDataTimeoutMs, appCfg.ipcClientResponseMaxFails, kIpcClientDefaultResponseQueueOpenMaxFails,
            sServerName);
    obj.outputFormatMode   = outputFormatMode;
    obj.gnuplotRightToLeft = gnuplotRightToLeft;

    if (vm.contains("hello")) return obj.hello();
    if (vm.contains("kill") && elemLimit == 0) {
      obj.netClient("kill", "");
    } else if (vm.contains("dir")) {
      std::print("{}", vm.contains("yaml") ? obj.dirYaml() : obj.dir());
    } else if (vm.contains("reset")) {
      // Plik czyta KLIENT, nie serwer: usluga chodzi zwykle na innym koncie (User=retractor
      // w jednostce systemd) i pliku operatora zwyczajnie nie otworzy. Kanalem IPC idzie
      // wiec tresc, a nie sciezka.
      std::ifstream planFile(sResetFile, std::ios::binary);
      if (!planFile.is_open()) {
        std::println(std::cerr, "xqry: cannot open plan file: {}", sResetFile);
        return system::errc::no_such_file_or_directory;
      }
      std::ostringstream planText;
      planText << planFile.rdbuf();
      planFile.close();
      if (obj.reset(planText.str())) return system::errc::protocol_error;
      std::println("Plan accepted by the server and scheduled for loading: {}", sResetFile);
    } else if (vm.contains("adhoc") && !sAdHoc.empty()) {
      if (obj.adhoc(sAdHoc)) return system::errc::no_such_file_or_directory;
    } else if (vm.contains("detail")) {
      auto ret = vm.contains("yaml") ? obj.detailShowYaml(sDetailStream) : obj.detailShow(sDetailStream);
      if (!ret.empty()) {
        std::print("{}", ret);
      } else
        return system::errc::no_such_file_or_directory;
    } else if (vm.contains("select") && sInputStream != "none") {
      // Tryby porażki są rozróżnialne po kodzie wyjścia (issue_215). Przedtem
      // wszystkie kończyły się albo zerem, albo `no_such_file_or_directory`,
      // więc harness nie umiał odróżnić przeciążonego serwera od literówki
      // w nazwie strumienia — a to inna diagnoza i inna naprawa.
      const selectResult result = obj.select(vm, elemLimit, sInputStream, gnuplotDim, obj.gnuplotRightToLeft);
      switch (result) {
        case selectResult::ok:
          break;
        case selectResult::streamNotFound:
          std::println(std::cerr, "xqry: {}: {}", sInputStream, toString(result));
          return system::errc::no_such_file_or_directory;
        case selectResult::serverNoResponse:
          std::println(std::cerr, "xqry: {}: {}", sInputStream, toString(result));
          return system::errc::timed_out;
        case selectResult::clientQueueMissing:
          std::println(std::cerr, "xqry: {}: {}", sInputStream, toString(result));
          return system::errc::no_stream_resources;
        case selectResult::noData:
          std::println(std::cerr, "xqry: {}: {}", sInputStream, toString(result));
          return system::errc::no_message_available;
        case selectResult::noActivePlan:
          std::println(std::cerr, "xqry: {}: {}", sInputStream, toString(result));
          return system::errc::no_such_file_or_directory;
      }
    } else {
      SPDLOG_ERROR("no argument.");
      return EPERM;  // ERROR defined in errno-base.h
    }
  } catch (IPC::interprocess_exception &ex) {
    SPDLOG_ERROR("IPC: {}", ex.what());
    return system::errc::no_child_process;
  } catch (std::exception &e) {
    SPDLOG_ERROR("Std: {}", e.what());
    return system::errc::interrupted;
  }
  return system::errc::success;
}
