#pragma once

#include <rdb/descriptor.h>  // rdb::Descriptor
#include <rdb/payload.h>     // rdb::payload
#include <rdb/storageacc.h>  // rdb::storageAccessor

#include <any>
#include <map>
#include <memory>  // unique_ptr
#include <string>
#include <vector>

struct streamInstance {
  std::unique_ptr<rdb::storageAccessor<>> storage;
  std::unique_ptr<rdb::payload> storagePayload;
  std::unique_ptr<rdb::payload> InternalPayload;

  streamInstance(const std::string file,                     // <- query %% filename
                 const rdb::Descriptor storageDescriptor,    // <- query %% descriptorExpression()
                 const rdb::Descriptor internalDescriptor);  // <- query %% descriptorFrom()
};

struct dataModel : public std::map<std::string, streamInstance> {};
