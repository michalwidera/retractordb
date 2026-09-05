#pragma once

#include <atomic>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>

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
  /// @param runModes maska bus::mode::* opisujaca TRYB URUCHOMIENIA (nie planu) — publikowana
  ///        w slocie magistrali i odtwarzana przy kazdym przeladowaniu planu.
  int run(qTree &coreInstance, FlockServiceGuard &guard, bus::Bus &xrdbbus, compiler &cm, vm_map &vm, const AppConfig &cfg,
          std::string_view serverName = {}, std::string_view systemdUnit = {}, std::uint32_t runModes = 0);

  enum : std::int8_t { inifitie_loop = 0, stop_now = 1 };

 private:
  static qTree *coreInstancePtr;
  static compiler *cmPtr;
  static int cfgQueueBufferSeconds;
  static int cfgMinQueueElements;
  static int cfgRtPriority;
  // Set from IpcServer's onReady callback once all IPC resources are ready.
  // run() waits on this before publishLockInfo(), so PID appears in the lock file
  // only after IPC is fully initialized and xqry can connect safely.
  static std::atomic<bool> ipcReady;

  static ptree commandProcessor(const ptree &ptInval);
  static ptree collectStreamsParameters();
  static ptree getAdHoc(const std::string &adHocQuery);
  static ptree attachAdHocRule(qTree &coreInstanceCopy, const std::string &streamName);

  // Przeladowanie planu (`xqry --reset`). Transfer jest trzyczesciowy, bo kolejka komend
  // przyjmuje komunikaty do ipc::kQueryQueueMaxMessageSize bajtow, a plan bywa dluzszy.
  static ptree resetBegin(const ptree &ptInval);
  static ptree resetChunk(const ptree &ptInval);
  static ptree resetCommit(const ptree &ptInval);

  /// Sprawdza zestaw RQL tak, jak zrobilby to start: parsowanie, kompilacja i rozlacznosc
  /// nazw wzgledem POZOSTALYCH zywych instancji. Zwraca pusty napis, gdy plan da sie przyjac.
  /// Wolane PRZED dotknieciem planu dzialajacego — odmowa nie zmienia niczego.
  static std::string validatePlanText(const std::string &planText);

  /// Wymienia plan na przyjety wczesniej `pendingPlanText` i przygotowuje kolejna epoke:
  /// nowe roszczenie magistrali, skasowanie artefaktow, licznik rotacji, plik zapytan.
  /// Plan, ktorego nie da sie tu zbudowac, konczy sie epoka PUSTA (tryb bezczynny) —
  /// nigdy smiercia procesu.
  void applyPendingPlan(FlockServiceGuard &guard, bus::Bus &xrdbbus, const AppConfig &cfg);

  std::set<std::string> getAwaitedStreamsSet(CRationalStreamMath::TimeLine &tl, qTree *coreInstancePtr);
  std::string printRowValue(const std::string &query_name);
};
