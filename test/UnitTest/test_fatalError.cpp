#include <gtest/gtest.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>

#include "fatalError.hpp"
#include "rdb/error.hpp"

// ctest -R '^ut_fatalError' -V

// Wywolania w silniku pisza FatalError bez przestrzeni nazw; nazwa globalna ma prowadzic
// do rdb::FatalError, a CTAD ma przejsc przez using-declaration.
static_assert(std::is_same_v<decltype(FatalError("x {}", 1)), rdb::FatalError<int>>);

// Tryb rzucaj nie moze byc polkniety przez `catch (const std::exception &)` w rdzeniu.
static_assert(!std::is_base_of_v<std::exception, rdb::EngineFatal>);

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

// Zgloszenie w zakresie trybu rzucaj; zwraca zlapany EngineFatal.
template <typename Raise>
rdb::EngineFatal catchInScope(Raise raise) {
  rdb::ThrowOnFatalScope scope;
  try {
    raise();
  } catch (const rdb::EngineFatal &fault) {
    return fault;
  }
  return {};
}

// Destruktor zapisuje decyzje trybu rzucaj w chwili, w ktorej sam sie wykonuje.
struct RecordActiveInDestructor {
  bool &active;
  ~RecordActiveInDestructor() { active = rdb::detail::throwOnFatalActive(); }
};

// Zgloszenie po zamknieciu zakresu.
[[noreturn]] void raiseAfterScope() {
  { rdb::ThrowOnFatalScope scope; }
  FatalError("fatal after scope");
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

// Tryb rzucaj (#476): K1 jako EngineFatal z klasa i komunikatem, bez zatrzasku uslugi.
TEST(FatalErrorTest, InternalThrowsInScopeAndKeepsLatchDown) {
  const auto fault = catchInScope([] { FatalError("internal {} {}", "fault", 42); });
  EXPECT_EQ(fault.cls, rdb::ErrorClass::Internal);
  EXPECT_EQ(fault.message, "internal fault 42");
  EXPECT_EQ(std::string(fault.where.file_name()), __FILE__);
  EXPECT_FALSE(fatalErrorRaised.load(std::memory_order_acquire));
}

// Tryb rzucaj: K4 jako EngineFatal klasy Environment.
TEST(FatalErrorTest, EnvironmentThrowsInScope) {
  const auto fault = catchInScope([] { rdb::EnvironmentError("environment {} {}", "fault", 7); });
  EXPECT_EQ(fault.cls, rdb::ErrorClass::Environment);
  EXPECT_EQ(fault.message, "environment fault 7");
  EXPECT_FALSE(fatalErrorRaised.load(std::memory_order_acquire));
}

// Tryb rzucaj przechodzi przez handler std::exception, jak ten w foldConstants.
TEST(FatalErrorTest, EngineFatalPassesStdExceptionHandler) {
  const auto fault = catchInScope([] {
    try {
      FatalError("through");
    } catch (const std::exception &) {
      ADD_FAILURE() << "EngineFatal polkniety przez catch (const std::exception &)";
    }
  });
  EXPECT_EQ(fault.message, "through");
}

// W trakcie zwijania stosu zakonczenie procesu, nie drugi wyjatek (std::terminate). Bez testu
// smierci - std::exit z lecacym wyjatkiem zostawia pod Valgrindem jego obiekt jako "possibly lost".
TEST(FatalErrorTest, NoThrowDuringUnwindingEvenInScope) {
  rdb::ThrowOnFatalScope scope;
  bool activeDuringUnwinding = true;
  try {
    RecordActiveInDestructor guard{activeDuringUnwinding};
    throw std::runtime_error("first");
  } catch (const std::runtime_error &) {}
  EXPECT_FALSE(activeDuringUnwinding);
  EXPECT_TRUE(rdb::detail::throwOnFatalActive());
}

// Zakres nie przechodzi na inne watki: w watku pobocznym tryb rzucaj jest wylaczony, wiec jego FatalError
// idzie dotychczasowa droga. Bez testu smierci - std::exit z watku pobocznego zostawia pod
// Valgrindem TLS tego watku jako "possibly lost".
TEST(FatalErrorTest, ScopeDoesNotReachOtherThreads) {
  rdb::ThrowOnFatalScope scope;
  bool activeInWorker = true;
  std::thread([&activeInWorker] { activeInWorker = rdb::detail::throwOnFatalActive(); }).join();
  EXPECT_FALSE(activeInWorker);
  EXPECT_TRUE(rdb::detail::throwOnFatalActive());
}

// Po zamknieciu zakresu dotychczasowa droga.
TEST(FatalErrorTest, AfterScopeExitsAgain) {
  EXPECT_EXIT(raiseAfterScope(), ::testing::ExitedWithCode(EXIT_FAILURE), "\nFATAL: fatal after scope\n");
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
