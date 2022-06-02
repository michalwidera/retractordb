#pragma once

#include <boost/core/noncopyable.hpp>        // for noncopyable
#include <boost/rational.hpp>                // for rational
#include <boost/smart_ptr/shared_array.hpp>  // for shared_array
#include <boost/variant/variant.hpp>         // for variant
#include <iosfwd>                            // for ostream
#include <list>                              // for list
#include <string>                            // for string
#include <vector>                            // for vector

#include "QStruct.h"

/** Data of this type are stored in streams */
typedef boost::variant<boost::rational<int>, int, double> number;

/**
 * Class covers CBuffer with own abstraction
 * Access is leaded by stream[1] or stream["field_name"]
 * Access to archival data stream.get(1)[1] or stream.get(1)["field_name"]
 * after store() opeartion data are no longer avaiable by
 * stream[1] or stream["field_name"] but new place is ready for tuple
 */
class dbStream : private boost::noncopyable {
  int frameSize;                      /**< Size of frame in bytes */
  boost::shared_array<char> pRawData; /**< Pointer for binary data */
  std::string streamName;             /**< Stream name */
  std::vector<number> mpRead;
  int mpReadNr, mpLenNr;

  std::list<field> schema;

 public:
  dbStream(std::string streamName, std::list<field> schema);

  // number &operator[](const int &_Keyval);

  void store();
  number readData(int offset = 0, int keyval = 0,
                  bool reverse = false); /**< Get data from archive */
};

long streamStoredSize(std::string filename);

// Set location of all database files - location is a path/folder in system
void setStorageLocation(std::string location);
