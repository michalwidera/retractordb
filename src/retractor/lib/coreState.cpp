#include <memory>

#include "executorsmState.hpp"
#include "persistentCounter.hpp"

// Stan, ktory rdzen silnika (biblioteka retractorcore) dzieli z usluga: licznik :ROTATION czyta
// streamInstance, model biezacej epoki - dumpManager. Wydzielony z executorsmState.cpp (#446, C3),
// zeby silnik osadzony (rdbembed) nie linkowal stanu uslugi - globalnego IpcServer, magistrali i
// flag watku komunikacyjnego. core_mutex definiuje dataModel.cpp. Sam ten stan jest nadal stanem
// PROCESU, wiec dwa silniki w jednym procesie go dziela - silnik osadzony odrzuca :ROTATION i
// reguly DUMP w compile() i nie publikuje pProc.

std::unique_ptr<PersistentCounter> pCounterPtr;
dataModel *pProc = nullptr;
