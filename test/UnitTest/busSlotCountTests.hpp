#pragma once

#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

// Wspolne regresje dla natywnego zamka i wymuszonej galezi fallback.
// Kazda operacja biegnie w potomku: przed naprawa licznik 4096 moze zabic
// proces, a pozostale przypadki nadal musza sie wykonac i posprzatac IPC.
class BusSlotCount : public ::testing::TestWithParam<std::uint32_t> {
 protected:
  const std::string name = "rdb_slot_count_" + std::to_string(getpid());

  void TearDown() override {
    const std::string path = bus::presencePath(name);
    const int fd           = lockfile::claimAbandoned(path);
    if (fd != -1) {
      IPC::shared_memory_object::remove(name.c_str());
      lockfile::removeAndRelease(path, fd);
    }
  }

  void exercise(const std::function<void(bus::Bus &, bus::Bus &, bus::Segment *)> &scenario) {
    bus::Bus owner(name);
    bus::Bus peer(name);
    bus::Bus candidate(name);
    ASSERT_TRUE(owner.attached());
    ASSERT_TRUE(peer.attached());
    ASSERT_TRUE(candidate.attached());
    ASSERT_EQ(owner.claim({.name = "owner", .streams = {"owned"}}).status, bus::ClaimStatus::Claimed);
    ASSERT_EQ(peer.claim({.name = "peer", .streams = {"foreign"}}).status, bus::ClaimStatus::Claimed);

    IPC::shared_memory_object object(IPC::open_only, name.c_str(), IPC::read_write);
    IPC::mapped_region mapping(object, IPC::read_write);
    auto *segment = static_cast<bus::Segment *>(mapping.get_address());
    // Wszystkie obiekty przeszly walidacje PRZED uszkodzeniem naglowka.
    segment->slotCount = GetParam();
    scenario(owner, candidate, segment);
    segment->slotCount = bus::kMaxSlots;
  }

  // Potomek testu smierci nie przekazuje zdarzen do drukarki gtest, wiec bez
  // tego raport rodzica mowi tylko "exit status 1". Stderr potomka trafia do
  // "Actual msg" rodzica.
  static void reportFailures() {
    const ::testing::TestResult *result = ::testing::UnitTest::GetInstance()->current_test_info()->result();
    for (int i = 0; i < result->total_part_count(); ++i) {
      const ::testing::TestPartResult &part = result->GetTestPartResult(i);
      if (part.failed()) std::cerr << part << '\n';
    }
  }

  void run(const std::function<void(bus::Bus &, bus::Bus &, bus::Segment *)> &scenario) {
    EXPECT_EXIT(
        {
          exercise(scenario);
          reportFailures();
          _exit(::testing::Test::HasFailure() ? 1 : 0);
        },
        ::testing::ExitedWithCode(0), "");
  }
};

TEST_P(BusSlotCount, InstancesStillSeeBothOwners) {
  run([](bus::Bus &owner, bus::Bus &, bus::Segment *) {
    const auto instances = owner.instances();
    ASSERT_EQ(instances.size(), 2U);
    EXPECT_EQ(instances[0].name, "owner");
    EXPECT_EQ(instances[0].streams, (std::vector<std::string>{"owned"}));
    EXPECT_EQ(instances[1].name, "peer");
    EXPECT_EQ(instances[1].streams, (std::vector<std::string>{"foreign"}));
  });
}

TEST_P(BusSlotCount, ClaimScansTheWholeArrayAndRefusesCollision) {
  run([](bus::Bus &, bus::Bus &candidate, bus::Segment *) {
    EXPECT_EQ(candidate.claim({.name = "candidate", .streams = {"fresh"}}).status, bus::ClaimStatus::Claimed);
    const auto refused = candidate.claim({.name = "candidate", .streams = {"foreign"}});
    EXPECT_EQ(refused.status, bus::ClaimStatus::Conflict);
    EXPECT_EQ(refused.ownerName, "peer");
  });
}

TEST_P(BusSlotCount, ReservePlanScansTheWholeArrayAndRefusesCollision) {
  run([](bus::Bus &owner, bus::Bus &, bus::Segment *) {
    ASSERT_EQ(owner.reservePlan({"reserved"}, "", {}).status, bus::ClaimStatus::Claimed);
    const auto refused = owner.reservePlan({"foreign"}, "", {});
    EXPECT_EQ(refused.status, bus::ClaimStatus::Conflict);
    EXPECT_EQ(refused.ownerName, "peer");
    ASSERT_EQ(owner.activateReservedPlan().status, bus::ClaimStatus::Claimed);
    const auto instances = owner.instances();
    ASSERT_EQ(instances.size(), 2U);
    EXPECT_EQ(instances[0].streams, (std::vector<std::string>{"reserved"}));
  });
}

TEST_P(BusSlotCount, ClaimAdditionalScansTheWholeArrayAndRefusesCollision) {
  run([](bus::Bus &owner, bus::Bus &, bus::Segment *) {
    ASSERT_EQ(owner.claimAdditional({"extra"}, {}).status, bus::ClaimStatus::Claimed);
    const auto refused = owner.claimAdditional({"foreign"}, {});
    EXPECT_EQ(refused.status, bus::ClaimStatus::Conflict);
    EXPECT_EQ(refused.ownerName, "peer");
    const auto instances = owner.instances();
    ASSERT_EQ(instances.size(), 2U);
    EXPECT_EQ(instances[0].streams, (std::vector<std::string>{"owned", "extra"}));
  });
}

TEST_P(BusSlotCount, DeadOwnerRecoveryClearsLastInterruptedSlotAndPreservesLiveSlots) {
  run([](bus::Bus &owner, bus::Bus &candidate, bus::Segment *segment) {
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
      auto &slot     = segment->slots[bus::kMaxSlots - 1];
      slot.pid       = static_cast<std::int32_t>(getpid());
      slot.startTime = bus::processStartTime(slot.pid);
      slot.seq       = 1;
      // Smierc bez unlock i bez destruktorow, po rozpoczeciu zapisu slotu.
      _exit(0);
    }
    int status = 0;
    ASSERT_EQ(waitpid(child, &status, 0), child);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);
    ASSERT_EQ(candidate.claim({.name = "candidate", .streams = {"fresh"}}).status, bus::ClaimStatus::Claimed);
    EXPECT_EQ(segment->slots[bus::kMaxSlots - 1].seq, 2U);  // naprawa konczy przerwany zapis: 1 -> 2
    EXPECT_EQ(segment->slots[bus::kMaxSlots - 1].pid, 0);
    const auto instances = owner.instances();
    ASSERT_EQ(instances.size(), 3U);
    EXPECT_EQ(instances[0].name, "owner");
    EXPECT_EQ(instances[1].name, "peer");
    EXPECT_EQ(instances[2].name, "candidate");
  });
}

TEST_P(BusSlotCount, NewAttachmentStillRejectsIncompatibleHeader) {
  run([this](bus::Bus &, bus::Bus &, bus::Segment *) {
    const bus::Bus newcomer(name, false);
    EXPECT_FALSE(newcomer.attached());
  });
}

INSTANTIATE_TEST_SUITE_P(CorruptedHeader, BusSlotCount, ::testing::Values(0U, 4096U));
