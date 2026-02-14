#pragma once

#include <map>
#include <memory>  // unique_ptr
#include <vector>

#include "streamInstance.h"  // streamInstance (transitively includes QStruct.h, rdb/payload.h)
class dataModel {
 private:
  qTree &coreInstance_;

  std::map<std::string, std::string> directive_{{":STORAGE", ""}, {":SUBSTRAT", ""}, {":ROTATION", ""}};

 public:
  std::map<std::string, std::unique_ptr<streamInstance>> qSet;

  explicit dataModel(qTree &coreInstance);
  ~dataModel();

  dataModel() = delete;

  bool addQueryToModel(std::string id);

  std::unique_ptr<rdb::payload>::pointer getPayload(const std::string &instance,  //
                                                    const int revOffset = 0);

  /*
   * This function creates Input payload for ConstructOutputPayload data source
   * function need to be here because it access different streams from qSet
   */
  void constructInputPayload(const std::string &instance);

  void processRows(const std::set<std::string> &inSet);
  void processZeroStep();

  std::vector<rdb::descFldVT> getRow(const std::string &instance, const int timeOffset);

  size_t streamStoredSize(const std::string &instance);

  /** This function return length of data stream */
  size_t getStreamCount(const std::string &instance);
};
