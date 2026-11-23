#include "CRSMath.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <limits>
#include <utility>

#include "rdb/error.hpp"
#include "rdb/rationalFormat.hpp"

using namespace CRationalStreamMath;

namespace {

using i128 = __int128;

constexpr i128 kInt64Max = std::numeric_limits<std::int64_t>::max();
constexpr i128 kIntMax   = std::numeric_limits<int>::max();
constexpr i128 kMsPerSec = 1000;

i128 gcd128(i128 a, i128 b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b != 0) {
    const i128 rest = a % b;
    a               = b;
    b               = rest;
  }
  return a;
}

double toSeconds(i128 num, i128 den) { return static_cast<double>(num) / static_cast<double>(den); }

}  // namespace

std::optional<std::int64_t> CRationalStreamMath::slotsAt(const TimePoint &t, const rational<int> &delta) {
  if (delta.numerator() == 0) return std::nullopt;
  // t / delta = (t.num * delta.den) / (t.den * delta.num). Oba iloczyny maja ponizej 2^95, wiec
  // __int128 liczy je dokladnie; skracanie krzyzowe w typie licznika przepelnialo sie po ok. 304 s
  // przy 1/48000 i 1/44100 (#640, krok 0 E).
  const i128 num = static_cast<i128>(t.numerator()) * delta.denominator();
  const i128 den = static_cast<i128>(t.denominator()) * delta.numerator();
  if (num % den != 0) return std::nullopt;
  const i128 quotient = num / den;
  if (quotient > kInt64Max || quotient < -kInt64Max - 1) return std::nullopt;
  return static_cast<std::int64_t>(quotient);
}

TimeLine::TimeLine(set<boost::rational<int>> const &inSet) {
  if (auto built = rebuild(inSet); !built) pending_ = std::move(built).error();
}

rdb::Result<> TimeLine::updateTimeIntervals(const set<boost::rational<int>> &inSet) {
  if (pending_.has_value()) return std::unexpected(*pending_);
  return rebuild(inSet);
}

rdb::Result<> TimeLine::rebuild(const set<boost::rational<int>> &inSet) {
  // Plan bez ani jednego strumienia odrzucaja wolajacy (Engine::compile, executorsm) - tu to niezmiennik.
  RDB_ASSERT(!inSet.empty(), "TimeLine: input interval set must not be empty");

  // Interwal, ktory jest naturalna wielokrotnoscia mniejszego, nie wnosi nowych chwil:
  // {1, 4, 1/2, 3/4} -> {1/2, 3/4}. val / x jest naturalne, gdy val.num * x.den dzieli sie przez
  // val.den * x.num - liczone w __int128, bez ilorazu w rational<int>.
  std::vector<rational<int>> kept;
  for (const auto &val : inSet) {
    RDB_ASSERT(val > 0, "TimeLine: interval {} is not positive", val);
    const bool isMultiple = std::ranges::any_of(inSet, [&val](const rational<int> &x) {
      return x < val &&
             (static_cast<i128>(val.numerator()) * x.denominator()) % (static_cast<i128>(val.denominator()) * x.numerator()) ==
                 0;
    });
    if (!isMultiple) kept.push_back(val);
  }

  // Podstawa tau = NWD licznikow / NWW mianownikow. Ulamek jest skrocony: liczba pierwsza dzielaca
  // NWD licznikow i ktorys mianownik dzielilaby licznik i mianownik tego samego skroconego interwalu.
  i128 tauNum = 0;
  i128 tauDen = 1;
  for (const auto &val : kept) {
    tauNum = gcd128(tauNum, val.numerator());
    tauDen = tauDen / gcd128(tauDen, val.denominator()) * val.denominator();
    if (tauDen > kInt64Max)
      return rdb::fail(rdb::Errc::Config,
                       std::format("time line: the least common multiple of the interval denominators exceeds int64 "
                                   "at interval {} s - the plan intervals have no common time base",
                                   val));
  }

  std::vector<Rate> rates;
  rates.reserve(kept.size());
  for (const auto &val : kept) {
    const i128 k = (val.numerator() / tauNum) * (tauDen / val.denominator());
    if (k > kInt64Max)
      return rdb::fail(rdb::Errc::Config,
                       std::format("time line: interval {} s is more than int64 steps of the common time base {}/{} s", val,
                                   static_cast<std::int64_t>(tauNum), static_cast<std::int64_t>(tauDen)));
    rates.push_back({.k = static_cast<std::int64_t>(k), .counter = 0});
  }

  // Ta sama chwila w nowej podstawie. Zbior interwalow tylko rosnie (import ad hoc dopisuje
  // wezly), wiec nowa podstawa dzieli stara i przeliczenie jest mnozeniem przez liczbe calkowita.
  i128 ticks = 0;
  if (ticks_ != 0) {
    if (tauNum_ % tauNum != 0 || tauDen % tauDen_ != 0)
      return rdb::fail(rdb::Errc::Logic,
                       std::format("time line: the new interval set drops a time base step of {}/{} s", tauNum_, tauDen_));
    const i128 scale = (static_cast<i128>(tauNum_) / tauNum) * (tauDen / tauDen_);
    if (scale > kInt64Max || static_cast<i128>(ticks_) * scale > kInt64Max)
      return rdb::fail(rdb::Errc::Config,
                       std::format("time line: the current slot {} s does not fit int64 steps of "
                                   "the new time base {}/{} s",
                                   ctSlot_, static_cast<std::int64_t>(tauNum), static_cast<std::int64_t>(tauDen)));
    ticks = static_cast<i128>(ticks_) * scale;
  }

  // Horyzont: INT_MAX slotow najszybszego interwalu (indeks rekordu jest int, #636), uciety tak,
  // zeby licznik chwili w sekundach (ticks * tauNum) i termin w ms (ticks * tauNum * 1000 / tauDen)
  // miescily sie w int64. Najszybszy interwal jest pierwszy w posortowanym zbiorze i nigdy nie odpada.
  i128 horizon = kIntMax * rates.front().k;
  horizon      = std::min(horizon, kInt64Max / tauNum);
  horizon      = std::min(horizon, kInt64Max * tauDen / (tauNum * kMsPerSec));
  if (ticks > horizon)
    return rdb::fail(rdb::Errc::Config,
                     std::format("time line: the current slot {} s is already past the time horizon {:.3f} s of "
                                 "the new interval set (fastest interval {} s)",
                                 ctSlot_, toSeconds(horizon * tauNum, tauDen), kept.front()));

  // Kazdy interwal zaczyna od pierwszego wystapienia SCISLE po biezacej chwili;
  // przy konstrukcji chwila jest zerem, wiec kazdy licznik startuje od 1.
  for (auto &rate : rates)
    rate.counter = static_cast<std::int64_t>(ticks / rate.k) + 1;

  rates_        = std::move(rates);
  tauNum_       = static_cast<std::int64_t>(tauNum);
  tauDen_       = static_cast<std::int64_t>(tauDen);
  ticks_        = static_cast<std::int64_t>(ticks);
  horizonTicks_ = static_cast<std::int64_t>(horizon);
  fastest_      = kept.front();
  return {};
}

bool TimeLine::isThisDeltaAwaitCurrentTimeSlot(const boost::rational<int> &inDelta) const {
  return slotsAt(ctSlot_, inDelta).has_value();
}

TimePoint TimeLine::horizon() const { return {horizonTicks_ * tauNum_, tauDen_}; }

rdb::Result<TimePoint> TimeLine::getNextTimeSlot() {
  if (pending_.has_value()) return std::unexpected(*pending_);
  RDB_ASSERT(!rates_.empty(), "TimeLine::getNextTimeSlot: internal stream rate set is empty");
  // Najblizsza chwila to minimum counter * k po interwalach; iloczyn w __int128, bo counter * k
  // moze przekroczyc horyzont (a z nim int64) o jeden krok interwalu.
  i128 next = static_cast<i128>(rates_.front().counter) * rates_.front().k;
  for (const auto &rate : rates_)
    next = std::min(next, static_cast<i128>(rate.counter) * rate.k);
  if (next > horizonTicks_) {
    const i128 limit = static_cast<i128>(horizonTicks_) * tauNum_;
    return rdb::fail(
        rdb::Errc::Config,
        std::format("time horizon of the plan reached: the next slot at {:.6f} s lies past the horizon "
                    "at {:.6f} s ({:.2f} h); the fastest interval {} s allows at most {} slots",
                    toSeconds(next * tauNum_, tauDen_), toSeconds(limit, tauDen_), toSeconds(limit, tauDen_) / 3600.0, fastest_,
                    static_cast<std::int64_t>(horizonTicks_ / rates_.front().k)));
  }
  ticks_ = static_cast<std::int64_t>(next);
  for (auto &rate : rates_)
    if (static_cast<i128>(rate.counter) * rate.k == next) ++rate.counter;
  ctSlot_ = TimePoint(ticks_ * tauNum_, tauDen_);
  return ctSlot_;
}
