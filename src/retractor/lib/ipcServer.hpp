#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <thread>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/property_tree/ptree.hpp>

#include "constants.hpp"

/// Transport IPC strony serwerowej: segment pamieci dzielonej z mapa odpowiedzi,
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

  struct Callbacks {
    CommandHandler onCommand;
    Notifier onReady;            // zasoby IPC gotowe -- raz, przed petla odbioru
    Notifier onFailure;          // zasobow IPC nie da sie zbudowac -- watek konczy prace
    Notifier onMessageReceived;  // odebrano komende -- przed jej obsluga
    StopPredicate shouldStop;
  };

  /// onReady i onFailure wykluczaja sie i razem sa WYCZERPUJACE: watek komunikacyjny zawsze
  /// wola dokladnie jedno z nich, zanim skonczy prace. Bez onFailure zawiedziona budowa zasobow
  /// (najczesciej brak miejsca w /dev/shm na kolejke komend) konczyla watek po cichu, a strona
  /// czekajaca na gotowosc czekala bez konca.

  IpcServer()                             = default;
  IpcServer(const IpcServer &)            = delete;
  IpcServer &operator=(const IpcServer &) = delete;

  /// Nazwa serwera wyznaczajaca komplet nazw obiektow IPC tej instancji. Wolac PRZED start();
  /// pozniejsza zmiana rozjechalaby nazwy juz utworzonych obiektow z nazwami uzywanymi do ich
  /// kasowania. Nazwa pusta (domyslna) daje nazwy historyczne, jednoserwerowe.
  void setServerName(std::string_view serverName);

  /// Startuje watek komunikacyjny. onReady wola sie z tego watku, gdy segment,
  /// muteks nazwany i kolejka komend juz istnieja.
  void start(Callbacks callbacks);

  /// Normalne zamkniecie: dolacza watek komunikacyjny.
  void stop();

  /// Sciezka atexit/FatalError: odpina lub dolacza watek i kasuje obiekty IPC.
  /// Kasuje ten sam zestaw co removeAllObjects(), z jednym zastrzezeniem: kolejki
  /// klientow tylko wtedy, gdy uda sie wziac muteks map bez czekania (patrz .cpp).
  void shutdownFromExitHandler();

  /// Uchwyt watku komunikacyjnego dla rtKeepThreadOffRtCpus. Wolac po start().
  std::thread::native_handle_type threadHandle();

  /// Rejestruje klienta na strumieniu i tworzy jego kolejke odpowiedzi.
  void subscribe(int clientId, const std::string &streamName, int maxElements);

  /// Rozsyla biezacy wiersz kazdego z podanych strumieni do jego subskrybentow.
  void broadcast(const std::set<std::string> &streams, const RowFormatter &formatRow);

  /// Zawiadamia klientow o koncu pracy serwera i czysci rejestr subskrypcji.
  void broadcastOutOfBusiness();

  /// Kasuje wszystkie obiekty IPC serwera: segment, kolejke komend, muteks
  /// nazwany i kolejki odpowiedzi pozostalych klientow. Wolac po stop().
  void removeAllObjects();

 private:
  void commandLoop() const;

  /// Segment, kolejka komend, muteks nazwany. Nie dotyka stanu klientow.
  void removeGlobalObjects() const;

  /// Kolejki odpowiedzi klientow plus wyczyszczenie rejestru subskrypcji.
  /// Wolajacy MUSI trzymac clientMapsMutex_.
  void removeClientQueues();

  Callbacks callbacks_;
  std::thread commsThread_;

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

  // Muteks obu map klienckich (sledztwo ~40 ms, JOURNAL.md 2026-07-18, Faza 3):
  // mapy sa modyfikowane przez watek komunikacyjny (rejestracja show) i czytane/
  // czyszczone przez watek przetwarzajacy (broadcast). Muteks trzymany wylacznie
  // na operacjach na mapach -- NIGDY podczas konstrukcji kolejki (mmap ~MB), aby
  // nie wnosic inwersji priorytetow do watku RT.
  std::mutex clientMapsMutex_;
};
