#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include "bus.hpp"
#include "compiler.hpp"
#include "CRSMath.hpp"
#include "fatalError.hpp"
#include "lockManager.hpp"
#include "rdb/faccbindev.hpp"

#include "appConfig.hpp"
#include "rdb/error.hpp"

using ptree  = boost::property_tree::ptree;
using vm_map = boost::program_options::variables_map;
class dataModel;
/// Obiekt klasy executorsm powinien:
/// - przed wejściem w pętlę uruchamiać serwer IPC i czekać na gotowość jego zasobów, a dopiero po niej publikować opis procesu w pliku blokady, tak aby linia PID oznaczała serwer gotowy do obsługi klientów.
/// - kończyć start kodem no_buffer_space, gdy zasobów IPC nie udało się zbudować, zamiast czekać bez końca.
/// - w każdej niepustej epoce planu budować model danych, wykonywać krok zerowy deklaracji plikowych (bez DEVICE),
///   a następnie w kolejnych taktach wyznaczać strumienie należne, liczyć je przez dataModel::processRows i rozsyłać ich
///   wiersze subskrybentom; dla pustego planu działać w trybie bezczynnym bez modelu danych.
/// - w trybie taktowanym spać do terminów slotów liczonych od wspólnej kotwicy, bez przesuwania jej po spóźnieniu;
///   przy --realtime wywoływać rtActivate dopiero po pomyślnym rtCheckAndPrint i kontynuować także przy nieudanej aktywacji;
///   w trybie --no-clock liczyć sloty bez czekania na zegar.
/// - przy --xqrywait wstrzymywać obliczenia do pierwszej obsłużonej komendy klienta.
/// - przy limicie pętli (--llimitqry) wykonywać dokładnie tyle slotów, a zatrzymanie klawiszem dopuszczać wyłącznie w przebiegu bez limitu i bez --noanykey.
/// - przy --until-eof kończyć pracę po slocie wykrywającym koniec wejścia BINFILE lub TEXTFILE, a dla DEVICE po fazie
///   oczekiwania, przed obliczeniem slotu; wpisywać do logu nazwę wyczerpanego źródła.
/// - przed czytaniem modelu w taktach zbierać rekordy należnych źródeł DEVICE z terminem efektywnym (tryb bez zegara, klauzula TIMEOUT, konfiguracja, 0).
/// - obsługiwać komendy hello (odpowiedź world), get (lista strumieni z parametrami), detail (schemat, okres, lokalizacja, pojemność, liczba rekordów i zapytanie strumienia), show (subskrypcja klienta), adhoc, kill (zatrzymanie pętli) oraz reset-begin, reset-chunk i reset-commit.
/// - na get, detail, show i adhoc odpowiadać przy braku planu odpowiedzią braku planu, a w trakcie zamykania odpowiedzią zamykania serwera, a dla nazwy spoza planu error.response z nazwą strumienia.
/// - w show odmawiać subskrypcji z error.response, gdy epoka planu skończyła się w trakcie subskrypcji, tak aby klient nie pozostał zarejestrowany na strumieniu planu, który już nie działa.
/// - zamieniać wyjątek obsługi komendy na error.response bez kończenia procesu, a złamany niezmiennik silnika w komendzie zgłaszać odpowiedzią engine error z wpisem krytycznym w logu.
/// - w adhoc kompilować zapytanie na kopii planu, roszczyć nowe nazwy na magistrali i importować je do działającego planu, a przy każdej porażce po imporcie przywracać plan i zwalniać roszczone zasoby; reguły DO SYSTEM odrzucać w tym kanale bezwarunkowo.
/// - przyjmować plan kanałem --reset w porcjach z ograniczoną liczbą porcji, rozmiarem tekstu i liczbą równoczesnych transferów, a przy reset-commit sprawdzać go jak przy starcie (parsowanie, kompilacja, limity, licznik rotacji, rozłączność na magistrali) i rezerwować jego zasoby bez zmiany działającego planu.
/// - odrzucać w kanale --reset plan z regułą DO SYSTEM, chyba że konfiguracja ustawia service.unrestricted.
/// - po przyjęciu planu --reset kończyć bieżącą epokę rozesłaniem znacznika końca pracy subskrybentom i zastępować plan
///   nowym; błąd ponownego parsowania lub kompilacji w applyPendingPlan zastępować pustym planem (tryb bezczynny).
/// - w razie nieudanej budowy modelu danych, także po --reset, kończyć run kodem EIO dla Errc::IO albo EPROTO dla
///   pozostałych błędów, zapisując przyczynę w initializationError.
/// - przy aktywacji zasobów nowego planu kończyć proces przez FatalError z kodem EXIT_FAILURE dla statusu NotReserved;
///   przy Unavailable kontynuować, a gdy magistrala jest dołączona, ostrzegać o braku publikacji nowego planu
///   i kontroli unikalności nazw.
/// - błąd odczytu lub rezerwacji numeru licznika rotacji zgłaszać jako Errc::IO i kończyć wykonanie kodem EIO;
///   błąd wykonania slotu zgłaszać z zatrzymaniem pętli i kodem odpowiadającym jego kategorii.
/// - przy SIGINT, SIGTERM, SIGHUP i zatrzymaniu pętli rozsyłać subskrybentom znacznik końca pracy i zatrzymywać wątek komunikacyjny przed zwróceniem kodu wyjścia.
struct executorsm {
  /// Odmowa budowy modelu wraca do launchera, ktory dla K2 woluje refusePlan (#474).
  rdb::Refusal initializationError;

  /// serverName pusta => tozsamosc historyczna (jeden serwer na maszyne, dotychczasowe nazwy
  /// obiektow IPC). Niepusta => wlasny, rozlaczny obszar IPC tej instancji.
  /// @param systemdUnit nazwa jednostki systemd, gdy proces nia jest; pusta => zwykly proces.
  int run(qTree &coreInstance, FlockServiceGuard &guard, bus::Bus &xrdbbus, compiler &cm, vm_map &vm, const AppConfig &cfg,
          std::string_view serverName = {}, std::string_view systemdUnit = {});

  enum : std::int8_t { inifitie_loop = 0, stop_now = 1 };

 private:
  static qTree *coreInstancePtr;
  static compiler *cmPtr;
  static int cfgQueueBufferSeconds;
  static int cfgMinQueueElements;
  static int cfgRtPriority;
  /// Domyslny katalog magazynu z konfiguracji. Uzywany przy wycenie zasobow planu, ktory
  /// dopiero ma zostac przyjety (`--reset`) i sam nie niesie dyrektywy `:STORAGE`.
  static std::string cfgStorageDir;
  /// Czy plan przyjmowany kanalem `--reset` moze niesc regule `DO SYSTEM`. Domyslnie nie -
  /// uzasadnienie granicy stoi przy sprawdzeniu w validatePlanText.
  static bool cfgUnrestricted;
  /// Budzet pamieci historii planu (`[limits] history_memory_mib`) dla kompilacji w watku
  /// komunikacyjnym: ad-hoc i `--reset`. Plan startowy kompiluje launcher tym samym budzetem.
  static int cfgHistoryMemoryMib;
  /// `[storage] default_retention` dla tych samych kompilacji - .desc strumienia nie moze zalezec
  /// od kanalu, ktorym przyszedl plan.
  static rdb::retention_t cfgDefaultRetention;
  /// `[sources] timeout_s` (#347): termin DEVICE bez klauzuli TIMEOUT - dla fazy DEVICE w petli
  /// slotow i dla ostrzezen kompilacji w kanale ad-hoc i `--reset`.
  static std::optional<double> cfgSourcesTimeout;
  /// Katalog magazynu, w ktorym pisze plan DZIALAJACY. Trzymany osobno, bo dataModel usuwa
  /// dyrektywy z drzewa planu - w czasie pracy `:STORAGE` nie da sie juz z niego odczytac,
  /// a zapytanie ad-hoc musi roscic dokladnie te sciezki, ktore powstana na dysku.
  static std::string activeStorageDir;
  // Set from IpcServer's onReady callback once all IPC resources are ready.
  // run() waits on this before publishLockInfo(), so PID appears in the lock file
  // only after IPC is fully initialized and xqry can connect safely.
  static std::atomic<bool> ipcReady;
  // Ustawiane z wywolania zwrotnego onFailure, gdy watek komunikacyjny nie zdolal zbudowac
  // zasobow IPC (najczestszy powod: brak miejsca w /dev/shm na kolejke komend). Bez tej flagi
  // run() czekal na ipcReady BEZ LIMITU CZASU, a watek, ktory mial ja podniesc, juz nie zyl --
  // proces stal w miejscu, nie liczac i nie odpowiadajac (odtworzone na tmpfs 512 KiB).
  static std::atomic<bool> ipcFailed;

  static ptree commandProcessor(const ptree &ptInval);
  /// Odpowiedz komendy na zlamany niezmiennik silnika: `error.response` = "engine error: ...",
  /// log krytyczny, proces liczy dalej. Ksztalt, ktory do 2026-10 skladal catch(const rdb::Error&)
  /// w commandProcessor - teraz skladany z wartosci w miejscu wykrycia.
  static ptree engineErrorResponse(std::string_view command, const std::string &message);
  static ptree collectStreamsParameters();
  static ptree getAdHoc(const std::string &adHocQuery);
  static ptree attachAdHocRule(qTree &coreInstanceCopy, const std::string &streamName);

  // Przeladowanie planu (`xqry --reset`). Transfer jest trzyczesciowy, bo kolejka komend
  // przyjmuje komunikaty do ipc::kQueryQueueMaxMessageSize bajtow, a plan bywa dluzszy.
  static ptree resetBegin(const ptree &ptInval);
  static ptree resetChunk(const ptree &ptInval);
  static ptree resetCommit(const ptree &ptInval);

  /// Sprawdza zestaw RQL tak, jak zrobilby to start: parsowanie, kompilacja, limity i
  /// rozlacznosc nazw wzgledem POZOSTALYCH zywych instancji. Przy sukcesie rezerwuje zasoby
  /// nowego planu; przy odmowie nie zmienia planu ani jego aktywnych roszczen.
  static std::string validatePlanText(const std::string &planText);

  /// Wymienia plan na przyjety wczesniej `pendingPlanText` i przygotowuje kolejna epoke:
  /// aktywacja rezerwacji magistrali, skasowanie artefaktow, licznik rotacji, plik zapytan.
  /// Plan, ktorego nie da sie tu zbudowac, konczy sie epoka PUSTA (tryb bezczynny) -
  /// nigdy smiercia procesu. Blad zwracany stad to co innego: magistrala, ktora odmowila
  /// aktywacji zarezerwowanych zasobow, albo licznik :ROTATION, ktorego nie da sie wczytac -
  /// run() konczy wtedy proces jak po bledzie silnika w epoce (FATAL, EXIT_FAILURE).
  [[nodiscard]] rdb::Result<> applyPendingPlan(FlockServiceGuard &guard, bus::Bus &xrdbbus, const AppConfig &cfg);

  /// Wyznacza strumienie nalezne w biezacym takcie: wypelnia dueMask_ i dueNames_.
  /// Wolajacy MUSI trzymac core_mutex, a maska jest wazna tylko dopoki uklad planu sie
  /// nie zmieni - patrz komentarz przy dueMask_.
  void collectAwaitedStreams(CRationalStreamMath::TimeLine &tl, qTree *coreInstancePtr);
  std::string printRowValue(const std::string &query_name);

  /// Migawka fazy DEVICE (#347): nalezne w biezacym takcie zrodla DEVICE z efektywnymi terminami
  /// liczonymi od @p wake. Wolajacy MUSI trzymac plan_epoch_mutex - akcesory zyja w modelu, a plan
  /// zmienia import ad-hoc. Kazde zrodlo dostaje markAwaited(); zdejmuje je rdb::awaitRecords().
  void collectDueDevices(CRationalStreamMath::TimeLine &tl, dataModel &proc, std::chrono::steady_clock::time_point wake);

  /// Wpisy fazy DEVICE, uzywane ponownie miedzy taktami jak dueMask_.
  std::vector<rdb::deviceWait> deviceWaits_;

  /// Strumienie nalezne w takcie, w DWOCH postaciach, bo konsumenci chca czego innego:
  /// dataModel::processRows pyta o przynaleznosc POZYCJI w planie, a IpcServer::broadcast
  /// ITERUJE po nazwach. Obie struktury sa skladowymi i uzywane ponownie miedzy taktami:
  /// assign/clear zachowuja pojemnosc, wiec po pierwszym takcie nie ma tu alokacji.
  ///
  /// Maska jest POZYCYJNA, wiec wolno jej uzyc tylko wobec tego ukladu planu, z ktorego
  /// powstala. Uklad zmienia import ad hoc (compiler::importFrom + compile, ktore dopisuja
  /// wezly i sortuja topologicznie); import idzie pod plan_epoch_mutex, wiec wypelnienie i
  /// konsumpcja maski musza wypasc pod TA SAMA blokada epoki. Z tego samego powodu widoki
  /// w dueNames_ wskazuja na query::id zywego planu i zyja tylko tak dlugo.
  std::vector<char> dueMask_;
  std::vector<std::string_view> dueNames_;
};
