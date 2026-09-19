// Wycena miejsca w pamieci dzielonej. Najwazniejszy tu jest pierwszy zestaw przypadkow:
// rozmiar kolejki liczymy WZOREM, bo boost trzyma message_queue::get_mem_size jako skladowa
// prywatna, wiec zmiana ukladu segmentu po stronie biblioteki nie jest u nas bledem
// kompilacji. Test tworzy kolejke naprawde i porownuje jej dlugosc z wynikiem wzoru --
// rozjazd wychodzi tutaj, a nie dopiero jako odmowa subskrypcji przy wolnej pamieci.

#include <sys/stat.h>

#include <filesystem>
#include <string>
#include <system_error>

#include <gtest/gtest.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/rational.hpp>

#include "constants.hpp"
#include "osPlatform.hpp"
#include "platformConfig.h"
#include "retractor/lib/bus.hpp"
#include "retractor/lib/shmBudget.hpp"

namespace {

namespace IPC = boost::interprocess;

using ratio = boost::rational<int>;

constexpr int kBufferSeconds = 10;
constexpr int kMinElements   = 100;

// Rozmiar segmentu kolejki widziany przez system plikow. Nazwa wlasna testu, zeby nie trafic
// w kolejke zywego klienta na tej maszynie.
std::uint64_t realQueueBytes(std::uint64_t maxMessages, std::uint64_t maxMessageSize) {
  const std::string name = "rdb_ut_shmbudget_probe";
  IPC::message_queue::remove(name.c_str());
  std::uint64_t retVal{0};
  {
    IPC::message_queue mq(IPC::create_only, name.c_str(), maxMessages, maxMessageSize);
    // Obiekt trzeba znalezc na dysku, a jego sciezka nie jest ta sama wszedzie:
    // z obiektami POSIX-owej pamieci dzielonej Boost tworzy wpis w /dev/shm,
    // a bez nich - zwykly plik w swoim katalogu roboczym. Szukamy wiec pod oboma
    // korzeniami, po nazwie, ktora sami nadalismy.
    struct stat st{};
    if (stat(("/dev/shm/" + name).c_str(), &st) == 0) {
      retVal = static_cast<std::uint64_t>(st.st_size);
    } else {
      for (const char *root : {"/tmp/boost_interprocess", "/tmp"}) {
        std::error_code ec;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(
                 root, std::filesystem::directory_options::skip_permission_denied, ec)) {
          if (ec) break;
          if (entry.is_regular_file(ec) && entry.path().filename() == name) {
            retVal = static_cast<std::uint64_t>(std::filesystem::file_size(entry.path(), ec));
            break;
          }
        }
        if (retVal != 0) break;
      }
    }
  }
  IPC::message_queue::remove(name.c_str());
  return retVal;
}

TEST(ShmBudget, QueueFormulaMatchesRealSegment) {
  for (const auto [messages, size] :
       {std::pair<std::uint64_t, std::uint64_t>{100, 1024}, std::pair<std::uint64_t, std::uint64_t>{1000, 1000},
        std::pair<std::uint64_t, std::uint64_t>{250, 1024}}) {
    const std::uint64_t real = realQueueBytes(messages, size);
    if (real == 0)
      GTEST_SKIP() << "segmentu kolejki nie da sie znalezc w systemie plikow na tej platformie; "
                      "pomiar odniesienia nie powstal, a wzoru nie ma z czym porownac";
    EXPECT_EQ(shmbudget::messageQueueBytes(messages, size), real)
        << "wzor rozjechal sie z boostem dla " << messages << " wiadomosci po " << size << " B";
  }
}

TEST(ShmBudget, ResponseQueueUsesResponseMessageSize) {
  EXPECT_EQ(shmbudget::responseQueueBytes(100), shmbudget::messageQueueBytes(100, ipc::kResponseQueueMaxMessageSize));
}

TEST(ShmBudget, ElementsFollowTickAndFloor) {
  // 10 s zapasu przy takcie 1 ms to dziesiec tysiecy elementow -- to jest ta liczba, ktora
  // zamienia sie na 10 MiB kolejki i wyczerpuje /dev/shm kontenera przy siodmym kliencie.
  EXPECT_EQ(shmbudget::responseQueueElements(ratio(1, 1000), kBufferSeconds, kMinElements), 10000);
  EXPECT_EQ(shmbudget::responseQueueElements(ratio(1, 8), kBufferSeconds, kMinElements), 100);
  // Podloga: wolny strumien dostaje minimum, nie osiem elementow.
  EXPECT_EQ(shmbudget::responseQueueElements(ratio(10), kBufferSeconds, kMinElements), kMinElements);
  // Takt zerowy nie istnieje w skompilowanym planie, ale wycena dostaje takze plany surowe;
  // dzielenie przez zero zamienialo by raport w wyjatek.
  EXPECT_EQ(shmbudget::responseQueueElements(ratio(0), kBufferSeconds, kMinElements), kMinElements);
}

TEST(ShmBudget, ElementsRoundUpOnFractionalFrequency) {
  // Takt o niecalkowitej odwrotnosci. Obciecie czestotliwosci PRZED pomnozeniem przez czas
  // bufora dawalo 22 * 10 = 220 zamiast 225: kolejka o piec elementow za krotka na kazdym
  // takcie 22,5 Hz. Caly iloraz musi byc liczony wymiernie i zaokraglony w gore.
  EXPECT_EQ(shmbudget::responseQueueElements(ratio(2, 45), kBufferSeconds, kMinElements), 225);
  // To samo widoczne bez podlogi: 10 s / (2/3 s) = 15 probek.
  EXPECT_EQ(shmbudget::responseQueueElements(ratio(2, 3), kBufferSeconds, /*minElements=*/1), 15);
  // Zaokraglenie w gore, a nie do najblizszej: 10 s / (3 s) = 3,33 probki to cztery miejsca.
  EXPECT_EQ(shmbudget::responseQueueElements(ratio(3), kBufferSeconds, /*minElements=*/1), 4);
}

TEST(ShmBudget, FixedReservationIsSumOfItsParts) {
  EXPECT_EQ(shmbudget::fixedReservationBytes(),
            bus::segmentBytes() + shmbudget::messageQueueBytes(ipc::kQueryQueueMaxMessages, ipc::kQueryQueueMaxMessageSize) +
                ipc::kShmemSegmentSize);
}

TEST(ShmBudget, SpaceIsMeasurableWhereSharedMemoryWorks) {
  const shmbudget::Space fs = shmbudget::space();
  ASSERT_TRUE(fs.known) << "pomiar nie przeszedl dla systemu plikow obiektow IPC (" << osplat::sharedMemoryBackingPath() << ")";
  EXPECT_GT(fs.total, 0U);
  EXPECT_LE(fs.available, fs.total);
}

TEST(ShmBudget, HumanBytesPicksUnit) {
  EXPECT_EQ(shmbudget::humanBytes(512), "512 B");
  EXPECT_EQ(shmbudget::humanBytes(2048), "2.0 KiB");
  EXPECT_EQ(shmbudget::humanBytes(3U * 1024 * 1024), "3.0 MiB");
}

}  // namespace
