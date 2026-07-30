#include <gtest/gtest.h>

#include <unistd.h>

#include <chrono>
#include <string>
#include <thread>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/property_tree/ptree.hpp>

#include "constants.hpp"
#include "qry/ipc_transport.hpp"

// ---- Lifecycle ----

TEST(IpcTransport, done_starts_false) {
  IpcTransport t;
  EXPECT_FALSE(t.done);
}

TEST(IpcTransport, done_can_be_set_and_read) {
  IpcTransport t;
  t.done = true;
  EXPECT_TRUE(t.done);
}

TEST(IpcTransport, done_is_independently_per_instance) {
  IpcTransport a;
  IpcTransport b;
  a.done = true;
  EXPECT_TRUE(a.done);
  EXPECT_FALSE(b.done);
}

// ---- popQueue ----

TEST(IpcTransport, popQueue_returns_false_on_empty) {
  IpcTransport t;
  boost::property_tree::ptree pt;
  EXPECT_FALSE(t.popQueue(pt));
}

TEST(IpcTransport, popQueue_does_not_modify_pt_on_empty) {
  IpcTransport t;
  boost::property_tree::ptree pt;
  pt.put("key", "original");
  t.popQueue(pt);
  EXPECT_EQ(pt.get<std::string>("key"), "original");
}

TEST(IpcTransport, multiple_instances_have_independent_queues) {
  IpcTransport a;
  IpcTransport b;
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
TEST(IpcTransport, producer_waits_for_late_response_queue) {
  QueueEraser eraser;
  boost::interprocess::message_queue::remove(responseQueueName().c_str());

  IpcTransport transport(kIpcTransportDefaultClientResponseMaxFails, 200);

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
TEST(IpcTransport, producer_reports_missing_queue_distinctly_from_normal_end) {
  QueueEraser eraser;
  boost::interprocess::message_queue::remove(responseQueueName().c_str());

  IpcTransport transport(kIpcTransportDefaultClientResponseMaxFails, 3);
  transport.producer();

  EXPECT_TRUE(transport.responseQueueMissing) << "brak kolejki musi byc odrozniony od normalnego konca";
  EXPECT_TRUE(transport.done);

  boost::property_tree::ptree pt;
  EXPECT_FALSE(transport.popQueue(pt)) << "nie moze byc danych, skoro kolejki nie bylo";
}
