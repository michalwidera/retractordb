#include <gtest/gtest.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

#include "fatalError.hpp"
#include "rdb/error.hpp"

// ctest -R '^ut_fatalError' -V

// Wywolania w silniku pisza FatalError bez przestrzeni nazw; nazwa globalna ma prowadzic
// do rdb::FatalError, a CTAD ma przejsc przez using-declaration.
static_assert(std::is_same_v<decltype(FatalError("x {}", 1)), rdb::FatalError<int>>);

namespace {

// Handler atexit widzi zatrzask tak, jak widzi go executorsm::cleanup(): po std::exit
// z akcji konczacej proces.
void reportLatch() { std::cerr << "latch=" << (fatalErrorRaised.load(std::memory_order_acquire) ? 1 : 0) << "\n"; }

[[noreturn]] void raiseInternal() {
  std::atexit(reportLatch);
  FatalError("internal {} {}", "fault", 42);
}

[[noreturn]] void raiseEnvironment() {
  std::atexit(reportLatch);
  rdb::EnvironmentError("environment {} {}", "fault", 7);
}

}  // namespace

// K1: kod EXIT_FAILURE, komunikat FATAL na stderr, zatrzask podniesiony przed atexit.
TEST(FatalErrorTest, InternalExitsWithFailureAndRaisesLatch) {
  EXPECT_EXIT(raiseInternal(), ::testing::ExitedWithCode(EXIT_FAILURE), "\nFATAL: internal fault 42\nlatch=1\n");
}

// K4: kod EIO, ten sam komunikat na stderr, zatrzask nie podniesiony.
TEST(FatalErrorTest, EnvironmentExitsWithEioAndKeepsLatchDown) {
  EXPECT_EXIT(raiseEnvironment(), ::testing::ExitedWithCode(EIO), "\nFATAL \\(environment\\): environment fault 7\nlatch=0\n");
}

// Nowa granica statusowa nie gubi kategorii ani errno przy zmianie typu wartosci.
TEST(ResultTest, PropagationPreservesErrorAndErrno) {
  const auto read      = []() -> rdb::Result<std::unique_ptr<int>> { return rdb::fail(rdb::Errc::IO, "read failed", ENOENT); };
  const auto propagate = [&]() -> rdb::Result<int> {
    RDB_TRY_ASSIGN(auto value, read());
    return *value;
  };
  const auto result = propagate();
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().code(), rdb::Errc::IO);
  EXPECT_EQ(result.error().err(), ENOENT);
  EXPECT_EQ(result.error().message(), "read failed");
}

TEST(ResultTest, TrySupportsVoidAndMoveOnlyValues) {
  const auto operation = []() -> rdb::Result<std::unique_ptr<int>> {
    RDB_TRY(rdb::Result<>{});
    RDB_TRY_ASSIGN(auto value, rdb::Result<std::unique_ptr<int>>{std::make_unique<int>(42)});
    return value;
  };
  const auto result = operation();
  ASSERT_TRUE(result);
  EXPECT_EQ(**result, 42);
}

TEST(ResultTest, DefaultFatalHandlerAborts) {
  EXPECT_EXIT(
      {
        rdb::setFatalHandler(nullptr);
        rdb::fatal("default invariant");
      },
      ::testing::KilledBySignal(SIGABRT), "FATAL: default invariant");
}

TEST(ResultTest, ReturningFatalHandlerStillAborts) {
  EXPECT_EXIT(
      {
        rdb::setFatalHandler([](std::string_view message, const std::source_location &where) noexcept {
          std::cerr << "host handler: " << message << " " << where.file_name() << "\n";
        });
        rdb::fatal("returning invariant");
      },
      ::testing::KilledBySignal(SIGABRT), "host handler: returning invariant.*FATAL: returning invariant");
}

TEST(ResultTest, DaemonHandlerKeepsFatalLatchContract) {
  EXPECT_EXIT(
      {
        std::atexit(reportLatch);
        rdb::setFatalHandler(&daemonFatalExit);
        rdb::fatal("daemon invariant");
      },
      ::testing::ExitedWithCode(EXIT_FAILURE), "FATAL: daemon invariant\nlatch=1");
}
