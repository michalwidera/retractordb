#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

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
