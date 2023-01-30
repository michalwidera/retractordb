#pragma once

#include <rdb/descriptor.h>  // rdb::Descriptor
#include <rdb/payloadacc.h>  // rdb::payloadAccessor
#include <rdb/storageacc.h>  // rdb::storageAccessor

#include <any>
#include <map>
#include <memory>  // unique_ptr
#include <string>
#include <vector>

struct streamInstance {
  std::unique_ptr<rdb::storageAccessor<>> storage;  // have own payload and descriptor
  std::unique_ptr<rdb::payloadAccessor> accessorStorage;

  std::unique_ptr<std::byte[]> payloadInternal;
  std::unique_ptr<rdb::payloadAccessor> accessorInternal;  // here is descriptor stored

  streamInstance(const std::string file,               // <- query %% filename
                 const rdb::Descriptor descStorage,    // <- query %% descriptorExpression()
                 const rdb::Descriptor descInternal);  // <- query %% descriptorFrom()
};

struct dataModel : public std::map<std::string, streamInstance> {};
