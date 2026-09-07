#include "executorsmState.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "appConfig.hpp"
#include "executorsm.hpp"
#include "ipcServer.hpp"
#include "persistentCounter.hpp"

// Definicje stanu opisanego w executorsmState.hpp. Komentarze — czym kazdy z tych obiektow
// jest, kto go pisze i pod jakim muteksem — mieszkaja przy deklaracjach w naglowku.

std::unique_ptr<PersistentCounter> pCounterPtr;
std::vector<std::pair<std::string, std::string>> processedLines;
dataModel *pProc = nullptr;
std::atomic<int> iLoopLimitCnt{executorsm::inifitie_loop};

namespace esm {
std::mutex plan_epoch_mutex;
std::condition_variable cv;
std::atomic<bool> dataModelExpected{false};
std::atomic<bool> firstQueryReceived{false};
std::atomic<std::uint64_t> adHocPlanRevision{0};
bool untilEofMode{false};
std::atomic<bool> planResetRequested{false};
std::string pendingPlanText;
std::atomic<bool> planSwapInFlight{false};
std::string serviceQueryFilePath;
IpcServer ipcServer;
FlockServiceGuard *serviceGuardPtr = nullptr;
bus::Bus *busPtr                   = nullptr;
}  // namespace esm

qTree *executorsm::coreInstancePtr = nullptr;
compiler *executorsm::cmPtr        = nullptr;
std::atomic<bool> executorsm::ipcReady{false};
std::atomic<bool> executorsm::ipcFailed{false};
int executorsm::cfgQueueBufferSeconds = appcfg::kDefaultIpcQueueBufferSeconds;
int executorsm::cfgMinQueueElements   = appcfg::kDefaultIpcMinQueueElements;
int executorsm::cfgRtPriority         = appcfg::kDefaultSchedulingRtPriority;
std::string executorsm::cfgStorageDir;
std::string executorsm::activeStorageDir;
