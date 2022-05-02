#pragma once

#include <QStruct.h>           // for field
#include <boost/any.hpp>       // for any
#include <boost/rational.hpp>  // for rational
#include <fstream>             // for ifstream
#include <list>                // for list
#include <memory>              // for shared_ptr
#include <string>              // for string
#include <vector>              // for vector

class inputFileInstance
{

    std::shared_ptr< std::ifstream > psFile ;
    int len ; /**< Length of file in bytes */
    int curPos ; /**< Actual position in file */
    void goBegin();
    std::string extension;

public:

    inputFileInstance(std::string inputFileName) ;
    inputFileInstance() ;

    template < typename T > T get();
};

class inputDF :
    public inputFileInstance
{
    std::vector < boost::any > lResult ;
    std::list < field > lSchema ;

public:

    inputDF();
    inputDF(std::string inputFileName, std::list < field > &lSchema) ;

    /** The purpose of this function is to retrieve a row
     * from the file is created based on the schema saved in the query.
     */
    void processRow() ;

    /** A function that returns the retrieved value from the file
     *  associated with the stream. It iss taken from the lResult buffer
     *  downloaded by processRow. This value is returned in the
     *  order indicated by field parameter does not go by
     *  reference because there is a mess with const
     *  Here, the conversion to CR type also takes place
     */
    boost::rational<int> getCR(field f);
};
