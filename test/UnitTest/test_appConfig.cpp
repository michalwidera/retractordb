#include <gtest/gtest.h>

#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <toml++/toml.hpp>

#include "logCapture.hpp"
#include "retractor/lib/appConfig.hpp"

// Ta sama implementacja; dolaczenie udostepnia testowi skladanie warstw bez zapisu do /etc.
#include "retractor/lib/appConfig.cpp"

namespace fs = std::filesystem;

namespace {

// Zapisuje treść do pliku, tworząc katalogi pośrednie.
void writeFile(const fs::path &path, const std::string &content) {
  fs::create_directories(path.parent_path());
  std::ofstream(path) << content;
}

// Test fixture: izoluje warstwę użytkownika przez XDG_CONFIG_HOME wskazujący na
// unikalny katalog tymczasowy. Warstwa systemowa nadal jest aktywna.
class AppConfigTest : public ::testing::Test {
 protected:
  fs::path tmpDir;
  std::string savedXdg;
  bool hadXdg{false};

  void SetUp() override {
    tmpDir = fs::temp_directory_path() / fs::path("ut_appConfig_" + std::to_string(::getpid()) + "_" +
                                                  ::testing::UnitTest::GetInstance()->current_test_info()->name());
    fs::remove_all(tmpDir);
    fs::create_directories(tmpDir);

    if (const char *x = std::getenv("XDG_CONFIG_HOME"); x != nullptr) {
      hadXdg   = true;
      savedXdg = x;
    }
    setenv("XDG_CONFIG_HOME", (tmpDir / "xdg").string().c_str(), 1);
  }

  void TearDown() override {
    if (hadXdg)
      setenv("XDG_CONFIG_HOME", savedXdg.c_str(), 1);
    else
      unsetenv("XDG_CONFIG_HOME");
    fs::remove_all(tmpDir);
  }

  // Ścieżka pliku konfiguracyjnego użytkownika zgodna z XDG ustawionym w SetUp.
  fs::path userConfigFile() const { return tmpDir / "xdg" / "retractor" / "retractor.toml"; }
};

}  // namespace

TEST_F(AppConfigTest, missing_files_yield_defaults) {
  // covers: appconfig.load-app-config.3~1
  // Brak jakiegokolwiek pliku w warstwie użytkownika → wartości domyślne, nie błąd.
  const AppConfig cfg = loadAppConfig();
  EXPECT_TRUE(cfg.storageDir.empty());
}

TEST_F(AppConfigTest, user_layer_provides_storage_dir) {
  // covers: appconfig.load-app-config.2~1, appconfig.load-app-config.7~1
  writeFile(userConfigFile(), "[storage]\ndir = \"/var/lib/retractor\"\n");

  const AppConfig cfg = loadAppConfig();

  // Niepusty katalog dostaje końcowy '/' (spójnie z dyrektywą :STORAGE).
  EXPECT_EQ(cfg.storageDir, "/var/lib/retractor/");
}

TEST_F(AppConfigTest, trailing_slash_preserved) {
  // covers: appconfig.load-app-config.7~1
  writeFile(userConfigFile(), "[storage]\ndir = \"/data/\"\n");

  const AppConfig cfg = loadAppConfig();

  EXPECT_EQ(cfg.storageDir, "/data/");
}

TEST_F(AppConfigTest, malformed_toml_in_layer_throws) {
  // covers: appconfig.load-app-config.4~1
  // Uszkodzony plik to blad startu, a nie brak konfiguracji.
  writeFile(userConfigFile(), "[storage\ndir = oops");

  try {
    (void)loadAppConfig();
    FAIL() << "Malformed layer was accepted";
  } catch (const toml::parse_error &e) {
    EXPECT_NE(std::string(e.what()).find(userConfigFile().string()), std::string::npos);
    EXPECT_NE(std::string(e.what()).find("Error while parsing"), std::string::npos);
  }
}

TEST_F(AppConfigTest, service_query_file_defaults_to_canonical_path) {
  const AppConfig cfg = loadAppConfig();
  EXPECT_EQ(cfg.serviceQueryFile, appcfg::kDefaultServiceQueryFile);
}

TEST_F(AppConfigTest, user_layer_overrides_service_query_file) {
  writeFile(userConfigFile(), "[service]\nquery_file = \"/srv/retractor/queries.rql\"\n");

  const AppConfig cfg = loadAppConfig();

  EXPECT_EQ(cfg.serviceQueryFile, "/srv/retractor/queries.rql");
}

TEST_F(AppConfigTest, server_autoname_defaults_to_off) {
  // Brak klucza = tryb historyczny: instancja bez nazwy, blokada i IPC bez sufiksu.
  const AppConfig cfg = loadAppConfig();
  EXPECT_FALSE(cfg.serverAutoName);
}

TEST_F(AppConfigTest, user_layer_enables_server_autoname) {
  writeFile(userConfigFile(), "[server]\nautoname = true\n");

  const AppConfig cfg = loadAppConfig();

  EXPECT_TRUE(cfg.serverAutoName);
}

TEST_F(AppConfigTest, explicit_config_path_is_read) {
  const fs::path explicitFile = tmpDir / "custom.toml";
  writeFile(explicitFile, "[storage]\ndir = \"/srv/rdb\"\n");

  const AppConfig cfg = loadAppConfig(explicitFile.string());

  EXPECT_EQ(cfg.storageDir, "/srv/rdb/");
}

TEST_F(AppConfigTest, explicit_missing_config_path_throws) {
  // covers: appconfig.load-app-config.1~1
  // Jawnie podana, nieistniejąca ścieżka to twardy błąd (żądanie użytkownika).
  const fs::path missing = tmpDir / "nope.toml";

  EXPECT_THROW(loadAppConfig(missing.string()), toml::parse_error);
}

TEST_F(AppConfigTest, explicit_path_ignores_a_malformed_user_layer) {
  // covers: appconfig.load-app-config.1~1
  writeFile(userConfigFile(), "[broken");
  const auto file = tmpDir / "explicit.toml";
  writeFile(file, "[ipc]\nmin_queue_elements = 17\n");
  const auto cfg = loadAppConfig(file.string());
  EXPECT_EQ(cfg.ipcMinQueueElements, 17);
  EXPECT_EQ(cfg.loadedFrom, (std::vector<std::string>{file.string()}));
}

TEST_F(AppConfigTest, malformed_explicit_path_names_the_file) {
  // covers: appconfig.load-app-config.1~1, appconfig.load-app-config.4~1
  const auto file = tmpDir / "broken.toml";
  writeFile(file, "[broken");
  try {
    (void)loadAppConfig(file.string());
    FAIL() << "Malformed explicit configuration was accepted";
  } catch (const toml::parse_error &error) {
    EXPECT_TRUE(std::string(error.what()).contains(file.string()));
  }
}

TEST_F(AppConfigTest, layered_tables_override_only_present_keys_and_validate_each_layer) {
  // covers: appconfig.load-app-config.2~1, appconfig.load-app-config.5~1
  AppConfig cfg;
  applyTable(toml::parse("[ipc]\nmin_queue_elements = 17\nqueue_buffer_seconds = 23\n"), cfg, "system.toml");
  applyTable(toml::parse("[ipc]\nmin_queue_elements = 31\n"), cfg, "user.toml");
  EXPECT_EQ(cfg.ipcMinQueueElements, 31);
  EXPECT_EQ(cfg.ipcQueueBufferSeconds, 23);
  try {
    applyTable(toml::parse("[paths]\nlock_dir = 'relative'\n"), cfg, "system.toml");
    FAIL() << "Invalid earlier layer was accepted";
  } catch (const std::invalid_argument &error) {
    EXPECT_TRUE(std::string(error.what()).contains("system.toml"));
  }
}

TEST_F(AppConfigTest, existence_error_is_not_treated_as_a_missing_layer) {
  // covers: appconfig.load-app-config.3~1
  fs::create_directories(userConfigFile().parent_path());
  fs::create_symlink(userConfigFile().filename(), userConfigFile());
  try {
    (void)loadAppConfig();
    FAIL() << "Symlink loop was treated as an absent configuration";
  } catch (const fs::filesystem_error &error) {
    EXPECT_EQ(error.path1(), userConfigFile());
    EXPECT_EQ(error.code(), std::errc::too_many_symbolic_link_levels);
  }
}

TEST_F(AppConfigTest, invalid_numeric_settings_fall_back_with_keyed_warnings) {
  // covers: appconfig.load-app-config.7~1
  for (const int value : {0, -1}) {
    SCOPED_TRACE(value);
    const auto text = std::to_string(value);
    const auto file = tmpDir / "numeric.toml";
    writeFile(file, "[ipc]\nqueue_buffer_seconds = " + text + "\nmin_queue_elements = " + text +
                        "\nclient_response_max_fails = " + text + "\n[timing]\nserver_startup_wait_s = " + text +
                        "\nserver_startup_poll_ms = " + text + "\nquery_no_data_timeout_ms = " + text +
                        "\n[scheduling]\nrt_priority = " + text + "\n[limits]\nhistory_memory_mib = " + text + "\n");
    LogCapture log;
    const auto cfg = loadAppConfig(file.string());
    const AppConfig defaults;
    EXPECT_EQ(cfg.ipcQueueBufferSeconds, defaults.ipcQueueBufferSeconds);
    EXPECT_EQ(cfg.ipcMinQueueElements, defaults.ipcMinQueueElements);
    EXPECT_EQ(cfg.ipcClientResponseMaxFails, defaults.ipcClientResponseMaxFails);
    EXPECT_EQ(cfg.timingServerStartupWaitSeconds, defaults.timingServerStartupWaitSeconds);
    EXPECT_EQ(cfg.timingServerStartupPollIntervalMs, defaults.timingServerStartupPollIntervalMs);
    EXPECT_EQ(cfg.timingQueryNoDataTimeoutMs, defaults.timingQueryNoDataTimeoutMs);
    EXPECT_EQ(cfg.schedulingRtPriority, defaults.schedulingRtPriority);
    EXPECT_EQ(cfg.historyMemoryMib, defaults.historyMemoryMib);
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
    for (const auto key : {"ipc.queue_buffer_seconds", "ipc.min_queue_elements", "ipc.client_response_max_fails",
                           "timing.server_startup_wait_s", "timing.server_startup_poll_ms", "timing.query_no_data_timeout_ms",
                           "scheduling.rt_priority", "limits.history_memory_mib"})
      EXPECT_TRUE(log.text().contains(key)) << key;
#endif
  }
}

TEST_F(AppConfigTest, rt_priority_accepts_both_bounds_and_refuses_one_above) {
  // covers: appconfig.load-app-config.7~1
  const auto file = tmpDir / "priority.toml";
  for (const int priority : {1, 99, 100}) {
    writeFile(file, "[scheduling]\nrt_priority = " + std::to_string(priority) + "\n");
    EXPECT_EQ(loadAppConfig(file.string()).schedulingRtPriority, priority == 100 ? 50 : priority);
  }
}

TEST_F(AppConfigTest, loaded_from_empty_when_no_file_found) {
  // covers: appconfig.load-app-config.6~1
  // Brak pliku → loadedFrom puste (sygnał, że użyto wartości domyślnych).
  const AppConfig cfg = loadAppConfig();
  EXPECT_TRUE(cfg.loadedFrom.empty());
}

TEST_F(AppConfigTest, loaded_from_records_user_layer_path) {
  // covers: appconfig.load-app-config.6~1
  writeFile(userConfigFile(), "[storage]\ndir = \"/var/lib/retractor\"\n");

  const AppConfig cfg = loadAppConfig();

  ASSERT_EQ(cfg.loadedFrom.size(), 1u);
  EXPECT_EQ(cfg.loadedFrom.front(), userConfigFile().string());
}

TEST_F(AppConfigTest, loaded_from_records_explicit_path) {
  // covers: appconfig.load-app-config.6~1
  const fs::path explicitFile = tmpDir / "custom.toml";
  writeFile(explicitFile, "[storage]\ndir = \"/srv/rdb\"\n");

  const AppConfig cfg = loadAppConfig(explicitFile.string());

  ASSERT_EQ(cfg.loadedFrom.size(), 1u);
  EXPECT_EQ(cfg.loadedFrom.front(), explicitFile.string());
}

// [limits] history_memory_mib - budzet pamieci historii planu, sprawdzany przy kazdej kompilacji
// (compiler::setHistoryMemoryBudget). Brak klucza = 1024 MiB.
TEST_F(AppConfigTest, history_memory_budget_defaults_to_1024_mib) {
  const AppConfig cfg = loadAppConfig();
  EXPECT_EQ(cfg.historyMemoryMib, 1024);
}

TEST_F(AppConfigTest, user_layer_sets_history_memory_budget) {
  writeFile(userConfigFile(), "[limits]\nhistory_memory_mib = 64\n");

  const AppConfig cfg = loadAppConfig();

  EXPECT_EQ(cfg.historyMemoryMib, 64);
}

TEST_F(AppConfigTest, non_positive_history_memory_budget_falls_back_to_default) {
  for (const std::string value : {"0", "-5"}) {
    const fs::path explicitFile = tmpDir / "custom.toml";
    writeFile(explicitFile, "[limits]\nhistory_memory_mib = " + value + "\n");

    const AppConfig cfg = loadAppConfig(explicitFile.string());

    EXPECT_EQ(cfg.historyMemoryMib, 1024) << value;
  }
}

TEST_F(AppConfigTest, history_memory_budget_of_wrong_type_is_ignored) {
  writeFile(userConfigFile(), "[limits]\nhistory_memory_mib = \"lots\"\n");

  const AppConfig cfg = loadAppConfig();

  EXPECT_EQ(cfg.historyMemoryMib, 1024);
}

// [storage] default_retention - retencja strumieni plikowych bez RETENTION (D8). Brak klucza =
// brak retencji: danych nie kasuje nic poza jawna decyzja operatora.
TEST_F(AppConfigTest, default_retention_defaults_to_none) {
  const AppConfig cfg = loadAppConfig();
  EXPECT_TRUE(cfg.defaultRetention.noRetention());
}

TEST_F(AppConfigTest, user_layer_sets_default_retention) {
  writeFile(userConfigFile(), "[storage]\ndefault_retention = [100, 4]\n");

  const AppConfig cfg = loadAppConfig();

  EXPECT_EQ(cfg.defaultRetention.capacity, 100U);
  EXPECT_EQ(cfg.defaultRetention.segments, 4U);
}

// Segmenty 0 to w RQL "bez limitu", wiec jako granica nic by nie ograniczaly.
TEST_F(AppConfigTest, invalid_default_retention_means_none) {
  for (const std::string value : {"[0, 4]", "[100, 0]", "[-1, 2]", "[100]", "[100, 4, 1]", "\"100 4\"", "[1.5, 2]",
                                  "[100, 3000000000]", "[\"a\", 2]", "100"}) {
    const fs::path explicitFile = tmpDir / "custom.toml";
    writeFile(explicitFile, "[storage]\ndefault_retention = " + value + "\n");

    const AppConfig cfg = loadAppConfig(explicitFile.string());

    EXPECT_TRUE(cfg.defaultRetention.noRetention()) << value;
  }
}

// `[sources] timeout_s` (#347): brak klucza to cos innego niz jawne 0 - w RQL wygrywa i tak klauzula,
// ale log startu mowi, skad pochodzi termin.
TEST_F(AppConfigTest, sources_timeout_defaults_to_none) {
  const AppConfig cfg = loadAppConfig();
  EXPECT_FALSE(cfg.sourcesTimeoutSeconds.has_value());
  EXPECT_TRUE(cfg.sourcesTimeoutError.empty());
}

TEST_F(AppConfigTest, user_layer_sets_sources_timeout) {
  for (const auto &[value, seconds] :
       std::vector<std::pair<std::string, double>>{{"0.0", 0.0}, {"0", 0.0}, {"0.25", 0.25}, {"2", 2.0}}) {
    writeFile(userConfigFile(), "[sources]\ntimeout_s = " + value + "\n");

    const AppConfig cfg = loadAppConfig();

    ASSERT_TRUE(cfg.sourcesTimeoutSeconds.has_value()) << value;
    EXPECT_EQ(*cfg.sourcesTimeoutSeconds, seconds) << value;
    EXPECT_TRUE(cfg.sourcesTimeoutError.empty()) << value;
  }
}

// Wartosc niepoprawna nie wraca po cichu do 0: zostaje powod, a start xretractora konczy sie bledem.
// loadAppConfig sam nie rzuca, bo ten sam plik czytaja xqry i xtrdb.
TEST_F(AppConfigTest, invalid_sources_timeout_is_an_error_not_a_default) {
  for (const std::string value : {"-1", "-0.5", "nan", "inf", "86401", "\"0.5\"", "true", "[0.5]"}) {
    const fs::path explicitFile = tmpDir / "custom.toml";
    writeFile(explicitFile, "[sources]\ntimeout_s = " + value + "\n");

    AppConfig cfg;
    ASSERT_NO_THROW(cfg = loadAppConfig(explicitFile.string())) << value;

    EXPECT_FALSE(cfg.sourcesTimeoutSeconds.has_value()) << value;
    EXPECT_TRUE(cfg.sourcesTimeoutError.starts_with("sources.timeout_s must be a number of seconds from 0 to 86400"))
        << value << ": " << cfg.sourcesTimeoutError;
  }
}

TEST_F(AppConfigTest, storage_ref_dirs_default_to_none) {
  const AppConfig cfg = loadAppConfig();
  EXPECT_TRUE(cfg.storageRefDirs.empty());
  EXPECT_TRUE(cfg.storageRefDirsError.empty());
}

TEST_F(AppConfigTest, user_layer_sets_storage_ref_dirs) {
  fs::create_directories(tmpDir / "archive");
  writeFile(userConfigFile(), "[storage]\nref_dirs = [\"" + (tmpDir / "archive").string() + "\"]\n");

  const AppConfig cfg = loadAppConfig();

  EXPECT_EQ(cfg.storageRefDirs, std::vector<std::string>{(tmpDir / "archive").string()});
  EXPECT_TRUE(cfg.storageRefDirsError.empty());
}

// Lista uprawnien zapisu nie zgaduje: wpis wzgledny, nieistniejacy albo nie-napis to powod odmowy,
// a nie pominiecie wpisu. loadAppConfig sam nie rzuca, bo ten sam plik czyta xretractor i xqry.
TEST_F(AppConfigTest, invalid_storage_ref_dirs_is_an_error) {
  std::ofstream(tmpDir / "file") << "x";
  for (const std::string &value :
       std::vector<std::string>{"\"" + tmpDir.string() + "\"", "[\"relative/dir\"]", "[\"" + (tmpDir / "none").string() + "\"]",
                                "[\"" + (tmpDir / "file").string() + "\"]", "[1]"}) {
    const fs::path explicitFile = tmpDir / "custom.toml";
    writeFile(explicitFile, "[storage]\nref_dirs = " + value + "\n");

    AppConfig cfg;
    ASSERT_NO_THROW(cfg = loadAppConfig(explicitFile.string())) << value;

    EXPECT_TRUE(cfg.storageRefDirs.empty()) << value;
    EXPECT_TRUE(cfg.storageRefDirsError.starts_with("storage.ref_dirs must be an array of absolute directory paths"))
        << value << ": " << cfg.storageRefDirsError;
  }
}

// #450: lista lagodnych katalogow blokad silnika osadzonego. Brak klucza (nullopt) znaczy z definicji
// katalog tymczasowy systemu, a pusta lista - zaden. Katalog nie musi istniec; wpis wzgledny albo
// nie-napis to powod odmowy, ktory zglasza silnik osadzony - loadAppConfig nie rzuca, bo ten sam plik
// czyta xretractor, ktory klucza nie uzywa.
TEST_F(AppConfigTest, embedded_lenient_lock_dirs) {
  EXPECT_FALSE(loadAppConfig().embeddedLenientLockDirs.has_value());

  const fs::path explicitFile = tmpDir / "custom.toml";
  writeFile(explicitFile, "[embedded]\nlenient_lock_dirs = [\"/tmp\", \"" + (tmpDir / "none").string() + "\"]\n");
  AppConfig cfg = loadAppConfig(explicitFile.string());
  EXPECT_EQ(cfg.embeddedLenientLockDirs, (std::vector<std::string>{"/tmp", (tmpDir / "none").string()}));
  EXPECT_TRUE(cfg.embeddedLenientLockDirsError.empty());

  writeFile(explicitFile, "[embedded]\nlenient_lock_dirs = []\n");
  cfg = loadAppConfig(explicitFile.string());
  EXPECT_EQ(cfg.embeddedLenientLockDirs, std::vector<std::string>{});

  for (const std::string &value : {std::string("\"/tmp\""), std::string("[\"relative/dir\"]"), std::string("[1]")}) {
    writeFile(explicitFile, "[embedded]\nlenient_lock_dirs = " + value + "\n");
    ASSERT_NO_THROW(cfg = loadAppConfig(explicitFile.string())) << value;
    EXPECT_FALSE(cfg.embeddedLenientLockDirs.has_value()) << value;
    EXPECT_TRUE(cfg.embeddedLenientLockDirsError.starts_with("embedded.lenient_lock_dirs must be an array of absolute paths"))
        << value << ": " << cfg.embeddedLenientLockDirsError;
  }
}

TEST_F(AppConfigTest, relative_lock_dir_is_an_error_in_discovered_and_explicit_files) {
  // covers: appconfig.load-app-config.5~1
  writeFile(userConfigFile(), "[paths]\nlock_dir = \"relative/locks\"\n");
  for (const auto &path : {std::optional<std::string>{}, std::optional<std::string>{userConfigFile().string()}}) {
    try {
      (void)loadAppConfig(path);
      FAIL() << "Relative lock directory was accepted";
    } catch (const std::invalid_argument &e) {
      const std::string message = e.what();
      EXPECT_NE(message.find(userConfigFile().string()), std::string::npos);
      EXPECT_NE(message.find("paths.lock_dir='relative/locks' must be an absolute path"), std::string::npos);
    }
  }
}

TEST_F(AppConfigTest, absolute_and_empty_lock_dirs_are_valid) {
  for (const std::string &dir : {tmpDir.string(), std::string{}}) {
    writeFile(userConfigFile(), "[paths]\nlock_dir = \"" + dir + "\"\n");
    EXPECT_EQ(loadAppConfig().lockDir, dir);
  }
}

TEST_F(AppConfigTest, discovered_file_that_cannot_be_read_is_not_absent) {
  // Root czyta plik mimo braku praw, wiec nie da sie tu odtworzyc pliku nieczytelnego.
  if (::geteuid() == 0) GTEST_SKIP() << "root ignores file permissions";
  writeFile(userConfigFile(), "[paths]\nlock_dir = \"\"\n");
  fs::permissions(userConfigFile(), fs::perms::none);
  EXPECT_THROW(loadAppConfig(), toml::parse_error);
  fs::permissions(userConfigFile(), fs::perms::owner_read | fs::perms::owner_write);
}
