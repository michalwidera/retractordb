#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include "constants.hpp"
#include "retractor/lib/ipcServer.hpp"

namespace {

namespace IPC = boost::interprocess;

// Identyfikatory klientow poza zakresem realnych PID-ow, zeby test nie mogl
// trafic w kolejke zywego procesu na tej maszynie.
constexpr int kClientA = 990001;
constexpr int kClientB = 990002;

// Nazwa serwera uzywana w testach rozlacznosci obszarow. Pusta nazwa = obszar historyczny.
constexpr std::string_view kServerA = "srva";
constexpr std::string_view kServerB = "srvb";

std::string queueNameFor(int clientId, std::string_view serverName = {}) {
  return ipc::names(serverName).responseQueue(clientId);
}

bool queueExists(int clientId, std::string_view serverName = {}) {
  try {
    IPC::message_queue mq(IPC::open_only, queueNameFor(clientId, serverName).c_str());
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
    for (const std::string_view server : {std::string_view{}, kServerA, kServerB}) {
      IPC::message_queue::remove(queueNameFor(kClientA, server).c_str());
      IPC::message_queue::remove(queueNameFor(kClientB, server).c_str());
      IPC::named_mutex::remove(ipc::names(server).mapMutex.c_str());
    }
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
  const std::string mutexName = ipc::names().mapMutex;
  IPC::named_mutex created(IPC::open_or_create, mutexName.c_str());
  ASSERT_TRUE(namedMutexExists(mutexName));

  IpcServer server;
  server.shutdownFromExitHandler();

  EXPECT_FALSE(namedMutexExists(mutexName));
}

// Pusta nazwa serwera musi dawac DOKLADNIE nazwy historyczne. To jest kontrakt
// parameteryzacji: sama w sobie nie moze zmienic ani jednej nazwy w /dev/shm.
TEST_F(IpcServerQueues, empty_server_name_yields_historical_names) {
  const ipc::ServerNames n = ipc::names();
  EXPECT_EQ(n.shmemSegment, std::string(ipc::kShmemSegment));
  EXPECT_EQ(n.mapMutex, std::string(ipc::kMapMutex));
  EXPECT_EQ(n.queryQueue, std::string(ipc::kQueryQueue));
  EXPECT_EQ(n.responseQueue(kClientA), std::string(ipc::kResponseQueuePrefix) + std::to_string(kClientA));
}

// Rozdzielenie obszarow: dwa serwery o roznych nazwach nie moga sie widziec ani
// nawzajem kasowac. To jest cel calego etapu -- bez tego wiele serwerow na jednej
// maszynie nie ma prawa dzialac.
TEST_F(IpcServerQueues, servers_with_distinct_names_have_disjoint_queues) {
  IpcServer serverA;
  serverA.setServerName(kServerA);
  IpcServer serverB;
  serverB.setServerName(kServerB);

  serverA.subscribe(kClientA, "strumien", 16);
  serverB.subscribe(kClientA, "strumien", 16);

  // Ten sam identyfikator klienta, dwie rozne kolejki.
  ASSERT_NE(queueNameFor(kClientA, kServerA), queueNameFor(kClientA, kServerB));
  ASSERT_TRUE(queueExists(kClientA, kServerA));
  ASSERT_TRUE(queueExists(kClientA, kServerB));

  // Sprzatanie serwera A nie moze ruszyc obszaru serwera B.
  serverA.removeAllObjects();

  EXPECT_FALSE(queueExists(kClientA, kServerA));
  EXPECT_TRUE(queueExists(kClientA, kServerB)) << "serwer A skasowal kolejke serwera B";

  serverB.removeAllObjects();
  EXPECT_FALSE(queueExists(kClientA, kServerB));
}

// Ta sama rozlacznosc dla muteksu nazwanego: sciezka atexit serwera A nie moze
// zdjac muteksu serwera B (przed rozdzieleniem obie strony uzywaly jednej nazwy).
TEST_F(IpcServerQueues, exit_handler_does_not_touch_other_servers_mutex) {
  const std::string mutexB = ipc::names(kServerB).mapMutex;
  IPC::named_mutex createdB(IPC::open_or_create, mutexB.c_str());
  ASSERT_TRUE(namedMutexExists(mutexB));

  IpcServer serverA;
  serverA.setServerName(kServerA);
  serverA.shutdownFromExitHandler();

  EXPECT_TRUE(namedMutexExists(mutexB)) << "sciezka atexit serwera A skasowala muteks serwera B";
  IPC::named_mutex::remove(mutexB.c_str());
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
