#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <limits>

#include <gtest/gtest.h>

#include "osPlatform.hpp"
#include "platformConfig.h"
#include "unreadableProcess.hpp"

TEST(OsPlatform, InvalidPidAndConfirmedAbsenceHaveEmptySnapshots) {
  // covers: osplatform.inspect-process.1~1, osplatform.inspect-process.3~1
  for (const auto pid : {0, -1, std::numeric_limits<pid_t>::max()}) {
    const auto snapshot = osplat::inspectProcess(pid);
    EXPECT_FALSE(snapshot.found);
    EXPECT_FALSE(snapshot.unreadable);
    EXPECT_FALSE(snapshot.zombie);
    EXPECT_EQ(snapshot.startTime, 0U);
  }
}

TEST(OsPlatform, PermissionDenialIsUnknownRatherThanDead) {
  // covers: osplatform.inspect-process.4~1
  struct Restore {
    ~Restore() { unreadablePid = 0; }
  } restore;
  unreadablePid       = std::numeric_limits<pid_t>::max();
  const auto snapshot = osplat::inspectProcess(unreadablePid.load());
  EXPECT_FALSE(snapshot.found);
  EXPECT_TRUE(snapshot.unreadable);
  EXPECT_FALSE(snapshot.zombie);
  EXPECT_EQ(snapshot.startTime, 0U);
}

TEST(OsPlatform, ExitedChildRemainsAZombieUntilReaped) {
  // covers: osplatform.inspect-process.2~1
  // Podejrzane poza procfs (Darwin): waitid(WNOWAIT) i czas startu zombie z sysctl nie byly tam wykonane.
  if (!RDB_HAS_PROCFS) GTEST_SKIP() << "zombie przez waitid(WNOWAIT) sprawdzony tylko na galezi procfs";
  const pid_t child = fork();
  ASSERT_GE(child, 0);
  if (child == 0) _exit(23);
  struct Reap {
    pid_t child;
    ~Reap() {
      int status = 0;
      while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    }
  } reap{child};
  siginfo_t status{};
  ASSERT_EQ(waitid(P_PID, static_cast<id_t>(child), &status, WEXITED | WNOWAIT), 0);
  ASSERT_EQ(status.si_status, 23);
  const auto snapshot = osplat::inspectProcess(child);
  EXPECT_TRUE(snapshot.found);
  EXPECT_FALSE(snapshot.unreadable);
  EXPECT_TRUE(snapshot.zombie);
  EXPECT_NE(snapshot.startTime, 0U);
}
