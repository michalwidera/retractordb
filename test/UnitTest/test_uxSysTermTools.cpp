#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <cstring>
#include <filesystem>
#include <string>

#include "uxSysTermTools.hpp"

// --- fixArgcv tests ---

TEST(FixArgcv, strips_trailing_cr_from_arguments) {
  char arg0[]  = "program\r";
  char arg1[]  = "hello\r";
  char *argv[] = {arg0, arg1};

  fixArgcv(2, argv);

  EXPECT_STREQ(argv[0], "program");
  EXPECT_STREQ(argv[1], "hello");
}

TEST(FixArgcv, does_not_modify_arguments_without_trailing_cr) {
  char arg0[]  = "program";
  char arg1[]  = "--flag";
  char *argv[] = {arg0, arg1};

  fixArgcv(2, argv);

  EXPECT_STREQ(argv[0], "program");
  EXPECT_STREQ(argv[1], "--flag");
}

TEST(FixArgcv, handles_empty_string_argument) {
  char arg0[]  = "";
  char *argv[] = {arg0};

  fixArgcv(1, argv);

  EXPECT_STREQ(argv[0], "");
}

TEST(FixArgcv, handles_argc_zero) {
  char *argv[] = {nullptr};
  fixArgcv(0, argv);
  // No crash expected
}

TEST(FixArgcv, handles_null_argv) {
  fixArgcv(1, nullptr);
  // No crash expected
}

TEST(FixArgcv, only_strips_last_character_if_cr) {
  char arg0[]  = "a\rb\r";
  char *argv[] = {arg0};

  fixArgcv(1, argv);

  EXPECT_STREQ(argv[0], "a\rb");
}

// --- _kbhit tests ---

TEST(KbHit, returns_zero_when_ignore_any_key) { EXPECT_EQ(_kbhit(true), 0); }

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
