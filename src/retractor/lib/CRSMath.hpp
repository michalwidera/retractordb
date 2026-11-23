#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <vector>

#include <boost/core/noncopyable.hpp>
#include <boost/rational.hpp>

#include "rdb/error.hpp"

namespace CRationalStreamMath {

using namespace boost;

using std::set;

/// Chwila na osi czasu w sekundach. Licznik i mianownik w int64, bo chwila slotu interwalu
/// o liczniku p > 1 ma licznik do p * INT_MAX (#640).
using TimePoint = rational<std::int64_t>;

/// Funkcja slotsAt powinna:
/// - [crsmath.slots-at.1~1] zwracać t / delta, gdy iloraz jest liczbą całkowitą mieszczącą się w int64, a nullopt, gdy nie jest całkowity, nie mieści się w int64 albo delta jest zerem - liczone dokładnie w __int128, bez skracania krzyżowego w typie licznika.
[[nodiscard]] std::optional<std::int64_t> slotsAt(const TimePoint &t, const rational<int> &delta);

/// @brief Klasa TimeLine wyznacza kolejne przedziały czasowe.
///
/// Oś liczy chwilę jako T taktów podstawy tau = NWD wszystkich interwałów (NWD liczników przez NWW
/// mianowników, #640). Każdy interwał to k * tau z k całkowitym, więc należność i przejście do
/// następnego slotu są działaniami na liczbach całkowitych; rachunek pośredni idzie w __int128.
/// Horyzont osi to INT_MAX slotów najszybszego interwału, bo indeks rekordu jest int (#636), dodatkowo
/// ucięty tak, żeby T * NWD liczników i termin w ms mieściły się w int64.
///
/// Obiekt klasy TimeLine powinien:
/// - [crsmath.time-line.1~1] być inicjalizowany niepustym zbiorem czasów między kolejnymi zdarzeniami.
/// - [crsmath.time-line.2~1] dostarczać funkcji zwracającej najbliższy kolejny termin w systemie odstępów czasów.
/// - [crsmath.time-line.3~1] być zabezpieczony przed kopiowaniem.
///   Sprawdzenie kompilacji: test/UnitTest/test_contracts.cpp - static_assert braku kopiowania i przypisania TimeLine.
/// - [crsmath.time-line.4~1] dostarczać funkcji sprawdzającej, czy dany odstęp czasowy jest oczekiwany w bieżącym przedziale czasowym.
/// - [crsmath.time-line.5~1] zamiast slotu za horyzontem osi zwracać błąd Config podający horyzont i najszybszy interwał, bez zmiany stanu osi.
/// - [crsmath.time-line.6~1] przy zmianie zbioru interwałów zachowywać dokładnie bieżącą chwilę i zaczynać nowy interwał od jego pierwszego wystąpienia po niej, a gdy podstawa osi albo chwila nie mieści się w int64, zwracać błąd zamiast liczyć.
/// - [crsmath.time-line.7~1] dla zbioru interwałów, którego podstawa nie mieści się w int64, zwracać błąd z pierwszego wywołania getNextTimeSlot.
/// - [crsmath.time-line.8~1] dostarczać funkcji statycznej horizonOf, zwracającej dla zbioru interwałów ten sam horyzont i najszybszy interwał, który miałaby zbudowana z niego oś, a błąd Config, gdy podstawa zbioru nie mieści się w int64.
/// - [crsmath.time-line.9~1] odpowiadać z horyzontu zwróconego przez horizonOf, czy chwila t powiększona o krok step leży nie dalej niż horyzont, dokładnie także wtedy, gdy t + step nie mieści się w int64.

class TimeLine : private boost::noncopyable {
  /// Interwal po odfiltrowaniu wielokrotnosci: k = interwal / tau i numer nastepnego wystapienia.
  struct Rate {
    std::int64_t k;
    std::int64_t counter;
  };

  std::vector<Rate> rates_;

  /** tau = tauNum_ / tauDen_ sekund, w postaci skroconej */
  std::int64_t tauNum_{1};
  std::int64_t tauDen_{1};

  /** biezaca chwila w taktach tau */
  std::int64_t ticks_{0};

  /** ostatnia dopuszczalna chwila w taktach tau */
  std::int64_t horizonTicks_{0};

  /** najszybszy interwal - wyznacza horyzont */
  rational<int> fastest_;

  /** biezaca chwila w sekundach */
  TimePoint ctSlot_{0};

  /** blad konstrukcji, oddawany z pierwszego getNextTimeSlot */
  std::optional<rdb::Error> pending_;

  /// Podstawa osi wyznaczona ze zbioru interwalow - wspolna dla rebuild i horizonOf.
  struct Base {
    std::vector<Rate> rates;
    std::int64_t tauNum{1};
    std::int64_t tauDen{1};
    std::int64_t horizonTicks{0};
    rational<int> fastest;
  };

  static rdb::Result<Base> computeBase(const set<rational<int>> &inSet);

  rdb::Result<> rebuild(const set<rational<int>> &inSet);

 public:
  /// Horyzont zbioru interwalow: ostatnia dopuszczalna chwila osi i interwal, ktory ja wyznacza.
  struct Horizon {
    TimePoint at;
    rational<int> fastest;
    /// Podstawa osi i horyzont w jej taktach - do dokladnego porownania w admits.
    std::int64_t tauNum{1};
    std::int64_t tauDen{1};
    std::int64_t ticks{0};

    /** Czy chwila t + step nie wychodzi za horyzont. Liczone w taktach tej podstawy w __int128;
     * suma t + step w TimePoint moglaby przepelnic int64. Chwila niewyrownana z podstawa jest
     * zaokraglana w gore, wiec odpowiedz bywa ostrozniejsza, nigdy zbyt laskawa.
     */
    [[nodiscard]] bool admits(const TimePoint &t, const rational<int> &step) const;
  };

  /** Horyzont osi, ktora powstalaby z danego zbioru interwalow - bez budowania osi. Sluzy
   * odmowie dolaczenia zapytania ad hoc, ktorego horyzont juz minal (#640).
   */
  [[nodiscard]] static rdb::Result<Horizon> horizonOf(const set<boost::rational<int>> &inSet);

  /** This constructor fills inherited set of values filtered
   * During call of constructor list with minimal times is created
   * Times that are multiplicity of other values are removed
   * Prepared list is then taken as argument by getNextTimeSlot function
   */
  explicit TimeLine(set<boost::rational<int>> const &inSet);
  TimeLine() = delete;

  /** Replace the set of stream intervals without rewinding the timeline.
   * Newly introduced rates start at their first occurrence after the current
   * time slot. This is required when an ad-hoc query introduces a new rate.
   * The new set must contain every interval of the old one.
   */
  [[nodiscard]] rdb::Result<> updateTimeIntervals(const set<boost::rational<int>> &inSet);

  /** Function return true if given delta is in current time slot
   * Note: This is not trivial
   */
  [[nodiscard]] bool isThisDeltaAwaitCurrentTimeSlot(const rational<int> &inDelta) const;

  /** This function computes next ctSlot value.
   * This value describe what is next time interval
   * that depends of series of deltas of time series data
   */
  [[nodiscard]] rdb::Result<TimePoint> getNextTimeSlot();

  /** Najszybszy interwal osi - ten, ktory wyznacza horyzont. */
  [[nodiscard]] const rational<int> &fastestInterval() const noexcept { return fastest_; }

  /** Biezaca chwila osi: ostatni slot zwrocony przez getNextTimeSlot, przed pierwszym 0. */
  [[nodiscard]] const TimePoint &now() const noexcept { return ctSlot_; }

  /** Horyzont osi w sekundach: ostatnia chwila, ktora getNextTimeSlot moze zwrocic. */
  [[nodiscard]] TimePoint horizon() const;
};

}  // namespace CRationalStreamMath
