// Wycena miejsca w pamieci dzielonej. Najwazniejszy tu jest pierwszy zestaw przypadkow:
// rozmiar kolejki liczymy WZOREM, bo boost trzyma message_queue::get_mem_size jako skladowa
// prywatna, wiec zmiana ukladu segmentu po stronie biblioteki nie jest u nas bledem
// kompilacji. Test tworzy kolejke naprawde i porownuje jej dlugosc z wynikiem wzoru --
// rozjazd wychodzi tutaj, a nie dopiero jako odmowa subskrypcji przy wolnej pamieci.

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>

#include <gtest/gtest.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/rational.hpp>

#include "constants.hpp"
#include "osPlatform.hpp"
#include "platformConfig.h"
#include "retractor/lib/bus.hpp"
#include "retractor/lib/qTree.hpp"
#include "retractor/lib/shmBudget.hpp"
#include "syscallWrap.hpp"

namespace {
bool failDescriptorMeasure = false, failPathMeasure = false;
std::optional<struct statvfs> measuredSpace;
bool probeWasUnlinked = false, probeWasEmpty = false;
}  // namespace
extern "C" int __real_fstatvfs(int, struct statvfs *);
extern "C" int __real_statvfs(const char *, struct statvfs *);
extern "C" int __wrap_fstatvfs(int fd, struct statvfs *result) {
  if (failDescriptorMeasure) {
    errno = EIO;
    return -1;
  }
  if (measuredSpace) {
    struct stat entry{};
    probeWasEmpty    = fstat(fd, &entry) == 0 && entry.st_size == 0;
    const auto name  = "/dev/shm/rdb_shm_probe_" + std::to_string(getpid());
    probeWasUnlinked = !std::filesystem::exists(name);
    *result          = *measuredSpace;
    return 0;
  }
  return __real_fstatvfs(fd, result);
}
extern "C" int __wrap_statvfs(const char *path, struct statvfs *result) {
  if (failPathMeasure) {
    errno = EIO;
    return -1;
  }
  if (measuredSpace) {
    *result = *measuredSpace;
    return 0;
  }
  return __real_statvfs(path, result);
}
RDB_WRAP_SYSCALL(int, fstatvfs, (int fd, struct statvfs *result), (fd, result));
RDB_WRAP_SYSCALL(int, statvfs, (const char *path, struct statvfs *result), (path, result));

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

// Kanarek (CLAUDE.md, Exception: canaries): na Linuksie Boost tworzy obiekty IPC przez shm_open.
// Gdyby makro zniknelo albo zmienilo nazwe, testy sondy nie moga zniknac razem z nim po cichu.
// Wyrazenie C++, nie #if: brak ktorejkolwiek stalej jest bledem kompilacji, a nie zerem.
static_assert(!RDB_OS_LINUX || RDB_HAS_BOOST_POSIX_SHM,
              "Linux: Boost.Interprocess musi uzywac shm_open; sprawdz makro po aktualizacji Boosta");

#if RDB_HAS_BOOST_POSIX_SHM
TEST(ShmBudget, SpaceRemovesItsProbe) {
  const std::string name = "rdb_shm_probe_" + std::to_string(getpid());
  EXPECT_TRUE(shmbudget::space().known);
  EXPECT_THROW((IPC::shared_memory_object(IPC::open_only, name.c_str(), IPC::read_only)), IPC::interprocess_exception);
}

TEST(ShmBudget, SpacePreservesExistingObjectOnProbeNameCollision) {
  const std::string name = "rdb_shm_probe_" + std::to_string(getpid());
  IPC::shared_memory_object existing(IPC::create_only, name.c_str(), IPC::read_write);
  existing.truncate(17);
  EXPECT_TRUE(shmbudget::space().known) << "kolizja powinna uzyc pomiaru z katalogu";
  EXPECT_NO_THROW({
    IPC::shared_memory_object preserved(IPC::open_only, name.c_str(), IPC::read_only);
    IPC::offset_t bytes = 0;
    EXPECT_TRUE(preserved.get_size(bytes));
    EXPECT_EQ(bytes, 17);
  });
  IPC::shared_memory_object::remove(name.c_str());
}
#endif

TEST(ShmBudget, HumanBytesPicksUnit) {
  EXPECT_EQ(shmbudget::humanBytes(0), "0 B");
  EXPECT_EQ(shmbudget::humanBytes(1023), "1023 B");
  EXPECT_EQ(shmbudget::humanBytes(512), "512 B");
  EXPECT_EQ(shmbudget::humanBytes(2048), "2.0 KiB");
  EXPECT_EQ(shmbudget::humanBytes(3U * 1024 * 1024), "3.0 MiB");
  EXPECT_EQ(shmbudget::humanBytes(1024), "1.0 KiB");
  EXPECT_EQ(shmbudget::humanBytes(UINT64_C(1073741824)), "1.0 GiB");
  EXPECT_EQ(shmbudget::humanBytes(UINT64_C(1073741823)), "1024.0 MiB");
  EXPECT_EQ(shmbudget::humanBytes(1536), "1.5 KiB");
}

TEST(ShmBudget, MeasurementUsesAvailableBlocksAndFragmentSize) {
  struct Restore {
    ~Restore() { measuredSpace.reset(); }
  } restore;
  measuredSpace.emplace();
  measuredSpace->f_blocks = 500;
  measuredSpace->f_bavail = 123;
  measuredSpace->f_bfree  = 321;
  measuredSpace->f_frsize = 32;
  measuredSpace->f_bsize  = 4096;
  const auto result       = shmbudget::space();
  EXPECT_TRUE(result.known);
  EXPECT_EQ(result.total, 16000U);
  EXPECT_EQ(result.available, 3936U);
#if RDB_HAS_BOOST_POSIX_SHM
  EXPECT_TRUE(probeWasUnlinked);
  EXPECT_TRUE(probeWasEmpty);
#endif
}

TEST(ShmBudget, ReportSubtractsReservationAndRoundsClientCountDown) {
  struct Restore {
    ~Restore() { measuredSpace.reset(); }
  } restore;
  const auto fixed = shmbudget::fixedReservationBytes();
  const auto queue = shmbudget::responseQueueBytes(10);
  measuredSpace.emplace();
  measuredSpace->f_frsize = 1;
  measuredSpace->f_blocks = fixed + 10 * queue;
  qTree plan;
  plan.push_back(query(1, "first"));
  plan.push_back(query(1, "second"));
  for (const auto available : {fixed + 3 * queue + queue / 2, fixed - 1}) {
    measuredSpace->f_bavail = available;
    const auto report       = shmbudget::report(plan, 10, 1);
    EXPECT_TRUE(report.contains("filesystem behind " + osplat::sharedMemoryBackingPath()));
    EXPECT_TRUE(report.contains("bus segment " + shmbudget::humanBytes(bus::segmentBytes())));
    EXPECT_TRUE(report.contains("command queue " + shmbudget::humanBytes(shmbudget::messageQueueBytes(
                                                       ipc::kQueryQueueMaxMessages, ipc::kQueryQueueMaxMessageSize))));
    EXPECT_TRUE(report.contains("response segment " + shmbudget::humanBytes(ipc::kShmemSegmentSize)));
    EXPECT_TRUE(report.contains(shmbudget::humanBytes(available)));
    EXPECT_TRUE(report.contains(shmbudget::humanBytes(available < fixed ? 0 : available - fixed)));
    const auto position = report.find("  1 s");
    ASSERT_NE(position, std::string::npos) << report;
    std::istringstream row(report.substr(position));
    std::string tick, seconds, amount, unit, first, second;
    int elements = -1, clients = -1;
    row >> tick >> seconds >> elements >> amount >> unit >> clients >> first >> second;
    EXPECT_EQ(tick, "1");
    EXPECT_EQ(seconds, "s");
    EXPECT_EQ(elements, 10);
    EXPECT_EQ(amount + " " + unit, shmbudget::humanBytes(queue));
    EXPECT_EQ(clients, available < fixed ? 0 : 3);
    EXPECT_EQ(first + " " + second, "first, second");
  }
}

TEST(ShmBudget, MeasurementFallbackAndUnknownReportAreExplicit) {
  struct Restore {
    ~Restore() { failDescriptorMeasure = failPathMeasure = false; }
  } restore;
  failDescriptorMeasure = true;
  EXPECT_TRUE(shmbudget::space().known);
  failPathMeasure = true;
  EXPECT_FALSE(shmbudget::space().known);
  qTree plan;
  const auto empty = shmbudget::report(plan, 10, 100);
  EXPECT_TRUE(empty.contains("unknown (statvfs failed)"));
  EXPECT_TRUE(empty.contains("plan has no subscribable streams"));
  plan.push_back(query(1, "src"));
  const auto unknown = shmbudget::report(plan, 10, 100);
  EXPECT_TRUE(unknown.contains("src"));
  EXPECT_TRUE(unknown.contains("?"));
}

TEST(ShmBudget, IntervalCostsGroupInPlanOrderAndSortByQueueBytes) {
  qTree plan;
  plan.push_back(query(0, ":STORAGE"));
  plan.push_back(query(1, "slow"));
  plan.push_back(query(boost::rational<int>(1, 20), "fast"));
  plan.push_back(query(1, "also_slow"));
  const auto costs = shmbudget::intervalCosts(plan, 10, 1);
  ASSERT_EQ(costs.size(), 2U);
  EXPECT_EQ(costs[0].streams, (std::vector<std::string>{"fast"}));
  EXPECT_EQ(costs[1].streams, (std::vector<std::string>{"slow", "also_slow"}));
  EXPECT_GT(costs[0].bytes, costs[1].bytes);
  EXPECT_EQ(costs[0].elements, 200);
  EXPECT_EQ(costs[1].elements, 10);
  EXPECT_EQ(costs[0].bytes, shmbudget::responseQueueBytes(200));
}

}  // namespace
