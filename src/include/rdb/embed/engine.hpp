#pragma once

#include <memory>
#include <string_view>

#include "rdb/memoryStore.hpp"
#include "rdb/storage.hpp"

/// Warstwa L2 architektury osadzania (docs/embedded-roadmap.md sekcja 2): fasada C++, ktorej
/// uzywaja WSZYSTKIE trzy cele - nanobind dla Pythona, ABI C dla Swifta i JNI dla Androida.
/// Jedno miejsce na polityke CYKLU ZYCIA, zeby kazdy z nich nie wymyslal jej po swojemu.
namespace rdb::embed {

/// Jedna instancja silnika w procesie hosta.
///
/// Powod istnienia jest jeden i jest to teza calej fazy 2: demon buduje swoj stan RAZ, wiec
/// "stan procesu" i "stan silnika" sa w nim nieodroznialne. Notatnik buduje go przy kazdym
/// uruchomieniu komorki i wtedy ta roznica staje sie bledem - dwie komorki dzielily pamiec
/// magazynu MEMORY po samej nazwie strumienia, nic o sobie nie wiedzac.
///
/// DZIS Engine trzyma tylko sklep MEMORY, bo tylko tyle stanu warstwa osadzalna jeszcze ma
/// (patrz bramka embedding_boundary). Klasa powstaje mimo to teraz, a nie przy fazie 3, bo
/// wlascicielstwo jest pytaniem, na ktore trzeba odpowiedziec raz: compile(), step() i rows()
/// z etapu J1 dopisza sie do ISTNIEJACEGO obiektu, zamiast wymuszac wymyslenie go pod presja
/// zupelnie innego problemu.
///
/// @note Obiekt jest NIEPRZENOSNY i NIEKOPIOWALNY celowo. storage trzyma surowy wskaznik na
///       MemoryStore tego silnika; przeniesienie Engine przesunieloby sklep i zostawiloby
///       kazdy zbudowany magazyn ze wskaznikiem na pustke.
class Engine {
 public:
  Engine()                          = default;
  Engine(const Engine &)            = delete;
  Engine &operator=(const Engine &) = delete;
  Engine(Engine &&)                 = delete;
  Engine &operator=(Engine &&)      = delete;
  ~Engine()                         = default;

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

 private:
  MemoryStore memory_;
};

}  // namespace rdb::embed
