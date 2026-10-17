#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <future>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/property_tree/info_parser.hpp>

#include "constants.hpp"
#include "ipcResponses.hpp"
#include "logCapture.hpp"
#include "retractor/lib/ipcServer.hpp"
#include "syscallWrap.hpp"

namespace {

namespace IPC = boost::interprocess;

std::string pausedQueue;
std::promise<void> *queueCreationEntered = nullptr;
std::shared_future<void> queueCreationResume;
bool noQueueSpace = false;

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

/// Stan, ktory zostawia proces zabity w srodku send/receive: wewnetrzny muteks kolejki zajety
/// przez proces, ktory juz nie zyje. Potomek bierze go wprost z segmentu kolejki, zglasza to
/// przez potok i czeka, a rodzic zabija go kill -9 - dopiero po zgloszeniu, wiec bez wyscigu.
/// Uklad naglowka (mq_hdr_t za ManagedOpenOrCreateUserOffset) to szczegol Boosta; gdyby sie
/// zmienil, testy ponizej oblewaja na kontroli dodatniej, a nie przechodza falszywie.
///
/// SIGKILL z zewnatrz, a nie _exit ani raise(SIGKILL): w obu tych przypadkach valgrind przed
/// koncem potomka robil kontrole wyciekow na stercie skopiowanej od rodzica, gdzie TLS jego
/// watku komunikacyjnego wychodzil jako "possibly lost". Nikt tego nie widzial, bo waitpid
/// gubil status - teraz status jest sprawdzany. Wyjatek w potomku zamyka potok: rodzic dostaje
/// EOF i tez zbiera go przez SIGKILL, bez kontroli odziedziczonej sterty. Potomek nigdy nie
/// wraca do gtest, zeby nie puscic reszty zestawu obok rodzica.
void abandonQueueLock(const std::string &queueName) {
  int locked[2];
  ASSERT_EQ(pipe(locked), 0);
  const pid_t child = fork();
  if (child < 0) {
    const int error = errno;
    close(locked[0]);
    close(locked[1]);
    FAIL() << "fork: errno " << error;
  }
  if (child == 0) {
    close(locked[0]);
    try {
      using Impl   = IPC::ipcdetail::managed_open_or_create_impl<IPC::shared_memory_object, 0, true, false>;
      using Header = IPC::ipcdetail::mq_hdr_t<IPC::offset_ptr<void>>;
      IPC::shared_memory_object shm(IPC::open_only, queueName.c_str(), IPC::read_write);
      IPC::mapped_region region(shm, IPC::read_write);
      auto *header = reinterpret_cast<Header *>(static_cast<char *>(region.get_address()) + Impl::ManagedOpenOrCreateUserOffset);
      header->m_mutex.lock();
      const char byte = 1;
      ssize_t written;
      do {
        written = write(locked[1], &byte, 1);
      } while (written < 0 && errno == EINTR);
      close(locked[1]);
      for (;;)
        pause();
    } catch (...) {
      close(locked[1]);
      for (;;)
        pause();
    }
  }
  close(locked[1]);
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  pollfd ready{locked[0], POLLIN, 0};
  char byte        = 0;
  ssize_t received = -1;
  int pipeError    = 0;
  bool timedOut    = false;
  for (;;) {
    const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
    if (remaining.count() <= 0) {
      timedOut = true;
      break;
    }
    const int result = poll(&ready, 1, static_cast<int>(remaining.count()));
    if (result < 0) {
      if (errno == EINTR) continue;
      pipeError = errno;
      break;
    }
    if (result == 0) {
      timedOut = true;
      break;
    }
    if (ready.revents & (POLLIN | POLLHUP)) {
      received = read(locked[0], &byte, 1);
      if (received < 0 && errno == EINTR) continue;
      if (received < 0) pipeError = errno;
      break;
    }
    pipeError = (ready.revents & POLLNVAL) ? EBADF : EIO;
    break;
  }
  close(locked[0]);
  const int killed    = kill(child, SIGKILL);
  const int killError = errno;
  int status          = 0;
  pid_t waited;
  do {
    waited = waitpid(child, &status, 0);
  } while (waited < 0 && errno == EINTR);
  ASSERT_EQ(waited, child) << "waitpid: errno " << errno;
  ASSERT_EQ(killed, 0) << "kill: errno " << killError;
  ASSERT_FALSE(timedOut) << "przekroczono czas oczekiwania na muteks kolejki";
  ASSERT_EQ(pipeError, 0) << "potok: errno " << pipeError;
  ASSERT_TRUE(received == 1 && byte == 1) << "potomek nie wzial muteksu kolejki (odczyt: " << received << ")";
  EXPECT_TRUE(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL) << "potomek: status " << status;
}

/// Kontrola dodatnia dla abandonQueueLock: kolejka po niej rzuca not_recoverable. Uchwyt
/// otwarty PRZED porzuceniem - serwer moze juz odtworzyc kolejke pod ta sama nazwa.
bool queueLockIsAbandoned(IPC::message_queue &mq) {
  try {
    std::array<char, ipc::kResponseQueueMaxMessageSize> buffer{};
    IPC::message_queue::size_type received = 0;
    unsigned int priority                  = 0;
    (void)mq.try_receive(buffer.data(), buffer.size(), received, priority);
    return false;
  } catch (const IPC::interprocess_exception &ex) {
    return ex.get_error_code() == IPC::not_recoverable;
  }
}

/// Czy martwy wlasciciel muteksu kolejki jest w ogole wykrywalny. Boost ustawia
/// BOOST_INTERPROCESS_MUTEX_USE_POSIX, gdy interprocess_mutex jest robust pthread mutexem; w przeciwnym
/// razie (Darwin, FORCE_GENERIC_EMULATION) to spin_mutex, na ktorym kazde zajecie po smierci wlasciciela
/// kreci sie bez konca - takze sama kontrola dodatnia queueLockIsAbandoned. Issue #410.
#if defined(BOOST_INTERPROCESS_MUTEX_USE_POSIX)
constexpr bool kDeadQueueOwnerDetectable = true;
#else
constexpr bool kDeadQueueOwnerDetectable = false;
#endif

bool segmentExists(const std::string &name) {
  try {
    IPC::shared_memory_object shm(IPC::open_only, name.c_str(), IPC::read_only);
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
  void TearDown() override {
    queueCreationEntered = nullptr;
    pausedQueue.clear();
    queueCreationResume = {};
    noQueueSpace        = false;
    wipe();
  }

  static void wipe() {
    for (const std::string_view server : {std::string_view{}, kServerA, kServerB}) {
      IPC::message_queue::remove(queueNameFor(kClientA, server).c_str());
      IPC::message_queue::remove(queueNameFor(kClientB, server).c_str());
      IPC::shared_memory_object::remove(ipc::names(server).shmemSegment.c_str());
    }
  }
};

}  // namespace

extern "C" int __real_shm_open(const char *name, int flags, mode_t mode);
extern "C" int __real_fstatvfs(int fd, struct statvfs *info);
extern "C" int __real_statvfs(const char *path, struct statvfs *info);
RDB_WRAP_SYSCALL(int, shm_open, (const char *name, int flags, mode_t mode), (name, flags, mode));
RDB_WRAP_SYSCALL(int, fstatvfs, (int fd, struct statvfs *info), (fd, info));
RDB_WRAP_SYSCALL(int, statvfs, (const char *path, struct statvfs *info), (path, info));
extern "C" int __wrap_shm_open(const char *name, int flags, mode_t mode) {
  std::string_view token(name);
  if (token.starts_with('/')) token.remove_prefix(1);
  if ((flags & O_CREAT) && queueCreationEntered != nullptr && token == pausedQueue) {
    queueCreationEntered->set_value();
    queueCreationResume.wait();
  }
  return __real_shm_open(name, flags, mode);
}
extern "C" int __wrap_fstatvfs(int fd, struct statvfs *info) {
  const int result = __real_fstatvfs(fd, info);
  if (result == 0 && noQueueSpace) info->f_bavail = 0;
  return result;
}
extern "C" int __wrap_statvfs(const char *path, struct statvfs *info) {
  const int result = __real_statvfs(path, info);
  if (result == 0 && noQueueSpace) info->f_bavail = 0;
  return result;
}

static_assert(!std::is_copy_constructible_v<IpcServer> && !std::is_copy_assignable_v<IpcServer>);

TEST_F(IpcServerQueues, abandonQueueLock_reports_eof_from_child) {
  EXPECT_FATAL_FAILURE(abandonQueueLock(queueNameFor(kClientA)), "potomek nie wzial muteksu kolejki");
}

TEST_F(IpcServerQueues, abandonQueueLock_times_out_when_mutex_is_busy) {
  IPC::message_queue mq(IPC::create_only, queueNameFor(kClientA).c_str(), 1, 1);
  using Impl   = IPC::ipcdetail::managed_open_or_create_impl<IPC::shared_memory_object, 0, true, false>;
  using Header = IPC::ipcdetail::mq_hdr_t<IPC::offset_ptr<void>>;
  IPC::shared_memory_object shm(IPC::open_only, queueNameFor(kClientA).c_str(), IPC::read_write);
  IPC::mapped_region region(shm, IPC::read_write);
  auto *header = reinterpret_cast<Header *>(static_cast<char *>(region.get_address()) + Impl::ManagedOpenOrCreateUserOffset);
  header->m_mutex.lock();
  EXPECT_FATAL_FAILURE(abandonQueueLock(queueNameFor(kClientA)), "przekroczono czas oczekiwania");
  header->m_mutex.unlock();
}

TEST_F(IpcServerQueues, subscribe_creates_response_queue) {
  IpcServer server;
  ASSERT_TRUE(server.subscribe(kClientA, "strumien", 16, server.subscriptionEpoch()));
  EXPECT_TRUE(queueExists(kClientA));
}

TEST_F(IpcServerQueues, removeAllObjects_removes_every_client_queue) {
  IpcServer server;
  ASSERT_TRUE(server.subscribe(kClientA, "strumien", 16, server.subscriptionEpoch()));
  ASSERT_TRUE(server.subscribe(kClientB, "strumien", 16, server.subscriptionEpoch()));
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
  ASSERT_TRUE(server.subscribe(kClientA, "strumien", 16, server.subscriptionEpoch()));
  ASSERT_TRUE(server.subscribe(kClientB, "strumien", 16, server.subscriptionEpoch()));
  ASSERT_TRUE(queueExists(kClientA));
  ASSERT_TRUE(queueExists(kClientB));

  server.shutdownFromExitHandler();

  EXPECT_FALSE(queueExists(kClientA)) << "kolejka klienta przetrwala sciezke atexit";
  EXPECT_FALSE(queueExists(kClientB)) << "kolejka klienta przetrwala sciezke atexit";
}

// Obie drogi wyjscia kasuja ten sam zestaw obiektow globalnych, takze segment odpowiedzi.
TEST_F(IpcServerQueues, exit_handler_removes_response_segment) {
  const std::string segmentName = ipc::names().shmemSegment;
  { const ipc::responses::Mapping created(IPC::create_only, segmentName); }
  ASSERT_TRUE(segmentExists(segmentName));

  IpcServer server;
  server.shutdownFromExitHandler();

  EXPECT_FALSE(segmentExists(segmentName));
}

// Pusta nazwa serwera musi dawac DOKLADNIE nazwy historyczne. To jest kontrakt
// parameteryzacji: sama w sobie nie moze zmienic ani jednej nazwy w /dev/shm.
TEST_F(IpcServerQueues, empty_server_name_yields_historical_names) {
  const ipc::ServerNames n = ipc::names();
  EXPECT_EQ(n.shmemSegment, std::string(ipc::kShmemSegment));
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

  ASSERT_TRUE(serverA.subscribe(kClientA, "strumien", 16, serverA.subscriptionEpoch()));
  ASSERT_TRUE(serverB.subscribe(kClientA, "strumien", 16, serverB.subscriptionEpoch()));

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

// Ta sama rozlacznosc dla segmentu odpowiedzi: sciezka atexit serwera A nie moze
// zdjac segmentu serwera B.
TEST_F(IpcServerQueues, exit_handler_does_not_touch_other_servers_segment) {
  const std::string segmentB = ipc::names(kServerB).shmemSegment;
  { const ipc::responses::Mapping createdB(IPC::create_only, segmentB); }
  ASSERT_TRUE(segmentExists(segmentB));

  IpcServer serverA;
  serverA.setServerName(kServerA);
  serverA.shutdownFromExitHandler();

  EXPECT_TRUE(segmentExists(segmentB)) << "sciezka atexit serwera A skasowala segment odpowiedzi serwera B";
}

// Po skasowaniu kolejek rejestr subskrypcji nie moze zostac z wpisami wskazujacymi
// na nieistniejace kolejki: drugie wywolanie ma nie miec czego kasowac i nie moze
// rzucic ani zawiesic.
// Klient zabity w srodku try_receive na swojej kolejce odpowiedzi. try_send rzucal wtedy
// lock_exception(not_recoverable) poza broadcast() - az za petle przetwarzania, czyli jeden
// klient konczyl serwer wszystkim. Kolejka martwego ma zniknac jak przepelniona, emisja trwac.
TEST_F(IpcServerQueues, broadcast_drops_queue_abandoned_by_dead_client) {
  if (!kDeadQueueOwnerDetectable) GTEST_SKIP() << "#410: zamek kolejki to spin_mutex - martwy wlasciciel = wieczny spin";
  IpcServer server;
  ASSERT_TRUE(server.subscribe(kClientA, "strumien", 16, server.subscriptionEpoch()));
  ASSERT_TRUE(server.subscribe(kClientB, "strumien", 16, server.subscriptionEpoch()));
  IPC::message_queue probe(IPC::open_only, queueNameFor(kClientA).c_str());
  ASSERT_NO_FATAL_FAILURE(abandonQueueLock(queueNameFor(kClientA)));
  ASSERT_TRUE(queueLockIsAbandoned(probe)) << "symulacja martwego wlasciciela nie zadzialala";

  const std::array<std::string_view, 1> streams{"strumien"};
  EXPECT_NO_THROW(server.broadcast(streams, [](const std::string &) { return std::string("wiersz"); }));

  EXPECT_FALSE(queueExists(kClientA)) << "kolejka martwego klienta zostala";
  EXPECT_TRUE(queueExists(kClientB)) << "zdrowy klient stracil kolejke";
  EXPECT_NO_THROW(server.broadcast(streams, [](const std::string &) { return std::string("wiersz"); }));
  server.removeAllObjects();
}

// #283: subscribe() buduje kolejke bez blokady epoki planu, wiec rejestr subskrypcji moze zostac
// zamkniety (OOB na koncu epoki) miedzy odczytem parametrow strumienia a rejestracja. Spozniona
// rejestracja przezylaby OOB i przy --reset trafila na strumien nowego planu o tej samej nazwie.
TEST_F(IpcServerQueues, subscribe_after_out_of_business_is_refused_without_queue) {
  IpcServer server;
  const std::uint64_t epoch = server.subscriptionEpoch();
  server.broadcastOutOfBusiness();

  EXPECT_FALSE(server.subscribe(kClientA, "strumien", 16, epoch));
  EXPECT_FALSE(queueExists(kClientA)) << "odmowa zostawila kolejke w /dev/shm";

  // Odmowa nie wpisala klienta: emisja nie ma do kogo pisac i nie probuje otwierac kolejki.
  const std::array<std::string_view, 1> streams{"strumien"};
  bool formatted = false;
  server.broadcast(streams, [&formatted](const std::string &) {
    formatted = true;
    return std::string("wiersz");
  });
  EXPECT_FALSE(formatted) << "odrzucony klient zostal w rejestrze";

  // Kontrola dodatnia: z epoka biezaca ta sama subskrypcja przechodzi.
  EXPECT_TRUE(server.subscribe(kClientA, "strumien", 16, server.subscriptionEpoch()));
  EXPECT_TRUE(queueExists(kClientA));
  server.removeAllObjects();
}

// Ta sama straz na drugiej drodze zamkniecia rejestru: kasowanie kolejek przy wyjsciu.
TEST_F(IpcServerQueues, subscribe_after_queue_removal_is_refused_without_queue) {
  IpcServer server;
  const std::uint64_t epoch = server.subscriptionEpoch();
  server.removeAllObjects();

  EXPECT_FALSE(server.subscribe(kClientA, "strumien", 16, epoch));
  EXPECT_FALSE(queueExists(kClientA)) << "odmowa zostawila kolejke w /dev/shm";
}

TEST_F(IpcServerQueues, removal_is_idempotent) {
  IpcServer server;
  ASSERT_TRUE(server.subscribe(kClientA, "strumien", 16, server.subscriptionEpoch()));

  server.removeAllObjects();
  EXPECT_NO_THROW(server.removeAllObjects());
  EXPECT_NO_THROW(server.shutdownFromExitHandler());
  EXPECT_FALSE(queueExists(kClientA));
}

TEST_F(IpcServerQueues, broadcast_formats_once_for_subscribers_and_drops_only_the_full_queue) {
  IpcServer server;
  ASSERT_TRUE(server.subscribe(kClientA, "stream", 1, server.subscriptionEpoch()));
  ASSERT_TRUE(server.subscribe(kClientB, "stream", 3, server.subscriptionEpoch()));
  IPC::message_queue full(IPC::open_only, queueNameFor(kClientA).c_str());
  IPC::message_queue healthy(IPC::open_only, queueNameFor(kClientB).c_str());
  full.send("sentinel", 8, 0);
  int formatted = 0;
  const std::array<std::string_view, 2> streams{"unsubscribed", "stream"};
  server.broadcast(streams, [&](const std::string &name) {
    EXPECT_EQ(name, "stream");
    ++formatted;
    return std::string("row");
  });
  EXPECT_EQ(formatted, 1);
  EXPECT_FALSE(queueExists(kClientA));
  EXPECT_TRUE(queueExists(kClientB));
  EXPECT_EQ(full.get_num_msg(), 1U);
  EXPECT_EQ(healthy.get_num_msg(), 1U);
  server.broadcast(streams, [&](const std::string &) {
    ++formatted;
    return std::string("next");
  });
  EXPECT_EQ(formatted, 2);
  EXPECT_EQ(healthy.get_num_msg(), 2U);
  server.removeAllObjects();
}

TEST_F(IpcServerQueues, oversized_rows_are_skipped_and_reported_once_per_stream) {
  IpcServer server;
  ASSERT_TRUE(server.subscribe(kClientA, "wide", 2, server.subscriptionEpoch()));
  ASSERT_TRUE(server.subscribe(kClientB, "other-wide", 2, server.subscriptionEpoch()));
  const LogCapture log;
  const std::array<std::string_view, 2> streams{"wide", "other-wide"};
  for (int i = 0; i < 2; ++i)
    EXPECT_NO_THROW(
        server.broadcast(streams, [](const std::string &) { return std::string(ipc::kResponseQueueMaxMessageSize + 1, 'x'); }));
  IPC::message_queue queue(IPC::open_only, queueNameFor(kClientA).c_str());
  IPC::message_queue other(IPC::open_only, queueNameFor(kClientB).c_str());
  EXPECT_EQ(queue.get_num_msg(), 0U);
  EXPECT_EQ(other.get_num_msg(), 0U);
  const std::string text = log.text();
  const auto first       = text.find("does not fit");
  ASSERT_NE(first, std::string::npos);
  const auto second = text.find("does not fit", first + 1);
  ASSERT_NE(second, std::string::npos);
  EXPECT_EQ(text.find("does not fit", second + 1), std::string::npos);
  EXPECT_NE(text.find("stream 'wide'"), std::string::npos);
  EXPECT_NE(text.find("stream 'other-wide'"), std::string::npos);
  server.broadcast(streams, [](const std::string &) { return std::string(ipc::kResponseQueueMaxMessageSize, 'x'); });
  EXPECT_EQ(queue.get_num_msg(), 1U);
  EXPECT_EQ(other.get_num_msg(), 1U);
  server.removeAllObjects();
}

TEST_F(IpcServerQueues, out_of_business_sends_marker_and_closes_the_subscription_epoch) {
  IpcServer server;
  const auto epoch = server.subscriptionEpoch();
  ASSERT_TRUE(server.subscribe(kClientA, "stream", 2, epoch));
  IPC::message_queue queue(IPC::open_only, queueNameFor(kClientA).c_str());
  server.broadcastOutOfBusiness();
  EXPECT_EQ(server.subscriptionEpoch(), epoch + 1);
  EXPECT_FALSE(queueExists(kClientA));
  char bytes[ipc::kResponseQueueMaxMessageSize]{};
  std::size_t size  = 0;
  unsigned priority = 0;
  ASSERT_TRUE(queue.try_receive(bytes, sizeof(bytes), size, priority));
  std::stringstream text(std::string(bytes, size));
  boost::property_tree::ptree row;
  read_info(text, row);
  EXPECT_EQ(row.get<std::string>("stream"), constants::Reserved_id_oob);
  bool formatted = false;
  const std::array<std::string_view, 1> streams{"stream"};
  server.broadcast(streams, [&](const std::string &) {
    formatted = true;
    return std::string("unexpected");
  });
  EXPECT_FALSE(formatted);
}

// OOB to jedna proba bez czekania: pelna kolejka nie dostaje znacznika, ale jej nazwa znika.
TEST_F(IpcServerQueues, out_of_business_to_full_queue_drops_the_marker_and_removes_the_queue) {
  IpcServer server;
  const auto epoch = server.subscriptionEpoch();
  ASSERT_TRUE(server.subscribe(kClientA, "stream", 1, epoch));
  IPC::message_queue queue(IPC::open_only, queueNameFor(kClientA).c_str());
  queue.send("sentinel", 8, 0);
  server.broadcastOutOfBusiness();
  EXPECT_EQ(server.subscriptionEpoch(), epoch + 1);
  EXPECT_FALSE(queueExists(kClientA));
  char bytes[ipc::kResponseQueueMaxMessageSize]{};
  std::size_t size  = 0;
  unsigned priority = 0;
  ASSERT_TRUE(queue.try_receive(bytes, sizeof(bytes), size, priority));
  EXPECT_EQ(std::string(bytes, size), "sentinel");
  EXPECT_FALSE(queue.try_receive(bytes, sizeof(bytes), size, priority)) << "znacznik konca trafil do pelnej kolejki";
}

// #523: try_send porzuconej kolejki rzuca not_recoverable, a wyjatek omijal removeOwned. Rejestr byl
// mimo to czyszczony, wiec kolejki nie usuwalo potem nawet removeAllObjects - zostawala w /dev/shm.
TEST_F(IpcServerQueues, out_of_business_removes_queue_abandoned_by_dead_client) {
  if (!kDeadQueueOwnerDetectable) GTEST_SKIP() << "#410: zamek kolejki to spin_mutex - martwy wlasciciel = wieczny spin";
  IpcServer server;
  const auto epoch = server.subscriptionEpoch();
  ASSERT_TRUE(server.subscribe(kClientA, "strumien", 16, epoch));
  ASSERT_TRUE(server.subscribe(kClientB, "strumien", 16, epoch));
  IPC::message_queue probe(IPC::open_only, queueNameFor(kClientA).c_str());
  ASSERT_NO_FATAL_FAILURE(abandonQueueLock(queueNameFor(kClientA)));
  ASSERT_TRUE(queueLockIsAbandoned(probe)) << "symulacja martwego wlasciciela nie zadzialala";

  EXPECT_NO_THROW(server.broadcastOutOfBusiness());
  EXPECT_EQ(server.subscriptionEpoch(), epoch + 1);
  EXPECT_FALSE(queueExists(kClientB)) << "zdrowa kolejka nie zostala usunieta";
  EXPECT_FALSE(queueExists(kClientA)) << "kolejka martwego klienta zostala po koncu epoki";
  bool formatted = false;
  const std::array<std::string_view, 1> streams{"strumien"};
  server.broadcast(streams, [&](const std::string &) {
    formatted = true;
    return std::string("unexpected");
  });
  EXPECT_FALSE(formatted) << "rejestr nie zostal wyczyszczony";
  server.removeAllObjects();
}

TEST_F(IpcServerQueues, insufficient_space_refuses_subscription_before_queue_creation) {
  IpcServer server;
  noQueueSpace = true;
  try {
    server.subscribe(kClientA, "expensive", 16, server.subscriptionEpoch());
    ADD_FAILURE() << "subskrypcja bez wolnej pamieci przeszla";
  } catch (const std::runtime_error &error) {
    const std::string reason(error.what());
    EXPECT_NE(reason.find("expensive"), std::string::npos);
    EXPECT_NE(reason.find("needs"), std::string::npos);
    EXPECT_NE(reason.find("16 elements"), std::string::npos);
    EXPECT_NE(reason.find("0 B free"), std::string::npos);
  }
  noQueueSpace = false;
  EXPECT_FALSE(queueExists(kClientA));
}

TEST_F(IpcServerQueues, epoch_can_close_while_queue_creation_is_paused) {
  if (!RDB_HAS_BOOST_POSIX_SHM) GTEST_SKIP() << "synchronizacja na shm_open wymaga backendu POSIX";
  IpcServer server;
  const auto epoch = server.subscriptionEpoch();
  std::promise<void> entered, resume;
  auto entrance        = entered.get_future();
  pausedQueue          = queueNameFor(kClientA);
  queueCreationResume  = resume.get_future().share();
  queueCreationEntered = &entered;
  auto subscription    = std::async(std::launch::async, [&] { return server.subscribe(kClientA, "stream", 2, epoch); });
  const bool reached   = entrance.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
  EXPECT_TRUE(reached);
  // Takze regresja trzymajaca muteks podczas mmap musi dac sie odblokowac po czerwonej asercji.
  auto closing = std::async(std::launch::async, [&] { server.broadcastOutOfBusiness(); });
  EXPECT_EQ(closing.wait_for(std::chrono::seconds(2)), std::future_status::ready)
      << "tworzenie kolejki zatrzymalo zamkniecie rejestru";
  resume.set_value();
  closing.get();
  EXPECT_FALSE(subscription.get());
  queueCreationEntered = nullptr;
  pausedQueue.clear();
  queueCreationResume = {};
  EXPECT_FALSE(queueExists(kClientA));
  EXPECT_EQ(server.subscriptionEpoch(), epoch + 1);
}

TEST_F(IpcServerQueues, exit_handler_does_not_wait_for_the_broadcast_map_lock) {
  IpcServer server;
  ASSERT_TRUE(server.subscribe(kClientA, "stream", 2, server.subscriptionEpoch()));
  std::promise<void> entered, resume;
  auto entrance  = entered.get_future();
  auto release   = resume.get_future().share();
  auto broadcast = std::async(std::launch::async, [&] {
    const std::array<std::string_view, 1> streams{"stream"};
    server.broadcast(streams, [&](const std::string &) {
      entered.set_value();
      release.wait();
      return std::string("row");
    });
  });
  EXPECT_EQ(entrance.wait_for(std::chrono::seconds(5)), std::future_status::ready);
  auto shutdown = std::async(std::launch::async, [&] { server.shutdownFromExitHandler(); });
  EXPECT_EQ(shutdown.wait_for(std::chrono::seconds(2)), std::future_status::ready);
  resume.set_value();
  broadcast.get();
  shutdown.get();
  EXPECT_TRUE(queueExists(kClientA));
  server.removeAllObjects();
  EXPECT_FALSE(queueExists(kClientA));
}

namespace {

// Wlasny obszar nazw: watek komunikacyjny kasuje na wejsciu segment i kolejke komend
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
  explicit RunningServer(IpcServer::CommandHandler handler = {}) : names_(ipc::names(kServerLoop)) {
    server_.setServerName(kServerLoop);
    auto readyFuture = ready_.get_future();
    server_.start({.onCommand =
                       [this, handler](const ptree &request) {
                         ++handled_;
                         if (handler) return handler(request);
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

  /// send ponawiany, dopoki kolejka pod ta nazwa nie przyjmie wiadomosci. Po odtworzeniu kolejki
  /// przez serwer pierwsze proby moga jeszcze trafic w stara, porzucona.
  [[nodiscard]] bool sendWithin(const std::string &bytes) const {
    const auto deadline = std::chrono::steady_clock::now() + kResponseBudget;
    while (std::chrono::steady_clock::now() < deadline) {
      try {
        send(bytes);
        return true;
      } catch (const IPC::interprocess_exception &) {
        std::this_thread::sleep_for(ipc::kClientResponsePollInterval);
      }
    }
    return false;
  }

  [[nodiscard]] const std::string &commandQueue() const { return names_.queryQueue; }

  /// Odpowiedz dla `clientId` ze slotow odpowiedzi, zdjeta tak, jak zdejmuje ja IpcClient::netClient.
  /// Komendy testu nie niosa db.seq, a `clientId` nie jest zywym procesem, wiec serwer zapisuje
  /// seq 0 i startTime 0 (znacznika startu nie ma czego odczytac).
  [[nodiscard]] std::optional<ptree> response(int clientId) const {
    const ipc::responses::Mapping responses(IPC::open_only, names_.shmemSegment);
    if (!responses.valid()) return std::nullopt;
    const auto text = ipc::responses::take(responses.segment(), {.pid = clientId, .startTime = 0, .seq = 0},
                                           std::chrono::steady_clock::now() + kResponseBudget, ipc::kClientResponsePollInterval);
    if (!text) return std::nullopt;
    std::stringstream stream(*text);
    ptree retVal;
    read_info(stream, retVal);
    return retVal;
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

// Klient zabity w srodku send z muteksem kolejki komend w reku. try_receive rzucal potem
// not_recoverable przy kazdym obrocie, a wyjatek konczyl watek komunikacyjny: serwer liczyl
// dalej, ale nie przyjmowal juz zadnej komendy. Ma odtworzyc kolejke i obsluzyc nastepna.
TEST(IpcServerLoop, command_queue_abandoned_by_dead_client_is_recreated) {
  if (!kDeadQueueOwnerDetectable) GTEST_SKIP() << "#410: zamek kolejki to spin_mutex - martwy wlasciciel = wieczny spin";
  RunningServer server;
  ASSERT_TRUE(server.ready()) << "watek komunikacyjny nie zbudowal zasobow IPC";

  IPC::message_queue probe(IPC::open_only, server.commandQueue().c_str());
  ASSERT_NO_FATAL_FAILURE(abandonQueueLock(server.commandQueue()));
  ASSERT_TRUE(queueLockIsAbandoned(probe)) << "symulacja martwego wlasciciela nie zadzialala";

  ASSERT_TRUE(server.sendWithin(command(kClientNext, "po"))) << "kolejka komend nie zostala odtworzona";
  const auto next = server.response(kClientNext);
  ASSERT_TRUE(next.has_value()) << "watek komunikacyjny przestal obslugiwac komendy";
  EXPECT_EQ(server.handled(), 1);
}

TEST(IpcServerLoop, handler_exception_is_answered_and_the_next_command_is_served) {
  RunningServer server([](const ptree &request) {
    if (request.get<std::string>("db.argument") == "throw") throw std::runtime_error("contract failure");
    ptree response;
    response.put("result", "next");
    return response;
  });
  ASSERT_TRUE(server.ready());
  server.send(command(kClientFull, "throw"));
  const auto failed = server.response(kClientFull);
  ASSERT_TRUE(failed.has_value());
  EXPECT_EQ(failed->get<std::string>("error.response", ""), "command handler failure: contract failure");
  server.send(command(kClientNext, "ok"));
  const auto next = server.response(kClientNext);
  ASSERT_TRUE(next.has_value());
  EXPECT_EQ(next->get<std::string>("result", ""), "next");
}

TEST(IpcServerLoop, exception_after_ready_stops_thread_without_announcing_startup_failure) {
  IpcServer server;
  server.setServerName("postready" + std::to_string(getpid()));
  int ready = 0, failure = 0;
  const LogCapture log;
  server.start({.onCommand        = [](const ptree &) { return ptree{}; },
                .onReady          = [&] { ++ready; },
                .onFailure        = [&] { ++failure; },
                .onCommandHandled = [] {},
                .shouldStop       = []() -> bool { throw std::runtime_error("stop predicate failure"); }});
  server.stop();
  server.removeAllObjects();
  EXPECT_EQ(ready, 1);
  EXPECT_EQ(failure, 0);
  EXPECT_NE(log.text().find("server no longer accepts commands: stop predicate failure"), std::string::npos);
}
