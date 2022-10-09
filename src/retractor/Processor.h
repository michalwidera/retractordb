#pragma once

#include <map>
#include <memory>

#include "inputFileInstance.h"

long streamStoredSize(std::string filename);

/** This function return lenght of data stream */
int getStreamCount(const std::string query_name);

/** Query processor */
class Processor : private boost::noncopyable {
  /** Archive of data streams - initStorage */
  // std::map<std::string, std::shared_ptr<dbStream>> storage;

  /** This function assue data access
   *  Due each field is computed in form schema/query
   *  it schould have access to this query
   *  therefore argument is reference to this query
   *  main solution (we need it make it better)
   */
  boost::rational<int> computeValue(field &f, query &q);

 public:
  /** Query processor works on core varialbe */
  Processor();

  /** This function take in proper moment data to process
   *  Main purpose of this function is fullfill all stream fields values
   *  on given moment by  realization all 1v2v3 elements stream programs
   */
  void processRows(std::set<std::string> inSet);

  /** Context functions */
  std::vector<number> getRow(std::string streamName, int timeOffset,
                             bool reverse = false);
};
