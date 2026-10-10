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
///
/// Ten czas NIE mierzy pracy wlasciciela przejsciowego, tylko czas, w jakim planista zdazy go
/// obudzic. Kilka wywolan jadra to mikrosekundy, ale watek, ktory na nie czeka, wraca na procesor
/// dopiero wtedy, gdy maszyna ma go gdzie postawic. Pomiar na tym samym protokole: przy 64-krotnym
/// przeciazeniu procesora przekazanie trwalo do 168 ms, wiec z budzetem 50 ms padalo 118 prob na
/// 120, a z 500 ms - zadna. Na rdzeniach asymetrycznych (Apple silicon: P i E) jest o to jeszcze
/// latwiej, bo obudzony watek moze trafic na rdzen oszczedny. Falszywe "juz dziala" zatrzymuje
/// start instancji, a nadmiar czekania kosztuje tylko tyle opoznienia na sciezce NAPRAWDE zajetej.
constexpr std::chrono::milliseconds kTransientHolderGrace{500};
constexpr std::chrono::milliseconds kRetryInterval{1};

/// Ile razy zaczynac od nowa po trafieniu na skasowany i-wezel. Kazde trafienie znaczy, ze
/// ktos zdazyl skasowac plik miedzy naszym open() a flock(); seria takich trafien nie jest
/// wyscigiem, tylko systemem plikow, na ktorym ten protokol nie dziala.
constexpr int kMaxRelinks      = 100;
constexpr int kMaxOpenAttempts = 100;

constexpr mode_t kMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

/// 0, gdy plik nadaje sie na blokade; inaczej powod odmowy jako errno.
int refusal(const struct stat &info, bool writable) {
  if (!S_ISREG(info.st_mode)) return EINVAL;
  // Czytelnik obecnosci/tozsamosci moze korzystac z pliku innego konta.
  if (writable && info.st_uid != geteuid()) return EPERM;
  if (writable && info.st_nlink != 1) return EMLINK;
  return 0;
}

/// refused odroznia odmowe pliku od bledu systemu - oba koncza sie -1 z errno.
int checkedOpen(const std::string &path, int access, bool &refused, bool create = false) {
  refused = false;
  // O_NONBLOCK pozwala odrzucic FIFO przez fstat zamiast czekac na drugiego uczestnika.
  const int flags = access | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK | (create ? O_CREAT | O_EXCL : 0);
  const int fd    = ::open(path.c_str(), flags, kMode);
  if (fd == -1) {
    // Przy O_NOFOLLOW ELOOP oznacza dowiazanie na koncu sciezki, a EISDIR - katalog.
    refused = errno == ELOOP || errno == EISDIR;
    if (errno == EISDIR) errno = EINVAL;
    return -1;
  }
  // I-wezel jest ustalony przy open(): typu i wlasciciela nie da sie mu pozniej podmienic,
  // a dowiazanie dodane pozniej nie przekierowuje zapisu - wystarcza ta jedna kontrola.
  struct stat info{};
  const bool described = ::fstat(fd, &info) == 0;
  const int error      = described ? refusal(info, access == O_RDWR) : errno;
  if (error == 0) return fd;
  refused = described;
  ::close(fd);
  errno = error;
  return -1;
}

int openOrCreate(const std::string &path, int access, bool &refused) {
  for (int attempt = 0; attempt < kMaxOpenAttempts; ++attempt) {
    const int fd = checkedOpen(path, access, refused);
    if (fd != -1 || errno != ENOENT) return fd;
    // O_EXCL: istniejacego pliku nie otwieramy z O_CREAT. Przy fs.protected_regular jadro
    // odrzuca O_CREAT na cudzym pliku w katalogu z bitem sticky, nawet gdy prawa by pozwalaly.
    const int created = checkedOpen(path, access, refused, true);
    if (created != -1) {
      // umask moglby odebrac innym prawo odczytu, a O_RDONLY wystarcza im do flock.
      ::fchmod(created, kMode);
      return created;
    }
    if (errno != EEXIST) return -1;
    // Plik powstal miedzy naszymi dwoma open(); otwieramy ten.
  }
  refused = true;
  errno   = EAGAIN;
  return -1;
}

}  // namespace

bool stillLinked(int fd, const std::string &path) {
  struct stat held{};
  struct stat named{};
  return ::fstat(fd, &held) == 0 && ::lstat(path.c_str(), &named) == 0 && S_ISREG(named.st_mode) &&
         held.st_dev == named.st_dev && held.st_ino == named.st_ino;
}

Result acquire(const std::string &path, bool exclusive, bool writable, int &fd) {
  const auto deadline = std::chrono::steady_clock::now() + kTransientHolderGrace;
  int relinks         = 0;
  for (;;) {
    bool refused        = false;
    const int candidate = openOrCreate(path, writable ? O_RDWR : O_RDONLY, refused);
    if (candidate == -1) return refused ? Result::Refused : Result::Error;

    // Takze czytelnik ma termin: zatrzymany posiadacz LOCK_EX nie moze zawiesic startu Bus.
    if (::flock(candidate, (exclusive ? LOCK_EX : LOCK_SH) | LOCK_NB) == -1) {
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
  bool refused = false;
  const int fd = checkedOpen(path, O_RDONLY, refused);
  if (fd == -1) return false;
  const bool held = ::flock(fd, LOCK_SH | LOCK_NB) == -1 && errno == EWOULDBLOCK;
  ::close(fd);
  return held;
}

int claimAbandoned(const std::string &path) {
  bool refused = false;
  const int fd = checkedOpen(path, O_RDONLY, refused);
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
    // Sprzatacz nie przejmuje pozostalosci innych kont, takze uruchomiony jako root.
    struct stat owner{};
    if (fstat(fd, &owner) != 0 || owner.st_uid != geteuid()) {
      ::close(fd);
      continue;
    }
    removeGuarded(name);
    removeAndRelease(path, fd);
    ++removed;
  }
  return removed;
}

}  // namespace lockfile
