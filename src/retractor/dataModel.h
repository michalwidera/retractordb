#pragma once

#include <QStruct.h>         // query
#include <rdb/descriptor.h>  // rdb::Descriptor
#include <rdb/payload.h>     // rdb::payload
#include <rdb/storageacc.h>  // rdb::storageAccessor

#include <any>
#include <map>
#include <memory>  // unique_ptr
#include <string>
#include <vector>

struct dataInstance {
  std::unique_ptr<rdb::storageAccessor> storage;
  std::unique_ptr<rdb::payload> storagePayload;
  std::unique_ptr<rdb::payload> internalPayload;

  dataInstance(const std::string descriptorname,           //
               const std::string storagename,              // <- query %% filename
               const rdb::Descriptor storageDescriptor,    // <- query %% descriptorExpression()
               const rdb::Descriptor internalDescriptor);  // <- query %% descriptorFrom()

  dataInstance(query &qry);
};
