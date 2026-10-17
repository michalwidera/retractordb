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
/// - używać nazw obiektów IPC serwera o podanej nazwie albo nazw historycznych dla nazwy pustej, a budżety prób przyjmować nie mniejsze niż 1.
///   Test: `ClientContract.nonpositive_budgets_are_clamped_to_one_attempt`, `ClientContract.requests_from_distinct_objects_have_distinct_sequences_and_ignore_stale_replies`, `IpcClient.netClient_honours_configured_budget_by_wall_clock`.
/// - w netClient wysyłać komendę z argumentem, pid procesu i numerem żądania unikalnym w obrębie procesu, także między kilkoma obiektami IpcClient, i odbierać odpowiedź wyłącznie ze slotu tego procesu (pid, czas startu procesu) i tego numeru żądania.
///   Test: `ClientContract.requests_from_distinct_objects_have_distinct_sequences_and_ignore_stale_replies`.
/// - wyznaczać jeden termin zegarem monotonicznym jako chwilę rozpoczęcia żądania plus max(1, budżet prób) razy interwał odpytywania i czekać na odpowiedź po wysłaniu komendy tylko do tego terminu, bez jego odnawiania. Termin ogranicza oczekiwanie, nie czas operacji systemowych ani opóźnienie planisty.
///   Test: `IpcClient.netClient_honours_configured_budget_by_wall_clock`, `IpcClient.netClient_default_budget_survives_briefly_starved_server`. Nietestowalne: wspolny termin obu czekan jest kontrola strukturalna netClient w ipcClient.cpp (jeden steady_clock deadline dla timed_send i take), nie pomiarem dwoch niezaleznych budzetow.
/// - przy pełnej kolejce komend odmawiać od razu, bez czekania na miejsce do terminu i bez nadpisywania komend już czekających w kolejce: timed_send Boosta interpretuje termin zegara monotonicznego jako czas bezwzględny CLOCK_REALTIME, który dawno minął (znany defekt, #522).
///   Test: `ClientContract.full_command_queue_is_refused_before_the_deadline_without_overwriting_the_old_command`.
/// - zwracać odpowiedź z error.response równym server not found i ustawiać done przy niezgodnym układzie segmentu odpowiedzi (przed wysłaniem komendy), przy pełnej kolejce komend albo gdy odpowiedź nie nadejdzie przed terminem.
///   Test: `ClientContract.incompatible_layout_is_refused_before_sending`, `ClientContract.full_command_queue_is_refused_before_the_deadline_without_overwriting_the_old_command`, `ClientContract.nonpositive_budgets_are_clamped_to_one_attempt`, `IpcClient.netClient_honours_configured_budget_by_wall_clock`.
/// - przy błędzie systemowym IPC lub innym wyjątku ustawiać done i przekazywać wyjątek wołającemu.
///   Test: `ClientContract.missing_command_queue_propagates_ipc_exception_and_sets_done`, `ClientContract.malformed_response_propagates_parser_exception_and_sets_done`, `IpcSecurity.ClientRejectsForeignCommandQueueWithoutSending`.
/// - w producer ponawiać otwarcie własnej kolejki odpowiedzi do wyczerpania budżetu prób albo ustawienia done, a odmowę zaufania obiektowi (ipc::security::Rejected) traktować jako brak kolejki bez ponawiania.
///   Test: `IpcClient.producer_waits_for_late_response_queue`, `IpcClient.producer_reports_missing_queue_distinctly_from_normal_end`, `IpcSecurity.ProducerRejectsForeignSubscriptionWithoutReceiving`.
/// - ustawiać responseQueueMissing wyłącznie po odmowie zaufania albo po wyczerpaniu budżetu prób otwarcia, a nie po przerwaniu oczekiwania przez wątek główny, i w każdym z tych przypadków kończyć producenta z ustawionym done.
///   Test: `ClientContract.stopping_before_queue_open_does_not_blame_the_server`, `IpcClient.producer_reports_missing_queue_distinctly_from_normal_end`, `IpcSecurity.ProducerRejectsForeignSubscriptionWithoutReceiving`.
/// - odbierać komunikaty kolejki odpowiedzi, czekając interwał odpytywania wyłącznie przy pustej kolejce, parsować je jako dokument info i przekazywać do kolejki SPSC o pojemności kSpscQueueCapacity, czekając na miejsce bez gubienia elementów, dopóki done nie jest ustawione.
///   Test: `ClientContract.producer_preserves_fifo_across_spsc_backpressure`, `IpcClient.full_consumer_queue_does_not_prevent_shutdown`. Nietestowalne: brak snu po udanym odbiorze jest kontrola strukturalna producer w ipcClient.cpp; nie mierzymy szybkosci planisty.
/// - przez popQueue zwracać kolejny element odebrany przez producenta bez blokowania.
///   Test: `ClientContract.producer_preserves_fifo_across_spsc_backpressure`, `IpcClient.popQueue_returns_false_on_empty`, `IpcClient.popQueue_does_not_modify_pt_on_empty`.
/// - kończyć pętlę producenta po ustawieniu done przez dowolny wątek, a przy wyjątku w odbiorze zapisywać diagnostykę i ustawiać done.
///   Test: `IpcClient.full_consumer_queue_does_not_prevent_shutdown`, `ClientContract.malformed_stream_row_stops_producer_and_logs_the_cause`.
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
