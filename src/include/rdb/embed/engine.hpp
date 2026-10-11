#pragma once

#include <memory>
#include <string_view>

#include "rdb/memoryStore.hpp"
#include "rdb/storage.hpp"

/// Fasada osadzenia (#446, C2): jedno miejsce na polityke CYKLU ZYCIA instancji silnika w procesie
/// hosta, zeby kazdy binding (dzis Python) nie wymyslal jej po swojemu.
namespace rdb::embed {

/// Jedna instancja silnika w procesie hosta.
///
/// Demon buduje swoj stan RAZ, wiec "stan procesu" i "stan silnika" sa w nim nieodroznialne.
/// Notatnik buduje go przy kazdym uruchomieniu komorki i wtedy ta roznica staje sie bledem - dwie
/// komorki dzielily pamiec magazynu MEMORY po samej nazwie strumienia, nic o sobie nie wiedzac.
///
/// DZIS Engine trzyma tylko Memory Store. compile() i step() dopisze etap C3 (#491) do ISTNIEJACEGO
/// obiektu, zamiast wymyslac wlasciciela pod presja innego problemu.
///
/// Bledy (decyzja (a) z #463): wywolania silnika ida w zakresie rdb::ThrowOnFatalScope, wiec K1
/// (FatalError) i K4 (EnvironmentError) wychodza jako rdb::EngineFatal zamiast konczyc proces
/// hosta. Odmowy K2/K3 silnik zglasza wartoscia (rdb::Refusal) - fasada ich nie przerabia.
///
/// @note Obiekt jest NIEPRZENOSNY i NIEKOPIOWALNY celowo. storage trzyma surowy wskaznik na
///       MemoryStore tego silnika; przeniesienie Engine przesunieloby Memory Store i zostawiloby kazdy
///       zbudowany magazyn ze wskaznikiem na pustke.
class Engine {
 public:
  Engine()                          = default;
  Engine(const Engine &)            = delete;
  Engine &operator=(const Engine &) = delete;
  Engine(Engine &&)                 = delete;
  Engine &operator=(Engine &&)      = delete;
  ~Engine()                         = default;

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

 private:
  MemoryStore memory_;
};

}  // namespace rdb::embed
