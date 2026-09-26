#include <gtest/gtest.h>

#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

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
  PersistentCounter pc(counterPath());
  EXPECT_EQ(pc.getCount(), 0);
}

// ============================================================
// Konstruktor rezerwuje nastepny numer (#281)
// ============================================================

// Numer nastepnej sesji jest w pliku, zanim ta sesja cokolwiek zarchiwizuje - nie dopiero
// po destrukcji obiektu.
TEST_F(PersistentCounterTest, construction_reserves_next_value) {
  PersistentCounter pc(counterPath());
  EXPECT_EQ(pc.getCount(), 0);

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
// Kolejne instancje akumulują licznik
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
// Nieczytelny plik - odmowa startu, nie rotacja 0 (#281)
// ============================================================

// Plik 0-bajtowy to slad procesu zabitego w trakcie dawnego zapisu (ofstream obcinal plik
// przy otwarciu). Czytany jako 0 kazal planowi nadpisywac archiwa .old0, .old1, ...
TEST_F(PersistentCounterTest, empty_file_is_not_rotation_zero) {
  std::ofstream(counterPath()).close();
  ASSERT_TRUE(std::filesystem::exists(counterPath()));
  ASSERT_EQ(std::filesystem::file_size(counterPath()), 0U);

  EXPECT_EXIT(PersistentCounter pc(counterPath()), ::testing::ExitedWithCode(EXIT_FAILURE),
              "Rotation counter file .* is unreadable \\(0 bytes");
}

TEST_F(PersistentCounterTest, non_numeric_file_is_fatal) {
  std::ofstream(counterPath()) << "not_a_number";
  EXPECT_EXIT(PersistentCounter pc(counterPath()), ::testing::ExitedWithCode(EXIT_FAILURE), "is unreadable");
}

// Dawny `>>` czytal z "12abc" liczbe 12 i ogon przemilczal.
TEST_F(PersistentCounterTest, trailing_garbage_is_fatal) {
  std::ofstream(counterPath()) << "12abc";
  EXPECT_EXIT(PersistentCounter pc(counterPath()), ::testing::ExitedWithCode(EXIT_FAILURE), "is unreadable");
}

// percounter < 0 wylacza w storage rotacje - plik z "-1" nie moze tego zrobic po cichu.
TEST_F(PersistentCounterTest, negative_value_is_fatal) {
  std::ofstream(counterPath()) << "-1";
  EXPECT_EXIT(PersistentCounter pc(counterPath()), ::testing::ExitedWithCode(EXIT_FAILURE), "is unreadable");
}

// Kontrola dodatnia: plik poprawiony recznie w edytorze konczy sie znakiem nowej linii.
TEST_F(PersistentCounterTest, trailing_newline_is_accepted) {
  std::ofstream(counterPath()) << "7\n";
  PersistentCounter pc(counterPath());
  EXPECT_EQ(pc.getCount(), 7);
}

// ============================================================
// Zapis przez plik tymczasowy i rename (#281)
// ============================================================

TEST_F(PersistentCounterTest, save_leaves_no_temp_file) {
  { PersistentCounter pc(counterPath()); }
  size_t entries = 0;
  for ([[maybe_unused]] const auto &entry : std::filesystem::directory_iterator(sandBoxFolder))
    ++entries;
  EXPECT_EQ(entries, 1U);
  EXPECT_TRUE(std::filesystem::exists(counterPath()));
}

// Nieudana rezerwacja zatrzymuje start i nie narusza poprzedniej wartosci. Katalog w miejscu
// pliku tymczasowego wymusza porazke open(); dawny ofstream pisal wprost do celu i by tu
// przeszedl. Katalog powstaje WEWNATRZ instrukcji smierci, bo nazwa pliku tymczasowego niesie
// pid, a instrukcja biegnie w procesie potomnym.
TEST_F(PersistentCounterTest, failed_reservation_is_fatal_and_keeps_previous_value) {
  std::ofstream(counterPath()) << "5";

  EXPECT_EXIT(
      {
        std::filesystem::create_directory(counterPath() + ".tmp." + std::to_string(::getpid()));
        PersistentCounter pc(counterPath());
      },
      ::testing::ExitedWithCode(EXIT_FAILURE), "Cannot reserve rotation number 6");

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
  { PersistentCounter pc(custom); }
  EXPECT_TRUE(std::filesystem::exists(custom));
  EXPECT_FALSE(std::filesystem::exists(counterPath()));
}
