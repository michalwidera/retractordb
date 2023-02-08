#include "rdb/facctxtsrc.h"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstring>  // memcpy
#include <memory>   // make_uniqe

namespace rdb {
constexpr const int kOpenBaseFlags = O_CLOEXEC;

template <class T>
textSrouceAccessor<T>::textSrouceAccessor(std::string fileName) : fileNameStr(fileName) {
  fd = ::open(fileNameStr.c_str(), O_RDONLY | kOpenBaseFlags, 0644);
}

template <class T>
textSrouceAccessor<T>::~textSrouceAccessor() {
  ::close(fd);
}

template <class T>
std::string textSrouceAccessor<T>::fileName() {
  return fileNameStr;
}

template <class T>
int textSrouceAccessor<T>::write(const T* ptrData, const size_t size, const size_t position) {
  // no write on data source supported
  return EXIT_FAILURE;
}

template <class T>
int textSrouceAccessor<T>::read(T* ptrData, const size_t size, const size_t position) {
  std::fstream myFile;
  myFile.rdbuf()->pubsetbuf(0, 0);
  myFile.open(fileName(), std::ios::in | std::ios::binary);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  myFile.seekg(position);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  // This (+1) look's like error (need investigate)
  // During test posix interface have properly read size data
  // but fstream get read instead 12 bytes only 11
  // Therefore +1 appears.
  // Last byte was omitted.
  // Look's like some inconsistency is here.
  myFile.get(reinterpret_cast<char*>(ptrData), size + 1);
  assert((myFile.rdstate() & std::ifstream::failbit) == 0);
  if ((myFile.rdstate() & std::ofstream::failbit) != 0) return EXIT_FAILURE;
  myFile.close();
  return EXIT_SUCCESS;
}

template <class T>
int textSrouceAccessor<T>::fctrl(void* ptrData, const size_t size) {
  descriptor = *(reinterpret_cast<rdb::Descriptor*>(ptrData));
  return EXIT_SUCCESS;
}
template class textSrouceAccessor<std::byte>;
template class textSrouceAccessor<char>;
template class textSrouceAccessor<unsigned char>;

}  // namespace rdb