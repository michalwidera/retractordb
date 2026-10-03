#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "rdbResult.hpp"
#include "retractor/lib/persistentCounter.hpp"

// ctest -R '^ut_persistentCounter' -V

class PersistentCounterTest : public ::testing::Test {
 protected:
  const std::filesystem::path sandBoxFolder = std::filesystem::temp_directory_path() / "test_persistentCounter";
  const std::string counterFile             = "test_counter";

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
// Pierwszy użycie - brak pliku
// ============================================================

TEST_F(PersistentCounterTest, starts_at_zero_when_no_file) {
  auto pc = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc->getCount(), 0);
}

// ============================================================
// Konstruktor rezerwuje nastepny numer (#281)
// ============================================================

// Numer nastepnej sesji jest w pliku, zanim ta sesja cokolwiek zarchiwizuje - nie dopiero
// po destrukcji obiektu.
TEST_F(PersistentCounterTest, construction_reserves_next_value) {
  auto pc = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc->getCount(), 0);

  std::ifstream in(counterPath());
  int saved = -1;
  in >> saved;
  EXPECT_EQ(saved, 1);
}

// ============================================================
// Druga instancja wczytuje zapisaną wartość
// ============================================================

TEST_F(PersistentCounterTest, second_instance_loads_saved_value) {
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }  // zapisuje 1

  auto pc2 = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc2->getCount(), 1);
}

// ============================================================
// Kolejne instancje akumulują licznik
// ============================================================

TEST_F(PersistentCounterTest, count_accumulates_across_instances) {
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }  // 0 → zapisuje 1
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }  // wczytuje 1 → zapisuje 2
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }  // wczytuje 2 → zapisuje 3

  auto pc = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc->getCount(), 3);
}

// ============================================================
// getCount() nie mutuje stanu przed destrukcją
// ============================================================

TEST_F(PersistentCounterTest, getCount_is_idempotent_before_destruction) {
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }  // zapisuje 1

  auto pc2 = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc2->getCount(), 1);
  EXPECT_EQ(pc2->getCount(), 1);
}

// ============================================================
// Nieczytelny plik - odmowa startu, nie rotacja 0 (#281)
// ============================================================
//
// Odmowa przychodzi od fazy 1 wyjatkiem, nie std::exit: demon konczy sie na nim kodem
// EXIT_FAILURE w main (start) albo w executorsm::run (plan przeladowany), a te testy pilnuja
// typu i komunikatu u zrodla.

// Plik 0-bajtowy to slad procesu zabitego w trakcie dawnego zapisu (ofstream obcinal plik
// przy otwarciu). Czytany jako 0 kazal planowi nadpisywac archiwa .old0, .old1, ...
TEST_F(PersistentCounterTest, empty_file_is_not_rotation_zero) {
  std::ofstream(counterPath()).close();
  ASSERT_TRUE(std::filesystem::exists(counterPath()));
  ASSERT_EQ(std::filesystem::file_size(counterPath()), 0U);

  {
    const auto created = PersistentCounter::create(counterPath());
    ASSERT_RDB_ERROR(created, rdb::Errc::Config);
    EXPECT_THAT(created.error().message(), ::testing::ContainsRegex("Rotation counter file .* is unreadable \\(0 bytes"));
  }
}

TEST_F(PersistentCounterTest, non_numeric_file_is_fatal) {
  std::ofstream(counterPath()) << "not_a_number";
  {
    const auto created = PersistentCounter::create(counterPath());
    ASSERT_RDB_ERROR(created, rdb::Errc::Config);
    EXPECT_THAT(created.error().message(), ::testing::HasSubstr("is unreadable"));
  }
}

// Dawny `>>` czytal z "12abc" liczbe 12 i ogon przemilczal.
TEST_F(PersistentCounterTest, trailing_garbage_is_fatal) {
  std::ofstream(counterPath()) << "12abc";
  {
    const auto created = PersistentCounter::create(counterPath());
    ASSERT_RDB_ERROR(created, rdb::Errc::Config);
    EXPECT_THAT(created.error().message(), ::testing::HasSubstr("is unreadable"));
  }
}

// percounter < 0 wylacza w storage rotacje - plik z "-1" nie moze tego zrobic po cichu.
TEST_F(PersistentCounterTest, negative_value_is_fatal) {
  std::ofstream(counterPath()) << "-1";
  {
    const auto created = PersistentCounter::create(counterPath());
    ASSERT_RDB_ERROR(created, rdb::Errc::Config);
    EXPECT_THAT(created.error().message(), ::testing::HasSubstr("is unreadable"));
  }
}

// Kontrola dodatnia: plik poprawiony recznie w edytorze konczy sie znakiem nowej linii.
TEST_F(PersistentCounterTest, trailing_newline_is_accepted) {
  std::ofstream(counterPath()) << "7\n";
  auto pc = rdbtest::ok(PersistentCounter::create(counterPath()));
  EXPECT_EQ(pc->getCount(), 7);
}

// ============================================================
// Zapis przez plik tymczasowy i rename (#281)
// ============================================================

TEST_F(PersistentCounterTest, save_leaves_no_temp_file) {
  { auto pc = rdbtest::ok(PersistentCounter::create(counterPath())); }
  size_t entries = 0;
  for ([[maybe_unused]] const auto &entry : std::filesystem::directory_iterator(sandBoxFolder))
    ++entries;
  EXPECT_EQ(entries, 1U);
  EXPECT_TRUE(std::filesystem::exists(counterPath()));
}

// Nieudana rezerwacja zatrzymuje start i nie narusza poprzedniej wartosci. Katalog w miejscu
// pliku tymczasowego wymusza porazke open(); dawny ofstream pisal wprost do celu i by tu
// przeszedl. Nazwa pliku tymczasowego niesie pid - tego procesu, bo odmowa jest wyjatkiem, a
// nie smiercia procesu potomnego.
TEST_F(PersistentCounterTest, failed_reservation_is_fatal_and_keeps_previous_value) {
  std::ofstream(counterPath()) << "5";

  std::filesystem::create_directory(counterPath() + ".tmp." + std::to_string(::getpid()));
  {
    const auto created = PersistentCounter::create(counterPath());
    ASSERT_RDB_ERROR(created, rdb::Errc::IO);
    EXPECT_THAT(created.error().message(), ::testing::HasSubstr("Cannot reserve rotation number 6"));
  }

  std::ifstream in(counterPath());
  std::string saved;
  in >> saved;
  EXPECT_EQ(saved, "5");
}

// ============================================================
// Własna nazwa pliku jest respektowana
// ============================================================

TEST_F(PersistentCounterTest, custom_filename_creates_correct_file) {
  std::string custom = (sandBoxFolder / "my_custom_counter").string();
  { auto pc = rdbtest::ok(PersistentCounter::create(custom)); }
  EXPECT_TRUE(std::filesystem::exists(custom));
  EXPECT_FALSE(std::filesystem::exists(counterPath()));
}
