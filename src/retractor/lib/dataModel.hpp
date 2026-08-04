#pragma once

#include <map>
#include <memory>  // unique_ptr
#include <set>
#include <vector>

#include <boost/rational.hpp>

#include "streamInstance.hpp"  // streamInstance (transitively includes qTree.hpp, rdb/payload.hpp)
class dataModel {
 private:
  qTree &coreInstance_;

  std::map<std::string, std::string> directive_{{":STORAGE", ""}, {":SUBSTRAT", ""}, {":ROTATION", ""}};

 public:
  std::map<std::string, std::unique_ptr<streamInstance>> qSet;

  explicit dataModel(qTree &coreInstance);
  ~dataModel();

  dataModel() = delete;

  bool addQueryToModel(const std::string &id);

  std::unique_ptr<rdb::payload>::pointer getPayload(const std::string &instance,  //
                                                    int revOffset = 0);

  /*
   * Rekord strumienia po indeksie POSTĘPUJĄCYM (0-bazowym) na osi czasu źródła —
   * używane przez przeplot (#) i rozplot (&, %), których formuły (SOperations.hpp)
   * zwracają indeksy postępujące. Indeks spoza dostępnego zakresu (przyszłość,
   * poza pojemnością historii) daje rekord all-null.
   */
  rdb::payload fetchForward(const std::string &instance, int forwardIndex);
  rdb::payload fetchBack(const std::string &instance, int revOffset);

  /*
   * This function creates Input payload for ConstructOutputPayload data source
   * function need to be here because it access different streams from qSet
   */
  void constructInputPayload(const std::string &instance);

  void processRows(const std::set<std::string> &inSet, const boost::rational<int> &currentTimeSlot = boost::rational<int>(0));
  void processZeroStep();

  std::vector<rdb::descFldVT> getRow(const std::string &instance, int timeOffset);

  size_t streamStoredSize(const std::string &instance);

  /** This function return length of data stream */
  size_t getStreamCount(const std::string &instance);
};
