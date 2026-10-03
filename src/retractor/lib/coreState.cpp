#include <memory>
#include <mutex>

#include "executorsmState.hpp"
#include "persistentCounter.hpp"

// Stan, ktory rdzen silnika (biblioteka retractorcore) dzieli z demonem. Wydzielony z
// executorsmState.cpp, zeby silnik osadzony (rdbembed) nie linkowal stanu demona - globalnego
// IpcServer, kolejek Boost.Interprocess i flag watku komunikacyjnego. Sam ten stan jest nadal
// stanem PROCESU, wiec dwa silniki w jednym procesie go dziela - znana luka, opisana w
// docs/embedded-realtime-gaps.md (i w docs/core-phase-2.md, sekcja 4).

std::unique_ptr<PersistentCounter> pCounterPtr;
dataModel *pProc = nullptr;

namespace esm {
std::mutex plan_epoch_mutex;
}  // namespace esm
