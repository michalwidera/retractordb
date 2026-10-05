#include "rdb/faccbindev.hpp"

#include <fcntl.h>
#include <poll.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <unistd.h>  // ::read, ::open ...

#include <algorithm>
#include <cerrno>
#include <cstring>
#include "fatalError.hpp"
#include "rdb/accessorFactory.hpp"

namespace rdb {

namespace {
constexpr mode_t kDefaultFileMode = 0644;
}

binaryDeviceRO::binaryDeviceRO(const std::string_view fileName,  //
                               const rdb::Descriptor &descriptor,
                               bool loopToBeginningIfEOF,  //
                               const std::string_view storageType)
    : filename_(std::string(fileName)),
      storageType_(std::string(storageType)),
      recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),
      descriptor_(descriptor),
      loopToBeginningIfEOF_(loopToBeginningIfEOF),
      lastNullBitset_(descriptor.size(), false),
      isDevice_(storageType == "DEVICE") {
  // BINFILE PRZED otwarciem: open(O_RDONLY) na FIFO bez pisarza wisi w samym wywolaniu, wiec
  // fstat po nim nie zdazylby niczego odrzucic. DEVICE otwiera sie z O_NONBLOCK (#347), wiec FIFO
  // bez pisarza nie wiesza startu, a pierwszy read() zwraca po prostu EOF - "brak pisarza".
  if (storageType_ == "BINFILE") initializationError_ = sourceKindMismatch(storageType_, filename_);
  if (!initializationError_.empty()) return;
  if (isDevice_) pending_.resize(static_cast<size_t>(recordSize_));

  fd_ = ::open(filename_.c_str(), O_RDONLY | O_CLOEXEC | (isDevice_ ? O_NONBLOCK : 0), kDefaultFileMode);
  if (fd_ < 0) {
    SPDLOG_WARN("Unable to open binary device source: {}", filename_);
    return;
  }
  // Rodzaj tego, co faktycznie otwarto - sciezka mogla zmienic sie po stat().
  struct stat openedStat{};
  if (::fstat(fd_, &openedStat) == 0) initializationError_ = sourceKindMismatch(storageType_, filename_, openedStat.st_mode);
  if (!initializationError_.empty()) {
    ::close(fd_);
    fd_ = -1;
  }
}

binaryDeviceRO::~binaryDeviceRO() {
#ifndef NDEBUG
  // Niezmiennik fazy DEVICE (#347): migawka zrodel powstaje pod blokada epoki, ale czekanie biegnie
  // juz bez niej, wiec w tym czasie nikt nie moze zniszczyc akcesora. Dzis trzyma to konstrukcja:
  // import ad hoc tylko dopisuje wezly, a rozbiorka epoki idzie w watku wykonawczym, po fazie.
  // Gdyby import zaczal usuwac wezly, migawka musi przejsc na wspolwlasnosc.
  if (awaited_.load(std::memory_order_relaxed))
    FatalError("binaryDeviceRO: DEVICE source '{}' destroyed during the DEVICE phase", filename_);
#endif
  if (fd_ >= 0) ::close(fd_);
}

auto binaryDeviceRO::name() -> std::string & { return filename_; }

// Krótki odczyt NIE oznacza końca danych: ::read na FIFO, potoku czy urządzeniu wolno zwrócić mniej
// bajtów, niż zażądano, a EINTR przerywa wywołanie bez utraty czegokolwiek. Rekord składamy więc
// z kolejnych porcji i ponawiamy przerwane wywołanie.
binaryDeviceRO::readOutcome binaryDeviceRO::readExact(uint8_t *ptrData) const {
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

void binaryDeviceRO::noteLink(const linkState state, const int error) {
  if (state == link_) return;
  switch (state) {
    case linkState::data:
      if (link_ != linkState::unknown) SPDLOG_WARN("DEVICE {}: data resumed", filename_);
      break;
    case linkState::noWriter:
      SPDLOG_WARN("DEVICE {}: no writer (end of file) - due records are all-null until data comes back", filename_);
      break;
    case linkState::failed:
      SPDLOG_WARN("DEVICE {}: read failed ({}) - due records are all-null", filename_, std::strerror(error));
      break;
    case linkState::unknown:
      break;
  }
  link_ = state;
}

binaryDeviceRO::fillResult binaryDeviceRO::fill() {
  attempted_ = true;
  if (complete_) return fillResult::complete;
  if (fd_ < 0) return fillResult::error;
  // Po wyczerpaniu (ONESHOT) zrodlo milczy jak BINFILE ONESHOT za koncem pliku: kolejny pisarz
  // nie otwiera nowego przebiegu.
  if (exhausted_) return fillResult::endOfFile;
  while (filled_ < recordSize_) {
    const ssize_t got = ::read(fd_, pending_.data() + filled_, static_cast<size_t>(recordSize_ - filled_));
    if (got > 0) {
      filled_ += got;
      sawData_ = true;
      continue;
    }
    if (got == 0) {
      if (filled_ > 0) {
        SPDLOG_WARN("DEVICE {}: writer left after {} of {} bytes of a record - incomplete record dropped", filename_, filled_,
                    recordSize_);
        filled_ = 0;
      }
      if (!loopToBeginningIfEOF_ && sawData_) exhausted_ = true;
      noteLink(linkState::noWriter);
      return fillResult::endOfFile;
    }
    if (errno == EINTR) continue;
    if (errno == EAGAIN || errno == EWOULDBLOCK) return fillResult::wouldBlock;
    noteLink(linkState::failed, errno);
    return fillResult::error;
  }
  complete_ = true;
  noteLink(linkState::data);
  return fillResult::complete;
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

  if (fd_ < 0) return markAllNullAndZero(EBADF);
  if (recordSize_ == 0) return markAllNullAndZero(EINVAL);

  if (position != 0) {
    return markAllNullAndZero(EINVAL);
  }

  if (isDevice_) {
    // Pod blokada modelu tylko przeniesienie gotowego rekordu. Proba nieblokujaca pada tu wylacznie
    // wtedy, gdy faza DEVICE nie objela zrodla w tym slocie (zrodlo dolaczone importem w jej trakcie).
    if (!attempted_) static_cast<void>(fill());
    attempted_ = false;
    // Brak rekordu w terminie nie jest bledem odczytu - SourceBuffer nie loguje go w kazdym takcie,
    // a zmiany stanu lacza zglasza noteLink().
    if (!complete_) return markAllNullAndZero(EXIT_SUCCESS);
    std::memcpy(ptrData, pending_.data(), static_cast<size_t>(recordSize_));
    complete_ = false;
    filled_   = 0;
    lastNullBitset_.assign(descriptor_.size(), false);
    nullBitset = lastNullBitset_;
    cnt_++;
    return EXIT_SUCCESS;
  }

  auto outcome = readExact(ptrData);
  if (outcome == readOutcome::error) return markAllNullAndZero(EIO);
  if (outcome == readOutcome::endOfFile) {
    if (!loopToBeginningIfEOF_) {
      // Koniec strumienia bez zawijania to koniec danych - przebieg z --until-eof ma sie tu zatrzymac.
      exhausted_ = true;
      return markAllNullAndZero(EXIT_SUCCESS);
    }
    if (::lseek(fd_, 0, SEEK_SET) < 0) return markAllNullAndZero(errno);
    if (readExact(ptrData) != readOutcome::complete) return markAllNullAndZero(EIO);
  }
  lastNullBitset_.assign(descriptor_.size(), false);
  nullBitset = lastNullBitset_;
  cnt_++;
  return EXIT_SUCCESS;
}

size_t binaryDeviceRO::count() { return cnt_; }

const std::vector<bool> &binaryDeviceRO::lastNullBitset() const { return lastNullBitset_; }

void awaitRecords(const std::span<const deviceWait> waits) {
  using clock = std::chrono::steady_clock;

  // Pierwsza runda: proba nieblokujaca dla kazdego zrodla. Czekaja tylko te, ktorym brakuje danych
  // i ktorych termin jeszcze nie minal - TIMEOUT 0 konczy sie wiec na tej jednej probie.
  std::vector<deviceWait> pending;
  pending.reserve(waits.size());
  for (const auto &wait : waits)
    if (wait.source->fill() == binaryDeviceRO::fillResult::wouldBlock && clock::now() < wait.deadline) pending.push_back(wait);

  std::vector<pollfd> fds;
  fds.reserve(pending.size());
  while (!pending.empty()) {
    // Termin kazdego zrodla jest staly od poczatku slotu. Po EINTR i po falszywym przebudzeniu
    // (EAGAIN) petla wraca tutaj i liczy pozostaly czas od nowa, wiec ponowienie go nie odnawia.
    const auto now = clock::now();
    std::erase_if(pending, [now](const deviceWait &wait) { return wait.deadline <= now; });
    if (pending.empty()) break;

    const auto nearest = std::ranges::min(pending, {}, &deviceWait::deadline).deadline;
    // poll() bierze milisekundy; zaokraglenie w gore, zeby nie budzic sie przed terminem i nie krecic
    // petla. ppoll() z nanosekundami nie istnieje na Darwinie.
    const auto waitMs = std::chrono::ceil<std::chrono::milliseconds>(nearest - now).count();

    fds.clear();
    for (const auto &wait : pending)
      fds.push_back(pollfd{.fd = wait.source->pollDescriptor(), .events = POLLIN, .revents = 0});
    const int ready = ::poll(fds.data(), static_cast<nfds_t>(fds.size()), static_cast<int>(waitMs));
    if (ready < 0) {
      if (errno == EINTR) continue;
      SPDLOG_WARN("awaitRecords: poll failed ({}) - DEVICE sources get no further wait in this slot", std::strerror(errno));
      break;
    }

    // POLLHUP, POLLERR i POLLNVAL tylko budza: o EOF i bledzie rozstrzyga read() w fill().
    size_t kept = 0;
    for (size_t i = 0; i < pending.size(); ++i) {
      const bool finished = fds[i].revents != 0 && pending[i].source->fill() != binaryDeviceRO::fillResult::wouldBlock;
      if (!finished) pending[kept++] = pending[i];
    }
    pending.resize(kept);
  }

  for (const auto &wait : waits)
    wait.source->markAwaited(false);
}

}  // namespace rdb
