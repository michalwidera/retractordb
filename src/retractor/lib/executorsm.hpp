#pragma once

#include <atomic>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "bus.hpp"
#include "compiler.hpp"
#include "CRSMath.hpp"
#include "lockManager.hpp"

#include "appConfig.hpp"

using ptree  = boost::property_tree::ptree;
using vm_map = boost::program_options::variables_map;
struct executorsm {
  /// serverName pusta => tozsamosc historyczna (jeden serwer na maszyne, dotychczasowe nazwy
  /// obiektow IPC). Niepusta => wlasny, rozlaczny obszar IPC tej instancji.
  /// @param systemdUnit nazwa jednostki systemd, gdy proces nia jest; pusta => zwykly proces.
  int run(qTree &coreInstance, FlockServiceGuard &guard, bus::Bus &xrdbbus, compiler &cm, vm_map &vm, const AppConfig &cfg,
          std::string_view serverName = {}, std::string_view systemdUnit = {});

  enum : std::int8_t { inifitie_loop = 0, stop_now = 1 };

 private:
  static qTree *coreInstancePtr;
  static compiler *cmPtr;
  static int cfgQueueBufferSeconds;
  static int cfgMinQueueElements;
  static int cfgRtPriority;
  /// Domyslny katalog magazynu z konfiguracji. Uzywany przy wycenie zasobow planu, ktory
  /// dopiero ma zostac przyjety (`--reset`) i sam nie niesie dyrektywy `:STORAGE`.
  static std::string cfgStorageDir;
  /// Czy plan przyjmowany kanalem `--reset` moze niesc regule `DO SYSTEM`. Domyslnie nie -
  /// uzasadnienie granicy stoi przy sprawdzeniu w validatePlanText.
  static bool cfgUnrestricted;
  /// Katalog magazynu, w ktorym pisze plan DZIALAJACY. Trzymany osobno, bo dataModel usuwa
  /// dyrektywy z drzewa planu - w czasie pracy `:STORAGE` nie da sie juz z niego odczytac,
  /// a zapytanie ad-hoc musi roscic dokladnie te sciezki, ktore powstana na dysku.
  static std::string activeStorageDir;
  // Set from IpcServer's onReady callback once all IPC resources are ready.
  // run() waits on this before publishLockInfo(), so PID appears in the lock file
  // only after IPC is fully initialized and xqry can connect safely.
  static std::atomic<bool> ipcReady;
  // Ustawiane z wywolania zwrotnego onFailure, gdy watek komunikacyjny nie zdolal zbudowac
  // zasobow IPC (najczestszy powod: brak miejsca w /dev/shm na kolejke komend). Bez tej flagi
  // run() czekal na ipcReady BEZ LIMITU CZASU, a watek, ktory mial ja podniesc, juz nie zyl --
  // proces stal w miejscu, nie liczac i nie odpowiadajac (odtworzone na tmpfs 512 KiB).
  static std::atomic<bool> ipcFailed;

  static ptree commandProcessor(const ptree &ptInval);
  static ptree collectStreamsParameters();
  static ptree getAdHoc(const std::string &adHocQuery);
  static ptree attachAdHocRule(qTree &coreInstanceCopy, const std::string &streamName);

  // Przeladowanie planu (`xqry --reset`). Transfer jest trzyczesciowy, bo kolejka komend
  // przyjmuje komunikaty do ipc::kQueryQueueMaxMessageSize bajtow, a plan bywa dluzszy.
  static ptree resetBegin(const ptree &ptInval);
  static ptree resetChunk(const ptree &ptInval);
  static ptree resetCommit(const ptree &ptInval);

  /// Sprawdza zestaw RQL tak, jak zrobilby to start: parsowanie, kompilacja, limity i
  /// rozlacznosc nazw wzgledem POZOSTALYCH zywych instancji. Przy sukcesie rezerwuje zasoby
  /// nowego planu; przy odmowie nie zmienia planu ani jego aktywnych roszczen.
  static std::string validatePlanText(const std::string &planText);

  /// Wymienia plan na przyjety wczesniej `pendingPlanText` i przygotowuje kolejna epoke:
  /// aktywacja rezerwacji magistrali, skasowanie artefaktow, licznik rotacji, plik zapytan.
  /// Plan, ktorego nie da sie tu zbudowac, konczy sie epoka PUSTA (tryb bezczynny) -
  /// nigdy smiercia procesu.
  void applyPendingPlan(FlockServiceGuard &guard, bus::Bus &xrdbbus, const AppConfig &cfg);

  /// Wyznacza strumienie nalezne w biezacym takcie: wypelnia dueMask_ i dueNames_.
  /// Wolajacy MUSI trzymac core_mutex, a maska jest wazna tylko dopoki uklad planu sie
  /// nie zmieni - patrz komentarz przy dueMask_.
  void collectAwaitedStreams(CRationalStreamMath::TimeLine &tl, qTree *coreInstancePtr);
  std::string printRowValue(const std::string &query_name);

  /// Strumienie nalezne w takcie, w DWOCH postaciach, bo konsumenci chca czego innego:
  /// dataModel::processRows pyta o przynaleznosc POZYCJI w planie, a IpcServer::broadcast
  /// ITERUJE po nazwach. Obie struktury sa skladowymi i uzywane ponownie miedzy taktami:
  /// assign/clear zachowuja pojemnosc, wiec po pierwszym takcie nie ma tu alokacji.
  ///
  /// Maska jest POZYCYJNA, wiec wolno jej uzyc tylko wobec tego ukladu planu, z ktorego
  /// powstala. Uklad zmienia import ad hoc (compiler::importFrom + compile, ktore dopisuja
  /// wezly i sortuja topologicznie); import idzie pod plan_epoch_mutex, wiec wypelnienie i
  /// konsumpcja maski musza wypasc pod TA SAMA blokada epoki. Z tego samego powodu widoki
  /// w dueNames_ wskazuja na query::id zywego planu i zyja tylko tak dlugo.
  std::vector<char> dueMask_;
  std::vector<std::string_view> dueNames_;
};
