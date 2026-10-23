#include <gtest/gtest.h>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <memory>
#include <sstream>
#include <string>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "constants.hpp"
#include "ipcResponses.hpp"
#include "ipcSecurity.hpp"
#include "logCapture.hpp"
#include "osPlatform.hpp"
#include "syscallWrap.hpp"

namespace {

namespace IPC  = boost::interprocess;
namespace resp = ipc::responses;

// Istniejacy punkt diagnostyczny po READY -> READING pozwala wymusic utrate
// slotu przed koncowym CAS, bez dodawania haka do implementacji.
resp::Slot *lostReadSlot = nullptr;

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

extern "C" char *__real_getenv(const char *name);
RDB_WRAP_SYSCALL(char *, getenv, (const char *name), (name));
extern "C" char *__wrap_getenv(const char *name) {
  if (lostReadSlot != nullptr && std::string_view(name) == "RDB_FAULT_CLIENT_READ_DELAY") {
    lostReadSlot->state = static_cast<std::uint32_t>(resp::SlotState::Writing);
    lostReadSlot        = nullptr;
    return nullptr;
  }
  return __real_getenv(name);
}

TEST(IpcResponses, publish_then_take_frees_the_slot) {
  // covers: ipcresponses.publish.2~1, ipcresponses.try-take.1~1
  auto segment = freshSegment();
  ASSERT_EQ(resp::publish(*segment, self(1), "odpowiedz"), resp::PublishStatus::Published);
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Ready);
  EXPECT_EQ(segment->slots[0].pid, self(1).pid);
  EXPECT_EQ(segment->slots[0].startTime, self(1).startTime);
  EXPECT_EQ(segment->slots[0].seq, 1U);
  EXPECT_EQ(segment->slots[0].length, 9U);

  EXPECT_EQ(resp::tryTake(*segment, self(1)), "odpowiedz");
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Free);
  EXPECT_EQ(resp::tryTake(*segment, self(1)), std::nullopt) << "odpowiedz odebrana dwa razy";
}

TEST(IpcResponses, foreign_pid_slot_is_left_alone) {
  // covers: ipcresponses.try-take.1~1
  auto segment = freshSegment();
  const resp::Owner other{.pid = 990001, .startTime = 7, .seq = 1};
  ASSERT_EQ(resp::publish(*segment, other, "cudza"), resp::PublishStatus::Published);

  EXPECT_EQ(resp::tryTake(*segment, self(1)), std::nullopt);
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Ready) << "klient ruszyl cudzy slot";
}

// Wlasne zadanie, na ktore klient przestal czekac: jego seq jest starsze niz biezace.
TEST(IpcResponses, own_pid_with_foreign_seq_is_released) {
  // covers: ipcresponses.try-take.2~2
  auto segment = freshSegment();
  ASSERT_EQ(resp::publish(*segment, self(1), "zalegla"), resp::PublishStatus::Published);
  ASSERT_EQ(resp::publish(*segment, self(2), "biezaca"), resp::PublishStatus::Published);

  EXPECT_EQ(resp::tryTake(*segment, self(2)), "biezaca");
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Free) << "zalegla odpowiedz zostala w slocie";
  EXPECT_EQ(stateOf(segment->slots[1]), resp::SlotState::Free);
}

// Dwa watki jednego procesu czekaja na raz. Odbior dla seq 1 nie moze zwolnic odpowiedzi
// dla seq 2, ktora jej watek dopiero odbierze - inaczej ten watek zglasza "server not found".
TEST(IpcResponses, answer_of_a_concurrent_request_of_this_process_is_left_for_its_thread) {
  // covers: ipcresponses.try-take.2~2, ipcresponses.pending-request.1~1
  auto segment = freshSegment();
  const resp::PendingRequest first(1);
  const resp::PendingRequest second(2);
  ASSERT_EQ(resp::publish(*segment, self(2), "drugiego"), resp::PublishStatus::Published);

  EXPECT_EQ(resp::tryTake(*segment, self(1)), std::nullopt);
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Ready) << "zwolniona odpowiedz zadania w toku";
  EXPECT_EQ(resp::tryTake(*segment, self(2)), "drugiego");
}

// Po zakonczeniu czekania numer przestaje byc "w toku": jego zalegla odpowiedz jest zwalniana.
TEST(IpcResponses, answer_of_a_finished_request_is_released) {
  // covers: ipcresponses.pending-request.2~1
  auto segment = freshSegment();
  { const resp::PendingRequest abandoned(1); }
  ASSERT_EQ(resp::publish(*segment, self(1), "porzucona"), resp::PublishStatus::Published);

  EXPECT_EQ(resp::tryTake(*segment, self(2)), std::nullopt);
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Free);
}

// Poprzedni proces o tym samym PID-ie liczyl seq od tego samego 1. Rozroznia go tylko startTime.
TEST(IpcResponses, own_pid_with_other_start_time_is_released_not_taken) {
  // covers: ipcresponses.try-take.2~2
  auto segment            = freshSegment();
  resp::Owner predecessor = self(1);
  predecessor.startTime += 1;
  ASSERT_EQ(resp::publish(*segment, predecessor, "poprzednika"), resp::PublishStatus::Published);

  EXPECT_EQ(resp::tryTake(*segment, self(1)), std::nullopt) << "przyjeta odpowiedz poprzedniego wcielenia PID-u";
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Free);
}

// Slot w trakcie zapisu jest niewidoczny dla klienta, nawet z jego pid i seq.
TEST(IpcResponses, writing_slot_is_not_taken) {
  // covers: ipcresponses.try-take.1~1
  auto segment = freshSegment();
  ASSERT_EQ(resp::publish(*segment, self(1), "x"), resp::PublishStatus::Published);
  segment->slots[0].state = static_cast<std::uint32_t>(resp::SlotState::Writing);

  EXPECT_EQ(resp::tryTake(*segment, self(1)), std::nullopt);
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Writing);
}

TEST(IpcResponses, taken_slot_is_reused) {
  // covers: ipcresponses.publish.2~1
  auto segment = freshSegment();
  fillWith(*segment, self(1));
  ASSERT_EQ(resp::tryTake(*segment, self(5)), "x");
  EXPECT_EQ(resp::publish(*segment, self(100), "nowa"), resp::PublishStatus::Published);
  EXPECT_EQ(resp::tryTake(*segment, self(100)), "nowa");
}

// Odpowiedz, po ktora martwy klient juz nie przyjdzie.
TEST(IpcResponses, ready_slot_of_dead_client_is_reclaimed_when_full) {
  // covers: ipcresponses.publish.3~1
  auto segment = freshSegment();
  fillWith(*segment, deadOwner());

  ASSERT_EQ(resp::publish(*segment, self(1), "po odzysku"), resp::PublishStatus::Reclaimed);
  EXPECT_EQ(resp::tryTake(*segment, self(1)), "po odzysku");
}

// Odbior przerwany smiercia klienta - stan, ktory zostawia kill -9 w trakcie kopiowania.
TEST(IpcResponses, reading_slot_of_dead_client_is_reclaimed_when_full) {
  // covers: ipcresponses.publish.3~1
  auto segment = freshSegment();
  fillWith(*segment, deadOwner());
  for (auto &slot : segment->slots)
    slot.state = static_cast<std::uint32_t>(resp::SlotState::Reading);

  ASSERT_EQ(resp::publish(*segment, self(1), "po odzysku"), resp::PublishStatus::Reclaimed);
  EXPECT_EQ(resp::tryTake(*segment, self(1)), "po odzysku");
}

// Odzysk dopiero przy braku wolnego: zalegla odpowiedz martwego zostaje, dopoki jest miejsce.
TEST(IpcResponses, free_slot_is_preferred_over_reclaim) {
  // covers: ipcresponses.publish.2~1
  auto segment = freshSegment();
  ASSERT_EQ(resp::publish(*segment, deadOwner(), "martwego"), resp::PublishStatus::Published);
  EXPECT_EQ(resp::publish(*segment, self(1), "nowa"), resp::PublishStatus::Published);
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Ready);
}

// Zywego wlasciciela nie wolno wywlaszczyc - jego odbior moze wlasnie trwac.
TEST(IpcResponses, live_owners_are_never_reclaimed) {
  // covers: ipcresponses.publish.3~1, ipcresponses.publish.4~1
  const LogCapture log;
  auto segment = freshSegment();
  fillWith(*segment, self(1));

  EXPECT_EQ(resp::publish(*segment, self(100), "nadmiarowa"), resp::PublishStatus::Full);
  for (std::size_t i = 0; i < ipc::kResponseSlotCount; ++i) {
    EXPECT_EQ(stateOf(segment->slots[i]), resp::SlotState::Ready);
    EXPECT_EQ(segment->slots[i].seq, 1 + i) << "slot " << i << " zywego klienta nadpisany";
  }
  EXPECT_NE(log.text().find("dropped: all"), std::string::npos);
}

TEST(IpcResponses, response_larger_than_slot_is_replaced_with_error) {
  // covers: ipcresponses.publish.1~1
  const LogCapture log;
  auto segment = freshSegment();
  const std::string huge(ipc::kResponseSlotDataSize + 1, 'a');
  ASSERT_EQ(resp::publish(*segment, self(1), huge), resp::PublishStatus::Published);

  const auto text = resp::tryTake(*segment, self(1));
  ASSERT_TRUE(text.has_value());
  std::stringstream stream(*text);
  boost::property_tree::ptree pt;
  read_info(stream, pt);
  EXPECT_EQ(pt.get<std::string>("error.response", ""),
            std::format("response too large ({} B > {} B)", ipc::kResponseSlotDataSize + 1, ipc::kResponseSlotDataSize));
  EXPECT_NE(log.text().find("replaced with an error"), std::string::npos);
}

TEST(IpcResponses, live_readers_are_not_reclaimed_or_modified) {
  // covers: ipcresponses.publish.3~1
  auto segment = freshSegment();
  fillWith(*segment, self(1));
  for (auto &slot : segment->slots)
    slot.state = static_cast<std::uint32_t>(resp::SlotState::Reading);
  EXPECT_EQ(resp::publish(*segment, self(100), "replacement"), resp::PublishStatus::Full);
  for (std::size_t i = 0; i < ipc::kResponseSlotCount; ++i) {
    EXPECT_EQ(stateOf(segment->slots[i]), resp::SlotState::Reading);
    EXPECT_EQ(segment->slots[i].seq, i + 1);
    EXPECT_EQ(segment->slots[i].data[0], 'x');
  }
}

TEST(IpcResponses, failed_release_discards_the_copy_and_keeps_scanning) {
  // covers: ipcresponses.try-take.4~1
  auto segment = freshSegment();
  ASSERT_EQ(resp::publish(*segment, self(1), "untrusted"), resp::PublishStatus::Published);
  ASSERT_EQ(resp::publish(*segment, self(1), "trusted"), resp::PublishStatus::Published);
  lostReadSlot = &segment->slots[0];
  EXPECT_EQ(resp::tryTake(*segment, self(1)), "trusted");
  EXPECT_EQ(lostReadSlot, nullptr);
  lostReadSlot = nullptr;
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Writing);
  EXPECT_EQ(stateOf(segment->slots[1]), resp::SlotState::Free);
}

TEST(IpcResponses, take_returns_a_ready_answer_without_waiting_for_the_deadline) {
  // covers: ipcresponses.take.1~1
  auto segment = freshSegment();
  ASSERT_EQ(resp::publish(*segment, self(9), "ready"), resp::PublishStatus::Published);
  EXPECT_EQ(
      resp::take(*segment, self(9), std::chrono::steady_clock::now() + std::chrono::seconds(5), std::chrono::milliseconds(1)),
      "ready");
  EXPECT_EQ(stateOf(segment->slots[0]), resp::SlotState::Free);
}

TEST(IpcResponses, mapping_checks_size_and_every_header_field_without_reinitializing_slots) {
  // covers: ipcresponses.initialize.2~1, ipcresponses.mapping.1~1, ipcresponses.mapping.3~1, ipcresponses.mapping.4~1
  const std::string name = "ut_ipcResponses_layout_" + std::to_string(getpid());
  IPC::shared_memory_object::remove(name.c_str());
  {
    auto memory = ipc::security::createMemory(name);
    memory.truncate(sizeof(resp::Segment) - 1);
    EXPECT_FALSE(resp::Mapping(IPC::open_only, name).valid());
  }
  ASSERT_TRUE(IPC::shared_memory_object::remove(name.c_str()));
  {
    const auto empty = ipc::security::createMemory(name);
    EXPECT_THROW(resp::Mapping(IPC::open_only, name), IPC::interprocess_exception);
  }
  ASSERT_TRUE(IPC::shared_memory_object::remove(name.c_str()));
  {
    const resp::Mapping server(IPC::create_only, name);
    for (const auto &slot : server.segment().slots)
      EXPECT_EQ(stateOf(slot), resp::SlotState::Free);
    ASSERT_EQ(resp::publish(server.segment(), self(7), "preserved"), resp::PublishStatus::Published);
    const resp::Mapping client(IPC::open_only, name);
    const auto original = server.segment().header;
    for (const int field : {0, 1, 2, 3}) {
      server.segment().header = original;
      auto &header            = server.segment().header;
      if (field == 0) ++header.magic;
      if (field == 1) ++header.layoutVersion;
      if (field == 2) ++header.slotCount;
      if (field == 3) ++header.slotSize;
      EXPECT_FALSE(client.valid()) << field;
    }
    server.segment().header = original;
    EXPECT_TRUE(client.valid());
    EXPECT_EQ(resp::tryTake(client.segment(), self(7)), "preserved");
  }
  EXPECT_TRUE(IPC::shared_memory_object::remove(name.c_str()));
}

TEST(IpcResponses, response_of_exactly_slot_size_fits) {
  // covers: ipcresponses.publish.1~1
  auto segment = freshSegment();
  const std::string exact(ipc::kResponseSlotDataSize, 'b');
  ASSERT_EQ(resp::publish(*segment, self(1), exact), resp::PublishStatus::Published);
  EXPECT_EQ(resp::tryTake(*segment, self(1)), exact);
}

TEST(IpcResponses, take_gives_up_at_deadline) {
  // covers: ipcresponses.take.1~1
  auto segment       = freshSegment();
  const auto start   = std::chrono::steady_clock::now();
  const auto result  = resp::take(*segment, self(1), start + std::chrono::milliseconds(50), std::chrono::milliseconds(5));
  const auto elapsed = std::chrono::steady_clock::now() - start;
  EXPECT_EQ(result, std::nullopt);
  EXPECT_GE(elapsed, std::chrono::milliseconds(50));
  EXPECT_LT(elapsed, std::chrono::seconds(2));
}

TEST(IpcResponses, header_identifies_layout) {
  // covers: ipcresponses.initialize.1~1, ipcresponses.initialize.2~1, ipcresponses.compatible.1~1
  const auto zeroed = std::make_unique<resp::Segment>();  // pol megabajta - nie na stosie
  EXPECT_FALSE(resp::compatible(*zeroed)) << "segment bez naglowka uznany za zgodny";
  resp::initialize(*zeroed);
  EXPECT_TRUE(resp::compatible(*zeroed));
  for (const auto &slot : zeroed->slots)
    EXPECT_EQ(stateOf(slot), resp::SlotState::Free);
  zeroed->header.layoutVersion += 1;
  EXPECT_FALSE(resp::compatible(*zeroed));
  resp::initialize(*zeroed);
  zeroed->header.slotCount += 1;
  EXPECT_FALSE(resp::compatible(*zeroed));
  resp::initialize(*zeroed);
  zeroed->header.slotSize += 1;
  EXPECT_FALSE(resp::compatible(*zeroed));
  resp::initialize(*zeroed);
  zeroed->header.magic = 0;
  EXPECT_FALSE(resp::compatible(*zeroed));
}

TEST(IpcResponses, mapping_refuses_missing_and_existing_names_without_replacing_the_owner) {
  // covers: ipcresponses.mapping.1~1, ipcresponses.mapping.3~1, ipcresponses.mapping.6~1
  const std::string name = "ut_ipcResponses_contract_" + std::to_string(getpid());
  IPC::shared_memory_object::remove(name.c_str());
  EXPECT_THROW(resp::Mapping(IPC::open_only, name), IPC::interprocess_exception);
  {
    const resp::Mapping server(IPC::create_only, name);
    ASSERT_TRUE(server.valid());
    EXPECT_THROW(resp::Mapping(IPC::create_only, name), ipc::security::Rejected);
    ASSERT_EQ(resp::publish(server.segment(), self(1), "unchanged"), resp::PublishStatus::Published);
    const resp::Mapping client(IPC::open_only, name);
    EXPECT_EQ(resp::tryTake(client.segment(), self(1)), "unchanged");
    server.segment().header.slotSize += 1;
    EXPECT_FALSE(server.valid());
    EXPECT_FALSE(client.valid());
  }
  EXPECT_TRUE(IPC::shared_memory_object::remove(name.c_str()));
}

// Odwzorowanie w pamieci dzielonej: serwer tworzy, klient otwiera i widzi to samo.
TEST(IpcResponses, mapping_create_and_open_share_the_segment) {
  // covers: ipcresponses.mapping.2~1, ipcresponses.mapping.5~1
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
