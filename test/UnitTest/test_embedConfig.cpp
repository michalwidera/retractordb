#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "embed/configBridge.hpp"
#include "rdbResult.hpp"

namespace rdb::embed {
namespace {
enum class ConfigFault { none, standard, unknown };
ConfigFault configFault = ConfigFault::none;

AppConfig boundaryLoadAppConfig(const std::string &path) {
  switch (std::exchange(configFault, ConfigFault::none)) {
    case ConfigFault::standard:
      throw std::runtime_error("injected config failure");
    case ConfigFault::unknown:
      throw 521;
    case ConfigFault::none:
      return loadAppConfig(path);
  }
  throw std::logic_error("invalid test fault");
}
}  // namespace
}  // namespace rdb::embed

// Kopia niezmienionej granicy z podmienionym tylko w tescie dostawca konfiguracji.
// Nazwa granicy tez jest osobna: Engine nadal uzywa oryginalnej biblioteki.
#define embeddedConfig embeddedConfigUnderTest
#define loadAppConfig  boundaryLoadAppConfig
#include "embed/configBridge.cc"
#undef loadAppConfig
#undef embeddedConfig

TEST(embedConfig, explicit_file_preserves_values_and_rejected_key_reasons) {
  // covers: configbridge.embedded-config.1~1
  const auto locks       = std::filesystem::absolute("chosen_locks").string();
  const std::string path = "explicit.toml";
  std::ofstream(path) << "[paths]\nlock_dir = '" << locks
                      << "'\n[sources]\ntimeout_s = -1\n"
                         "[embedded]\nlenient_lock_dirs = ['relative']\n";
  const auto config = rdb::embed::embeddedConfig(path);
  ASSERT_TRUE(config) << config.error().message();
  EXPECT_EQ(config->lockDir, locks);
  EXPECT_FALSE(config->sourcesTimeoutError.empty());
  EXPECT_FALSE(config->embeddedLenientLockDirsError.empty());
  std::filesystem::remove(path);
}

TEST(embedConfig, missing_malformed_and_relative_lock_paths_are_config_errors) {
  // covers: configbridge.embedded-config.2~1
  const std::string path = "invalid.toml";
  std::filesystem::remove(path);
  for (const std::string contents : {"", "[broken", "[paths]\nlock_dir = 'relative'\n"}) {
    if (!contents.empty()) std::ofstream(path) << contents;
    const auto config = rdb::embed::embeddedConfig(path);
    ASSERT_RDB_ERROR(config, rdb::Errc::Config, path);
    EXPECT_EQ(config.error().err(), 0);
    std::filesystem::remove(path);
  }
}

TEST(embedConfig, standard_and_unknown_loader_exceptions_return_config_results) {
  // covers: configbridge.embedded-config.2~1, configbridge.embedded-config.3~1, configbridge.embedded-config.4~1
  using namespace rdb::embed;
  configFault         = ConfigFault::standard;
  const auto standard = embeddedConfigUnderTest("chosen.toml");
  ASSERT_RDB_ERROR(standard, rdb::Errc::Config, "injected config failure");
  EXPECT_EQ(standard.error().message(), "injected config failure");
  EXPECT_EQ(standard.error().err(), 0);
  configFault        = ConfigFault::unknown;
  const auto unknown = embeddedConfigUnderTest("chosen.toml");
  ASSERT_RDB_ERROR(unknown, rdb::Errc::Config);
  EXPECT_EQ(unknown.error().message(), "cannot read embedded configuration: chosen.toml");
  EXPECT_EQ(unknown.error().err(), 0);
  EXPECT_EQ(configFault, ConfigFault::none);
}
