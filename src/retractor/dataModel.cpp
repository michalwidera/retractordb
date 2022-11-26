#include "dataModel.h"

#include <assert.h>
#include <rdb/payloadacc.h>  // rdb::payLoadAccessor

#include "QStruct.h"

extern "C" qTree coreInstance;

enum { noHexFormat = false, HexFormat = true };

streamInstance::streamInstance(const std::string file) {
  // Ta sekwencja otworzy plik danych i deskrypor
  storage.reset(new rdb::DataStorageAccessor(file));
  payload.reset(new std::byte[storage->getDescriptor().GetSize()]);
  accessor.reset(new rdb::payLoadAccessor(storage->getDescriptor(),
                                          payload.get(), noHexFormat));
  internalView.reset(
      new std::byte[coreInstance[file].getInternalDescriptor().GetSize()]);
  internalAccessor.reset(
      new rdb::payLoadAccessor(coreInstance[file].getInternalDescriptor(),
                               internalView.get(), noHexFormat));
};

std::any streamInstance::getPublic(int position) {
  return accessor->get_item(position);
}

void streamInstance::setPublic(int position, std::any value) {
  accessor->set_item(position, value);
}