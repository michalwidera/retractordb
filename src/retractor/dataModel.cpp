#include "dataModel.h"

#include <cassert>

#include "QStruct.h"  // coreInstance

extern "C" qTree coreInstance;

std::any streamComposite::get(int position) { return accessor->getItem(position); };

void streamComposite::set(int position, std::any value) { accessor->setItem(position, value); };

streamComposite::streamComposite(rdb::Descriptor descriptor) {
  payload.reset(new std::byte[descriptor.getSizeInBytes()]);
  accessor.reset(new rdb::payLoadAccessor(descriptor, payload.get(), noHexFormat));
};

streamInstance::streamInstance(const std::string file) {
  storage.reset(new rdb::DataStorageAccessor(file));
  external.reset(new streamComposite(storage->getDescriptor()));
  assert(storage->getDescriptor() == coreInstance[file].descriptorExpression());
  internal.reset(new streamComposite(coreInstance[file].descriptorFrom()));
};
