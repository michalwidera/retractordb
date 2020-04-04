#pragma once

#include <fstream>  // for fstream, ios
#include <string>   // for string

using std::fstream ;
using std::string ;
using std::ios ;

class CRandomFile : public fstream {

    bool destroyOnDestructor ;

  public:

    string filename;

    CRandomFile(string murExt = "mur", const bool destroyOnDestructor = true);
    virtual ~CRandomFile();
    const CRandomFile &operator << (string &text);
};
