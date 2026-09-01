#include <gtest/gtest.h>

#include <unistd.h>

#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <utility>

#include <boost/container/map.hpp>
#include <boost/container/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/property_tree/ptree.hpp>

#include "constants.hpp"
#include "qry/ipcClient.hpp"

// ---- Lifecycle ----

TEST(IpcClient, done_starts_false) {
  IpcClient t;
  EXPECT_FALSE(t.done);
}

TEST(IpcClient, done_can_be_set_and_read) {
  IpcClient t;
  t.done = true;
  EXPECT_TRUE(t.done);
}

TEST(IpcClient, done_is_independently_per_instance) {
  IpcClient a;
  IpcClient b;
  a.done = true;
  EXPECT_TRUE(a.done);
  EXPECT_FALSE(b.done);
}

// ---- popQueue ----

TEST(IpcClient, popQueue_returns_false_on_empty) {
  IpcClient t;
  boost::property_tree::ptree pt;
  EXPECT_FALSE(t.popQueue(pt));
}

TEST(IpcClient, popQueue_does_not_modify_pt_on_empty) {
  IpcClient t;
  boost::property_tree::ptree pt;
  pt.put("key", "original");
  t.popQueue(pt);
  EXPECT_EQ(pt.get<std::string>("key"), "original");
}

TEST(IpcClient, multiple_instances_have_independent_queues) {
  IpcClient a;
  IpcClient b;
  boost::property_tree::ptree pt;
  EXPECT_FALSE(a.popQueue(pt));
  EXPECT_FALSE(b.popQueue(pt));
}

// === Regresja defektu klienta wykrytego w kampanii K6b (issue_215) ===
//
// Kolejkę odpowiedzi tworzy SERWER w reakcji na rejestrację klienta, więc
// `open_only` wołane natychmiast po starcie wątku producenta bywa o krok za
// wcześnie. Stara wersja poddawała się po PIERWSZEJ nieudanej próbie i ustawiała
// `done`; pętla `select()` nie wykonywała wtedy ani jednego obrotu, a klient
// kończył się kodem 0, nie przeczytawszy nic. Cichy sukces bez danych.

namespace {
std::string responseQueueName() { return std::string(ipc::kResponseQueuePrefix) + std::to_string(::getpid()); }

struct QueueEraser {
  ~QueueEraser() { boost::interprocess::message_queue::remove(responseQueueName().c_str()); }
};
}  // namespace

// Producent musi PRZECZEKAĆ opóźnione utworzenie kolejki, a nie poddać się od razu.
TEST(IpcClient, producer_waits_for_late_response_queue) {
  QueueEraser eraser;
  boost::interprocess::message_queue::remove(responseQueueName().c_str());

  IpcClient transport(kIpcClientDefaultResponseMaxFails, 200);

  std::thread creator([] {
    // Kolejka pojawia się dopiero po chwili — dokładnie ten wyścig, który
    // wywracał klienta przy starcie obciążonego serwera.
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    boost::interprocess::message_queue(boost::interprocess::create_only, responseQueueName().c_str(), 8,
                                       ipc::kResponseQueueMaxMessageSize);
  });

  std::thread producer([&transport] { transport.producer(); });
  std::this_thread::sleep_for(std::chrono::milliseconds(400));

  EXPECT_FALSE(transport.responseQueueMissing) << "producent poddal sie mimo ze kolejka powstala";
  EXPECT_FALSE(transport.done) << "producent zakonczyl prace mimo dostepnej kolejki";

  transport.done = true;
  creator.join();
  producer.join();
}

// Gdy kolejka nie powstanie nigdy, producent musi to ZAMELDOWAĆ osobną flagą.
// Samo `done` jest nieodróżnialne od normalnego końca strumienia — i to była
// przyczyna, dla której klient kończył się zerem bez danych.
TEST(IpcClient, producer_reports_missing_queue_distinctly_from_normal_end) {
  QueueEraser eraser;
  boost::interprocess::message_queue::remove(responseQueueName().c_str());

  IpcClient transport(kIpcClientDefaultResponseMaxFails, 3);
  transport.producer();

  EXPECT_TRUE(transport.responseQueueMissing) << "brak kolejki musi byc odrozniony od normalnego konca";
  EXPECT_TRUE(transport.done);

  boost::property_tree::ptree pt;
  EXPECT_FALSE(transport.popQueue(pt)) << "nie moze byc danych, skoro kolejki nie bylo";
}

// === Regresja defektu klienta wykrytego w kampanii K6c (issue_217) ===
//
// Klient „znikał" przy dołączaniu do obciążonego serwera. Nie był to crash:
// kończył się czysto kodem `timed_out`, bo budżet oczekiwania na odpowiedź
// serwera na `get` wynosił 10 prób × 10 ms = 100 ms. Wątek komunikacyjny
// silnika (`commandProcessorLoop`) powstaje PRZED `rtActivate`, więc zostaje
// SCHED_OTHER, podczas gdy wątek przetwarzania dostaje SCHED_FIFO 50; `taskset`
// przypina oba do jednego rdzenia. Przy wysyceniu rdzenia wątek komunikacyjny
// dostaje CPU dopiero w oknie throttlingu RT, a to trwa dłużej niż 100 ms.
//
// Zmierzone w kampanii: `W8_Q32` pracuje z duty ~200 % slotu, więc wysycenie
// jest trwałe, a nie chwilowe.

namespace {

// Atrapa serwera: tworzy dokładnie te obiekty IPC, których szuka `netClient`,
// ale NIGDY nie wstawia odpowiedzi do mapy. Odwzorowuje serwer, którego wątek
// komunikacyjny jest zagłodzony przez wątek czasu rzeczywistego.
class SilentServer {
  using segment_manager_t = boost::interprocess::managed_shared_memory::segment_manager;
  using CharAllocator     = boost::interprocess::allocator<char, segment_manager_t>;
  using IPCString         = boost::container::basic_string<char, std::char_traits<char>, CharAllocator>;
  using ValueType         = std::pair<const int, IPCString>;
  using ShmemAllocator    = boost::interprocess::allocator<ValueType, segment_manager_t>;
  using IPCMap            = boost::container::map<int, IPCString, std::less<>, ShmemAllocator>;

  boost::interprocess::managed_shared_memory segment_;
  boost::interprocess::named_mutex mutex_;
  boost::interprocess::message_queue queue_;

 public:
  SilentServer()
      : segment_(boost::interprocess::open_or_create, std::string(ipc::kShmemSegment).c_str(), ipc::kShmemSegmentSize),
        mutex_(boost::interprocess::open_or_create, std::string(ipc::kMapMutex).c_str()),
        queue_(boost::interprocess::open_or_create, std::string(ipc::kQueryQueue).c_str(), ipc::kQueryQueueMaxMessages,
               ipc::kQueryQueueMaxMessageSize) {
    segment_.construct<IPCMap>(std::string(ipc::kMapObject).c_str())(std::less<>(),
                                                                     ShmemAllocator(segment_.get_segment_manager()));
  }

  ~SilentServer() {
    boost::interprocess::shared_memory_object::remove(std::string(ipc::kShmemSegment).c_str());
    boost::interprocess::named_mutex::remove(std::string(ipc::kMapMutex).c_str());
    boost::interprocess::message_queue::remove(std::string(ipc::kQueryQueue).c_str());
  }
};

}  // namespace

// Domyślny budżet klienta musi przetrwać serwer, który przez chwilę nie dostaje
// CPU. 100 ms nie wystarczało: okno throttlingu RT ma okres rzędu sekundy, więc
// klient poddawał się, zanim wątek komunikacyjny w ogóle został zaszeregowany.
TEST(IpcClient, netClient_default_budget_survives_briefly_starved_server) {
  const SilentServer server;

  IpcClient transport;  // domyślny budżet — to on był defektem
  const auto start = std::chrono::steady_clock::now();
  const auto pt    = transport.netClient("get", "");
  const auto spent = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

  EXPECT_EQ(pt.get<std::string>("error.response", ""), "server not found") << "milczacy serwer musi dac rozpoznawalny blad";
  EXPECT_GE(spent.count(), 2000) << "domyslny budzet " << spent.count() << " ms nie przetrwa zaglodzonego watku komunikacyjnego";
}

// Budżet ma być liczony ZEGAREM, a nie liczbą obrotów pętli. Przy liczeniu
// obrotów przeciążony klient (albo dłuższy `lock`) skracał faktyczne czekanie
// poniżej zadeklarowanego, czyli dokładnie wtedy, gdy potrzebne było najdłuższe.
TEST(IpcClient, netClient_honours_configured_budget_by_wall_clock) {
  const SilentServer server;

  IpcClient transport(120, kIpcClientDefaultResponseQueueOpenMaxFails);  // 120 × 10 ms = 1200 ms
  const auto start = std::chrono::steady_clock::now();
  transport.netClient("get", "");
  const auto spent = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

  EXPECT_GE(spent.count(), 1000) << "czekano " << spent.count() << " ms zamiast zadeklarowanych 1200 ms";
  EXPECT_LE(spent.count(), 4000) << "czekano " << spent.count() << " ms, budzet ma byc ograniczony z gory";
}
