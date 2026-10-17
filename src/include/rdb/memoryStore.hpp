#pragma once

#include <sys/types.h>  // ssize_t

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace rdb {

/// Stan JEDNEGO strumienia magazynu MEMORY: rekordy, ich bitsety null i licznik zapisów - w jednym
/// węźle mapy.
///
/// Do 2026-09-23 były to TRZY mapy kluczowane tą samą nazwą, a każdy dostęp do rekordu płacił
/// za wyszukanie w każdej z nich osobno (read: trzy, write: do czterech). Callgrind przypisywał
/// samym `map::operator[]` 3,75 % instrukcji slotu na planie ADD-owym i 4,86 % na potoku EKG,
/// nie licząc udziału w `__memcmp_avx2` na porównaniach kluczy. Jeden węzeł na strumień znosi
/// cały ten rachunek: wyszukanie jest JEDNO i zapada w konstruktorze memoryFile.
/// @requirements-exempt memoryBucket: Agregat rekordow i licznikow magazynu MEMORY; operacje realizuje memoryFile.
struct memoryBucket {
  std::vector<std::vector<uint8_t>> data;
  std::vector<std::vector<bool>> nulls;
  size_t writeCount{0};   // Licznik zapisów - logiczna pozycja niezależna od instancji.
  ssize_t recordSize{0};  // Rozmiar rekordów w `data` - read() kopiuje rekord w całości.
  size_t users{0};        // Żywe instancje memoryFile; ostatnia kasuje kubełek.
};

/// Właściciel pamięci magazynu MEMORY: kubełki strumieni indeksowane NAZWĄ STRUMIENIA.
///
/// Współdzielenie po nazwie jest kontraktem, nie przeoczeniem: w jednym planie piszący i czytający
/// ten sam strumień MEMORY to DWA różne obiekty memoryFile; gdyby każdy trzymał własne wektory,
/// czytający nie zobaczyłby niczego. Rzecz w tym, KTO jest właścicielem tej pamięci. Do fazy 2
/// (#446, C2) była to mapa `static` w faccmemory.cc, czyli cały proces: dwa niezależne silniki w
/// jednym procesie - a to jest dokładnie kształt notatnika - dzieliły stan bez słowa. Tutaj
/// właściciel jest obiektem, więc da się go podać.
///
/// Instancja domyślna procesu zachowuje dawne zachowanie dla wszystkich, którzy żadnej nie podają
/// (xretractor, xtrdb). Nowego właściciela wnosi się świadomie i wtedy izolacja jest pełna.
///
/// @note Bez synchronizacji, tak samo jak mapa, którą zastępuje: magazyn MEMORY jednego planu jest
///       obsługiwany w wątku przetwarzania. Współdzielenie jednego Memory Store przez wątki wymagałoby
///       zamka i nie jest przez tę klasę obiecane.
/// Obiekt klasy MemoryStore powinien:
/// - [memorystore.memory-store.1~1] przechowywać stan magazynów MEMORY jako jeden kubełek na nazwę strumienia, tworzony pusty przy pierwszym sięgnięciu przez bucket.
/// - [memorystore.memory-store.2~1] zwracać przy kolejnych wywołaniach bucket dla tej samej nazwy ten sam kubełek, tak aby piszący i czytający ten sam strumień w jednym planie widzieli te same rekordy.
/// - [memorystore.memory-store.3~1] zapewniać ważność referencji zwróconej przez bucket do wywołania erase dla tej nazwy, niezależnie od dodawania i usuwania innych kubełków.
/// - [memorystore.memory-store.4~1] przez erase usuwać kubełek wraz z nazwą, a przez size i empty raportować liczbę istniejących kubełków.
/// - [memorystore.memory-store.5~1] nie współdzielić kubełków między różnymi instancjami MemoryStore, także dla strumieni o tych samych nazwach.
/// - [memorystore.memory-store.6~1] przez processDefault udostępniać jedną instancję na proces, inicjowaną przy pierwszym użyciu, dla wołających, którzy nie podają własnego MemoryStore.
/// - [memorystore.memory-store.7~1] nie zapewniać synchronizacji dostępu z wielu wątków.
///   Nietestowalne: brak obietnicy bezpieczenstwa wielowatkowego, a nie zachowanie do wywolania; uzycie rownolegle ma niezdefiniowane zachowanie.
class MemoryStore {
 public:
  /// Kubełek strumienia; tworzy pusty przy pierwszym sięgnięciu. Węzły std::map są stabilne -
  /// referencja jest ważna do erase() tej nazwy.
  memoryBucket &bucket(const std::string &stream) { return buckets_[stream]; }

  /// Kasuje kubełek strumienia razem z kluczem - woła go ostatnia instancja memoryFile.
  void erase(const std::string &stream) { buckets_.erase(stream); }

  [[nodiscard]] size_t size() const { return buckets_.size(); }
  [[nodiscard]] bool empty() const { return buckets_.empty(); }

  /// Memory Store dzielony przez cały proces - domyślny właściciel dla wołających, którzy własnego nie
  /// podają. Funkcja, nie zmienna globalna: inicjalizacja przy pierwszym użyciu omija kolejność
  /// inicjalizacji statyków między jednostkami translacji.
  static MemoryStore &processDefault();

 private:
  std::map<std::string, memoryBucket> buckets_;
};

}  // namespace rdb
