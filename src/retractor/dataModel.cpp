#include "dataModel.h"

#include <cassert>
#include <regex>

#include "QStruct.h"  // coreInstance
#include "spdlog/spdlog.h"

// ctest -R '^unittest-test-schema'

extern "C" qTree coreInstance;

std::string removeCRLF(std::string input) { return std::regex_replace(input, std::regex("\\r\\n|\\r|\\n"), ""); }

std::any streamComposite::get(int position) { return accessor->getItem(position); };

void streamComposite::set(int position, std::any value) { accessor->setItem(position, value); };

streamComposite::streamComposite(rdb::Descriptor descriptor) {
  payload.reset(new std::byte[descriptor.getSizeInBytes()]);
  accessor.reset(new rdb::payLoadAccessor(descriptor, payload.get(), noHexFormat));
};

streamInstance::streamInstance(          //
    const std::string file,              //
    const rdb::Descriptor descInternal,  //
    const rdb::Descriptor descExternal   //
) {
  storage.reset(new rdb::DataStorageAccessor(file));
  storage->createDescriptor(descExternal);
  external.reset(new streamComposite(descExternal));
  internal.reset(new streamComposite(descInternal));

  {
    std::stringstream strStream;
    strStream << storage->getDescriptor();
    SPDLOG_INFO("storage descriptor: {}", removeCRLF(strStream.str()));
  }
  {
    std::stringstream strStream;
    strStream << external->accessor->getDescriptor();
    SPDLOG_INFO("external descriptor: {}", removeCRLF(strStream.str()));
  }
  {
    std::stringstream strStream;
    strStream << internal->accessor->getDescriptor();
    SPDLOG_INFO("internal descriptor: {}", removeCRLF(strStream.str()));
  }
};

streamInstance::streamInstance(const std::string file) {
  storage.reset(new rdb::DataStorageAccessor(file));
  external.reset(new streamComposite(storage->getDescriptor()));
  // assert(storage->getDescriptor() == coreInstance[file].descriptorExpression());

  std::stringstream strStream;
  strStream << storage->getDescriptor();
  SPDLOG_INFO("storage descriptor: {}", removeCRLF(strStream.str()));

  // strStream.clear();
  // strStream << coreInstance[file].descriptorFrom();
  // SPDLOG_INFO("descriptorFrom descriptor: {}", strStream.str());

  // strStream.clear();
  // strStream << coreInstance[file].descriptorExpression();
  // SPDLOG_INFO("descriptorExpression descriptor: {}", strStream.str());

  // internal.reset(new streamComposite(coreInstance[file].descriptorFrom()));
};
