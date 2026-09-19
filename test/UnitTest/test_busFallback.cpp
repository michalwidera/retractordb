#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <future>
#include <string>
#include <thread>

#include <gtest/gtest.h>

// Ta binarka testuje rzeczywista galaz bez robust mutex takze na Linuksie.
// Wlaczenie implementacji daje dostep do ukladu segmentu: zatrzymujemy zapis
// deterministycznie, bez haka w produkcyjnym torze przejmowania blokady.
#include "platformConfig.h"
#undef RDB_HAS_ROBUST_MUTEX
#define RDB_HAS_ROBUST_MUTEX 0
#include "retractor/lib/bus.cpp"

namespace {
using namespace std::chrono_literals;

class FallbackLock : public ::testing::Test {
 protected:
  std::string name = "rdb_bus_fallback_" + std::to_string(getpid());
  pid_t child{-1};

  void TearDown() override {
    if (child > 0) {
      kill(child, SIGKILL);
      waitpid(child, nullptr, 0);
    }
    IPC::shared_memory_object::remove(name.c_str());
  }

  void stopOwner(bus::Segment *segment) {
    child = fork();
    ASSERT_NE(child, -1);
    if (child == 0) {
      const auto pid   = static_cast<std::uint32_t>(getpid());
      const auto start = bus::processStartTime(pid);
      std::atomic_ref<std::uint64_t>(segment->mutex.owner).store((std::uint64_t(pid) << 32) | std::uint32_t(start));
      segment->slots[0].pid       = pid;
      segment->slots[0].startTime = start;
      segment->slots[0].seq       = 1;  // zapis przerwany w srodku sekcji krytycznej
      raise(SIGSTOP);
      segment->slots[0].seq = 2;
      std::atomic_ref<std::uint64_t>(segment->mutex.owner).store(0, std::memory_order_release);
      _exit(0);
    }
    int status = 0;
    ASSERT_EQ(waitpid(child, &status, WUNTRACED), child);
    ASSERT_TRUE(WIFSTOPPED(status));
  }
};

TEST_F(FallbackLock, LiveStoppedOwnerKeepsLockBeyondDiagnosticDeadline) {
  bus::Bus candidate(name);
  ASSERT_TRUE(candidate.attached());
  IPC::shared_memory_object object(IPC::open_only, name.c_str(), IPC::read_write);
  IPC::mapped_region mapping(object, IPC::read_write);
  auto *segment = static_cast<bus::Segment *>(mapping.get_address());
  stopOwner(segment);
  auto claimant = std::async(std::launch::async, [&] { return candidate.claim({.name = "candidate", .streams = {"dst"}}); });
  // Stary kod po 10 s przejmowal zamek i zerowal niedokonczony slot zywego procesu.
  EXPECT_EQ(claimant.wait_for(11s), std::future_status::timeout);
  EXPECT_EQ(std::atomic_ref<std::uint32_t>(segment->slots[0].seq).load(), 1U);
  kill(child, SIGCONT);
  waitpid(child, nullptr, 0);
  child = -1;
  EXPECT_EQ(claimant.get().status, bus::ClaimStatus::Claimed);
}

TEST_F(FallbackLock, DeadOwnerIsRecoveredAndInterruptedSlotInvalidated) {
  bus::Bus candidate(name);
  ASSERT_TRUE(candidate.attached());
  IPC::shared_memory_object object(IPC::open_only, name.c_str(), IPC::read_write);
  IPC::mapped_region mapping(object, IPC::read_write);
  auto *segment = static_cast<bus::Segment *>(mapping.get_address());
  stopOwner(segment);
  kill(child, SIGKILL);
  waitpid(child, nullptr, 0);
  child = -1;
  EXPECT_EQ(candidate.claim({.name = "candidate", .streams = {"dst"}}).status, bus::ClaimStatus::Claimed);
  const auto instances = candidate.instances();
  ASSERT_EQ(instances.size(), 1U);
  EXPECT_EQ(instances[0].name, "candidate");
  EXPECT_EQ(segment->slots[0].seq % 2, 0U);
}
}  // namespace
