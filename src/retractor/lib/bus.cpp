#include "bus.hpp"

#include <pthread.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include <spdlog/spdlog.h>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

namespace IPC = boost::interprocess;

namespace bus {

namespace {

// "XRDBBUS\0" zapisane jako liczba: rozpoznaje segment tej magistrali i odroznia go od
// obcego obiektu o tej samej nazwie. Wpisywane JAKO OSTATNIE przy tworzeniu segmentu,
// wiec jego obecnosc jest dowodem, ze naglowek i muteks sa juz zainicjowane.
constexpr std::uint64_t kMagic = 0x5852'4442'4255'5300ULL;

// Numer ukladu slotu. Niezgodnosc => odmowa podlaczenia (praca bez magistrali), bo dwie
// instancje o roznym ukladzie czytalyby swoje sloty jako smiec.
constexpr std::uint32_t kLayoutVersion = 1;

// Ile czasu instancja podlaczajaca sie czeka na dokonczenie inicjalizacji przez tworce.
// Inicjalizacja to truncate + memset + pthread_mutex_init, czyli mikrosekundy; dwie sekundy
// sa buforem na obciazona maszyne, nie oczekiwanym czasem.
constexpr std::chrono::milliseconds kInitWaitLimit{2000};
constexpr std::chrono::milliseconds kInitPollInterval{1};

// Ile razy czytelnik ponawia migawke slotu, zanim uzna go za nieczytelny. Zapis slotu trwa
// jeden memcpy, wiec kilka prob wystarcza; limit istnieje po to, by odczyt nie mogl zawisnac
// nad slotem instancji, ktora zginela w polowie zapisu.
constexpr int kSnapshotRetries = 8;

/// Slot jednej instancji. Wylacznie POD -- patrz komentarz naglowka.
struct Slot {
  std::uint32_t seq;  ///< seqlock: nieparzysty => zapis w toku
  std::int32_t pid;
  std::uint64_t startTime;
  std::uint32_t streamCount;
  char name[kInstanceNameSize];
  char queryFile[kQueryFileSize];
  char streams[kMaxStreams][kStreamNameSize];
};

struct Segment {
  std::uint64_t magic;
  std::uint32_t layoutVersion;
  std::uint32_t slotCount;
  // Rozmiar slotu w naglowku, obok numeru wersji: numer wersji chroni przed zmiana ZNACZENIA
  // pol, a ten rozmiar przed zmiana POJEMNOSCI (kMaxStreams, kStreamNameSize), ktora latwo
  // przeoczyc przy bumpie wersji. Bez niego segment o starym ukladzie czytalby sie jako smiec.
  std::uint32_t slotSize;
  std::uint32_t reserved;  // wyrownanie muteksu do 8 bajtow
  pthread_mutex_t mutex;   ///< robust + pshared; NIE boost::named_mutex -- patrz nizej
  Slot slots[kMaxSlots];
};

/// Kopiuje napis do tablicy o stalym rozmiarze, zawsze zostawiajac terminator.
void storeString(char *dst, std::size_t capacity, std::string_view src) {
  const std::size_t len = std::min(src.size(), capacity - 1);
  std::memcpy(dst, src.data(), len);
  std::memset(dst + len, 0, capacity - len);
}

/// Odczyt tablicy o stalym rozmiarze, odporny na brak terminatora w uszkodzonym slocie.
std::string loadString(const char *src, std::size_t capacity) {
  const auto *end = static_cast<const char *>(std::memchr(src, '\0', capacity));
  return {src, end != nullptr ? static_cast<std::size_t>(end - src) : capacity};
}

void beginWrite(Slot &slot) {
  std::atomic_ref<std::uint32_t> seq(slot.seq);
  seq.store(seq.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_release);
}

void endWrite(Slot &slot) {
  std::atomic_ref<std::uint32_t> seq(slot.seq);
  std::atomic_thread_fence(std::memory_order_release);
  seq.store(seq.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
}

/// Migawka slotu bez blokady. false => slot byl w trakcie zapisu przez cale kSnapshotRetries
/// prob; wolajacy traktuje go wtedy jak nieistniejacy.
bool snapshot(Slot &slot, Slot &out) {
  std::atomic_ref<std::uint32_t> seq(slot.seq);
  for (int attempt = 0; attempt < kSnapshotRetries; ++attempt) {
    const std::uint32_t before = seq.load(std::memory_order_acquire);
    if ((before & 1U) != 0U) continue;
    std::memcpy(&out, &slot, sizeof(Slot));
    std::atomic_thread_fence(std::memory_order_acquire);
    if (seq.load(std::memory_order_relaxed) == before) return true;
  }
  return false;
}

void clearSlot(Slot &slot) {
  beginWrite(slot);
  slot.pid          = 0;
  slot.startTime    = 0;
  slot.streamCount  = 0;
  slot.name[0]      = '\0';
  slot.queryFile[0] = '\0';
  endWrite(slot);
}

}  // namespace

std::uint64_t processStartTime(std::int32_t pid) {
  std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
  if (!stat.is_open()) return 0;

  std::string line;
  if (!std::getline(stat, line)) return 0;

  // Pole 2 (comm) jest w nawiasach i moze zawierac spacje oraz nawiasy, wiec parsowanie
  // zaczyna sie od OSTATNIEGO ')' w linii. Za nim stoi pole 3 (state), a starttime jest
  // polem 22 -- czyli dziewietnastym tokenem za stanem.
  const auto lastParen = line.rfind(')');
  if (lastParen == std::string::npos) return 0;

  std::istringstream fields(line.substr(lastParen + 1));
  std::string token;
  for (int index = 3; index < 22; ++index)
    if (!(fields >> token)) return 0;

  std::uint64_t startTime{0};
  if (!(fields >> startTime)) return 0;
  return startTime;
}

bool isProcessAlive(std::int32_t pid, std::uint64_t startTime) {
  if (pid <= 0) return false;
  const std::uint64_t current = processStartTime(pid);
  return current != 0 && current == startTime;
}

struct Bus::Impl {
  std::unique_ptr<IPC::shared_memory_object> shm;
  std::unique_ptr<IPC::mapped_region> region;
  Segment *segment{nullptr};
  int slotIndex{-1};

  /// Bierze muteks magistrali, obslugujac smierc poprzedniego wlasciciela.
  ///
  /// Boost NIE udostepnia atrybutu robust (named_mutex nie jest robust, wiec proces
  /// zabity z muteksem w reku zawiesilby wszystkie pozostale). Stad surowy pthread_mutex_t
  /// w segmencie: przy EOWNERDEAD stan da sie naprawic.
  bool lock() {
    int rc = pthread_mutex_lock(&segment->mutex);
    if (rc == EOWNERDEAD) {
      // Poprzedni wlasciciel zginal trzymajac muteks. Trzymanie muteksu jest tu dowodem,
      // ze zadna ZYWA instancja nie jest w trakcie zapisu slotu, wiec slot o nieparzystym
      // seq to slot przerwany w polowie -- jego tresc jest smieciem i musi zniknac.
      for (std::uint32_t i = 0; i < segment->slotCount; ++i) {
        Slot &slot = segment->slots[i];
        std::atomic_ref<std::uint32_t> seq(slot.seq);
        if ((seq.load(std::memory_order_relaxed) & 1U) == 0U) continue;
        SPDLOG_WARN("xrdbbus: slot {} interrupted mid-write by a dead owner, invalidating.", i);
        std::memset(&slot, 0, sizeof(Slot));
      }
      pthread_mutex_consistent(&segment->mutex);
      rc = 0;
    }
    if (rc != 0) SPDLOG_ERROR("xrdbbus: cannot lock bus mutex, rc={} ({})", rc, std::strerror(rc));
    return rc == 0;
  }

  void unlock() { pthread_mutex_unlock(&segment->mutex); }
};

Bus::Bus(std::string_view segmentName) : impl(std::make_unique<Impl>()) {
  const std::string name(segmentName);

  bool creator = false;
  try {
    impl->shm = std::make_unique<IPC::shared_memory_object>(IPC::create_only, name.c_str(), IPC::read_write);
    creator   = true;
  } catch (const IPC::interprocess_exception &) {
    try {
      impl->shm = std::make_unique<IPC::shared_memory_object>(IPC::open_only, name.c_str(), IPC::read_write);
    } catch (const IPC::interprocess_exception &e) {
      SPDLOG_WARN("xrdbbus: cannot attach to bus segment '{}': {}", name, e.what());
      impl->shm.reset();
      return;
    }
  }

  try {
    if (creator) impl->shm->truncate(static_cast<IPC::offset_t>(sizeof(Segment)));

    // Instancja podlaczajaca sie moze trafic na segment miedzy create_only a truncate.
    // Odwzorowanie krotszego segmentu dawaloby SIGBUS przy pierwszym dotknieciu, wiec
    // czekamy na pelny rozmiar.
    const auto deadline = std::chrono::steady_clock::now() + kInitWaitLimit;
    IPC::offset_t size{0};
    while (impl->shm->get_size(size) && size < static_cast<IPC::offset_t>(sizeof(Segment))) {
      if (std::chrono::steady_clock::now() > deadline) {
        SPDLOG_WARN("xrdbbus: segment '{}' stayed undersized ({} B, expected {} B); remove /dev/shm/{} to repair.", name, size,
                    sizeof(Segment), name);
        impl->shm.reset();
        return;
      }
      std::this_thread::sleep_for(kInitPollInterval);
    }

    impl->region  = std::make_unique<IPC::mapped_region>(*impl->shm, IPC::read_write);
    impl->segment = static_cast<Segment *>(impl->region->get_address());
  } catch (const IPC::interprocess_exception &e) {
    SPDLOG_WARN("xrdbbus: cannot map bus segment '{}': {}", name, e.what());
    impl->region.reset();
    impl->shm.reset();
    impl->segment = nullptr;
    return;
  }

  std::atomic_ref<std::uint64_t> magic(impl->segment->magic);

  if (creator) {
    std::memset(impl->segment, 0, sizeof(Segment));
    impl->segment->layoutVersion = kLayoutVersion;
    impl->segment->slotCount     = kMaxSlots;
    impl->segment->slotSize      = static_cast<std::uint32_t>(sizeof(Slot));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    const int rc = pthread_mutex_init(&impl->segment->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    if (rc != 0) {
      SPDLOG_ERROR("xrdbbus: cannot initialize robust mutex, rc={} ({})", rc, std::strerror(rc));
      impl->segment = nullptr;
      return;
    }

    // Magic na koncu i przez zapis zwalniajacy: dopiero on oglasza pozostalym, ze naglowek
    // i muteks sa gotowe do uzycia.
    magic.store(kMagic, std::memory_order_release);
  } else {
    const auto deadline = std::chrono::steady_clock::now() + kInitWaitLimit;
    while (magic.load(std::memory_order_acquire) == 0) {
      if (std::chrono::steady_clock::now() > deadline) {
        // Tworca segmentu zginal miedzy create_only a zapisem magic. Segmentu nie kasujemy
        // samoczynnie -- operator usuwa go recznie, bo skasowanie zywego segmentu zerwaloby
        // magistrale pozostalym instancjom.
        SPDLOG_ERROR("xrdbbus: segment '{}' was never initialized; remove /dev/shm/{} to repair.", name, name);
        impl->segment = nullptr;
        return;
      }
      std::this_thread::sleep_for(kInitPollInterval);
    }
  }

  if (magic.load(std::memory_order_acquire) != kMagic || impl->segment->layoutVersion != kLayoutVersion ||
      impl->segment->slotCount != kMaxSlots || impl->segment->slotSize != sizeof(Slot)) {
    SPDLOG_ERROR(
        "xrdbbus: segment '{}' has a foreign or incompatible layout (version {}, {} slots of {} B); "
        "remove /dev/shm/{} to repair. Running without the bus.",
        name, impl->segment->layoutVersion, impl->segment->slotCount, impl->segment->slotSize, name);
    impl->segment = nullptr;
  }
}

Bus::~Bus() {
  release();
  // Segmentu nie kasujemy: inne instancje trzymaja jego odwzorowanie.
}

bool Bus::attached() const { return impl->segment != nullptr; }

ClaimResult Bus::claim(std::string_view serverName, std::string_view queryFile, const std::vector<std::string> &streams) {
  ClaimResult retVal;

  if (!attached()) {
    retVal.detail = "bus segment unavailable";
    return retVal;
  }

  if (streams.size() > kMaxStreams) {
    retVal.status = ClaimStatus::TooLarge;
    retVal.detail = "plan has " + std::to_string(streams.size()) + " streams, the bus slot holds " + std::to_string(kMaxStreams);
    return retVal;
  }
  for (const auto &stream : streams)
    if (stream.size() >= kStreamNameSize) {
      retVal.status = ClaimStatus::TooLarge;
      retVal.stream = stream;
      retVal.detail = "stream name is longer than " + std::to_string(kStreamNameSize - 1) + " characters";
      return retVal;
    }

  release();  // roszczenie jest jednorazowe; powtorne nie ma zostawiac starego slotu

  if (!impl->lock()) {
    retVal.detail = "bus mutex unusable";
    return retVal;
  }

  Segment &segment = *impl->segment;
  auto scratch     = std::make_unique<Slot>();  // ~16 KiB -- na stercie, nie na stosie
  int freeSlot     = -1;

  for (std::uint32_t i = 0; i < segment.slotCount; ++i) {
    Slot &slot = segment.slots[i];
    if (!snapshot(slot, *scratch)) continue;

    if (!isProcessAlive(scratch->pid, scratch->startTime)) {
      // Slot martwy jest wolny. Kasuje go ten, kto to zauwazyl -- bez demona sprzatajacego.
      if (scratch->pid != 0) clearSlot(slot);
      if (freeSlot < 0) freeSlot = static_cast<int>(i);
      continue;
    }

    for (std::uint32_t s = 0; s < std::min(scratch->streamCount, static_cast<std::uint32_t>(kMaxStreams)); ++s) {
      const std::string owned = loadString(scratch->streams[s], kStreamNameSize);
      if (std::ranges::find(streams, owned) == streams.end()) continue;
      retVal.status    = ClaimStatus::Conflict;
      retVal.stream    = owned;
      retVal.ownerName = loadString(scratch->name, kInstanceNameSize);
      retVal.ownerPid  = scratch->pid;
      impl->unlock();
      return retVal;
    }
  }

  if (freeSlot < 0) {
    retVal.status = ClaimStatus::NoFreeSlot;
    retVal.detail = "all " + std::to_string(segment.slotCount) + " bus slots are held by live instances";
    impl->unlock();
    return retVal;
  }

  Slot &mine = segment.slots[freeSlot];
  beginWrite(mine);
  mine.pid         = static_cast<std::int32_t>(getpid());
  mine.startTime   = processStartTime(mine.pid);
  mine.streamCount = static_cast<std::uint32_t>(streams.size());
  storeString(mine.name, kInstanceNameSize, serverName);
  storeString(mine.queryFile, kQueryFileSize, queryFile);
  for (std::size_t s = 0; s < streams.size(); ++s)
    storeString(mine.streams[s], kStreamNameSize, streams[s]);
  endWrite(mine);

  impl->slotIndex = freeSlot;
  impl->unlock();

  retVal.status = ClaimStatus::Claimed;
  return retVal;
}

void Bus::release() {
  if (!attached() || impl->slotIndex < 0) return;

  // Muteks takze przy zwalnianiu: dzieki temu "nieparzysty seq przy trzymanym muteksie"
  // znaczy jednoznacznie "pisarz zginal", co jest podstawa naprawy po EOWNERDEAD.
  if (impl->lock()) {
    clearSlot(impl->segment->slots[impl->slotIndex]);
    impl->unlock();
  }
  impl->slotIndex = -1;
}

std::vector<InstanceInfo> Bus::instances() const {
  std::vector<InstanceInfo> retVal;
  if (!attached()) return retVal;

  Segment &segment = *impl->segment;
  auto scratch     = std::make_unique<Slot>();

  for (std::uint32_t i = 0; i < segment.slotCount; ++i) {
    if (!snapshot(segment.slots[i], *scratch)) continue;
    if (!isProcessAlive(scratch->pid, scratch->startTime)) continue;

    InstanceInfo info;
    info.name      = loadString(scratch->name, kInstanceNameSize);
    info.pid       = scratch->pid;
    info.queryFile = loadString(scratch->queryFile, kQueryFileSize);
    for (std::uint32_t s = 0; s < std::min(scratch->streamCount, static_cast<std::uint32_t>(kMaxStreams)); ++s)
      info.streams.push_back(loadString(scratch->streams[s], kStreamNameSize));
    retVal.push_back(std::move(info));
  }
  return retVal;
}

}  // namespace bus
