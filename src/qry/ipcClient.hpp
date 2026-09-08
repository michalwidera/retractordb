#pragma once
#include <atomic>
#include <string>
#include <string_view>

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/property_tree/ptree.hpp>

#include "constants.hpp"

// Pojemność wewnętrznej kolejki SPSC między wątkiem producenta (IPC) a wątkiem select().
// Wartość musi być potęgą 2 i wystarczająco duża by zaabsorbować burstowe dane strumieniowe.
constexpr int kSpscQueueCapacity = 1024;

// Budżet klienta na odpowiedź serwera; efektywny czas to ta liczba razy
// ipc::kClientResponsePollInterval (300 × 10 ms = 3 s).
//
// Poprzednie 10 prób (100 ms) było za mało. Wątek komunikacyjny serwera powstaje
// przed `rtActivate`, więc zostaje SCHED_OTHER, podczas gdy wątek przetwarzania
// dostaje SCHED_FIFO; przy pracy pod `taskset` na jednym rdzeniu wątek
// komunikacyjny dostaje CPU dopiero w oknie throttlingu RT, którego okres jest
// rzędu sekundy. Klient poddawał się, zanim serwer w ogóle został zaszeregowany
// — i kończył się kodem `timed_out`, co harness pomiarowy odczytywał jako
// zniknięcie klienta (issue_217).
constexpr int kIpcClientDefaultResponseMaxFails = 300;

// Ile razy producent ponawia otwarcie własnej kolejki odpowiedzi, zanim uzna, że
// serwer jej nie utworzy. Kolejka powstaje po stronie serwera w reakcji na
// rejestrację klienta, więc przy starcie istnieje WYŚCIG: `open_only` wołane
// natychmiast potrafi trafić w moment przed jej utworzeniem. Odstęp między
// próbami to ipc::kClientResponsePollInterval.
constexpr int kIpcClientDefaultResponseQueueOpenMaxFails = 100;

class IpcClient {
  boost::lockfree::spsc_queue<boost::property_tree::ptree, boost::lockfree::capacity<kSpscQueueCapacity>> spsc_queue_;
  int clientResponseMaxFails_{kIpcClientDefaultResponseMaxFails};
  int responseQueueOpenMaxFails_{kIpcClientDefaultResponseQueueOpenMaxFails};

  // Komplet nazw obiektow IPC serwera, z ktorym rozmawia ten klient. Nazwa pusta =
  // nazwy historyczne, jednoserwerowe.
  ipc::ServerNames names_{ipc::names()};

 public:
  std::atomic<bool> done{false};

  // Producent nie zdołał otworzyć własnej kolejki odpowiedzi mimo ponowień.
  // Bez tego rozróżnienia `done` po nieudanym otwarciu wyglądało identycznie
  // jak `done` po normalnym końcu strumienia — i klient kończył się zerem, nie
  // przeczytawszy nic (issue_215).
  std::atomic<bool> responseQueueMissing{false};

  explicit IpcClient(int clientResponseMaxFails    = kIpcClientDefaultResponseMaxFails,
                     int responseQueueOpenMaxFails = kIpcClientDefaultResponseQueueOpenMaxFails,
                     std::string_view serverName   = {});

  boost::property_tree::ptree netClient(const std::string &netCommand, const std::string &netArgument);
  void producer();
  bool popQueue(boost::property_tree::ptree &pt);
};
