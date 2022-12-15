#include "dataModel.h"

#include <assert.h>
#include <rdb/payloadacc.h>  // rdb::payLoadAccessor

#include "QStruct.h"

extern "C" qTree coreInstance;

enum { noHexFormat = false, HexFormat = true };

streamInstance::streamInstance(const std::string file) {
  // Ta sekwencja otworzy plik danych i deskrypor
  storage.reset(new rdb::DataStorageAccessor(file));
  publicSchema.payload.reset(new std::byte[storage->getDescriptor().GetSize()]);
  publicSchema.accessor.reset(new rdb::payLoadAccessor(storage->getDescriptor(),
                                          publicSchema.payload.get(), noHexFormat));
  internalSchema.payload.reset(
      new std::byte[coreInstance[file].getInternalDescriptor().GetSize()]);
  internalSchema.accessor.reset(
      new rdb::payLoadAccessor(coreInstance[file].getInternalDescriptor(),
                               internalSchema.payload.get(), noHexFormat));
};
