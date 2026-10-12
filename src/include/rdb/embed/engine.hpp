#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/rational.hpp>

#include "fatalError.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/memoryStore.hpp"
#include "rdb/payload.hpp"
#include "rdb/storage.hpp"

/// Fasada osadzenia (#446): jedno miejsce na polityke CYKLU ZYCIA instancji silnika w procesie
/// hosta, zeby kazdy binding (dzis Python) nie wymyslal jej po swojemu.
///
/// Naglowek celowo NIE wlacza niczego z src/retractor/lib: plan, kompilator, model danych i os
/// czasu siedza za wskaznikiem do typu niekompletnego (Plan), wiec konsument fasady nie kompiluje
/// naglowkow silnika, ktorych nie uzywa.
namespace rdb::embed {

/// Odmowa K2/K3 na granicy fasady: wejscie hosta jest zle, silnik jest caly i dziala dalej.
///
/// Niesie rdb::Refusal (klasa i komunikat w postaci, w jakiej zglasza je rdzen - w tym klasa
/// Environment z checkPlanStorage) oraz etap, na ktorym zapadla:
///  - Parse   - tekst RQL nie parsuje sie (status parsePlanText, ten sam co "Parse result:");
///  - Check   - plan parsuje sie, ale nie przechodzi kompilacji ani kontroli magazynu ("Check
///              result:"), albo uzywa czegos, czego silnik osadzony nie ma (patrz compile);
///  - Request - zle wywolanie API: brak planu, strumien spoza planu, indeks poza zakresem.
/// Bledy K1/K4 nie sa odmowa - wychodza jako rdb::EngineFatal.
class Refused : public std::runtime_error {
 public:
  enum class Stage { Parse, Check, Request };

  Refused(Stage stage, Refusal refusal) : std::runtime_error(refusal.message), stage_(stage), refusal_(std::move(refusal)) {}

  [[nodiscard]] Stage stage() const noexcept { return stage_; }
  [[nodiscard]] const Refusal &refusal() const noexcept { return refusal_; }

 private:
  Stage stage_;
  Refusal refusal_;
};

/// Jedna instancja silnika w procesie hosta.
///
/// Usluga buduje swoj stan RAZ, wiec "stan procesu" i "stan silnika" sa w niej nieodroznialne.
/// Notatnik buduje go przy kazdym uruchomieniu komorki i wtedy ta roznica staje sie bledem - dwie
/// komorki dzielily pamiec magazynu MEMORY po samej nazwie strumienia, nic o sobie nie wiedzac.
///
/// Plan wchodzi przez compile(), a jego wykonanie jest CIAGNIETE przez step() - jeden slot na
/// wywolanie, bez zegara sciennego, bez watku komunikacyjnego, bez terminala. Cialo slotu jest tym
/// samym, ktore wykonuje executorsm::run() (os czasu -> maska naleznych strumieni ->
/// dataModel::processRows); rozni sie tylko to, KTO wola i KIEDY.
///
/// Bledy (decyzja (a) z #463): wywolania rdzenia ida w zakresie rdb::ThrowOnFatalScope, wiec K1
/// (FatalError) i K4 (EnvironmentError) wychodza jako rdb::EngineFatal zamiast konczyc proces
/// hosta; odmowy K2/K3 - jako Refused. Blad w slocie zatrzaskuje plan: model moze stac w polowie
/// przeliczenia, wiec kazde nastepne step() zglasza EngineFatal tej samej klasy co pierwszy blad (K4
/// zostaje K4, inny wyjatek niz EngineFatal - K1) z jego trescia. Odczyty dzialaja dalej - host moze
/// obejrzec, co powstalo przed bledem. Wyjscie z tego stanu to compile() albo close().
///
/// @note Obiekt jest NIEPRZENOSNY i NIEKOPIOWALNY celowo. storage trzyma surowy wskaznik na
///       MemoryStore tego silnika; przeniesienie Engine przesunieloby Memory Store i zostawiloby kazdy
///       zbudowany magazyn ze wskaznikiem na pustke.
class Engine {
 public:
  /// @param storageDir katalog magazynu dla planu, ktory nie niesie wlasnej dyrektywy :STORAGE
  ///        (dyrektywa z RQL wygrywa - ta sama regula co `[storage] dir` w konfiguracji uslugi).
  ///        Pusty => katalog roboczy procesu.
  explicit Engine(std::string storageDir = {});
  Engine(const Engine &)            = delete;
  Engine &operator=(const Engine &) = delete;
  Engine(Engine &&)                 = delete;
  Engine &operator=(Engine &&)      = delete;
  ~Engine();

  /// Memory Store tej instancji. Publiczny, bo test izolacji musi umiec zapytac, do ktorego
  /// Memory Store trafil zapis.
  [[nodiscard]] MemoryStore &memory() noexcept { return memory_; }
  [[nodiscard]] const MemoryStore &memory() const noexcept { return memory_; }

  /// Magazyn zwiazany z TYM silnikiem: dostaje jego Memory Store zamiast instancji domyslnej
  /// procesu. Poza tym argumenty i znaczenie jak w konstruktorze rdb::storage.
  ///
  /// Zwracany magazyn NIE MOZE przezyc silnika - trzyma wskaznik na jego Memory Store.
  /// @throws rdb::EngineFatal blad K1/K4 przy budowie magazynu (proces hosta zyje).
  [[nodiscard]] std::unique_ptr<storage> openStorage(std::string_view qryID,                    //
                                                     std::string_view fileName,                 //
                                                     std::string_view storageParam,             //
                                                     std::string_view storageType = "DEFAULT",  //
                                                     bool oneShot                 = false,      //
                                                     bool isHold                  = false,      //
                                                     int percounter               = -1);

  /// Przyjmuje plan: parsowanie, kompilacja, kontrola magazynu, budowa modelu danych i osi czasu.
  /// Poprzedni plan, jesli byl, jest zamykany PRZED budowa nowego.
  ///
  /// Silnik osadzony nie ma czterech rzeczy uslugi i odrzuca je tutaj (Refused, etap Check):
  /// dyrektywy :ROTATION (licznik rotacji jest stanem uslugi), regul DUMP (siegaja po model przez
  /// globalny pProc), regul SYSTEM (::system() w procesie hosta) i zrodel DEVICE (faza DEVICE
  /// petli uslugi, #347).
  ///
  /// @param rql tekst zestawu RQL - to samo, co plik podawany xretractorowi.
  /// @param untilEof zrodla deklarowane czytane BEZ zawijania, jak przy `xretractor -u`: po koncu
  ///        pliku step() zglasza koniec wejscia zamiast produkowac rekordy z danych, ktore juz raz
  ///        przeszly. Musi byc znane tutaj, nie przy step(): ONESHOT wchodzi do fabryki akcesorow
  ///        przy budowie modelu.
  /// @throws Refused (Parse, Check); rdb::EngineFatal (K1/K4). Po bledzie silnik jest bez planu.
  void compile(std::string_view rql, bool untilEof = true);

  /// Ostrzezenia K6 ostatniego udanego compile(): przestarzale `DECLARE ... FILE` i notacja
  /// przyrostkowa `s.avg` (w tej kolejnosci, kazde z numerem wiersza planu). Usluga wypisuje je na stderr albo do logu;
  /// host osadzony pokazuje je po swojemu (Python: DeprecationWarning).
  [[nodiscard]] const std::vector<std::string> &warnings() const noexcept { return warnings_; }

  /// Czy jest przyjety plan. false po zbudowaniu i po close().
  [[nodiscard]] bool hasPlan() const noexcept { return plan_ != nullptr; }

  /// Jeden slot planu. Pierwsze wywolanie wykonuje najpierw krok zerowy (rekord startowy kazdej
  /// deklaracji), dokladnie jak petla uslugi przed pierwszym slotem.
  ///
  /// @return numer wykonanego slotu (od 0) albo nullopt, gdy wejscie sie skonczylo (patrz untilEof
  ///         w compile). Rekordy policzone w slocie, ktory wykryl koniec wejscia, sa poprawne:
  ///         deklaracje czytane sa na koncu slotu, a ich rekord konsumuje dopiero slot nastepny -
  ///         ta sama regula co w executorsm::run().
  /// @throws Refused (Request) bez planu; rdb::EngineFatal z wnetrza slotu albo po zatrzasnieciu
  ///         planu (patrz opis klasy).
  std::optional<std::uint64_t> step();

  /// Liczba slotow wykonanych od compile().
  [[nodiscard]] std::uint64_t slotsDone() const noexcept;

  /// Czas ostatniego wykonanego slotu na osi planu (sekundy); 0 przed pierwszym slotem.
  [[nodiscard]] boost::rational<int> time() const noexcept;

  /// Czy step() zglosil juz koniec wejscia.
  [[nodiscard]] bool endOfInput() const noexcept;

  /// Czy step() zakonczyl sie bledem, po ktorym plan nie liczy dalej (patrz opis klasy).
  [[nodiscard]] bool failed() const noexcept;

  /// Identyfikatory strumieni planu w kolejnosci wykonania (bez dyrektyw).
  /// @throws Refused (Request) bez planu.
  [[nodiscard]] std::vector<std::string> streams() const;

  /// Deskryptor magazynu strumienia - uklad rekordu, ktory zwraca record().
  /// @throws Refused (Request) dla nazwy spoza planu albo bez planu.
  [[nodiscard]] const Descriptor &schema(const std::string &stream) const;

  /// Czy strumien jest deklaracja (zrodlo czytane z pliku), a nie zapytaniem SELECT.
  [[nodiscard]] bool isDeclared(const std::string &stream) const;

  /// Liczba rekordow strumienia zapisanych od startu planu.
  [[nodiscard]] std::size_t recordCount(const std::string &stream) const;

  /// Indeks najstarszego rekordu, ktory da sie jeszcze odczytac. Zero dla strumienia na dysku; dla
  /// deklaracji i strumieni VOLATILE (pierscien MEMORY) wiekszy, bo starsze rekordy juz nie
  /// istnieja - trzymany jest tylko ogon o pojemnosci z kompilatora.
  [[nodiscard]] std::size_t retainedFrom(const std::string &stream) const;

  /// Rekord strumienia o indeksie POSTEPUJACYM (0 = najstarszy). KOPIA - nastepny slot nie ma jak
  /// go uniewaznic. Indeks ponizej retainedFrom() zglaszany jest odmowa (Request), nigdy oddawany
  /// jako cudzy rekord.
  [[nodiscard]] payload record(const std::string &stream, std::size_t index);

  /// Gesta projekcja: rekordy [first, first+count) x elementy splaszczone `flatFields` (kolejnosc
  /// jak w argumencie), wierszami. Wartosc null daje NaN. To jest kopia, o ktorej mowi decyzja
  /// "kopiuj, nie przypinaj" z docs/jupyter-integration.md sekcja 4; double miesci dokladnie kazdy
  /// typ liczbowy deskryptora poza UINT powyzej 2^53.
  /// @throws Refused (Request) dla pola nieliczbowego (STRING, pary) albo zakresu poza strumieniem.
  [[nodiscard]] std::vector<double> project(const std::string &stream, const std::vector<int> &flatFields, std::size_t first,
                                            std::size_t count);

  /// Zamyka plan: model danych i jego magazyny. Idempotentne; destruktor wola to samo.
  void close() noexcept;

 private:
  struct Plan;

  /// Wylacznie plan, ktory jest; bez planu Refused (Request) z nazwa operacji.
  [[nodiscard]] Plan &requirePlan(const char *operation) const;
  [[nodiscard]] storage &streamStorage(const std::string &stream, const char *operation) const;
  /// Cialo step() bez zatrzasku planu.
  std::optional<std::uint64_t> stepPlan(Plan &plan);

  std::string storageDir_;
  std::vector<std::string> warnings_;
  MemoryStore memory_;
  std::unique_ptr<Plan> plan_;
};

}  // namespace rdb::embed
