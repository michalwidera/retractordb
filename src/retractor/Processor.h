#pragma once

#include <map>
#include <memory>
#include <variant>

//#include "dbstream.h"
#include "inputFileInstance.h"

/** Data of this type are stored in streams */
typedef std::variant<boost::rational<int>, int, double> number;

long streamStoredSize(std::string filename);

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

  /** Variable that contains sources of data */
  std::map<std::string, inputDF> gFileMap;

  /** Length of data streams processed by processor */
  std::map<std::string, int> gStreamSize;

  /** Context variables */
  std::map<std::string, std::vector<number>>
      gContextValMap;  // schema name/record values

  /** Context variables
   *  NOTE: There shoulnd not appear two different stream lengths
   *  Existence of this should be revisited. Probably need to remove.
   */
  std::map<std::string, int> gContextLenMap;

  /** Context functions */
  number getValueProc(std::string streamName, int timeOffset, int schemaOffset,
                      bool reverse = false);

  /** Context functions */
  std::vector<number> getRow(std::string streamName, int timeOffset,
                             bool reverse = false);

  /** Function will return offsets according to stack operations
   *  when A#B offsets A i B are equal , when A+B then A=0, B=0+Size(A)
   */
  int getArgumentOffset(const std::string &streamName,
                        const std::string &streamArgument);

 public:
  /** Query processor works on core varialbe */
  Processor();

  /** String representation of stream */
  std::string printRowValue(const std::string query_name);

  /** This function take in proper moment data to process */
  void processRows(std::set<std::string> inSet);

  /** Main purpose of this function is fullfill all stream fields values
   *  on given moment by  realization all 1v2v3 elements stream programs
   */
  void updateContext(std::set<std::string> inSet);

  /** This function return lenght of data stream */
  int getStreamCount(const std::string query_name);

  /** This function try to roll up argument and reads data from schema
   * This is not finished - need to be fixed */
  number getValueOfRollup(const query &q, int offset, int timeOffset);
};
