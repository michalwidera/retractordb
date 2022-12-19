#include "dataModel.h"

#include <assert.h>
#include <rdb/payloadacc.h>  // rdb::payLoadAccessor

#include "QStruct.h"

extern "C" qTree coreInstance;

streamInstance::streamInstance(const std::string file) {
  // Ta sekwencja otworzy plik danych i deskrypor
  storage.reset(new rdb::DataStorageAccessor(file));
  publicSchema.reset(new streamComposite(storage->getDescriptor()));
  internalSchema.reset(
      new streamComposite(coreInstance[file].getInternalDescriptor()));
};
