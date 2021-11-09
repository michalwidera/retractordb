#pragma once

#include <boost/core/noncopyable.hpp>        // for noncopyable
#include <boost/rational.hpp>                // for rational
#include <boost/smart_ptr/shared_array.hpp>  // for shared_array
#include <boost/variant/variant.hpp>         // for variant
#include <iosfwd>                            // for ostream
#include <list>                              // for list
#include <string>                            // for string
#include <vector>                            // for vector

/** Data of this type are stored in streams */
typedef boost::variant< boost::rational<int>, int, double > number ;

/**
 * Class covers CBuffer with own abstraction
 * Access is leaded by stream[1] or stream["field_name"]
 * Access to archival data stream.get(1)[1] or stream.get(1)["field_name"]
 * after store() opeartion data are no longer avaiable by
 * stream[1] or stream["field_name"] but new place is ready for tuple
 */
class dbStream : private boost::noncopyable
{

    int frameSize ; /**< Size of frame in bytes */
    boost::shared_array< char >  pRawData ; /**< Pointer for binary data */
    std::string streamName ; /**< Stream name */
    std::list < std::string > schema ; /**< Schema - otherwize, list of field in stream */
    std::vector < number >  mpShadow ;
    std::vector < number >  mpRead ;
    int mpReadNr, mpLenNr ;

public:

    dbStream(std::string streamName, std::list < std::string > schema);

    number &operator[](const int &_Keyval);
    number readCache(const int &_Keyval);
    void store();
    void get(int offset = 0, bool reverse = false); /**< Get data from archive */
};

void saveStreamsToFile(std::string filename);

long streamStoredSize(std::string filename);

void Dump(std::ostream &os);
