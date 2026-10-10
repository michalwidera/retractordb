#include <gtest/gtest.h>

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <type_traits>

#include "fatalError.hpp"

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
  EXPECT_EXIT(raiseEnvironment(), ::testing::ExitedWithCode(EIO), "\nFATAL: environment fault 7\nlatch=0\n");
}
