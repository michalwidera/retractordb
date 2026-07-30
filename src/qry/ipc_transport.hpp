#pragma once
#include <atomic>
#include <string>

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/property_tree/ptree.hpp>

// Pojemność wewnętrznej kolejki SPSC między wątkiem producenta (IPC) a wątkiem select().
// Wartość musi być potęgą 2 i wystarczająco duża by zaabsorbować burstowe dane strumieniowe.
constexpr int kSpscQueueCapacity                         = 1024;
constexpr int kIpcTransportDefaultClientResponseMaxFails = 10;

// Ile razy producent ponawia otwarcie własnej kolejki odpowiedzi, zanim uzna, że
// serwer jej nie utworzy. Kolejka powstaje po stronie serwera w reakcji na
// rejestrację klienta, więc przy starcie istnieje WYŚCIG: `open_only` wołane
// natychmiast potrafi trafić w moment przed jej utworzeniem. Odstęp między
// próbami to ipc::kClientResponsePollInterval.
constexpr int kIpcTransportDefaultResponseQueueOpenMaxFails = 100;

class IpcTransport {
  boost::lockfree::spsc_queue<boost::property_tree::ptree, boost::lockfree::capacity<kSpscQueueCapacity>> spsc_queue_;
  int clientResponseMaxFails_{kIpcTransportDefaultClientResponseMaxFails};
  int responseQueueOpenMaxFails_{kIpcTransportDefaultResponseQueueOpenMaxFails};

 public:
  std::atomic<bool> done{false};

  // Producent nie zdołał otworzyć własnej kolejki odpowiedzi mimo ponowień.
  // Bez tego rozróżnienia `done` po nieudanym otwarciu wyglądało identycznie
  // jak `done` po normalnym końcu strumienia — i klient kończył się zerem, nie
  // przeczytawszy nic (issue_215).
  std::atomic<bool> responseQueueMissing{false};

  explicit IpcTransport(int clientResponseMaxFails    = kIpcTransportDefaultClientResponseMaxFails,
                        int responseQueueOpenMaxFails = kIpcTransportDefaultResponseQueueOpenMaxFails);

  boost::property_tree::ptree netClient(const std::string &netCommand, const std::string &netArgument);
  void producer();
  bool popQueue(boost::property_tree::ptree &pt);
};
