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
#include "rdbResult.hpp"

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

TEST(ResultTest, ErrorNamesRefusalClassesAndHandlerExchange) {
  for (const auto &[code, name] :
       std::vector<std::pair<rdb::Errc, std::string>>{{rdb::Errc::Config, "ConfigError"},
                                                      {rdb::Errc::CorruptDescriptor, "CorruptDescriptor"},
                                                      {rdb::Errc::Syntax, "SyntaxError"},
                                                      {rdb::Errc::Compile, "CompileError"},
                                                      {rdb::Errc::Eval, "EvalError"},
                                                      {rdb::Errc::IO, "IOError"},
                                                      {rdb::Errc::Logic, "LogicError"}})
    EXPECT_EQ(rdb::errcName(code), name);
  EXPECT_EQ(rdb::errcName(static_cast<rdb::Errc>(255)), "Error");
  for (const auto cls : {rdb::ErrorClass::Environment, rdb::ErrorClass::Internal, rdb::ErrorClass::Plan,
                         rdb::ErrorClass::Request, rdb::ErrorClass::Data}) {
    const rdb::Result<> result = rdb::fail(rdb::Refusal{.cls = cls, .message = "cause", .err = EACCES});
    const auto expected        = cls == rdb::ErrorClass::Environment ? rdb::Errc::IO
                                 : cls == rdb::ErrorClass::Internal  ? rdb::Errc::Logic
                                                                     : rdb::Errc::Compile;
    ASSERT_RDB_ERROR(result, expected, "cause");
    EXPECT_EQ(result.error().err(), EACCES);
  }
  const rdb::Result<> literal = rdb::fail(rdb::Errc::Config, "literal");
  ASSERT_RDB_ERROR(literal, rdb::Errc::Config, "literal");
  EXPECT_EQ(literal.error().err(), 0);
  const auto handler  = +[](std::string_view, const std::source_location &) noexcept {};
  const auto previous = rdb::setFatalHandler(handler);
  EXPECT_EQ(rdb::setFatalHandler(nullptr), handler);
  rdb::setFatalHandler(previous);
}

TEST(FatalErrorTest, FormattingFailureStillRunsAtexit) {
  EXPECT_EXIT(
      {
        std::atexit(reportLatch);
        rdb::detail::terminateProcess<rdb::ErrorClass::Internal>(std::source_location::current(), fmt::runtime("{"), 1);
      },
      ::testing::ExitedWithCode(EXIT_FAILURE), "latch=1");
}
