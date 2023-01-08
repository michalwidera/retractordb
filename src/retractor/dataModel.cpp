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

streamInstance::streamInstance(         //
    const std::string file,             //
    const rdb::Descriptor descStorage,  //
    const rdb::Descriptor descInternal  //
) {
  storage.reset(new rdb::DataStorageAccessor(file));
  storage->createDescriptor(descStorage);
  internal.reset(new streamComposite(descInternal));

  {
    std::stringstream strStream;
    strStream << storage->getDescriptor();
    SPDLOG_INFO("storage/external descriptor: {}", removeCRLF(strStream.str()));
  }
  {
    std::stringstream strStream;
    strStream << internal->accessor->getDescriptor();
    SPDLOG_INFO("image/internal descriptor: {}", removeCRLF(strStream.str()));
  }
};
