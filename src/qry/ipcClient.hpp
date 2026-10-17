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
// - i kończył się kodem `timed_out`, co harness pomiarowy odczytywał jako
// zniknięcie klienta (issue_217).
constexpr int kIpcClientDefaultResponseMaxFails = 300;

// Ile razy producent ponawia otwarcie własnej kolejki odpowiedzi, zanim uzna, że
// serwer jej nie utworzy. Kolejka powstaje po stronie serwera w reakcji na
// rejestrację klienta, więc przy starcie istnieje WYŚCIG: `open_only` wołane
// natychmiast potrafi trafić w moment przed jej utworzeniem. Odstęp między
// próbami to ipc::kClientResponsePollInterval.
constexpr int kIpcClientDefaultResponseQueueOpenMaxFails = 100;

/// Obiekt klasy IpcClient powinien:
/// - [ipcclient.ipc-client.1~1] używać nazw obiektów IPC serwera o podanej nazwie albo nazw historycznych dla nazwy pustej, a budżety prób przyjmować nie mniejsze niż 1.
/// - [ipcclient.ipc-client.2~1] w netClient wysyłać komendę z argumentem, pid procesu i numerem żądania unikalnym w obrębie procesu, także między kilkoma obiektami IpcClient, i odbierać odpowiedź wyłącznie ze slotu tego procesu (pid, czas startu procesu) i tego numeru żądania.
/// - [ipcclient.ipc-client.3~1] wyznaczać jeden termin zegarem monotonicznym jako chwilę rozpoczęcia żądania plus max(1, budżet prób) razy interwał odpytywania; czekać do niego na miejsce w kolejce komend, ponawiając próbę wysłania co interwał odpytywania, a po wysłaniu na odpowiedź tylko przez pozostały czas, bez odnawiania terminu. Termin ogranicza oczekiwanie, nie czas operacji systemowych ani opóźnienie planisty.
///   Nietestowalne: wspolny termin obu czekan jest kontrola strukturalna netClient w ipcClient.cpp (jeden steady_clock deadline dla petli try_send i take), nie pomiarem dwoch niezaleznych budzetow.
/// - [ipcclient.ipc-client.4~1] nie nadpisywać komend już czekających w kolejce komend.
/// - [ipcclient.ipc-client.5~1] zwracać odpowiedź z error.response równym server not found i ustawiać done przy niezgodnym układzie segmentu odpowiedzi (przed wysłaniem komendy), przy kolejce komend pełnej do terminu albo gdy odpowiedź nie nadejdzie przed terminem.
/// - [ipcclient.ipc-client.6~1] przy błędzie systemowym IPC lub innym wyjątku ustawiać done i przekazywać wyjątek wołającemu.
/// - [ipcclient.ipc-client.7~1] w producer ponawiać otwarcie własnej kolejki odpowiedzi do wyczerpania budżetu prób albo ustawienia done, a odmowę zaufania obiektowi (ipc::security::Rejected) traktować jako brak kolejki bez ponawiania.
/// - [ipcclient.ipc-client.8~1] ustawiać responseQueueMissing wyłącznie po odmowie zaufania albo po wyczerpaniu budżetu prób otwarcia, a nie po przerwaniu oczekiwania przez wątek główny, i w każdym z tych przypadków kończyć producenta z ustawionym done.
/// - [ipcclient.ipc-client.9~1] odbierać komunikaty kolejki odpowiedzi, czekając interwał odpytywania wyłącznie przy pustej kolejce, parsować je jako dokument info i przekazywać do kolejki SPSC o pojemności kSpscQueueCapacity, czekając na miejsce bez gubienia elementów, dopóki done nie jest ustawione.
///   Nietestowalne: brak snu po udanym odbiorze jest kontrola strukturalna producer w ipcClient.cpp; nie mierzymy szybkosci planisty.
/// - [ipcclient.ipc-client.10~1] przez popQueue zwracać kolejny element odebrany przez producenta bez blokowania.
/// - [ipcclient.ipc-client.11~1] kończyć pętlę producenta po ustawieniu done przez dowolny wątek, a przy wyjątku w odbiorze zapisywać diagnostykę i ustawiać done.
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
  // jak `done` po normalnym końcu strumienia - i klient kończył się zerem, nie
  // przeczytawszy nic (issue_215).
  std::atomic<bool> responseQueueMissing{false};

  explicit IpcClient(int clientResponseMaxFails    = kIpcClientDefaultResponseMaxFails,
                     int responseQueueOpenMaxFails = kIpcClientDefaultResponseQueueOpenMaxFails,
                     std::string_view serverName   = {});

  boost::property_tree::ptree netClient(const std::string &netCommand, const std::string &netArgument);
  void producer();
  bool popQueue(boost::property_tree::ptree &pt);
};
