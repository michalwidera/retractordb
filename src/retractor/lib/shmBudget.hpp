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
/// @requirements-exempt Space: Wynik pomiaru dostepnej przestrzeni pamieci dzielonej.
struct Space {
  bool known{false};
  std::uint64_t total{0};
  std::uint64_t available{0};
};

/// Funkcja space powinna:
/// - przy obiektach POSIX-owej pamięci dzielonej Boosta mierzyć pojemność i wolne miejsce (f_blocks i f_bavail razy f_frsize) przez fstatvfs na deskryptorze sondy shm_open o zerowej długości, utworzonej z O_EXCL i usuniętej przed pomiarem.
///   Test: `ShmBudget.MeasurementUsesAvailableBlocksAndFragmentSize`, `ShmBudget.SpaceRemovesItsProbe`, `ShmBudget.SpacePreservesExistingObjectOnProbeNameCollision`. Ograniczenie: sonda POSIX jest sprawdzana tylko z RDB_HAS_BOOST_POSIX_SHM.
/// - gdy sondy nie da się utworzyć albo zmierzyć, a także bez obiektów POSIX-owych w Boost.Interprocess, mierzyć statvfs na osplat::sharedMemoryBackingPath().
///   Test: `ShmBudget.MeasurementFallbackAndUnknownReportAreExplicit`.
/// - zwracać known == false wyłącznie wtedy, gdy zawiódł także pomiar zapasowy.
///   Test: `ShmBudget.MeasurementFallbackAndUnknownReportAreExplicit`.
///
/// @note Sciezka /dev/shm jest szczegolem libc, a w kontenerze rozmiar tmpfs ustawia --shm-size (domyslnie
///       64 MiB). known == false to "nie wiem", nie zero bajtow: wolajacy ma wtedy przepuscic operacje.
[[nodiscard]] Space space();

/// Funkcja responseQueueElements powinna:
/// - dla interval == 0 zwracać minElements.
///   Test: `ShmBudget.ElementsFollowTickAndFloor`.
/// - w pozostałych przypadkach zwracać bufferSeconds / interval liczone wymiernie i zaokrąglone w górę, nie mniej niż minElements.
///   Test: `ShmBudget.IntervalCostsGroupInPlanOrderAndSortByQueueBytes`.
///   Test: `ShmBudget.ElementsRoundUpOnFractionalFrequency`.
///
/// @param interval takt strumienia w sekundach.
///
/// @note Wspolne zrodlo dla executorsm (tworzy kolejke) i raportu (wycenia ja przed startem). Obciecie
///       czestotliwosci przed mnozeniem dawalo dla taktu 2/45 s 220 elementow zamiast 225.
[[nodiscard]] int responseQueueElements(const boost::rational<int> &interval, int bufferSeconds, int minElements);

/// Funkcja messageQueueBytes powinna:
/// - zwracać rozmiar segmentu boost::interprocess::message_queue o maxMessages wiadomościach po maxMessageSize bajtów, liczony wzorem samego Boosta (mq_hdr_t::get_mem_size) na bieżącej platformie.
///   Test: `ShmBudget.QueueFormulaMatchesRealSegment`.
///
/// @note Przypieta stala 208 zanizala kazda kolejke o 8 B na aarch64 (pthread_mutex_t 48 B zamiast 40 B).
///       Wzor jest przypiety testem jednostkowym do rozmiaru realnie utworzonej kolejki.
[[nodiscard]] std::uint64_t messageQueueBytes(std::uint64_t maxMessages, std::uint64_t maxMessageSize);

/// Funkcja responseQueueBytes powinna:
/// - zwracać messageQueueBytes(maxElements, ipc::kResponseQueueMaxMessageSize) - rozmiar kolejki odpowiedzi jednego subskrybenta.
///   Test: `ShmBudget.ResponseQueueUsesResponseMessageSize`.
[[nodiscard]] std::uint64_t responseQueueBytes(int maxElements);

/// Funkcja fixedReservationBytes powinna:
/// - zwracać sumę rozmiaru segmentu magistrali (bus::segmentBytes), kolejki komend (messageQueueBytes z ipc::kQueryQueueMaxMessages i ipc::kQueryQueueMaxMessageSize) i segmentu ipc::kShmemSegmentSize.
///   Test: `ShmBudget.FixedReservationIsSumOfItsParts`.
///
/// @note Rezerwacja niezalezna od liczby klientow. Segment magistrali jest wspolny dla przestrzeni nazw, wiec
///       dla instancji dolaczajacej sie do istniejacej magistrali to szacunek z gory.
[[nodiscard]] std::uint64_t fixedReservationBytes();

/// Koszt subskrypcji strumieni o jednym takcie. Strumienie o rownym takcie maja rowna kolejke,
/// wiec raport grupuje je razem -- plan z pieciodziesieciu wezlow ma zwykle dwa lub trzy takty.
/// @requirements-exempt IntervalCost: Dane kosztu kolejek dla interwalu, uzywane w raporcie budzetu.
struct IntervalCost {
  boost::rational<int> interval;
  int elements{0};
  std::uint64_t bytes{0};
  std::vector<std::string> streams;
};

/// Funkcja intervalCosts powinna:
/// - grupować węzły planu poza dyrektywami kompilatora według taktu i dla każdego taktu zwracać liczbę elementów z responseQueueElements, rozmiar z responseQueueBytes i nazwy strumieni w kolejności planu.
///   Test: `ShmBudget.IntervalCostsGroupInPlanOrderAndSortByQueueBytes`.
/// - porządkować grupy malejąco po rozmiarze kolejki.
///   Test: `ShmBudget.IntervalCostsGroupInPlanOrderAndSortByQueueBytes`.
///
/// @note Dyrektyw nie da sie subskrybowac komenda `show`, wiec nie maja kolejki.
[[nodiscard]] std::vector<IntervalCost> intervalCosts(const qTree &plan, int bufferSeconds, int minElements);

/// Funkcja humanBytes powinna:
/// - zwracać "<n> B" poniżej 1 KiB, a od 1 KiB, 1 MiB i 1 GiB wartość z jednym miejscem po przecinku w KiB, MiB albo GiB (potęgi 1024).
///   Test: `ShmBudget.HumanBytesPicksUnit`.
[[nodiscard]] std::string humanBytes(std::uint64_t bytes);

/// Funkcja report powinna:
/// - zwracać tekst z systemem plików pod osplat::sharedMemoryBackingPath(), jego pojemnością i wolnym miejscem albo "unknown (statvfs failed)", gdy space() nie zna stanu.
///   Test: `ShmBudget.MeasurementFallbackAndUnknownReportAreExplicit`.
///   Test: `ShmBudget.ReportSubtractsReservationAndRoundsClientCountDown`.
/// - podawać rezerwację stałą z rozbiciem na segment magistrali, kolejkę komend i segment odpowiedzi, a przy znanym stanie także wolne miejsce po rezerwacji (0, gdy rezerwacja go nie mieści).
///   Test: `ShmBudget.ReportSubtractsReservationAndRoundsClientCountDown`.
/// - dla każdego taktu z intervalCosts podawać takt, liczbę elementów, rozmiar kolejki na klienta, liczbę mieszczących się klientów (wolne po rezerwacji / rozmiar kolejki, "?" przy nieznanym stanie) i strumienie.
///   Test: `ShmBudget.MeasurementFallbackAndUnknownReportAreExplicit`.
///   Test: `ShmBudget.ReportSubtractsReservationAndRoundsClientCountDown`.
/// - dla planu bez strumieni do subskrypcji kończyć raport zdaniem "plan has no subscribable streams".
///   Test: `ShmBudget.MeasurementFallbackAndUnknownReportAreExplicit`.
///
/// @note Zapas liczony po rezerwacji stalej i z gory, bo raport ma nie obiecywac miejsca.
[[nodiscard]] std::string report(const qTree &plan, int bufferSeconds, int minElements);

}  // namespace shmbudget
