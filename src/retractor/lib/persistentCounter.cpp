#include "persistentCounter.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
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
    FatalError(
        "Cannot reserve rotation number {} in '{}'; refusing to start - otherwise the next session would reuse number {} "
        "and overwrite the archives of this one.",
        count_ + 1, persistentCounterFilename_, count_);
}

int PersistentCounter::getCount() const { return count_; }

// Brak pliku to pierwsze uzycie planu z :ROTATION - rotacja 0. Kazdy INNY stan pliku, ktorego
// nie da sie odczytac w calosci jako nieujemnej liczby, zatrzymuje silnik. Dawniej `infile >>
// count_` na pustym pliku wpisywal 0 (od C++11 nieudany odczyt zeruje zmienna), wiec plik
// uciety przez awarie udawal rotacje 0: plan zaczynal archiwizacje od nowa i nadpisywal
// .old0, .old1, ... poprzednich sesji bez sladu w logu. Wartosc ujemna odpada z tego samego
// powodu - percounter < 0 znaczy w storage "bez rotacji", wiec plik z "-1" wylaczylby
// archiwizacje po cichu.
// Odmowa zamiast wartosci zastepczej (decyzja operacyjna, #281): wlasciwego numeru nie da sie
// tu odtworzyc, a kazdy zgadniety moze nadpisac archiwum. Operator naprawia plik albo
// swiadomie go usuwa.
void PersistentCounter::load() {
  std::error_code ec;
  if (!std::filesystem::exists(persistentCounterFilename_, ec) && !ec) {
    count_ = 0;
    return;
  }

  std::ifstream infile(persistentCounterFilename_, std::ios::binary);
  std::ostringstream content;
  if (infile.is_open()) content << infile.rdbuf();
  const std::string text = content.str();

  const char *first = text.data();
  const char *last  = text.data() + text.size();
  while (last != first && std::isspace(static_cast<unsigned char>(last[-1])))
    --last;
  int value{};
  const auto [ptr, err] = std::from_chars(first, last, value);
  if (!infile.is_open() || err != std::errc() || ptr != last || value < 0)
    FatalError(
        "Rotation counter file '{}' is unreadable ({} bytes: '{}'); refusing to start. A guessed rotation number could "
        "overwrite archives of previous sessions. Repair the file (a non-negative integer), or delete it to restart "
        "rotation from 0 - existing .old<N> archives will then be overwritten.",
        persistentCounterFilename_, text.size(), text.substr(0, 32));
  count_ = value;
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
