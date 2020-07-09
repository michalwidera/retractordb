#include <inputFileInstance.h>
#include <assert.h>                                // for assert
#include <ctype.h>                                 // for tolower
#include <algorithm>                               // for transform
#include <boost/algorithm/string/trim.hpp>         // for trim_right
#include <boost/filesystem/convenience.hpp>        // for extension
#include <boost/rational.hpp>                      // for rational, operator>>
#include <boost/type_index.hpp>                    // for type_info
#include <boost/type_index/type_index_facade.hpp>  // for operator==
#include <stdexcept>                               // for out_of_range
#include "QStruct.h"                               // for field, field::BAD

#include <iostream>

using namespace std ;
using namespace boost ;

void inputFileInstance::goBegin() {
    if (extension == ".dat") {
        psFile->clear();
        psFile->seekg(0, ios::beg);
    }

    curPos = 0 ;
}

inputFileInstance::inputFileInstance(std::string inputFileName)
    : len(0), curPos(0) {
    extension = boost::filesystem::extension(inputFileName);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    // Parser feeds space at the end of string
    boost::trim_right(inputFileName);
    ifstream* pstream = new ifstream(
        inputFileName,
        (extension == ".txt") ? ios::in : ios::in | ios::binary
    );
    psFile.reset(pstream);
    assert(psFile);

    if (! psFile.get()) {
        len = -1 ;
    } else {
        if (extension == ".dat") {
            psFile->seekg(0, ios::end);
            len = psFile->tellg();
        }

        goBegin();
    }

    //Don't throw exception from constructor - Rule!
}

inputFileInstance::inputFileInstance() : len(0), curPos(0) {
}

template < typename T >
T inputFileInstance::get() {
    T retVal ;

    if (len == -1) {
        throw std::out_of_range("no file connected to object");
    }

    if (extension == ".txt") {
        *psFile >> retVal ;

        if (psFile->eof()) {
            goBegin();
            *psFile >> retVal ;
        }
    } else {
        if (curPos > (len - sizeof(T))) {
            goBegin();
        }

        psFile->read(reinterpret_cast<char*>(&retVal), sizeof retVal);
        curPos += sizeof(T) ;
    }

    return retVal ;
}

template unsigned char inputFileInstance::get<unsigned char>() ;
template int inputFileInstance::get<int>() ;
template boost::rational<int> inputFileInstance::get<boost::rational<int>>() ;

//-------------------------------------------------------------
//input DISK FILE - part
//-------------------------------------------------------------

inputDF::inputDF() : inputFileInstance() {
}

inputDF::inputDF(std::string inputFileName, std::list < field > &lSchema) :
    inputFileInstance(inputFileName),
    lSchema(lSchema) {
}

void inputDF::processRow() {
    lResult.clear();

    for (auto &f : lSchema) {
        switch (f.dFieldType) {
            case field::BYTE:
                lResult.push_back(get<unsigned char>());
                break ;

            case field::INTEGER:
                lResult.push_back(get<int>()) ;
                break ;

            case field::RATIONAL:
                lResult.push_back(get<boost::rational<int>>());
                break ;

            case field::BAD:
            default:
                std::cerr << "field:" << f.getFirstFieldName() << std::endl ;
                throw std::out_of_range("processRow/undefined type");
                break ; //proforma
        }
    }
}

boost::rational<int> inputDF::getCR(field f) {
    boost::rational<int> retValue(0) ;
    int cnt(0);

    for (auto &v : lSchema) {
        if (v.getFirstFieldName() == f.getFirstFieldName()) {
            break ;
        }

        cnt++ ;
    }

    if (lResult.size() == 0) {
        processRow();
    }

    assert(lResult.size() != 0);

    if (lResult[ cnt ].type() == typeid(unsigned char)) {
        retValue  = any_cast<unsigned char> (lResult[ cnt ]);
    } else if (lResult[ cnt ].type() == typeid(int)) {
        retValue = any_cast<int> (lResult[ cnt ]);
    } else if (lResult[ cnt ].type() == typeid(boost::rational<int>)) {
        retValue = any_cast<boost::rational<int>> (lResult[ cnt ]);
    } else {
        throw std::out_of_range("getCR/undefined type");
    }

    return retValue ;
}
