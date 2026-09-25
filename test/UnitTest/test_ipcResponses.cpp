#include <gtest/gtest.h>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "constants.hpp"
#include "ipcResponses.hpp"
#include "osPlatform.hpp"

namespace {

namespace IPC  = boost::interprocess;
namespace resp = ipc::responses;

resp::Owner self(std::uint64_t seq) {
  const auto pid = static_cast<std::int32_t>(getpid());
  return {.pid = pid, .startTime = osplat::inspectProcess(pid).startTime, .seq = seq};
}

resp::SlotState stateOf(const resp::Slot &slot) { return static_cast<resp::SlotState>(slot.state); }

/// Segment na stercie zamiast w pamieci dzielonej: protokol slotow nie zalezy od tego, gdzie
/// lezy pamiec, a test nie zostawia wtedy niczego w /dev/shm.
std::unique_ptr<resp::Segment> freshSegment() {
  auto retVal = std::make_unique<resp::Segment>();  // wartosciowa inicjalizacja = zera = FREE
  resp::initialize(*retVal);
  return retVal;
}

/// Proces zabity SIGKILL-em i zebrany przez waitpid - wlasciciel, ktory na pewno nie zyje.
/// startTime odczytany za zycia, tak jak odczytuje go serwer w chwili zapisu.
resp::Owner deadOwner() {
  const pid_t child = fork();
  if (child == 0) {
    for (;;)
      pause();
  }
  const resp::Owner retVal{.pid = child, .startTime = osplat::inspectProcess(child).startTime, .seq = 1};
  kill(child, SIGKILL);
  waitpid(child, nullptr, 0);
  return retVal;
}

void fillWith(resp::Segment &segment, const resp::Owner &owner) {
  for (std::size_t i = 0; i < ipc::kResponseSlotCount; ++i) {
    resp::Owner numbered = owner;
    numbered.seq         = owner.seq + i;
    ASSERT_EQ(resp::publish(segment, numbered, "x"), resp::PublishStatus::Published);
  }
}

}  // namespace

TEST(IpcResponses, publish_then_take_frees_the_slot) {
  auto segment = freshSegment();
  ASSERT_EQ(resp::publish(*segment, self(1), "odpowiedz"), resp::PublishStatus::Published);
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Ready);

  EXPECT_EQ(resp::tryTake(*segment, self(1)), "odpowiedz");
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Free);
  EXPECT_EQ(resp::tryTake(*segment, self(1)), std::nullopt) << "odpowiedz odebrana dwa razy";
}

TEST(IpcResponses, foreign_pid_slot_is_left_alone) {
  auto segment = freshSegment();
  const resp::Owner other{.pid = 990001, .startTime = 7, .seq = 1};
  ASSERT_EQ(resp::publish(*segment, other, "cudza"), resp::PublishStatus::Published);

  EXPECT_EQ(resp::tryTake(*segment, self(1)), std::nullopt);
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Ready) << "klient ruszyl cudzy slot";
}

// Wlasne zadanie, na ktore klient przestal czekac: jego seq jest starsze niz biezace.
TEST(IpcResponses, own_pid_with_foreign_seq_is_released) {
  auto segment = freshSegment();
  ASSERT_EQ(resp::publish(*segment, self(1), "zalegla"), resp::PublishStatus::Published);
  ASSERT_EQ(resp::publish(*segment, self(2), "biezaca"), resp::PublishStatus::Published);

  EXPECT_EQ(resp::tryTake(*segment, self(2)), "biezaca");
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Free) << "zalegla odpowiedz zostala w slocie";
  EXPECT_EQ(stateOf(segment->slots[1]), resp::SlotState::Free);
}

// Poprzedni proces o tym samym PID-ie liczyl seq od tego samego 1. Rozroznia go tylko startTime.
TEST(IpcResponses, own_pid_with_other_start_time_is_released_not_taken) {
  auto segment            = freshSegment();
  resp::Owner predecessor = self(1);
  predecessor.startTime += 1;
  ASSERT_EQ(resp::publish(*segment, predecessor, "poprzednika"), resp::PublishStatus::Published);

  EXPECT_EQ(resp::tryTake(*segment, self(1)), std::nullopt) << "przyjeta odpowiedz poprzedniego wcielenia PID-u";
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Free);
}

// Slot w trakcie zapisu jest niewidoczny dla klienta, nawet z jego pid i seq.
TEST(IpcResponses, writing_slot_is_not_taken) {
  auto segment = freshSegment();
  ASSERT_EQ(resp::publish(*segment, self(1), "x"), resp::PublishStatus::Published);
  segment->slots[0].state = static_cast<std::uint32_t>(resp::SlotState::Writing);

  EXPECT_EQ(resp::tryTake(*segment, self(1)), std::nullopt);
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Writing);
}

TEST(IpcResponses, taken_slot_is_reused) {
  auto segment = freshSegment();
  fillWith(*segment, self(1));
  ASSERT_EQ(resp::tryTake(*segment, self(5)), "x");
  EXPECT_EQ(resp::publish(*segment, self(100), "nowa"), resp::PublishStatus::Published);
  EXPECT_EQ(resp::tryTake(*segment, self(100)), "nowa");
}

// Odpowiedz, po ktora martwy klient juz nie przyjdzie.
TEST(IpcResponses, ready_slot_of_dead_client_is_reclaimed_when_full) {
  auto segment = freshSegment();
  fillWith(*segment, deadOwner());

  ASSERT_EQ(resp::publish(*segment, self(1), "po odzysku"), resp::PublishStatus::Reclaimed);
  EXPECT_EQ(resp::tryTake(*segment, self(1)), "po odzysku");
}

// Odbior przerwany smiercia klienta - stan, ktory zostawia kill -9 w trakcie kopiowania.
TEST(IpcResponses, reading_slot_of_dead_client_is_reclaimed_when_full) {
  auto segment = freshSegment();
  fillWith(*segment, deadOwner());
  for (auto &slot : segment->slots)
    slot.state = static_cast<std::uint32_t>(resp::SlotState::Reading);

  ASSERT_EQ(resp::publish(*segment, self(1), "po odzysku"), resp::PublishStatus::Reclaimed);
  EXPECT_EQ(resp::tryTake(*segment, self(1)), "po odzysku");
}

// Odzysk dopiero przy braku wolnego: zalegla odpowiedz martwego zostaje, dopoki jest miejsce.
TEST(IpcResponses, free_slot_is_preferred_over_reclaim) {
  auto segment = freshSegment();
  ASSERT_EQ(resp::publish(*segment, deadOwner(), "martwego"), resp::PublishStatus::Published);
  EXPECT_EQ(resp::publish(*segment, self(1), "nowa"), resp::PublishStatus::Published);
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Ready);
}

// Zywego wlasciciela nie wolno wywlaszczyc - jego odbior moze wlasnie trwac.
TEST(IpcResponses, live_owners_are_never_reclaimed) {
  auto segment = freshSegment();
  fillWith(*segment, self(1));

  EXPECT_EQ(resp::publish(*segment, self(100), "nadmiarowa"), resp::PublishStatus::Full);
  for (std::size_t i = 0; i < ipc::kResponseSlotCount; ++i) {
    EXPECT_EQ(stateOf(segment->slots[i]), resp::SlotState::Ready);
    EXPECT_EQ(segment->slots[i].seq, 1 + i) << "slot " << i << " zywego klienta nadpisany";
  }
}

TEST(IpcResponses, response_larger_than_slot_is_replaced_with_error) {
  auto segment = freshSegment();
  const std::string huge(ipc::kResponseSlotDataSize + 1, 'a');
  ASSERT_EQ(resp::publish(*segment, self(1), huge), resp::PublishStatus::Published);

  const auto text = resp::tryTake(*segment, self(1));
  ASSERT_TRUE(text.has_value());
  std::stringstream stream(*text);
  boost::property_tree::ptree pt;
  read_info(stream, pt);
  EXPECT_EQ(pt.get<std::string>("error.response", ""), "response too large (32769 B > 32768 B)");
}

TEST(IpcResponses, response_of_exactly_slot_size_fits) {
  auto segment = freshSegment();
  const std::string exact(ipc::kResponseSlotDataSize, 'b');
  ASSERT_EQ(resp::publish(*segment, self(1), exact), resp::PublishStatus::Published);
  EXPECT_EQ(resp::tryTake(*segment, self(1)), exact);
}

TEST(IpcResponses, take_gives_up_at_deadline) {
  auto segment       = freshSegment();
  const auto start   = std::chrono::steady_clock::now();
  const auto result  = resp::take(*segment, self(1), start + std::chrono::milliseconds(50), std::chrono::milliseconds(5));
  const auto elapsed = std::chrono::steady_clock::now() - start;
  EXPECT_EQ(result, std::nullopt);
  EXPECT_GE(elapsed, std::chrono::milliseconds(50));
  EXPECT_LT(elapsed, std::chrono::seconds(2));
}

TEST(IpcResponses, header_identifies_layout) {
  const auto zeroed = std::make_unique<resp::Segment>();  // pol megabajta - nie na stosie
  EXPECT_FALSE(resp::compatible(*zeroed)) << "segment bez naglowka uznany za zgodny";
  resp::initialize(*zeroed);
  EXPECT_TRUE(resp::compatible(*zeroed));
  zeroed->header.layoutVersion += 1;
  EXPECT_FALSE(resp::compatible(*zeroed));
}

// Odwzorowanie w pamieci dzielonej: serwer tworzy, klient otwiera i widzi to samo.
TEST(IpcResponses, mapping_create_and_open_share_the_segment) {
  const std::string name = "ut_ipcResponses_" + std::to_string(getpid());
  IPC::shared_memory_object::remove(name.c_str());
  {
    const resp::Mapping server(IPC::create_only, name);
    const resp::Mapping client(IPC::open_only, name);
    ASSERT_TRUE(client.valid());
    ASSERT_EQ(resp::publish(server.segment(), self(1), "przez pamiec dzielona"), resp::PublishStatus::Published);
    EXPECT_EQ(resp::tryTake(client.segment(), self(1)), "przez pamiec dzielona");
  }
  EXPECT_TRUE(IPC::shared_memory_object::remove(name.c_str()));
}
