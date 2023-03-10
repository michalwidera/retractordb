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

  // This constructor cover issue when storage name is differnt from descriptor name
  dataInstance(const std::string descriptorname,           //
               const std::string storagename,              // <- query %% filename
               const rdb::Descriptor storageDescriptor,    // <- query %% descriptorExpression()
               const rdb::Descriptor internalDescriptor);  // <- query %% descriptorFrom()

  // This constructor cover same name for storge and decriptor file name (+.desc)
  dataInstance(const std::string idAndStorageName,         // <- query %% filename
               const rdb::Descriptor storageDescriptor,    // <- query %% descriptorExpression()
               const rdb::Descriptor internalDescriptor);  // <- query %% descriptorFrom()

  // This constructor will create data based on QStruct query
  dataInstance(query &qry);
};
