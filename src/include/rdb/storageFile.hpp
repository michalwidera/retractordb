#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <span>
#include <string>

#include <spdlog/spdlog.h>

namespace rdb {

/// @brief Otwarcie pliku magazynu bez podazania za dowiazaniem symbolicznym (#374).
///
/// Pliki magazynu - dane, cien danych, `.meta`, `.meta.shadow`, `.desc` - otwiera sie z O_NOFOLLOW:
/// dowiazanie lezace pod ich nazwa jest odmowa (ELOOP), wiec kto ma prawo zapisu do katalogu
/// magazynu, nie skieruje przez nie zapisu ani obciecia w inne miejsce dostepne dla konta uslugi.
/// O_NOFOLLOW dotyczy tylko ostatniego komponentu: katalog magazynu i jego przodkowie moga byc
/// dowiazaniami jak dotad. Wyjatkiem jest plik danych spod REF podanego przez wolajacego (plan,
/// schemat w xtrdb) - to decyzja operatora (#278), wiec akcesor dostaje wtedy followFinalLink.
///
/// Zostaje wyscig na katalogu posrednim (podmiana katalogu po sprawdzeniu w relocateFromRef());
/// wymaga zapisu do katalogu nadrzednego magazynu, wiec wykracza poza ten model zagrozenia.
///
/// Domyslne uprawnienia to 0666 & ~umask - tak tworzyly pliki zastapione tu std::fstream.
inline constexpr mode_t kStreamFileMode = 0666;

[[nodiscard]] inline int openStorageFile(const std::string &path, const int flags, const mode_t mode = kStreamFileMode,
                                         const bool followFinalLink = false) {
  const int fd = ::open(path.c_str(), flags | O_CLOEXEC | (followFinalLink ? 0 : O_NOFOLLOW), mode);
  if (fd < 0 && errno == ELOOP) {
    const int openErrno = errno;  // log moze ruszyc errno, a wolajacy go czyta
    SPDLOG_ERROR("storage: '{}' is a symbolic link - storage files are not opened through links", path);
    errno = openErrno;
  }
  return fd;
}

/// @brief Deskryptor pliku magazynu zamykany w destruktorze; zastepuje std::fstream, ktory nie
///        przyjmuje deskryptora (a __gnu_cxx::stdio_filebuf nie istnieje w libc++ na macOS).
class StorageFd {
  int fd_;

 public:
  StorageFd(const std::string &path, const int flags, const mode_t mode = kStreamFileMode, const bool followFinalLink = false)
      : fd_(openStorageFile(path, flags, mode, followFinalLink)) {}
  ~StorageFd() {
    if (fd_ >= 0) ::close(fd_);
  }
  StorageFd(const StorageFd &)            = delete;
  StorageFd &operator=(const StorageFd &) = delete;

  [[nodiscard]] bool isOpen() const { return fd_ >= 0; }

  /// @brief Rozmiar pliku; -1 przy bledzie fstat().
  [[nodiscard]] off_t size() const {
    struct stat st{};
    return ::fstat(fd_, &st) == 0 ? st.st_size : -1;
  }

  /// @brief Zapis calosci od biezacej pozycji (albo na koniec przy O_APPEND).
  [[nodiscard]] bool write(std::span<const std::byte> bytes) const {
    while (!bytes.empty()) {
      const ssize_t done = ::write(fd_, bytes.data(), bytes.size());
      if (done < 0 && errno == EINTR) continue;
      if (done <= 0) return false;
      bytes = bytes.subspan(static_cast<size_t>(done));
    }
    return true;
  }

  /// @brief Zapis calosci od pozycji @p offset.
  [[nodiscard]] bool writeAt(std::span<const std::byte> bytes, off_t offset) const {
    while (!bytes.empty()) {
      const ssize_t done = ::pwrite(fd_, bytes.data(), bytes.size(), offset);
      if (done < 0 && errno == EINTR) continue;
      if (done <= 0) return false;
      bytes = bytes.subspan(static_cast<size_t>(done));
      offset += done;
    }
    return true;
  }

  /// @brief Odczyt od pozycji @p offset do zapelnienia @p bytes albo do konca pliku;
  ///        zwraca liczbe przeczytanych bajtow albo -1 przy bledzie.
  [[nodiscard]] ssize_t readAt(std::span<std::byte> bytes, off_t offset) const {
    size_t total = 0;
    while (total < bytes.size()) {
      const ssize_t done = ::pread(fd_, bytes.data() + total, bytes.size() - total, offset + static_cast<off_t>(total));
      if (done < 0 && errno == EINTR) continue;
      if (done < 0) return -1;
      if (done == 0) break;
      total += static_cast<size_t>(done);
    }
    return static_cast<ssize_t>(total);
  }
};

}  // namespace rdb
