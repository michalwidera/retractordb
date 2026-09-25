#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/property_tree/info_parser.hpp>

#include "constants.hpp"
#include "ipcTypes.hpp"
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
// IGNORUJE zadana pojemnosc - klient z powtorzonym PID-em dostawal glebokosc
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

namespace {

// Wlasny obszar nazw: watek komunikacyjny kasuje na wejsciu segment, kolejke komend i muteks
// SWOJEGO serwera, wiec obszar historyczny i obszary kServerA/kServerB zostaja nietkniete.
constexpr std::string_view kServerLoop = "srvloop";
constexpr int kClientFull              = 990003;
constexpr int kClientNext              = 990004;
constexpr auto kResponseBudget         = std::chrono::seconds(5);

using ptree = IpcServer::ptree;

/// Serwer z prawdziwym watkiem komunikacyjnym i zaslepkami zamiast executorsm. Odpowiedz niesie
/// dlugosc otrzymanego db.argument - po niej widac, czy komenda doszla w calosci. Licznik wywolan
/// handlera odroznia komende odrzucona od wykonanej, ktorej odpowiedz tylko przepadla.
class RunningServer {
 public:
  RunningServer() : names_(ipc::names(kServerLoop)) {
    server_.setServerName(kServerLoop);
    auto readyFuture = ready_.get_future();
    server_.start({.onCommand =
                       [this](const ptree &request) {
                         ++handled_;
                         ptree response;
                         response.put("argumentLength", request.get("db.argument", std::string{}).size());
                         return response;
                       },
                   .onReady          = [this] { ready_.set_value(true); },
                   .onFailure        = [this] { ready_.set_value(false); },
                   .onCommandHandled = [] {},
                   .shouldStop       = [this] { return stop_.load(); }});
    readyOk_ = readyFuture.wait_for(kResponseBudget) == std::future_status::ready && readyFuture.get();
  }

  ~RunningServer() {
    stop_ = true;
    server_.stop();
    server_.removeAllObjects();
  }

  RunningServer(const RunningServer &)            = delete;
  RunningServer &operator=(const RunningServer &) = delete;

  [[nodiscard]] bool ready() const { return readyOk_; }
  [[nodiscard]] int handled() const { return handled_.load(); }

  /// Surowe bajty wprost do kolejki komend - z pominieciem IpcClient, ktory takiej komendy nie zbuduje.
  void send(const std::string &bytes) const {
    IPC::message_queue mq(IPC::open_only, names_.queryQueue.c_str());
    mq.send(bytes.data(), bytes.size(), 0);
  }

  /// Odpowiedz dla `clientId` z mapy odpowiedzi, zdjeta tak, jak zdejmuje ja IpcClient::netClient.
  [[nodiscard]] std::optional<ptree> response(int clientId) const {
    IPC::managed_shared_memory segment(IPC::open_only, names_.shmemSegment.c_str());
    IPC::named_mutex mutex(IPC::open_only, names_.mapMutex.c_str());
    ipc::IPCMap *map = segment.find<ipc::IPCMap>(std::string(ipc::kMapObject).c_str()).first;
    if (map == nullptr) return std::nullopt;
    const auto deadline = std::chrono::steady_clock::now() + kResponseBudget;
    while (std::chrono::steady_clock::now() < deadline) {
      {
        IPC::scoped_lock<IPC::named_mutex> lock(mutex);
        if (auto it = map->find(clientId); it != map->end()) {
          std::stringstream text;
          text << it->second;
          map->erase(it);
          ptree retVal;
          read_info(text, retVal);
          return retVal;
        }
      }
      std::this_thread::sleep_for(ipc::kClientResponsePollInterval);
    }
    return std::nullopt;
  }

 private:
  IpcServer server_;
  ipc::ServerNames names_;
  std::promise<bool> ready_;
  bool readyOk_{false};
  std::atomic<bool> stop_{false};
  std::atomic<int> handled_{0};
};

/// Komenda w formacie IpcClient::netClient; brak `clientId` daje komende bez db.id.
std::string command(std::optional<int> clientId, const std::string &argument) {
  ptree request;
  request.put("db.message", "hello");
  if (clientId) request.put("db.id", *clientId);
  request.put("db.argument", argument);
  std::stringstream text;
  write_info(text, request);
  return text.str();
}

}  // namespace

// Regresja S-02: bufor odbiorczy mial rozmiar kQueryQueueMaxMessageSize, a kolejka przyjmuje
// komende DOKLADNIE tej dlugosci - terminator ladowal jeden bajt za tablica. W zwyklym buildzie
// zapis bywa niewidoczny (trafia w zerowy bajt kanarka stosu), wiec czerwien daje dopiero
// -DRDB_SANITIZE=address; niezmiennik w kazdym buildzie pilnuje static_assert w commandLoop.
// Test pilnuje zachowania na granicy: komenda o pelnej dlugosci dochodzi w calosci,
// a watek komunikacyjny obsluguje nastepna.
TEST(IpcServerLoop, full_size_command_is_received_whole_and_server_keeps_serving) {
  RunningServer server;
  ASSERT_TRUE(server.ready()) << "watek komunikacyjny nie zbudowal zasobow IPC";

  // Dlugosc serializacji rosnie o bajt na znak argumentu, wiec dopelnienie liczy sie z jednej proby.
  const std::size_t overhead       = command(kClientFull, "x").size() - 1;
  const std::size_t argumentLength = ipc::kQueryQueueMaxMessageSize - overhead;
  const std::string full           = command(kClientFull, std::string(argumentLength, 'x'));
  ASSERT_EQ(full.size(), static_cast<std::size_t>(ipc::kQueryQueueMaxMessageSize));

  server.send(full);
  const auto first = server.response(kClientFull);
  ASSERT_TRUE(first.has_value()) << "brak odpowiedzi na komende o pelnej dlugosci";
  EXPECT_EQ(first->get<std::size_t>("argumentLength"), argumentLength) << "komenda dotarla obcieta";

  server.send(command(kClientNext, "po"));
  const auto next = server.response(kClientNext);
  ASSERT_TRUE(next.has_value()) << "watek komunikacyjny przestal obslugiwac komendy";
  EXPECT_EQ(next->get<std::size_t>("argumentLength"), 2U);
}

// Regresja S-03: tresc, ktorej parser INFO nie przyjmuje, rzucala info_parser_error poza
// jakimkolwiek catch watku komunikacyjnego - std::terminate calego serwera jedna wiadomoscia
// od dowolnego lokalnego procesu. Ma byc odmowa: nic nie wykonane, nastepna komenda obsluzona.
TEST(IpcServerLoop, garbage_command_is_rejected_and_server_keeps_serving) {
  RunningServer server;
  ASSERT_TRUE(server.ready()) << "watek komunikacyjny nie zbudowal zasobow IPC";

  server.send("}");

  server.send(command(kClientNext, "po"));
  const auto next = server.response(kClientNext);
  ASSERT_TRUE(next.has_value()) << "watek komunikacyjny przestal obslugiwac komendy";
  EXPECT_EQ(server.handled(), 1) << "znieksztalcona wiadomosc dotarla do handlera";
}

// Regresja S-03, druga droga: poprawne INFO bez db.id rzucalo bad_lexical_cast - i to dopiero
// PO wykonaniu komendy. Komenda bez adresu zwrotnego ma byc odrzucona PRZED handlerem, zeby
// odmowa nie miala skutkow ubocznych (kill, reset-commit bez nadawcy).
TEST(IpcServerLoop, command_without_client_id_is_rejected_and_server_keeps_serving) {
  RunningServer server;
  ASSERT_TRUE(server.ready()) << "watek komunikacyjny nie zbudowal zasobow IPC";

  server.send(command(std::nullopt, "bez adresu"));

  server.send(command(kClientNext, "po"));
  const auto next = server.response(kClientNext);
  ASSERT_TRUE(next.has_value()) << "watek komunikacyjny przestal obslugiwac komendy";
  EXPECT_EQ(server.handled(), 1) << "komenda bez db.id zostala wykonana";
}
