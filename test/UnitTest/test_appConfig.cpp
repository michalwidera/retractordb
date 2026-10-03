#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <toml++/toml.hpp>

#include "retractor/lib/appConfig.hpp"

namespace fs = std::filesystem;

namespace {

// Zapisuje treść do pliku, tworząc katalogi pośrednie.
void writeFile(const fs::path &path, const std::string &content) {
  fs::create_directories(path.parent_path());
  std::ofstream(path) << content;
}

// Test fixture: izoluje warstwę użytkownika przez XDG_CONFIG_HOME wskazujący na
// unikalny katalog tymczasowy. Dzięki temu testy nie zależą od /etc/retractor.
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
  // Brak jakiegokolwiek pliku w warstwie użytkownika → wartości domyślne, nie błąd.
  const AppConfig cfg = loadAppConfig();
  EXPECT_TRUE(cfg.storageDir.empty());
}

TEST_F(AppConfigTest, user_layer_provides_storage_dir) {
  writeFile(userConfigFile(), "[storage]\ndir = \"/var/lib/retractor\"\n");

  const AppConfig cfg = loadAppConfig();

  // Niepusty katalog dostaje końcowy '/' (spójnie z dyrektywą :STORAGE).
  EXPECT_EQ(cfg.storageDir, "/var/lib/retractor/");
}

TEST_F(AppConfigTest, trailing_slash_preserved) {
  writeFile(userConfigFile(), "[storage]\ndir = \"/data/\"\n");

  const AppConfig cfg = loadAppConfig();

  EXPECT_EQ(cfg.storageDir, "/data/");
}

TEST_F(AppConfigTest, malformed_toml_in_layer_falls_back_to_defaults) {
  // Uszkodzony TOML w wyszukiwaniu warstwowym nie może wywrócić usługi.
  writeFile(userConfigFile(), "[storage\ndir = oops");

  const AppConfig cfg = loadAppConfig();

  EXPECT_TRUE(cfg.storageDir.empty());
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
  // Jawnie podana, nieistniejąca ścieżka to twardy błąd (żądanie użytkownika).
  const fs::path missing = tmpDir / "nope.toml";

  EXPECT_THROW(loadAppConfig(missing.string()), toml::parse_error);
}

TEST_F(AppConfigTest, loaded_from_empty_when_no_file_found) {
  // Brak pliku → loadedFrom puste (sygnał, że użyto wartości domyślnych).
  const AppConfig cfg = loadAppConfig();
  EXPECT_TRUE(cfg.loadedFrom.empty());
}

TEST_F(AppConfigTest, loaded_from_records_user_layer_path) {
  writeFile(userConfigFile(), "[storage]\ndir = \"/var/lib/retractor\"\n");

  const AppConfig cfg = loadAppConfig();

  ASSERT_EQ(cfg.loadedFrom.size(), 1u);
  EXPECT_EQ(cfg.loadedFrom.front(), userConfigFile().string());
}

TEST_F(AppConfigTest, loaded_from_records_explicit_path) {
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
