#include "dataModel.h"
#include "spdlog/spdlog.h"
#include <cassert>

#include "QStruct.h"  // coreInstance

// ctest -R '^unittest-test-schema'

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
  //assert(storage->getDescriptor() == coreInstance[file].descriptorExpression());

  std::stringstream strStream;
  strStream << storage->getDescriptor();
  SPDLOG_INFO("storage descriptor: {}", strStream.str());

  //strStream.clear();
  //strStream << coreInstance[file].descriptorFrom();
  //SPDLOG_INFO("descriptorFrom descriptor: {}", strStream.str());

  //strStream.clear();
  //strStream << coreInstance[file].descriptorExpression();
  //SPDLOG_INFO("descriptorExpression descriptor: {}", strStream.str());

  //internal.reset(new streamComposite(coreInstance[file].descriptorFrom()));
};
