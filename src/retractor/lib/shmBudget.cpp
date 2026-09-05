#include "shmBudget.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include <algorithm>
#include <format>
#include <map>
#include <string>
#include <vector>

#include <boost/rational.hpp>

#include "bus.hpp"
#include "constants.hpp"
#include "qTree.hpp"

namespace {

// Uklad segmentu boost::interprocess::message_queue (mq_hdr_t::get_mem_size).
//
// Boost trzyma te funkcje jako skladowa prywatna message_queue, wiec nie da sie jej zawolac;
// zostaje odtworzenie wzoru. Rozjazd z Boostem nie jest bledem kompilacji, wiec pilnuje go
// test jednostkowy, ktory porownuje wynik z rozmiarem NAPRAWDE utworzonej kolejki.
//
// Wielkosci dla 64-bitowego celu: naglowek segmentu razem z ManagedOpenOrCreateUserOffset,
// wpis indeksu (offset_ptr<msg_header>) i naglowek pojedynczej wiadomosci.
constexpr std::uint64_t kQueueHeaderBytes        = 208;
constexpr std::uint64_t kQueueIndexEntryBytes    = 8;
constexpr std::uint64_t kQueueMessageHeaderBytes = 16;
constexpr std::uint64_t kQueueAlignment          = 8;

constexpr std::uint64_t roundUp(std::uint64_t value, std::uint64_t alignment) {
  return (value + alignment - 1) / alignment * alignment;
}

/// Zapasowa droga pomiaru, gdy sonda shm_open nie przejdzie.
shmbudget::Space spaceFromPath(const char *path) {
  struct statvfs vfs{};
  if (statvfs(path, &vfs) != 0) return {};
  return {.known     = true,
          .total     = static_cast<std::uint64_t>(vfs.f_blocks) * vfs.f_frsize,
          .available = static_cast<std::uint64_t>(vfs.f_bavail) * vfs.f_frsize};
}

/// Nazwy strumieni jednego taktu w kolejnosci planu, bez powtorzen.
std::string intervalText(const boost::rational<int> &interval) {
  if (interval.denominator() == 1) return std::format("{} s", interval.numerator());
  return std::format("{}/{} s", interval.numerator(), interval.denominator());
}

}  // namespace

namespace shmbudget {

Space space() {
  // Sonda: obiekt pamieci dzielonej powstaje ta sama droga co obiekty silnika (shm_open),
  // wiec fstatvfs na jego deskryptorze opisuje system plikow, ktory NAPRAWDE ich dotyczy --
  // niezaleznie od tego, gdzie libc go zamontowala. Obiekt ma zerowa dlugosc, wiec sam pomiar
  // nie zajmuje miejsca, ktore mierzy.
  const std::string probeName = std::format("/rdb_shm_probe_{}", getpid());
  const int fd                = shm_open(probeName.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd < 0) return spaceFromPath("/dev/shm");

  struct statvfs vfs{};
  const bool measured = fstatvfs(fd, &vfs) == 0;
  close(fd);
  shm_unlink(probeName.c_str());
  if (!measured) return spaceFromPath("/dev/shm");

  return {.known     = true,
          .total     = static_cast<std::uint64_t>(vfs.f_blocks) * vfs.f_frsize,
          .available = static_cast<std::uint64_t>(vfs.f_bavail) * vfs.f_frsize};
}

int responseQueueElements(const boost::rational<int> &interval, int bufferSeconds, int minElements) {
  if (interval == 0) return minElements;
  return std::max(boost::rational_cast<int>(1 / interval) * bufferSeconds, minElements);
}

std::uint64_t messageQueueBytes(std::uint64_t maxMessages, std::uint64_t maxMessageSize) {
  const std::uint64_t perMessage = roundUp(maxMessageSize, kQueueAlignment) + kQueueMessageHeaderBytes;
  return kQueueHeaderBytes + maxMessages * kQueueIndexEntryBytes + maxMessages * perMessage;
}

std::uint64_t responseQueueBytes(int maxElements) {
  return messageQueueBytes(static_cast<std::uint64_t>(maxElements), ipc::kResponseQueueMaxMessageSize);
}

std::uint64_t fixedReservationBytes() {
  return bus::segmentBytes() + messageQueueBytes(ipc::kQueryQueueMaxMessages, ipc::kQueryQueueMaxMessageSize) +
         ipc::kShmemSegmentSize;
}

std::vector<IntervalCost> intervalCosts(const qTree &plan, int bufferSeconds, int minElements) {
  std::map<boost::rational<int>, IntervalCost> byInterval;
  for (const auto &q : plan) {
    // Dyrektywy kompilatora (:STORAGE, :ROTATION, ...) nie sa strumieniami: nie da sie ich
    // subskrybowac komenda `show`, wiec nie maja kolejki i nie moga trafic do wyceny.
    if (q.isCompilerDirective()) continue;
    auto &entry = byInterval[q.rInterval];
    if (entry.streams.empty()) {
      entry.interval = q.rInterval;
      entry.elements = responseQueueElements(q.rInterval, bufferSeconds, minElements);
      entry.bytes    = responseQueueBytes(entry.elements);
    }
    entry.streams.push_back(q.id);
  }

  std::vector<IntervalCost> retVal;
  retVal.reserve(byInterval.size());
  for (auto &[interval, cost] : byInterval)
    retVal.push_back(std::move(cost));
  std::ranges::sort(retVal, [](const IntervalCost &lhs, const IntervalCost &rhs) { return lhs.bytes > rhs.bytes; });
  return retVal;
}

std::string humanBytes(std::uint64_t bytes) {
  constexpr std::uint64_t kKiB = 1024;
  constexpr std::uint64_t kMiB = kKiB * 1024;
  constexpr std::uint64_t kGiB = kMiB * 1024;

  if (bytes >= kGiB) return std::format("{:.1f} GiB", static_cast<double>(bytes) / kGiB);
  if (bytes >= kMiB) return std::format("{:.1f} MiB", static_cast<double>(bytes) / kMiB);
  if (bytes >= kKiB) return std::format("{:.1f} KiB", static_cast<double>(bytes) / kKiB);
  return std::format("{} B", bytes);
}

std::string report(const qTree &plan, int bufferSeconds, int minElements) {
  const Space fs            = space();
  const std::uint64_t fixed = fixedReservationBytes();

  std::string retVal = "Shared memory budget (shm_open filesystem, usually /dev/shm)\n";
  if (fs.known)
    retVal += std::format("  capacity                 {:>12}\n  free now                 {:>12}\n", humanBytes(fs.total),
                          humanBytes(fs.available));
  else
    retVal += "  capacity                     unknown (statvfs failed)\n";

  retVal += std::format("  fixed reservation        {:>12}   bus segment {} + command queue {} + map segment {}\n",  //
                        humanBytes(fixed), humanBytes(bus::segmentBytes()),
                        humanBytes(messageQueueBytes(ipc::kQueryQueueMaxMessages, ipc::kQueryQueueMaxMessageSize)),
                        humanBytes(ipc::kShmemSegmentSize));

  // Zapas liczony PO rezerwacji stalej: klient placi ze swojej kolejki dopiero z tego, co
  // zostanie instancji. Segment magistrali bywa juz zalozony przez inna instancje, wiec dla
  // dolaczajacej sie jest to ocena z gory -- i taka ma byc, bo raport ma nie obiecywac miejsca.
  const std::uint64_t headroom = (fs.known && fs.available > fixed) ? fs.available - fixed : 0;
  if (fs.known) retVal += std::format("  free after reservation   {:>12}\n", humanBytes(headroom));

  const std::vector<IntervalCost> costs = intervalCosts(plan, bufferSeconds, minElements);
  if (costs.empty()) return retVal + "\n  plan has no subscribable streams\n";

  retVal += "\n  tick          elements   queue per client   clients that fit   streams\n";
  for (const auto &cost : costs) {
    const std::string fits = fs.known ? std::to_string(headroom / cost.bytes) : std::string("?");
    std::string streams    = cost.streams.front();
    for (std::size_t i = 1; i < cost.streams.size(); ++i)
      streams += ", " + cost.streams[i];
    retVal += std::format("  {:<12} {:>8}   {:>16}   {:>16}   {}\n", intervalText(cost.interval), cost.elements,
                          humanBytes(cost.bytes), fits, streams);
  }
  return retVal;
}

}  // namespace shmbudget
