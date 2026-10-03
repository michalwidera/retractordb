#pragma once

#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <thread>

#include <gtest/gtest.h>

// Regresje X-03 (#282): naprawa slotu po smierci wlasciciela zamka ma trzymac dyscypline
// seqlocka. Dolaczane po bus.cpp, wiec siegaja do snapshot() i ukladu Slot bez haka
// w produkcji; dwie binarki pokrywaja natywny zamek i galaz fallback.
class BusRepairSeqlock : public ::testing::Test {
 protected:
  static constexpr std::uint32_t kVictim = bus::kMaxSlots - 1;
  const std::string name                 = "rdb_repair_seqlock_" + std::to_string(getpid());
  const std::unique_ptr<bus::Slot> zero  = std::make_unique<bus::Slot>();

  void TearDown() override {
    const std::string path = bus::presencePath(name);
    const int fd           = lockfile::claimAbandoned(path);
    if (fd != -1) {
      IPC::shared_memory_object::remove(name.c_str());
      lockfile::removeAndRelease(path, fd);
    }
  }

  // Potomek bierze zamek magistrali, zaczyna zapis slotu kVictim, wypelnia cala jego tresc
  // wzorcem i ginie bez unlock - pisarz przerwany w polowie zapisu. Ginie od SIGKILL z rodzica,
  // nie od _exit: pod Valgrindem _exit potomka procesu wielowatkowego robi kontrole wyciekow,
  // widzi stos watku czytelnika jako "possibly lost" i konczy sie kodem --error-exitcode.
  static void dieMidWrite(bus::Segment *segment) {
    const pid_t child = fork();
    ASSERT_NE(child, -1);
    if (child == 0) {
#if RDB_HAS_ROBUST_MUTEX
      if (pthread_mutex_lock(&segment->mutex) != 0) _exit(2);
#else
      const auto pid   = static_cast<std::uint32_t>(getpid());
      const auto start = bus::processStartTime(pid);
      std::atomic_ref<std::uint64_t>(segment->mutex.owner).store((std::uint64_t(pid) << 32) | std::uint32_t(start));
#endif
      bus::Slot &slot = segment->slots[kVictim];
      bus::beginWrite(slot);
      std::memset(reinterpret_cast<char *>(&slot) + sizeof(slot.seq), 0xAB, sizeof(bus::Slot) - sizeof(slot.seq));
      raise(SIGSTOP);
      _exit(3);  // nieosiagalne: rodzic zabija zatrzymanego potomka
    }
    int status = 0;
    ASSERT_EQ(waitpid(child, &status, WUNTRACED), child);
    ASSERT_TRUE(WIFSTOPPED(status)) << "status potomka " << status;
    ASSERT_EQ(kill(child, SIGKILL), 0);
    ASSERT_EQ(waitpid(child, &status, 0), child);
    ASSERT_TRUE(WIFSIGNALED(status));
    ASSERT_EQ(WTERMSIG(status), SIGKILL);
  }

  bool contentIsZero(const bus::Slot &slot) const {
    const auto *bytes = reinterpret_cast<const char *>(&slot);
    return std::memcmp(bytes + sizeof(slot.seq), reinterpret_cast<const char *>(zero.get()) + sizeof(slot.seq),
                       sizeof(bus::Slot) - sizeof(slot.seq)) == 0;
  }
};

// Czytelnik zapamietal parzyste `before` przed smiercia pisarza i skopiowal tresc w trakcie
// jego zapisu. snapshot() przyjmie te kopie, jesli po naprawie zobaczy znowu `before`, wiec
// naprawa nie moze cofnac licznika - musi opublikowac nowa parzysta wartosc.
TEST_F(BusRepairSeqlock, RepairPublishesFreshEvenSequence) {
  bus::Bus candidate(name);
  ASSERT_TRUE(candidate.attached());
  IPC::shared_memory_object object(IPC::open_only, name.c_str(), IPC::read_write);
  IPC::mapped_region mapping(object, IPC::read_write);
  auto *segment       = static_cast<bus::Segment *>(mapping.get_address());
  bus::Slot &victim   = segment->slots[kVictim];
  const auto sequence = [&victim] { return std::atomic_ref<std::uint32_t>(victim.seq).load(std::memory_order_acquire); };

  const std::uint32_t before = sequence();
  ASSERT_EQ(before & 1U, 0U);
  dieMidWrite(segment);
  ASSERT_EQ(sequence() & 1U, 1U);

  ASSERT_EQ(candidate.claim({.name = "candidate", .streams = {"fresh"}}).status, bus::ClaimStatus::Claimed);
  const std::uint32_t after = sequence();
  EXPECT_EQ(after & 1U, 0U);
  EXPECT_NE(after, before) << "czytelnik z before=" << before << " przyjalby kopie zrobiona w trakcie zapisu";
  EXPECT_TRUE(contentIsZero(victim));
}

// Wszystkie udane migawki slotu naprawianego w kolko musza miec tresc samych zer: przed
// naprawa seq jest nieparzyste, po niej tresc jest wyzerowana. Kopia z bajtem wzorca to
// tresc rozerwana przyjeta przez seqlock. Test statystyczny, ale bez poprawki trafial srednio
// raz na probe (2026-10-03: ~2000 rozerwanych migawek na 2000 prob, obie galezie zamka);
// z poprawka nie moze trafic wcale.
TEST_F(BusRepairSeqlock, ReaderNeverAcceptsTornSlotDuringRepair) {
  using namespace std::chrono_literals;
  constexpr int kMaxTrials = 500;
  constexpr auto kBudget   = 3s;  // pod Valgrindem kazda proba trwa duzo dluzej

  bus::Bus candidate(name);
  ASSERT_TRUE(candidate.attached());
  IPC::shared_memory_object object(IPC::open_only, name.c_str(), IPC::read_write);
  IPC::mapped_region mapping(object, IPC::read_write);
  auto *segment     = static_cast<bus::Segment *>(mapping.get_address());
  bus::Slot &victim = segment->slots[kVictim];

  std::atomic<bool> stop{false};
  std::atomic<std::uint64_t> accepted{0};
  std::atomic<std::uint64_t> torn{0};
  // Watek czytelnika nie alokuje i nie zglasza bledow gtest: fork() w procesie wielowatkowym.
  auto copy = std::make_unique<bus::Slot>();
  std::thread reader([&] {
    while (!stop.load(std::memory_order_relaxed)) {
      if (!bus::snapshot(victim, *copy)) continue;
      accepted.fetch_add(1, std::memory_order_relaxed);
      if (!contentIsZero(*copy)) torn.fetch_add(1, std::memory_order_relaxed);
    }
  });

  int trials          = 0;
  const auto deadline = std::chrono::steady_clock::now() + kBudget;
  while (trials < kMaxTrials && std::chrono::steady_clock::now() < deadline && !HasFailure()) {
    dieMidWrite(segment);
    // claim() bierze zamek i naprawia slot kVictim; release() oddaje slot na nastepna probe.
    EXPECT_EQ(candidate.claim({.name = "candidate", .streams = {"fresh"}}).status, bus::ClaimStatus::Claimed);
    candidate.release();
    ++trials;
  }
  stop.store(true, std::memory_order_relaxed);
  reader.join();

  EXPECT_EQ(torn.load(), 0U) << "rozerwane migawki: " << torn.load() << " z " << accepted.load() << " udanych, prob " << trials;
}
