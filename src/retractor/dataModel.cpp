#include "dataModel.h"

#include <rdb/payloadacc.h>  // rdb::payLoadAccessor

enum { noHexFormat = false, HexFormat = true };

streamInstance::streamInstance(const std::string file) {
  // Ta sekwencja otworzy plik danych i deskrypor
  storage.reset(new rdb::DataStorageAccessor(file));
  payload.reset(new std::byte[storage->getDescriptor().GetSize()]);
  plAcces.reset(new rdb::payLoadAccessor(externalDataDescriptor, payload.get(),
                                         noHexFormat));
  // internalData.resize(internalDataDescriptor.size());
  // externalData.resize(externalDataDescriptor.size());
};
