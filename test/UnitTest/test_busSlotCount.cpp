// Tak jak test_busFallback: prywatny uklad sluzy tylko do podmiany naglowka
// przez drugie mapowanie. Nie dodajemy haka ani publicznego API do produkcji.
#include "retractor/lib/bus.cpp"

#include <sstream>

#include <spdlog/sinks/ostream_sink.h>

#include "busRepairSeqlockTests.hpp"
#include "busSlotCountTests.hpp"

#if RDB_HAS_ROBUST_MUTEX
namespace {

// Logger wraca na miejsce takze po ASSERT: pozostale testy zachowuja swoj plik logu.
struct BusLogCapture {
  std::ostringstream output;
  std::shared_ptr<spdlog::logger> previous = spdlog::default_logger();

  BusLogCapture() {
    auto sink   = std::make_shared<spdlog::sinks::ostream_sink_mt>(output);
    auto logger = std::make_shared<spdlog::logger>("bus-capture", sink);
    logger->set_pattern("%l: %v");
    spdlog::set_default_logger(logger);
  }

  ~BusLogCapture() { spdlog::set_default_logger(previous); }
};

}  // namespace

// ENOTRECOVERABLE powstaje po unlock bez consistent, nie po samej smierci naprawiajacego.
TEST_F(BusRepairSeqlock, UnrecoverableMutexReportsRecoveryInstructions) {
  bus::Bus candidate(name);
  ASSERT_EQ(candidate.claim({.name = "candidate", .streams = {"old"}}).status, bus::ClaimStatus::Claimed);
  ASSERT_EQ(candidate.reservePlan({"replacement"}, {}, {}).status, bus::ClaimStatus::Claimed);
  IPC::shared_memory_object object(IPC::open_only, name.c_str(), IPC::read_write);
  IPC::mapped_region mapping(object, IPC::read_write);
  auto *segment = static_cast<bus::Segment *>(mapping.get_address());

  dieMidWrite(segment);
  ASSERT_FALSE(HasFatalFailure());
  ASSERT_EQ(pthread_mutex_lock(&segment->mutex), EOWNERDEAD);
  ASSERT_EQ(pthread_mutex_unlock(&segment->mutex), 0);
  ASSERT_EQ(pthread_mutex_lock(&segment->mutex), ENOTRECOVERABLE);

  BusLogCapture log;
  const std::string expected =
      "bus mutex is unrecoverable (ENOTRECOVERABLE); remove /dev/shm/" + name + " once no instance maps it";
  const auto check = [&](const bus::ClaimResult &result) {
    EXPECT_EQ(result.status, bus::ClaimStatus::Unavailable);
    EXPECT_EQ(result.detail, expected);
  };
  check(candidate.claimAdditional({"extra"}, {}));
  check(candidate.reservePlan({"new"}, {}, {}));
  check(candidate.activateReservedPlan());
  bus::Bus newcomer(name);
  check(newcomer.claim({.name = "newcomer", .streams = {"fresh"}}));
  // Raz na obiekt Bus: trzy proby candidate daja jedna linie, newcomer druga.
  const std::string line = "error: xrdbbus: " + expected + ". Running WITHOUT name uniqueness.";
  const std::string text = log.output.str();
  std::size_t lines      = 0;
  for (auto at = text.find(line); at != std::string::npos; at = text.find(line, at + line.size()))
    ++lines;
  EXPECT_EQ(lines, 2U);
}

// Drugi wlasciciel tez ginie przed consistent: trzeci nadal moze naprawic muteks i slot.
TEST_F(BusRepairSeqlock, InterruptedRepairRemainsRecoverable) {
  bus::Bus candidate(name);
  ASSERT_TRUE(candidate.attached());
  IPC::shared_memory_object object(IPC::open_only, name.c_str(), IPC::read_write);
  IPC::mapped_region mapping(object, IPC::read_write);
  auto *segment = static_cast<bus::Segment *>(mapping.get_address());
  dieMidWrite(segment);
  ASSERT_FALSE(HasFatalFailure());

  const pid_t child = fork();
  ASSERT_NE(child, -1);
  if (child == 0) {
    if (pthread_mutex_lock(&segment->mutex) != EOWNERDEAD) _exit(2);
    raise(SIGSTOP);
    _exit(3);
  }
  int status = 0;
  ASSERT_EQ(waitpid(child, &status, WUNTRACED), child);
  ASSERT_TRUE(WIFSTOPPED(status));
  ASSERT_EQ(kill(child, SIGKILL), 0);
  ASSERT_EQ(waitpid(child, &status, 0), child);
  ASSERT_TRUE(WIFSIGNALED(status));
  ASSERT_EQ(WTERMSIG(status), SIGKILL);

  EXPECT_EQ(candidate.claim({.name = "candidate", .streams = {"fresh"}}).status, bus::ClaimStatus::Claimed);
  EXPECT_TRUE(contentIsZero(segment->slots[kVictim]));
  ASSERT_EQ(pthread_mutex_lock(&segment->mutex), 0);
  EXPECT_EQ(pthread_mutex_unlock(&segment->mutex), 0);
}
#endif
