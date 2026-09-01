#include <gtest/gtest.h>

#include <string>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/lexical_cast.hpp>

#include "constants.hpp"
#include "retractor/lib/ipcServer.hpp"

namespace {

namespace IPC = boost::interprocess;

// Identyfikatory klientow poza zakresem realnych PID-ow, zeby test nie mogl
// trafic w kolejke zywego procesu na tej maszynie.
constexpr int kClientA = 990001;
constexpr int kClientB = 990002;

std::string queueNameFor(int clientId) {
  return std::string(ipc::kResponseQueuePrefix) + boost::lexical_cast<std::string>(clientId);
}

bool queueExists(int clientId) {
  try {
    IPC::message_queue mq(IPC::open_only, queueNameFor(clientId).c_str());
    return true;
  } catch (const IPC::interprocess_exception &) {
    return false;
  }
}

bool namedMutexExists(const std::string &name) {
  try {
    IPC::named_mutex m(IPC::open_only, name.c_str());
    return true;
  } catch (const IPC::interprocess_exception &) {
    return false;
  }
}

// Kazdy test zaczyna i konczy sie bez sladu po sobie w /dev/shm, niezaleznie
// od tego, czy przeszedl.
class IpcServerQueues : public ::testing::Test {
 protected:
  void SetUp() override { wipe(); }
  void TearDown() override { wipe(); }

  static void wipe() {
    IPC::message_queue::remove(queueNameFor(kClientA).c_str());
    IPC::message_queue::remove(queueNameFor(kClientB).c_str());
    IPC::named_mutex::remove(std::string(ipc::kMapMutex).c_str());
  }
};

}  // namespace

TEST_F(IpcServerQueues, subscribe_creates_response_queue) {
  IpcServer server;
  server.subscribe(kClientA, "strumien", 16);
  EXPECT_TRUE(queueExists(kClientA));
}

TEST_F(IpcServerQueues, removeAllObjects_removes_every_client_queue) {
  IpcServer server;
  server.subscribe(kClientA, "strumien", 16);
  server.subscribe(kClientB, "strumien", 16);
  ASSERT_TRUE(queueExists(kClientA));
  ASSERT_TRUE(queueExists(kClientB));

  server.removeAllObjects();

  EXPECT_FALSE(queueExists(kClientA));
  EXPECT_FALSE(queueExists(kClientB));
}

// Regresja: sciezka atexit/FatalError kasowala wylacznie segment i kolejke komend,
// wiec po smierci serwera zostawaly kolejki `brcdbr<pid>`. Nie jest to sam smiec:
// subscribe() otwiera je przez open_or_create, ktory przy istniejacej kolejce
// IGNORUJE zadana pojemnosc — klient z powtorzonym PID-em dostawal glebokosc
// z poprzedniego przebiegu.
TEST_F(IpcServerQueues, exit_handler_removes_client_queues_too) {
  IpcServer server;
  server.subscribe(kClientA, "strumien", 16);
  server.subscribe(kClientB, "strumien", 16);
  ASSERT_TRUE(queueExists(kClientA));
  ASSERT_TRUE(queueExists(kClientB));

  server.shutdownFromExitHandler();

  EXPECT_FALSE(queueExists(kClientA)) << "kolejka klienta przetrwala sciezke atexit";
  EXPECT_FALSE(queueExists(kClientB)) << "kolejka klienta przetrwala sciezke atexit";
}

// Obie drogi wyjscia kasuja ten sam zestaw obiektow globalnych. Muteks nazwany
// leczyl sie sam dopiero na starcie nastepnej instancji.
TEST_F(IpcServerQueues, exit_handler_removes_named_mutex) {
  IPC::named_mutex created(IPC::open_or_create, std::string(ipc::kMapMutex).c_str());
  ASSERT_TRUE(namedMutexExists(std::string(ipc::kMapMutex)));

  IpcServer server;
  server.shutdownFromExitHandler();

  EXPECT_FALSE(namedMutexExists(std::string(ipc::kMapMutex)));
}

// Po skasowaniu kolejek rejestr subskrypcji nie moze zostac z wpisami wskazujacymi
// na nieistniejace kolejki: drugie wywolanie ma nie miec czego kasowac i nie moze
// rzucic ani zawiesic.
TEST_F(IpcServerQueues, removal_is_idempotent) {
  IpcServer server;
  server.subscribe(kClientA, "strumien", 16);

  server.removeAllObjects();
  EXPECT_NO_THROW(server.removeAllObjects());
  EXPECT_NO_THROW(server.shutdownFromExitHandler());
  EXPECT_FALSE(queueExists(kClientA));
}
