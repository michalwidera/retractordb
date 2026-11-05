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
#include <utility>
#include <vector>

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
/// - [ipcserver.ipc-server.1~1] wyznaczać nazwy wszystkich swoich obiektów IPC z nazwy serwera ustawionej przed start i używać wyłącznie ich przy tworzeniu, emisji i usuwaniu, tak aby nie usunąć obiektów innego serwera.
///   Zakres testow: `IpcServerQueues.servers_with_distinct_names_have_disjoint_queues`, `IpcServerQueues.exit_handler_does_not_touch_other_servers_segment`; integracja `it_ipc_identity_lock`.
/// - [ipcserver.ipc-server.2~1] w start uruchamiać wątek komunikacyjny, który usuwa pozostałości własnych obiektów tylko po kontroli zaufania, tworzy segment odpowiedzi i kolejkę komend, a następnie wywołuje onReady albo, gdy budowa zasobów kończy się std::exception, onFailure, dokładnie jedno z nich. Warunek kontraktu: wołający przekazuje oba callbacki i żaden z nich nie rzuca; wyjście przez sygnał, std::exit albo wyjątek spoza std::exception nie jest objęte tą gwarancją.
///   Zakres testow: `IpcServerLoop.full_size_command_is_received_whole_and_server_keeps_serving`; integracja `it_ipc_security-lifecycle`. Rzucajace onReady jest poza kontraktem (proba diagnostyczna w #521: wywoluje wtedy takze onFailure).
/// - [ipcserver.ipc-server.3~2] odbierać komendy z kolejki do chwili, gdy shouldStop zwróci prawdę, odrzucać z wpisem w logu komendę bez poprawnego db.id, przekazywać pozostałe do onCommand, po obsłudze wywoływać onCommandHandled i publikować odpowiedź w slocie adresata (pid, czas startu procesu, db.seq), biorąc czas startu z db.start, a gdy go brak, jest niepoprawny albo równy 0 - odczytując go z wpisu procesu adresata.
///   Zakres testow: `IpcServerLoop.full_size_command_is_received_whole_and_server_keeps_serving`, `IpcServerLoop.command_without_client_id_is_rejected_and_server_keeps_serving`, `IpcServerLoop.response_slot_takes_client_start_time_from_command`, `ClientContract.requests_from_distinct_objects_have_distinct_sequences_and_ignore_stale_replies`; kolejnosc onCommandHandled sprawdza integracja `it_xqrywait_first_row-run`.
/// - [ipcserver.ipc-server.4~1] zamieniać wyjątek z onCommand dziedziczący po std::exception na odpowiedź error.response bez przerywania wątku; std::exception z pozostałej obsługi pojedynczego komunikatu (np. parsowania lub serializacji) kończyć jego odrzuceniem z wpisem w logu. Funkcje zwrotne mogą zgłaszać błędy tylko przez std::exception, z wyjątkiem onReady i onFailure, które nie mogą rzucać; przechwytywanie wyjątków innych typów nie jest gwarantowane.
/// - [ipcserver.ipc-server.5~1] odtwarzać kolejkę komend porzuconą przez proces zmarły z jej blokadą (not_recoverable) zamiast kończyć obsługę komend.
///   Zakres testow: `IpcServerLoop.command_queue_abandoned_by_dead_client_is_recreated` (wykonanie wymaga wykrywalnej smierci wlasciciela muteksu Boosta).
/// - [ipcserver.ipc-server.6~1] w subscribe sprawdzać przed utworzeniem kolejki klienta, czy pamięć dzielona ma na nią miejsce, zgłaszając brak wyjątkiem z wyceną, a następnie zastępować poprzednią kolejkę klienta nową i rejestrować go na strumieniu wyłącznie wtedy, gdy epoka rejestru nie zmieniła się od expectedEpoch, w przeciwnym razie usuwać utworzoną kolejkę i zwracać fałsz.
///   Zakres testow: `IpcServerQueues.insufficient_space_refuses_subscription_before_queue_creation`, `IpcSecurity.ResubscriptionRecreatesOwnQueueWithRequestedCapacity`, `IpcServerQueues.epoch_can_close_while_queue_creation_is_paused`, `IpcServerQueues.subscribe_after_out_of_business_is_refused_without_queue`, `IpcServerQueues.subscribe_after_queue_removal_is_refused_without_queue`; integracja `it_subscribe_epoch_race-run`.
/// - [ipcserver.ipc-server.7~1] nie trzymać blokady map klienckich podczas tworzenia kolejki klienta.
///   Zakres testow: `IpcServerQueues.epoch_can_close_while_queue_creation_is_paused` (backend POSIX); kontrola strukturalna subscribe w ipcServer.cpp dla backendu plikowego, niewykonana na Darwinie.
/// - [ipcserver.ipc-server.8~2] w broadcast przetwarzać span streams w jego kolejności, bez deduplikacji nazw; dla każdego wystąpienia nazwy formatować wiersz najwyżej raz i tylko gdy strumień nadal ma subskrybenta, współdzielić ten wiersz między jego subskrybentami i wysyłać go bez czekania na miejsce w kolejce. Powtórzona nazwa oznacza kolejną emisję, o ile pozostali subskrybenci; klienta z pełną lub porzuconą kolejką wyrejestrowywać od razu, a jego kolejkę przekazywać do odroczonego zwolnienia.
///   Ograniczenie weryfikacji: kolejnosc i powtorzenia nazw potwierdza kontrola strukturalna petli broadcast w ipcServer.cpp; brak osobnej asercji duplikatow.
/// - [ipcserver.ipc-server.9~1] pomijać emisję wiersza dłuższego niż slot kolejki odpowiedzi, zgłaszając to w logu jeden raz na strumień, bez wyjątku.
/// - [ipcserver.ipc-server.10~1] w broadcastOutOfBusiness podejmować dla każdego subskrybenta jedną próbę wysłania znacznika końca pracy, bez czekania na miejsce w kolejce i bez ponawiania; pełna kolejka znacznika nie dostaje.
/// - [ipcserver.ipc-server.11~1] po próbie wysłania usuwać kolejkę każdego subskrybenta przez removeOwned niezależnie od jej wyniku, także gdy wysyłka zgłosiła std::exception (np. kolejka porzucona przez zmarły proces z jej blokadą); błąd wysyłki zapisywać ostrzeżeniem w logu.
///   Zakres testow: `IpcServerQueues.out_of_business_to_full_queue_drops_the_marker_and_removes_the_queue`, `IpcServerQueues.out_of_business_removes_queue_abandoned_by_dead_client` (wykonanie wymaga wykrywalnej smierci wlasciciela muteksu Boosta).
/// - [ipcserver.ipc-server.12~1] po przejściu wszystkich subskrybentów czyścić rejestr i zwiększać epokę rejestru dokładnie raz; removeClientQueues zwiększa ją tak samo.
/// - [ipcserver.ipc-server.13~1] w stop dołączać wątek komunikacyjny, a w removeAllObjects usuwać segment odpowiedzi, kolejkę komend i kolejki wszystkich zarejestrowanych klientów.
///   Zakres testow: `IpcServerQueues.removeAllObjects_removes_every_client_queue`, `IpcServerQueues.removal_is_idempotent`, `IpcServerLoop.exception_after_ready_stops_thread_without_announcing_startup_failure`; integracja `it_ipc_security-lifecycle`.
/// - [ipcserver.ipc-server.14~1] w shutdownFromExitHandler odpinać wątek komunikacyjny, gdy wywołanie pochodzi z niego samego, w przeciwnym razie czekać na jego zakończenie najwyżej 2 s, a następnie usuwać obiekty globalne i kolejki klientów, te ostatnie tylko gdy blokadę map da się wziąć bez czekania.
///   Zakres testow: `IpcServerQueues.exit_handler_removes_client_queues_too`, `IpcServerQueues.exit_handler_removes_response_segment`, `IpcServerQueues.exit_handler_does_not_wait_for_the_broadcast_map_lock`; integracja `it_fatal_exit_path-run` (proces potomny i limit czasu). Nietestowalne: dokladne 2 s i detach biezacego watku - kontrola strukturalna shutdownFromExitHandler w ipcServer.cpp, bez asercji dokladnego czasu planisty.
/// - [ipcserver.ipc-server.15~1] po poprawnym powrocie z onReady kończyć wątek komunikacyjny z wpisem w logu przy std::exception nieobsłużonym wewnątrz pętli, bez ponownego wywołania onReady ani onFailure. Wyjątki spoza std::exception są poza kontraktem i nie mają gwarancji zatrzymania ograniczonego do tego wątku.
/// - [ipcserver.ipc-server.16~1] uniemożliwiać kopiowanie obiektu.
///   Sprawdzenie kompilacji: test/UnitTest/test_ipcServer.cpp, static_assert cech kopiowania IpcServer.
/// - [ipcserver.ipc-server.17~1] odrzucać z wpisem w logu, przed parserem INFO i bez wywołania onCommand, komendę zawierającą linię, której pierwszy znak inny niż biały to '#' (dyrektywa parsera, np. #include otwierająca plik), tak aby treść komendy nie otwierała plików z prawami serwera.
/// - [ipcserver.ipc-server.18~1] w broadcast nie zamykać mapowania ani nie usuwać nazwy porzuconej kolejki, tylko przenosić jej uchwyt na listę do zwolnienia, tak aby koszt munmap i unlink (do ~50 ms przy kolejce 64 MiB) nie trafiał do slotu.
/// - [ipcserver.ipc-server.19~2] w releaseAbandonedQueues, wołanym przez wątek komunikacyjny w każdym obrocie pętli odbioru, zabierać listę do zwolnienia pod blokadą map i poza nią każdą kolejkę oddawać do puli albo, gdy się to nie uda, zamykać jej mapowanie, a potem usuwać jej nazwę; przy pustej liście nie brać blokady map.
/// - [ipcserver.ipc-server.20~1] w subscribe przejmować zaległe zwolnienie kolejki tego samego klienta pod tym samym zajęciem blokady map, pod którym wypisuje go z rejestru, tak aby późniejsze zwolnienie listy nie usunęło nazwy nowej kolejki.
/// - [ipcserver.ipc-server.21~1] w broadcastOutOfBusiness i removeClientQueues (a przez nią w removeAllObjects i shutdownFromExitHandler) zwalniać także wszystkie kolejki z listy do zwolnienia.
/// - [ipcserver.ipc-server.22~1] w releaseAbandonedQueues oddawać porzuconą kolejkę do puli zamiast ją zwalniać wyłącznie wtedy, gdy jądro potwierdziło brak procesu klienta (inspectProcess: found i unreadable równe false), odebranie przez try_receive na uchwycie serwera opróżniło ją bez błędu i renameOwned przeniósł jej nazwę na wolną nazwę responsePool; pula ma najwyżej kResponseQueuePoolSegments wpisów, a pełna zwalnia przed zmianą nazwy swój najstarszy wpis. W każdym innym przypadku zwalniać kolejkę jak dotąd.
/// - [ipcserver.ipc-server.23~1] w subscribe brać z puli kolejkę o liczbie elementów równej maxElements: przenosić przez renameOwned jej nazwę na nazwę kolejki klienta, a jej uchwyt do rejestru, bez tworzenia kolejki i bez sprawdzania miejsca; gdy zmiana nazwy się nie uda, zwalniać ten wpis puli i tworzyć kolejkę jak bez puli.
/// - [ipcserver.ipc-server.24~1] w subscribe przy braku miejsca na nową kolejkę zwalniać bezczynne kolejki puli od najstarszej, dopóki miejsca nie wystarczy albo pula się nie opróżni, i dopiero wtedy zgłaszać brak; kolejki odmówionej przez epokę, zastąpionej przy ponownej subskrypcji albo przejętej z listy do zwolnienia tego samego klienta nie oddawać do puli.
/// - [ipcserver.ipc-server.25~1] w removeAllObjects zamykać mapowania puli i usuwać jej nazwy, a w shutdownFromExitHandler i na starcie wątku komunikacyjnego usuwać nazwy responsePool(0..kResponseQueuePoolSegments-1) bez dotykania struktury puli.
/// - [ipcserver.ipc-server.26~1] obsługiwać pulę wyłącznie z wątku komunikacyjnego (subscribe, releaseAbandonedQueues) i z removeAllObjects po stop, bez blokady map.
///   Kontrola strukturalna: subscribe wolaja tylko commandProcessor w watku komunikacyjnym i testy jednostkowe; broadcast i broadcastOutOfBusiness nie siegaja do responsePool_.
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
  /// Kolejke kazdego subskrybenta usuwa takze po nieudanej wysylce: wymagania powyzej.
  void broadcastOutOfBusiness();

  /// Zwalnia kolejki porzucone w broadcast(): zamyka mapowanie, potem usuwa nazwe. Wola go
  /// watek komunikacyjny; pusta lista kosztuje jeden odczyt atomowy, bez blokady map.
  void releaseAbandonedQueues();

  /// Kasuje wszystkie obiekty IPC serwera: segment odpowiedzi, kolejke komend
  /// i kolejki odpowiedzi pozostalych klientow. Wolac po stop().
  void removeAllObjects();

 private:
  void commandLoop();

  /// Segment odpowiedzi i kolejka komend. Nie dotyka stanu klientow.
  void removeGlobalObjects() const;

  /// Kolejki odpowiedzi klientow plus wyczyszczenie rejestru subskrypcji.
  /// Wolajacy MUSI trzymac clientMapsMutex_.
  void removeClientQueues();

  /// Porzucona kolejka do puli (#603): oprozniona, pod nazwa responsePool. Falsz = kolejka nadal
  /// pod nazwa klienta, z uchwytem w @p queue, i wolajacy zwalnia ja jak dotad.
  bool poolAbandonedQueue(int clientId, std::unique_ptr<boost::interprocess::message_queue> &queue);

  /// Kolejka z puli o pojemnosci @p maxElements, juz pod nazwa @p queueName, albo nullptr.
  std::unique_ptr<boost::interprocess::message_queue> takePooledQueue(int maxElements, const std::string &queueName);

  /// Oproznienie kolejki przez jej uchwyt. Falsz = kolejka nie do ponownego uzycia (np. not_recoverable).
  static bool drainQueue(boost::interprocess::message_queue &queue, int clientId);

  /// Zwolnienie najstarszego wpisu puli: munmap, potem unlink. Pula nie moze byc pusta.
  void releaseOldestPooled();

  /// Czy pula ma kolejke o pojemnosci @p maxElements.
  [[nodiscard]] bool poolHas(int maxElements) const;

  /// Nazwy wszystkich miejsc puli, bez dotykania responsePool_ (sciezka atexit, start watku).
  void removePoolNames() const;

  /// Synchroniczne zwolnienie calej listy pendingRelease_. Wolajacy MUSI trzymac
  /// clientMapsMutex_; tylko dla sciezek poza slotem (koniec epoki, zamkniecie).
  void releasePendingLocked();

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

  // Kolejki porzucone w broadcast(), czekajace na zwolnienie w watku komunikacyjnym (#600).
  // munmap i unlink segmentu kosztowaly w slocie do ~50 ms (kolejka 64 MiB pod mlockall).
  // Niezmiennik: klient jest albo w mapach powyzej, albo tutaj, nigdy w obu - broadcast
  // przenosi go stad i stamtad pod jednym zajeciem muteksu, a subscribe() zabiera stad
  // wpis swojego klienta, zanim utworzy nowa kolejke o tej samej nazwie. Dzieki temu
  // usuniecie nazwy przy zwolnieniu nigdy nie trafia w kolejke nowego klienta.
  // Pisane pod clientMapsMutex_.
  std::vector<std::pair<int, std::unique_ptr<boost::interprocess::message_queue>>> pendingRelease_;
  // Czy pendingRelease_ ma wpisy. Watek komunikacyjny budzi sie co kQueuePollInterval
  // i bez tej flagi bralby muteks map w kazdym obrocie - std::mutex nie dziedziczy
  // priorytetu, wiec kazde zajecie przez watek spoza RT to okazja do inwersji wobec slotu.
  // Ustawiana i zerowana pod clientMapsMutex_, czytana bez niego.
  std::atomic<bool> releasePending_{false};

  // Pula bezczynnych kolejek odpowiedzi (#603), najstarsza na poczatku. Zwolnienie kolejki w czasie
  // pracy to munmap calego segmentu (na Cortex-A72 od 2 MiB uniewaznia TLB calego procesu, takze
  // na rdzeniu slotu) i unlink, ktory oddaje jego strony jadru; obu towarzyszyl dluzszy slot (R603).
  // Wpis zachowuje mapowanie serwera, wiec ponowne uzycie nie robi ani munmap, ani mmap. Tylko watek
  // komunikacyjny i removeAllObjects po stop - bez blokady, bo watek RT tu nie siega.
  struct PooledQueue {
    int index;  // miejsce w puli: nazwa names_.responsePool(index)
    std::unique_ptr<boost::interprocess::message_queue> queue;
  };
  std::vector<PooledQueue> responsePool_;

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
