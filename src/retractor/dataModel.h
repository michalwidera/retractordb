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

struct streamInstance {
  std::unique_ptr<rdb::storageAccessor> storage;
  std::unique_ptr<rdb::payload> internalPayload;

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
};

class dataModel {
 private:
  std::string storagePath{""};

 public:
  std::map<std::string, std::unique_ptr<streamInstance>> qSet;

  dataModel(/* args */);
  ~dataModel();

  void computeInstance(std::string instance);
  void load(std::string compiledQueryFile);
  void prepare();
};
