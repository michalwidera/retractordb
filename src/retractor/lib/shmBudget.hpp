#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/rational.hpp>

class qTree;

/// @brief Budzet pamieci dzielonej: ile obiekty IPC silnika zajmuja w /dev/shm i ile tam miejsca jest.
///
/// Modul istnieje po to, zeby liczba bajtow byla wyliczana w JEDNYM miejscu. Rozmiar kolejki
/// odpowiedzi wchodzi do trzech niezaleznych decyzji -- strazy przy subskrypcji, raportu z
/// kompilacji i oceny, czy instancja w ogole ma gdzie wystartowac -- a rozjazd miedzy nimi nie
/// jest bledem kompilacji, tylko odmowa w jednym miejscu przy zgodzie w drugim.
namespace shmbudget {

/// Stan systemu plikow, na ktorym leza obiekty pamieci dzielonej.
///
/// `known == false` znaczy "nie udalo sie ustalic", i jest to stan ROZNY od zera bajtow:
/// wolajacy ma wtedy przepuscic operacje, a nie odmowic. Odmowa na podstawie nieudanego
/// pomiaru zatrzymalaby prace tam, gdzie miejsca moze byc pod dostatkiem.
struct Space {
  bool known{false};
  std::uint64_t total{0};
  std::uint64_t available{0};
};

/// Pojemnosc i wolne miejsce systemu plikow obslugujacego shm_open().
///
/// Pomiar idzie przez fstatvfs() na deskryptorze obiektu, ktory naprawde powstal, a nie przez
/// statvfs("/dev/shm"): sciezka jest szczegolem implementacji libc, a w kontenerze rozmiar tego
/// tmpfs ustawia --shm-size i bywa maly (domyslnie 64 MiB). Gdy sonda nie przejdzie, zostaje
/// statvfs("/dev/shm") jako droga zapasowa.
[[nodiscard]] Space space();

/// Liczba elementow kolejki odpowiedzi dla strumienia o takcie `interval` [s].
/// Wspolne zrodlo dla executorsm (tworzy kolejke) i raportu (wycenia ja przed startem).
[[nodiscard]] int responseQueueElements(const boost::rational<int> &interval, int bufferSeconds, int minElements);

/// Rozmiar segmentu boost::interprocess::message_queue o zadanej pojemnosci.
/// Wzor jest przypiety testem jednostkowym do rozmiaru realnie utworzonej kolejki.
[[nodiscard]] std::uint64_t messageQueueBytes(std::uint64_t maxMessages, std::uint64_t maxMessageSize);

/// Kolejka odpowiedzi jednego subskrybenta: messageQueueBytes z rozmiarem wiadomosci odpowiedzi.
[[nodiscard]] std::uint64_t responseQueueBytes(int maxElements);

/// Rezerwacja niezalezna od liczby klientow: segment magistrali, kolejka komend, segment mapy.
/// Segment magistrali jest wspolny dla maszyny (a scislej: dla przestrzeni nazw), wiec dla
/// instancji dolaczajacej sie do istniejacej magistrali jest to szacunek z gory.
[[nodiscard]] std::uint64_t fixedReservationBytes();

/// Koszt subskrypcji strumieni o jednym takcie. Strumienie o rownym takcie maja rowna kolejke,
/// wiec raport grupuje je razem -- plan z pieciodziesieciu wezlow ma zwykle dwa lub trzy takty.
struct IntervalCost {
  boost::rational<int> interval;
  int elements{0};
  std::uint64_t bytes{0};
  std::vector<std::string> streams;
};

/// Koszty subskrypcji planu, malejaco po rozmiarze kolejki.
[[nodiscard]] std::vector<IntervalCost> intervalCosts(const qTree &plan, int bufferSeconds, int minElements);

/// Rozmiar w postaci czytelnej dla czlowieka (B / KiB / MiB / GiB).
[[nodiscard]] std::string humanBytes(std::uint64_t bytes);

/// Raport budzetu planu: rezerwacja stala, stan systemu plikow i koszt subskrypcji per takt.
[[nodiscard]] std::string report(const qTree &plan, int bufferSeconds, int minElements);

}  // namespace shmbudget
