#include <gtest/gtest.h>
#include <sys/resource.h>

#include <algorithm>

#include "retractor/lib/processLimits.hpp"

// ctest -R '^ut_processLimits' -V

namespace {

// Kazdy przypadek zmienia tylko miekki limit procesu testu i przywraca go w TearDown. Limitu
// twardego nie rusza: proces bez uprawnien nie odzyskalby obnizonego limitu twardego.
class OpenFilesLimitTest : public ::testing::Test {
 protected:
  struct rlimit saved{};

  void SetUp() override { ASSERT_EQ(::getrlimit(RLIMIT_NOFILE, &saved), 0); }
  void TearDown() override { ASSERT_EQ(::setrlimit(RLIMIT_NOFILE, &saved), 0); }

  void setSoft(const rlim_t soft) const {
    struct rlimit limit = saved;
    limit.rlim_cur      = soft;
    ASSERT_EQ(::setrlimit(RLIMIT_NOFILE, &limit), 0);
  }

  static rlim_t soft() {
    struct rlimit limit{};
    EXPECT_EQ(::getrlimit(RLIMIT_NOFILE, &limit), 0);
    return limit.rlim_cur;
  }
};

}  // namespace

// Domyslny miekki limit macOS (256) rosnie do celu, ale nie ponad limit twardy.
TEST_F(OpenFilesLimitTest, raises_the_macos_default_to_the_target) {
  // covers: proclimits.raise-open-files-limit.1~1
  const rlim_t start = std::min<rlim_t>(256, saved.rlim_max);
  setSoft(start);
  const auto files    = proclimits::raiseOpenFilesLimit();
  const rlim_t wanted = std::min(proclimits::kOpenFilesTarget, saved.rlim_max);
  EXPECT_EQ(files.err, 0);
  EXPECT_EQ(files.before, start);
  EXPECT_EQ(files.after, wanted);
  EXPECT_EQ(files.hard, saved.rlim_max);
  EXPECT_EQ(soft(), wanted);
}

// Cel ponad limitem twardym konczy sie na limicie twardym, bez bledu.
TEST_F(OpenFilesLimitTest, stops_at_the_hard_limit) {
  // covers: proclimits.raise-open-files-limit.1~1
  if (saved.rlim_max == RLIM_INFINITY) GTEST_SKIP() << "hard RLIMIT_NOFILE is unlimited; no target exceeds it";
  setSoft(std::min<rlim_t>(256, saved.rlim_max));
  const auto files = proclimits::raiseOpenFilesLimit(saved.rlim_max + 1);
  EXPECT_EQ(files.err, 0);
  EXPECT_EQ(files.after, saved.rlim_max);
  EXPECT_EQ(soft(), saved.rlim_max);
}

// Limit juz wyzszy niz cel zostaje, jaki byl.
TEST_F(OpenFilesLimitTest, never_lowers_a_higher_limit) {
  // covers: proclimits.raise-open-files-limit.2~1
  const rlim_t start = std::min<rlim_t>(2 * proclimits::kOpenFilesTarget, saved.rlim_max);
  setSoft(start);
  const auto files = proclimits::raiseOpenFilesLimit();
  EXPECT_EQ(files.err, 0);
  EXPECT_EQ(files.before, start);
  EXPECT_EQ(files.after, start);
  EXPECT_EQ(soft(), start);
}
