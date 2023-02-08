#include "rdb/facctxtsrc.h"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstring>  // memcpy
#include <memory>   // make_uniqe
#include <ratio>

#include "spdlog/spdlog.h"

namespace rdb {
constexpr const int kOpenBaseFlags = O_CLOEXEC;

template <typename K>
K readFromFstream(std::fstream& myFile) {
  K var;
  myFile >> var;
  if (myFile.eof()) {
    myFile.clear();
    myFile.seekg(0, std::ios::beg);
    myFile >> var;
  };
  return var;
}

template <typename T>
textSrouceAccessor<T>::textSrouceAccessor(std::string fileName) : fileNameStr(fileName) {
  myFile.rdbuf()->pubsetbuf(0, 0);
  myFile.open(fileName, std::ios::in);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);
}

template <typename T>
textSrouceAccessor<T>::~textSrouceAccessor() {
  myFile.close();
}

template <class T>
std::string textSrouceAccessor<T>::fileName() {
  return fileNameStr;
}

template <typename T>
int textSrouceAccessor<T>::write(const T* ptrData, const size_t size, const size_t position) {
  // no write on data source supported
  return EXIT_FAILURE;
}

template <typename T>
int textSrouceAccessor<T>::read(T* ptrData, const size_t size, const size_t position) {
  assert(position == 0);
  assert(size == 0);

  // myFile.seekg(position);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);

  // This (+1) look's like error (need investigate)
  // During test posix interface have properly read size data
  // but fstream get read instead 12 bytes only 11
  // Therefore +1 appears.
  // Last byte was omitted.
  // Look's like some inconsistency is here.

  for (auto item : descriptor) {
    auto i = 0;
    if (std::get<rlen>(item) != 0) {
      if (std::get<rtype>(item) == rdb::INTEGER) {
        auto var = readFromFstream<int>(myFile);
        payload->setItem(i, var);
      } else if (std::get<rtype>(item) == rdb::UINT) {
        auto var = readFromFstream<unsigned>(myFile);
        payload->setItem(i, var);
      } else if (std::get<rtype>(item) == rdb::FLOAT) {
        auto var = readFromFstream<float>(myFile);
        payload->setItem(i, var);
      } else if (std::get<rtype>(item) == rdb::DOUBLE) {
        auto var = readFromFstream<double>(myFile);
        payload->setItem(i, var);
      } else if (std::get<rtype>(item) == rdb::BYTE) {  // This is different!
        auto var = readFromFstream<unsigned>(myFile);
        payload->setItem(i, static_cast<unsigned char>(var));
      } else {
        SPDLOG_ERROR("Unsported type in text data source: {}", std::get<rtype>(item));
        abort();
      }

      // rdb::RATIONAL - deprecate ?

      // DECL(STRING),     // TODO
      // DECL(BYTEARRAY),  //
      // DECL(INTARRAY),   //
      i++;
    }
  }
  std::memcpy(reinterpret_cast<char*>(ptrData), payload->get(), descriptor.getSizeInBytes());
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);

  return EXIT_SUCCESS;
}

template <typename T>
int textSrouceAccessor<T>::fctrl(void* ptrData, const size_t size) {
  descriptor = *(reinterpret_cast<rdb::Descriptor*>(ptrData));
  payload = std::make_unique<rdb::payload>(descriptor);
  return EXIT_SUCCESS;
}

template class textSrouceAccessor<std::byte>;
template class textSrouceAccessor<char>;
template class textSrouceAccessor<unsigned char>;

}  // namespace rdb