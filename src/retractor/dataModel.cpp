#include "dataModel.h"

#include <rdb/payloadacc.h>  // rdb::payLoadAccessor

enum { noHexFormat = false, HexFormat = true };

streamInstance::streamInstance(const std::string file) {
  // Ta sekwencja otworzy plik danych i deskrypor
  uPtr_storage.reset(new rdb::DataStorageAccessor(file));
  uPtr_payload.reset(new std::byte[uPtr_storage->getDescriptor().GetSize()]);
  uPtr_plAcces.reset(new rdb::payLoadAccessor(externalDataDescriptor,
                                              uPtr_payload.get(), noHexFormat));
  // internalData.resize(internalDataDescriptor.size());
  // externalData.resize(externalDataDescriptor.size());
};
