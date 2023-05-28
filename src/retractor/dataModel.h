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
  std::unique_ptr<rdb::storageAccessor> storage;  // here is payload that will be stored - select clause
  std::unique_ptr<rdb::payload> fromPayload;      // payload used for computation in select
                                                  // clause - created by from clause.

  // This constructor cover issue when storage name is different from descriptor name
  streamInstance(const std::string descriptorName,           //
                 const std::string storageName,              // <- query %% filename
                 const rdb::Descriptor storageDescriptor,    // <- query %% descriptorStorage()
                 const rdb::Descriptor internalDescriptor);  // <- query %% descriptorFrom()

  // This constructor cover same name for storage and descriptor file name (+.desc)
  streamInstance(const std::string idAndStorageName,         // <- query %% filename
                 const rdb::Descriptor storageDescriptor,    // <- query %% descriptorStorage()
                 const rdb::Descriptor internalDescriptor);  // <- query %% descriptorFrom()

  // This constructor will create data based on QStruct query
  streamInstance(query &qry);

  rdb::payload constructAgsePayload(int length, int offset);
  rdb::payload constructAggregate(command_id cmd, std::string name);

  // constructStoragePayload uses only data from fromPayload
  void constructStoragePayload(const std::list<field> &fields);
};

class dataModel {
 private:
  std::string storagePath{""};
  qTree &coreInstance;

 public:
  std::map<std::string, std::unique_ptr<streamInstance>> qSet;

  dataModel(qTree &coreInstance);
  ~dataModel();

  dataModel() = delete;

  std::unique_ptr<rdb::payload>::pointer getPayload(std::string instance, int revOffset = 0);

  // constructFromPayload function need to be here because it access different streams from qSet
  void constructFromPayload(std::string instance);

  void storeInstance(std::string instance);
  void processRows(std::set<std::string> inSet);
};
