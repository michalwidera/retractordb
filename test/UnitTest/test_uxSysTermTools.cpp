#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "platformConfig.h"
#include "uxSysTermTools.hpp"

// --- fixArgcv tests ---

TEST(FixArgcv, strips_trailing_cr_from_arguments) {
  // covers: uxsystermtools.fix-argcv.1~1
  std::string arg0           = "program\r";
  std::string arg1           = "hello\r";
  std::array<char *, 2> argv = {arg0.data(), arg1.data()};

  fixArgcv(2, argv.data());

  EXPECT_STREQ(argv[0], "program");
  EXPECT_STREQ(argv[1], "hello");
}

TEST(FixArgcv, does_not_modify_arguments_without_trailing_cr) {
  // covers: uxsystermtools.fix-argcv.1~1
  std::string arg0           = "program";
  std::string arg1           = "--flag";
  std::array<char *, 2> argv = {arg0.data(), arg1.data()};

  fixArgcv(2, argv.data());

  EXPECT_STREQ(argv[0], "program");
  EXPECT_STREQ(argv[1], "--flag");
}

TEST(FixArgcv, handles_empty_string_argument) {
  // covers: uxsystermtools.fix-argcv.1~1
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
  // covers: uxsystermtools.fix-argcv.1~1
  std::string arg0           = "a\rb\r";
  std::array<char *, 1> argv = {arg0.data()};

  fixArgcv(1, argv.data());

  EXPECT_STREQ(argv[0], "a\rb");
}

// --- _kbhit tests ---

TEST(KbHit, returns_zero_when_ignore_any_key) {
  // covers: uxsystermtools.kbhit.2~1
  EXPECT_EQ(_kbhit(true), 0);
}

// Regresja defektu wykrytego w kampanii K6b (issue_215).
//
// `_kbhit` woła `getchar()` na stdin. Gdy stdin nie jest terminalem, pierwszy
// bajt WEJŚCIA był brany za klawisz operatora: pętla czytająca dane kończyła
// się natychmiast i wyglądało to na normalne zakończenie. Klient `xqry`
// uruchamiany ze stdin przekierowanym z pliku wychodził kodem 0, nie
// przeczytawszy ani jednego elementu strumienia.
//
// Podmieniamy stdin na plik z zawartością - czyli dokładnie ten przypadek,
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
  // covers: uxsystermtools.kbhit.2~1
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
  // covers: uxsystermtools.kbhit.2~1
  redirectStdinFrom("");
  ASSERT_EQ(::isatty(STDIN_FILENO), 0);
  EXPECT_FALSE(_kbhit(false));
}

TEST_F(KbHitRedirectedStdin, getch_reads_the_next_character) {
  // covers: uxsystermtools.getch.1~1
  redirectStdinFrom("qr");
  std::clearerr(stdin);
  EXPECT_EQ(_getch(), 'q');
  EXPECT_EQ(_getch(), 'r');
}

TEST_F(KbHitRedirectedStdin, tty_key_remains_available_and_terminal_mode_is_restored) {
  // covers: uxsystermtools.kbhit.1~1, uxsystermtools.getch.1~1
  struct Pty {
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    int slave  = -1;
    ~Pty() {
      if (slave >= 0) close(slave);
      if (master >= 0) close(master);
    }
  } pty;
  ASSERT_GE(pty.master, 0);
  ASSERT_EQ(grantpt(pty.master), 0);
  ASSERT_EQ(unlockpt(pty.master), 0);
  pty.slave = open(ptsname(pty.master), O_RDWR | O_NOCTTY);
  ASSERT_GE(pty.slave, 0);
  savedStdin = dup(STDIN_FILENO);
  ASSERT_GE(savedStdin, 0);
  ASSERT_GE(dup2(pty.slave, STDIN_FILENO), 0);
  std::clearerr(stdin);
  struct termios before{}, after{};
  ASSERT_EQ(tcgetattr(STDIN_FILENO, &before), 0);
  EXPECT_FALSE(_kbhit(false));
  std::clearerr(stdin);
  ASSERT_EQ(write(pty.master, "k", 1), 1);
  EXPECT_TRUE(_kbhit(false));
  ASSERT_EQ(tcgetattr(STDIN_FILENO, &after), 0);
  // PENDIN jest stanem jadra ustawianym przy powrocie do ICANON, nie ustawieniem operatora.
  const auto settingsMask = ~static_cast<tcflag_t>(PENDIN);
  EXPECT_EQ(after.c_lflag & settingsMask, before.c_lflag & settingsMask);
  EXPECT_EQ(after.c_iflag, before.c_iflag);
  EXPECT_EQ(fcntl(STDIN_FILENO, F_GETFL) & O_NONBLOCK, 0);
  EXPECT_EQ(_getch(), 'k');
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

TEST_F(SetupLoggerMainTest, file_logging_appends_and_dual_logging_keeps_stdout_clean) {
  // covers: uxsystermtools.setup-logger-main.2~1, uxsystermtools.setup-logger-main.3~1, uxsystermtools.setup-logger-main.4~1
  // covers: launcher.xretractor.7~1
  const std::string name = "contract_logger";
  const auto path        = setupLoggerMain(name, false);
  spdlog::error("first marker");
  spdlog::default_logger()->flush();
  spdlog::drop_all();
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  const auto samePath = setupLoggerMain("ignored/directory/" + name, true);
  spdlog::error("second marker");
  spdlog::default_logger()->flush();
  const auto stderrText = testing::internal::GetCapturedStderr();
  const auto stdoutText = testing::internal::GetCapturedStdout();
  EXPECT_EQ(samePath, path);
  EXPECT_TRUE(stdoutText.empty());
  EXPECT_TRUE(stderrText.contains("second marker"));
  EXPECT_EQ(spdlog::default_logger()->level(), static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
  std::ifstream input(path);
  const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  EXPECT_TRUE(text.contains("first marker"));
  EXPECT_TRUE(text.contains("second marker"));
  std::filesystem::remove(path);
}

TEST_F(SetupLoggerMainTest, service_logging_has_no_file_timestamp_or_stdout_and_flushes_each_line) {
  // covers: uxsystermtools.setup-logger-main.1~1
  const auto name = "rdb521-service-log-" + std::to_string(getpid());
  const auto file = std::filesystem::temp_directory_path() / (name + ".log");
  ASSERT_FALSE(std::filesystem::exists(file));
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  const auto destination = setupLoggerMain(name, true, true);
  spdlog::error("service marker");
  const auto error  = testing::internal::GetCapturedStderr();
  const auto output = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(output.empty());
  EXPECT_FALSE(std::filesystem::exists(file));
  EXPECT_EQ(spdlog::default_logger()->sinks().size(), 1U);
  EXPECT_EQ(spdlog::default_logger()->flush_level(), spdlog::level::trace);
  EXPECT_EQ(spdlog::default_logger()->level(), static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
#if RDB_HAS_SYSTEMD
  EXPECT_EQ(destination, "journald (stderr)");
  EXPECT_EQ(error, "<3>[E] service marker\n");
#else
  EXPECT_EQ(destination, "system log (stderr)");
  EXPECT_EQ(error, "[E] service marker\n");
#endif
}

// Tryb usługowy: log na stderr z prefiksem priorytetu sd-daemon "<N>". Przechwytujemy stderr
// (dup2 na plik), logujemy INFO i ERROR, po czym sprawdzamy prefiksy: INFO=>6, ERROR=>3.
TEST_F(SetupLoggerMainTest, service_mode_emits_sd_daemon_priority_prefix) {
  // covers: uxsystermtools.setup-logger-main.1~1, launcher.xretractor.20~1, launcher.xretractor.21~1, launcher.xretractor.22~1
  // covers: launcher.xretractor.24~1, launcher.xretractor.25~1
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

#if RDB_HAS_SYSTEMD
  EXPECT_EQ(result, "journald (stderr)");
#else
  EXPECT_EQ(result, "system log (stderr)");
#endif

  std::ifstream in(captureFile);
  std::stringstream ss;
  ss << in.rdbuf();
  const std::string out = ss.str();

  // Wyjscie trafia do pliku, nie na terminal: log dla journala ma byc czystym tekstem,
  // a kazda linia ma byc zrzucona od razu, bez czekania na jawny flush.
  EXPECT_EQ(out.find('\033'), std::string::npos) << "kod ANSI w logu uslugowym bez TTY";
  EXPECT_EQ(spdlog::default_logger()->flush_level(), spdlog::level::trace);

  // Test podaza za decyzja projektowa: ciche raportowanie w Release
  // (SPDLOG_ACTIVE_LEVEL=ERROR - zwiazane z efektywnoscia) odfiltrowuje INFO na
  // poziomie runtime (setupLoggerMain ustawia poziom = SPDLOG_ACTIVE_LEVEL), wiec
  // marker INFO w Release LEGALNIE nie powstaje. Prefiks INFO=><6> sprawdzamy tylko
  // gdy build przepuszcza INFO (Debug); prefiks ERROR=><3> obowiazuje zawsze.
  //
  // Prefiks "<N>" jest PROTOKOLEM sd-daemon i powstaje tylko tam, gdzie po drugiej
  // stronie stoi journald. Bez systemd log uslugowy ma sam poziom i tresc - patrz
  // setupLoggerMain - wiec asercja na prefiks bylaby tam asercja na smiec w logu.
#if RDB_HAS_SYSTEMD
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
  EXPECT_NE(out.find("<6>[I] idle marker"), std::string::npos);
#endif
  EXPECT_NE(out.find("<3>[E] error marker"), std::string::npos);
#else
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
  EXPECT_NE(out.find("[I] idle marker"), std::string::npos);
#endif
  EXPECT_NE(out.find("[E] error marker"), std::string::npos);
  EXPECT_EQ(out.find('<'), std::string::npos) << "prefiks sd-daemon nie ma prawa powstac poza systemd";
#endif

  fs::remove(captureFile);
}
