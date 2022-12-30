#include <ctype.h>  // for tolower
#include <inputFileInstance.h>

#include <algorithm>                               // for transform
#include <boost/algorithm/string/trim.hpp>         // for trim_right
#include <boost/rational.hpp>                      // for rational, operator>>
#include <boost/type_index.hpp>                    // for type_info
#include <boost/type_index/type_index_facade.hpp>  // for operator==
#include <cassert>                                 // for assert
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>  // for out_of_range

#include "QStruct.h"  // for field, BAD

using namespace boost;

void inputDF::goBegin() {
  psFile->clear();
  psFile->seekg(0, std::ios::beg);
  curPos = 0;
}

template <typename T>
T inputDF::readFromFile() {
  T retVal;
  if (len == -1) throw std::out_of_range("no file connected to object");
  if (extension == ".txt") {
    *psFile >> retVal;
    if (psFile->eof()) {
      goBegin();
      *psFile >> retVal;
    }
  } else {
    if (curPos > (len - sizeof(T))) goBegin();
    psFile->read(reinterpret_cast<char *>(&retVal), sizeof(retVal));
    curPos += sizeof(T);
  }
  return retVal;
}

inputDF::inputDF() {}

inputDF::inputDF(std::string inputFileName, std::list<field> &lSchema)
    : filename(inputFileName), lSchema(lSchema), len(0), curPos(0) {
  extension = std::filesystem::path(inputFileName).extension();
  std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
  // Parser feeds space at the end of string
  boost::trim_right(inputFileName);
  std::ifstream *pstream =
      new std::ifstream(inputFileName, (extension == ".txt") ? std::ios::in : std::ios::in | std::ios::binary);
  psFile.reset(pstream);
  assert(psFile);
  if (!psFile.get())
    len = -1;
  else {
    if (extension == ".dat") {
      psFile->seekg(0, std::ios::end);
      len = psFile->tellg();
    }
    goBegin();
  }
  // Don't throw exception from constructor - Rule!
}

//
// This should _REALLY_ return number
// but now there is a mess with types and
// std::get: wrong index for variant
// us reported if we return someting other
// than rational<int> from this file
//
std::vector<number> inputDF::getFileRow() {
  std::vector<number> result;
  for (auto &f : lSchema) {
    boost::rational<int> val;
    switch (f.fieldType) {
      case rdb::BYTE:
        val = readFromFile<unsigned char>();
        break;
      case rdb::INTEGER:
        val = readFromFile<int>();
        break;
      case rdb::FLOAT:
        val = Rationalize(readFromFile<double>());
        break;
      case rdb::RATIONAL:
        val = readFromFile<boost::rational<int>>();
        break;
      case rdb::BAD:
      default:
        std::cerr << "field:" << f.fieldType << std::endl;
        throw std::out_of_range("processRow/undefined type");
        break;  // proforma
    }
    result.push_back(val);
  }
  return result;
}
