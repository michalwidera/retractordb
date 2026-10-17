#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <thread>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/property_tree/ptree.hpp>

#include "constants.hpp"

/// Transport IPC strony serwerowej: segment pamieci dzielonej ze slotami odpowiedzi,
/// kolejka komend, kolejki rozgloszeniowe per klient oraz watek komunikacyjny.
///
/// Klasa nie zna qTree, dataModel ani compilera. Warstwa protokolu (rozpoznanie
/// komendy, kompilacja ad hoc, formatowanie wiersza) wchodzi wylacznie przez
/// wywolania zwrotne przekazane do start() i broadcast().
///
/// Jeden proces prowadzi jedna instancje. Nie ma tu RAII na zasobach IPC:
/// sciezka FatalError konczy proces przez std::exit, ktory NIE uruchamia
/// destruktorow, wiec sprzatanie musi byc osiagalne z handlera atexit --
/// patrz shutdownFromExitHandler().
/// Obiekt klasy IpcServer powinien:
/// - wyznaczać nazwy wszystkich swoich obiektów IPC z nazwy serwera ustawionej przed start i używać wyłącznie ich przy tworzeniu, emisji i usuwaniu, tak aby nie usunąć obiektów innego serwera.
///   Test: `IpcServerQueues.servers_with_distinct_names_have_disjoint_queues`, `IpcServerQueues.exit_handler_does_not_touch_other_servers_segment`; integracja `it_ipc_identity_lock`.
/// - w start uruchamiać wątek komunikacyjny, który usuwa pozostałości własnych obiektów tylko po kontroli zaufania, tworzy segment odpowiedzi i kolejkę komend, a następnie wywołuje onReady albo, gdy budowa zasobów kończy się std::exception, onFailure, dokładnie jedno z nich. Warunek kontraktu: wołający przekazuje oba callbacki i żaden z nich nie rzuca; wyjście przez sygnał, std::exit albo wyjątek spoza std::exception nie jest objęte tą gwarancją.
///   Test: `IpcServerLoop.full_size_command_is_received_whole_and_server_keeps_serving`; integracja `it_ipc_security-lifecycle`. Rzucajace onReady jest poza kontraktem (proba diagnostyczna w #521: wywoluje wtedy takze onFailure).
/// - odbierać komendy z kolejki do chwili, gdy shouldStop zwróci prawdę, odrzucać z wpisem w logu komendę bez poprawnego db.id, przekazywać pozostałe do onCommand, po obsłudze wywoływać onCommandHandled i publikować odpowiedź w slocie adresata (pid, czas startu procesu, db.seq).
///   Test: `IpcServerLoop.full_size_command_is_received_whole_and_server_keeps_serving`, `IpcServerLoop.command_without_client_id_is_rejected_and_server_keeps_serving`, `ClientContract.requests_from_distinct_objects_have_distinct_sequences_and_ignore_stale_replies`; kolejnosc onCommandHandled sprawdza integracja `it_xqrywait_first_row-run`.
/// - zamieniać wyjątek z onCommand dziedziczący po std::exception na odpowiedź error.response bez przerywania wątku; std::exception z pozostałej obsługi pojedynczego komunikatu (np. parsowania lub serializacji) kończyć jego odrzuceniem z wpisem w logu. Funkcje zwrotne mogą zgłaszać błędy tylko przez std::exception, z wyjątkiem onReady i onFailure, które nie mogą rzucać; przechwytywanie wyjątków innych typów nie jest gwarantowane.
///   Test: `IpcServerLoop.handler_exception_is_answered_and_the_next_command_is_served`, `IpcServerLoop.garbage_command_is_rejected_and_server_keeps_serving`.
/// - odtwarzać kolejkę komend porzuconą przez proces zmarły z jej blokadą (not_recoverable) zamiast kończyć obsługę komend.
///   Test: `IpcServerLoop.command_queue_abandoned_by_dead_client_is_recreated` (wykonanie wymaga wykrywalnej smierci wlasciciela muteksu Boosta).
/// - w subscribe sprawdzać przed utworzeniem kolejki klienta, czy pamięć dzielona ma na nią miejsce, zgłaszając brak wyjątkiem z wyceną, a następnie zastępować poprzednią kolejkę klienta nową i rejestrować go na strumieniu wyłącznie wtedy, gdy epoka rejestru nie zmieniła się od expectedEpoch, w przeciwnym razie usuwać utworzoną kolejkę i zwracać fałsz.
///   Test: `IpcServerQueues.insufficient_space_refuses_subscription_before_queue_creation`, `IpcSecurity.ResubscriptionRecreatesOwnQueueWithRequestedCapacity`, `IpcServerQueues.epoch_can_close_while_queue_creation_is_paused`, `IpcServerQueues.subscribe_after_out_of_business_is_refused_without_queue`, `IpcServerQueues.subscribe_after_queue_removal_is_refused_without_queue`; integracja `it_subscribe_epoch_race-run`.
/// - nie trzymać blokady map klienckich podczas tworzenia kolejki klienta.
///   Test: `IpcServerQueues.epoch_can_close_while_queue_creation_is_paused` (backend POSIX); kontrola strukturalna subscribe w ipcServer.cpp dla backendu plikowego, niewykonana na Darwinie.
/// - w broadcast przetwarzać span streams w jego kolejności, bez deduplikacji nazw; dla każdego wystąpienia nazwy formatować wiersz najwyżej raz i tylko gdy strumień nadal ma subskrybenta, współdzielić ten wiersz między jego subskrybentami i wysyłać go bez czekania na miejsce w kolejce. Powtórzona nazwa oznacza kolejną emisję, o ile pozostali subskrybenci; klienta z pełną lub porzuconą kolejką wyrejestrowywać i usuwać jego kolejkę.
///   Test: `IpcServerQueues.broadcast_formats_once_for_subscribers_and_drops_only_the_full_queue`, `IpcServerQueues.broadcast_drops_queue_abandoned_by_dead_client`. Ograniczenie weryfikacji: kolejnosc i powtorzenia nazw potwierdza kontrola strukturalna petli broadcast w ipcServer.cpp; brak osobnej asercji duplikatow.
/// - pomijać emisję wiersza dłuższego niż slot kolejki odpowiedzi, zgłaszając to w logu jeden raz na strumień, bez wyjątku.
///   Test: `IpcServerQueues.oversized_rows_are_skipped_and_reported_once_per_stream`.
/// - w broadcastOutOfBusiness podejmować dla każdego subskrybenta jedną próbę wysłania znacznika końca pracy, bez czekania na miejsce w kolejce i bez ponawiania, a po próbie zakończonej bez wyjątku usuwać jego kolejkę przez removeOwned, także gdy pełna kolejka znacznika nie przyjęła.
///   Test: `IpcServerQueues.out_of_business_sends_marker_and_closes_the_subscription_epoch`, `IpcServerQueues.out_of_business_to_full_queue_drops_the_marker_and_removes_the_queue`.
/// - przy std::exception z wysyłki (np. kolejka porzucona przez zmarły proces z jej blokadą) zapisywać ostrzeżenie w logu i pomijać usunięcie tej kolejki, która zostaje bez wpisu w rejestrze (znany defekt, #523).
///   Test: `IpcServerQueues.out_of_business_leaves_queue_abandoned_by_dead_client` (wykonanie wymaga wykrywalnej smierci wlasciciela muteksu Boosta).
/// - po przejściu wszystkich subskrybentów czyścić rejestr i zwiększać epokę rejestru dokładnie raz; removeClientQueues zwiększa ją tak samo.
///   Test: `IpcServerQueues.out_of_business_sends_marker_and_closes_the_subscription_epoch`, `IpcServerQueues.out_of_business_leaves_queue_abandoned_by_dead_client`, `IpcServerQueues.subscribe_after_queue_removal_is_refused_without_queue`.
/// - w stop dołączać wątek komunikacyjny, a w removeAllObjects usuwać segment odpowiedzi, kolejkę komend i kolejki wszystkich zarejestrowanych klientów.
///   Test: `IpcServerQueues.removeAllObjects_removes_every_client_queue`, `IpcServerQueues.removal_is_idempotent`, `IpcServerLoop.exception_after_ready_stops_thread_without_announcing_startup_failure`; integracja `it_ipc_security-lifecycle`.
/// - w shutdownFromExitHandler odpinać wątek komunikacyjny, gdy wywołanie pochodzi z niego samego, w przeciwnym razie czekać na jego zakończenie najwyżej 2 s, a następnie usuwać obiekty globalne i kolejki klientów, te ostatnie tylko gdy blokadę map da się wziąć bez czekania.
///   Test: `IpcServerQueues.exit_handler_removes_client_queues_too`, `IpcServerQueues.exit_handler_removes_response_segment`, `IpcServerQueues.exit_handler_does_not_wait_for_the_broadcast_map_lock`; integracja `it_fatal_exit_path-run` (proces potomny i limit czasu). Nietestowalne: dokladne 2 s i detach biezacego watku - kontrola strukturalna shutdownFromExitHandler w ipcServer.cpp, bez asercji dokladnego czasu planisty.
/// - po poprawnym powrocie z onReady kończyć wątek komunikacyjny z wpisem w logu przy std::exception nieobsłużonym wewnątrz pętli, bez ponownego wywołania onReady ani onFailure. Wyjątki spoza std::exception są poza kontraktem i nie mają gwarancji zatrzymania ograniczonego do tego wątku.
///   Test: `IpcServerLoop.exception_after_ready_stops_thread_without_announcing_startup_failure`.
/// - uniemożliwiać kopiowanie obiektu.
///   Sprawdzenie kompilacji: test/UnitTest/test_ipcServer.cpp, static_assert cech kopiowania IpcServer.
class IpcServer {
 public:
  using ptree = boost::property_tree::ptree;

  /// Obsluga jednej komendy: ptree wejsciowy -> ptree odpowiedzi.
  using CommandHandler = std::function<ptree(const ptree &)>;
  /// Sygnal bez argumentow do warstwy sterujacej petla przetwarzania.
  using Notifier = std::function<void()>;
  /// Predykat zatrzymania petli komunikacyjnej.
  using StopPredicate = std::function<bool()>;
  /// Formatowanie wiersza strumienia do postaci wysylanej klientom.
  using RowFormatter = std::function<std::string(const std::string &)>;

  /// @requirements-exempt Callbacks: Zestaw funkcji zwrotnych przekazywanych serwerowi IPC.
  struct Callbacks {
    CommandHandler onCommand;
    Notifier onReady;           // zasoby IPC gotowe -- raz, przed petla odbioru
    Notifier onFailure;         // zasobow IPC nie da sie zbudowac -- watek konczy prace
    Notifier onCommandHandled;  // komenda OBSLUZONA -- po powrocie z onCommand
    StopPredicate shouldStop;
  };

  /// Wolajacy przekazuje onReady i onFailure, oba nierzucajace. W tym kontrakcie zwykly start
  /// albo blad budowy zasobow typu std::exception wywoluje dokladnie jedno z nich.
  /// Wyjatek z callbacku narusza kontrakt; gwarancja nie obejmuje przerwania procesu. Bez onFailure
  /// zawiedziona budowa zasobow (najczesciej brak miejsca w /dev/shm na kolejke komend) konczyla watek po cichu, a strona
  /// czekajaca na gotowosc czekala bez konca.

  IpcServer()                             = default;
  IpcServer(const IpcServer &)            = delete;
  IpcServer &operator=(const IpcServer &) = delete;

  /// Nazwa serwera wyznaczajaca komplet nazw obiektow IPC tej instancji. Wolac PRZED start();
  /// pozniejsza zmiana rozjechalaby nazwy juz utworzonych obiektow z nazwami uzywanymi do ich
  /// kasowania. Nazwa pusta (domyslna) daje nazwy historyczne, jednoserwerowe.
  void setServerName(std::string_view serverName);

  /// Startuje watek komunikacyjny. onReady wola sie z tego watku, gdy segment
  /// odpowiedzi i kolejka komend juz istnieja.
  void start(Callbacks callbacks);

  /// Normalne zamkniecie: dolacza watek komunikacyjny.
  void stop();

  /// Sciezka atexit/FatalError: odpina lub dolacza watek i kasuje obiekty IPC.
  /// Kasuje ten sam zestaw co removeAllObjects(), z jednym zastrzezeniem: kolejki
  /// klientow tylko wtedy, gdy uda sie wziac clientMapsMutex_ bez czekania (patrz .cpp).
  void shutdownFromExitHandler();

  /// Uchwyt watku komunikacyjnego dla rtKeepThreadOffRtCpus. Wolac po start().
  std::thread::native_handle_type threadHandle();

  /// Epoka rejestru subskrypcji: rosnie przy kazdym jego zamknieciu (broadcastOutOfBusiness,
  /// removeClientQueues). Wolajacy odczytuje ja razem z parametrami subskrypcji, poki trzyma
  /// model, i przekazuje do subscribe() -- patrz subscriptionEpoch_.
  [[nodiscard]] std::uint64_t subscriptionEpoch() const { return subscriptionEpoch_.load(std::memory_order_acquire); }

  /// Tworzy kolejke odpowiedzi klienta i rejestruje go na strumieniu, jesli rejestr nie zostal
  /// zamkniety od odczytu @p expectedEpoch. Falsz = odmowa: kolejka usunieta, rejestr nietkniety.
  bool subscribe(int clientId, const std::string &streamName, int maxElements, std::uint64_t expectedEpoch);

  /// Rozsyla biezacy wiersz kazdego z podanych strumieni do jego subskrybentow.
  /// Kazde wystapienie nazwy jest osobna emisja; span nie jest deduplikowany.
  ///
  /// Nazwy przychodza jako WIDOKI na napisy wolajacego (u serwera: identyfikatory zywego planu)
  /// i musza zyc przez cale wywolanie. Serwer nie zapamietuje zadnego z nich; jedyny, ktory tu
  /// zostaje - nazwa strumienia o zbyt dlugim wierszu - trafia do oversizedRowStreams_ jako
  /// kopia. Emisja bierze zbior taki, jaki wyznaczyl takt: bez odtwarzania go z napisow.
  void broadcast(std::span<const std::string_view> streams, const RowFormatter &formatRow);

  /// Probuje zawiadomic klientow o koncu pracy bez czekania na miejsce w kolejce i czysci rejestr.
  /// Sprzatanie kolejek, takze znany defekt po wyjatku wysylki (#523): wymagania powyzej.
  void broadcastOutOfBusiness();

  /// Kasuje wszystkie obiekty IPC serwera: segment odpowiedzi, kolejke komend
  /// i kolejki odpowiedzi pozostalych klientow. Wolac po stop().
  void removeAllObjects();

 private:
  void commandLoop() const;

  /// Segment odpowiedzi i kolejka komend. Nie dotyka stanu klientow.
  void removeGlobalObjects() const;

  /// Kolejki odpowiedzi klientow plus wyczyszczenie rejestru subskrypcji.
  /// Wolajacy MUSI trzymac clientMapsMutex_.
  void removeClientQueues();

  /// Czy wiersz miesci sie w slocie kolejki odpowiedzi. Falsz melduje przyczyne raz na
  /// strumien i jest rownoznaczny z pominieciem emisji -- nigdy z wyjatkiem, bo wyjatek
  /// z try_send konczyl cala usluge. Wolajacy MUSI trzymac clientMapsMutex_.
  bool rowFitsSlot(const std::string &row, std::string_view streamName);

  /// try_send, ktore kolejke z muteksem porzuconym przez martwy proces traktuje jak przepelniona
  /// (falsz) zamiast rzucac. Pozostale bledy IPC przechodza dalej bez zmian.
  static bool trySendOrAbandoned(boost::interprocess::message_queue &queue, const std::string &row, int clientId);

  Callbacks callbacks_;
  std::thread commsThread_;
  /// Watek komunikacyjny wyszedl z commandLoop(). shutdownFromExitHandler() czeka na to
  /// z limitem, zamiast dolaczac bez konca watek, ktory moze stac na blokadzie epoki.
  std::atomic<bool> commsFinished_{false};

  // Komplet nazw obiektow IPC tej instancji. Wszystkie sciezki -- tworzenie, emisja i KASOWANIE
  // -- czytaja nazwy stad, nigdy ze stalych globalnych; to jest warunek tego, zeby serwer nie
  // mogl skasowac obiektow innego serwera.
  ipc::ServerNames names_{ipc::names()};

  // Mapa relacji processId -> nazwa wysylanego strumienia.
  std::map<const int, std::string> id2StreamNameRelation_;

  // Cache otwartych kolejek IPC per klient. Konstrukcja message_queue(open_only)
  // to shm_open+mmap -- wykonywana w kazdym slocie kosztowala ~3,2 ms/slot na
  // Pi 400 (zmierzone, JOURNAL.md kampania 7bis) i lamala os czasu 360 Hz.
  // Uchwyt jest wazny dopoki kolejka istnieje: otwieramy przy pierwszej emisji,
  // usuwamy z cache przy usunieciu kolejki (przepelnienie) i re-rejestracji
  // klienta.
  std::map<const int, std::unique_ptr<boost::interprocess::message_queue>> id2QueueCache_;

  // Strumienie, o ktorych zbyt dlugim wierszu juz zameldowalismy. Kryterium jest per strumien,
  // nie per slot: wiersz nie miesci sie w kazdym slocie tak samo, a przy takcie 1 ms ten sam
  // komunikat zalalby dziennik tysiac razy na sekunde. Czytane i pisane wylacznie w broadcast(),
  // czyli pod clientMapsMutex_.
  std::set<std::string> oversizedRowStreams_;

  // Muteks obu map klienckich (sledztwo ~40 ms, JOURNAL.md 2026-07-18, Faza 3):
  // mapy sa modyfikowane przez watek komunikacyjny (rejestracja show) i czytane/
  // czyszczone przez watek przetwarzajacy (broadcast). Muteks trzymany wylacznie
  // na operacjach na mapach -- NIGDY podczas konstrukcji kolejki (mmap ~MB), aby
  // nie wnosic inwersji priorytetow do watku RT.
  std::mutex clientMapsMutex_;

  // Licznik zamkniec rejestru subskrypcji. subscribe() buduje kolejke bez blokady epoki planu
  // (#283), wiec miedzy odczytem parametrow strumienia a rejestracja epoka moze sie skonczyc:
  // broadcastOutOfBusiness wysyla OOB i czysci mapy, a spozniona rejestracja przezylaby to bez
  // OOB -- przy --reset na strumien nowego planu o tej samej nazwie. Pisany pod clientMapsMutex_,
  // tam tez porownywany przy rejestracji; czytany bez muteksu przez subscriptionEpoch().
  std::atomic<std::uint64_t> subscriptionEpoch_{0};
};
