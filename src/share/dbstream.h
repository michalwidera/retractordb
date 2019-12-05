#pragma once

#include <list>
#include <map>
#include <vector>
#include <string>
#include <boost/shared_array.hpp>
#include <boost/variant.hpp>
#include <boost/rational.hpp>
#include <boost/core/noncopyable.hpp>

/** Data of this type are stored in streams */
typedef boost::variant< boost::rational<int>, int, double > number ;

/**
 * Class covers CBuffer with own abstraction
 * Access is leaded by stream[1] or stream["field_name"]
 * Access to archival data stream.get(1)[1] or stream.get(1)["field_name"]
 * after store() opeartion data are no longer avaiable by
 * stream[1] or stream["field_name"] but new place is ready for tuple
 */
class dbStream : private boost::noncopyable {
    
    int frameSize ; /**< Size of frame in bytes */
    boost::shared_array< char >  pRawData ; /**< Pointer for binary data */
    std::string streamName ; /**< Stream name */
    std::list < std::string > schema ; /**< Schema - otherwize, list of field in stream */
    std::vector < number >  mpShadow ;
    std::vector < number >  mpRead ; 
    int mpReadNr,mpLenNr ;

  public:

    dbStream(std::string streamName, std::list < std::string > schema);

    number& operator[](const int& _Keyval);
    number readCache(const int& _Keyval);
    void store();
    bool get( int offset = 0 );
};

void saveStreamsToFile( std::string filename );

long streamStoredSize( std::string filename );

void Dump( std::ostream &os );
