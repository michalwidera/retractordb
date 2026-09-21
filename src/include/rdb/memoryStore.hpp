#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace rdb {

/// Pamiec magazynu MEMORY: rekordy, ich mapy NULL i licznik zapisow, indeksowane NAZWA
/// STRUMIENIA.
///
/// Wspoldzielenie po nazwie nie jest tu przypadkiem ani przeoczeniem - jest kontraktem.
/// W jednym planie piszacy i czytajacy ten sam strumien MEMORY to DWA rozne obiekty
/// memoryFile; gdyby kazdy trzymal wlasne wektory, czytajacy nie zobaczylby niczego.
/// Rzecz w tym, KTO jest wlascicielem tej pamieci. Do fazy 2 byly to trzy `static` w
/// faccmemory.cc, czyli caly proces: dwa niezalezne silniki w jednym procesie - a to jest
/// dokladnie ksztalt notatnika - dzielily stan bez slowa. Tutaj wlasciciel jest obiektem,
/// wiec da sie go podac.
///
/// Instancja domyslna procesu zachowuje dawne zachowanie dla wszystkich, ktorzy zadnej nie
/// podaja. Nowego wlasciciela wnosi sie swiadomie i wtedy izolacja jest pelna.
///
/// @note Bez synchronizacji, tak samo jak mapy, ktore zastepuje: magazyn MEMORY jednego
///       planu jest obslugiwany w watku przetwarzania. Wspoldzielenie jednego sklepu przez
///       watki wymagaloby zamka i nie jest przez te klase obiecane.
class MemoryStore {
 public:
  using Records = std::vector<std::vector<std::uint8_t>>;
  using Nulls   = std::vector<std::vector<bool>>;

  /// Rekordy strumienia; tworzy pusty wpis przy pierwszym siegnieciu.
  Records &records(const std::string &stream) { return records_[stream]; }
  /// Mapy NULL strumienia, indeksowane rownolegle do records().
  Nulls &nulls(const std::string &stream) { return nulls_[stream]; }
  /// Liczba zapisow append - logiczna dlugosc strumienia, niezalezna od bufora kolowego.
  std::size_t &writeCount(const std::string &stream) { return writeCount_[stream]; }

  /// Kasuje pamiec strumienia; odpowiednik write(nullptr, ...).
  void clear(const std::string &stream) {
    records_[stream].clear();
    nulls_[stream].clear();
    writeCount_[stream] = 0;
  }

  /// Sklep dzielony przez cały proces - domyslny wlasciciel dla wolajacych, ktorzy wlasnego
  /// nie podaja. Funkcja, nie zmienna globalna: inicjalizacja przy pierwszym uzyciu omija
  /// kolejnosc inicjalizacji statykow miedzy jednostkami translacji.
  static MemoryStore &processDefault();

 private:
  std::map<std::string, Records> records_;
  std::map<std::string, Nulls> nulls_;
  std::map<std::string, std::size_t> writeCount_;
};

}  // namespace rdb
