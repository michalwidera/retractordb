#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "uxSysTermTools.hpp"

// --- fixArgcv tests ---

TEST(FixArgcv, strips_trailing_cr_from_arguments) {
  std::string arg0           = "program\r";
  std::string arg1           = "hello\r";
  std::array<char *, 2> argv = {arg0.data(), arg1.data()};

  fixArgcv(2, argv.data());

  EXPECT_STREQ(argv[0], "program");
  EXPECT_STREQ(argv[1], "hello");
}

TEST(FixArgcv, does_not_modify_arguments_without_trailing_cr) {
  std::string arg0           = "program";
  std::string arg1           = "--flag";
  std::array<char *, 2> argv = {arg0.data(), arg1.data()};

  fixArgcv(2, argv.data());

  EXPECT_STREQ(argv[0], "program");
  EXPECT_STREQ(argv[1], "--flag");
}

TEST(FixArgcv, handles_empty_string_argument) {
  std::string arg0;
  std::array<char *, 1> argv = {arg0.data()};

  fixArgcv(1, argv.data());

  EXPECT_STREQ(argv[0], "");
}

TEST(FixArgcv, handles_argc_zero) {
  std::array<char *, 1> argv = {nullptr};
  fixArgcv(0, argv.data());
  // No crash expected
}

TEST(FixArgcv, handles_null_argv) {
  fixArgcv(1, nullptr);
  // No crash expected
}

TEST(FixArgcv, only_strips_last_character_if_cr) {
  std::string arg0           = "a\rb\r";
  std::array<char *, 1> argv = {arg0.data()};

  fixArgcv(1, argv.data());

  EXPECT_STREQ(argv[0], "a\rb");
}

// --- _kbhit tests ---

TEST(KbHit, returns_zero_when_ignore_any_key) { EXPECT_EQ(_kbhit(true), 0); }

// Regresja defektu wykrytego w kampanii K6b (issue_215).
//
// `_kbhit` woła `getchar()` na stdin. Gdy stdin nie jest terminalem, pierwszy
// bajt WEJŚCIA był brany za klawisz operatora: pętla czytająca dane kończyła
// się natychmiast i wyglądało to na normalne zakończenie. Klient `xqry`
// uruchamiany ze stdin przekierowanym z pliku wychodził kodem 0, nie
// przeczytawszy ani jednego elementu strumienia.
//
// Podmieniamy stdin na plik z zawartością — czyli dokładnie ten przypadek,
// w którym stara implementacja meldowała „naciśnięto klawisz".
class KbHitRedirectedStdin : public ::testing::Test {
 protected:
  int savedStdin{-1};
  std::filesystem::path path;

  void redirectStdinFrom(const std::string &content) {
    path = std::filesystem::temp_directory_path() /
           ("kbhit_" + std::to_string(::getpid()) + "_" + ::testing::UnitTest::GetInstance()->current_test_info()->name());
    std::ofstream(path) << content;
    savedStdin = ::dup(STDIN_FILENO);
    ASSERT_NE(savedStdin, -1);
    const int fd = ::open(path.c_str(), O_RDONLY);
    ASSERT_NE(fd, -1);
    ASSERT_NE(::dup2(fd, STDIN_FILENO), -1);
    ::close(fd);
  }

  void TearDown() override {
    if (savedStdin != -1) {
      ::dup2(savedStdin, STDIN_FILENO);
      ::close(savedStdin);
    }
    if (!path.empty()) std::filesystem::remove(path);
  }
};

TEST_F(KbHitRedirectedStdin, non_tty_with_pending_bytes_is_not_a_keypress) {
  redirectStdinFrom("W2_Q08\tSTRUCT\t5\tw2_out_000\n");
  ASSERT_EQ(::isatty(STDIN_FILENO), 0) << "test wymaga stdin, ktory nie jest terminalem";
  EXPECT_FALSE(_kbhit(false));
}

TEST_F(KbHitRedirectedStdin, non_tty_does_not_consume_stdin) {
  redirectStdinFrom("pierwsza linia\ndruga linia\n");
  ASSERT_EQ(::isatty(STDIN_FILENO), 0);

  EXPECT_FALSE(_kbhit(false));

  // Drugi skutek defektu, gorszy od pierwszego: `_kbhit` KONSUMOWAŁ bajt
  // wejścia. Proces potomny dziedziczący stdin po pętli `while read < plik`
  // gubił przez to dane, bez jednego komunikatu.
  std::array<char, 16> buffer{};
  const ssize_t got = ::read(STDIN_FILENO, buffer.data(), buffer.size() - 1);
  ASSERT_GT(got, 0);
  EXPECT_EQ(std::string(buffer.data(), static_cast<std::size_t>(got)).substr(0, 8), "pierwsza");
}

TEST_F(KbHitRedirectedStdin, non_tty_at_eof_is_not_a_keypress) {
  redirectStdinFrom("");
  ASSERT_EQ(::isatty(STDIN_FILENO), 0);
  EXPECT_FALSE(_kbhit(false));
}

// --- setupLoggerMain tests ---

class SetupLoggerMainTest : public ::testing::Test {
 protected:
  void SetUp() override { spdlog::drop_all(); }
  void TearDown() override { spdlog::drop_all(); }
};

TEST_F(SetupLoggerMainTest, returns_path_ending_with_log) {
  auto result = setupLoggerMain("test_unit_logger", false);
  EXPECT_TRUE(result.ends_with(".log"));
  EXPECT_NE(result.find("test_unit_logger"), std::string::npos);
}

TEST_F(SetupLoggerMainTest, creates_log_file) {
  auto result = setupLoggerMain("test_unit_creates", false);
  spdlog::default_logger()->flush();
  EXPECT_TRUE(std::filesystem::exists(result));
  std::filesystem::remove(result);
}

TEST_F(SetupLoggerMainTest, dual_mode_returns_valid_path) {
  auto result = setupLoggerMain("test_unit_dual", true);
  EXPECT_TRUE(result.ends_with(".log"));
  EXPECT_NE(result.find("test_unit_dual"), std::string::npos);
  spdlog::default_logger()->flush();
  std::filesystem::remove(result);
}

// Tryb usługowy: log na stderr z prefiksem priorytetu sd-daemon "<N>". Przechwytujemy stderr
// (dup2 na plik), logujemy INFO i ERROR, po czym sprawdzamy prefiksy: INFO=>6, ERROR=>3.
TEST_F(SetupLoggerMainTest, service_mode_emits_sd_daemon_priority_prefix) {
  namespace fs               = std::filesystem;
  const fs::path captureFile = fs::temp_directory_path() / "xretractor_service_log_capture.txt";

  std::fflush(stderr);
  const int savedStderr = dup(STDERR_FILENO);
  ASSERT_NE(savedStderr, -1);
  const int captureFd = open(captureFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  ASSERT_NE(captureFd, -1);
  ASSERT_NE(dup2(captureFd, STDERR_FILENO), -1);

  const auto result = setupLoggerMain("test_unit_service", false /* dual */, true /* service */);
  spdlog::info("idle marker");
  spdlog::error("error marker");
  spdlog::default_logger()->flush();

  std::fflush(stderr);
  dup2(savedStderr, STDERR_FILENO);
  close(savedStderr);
  close(captureFd);

  EXPECT_EQ(result, "journald (stderr)");

  std::ifstream in(captureFile);
  std::stringstream ss;
  ss << in.rdbuf();
  const std::string out = ss.str();

  // Test podaza za decyzja projektowa: ciche raportowanie w Release
  // (SPDLOG_ACTIVE_LEVEL=ERROR — zwiazane z efektywnoscia) odfiltrowuje INFO na
  // poziomie runtime (setupLoggerMain ustawia poziom = SPDLOG_ACTIVE_LEVEL), wiec
  // marker INFO w Release LEGALNIE nie powstaje. Prefiks INFO=><6> sprawdzamy tylko
  // gdy build przepuszcza INFO (Debug); prefiks ERROR=><3> obowiazuje zawsze.
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
  EXPECT_NE(out.find("<6>[I] idle marker"), std::string::npos);
#endif
  EXPECT_NE(out.find("<3>[E] error marker"), std::string::npos);

  fs::remove(captureFile);
}
