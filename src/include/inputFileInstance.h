#pragma once

#include <QStruct.h>  // for field

#include <boost/any.hpp>       // for any
#include <boost/rational.hpp>  // for rational
#include <fstream>             // for ifstream
#include <list>                // for list
#include <memory>              // for shared_ptr
#include <string>              // for string
#include <vector>              // for vector

class inputDF {
  std::vector<boost::any> lResult;
  std::list<field> lSchema;
  std::string filename;
  std::shared_ptr<std::ifstream> psFile;
  int len;    /**< Length of file in bytes */
  int curPos; /**< Actual position in file */
  void goBegin();
  std::string extension;

  template <typename T>
  T readFromFile();

 public:
  inputDF();
  inputDF(std::string inputFileName, std::list<field> &lSchema);

  std::vector<number> getFileRow();
};
