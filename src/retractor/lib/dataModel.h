#pragma once

#include <any>
#include <map>
#include <memory>  // unique_ptr
#include <string>
#include <vector>

#include "QStruct.h"         // qTree
#include "rdb/descriptor.h"  // rdb::Descriptor
#include "rdb/payload.h"     // rdb::payload
#include "streamInstance.h"  // streamInstance
class dataModel {
 private:
  qTree &coreInstance;

  std::map<std::string, std::string> directive{{":STORAGE", ""}, {":SUBSTRAT", ""}, {":ROTATION", ""}};

 public:
  std::map<std::string, std::unique_ptr<streamInstance>> qSet;

  explicit dataModel(qTree &coreInstance);
  ~dataModel();

  dataModel() = delete;

  bool addQueryToModel(std::string id);

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
