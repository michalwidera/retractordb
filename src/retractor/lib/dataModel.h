#pragma once

#include <rdb/descriptor.h>  // rdb::Descriptor
#include <rdb/payload.h>     // rdb::payload
#include <rdb/storageacc.h>  // rdb::storageAccessor

#include <any>
#include <map>
#include <memory>  // unique_ptr
#include <string>
#include <vector>

#include "QStruct.h"  // query

struct streamInstance {
  std::unique_ptr<rdb::storageAccessor> outputPayload;  // here is payload that will be stored - select clause
  std::unique_ptr<rdb::payload> inputPayload;           // payload used for computation in select
                                                        // clause - created by from clause.

  // This constructor cover issue when storage name is different from descriptor name
  explicit streamInstance(const std::string &descriptorName,           //
                          const std::string &storageName,              // <- query %% filename
                          const rdb::Descriptor &storageDescriptor,    // <- query %% descriptorStorage()
                          const rdb::Descriptor &internalDescriptor);  // <- query %% descriptorFrom()

  // This constructor cover same name for storage and descriptor file name (+.desc)
  explicit streamInstance(const std::string &idAndStorageName,         // <- query %% filename
                          const rdb::Descriptor &storageDescriptor,    // <- query %% descriptorStorage()
                          const rdb::Descriptor &internalDescriptor);  // <- query %% descriptorFrom()

  // This constructor will create data based on QStruct query
  explicit streamInstance(query &qry);

  rdb::payload constructAgsePayload(const int length,             //  _@(_,length)
                                    const int step,               //  _@(step,_)
                                    const std::string &instance,  //  instance@(_,_)
                                    const int storedRecordsCountDst);
  rdb::payload constructAggregate(command_id cmd, const std::string &instance);

  /*
   * This function will create OutputPayload based on all field from query
   * constructOutputPayload uses only data from inputPayload
   * inputPayload need to be filled first before this constructOutputPayload will be called.
   */
  void constructOutputPayload(const std::list<field> &fields);
};

class dataModel {
 private:
  std::string storagePath{""};
  qTree &coreInstance;

 public:
  std::map<std::string, std::unique_ptr<streamInstance>> qSet;

  explicit dataModel(qTree &coreInstance);
  ~dataModel();

  dataModel() = delete;

  std::unique_ptr<rdb::payload>::pointer getPayload(const std::string &instance,  //
                                                    const int revOffset = 0);

  bool fetchPayload(const std::string &instance,                     //
                    std::unique_ptr<rdb::payload>::pointer payload,  //
                    const int revOffset = 0);
  /*
   * This function creates Input payload for ConstructOutputPayload data source
   * function need to be here because it access different streams from qSet
   */
  void constructInputPayload(const std::string &instance);

  void fetchDeclaredPayload(const std::string &instance);

  void processRows(const std::set<std::string> &inSet);

  std::vector<rdb::descFldVT> getRow(const std::string &instance, const int timeOffset);

  size_t streamStoredSize(const std::string &instance);

  /** This function return length of data stream */
  size_t getStreamCount(const std::string &instance);
};
