#include "persistentCounter.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

#include <spdlog/spdlog.h>

#include "fatalError.hpp"

// Numer N+1 jest rezerwowany przy starcie, a nie zapisywany w destruktorze (#281). Dawny
// destruktor zapisywal go na koniec sesji i przy porazce mogl ja jedynie przemilczec: nastepna
// sesja dostawala znow N i nadpisywala .old<N>. Tu porazka zatrzymuje proces, zanim powstanie
// pierwsze archiwum. Przy okazji znika okno przy wyjsciu: pCounterPtr jest globalny, wiec jego
// destruktor biegl dopiero po cleanup() z atexit - po zwolnieniu slotu magistrali i blokady,
// kiedy druga instancja mogla juz wczytac stare N. Cena: sesja zabita w trakcie zuzywa swoj
// numer i w numeracji .old<N> zostaje dziura - nic nie jest nadpisywane.
PersistentCounter::PersistentCounter(std::string initFilename)
    :                                                      //
      persistentCounterFilename_(std::move(initFilename))  //
{
  load();
  if (!save(count_ + 1))
    rdb::EnvironmentError(
        "Cannot reserve rotation number {} in '{}'; refusing to start - otherwise the next session would reuse number {} "
        "and overwrite the archives of this one.",
        count_ + 1, persistentCounterFilename_, count_);
}

int PersistentCounter::getCount() const { return count_; }

// Brak pliku to pierwsze uzycie planu z :ROTATION - rotacja 0. Kazdy INNY stan pliku, ktorego
// nie da sie odczytac w calosci jako nieujemnej liczby, nie daje numeru. Dawniej `infile >>
// count_` na pustym pliku wpisywal 0 (od C++11 nieudany odczyt zeruje zmienna), wiec plik
// uciety przez awarie udawal rotacje 0: plan zaczynal archiwizacje od nowa i nadpisywal
// .old0, .old1, ... poprzednich sesji bez sladu w logu. Wartosc ujemna odpada z tego samego
// powodu - percounter < 0 znaczy w storage "bez rotacji", wiec plik z "-1" wylaczylby
// archiwizacje po cichu.
//
// Brak pliku musi byc POTWIERDZONY: blad samego sprawdzenia (np. EACCES na katalogu) nie jest
// brakiem. Odczyt przez open/read, a nie ifstream, bo tylko tak zostaje errno: ifstream otwiera
// katalog bez bledu i czyta z niego zero bajtow, czyli blad nosnika udawalby pusty plik (K2).
PersistentCounter::Reading PersistentCounter::read(const std::string &filename) {
  constexpr std::size_t kErrorPreviewBytes = 32;
  std::error_code ec;
  const bool exists = std::filesystem::exists(filename, ec);
  if (ec) return {.state = Reading::State::Unreadable, .detail = "cannot check whether it exists: " + ec.message()};
  if (!exists) return {.state = Reading::State::Missing};

  const int fd = ::open(filename.c_str(), O_RDONLY | O_CLOEXEC);
  if (fd == -1) return {.state = Reading::State::Unreadable, .detail = std::string("open: ") + std::strerror(errno)};
  std::string text;
  char buffer[64];
  ssize_t got = 0;
  while ((got = ::read(fd, buffer, sizeof(buffer))) != 0) {
    if (got > 0) {
      text.append(buffer, static_cast<std::size_t>(got));
    } else if (errno != EINTR) {
      const int readErrno = errno;
      ::close(fd);
      return {.state = Reading::State::Unreadable, .detail = std::string("read: ") + std::strerror(readErrno)};
    }
  }
  ::close(fd);

  const char *first = text.data();
  const char *last  = text.data() + text.size();
  while (last != first && std::isspace(static_cast<unsigned char>(last[-1])))
    --last;
  int value{};
  const auto [ptr, err] = std::from_chars(first, last, value);
  if (err != std::errc() || ptr != last || value < 0)
    return {.state  = Reading::State::Invalid,
            .detail = std::to_string(text.size()) + " bytes: '" + text.substr(0, kErrorPreviewBytes) + "'"};
  return {.state = Reading::State::Value, .value = value};
}

// Odmowa zamiast wartosci zastepczej (decyzja operacyjna, #281): wlasciwego numeru nie da sie
// tu odtworzyc, a kazdy zgadniety moze nadpisac archiwum. Operator naprawia plik albo
// swiadomie go usuwa. Klasa K4 dla obu przyczyn (#472): wadliwa tresc odrzucilo juz jako blad
// planu checkRotationCounter() przed startem, wiec tutaj plik zmienil sie po tamtym sprawdzeniu.
// Plan zostaje, a nastepny start rozpozna tresc ta sama droga co wtedy.
void PersistentCounter::load() {
  const Reading reading = read(persistentCounterFilename_);
  switch (reading.state) {
    case Reading::State::Missing:
      count_ = 0;
      return;
    case Reading::State::Value:
      count_ = reading.value;
      return;
    case Reading::State::Unreadable:
      rdb::EnvironmentError("Rotation counter file '{}' cannot be read ({}); refusing to start.", persistentCounterFilename_,
                            reading.detail);
    case Reading::State::Invalid:
      rdb::EnvironmentError(
          "Rotation counter file '{}' is unreadable ({}); refusing to start. A guessed rotation number could overwrite "
          "archives of previous sessions. Repair the file (a non-negative integer), or delete it to restart rotation from 0 "
          "- existing .old<N> archives will then be overwritten.",
          persistentCounterFilename_, reading.detail);
  }
}

// Zapis przez plik tymczasowy, wzorem servicecontrol::writeQueryFile. Samo
// `std::ofstream(cel)` nie wystarcza: konstrukcja strumienia OBCINA plik do zera, a wartosc
// trafia do niego dopiero przy close(). Proces zabity miedzy jednym a drugim zostawial plik
// 0-bajtowy, ktory load() czytal jako rotacje 0. Tu plik docelowy do chwili rename() trzyma
// stara wartosc, a rename() podmienia go atomowo: kazdy odczyt widzi stara albo nowa liczbe.
//
// Wobec zabicia PROCESU sam rename() wystarcza - dane po write() sa w pamieci podrecznej
// jadra i smierc procesu ich nie zabiera. Wobec utraty zasilania albo paniki jadra nie, stad
// oba fsync:
// - pliku tymczasowego PRZED rename(): bez niego metadane rename moga trafic na dysk przed
//   danymi i po restarcie cel ma 0 bajtow - ten sam objaw inna droga (ext4 lata to
//   heurystyka auto_da_alloc, ale ani POSIX, ani XFS tego nie gwarantuja);
// - katalogu PO rename(): bez niego po awarii moze wrocic stara wartosc, czyli numer, ktory
//   dostala juz sesja sprzed awarii.
// Archiwa utrwala fsync w faccposix, wiec licznik bez fsync odbieralby im te trwalosc. Koszt
// jest pomijalny: jeden zapis na start albo przeladowanie planu. Na macOS fsync nie oproznia
// pamieci podrecznej samego dysku (robi to F_FULLFSYNC) - tak samo jak w faccposix
// i lockManager, bez osobnej galezi platformowej.
//
// Porazka fsync katalogu to blad w logu, a nie odmowa startu: rename juz sie odbyl, wiec numer
// jest zarezerwowany, a nie kazdy system plikow obsluguje fsync katalogu - odmowa na takim
// zablokowalaby start na stale.
bool PersistentCounter::save(int value) const {
  const std::filesystem::path target(persistentCounterFilename_);
  const std::filesystem::path tmp = target.parent_path() / (target.filename().string() + ".tmp." + std::to_string(::getpid()));
  const std::string text          = std::to_string(value);

  const int fd = ::open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  bool ok      = fd != -1;
  ok           = ok && ::write(fd, text.data(), text.size()) == static_cast<ssize_t>(text.size());
  ok           = ok && ::fsync(fd) == 0;
  if (fd != -1 && ::close(fd) != 0) ok = false;

  std::error_code ec;
  if (ok) std::filesystem::rename(tmp, target, ec);
  if (!ok || ec) {
    SPDLOG_ERROR("Rotation counter '{}' NOT saved (value {}): {}", target.string(), value,
                 ec ? ec.message() : std::strerror(errno));
    std::filesystem::remove(tmp, ec);
    return false;
  }

  const std::filesystem::path dir = target.has_parent_path() ? target.parent_path() : ".";
  const int dirFd                 = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (dirFd == -1 || ::fsync(dirFd) != 0)
    SPDLOG_ERROR("Rotation counter '{}' saved, but directory fsync failed: {}", target.string(), std::strerror(errno));
  if (dirFd != -1) ::close(dirFd);
  return true;
}
