#pragma once

#include <sys/types.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace rdb {

/// Stan JEDNEGO strumienia MEMORY: rekordy, ich bitsety null i licznik zapisow - w jednym wezle mapy.
///
/// Do 2026-09-23 byly to TRZY mapy kluczowane ta sama nazwa, a kazdy dostep do rekordu placil
/// za wyszukanie w kazdej z nich osobno (read: trzy, write: do czterech). Callgrind przypisywal
/// samym `map::operator[]` 3,75 % instrukcji slotu na planie ADD-owym i 4,86 % na potoku EKG,
/// nie liczac udzialu w `__memcmp_avx2` na porownaniach kluczy. Jeden wezel na strumien znosi
/// caly ten rachunek: wyszukanie jest JEDNO i zapada w konstruktorze memoryFile.
struct memoryBucket {
  std::vector<std::vector<std::uint8_t>> data;
  std::vector<std::vector<bool>> nulls;
  std::size_t writeCount{0};  // Licznik zapisow - logiczna pozycja niezalezna od instancji.
  ssize_t recordSize{0};      // Rozmiar rekordow w `data` - read() kopiuje rekord w calosci.
  std::size_t users{0};       // Zywe instancje memoryFile; ostatnia kasuje kubelek.
};

/// Pamiec magazynu MEMORY: kubelki strumieni (memoryBucket), indeksowane NAZWA STRUMIENIA.
///
/// Wspoldzielenie po nazwie nie jest tu przypadkiem ani przeoczeniem - jest kontraktem.
/// W jednym planie piszacy i czytajacy ten sam strumien MEMORY to DWA rozne obiekty
/// memoryFile; gdyby kazdy trzymal wlasne wektory, czytajacy nie zobaczylby niczego.
/// Rzecz w tym, KTO jest wlascicielem tej pamieci. Do fazy 2 byla to mapa `static` w
/// faccmemory.cc, czyli caly proces: dwa niezalezne silniki w jednym procesie - a to jest
/// dokladnie ksztalt notatnika - dzielily stan bez slowa. Tutaj wlasciciel jest obiektem,
/// wiec da sie go podac.
///
/// Kubelek zyje tak dlugo, jak ostatnia instancja memoryFile, ktora go uzywa (licznik
/// `users`): strumien o tej samej nazwie w nastepnym planie zaczyna od zera, a nazwy
/// nieobecne w nowym planie nie zostaja w pamieci (#306, D3).
///
/// Instancja domyslna procesu zachowuje dawne zachowanie dla wszystkich, ktorzy zadnej nie
/// podaja. Nowego wlasciciela wnosi sie swiadomie i wtedy izolacja jest pelna.
///
/// @note Bez synchronizacji, tak samo jak mapa, ktora zastepuje: magazyn MEMORY jednego
///       planu jest obslugiwany w watku przetwarzania. Wspoldzielenie jednego sklepu przez
///       watki wymagaloby zamka i nie jest przez te klase obiecane.
class MemoryStore {
 public:
  /// Kubelek strumienia; tworzy pusty przy pierwszym siegnieciu. Wezly std::map sa stabilne -
  /// uniewaznia je wylacznie erase() tego klucza - wiec memoryFile moze zapamietac adres.
  memoryBucket &bucket(const std::string &stream) { return buckets_[stream]; }

  /// Kasuje kubelek strumienia; wola go destruktor ostatniej instancji memoryFile.
  void erase(const std::string &stream) { buckets_.erase(stream); }

  /// Czy sklep nie ma zadnego kubelka. Istnieje dla testu izolacji: pozwala stwierdzic, ze
  /// zapis trafil do TEGO sklepu, bez zgadywania klucza (sciezki pliku).
  [[nodiscard]] bool empty() const { return buckets_.empty(); }

  /// Liczba kubelkow - jedyny punkt obserwacji, ze wymiana planu nie zostawia kubelkow po
  /// strumieniach, ktorych juz nie ma.
  [[nodiscard]] std::size_t size() const { return buckets_.size(); }

  /// Sklep dzielony przez caly proces - domyslny wlasciciel dla wolajacych, ktorzy wlasnego
  /// nie podaja. Funkcja, nie zmienna globalna: inicjalizacja przy pierwszym uzyciu omija
  /// kolejnosc inicjalizacji statykow miedzy jednostkami translacji.
  static MemoryStore &processDefault();

 private:
  std::map<std::string, memoryBucket> buckets_;
};

}  // namespace rdb
