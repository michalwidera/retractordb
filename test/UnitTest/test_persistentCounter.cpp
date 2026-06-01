#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "retractor/lib/persistentCounter.hpp"

// ctest -R '^ut_persistentCounter' -V

class PersistentCounterTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder =
      std::filesystem::temp_directory_path() / "test_persistentCounter";
  const std::string counterFile = "test_counter";

  void SetUp() override {
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
    std::filesystem::create_directories(sandBoxFolder);
    std::filesystem::current_path(sandBoxFolder);
  }

  void TearDown() override {
    if (std::filesystem::is_directory(sandBoxFolder)) {
      std::filesystem::remove_all(sandBoxFolder);
    }
  }

  std::string counterPath() { return (sandBoxFolder / counterFile).string(); }
};

// ============================================================
// Pierwszy użycie — brak pliku
// ============================================================

TEST_F(PersistentCounterTest, starts_at_zero_when_no_file) {
  PersistentCounter pc(counterPath());
  EXPECT_EQ(pc.getCount(), 0);
}

// ============================================================
// Destruktor inkrementuje i zapisuje
// ============================================================

TEST_F(PersistentCounterTest, destructor_saves_incremented_value) {
  {
    PersistentCounter pc(counterPath());
    EXPECT_EQ(pc.getCount(), 0);
  }
  std::ifstream in(counterPath());
  int saved = -1;
  in >> saved;
  EXPECT_EQ(saved, 1);
}

// ============================================================
// Druga instancja wczytuje zapisaną wartość
// ============================================================

TEST_F(PersistentCounterTest, second_instance_loads_saved_value) {
  { PersistentCounter pc(counterPath()); }  // zapisuje 1

  PersistentCounter pc2(counterPath());
  EXPECT_EQ(pc2.getCount(), 1);
}

// ============================================================
// Wielokrotne destrukcje akumulują licznik
// ============================================================

TEST_F(PersistentCounterTest, count_accumulates_across_instances) {
  { PersistentCounter pc(counterPath()); }  // 0 → zapisuje 1
  { PersistentCounter pc(counterPath()); }  // wczytuje 1 → zapisuje 2
  { PersistentCounter pc(counterPath()); }  // wczytuje 2 → zapisuje 3

  PersistentCounter pc(counterPath());
  EXPECT_EQ(pc.getCount(), 3);
}

// ============================================================
// getCount() nie mutuje stanu przed destrukcją
// ============================================================

TEST_F(PersistentCounterTest, getCount_is_idempotent_before_destruction) {
  { PersistentCounter pc(counterPath()); }  // zapisuje 1

  PersistentCounter pc2(counterPath());
  EXPECT_EQ(pc2.getCount(), 1);
  EXPECT_EQ(pc2.getCount(), 1);
}

// ============================================================
// Uszkodzony plik — fallback do zera
// ============================================================

TEST_F(PersistentCounterTest, corrupted_file_defaults_to_zero) {
  std::ofstream out(counterPath());
  out << "not_a_number";
  out.close();

  PersistentCounter pc(counterPath());
  EXPECT_EQ(pc.getCount(), 0);
}

// ============================================================
// Własna nazwa pliku jest respektowana
// ============================================================

TEST_F(PersistentCounterTest, custom_filename_creates_correct_file) {
  std::string custom = (sandBoxFolder / "my_custom_counter").string();
  { PersistentCounter pc(custom); }
  EXPECT_TRUE(std::filesystem::exists(custom));
  EXPECT_FALSE(std::filesystem::exists(counterPath()));
}
