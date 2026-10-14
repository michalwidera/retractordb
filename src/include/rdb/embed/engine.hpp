#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/rational.hpp>

#include "rdb/descriptor.hpp"
#include "rdb/embed/storageGuard.hpp"
#include "rdb/error.hpp"
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
/// Operacje zwracaja Result: odmowa planu lub API nie przerywa procesu. Blad w slocie
/// zatrzaskuje plan z pierwotna przyczyna; odczyty pozostaja dostepne do compile()/close().
/// Naruszenie niezmiennika przez rdb::fatal() konczy proces hosta zgodnie z jego handlerem.
///
/// @note Obiekt jest NIEPRZENOSNY i NIEKOPIOWALNY celowo. storage trzyma surowy wskaznik na
///       MemoryStore tego silnika; przeniesienie Engine przesunieloby Memory Store i zostawiloby kazdy
///       zbudowany magazyn ze wskaznikiem na pustke.
class Engine {
 public:
  /// @param storageDir katalog magazynu dla planu, ktory nie niesie wlasnej dyrektywy :STORAGE
  ///        (dyrektywa z RQL wygrywa - ta sama regula co `[storage] dir` w konfiguracji uslugi).
  ///        Pusty => prywatny katalog tymczasowy, zyjacy do zniszczenia Engine.
  /// @param lockDir katalog blokad serwisu (paths.lock_dir); pusty => TMPDIR albo paths.lock_dir z configPath.
  /// @param configPath plik konfiguracji czytany tylko, gdy podany (paths.lock_dir, embedded.lenient_lock_dirs).
  ///        Katalogu prywatnego kontrola blokad serwisu nie dotyczy.
  [[nodiscard]] static Result<std::unique_ptr<Engine>> create(std::string storageDir = {}, std::string lockDir = {},
                                                              std::string configPath = {});
  Engine(const Engine &)            = delete;
  Engine &operator=(const Engine &) = delete;
  Engine(Engine &&)                 = delete;
  Engine &operator=(Engine &&)      = delete;
  ~Engine();

  /// Efektywny katalog planu, a bez planu domyslny katalog tego Engine.
  [[nodiscard]] const std::string &storageDir() const noexcept;

  /// Memory Store tej instancji. Publiczny, bo test izolacji musi umiec zapytac, do ktorego
  /// Memory Store trafil zapis.
  [[nodiscard]] MemoryStore &memory() noexcept { return memory_; }
  [[nodiscard]] const MemoryStore &memory() const noexcept { return memory_; }

  /// Magazyn zwiazany z TYM silnikiem: dostaje jego Memory Store zamiast instancji domyslnej
  /// procesu. Poza tym argumenty i znaczenie jak w konstruktorze rdb::storage.
  ///
  /// Zwracany magazyn NIE MOZE przezyc silnika - trzyma wskaznik na jego Memory Store.
  /// @return sukces albo blad Result z kategoria i komunikatem.
  [[nodiscard]] Result<std::unique_ptr<storage>> openStorage(std::string_view qryID,                    //
                                                             std::string_view fileName,                 //
                                                             std::string_view storageParam,             //
                                                             std::string_view storageType = "DEFAULT",  //
                                                             bool oneShot                 = false,      //
                                                             bool isHold                  = false,      //
                                                             int percounter               = -1);

  /// Przyjmuje plan: parsowanie, kompilacja, kontrola magazynu, budowa modelu danych i osi czasu.
  /// Poprzedni plan, jesli byl, jest zamykany PRZED budowa nowego.
  ///
  /// Silnik osadzony nie ma czterech rzeczy uslugi i odrzuca je tutaj (Errc::Compile):
  /// dyrektywy :ROTATION (licznik rotacji jest stanem uslugi), regul DUMP (siegaja po model przez
  /// globalny pProc), regul SYSTEM (::system() w procesie hosta) i zrodel DEVICE (faza DEVICE
  /// petli uslugi, #347).
  ///
  /// @param rql tekst zestawu RQL - to samo, co plik podawany xretractorowi.
  /// @param untilEof zrodla deklarowane czytane BEZ zawijania, jak przy `xretractor -u`: po koncu
  ///        pliku step() zglasza koniec wejscia zamiast produkowac rekordy z danych, ktore juz raz
  ///        przeszly. Musi byc znane tutaj, nie przy step(): ONESHOT wchodzi do fabryki akcesorow
  ///        przy budowie modelu.
  /// @return sukces albo blad Result z kategoria i komunikatem.
  [[nodiscard]] Result<> compile(std::string_view rql, bool untilEof = true);

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
  /// Blad Result zatrzymuje plan z pierwotna przyczyna (patrz opis klasy).
  [[nodiscard]] Result<std::optional<std::uint64_t>> step();

  /// Liczba slotow wykonanych od compile().
  [[nodiscard]] std::uint64_t slotsDone() const noexcept;

  /// Czas ostatniego wykonanego slotu na osi planu (sekundy); 0 przed pierwszym slotem.
  [[nodiscard]] boost::rational<int> time() const noexcept;

  /// Czy step() zglosil juz koniec wejscia.
  [[nodiscard]] bool endOfInput() const noexcept;

  /// Czy step() zakonczyl sie bledem, po ktorym plan nie liczy dalej (patrz opis klasy).
  [[nodiscard]] bool failed() const noexcept;

  /// Identyfikatory strumieni planu w kolejnosci wykonania (bez dyrektyw).
  /// @return sukces albo blad Result z kategoria i komunikatem.
  [[nodiscard]] Result<std::vector<std::string>> streams() const;

  /// Deskryptor magazynu strumienia - uklad rekordu, ktory zwraca record().
  /// @return sukces albo blad Result z kategoria i komunikatem.
  [[nodiscard]] Result<const Descriptor *> schema(const std::string &stream) const;

  /// Czy strumien jest deklaracja (zrodlo czytane z pliku), a nie zapytaniem SELECT.
  [[nodiscard]] Result<bool> isDeclared(const std::string &stream) const;

  /// Liczba rekordow strumienia zapisanych od startu planu.
  [[nodiscard]] Result<std::size_t> recordCount(const std::string &stream) const;

  /// Indeks najstarszego rekordu, ktory da sie jeszcze odczytac. Zero dla strumienia na dysku; dla
  /// deklaracji i strumieni VOLATILE (pierscien MEMORY) wiekszy, bo starsze rekordy juz nie
  /// istnieja - trzymany jest tylko ogon o pojemnosci z kompilatora.
  [[nodiscard]] Result<std::size_t> retainedFrom(const std::string &stream) const;

  /// Rekord strumienia o indeksie POSTEPUJACYM (0 = najstarszy). KOPIA - nastepny slot nie ma jak
  /// go uniewaznic. Indeks ponizej retainedFrom() zglaszany jest odmowa (Request), nigdy oddawany
  /// jako cudzy rekord.
  [[nodiscard]] Result<payload> record(const std::string &stream, std::size_t index);

  /// Gesta projekcja: rekordy [first, first+count) x elementy splaszczone `flatFields` (kolejnosc
  /// jak w argumencie), wierszami. Wartosc null daje NaN. To jest kopia, o ktorej mowi decyzja
  /// "kopiuj, nie przypinaj" z zalacznika API Python:
  /// https://documentation.retractordb.com/appendices/python-jupyter-api.html#memory-ownership-numpy-and-dlpack
  /// Double miesci dokladnie kazdy typ liczbowy deskryptora poza UINT powyzej 2^53.
  /// @return sukces albo blad Result z kategoria i komunikatem.
  [[nodiscard]] Result<std::vector<double>> project(const std::string &stream, const std::vector<int> &flatFields,
                                                    std::size_t first, std::size_t count);

  /// Zamyka plan: model danych i jego magazyny. Idempotentne; destruktor wola to samo.
  void close() noexcept;

 private:
  struct Plan;
  Engine(OfflinePolicy offline, std::string storageDir, bool temporaryDir);

  /// Wylacznie plan, ktory jest; bez planu Errc::Config z nazwa operacji.
  [[nodiscard]] Result<Plan *> requirePlan(const char *operation) const;
  [[nodiscard]] Result<storage *> streamStorage(const std::string &stream, const char *operation) const;
  /// Cialo step() bez zatrzasku planu.
  [[nodiscard]] Result<std::optional<std::uint64_t>> stepPlan(Plan &plan);

  // Przed storageDir_: odmowa konfiguracji nie moze zostawic utworzonego katalogu prywatnego.
  OfflinePolicy offline_;
  std::string storageDir_;
  bool temporaryDir_{false};
  std::vector<std::string> warnings_;
  MemoryStore memory_;
  std::unique_ptr<Plan> plan_;
};

}  // namespace rdb::embed
