#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/rational.hpp>

#include "rdb/descriptor.hpp"
#include "rdb/exceptions.hpp"
#include "rdb/memoryStore.hpp"
#include "rdb/payload.hpp"
#include "rdb/storage.hpp"

/// Warstwa L2 architektury osadzania (docs/embedded-roadmap.md sekcja 2): fasada C++, ktorej
/// uzywaja WSZYSTKIE trzy cele - nanobind dla Pythona, ABI C dla Swifta i JNI dla Androida.
/// Jedno miejsce na polityke CYKLU ZYCIA, zeby kazdy z nich nie wymyslal jej po swojemu.
///
/// Naglowek celowo NIE wlacza niczego z src/retractor/lib: plan, kompilator, model danych i os
/// czasu siedza za wskaznikiem do typu niekompletnego (Plan). Dzieki temu bramka
/// embedding_boundary widzi tu tylko to, co widzi w reszcie src/include/rdb, a konsument
/// warstwy L3 nie kompiluje naglowkow silnika, ktorych nie uzywa.
namespace rdb::embed {

/// Tekst RQL nie parsuje sie. Komunikat jest DOKLADNIE tym, co parser zglasza statusem -
/// z numerem wiersza, jesli parser go podal.
///
/// Oba typy ponizej dziedzicza po ConfigError, bo taka jest ich natura: wejscie jest zle,
/// silnik jest caly. Pelna taksonomia (faza 5) moze je przeniesc do rdb/exceptions.hpp;
/// dzis rzuca je wylacznie Engine::compile(), wiec mieszkaja przy nim.
class SyntaxError : public ConfigError {
 public:
  using ConfigError::ConfigError;
};

/// Plan parsuje sie, ale nie przechodzi kompilacji ("Check result:" launchera) albo
/// uzywa czegos, czego silnik osadzony nie ma (akcje regul DUMP i SYSTEM, dyrektywa
/// :ROTATION - patrz Engine::compile).
class CompileError : public ConfigError {
 public:
  using ConfigError::ConfigError;
};

/// Jedna instancja silnika w procesie hosta.
///
/// Powod istnienia jest jeden i jest to teza calej fazy 2: demon buduje swoj stan RAZ, wiec
/// "stan procesu" i "stan silnika" sa w nim nieodroznialne. Notatnik buduje go przy kazdym
/// uruchomieniu komorki i wtedy ta roznica staje sie bledem - dwie komorki dzielily pamiec
/// magazynu MEMORY po samej nazwie strumienia, nic o sobie nie wiedzac.
///
/// Faza 3 (docs/core-phase-3.md) dolozyla to, na co faza 2 zostawila miejsce: plan wchodzi
/// przez compile(), a jego wykonanie jest CIAGNIETE przez step() - jeden slot na wywolanie,
/// bez zegara sciennego, bez watku komunikacyjnego, bez terminala. Cialo slotu jest tym samym,
/// ktore wykonuje executorsm::run() (os czasu -> zbior oczekujacych strumieni ->
/// dataModel::processRows); rozni sie tylko to, KTO wola i KIEDY.
///
/// @note Obiekt jest NIEPRZENOSNY i NIEKOPIOWALNY celowo. storage trzyma surowy wskaznik na
///       MemoryStore tego silnika; przeniesienie Engine przesunieloby sklep i zostawiloby
///       kazdy zbudowany magazyn ze wskaznikiem na pustke.
class Engine {
 public:
  /// @param storageDir katalog magazynu dla planu, ktory nie niesie wlasnej dyrektywy
  ///        :STORAGE (dyrektywa z RQL wygrywa - ta sama regula co `[storage] dir` w
  ///        konfiguracji demona). Pusty => katalog roboczy procesu.
  explicit Engine(std::string storageDir = {});
  Engine(const Engine &)            = delete;
  Engine &operator=(const Engine &) = delete;
  Engine(Engine &&)                 = delete;
  Engine &operator=(Engine &&)      = delete;
  ~Engine();

  /// Sklep magazynu MEMORY tej instancji. Publiczny, bo test izolacji musi umiec zapytac,
  /// do ktorego sklepu trafil zapis.
  [[nodiscard]] MemoryStore &memory() noexcept { return memory_; }
  [[nodiscard]] const MemoryStore &memory() const noexcept { return memory_; }

  /// Magazyn zwiazany z TYM silnikiem: dostaje jego sklep MEMORY zamiast instancji domyslnej
  /// procesu. Poza tym argumenty i znaczenie jak w konstruktorze rdb::storage.
  ///
  /// Zwracany magazyn NIE MOZE przezyc silnika - trzyma wskaznik na jego sklep.
  [[nodiscard]] std::unique_ptr<storage> openStorage(std::string_view qryID,                    //
                                                     std::string_view fileName,                 //
                                                     std::string_view storageParam,             //
                                                     std::string_view storageType = "DEFAULT",  //
                                                     bool oneShot                 = false,      //
                                                     bool isHold                  = false,      //
                                                     int percounter               = -1);

  /// Przyjmuje plan: parsowanie, kompilacja, budowa modelu danych i osi czasu. Poprzedni
  /// plan, jesli byl, jest zamykany PRZED budowa nowego.
  ///
  /// @param rql tekst zestawu RQL - to samo, co plik podawany xretractorowi.
  /// @param untilEof zrodla deklarowane czytane BEZ zawijania, jak przy `xretractor -u`:
  ///        po koncu pliku step() zglasza koniec wejscia zamiast produkowac rekordy z
  ///        danych, ktore juz raz przeszly. Domyslnie TAK, bo notatnik czyta plik jako
  ///        zbior danych, a nie jako zrodlo bez konca. Musi byc znane tutaj, nie przy
  ///        step(): ONESHOT wchodzi do fabryki akcesorow przy budowie modelu.
  /// @throws SyntaxError, CompileError, ConfigError (np. katalog :STORAGE nie istnieje).
  void compile(std::string_view rql, bool untilEof = true);

  /// Czy jest przyjety plan. false po zbudowaniu i po close().
  [[nodiscard]] bool hasPlan() const noexcept { return plan_ != nullptr; }

  /// Jeden slot planu. Pierwsze wywolanie wykonuje najpierw krok zerowy (rekord startowy
  /// kazdej deklaracji), dokladnie jak petla demona przed pierwszym slotem.
  ///
  /// @return numer wykonanego slotu (od 0) albo nullopt, gdy wejscie sie skonczylo
  ///         (patrz untilEof w compile). Rekordy policzone w slocie, ktory wykryl koniec
  ///         wejscia, sa poprawne: deklaracje czytane sa na koncu slotu, a ich rekord
  ///         konsumuje dopiero slot nastepny - ta sama regula co w executorsm::run().
  /// @throws ConfigError bez planu; rdb::Error z wnetrza slotu.
  std::optional<std::uint64_t> step();

  /// Liczba slotow wykonanych od compile().
  [[nodiscard]] std::uint64_t slotsDone() const noexcept;

  /// Czas ostatniego wykonanego slotu na osi planu (sekundy); 0 przed pierwszym slotem.
  [[nodiscard]] boost::rational<int> time() const noexcept;

  /// Czy step() zglosil juz koniec wejscia.
  [[nodiscard]] bool endOfInput() const noexcept;

  /// Identyfikatory strumieni planu w kolejnosci wykonania (bez dyrektyw).
  [[nodiscard]] std::vector<std::string> streams() const;

  /// Deskryptor magazynu strumienia - uklad rekordu, ktory zwraca record().
  /// @throws ConfigError dla nazwy spoza planu.
  [[nodiscard]] const Descriptor &schema(const std::string &stream) const;

  /// Czy strumien jest deklaracja (zrodlo czytane z pliku), a nie zapytaniem SELECT.
  [[nodiscard]] bool isDeclared(const std::string &stream) const;

  /// Liczba rekordow strumienia zapisanych od startu planu.
  [[nodiscard]] std::size_t recordCount(const std::string &stream) const;

  /// Indeks najstarszego rekordu, ktory da sie jeszcze odczytac. Zero dla strumienia na
  /// dysku; dla deklaracji i strumieni VOLATILE (pierscien MEMORY) wiekszy, bo starsze
  /// rekordy juz nie istnieja - trzymany jest tylko ogon o pojemnosci z kompilatora.
  [[nodiscard]] std::size_t retainedFrom(const std::string &stream) const;

  /// Rekord strumienia o indeksie POSTEPUJACYM (0 = najstarszy). KOPIA - nastepny slot nie ma
  /// jak go uniewaznic. Indeks ponizej retainedFrom() zglaszany jest ConfigError, nigdy
  /// oddawany jako cudzy rekord.
  [[nodiscard]] payload record(const std::string &stream, std::size_t index);

  /// Gesta projekcja: rekordy [first, first+count) x elementy splaszczone `flatFields`
  /// (kolejnosc jak w argumencie), wierszami. Wartosc null daje NaN. To jest kopia, o ktorej
  /// mowi decyzja "kopiuj, nie przypinaj" z docs/jupyter-integration.md sekcja 4; double miesci
  /// dokladnie kazdy typ liczbowy deskryptora poza UINT powyzej 2^53.
  /// @throws ConfigError dla pola nieliczbowego (STRING, pary) albo zakresu poza strumieniem.
  [[nodiscard]] std::vector<double> project(const std::string &stream, const std::vector<int> &flatFields, std::size_t first,
                                            std::size_t count);

  /// Zamyka plan: model danych i jego magazyny. Idempotentne; destruktor wola to samo.
  void close() noexcept;

 private:
  struct Plan;

  /// Wylacznie plan, ktory jest; bez planu ConfigError z nazwa operacji.
  [[nodiscard]] Plan &requirePlan(const char *operation) const;
  [[nodiscard]] storage &streamStorage(const std::string &stream, const char *operation) const;

  std::string storageDir_;
  MemoryStore memory_;
  std::unique_ptr<Plan> plan_;
};

}  // namespace rdb::embed
