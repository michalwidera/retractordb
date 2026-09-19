#include "lockFile.hpp"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <system_error>
#include <thread>

namespace lockfile {
namespace {

/// Jak dlugo czekac, az blokade zwolni wlasciciel przejsciowy. Sprzatacz i proces kasujacy plik
/// trzymaja ja przez kilka wywolan jadra; zywa instancja trzyma ja do konca, wiec po tym czasie
/// odpowiedz "zajete" jest prawdziwa.
constexpr std::chrono::milliseconds kTransientHolderGrace{50};
constexpr std::chrono::milliseconds kRetryInterval{1};

/// Ile razy zaczynac od nowa po trafieniu na skasowany i-wezel. Kazde trafienie znaczy, ze
/// ktos zdazyl skasowac plik miedzy naszym open() a flock(); seria takich trafien nie jest
/// wyscigiem, tylko systemem plikow, na ktorym ten protokol nie dziala.
constexpr int kMaxRelinks = 100;

constexpr mode_t kMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

int openOrCreate(const std::string &path, int access) {
  for (;;) {
    const int fd = ::open(path.c_str(), access | O_CLOEXEC);
    if (fd != -1 || errno != ENOENT) return fd;
    // O_EXCL: istniejacego pliku nie otwieramy z O_CREAT. Przy fs.protected_regular jadro
    // odrzuca O_CREAT na cudzym pliku w katalogu z bitem sticky, nawet gdy prawa by pozwalaly.
    const int created = ::open(path.c_str(), access | O_CREAT | O_EXCL | O_CLOEXEC, kMode);
    if (created != -1) {
      // umask moglby odebrac innym prawo odczytu, a O_RDONLY wystarcza im do flock.
      ::fchmod(created, kMode);
      return created;
    }
    if (errno != EEXIST) return -1;
    // Plik powstal miedzy naszymi dwoma open(); otwieramy ten.
  }
}

}  // namespace

bool stillLinked(int fd, const std::string &path) {
  struct stat held{};
  struct stat named{};
  return ::fstat(fd, &held) == 0 && ::stat(path.c_str(), &named) == 0 && held.st_dev == named.st_dev &&
         held.st_ino == named.st_ino;
}

Result acquire(const std::string &path, bool exclusive, bool writable, int &fd) {
  const auto deadline = std::chrono::steady_clock::now() + kTransientHolderGrace;
  int relinks         = 0;
  for (;;) {
    const int candidate = openOrCreate(path, writable ? O_RDWR : O_RDONLY);
    if (candidate == -1) return Result::Error;

    if (::flock(candidate, exclusive ? (LOCK_EX | LOCK_NB) : LOCK_SH) == -1) {
      const int error   = errno;
      const bool linked = stillLinked(candidate, path);
      ::close(candidate);
      if (error == EINTR) continue;
      if (error != EWOULDBLOCK) {
        errno = error;
        return Result::Error;
      }
      // Wlasciciel wlasnie skasowal plik - nastepny open() trafi na nowy albo go utworzy.
      if (!linked && ++relinks < kMaxRelinks) continue;
      if (std::chrono::steady_clock::now() >= deadline) return Result::Busy;
      std::this_thread::sleep_for(kRetryInterval);
      continue;
    }

    if (!stillLinked(candidate, path)) {
      // Zajelismy i-wezel, ktory ktos skasowal miedzy open() a flock(). Blokada na nim nie
      // chroni juz niczego: kolejny uczestnik utworzy pod ta sciezka nowy plik.
      ::close(candidate);
      if (++relinks < kMaxRelinks) continue;
      errno = ESTALE;
      return Result::Error;
    }

    fd = candidate;
    return Result::Acquired;
  }
}

void removeAndRelease(const std::string &path, int fd) {
  if (stillLinked(fd, path)) ::unlink(path.c_str());
  ::close(fd);
}

bool isHeld(const std::string &path) {
  const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd == -1) return false;
  const bool held = ::flock(fd, LOCK_SH | LOCK_NB) == -1 && errno == EWOULDBLOCK;
  ::close(fd);
  return held;
}

int claimAbandoned(const std::string &path) {
  const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd == -1) return -1;
  if (::flock(fd, LOCK_EX | LOCK_NB) == -1 || !stillLinked(fd, path)) {
    ::close(fd);
    return -1;
  }
  return fd;
}

std::size_t sweep(const std::string &dir, const std::function<bool(std::string_view)> &accept,
                  const std::function<void(std::string_view)> &removeGuarded) {
  std::size_t removed = 0;
  std::error_code ec;
  for (std::filesystem::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
    const std::string name = it->path().filename().string();
    if (!accept(name)) continue;
    const std::string path = it->path().string();
    const int fd           = claimAbandoned(path);
    if (fd == -1) continue;
    removeGuarded(name);
    removeAndRelease(path, fd);
    ++removed;
  }
  return removed;
}

}  // namespace lockfile
