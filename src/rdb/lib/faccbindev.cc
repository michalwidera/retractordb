#include "rdb/faccbindev.hpp"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <unistd.h>  // ::read, ::open ...

#include <cerrno>
#include <cstring>
#include "fatalError.hpp"

namespace rdb {

namespace {
constexpr mode_t kDefaultFileMode = 0644;
}

binaryDeviceRO::binaryDeviceRO(const std::string_view fileName,  //
                               const rdb::Descriptor &descriptor,
                               bool loopToBeginningIfEOF)  //
    : filename_(std::string(fileName)),
      recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),
      descriptor_(descriptor),
      loopToBeginningIfEOF_(loopToBeginningIfEOF),
      lastNullBitset_(descriptor.size(), false) {
  fd_ = ::open(filename_.c_str(), O_RDONLY | O_CLOEXEC, kDefaultFileMode);
  if (fd_ < 0) {
    SPDLOG_WARN("Unable to open binary device source: {}", filename_);
  }
}

binaryDeviceRO::~binaryDeviceRO() {
  if (fd_ >= 0) ::close(fd_);
}

auto binaryDeviceRO::name() -> std::string & { return filename_; }

// Krótki odczyt NIE oznacza końca danych: ::read na FIFO, potoku czy urządzeniu wolno zwrócić mniej
// bajtów, niż zażądano, a EINTR przerywa wywołanie bez utraty czegokolwiek. Rekord składamy więc
// z kolejnych porcji i ponawiamy przerwane wywołanie.
binaryDeviceRO::readOutcome binaryDeviceRO::readExact(uint8_t *ptrData) {
  ssize_t done = 0;
  while (done < recordSize_) {
    const ssize_t readSize = ::read(fd_, ptrData + done, static_cast<size_t>(recordSize_ - done));
    if (readSize > 0) {
      done += readSize;
      continue;
    }
    if (readSize == 0) return readOutcome::endOfFile;
    if (errno == EINTR) continue;
    return readOutcome::error;
  }
  return readOutcome::complete;
}

ssize_t binaryDeviceRO::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  auto markAllNullAndZero = [&](ssize_t status) {
    lastNullBitset_.assign(descriptor_.size(), true);
    if (ptrData != nullptr) {
      std::memset(ptrData, 0, recordSize_);
    }
    nullBitset = lastNullBitset_;
    cnt_++;
    return status;
  };

  if (fd_ < 0) return markAllNullAndZero(EXIT_FAILURE);
  if (recordSize_ == 0) return markAllNullAndZero(EXIT_FAILURE);

  if (position != 0) {
    return markAllNullAndZero(EXIT_FAILURE);
  }

  auto outcome = readExact(ptrData);  // /dev/random no seek supported
  if (outcome == readOutcome::error) return markAllNullAndZero(EXIT_FAILURE);
  if (outcome == readOutcome::endOfFile) {  // dev/random has no seek - but binary files should loop?
    if (!loopToBeginningIfEOF_) {
      // Koniec strumienia bez zawijania to koniec danych — przebieg z --until-eof ma sie tu zatrzymac.
      exhausted_ = true;
      return markAllNullAndZero(EXIT_SUCCESS);
    }
    if (::lseek(fd_, 0, SEEK_SET) < 0) return markAllNullAndZero(EXIT_FAILURE);
    if (readExact(ptrData) != readOutcome::complete) return markAllNullAndZero(EXIT_FAILURE);
  }
  lastNullBitset_.assign(descriptor_.size(), false);
  nullBitset = lastNullBitset_;
  cnt_++;
  return EXIT_SUCCESS;
}

size_t binaryDeviceRO::count() { return cnt_; }

const std::vector<bool> &binaryDeviceRO::lastNullBitset() const { return lastNullBitset_; }

}  // namespace rdb
