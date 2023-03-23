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

class streamInstance {
  std::unique_ptr<rdb::payload> localPayload;

 public:
  std::unique_ptr<rdb::storageAccessor> storage;  // here is payload that will be stored - select clause
  std::unique_ptr<rdb::payload> fromPayload;      // payload used for computation in select
                                                  // clause - created by from clause.

  // This constructor cover issue when storage name is different from descriptor name
  streamInstance(const std::string descriptorName,           //
                 const std::string storageName,              // <- query %% filename
                 const rdb::Descriptor storageDescriptor,    // <- query %% descriptorExpression()
                 const rdb::Descriptor internalDescriptor);  // <- query %% descriptorFrom()

  // This constructor cover same name for storage and descriptor file name (+.desc)
  streamInstance(const std::string idAndStorageName,         // <- query %% filename
                 const rdb::Descriptor storageDescriptor,    // <- query %% descriptorExpression()
                 const rdb::Descriptor internalDescriptor);  // <- query %% descriptorFrom()

  // This constructor will create data based on QStruct query
  streamInstance(query &qry);

  void constructPayload(int offset, int length);
};

class dataModel {
 private:
  std::string storagePath{""};

 public:
  std::map<std::string, std::unique_ptr<streamInstance>> qSet;

  dataModel(/* args */);
  ~dataModel();

  std::unique_ptr<rdb::payload>::pointer getPayload(std::string instance, int offset = 0);

  void computeInstance(std::string instance);
  void load(std::string compiledQueryFile);
  void prepare();
};
