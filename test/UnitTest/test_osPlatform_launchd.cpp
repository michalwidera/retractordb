#include <unistd.h>

#include <array>
#include <cstdlib>
#include <optional>
#include <string>

#include <gtest/gtest.h>

#include "osPlatform.hpp"
#include "platformConfig.h"

namespace {
pid_t parent       = 1;
uid_t effectiveUid = 0;
pid_t launchdParent() { return parent; }
uid_t launchdUid() { return effectiveUid; }
}  // namespace

// Test heurystyki launchd na kazdej platformie, jak test_busFallback dla muteksu.
#undef RDB_HAS_SYSTEMD
#define RDB_HAS_SYSTEMD 0
#undef RDB_HAS_LAUNCHD
#define RDB_HAS_LAUNCHD 1
#define getppid         launchdParent
#define geteuid         launchdUid
#include "common/osPlatform.cpp"
#undef getppid
#undef geteuid

TEST(LaunchdIdentity, LabelParentAndEffectiveUidSelectTheIdentity) {
  // covers: osplatform.detect-service-identity.2~1, osplatform.detect-service-identity.3~1
  const char *value = std::getenv("XPC_SERVICE_NAME");
  struct Restore {
    std::optional<std::string> value;
    ~Restore() {
      if (value)
        setenv("XPC_SERVICE_NAME", value->c_str(), 1);
      else
        unsetenv("XPC_SERVICE_NAME");
    }
  } restore{value ? std::optional<std::string>{value} : std::nullopt};
  for (const char *label : std::array<const char *, 5>{nullptr, "", "0", "application.test", "com.retractordb.test"}) {
    for (const pid_t pid : {1, 42}) {
      for (const uid_t uid : {0U, 1000U}) {
        SCOPED_TRACE(label ? label : "<unset>");
        SCOPED_TRACE(pid);
        SCOPED_TRACE(uid);
        ASSERT_EQ(label ? setenv("XPC_SERVICE_NAME", label, 1) : unsetenv("XPC_SERVICE_NAME"), 0);
        parent              = pid;
        effectiveUid        = uid;
        const auto identity = osplat::detectServiceIdentity();
        const bool service  = label && std::string(label) == "com.retractordb.test" && pid == 1;
        EXPECT_EQ(identity.unit, service ? std::optional<std::string>{label} : std::nullopt);
        EXPECT_EQ(identity.userScope, service && uid != 0);
      }
    }
  }
}
