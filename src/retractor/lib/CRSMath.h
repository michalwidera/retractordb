#pragma once

#include <map>
#include <set>

#include <boost/core/noncopyable.hpp>
#include <boost/rational.hpp>

namespace CRationalStreamMath {

using namespace boost;

using std::map;
using std::set;

/**
 * This class is designed to time management and scheduling task
 * Note: This is complicated
 */
class TimeLine : private boost::noncopyable {
  /** Set of _SORTED_ deltas */
  set<rational<int>> sr_;

  /** Here is multiplier for time values */
  map<rational<int>, long> counter_;

  /** Here is actual value of time slot
   *  ctslot = current time slot
   */
  rational<int> ctSlot_;

 public:
  /** This constructor fills inherited set of values filtered
   * During call of constructor list with minimal times is created
   * Times that are multiplicity of other values are removed
   * Prepared list is then taken as argument by getNextTimeSlot function
   */
  explicit TimeLine(set<rational<int>> const &inSet);
  TimeLine() = delete;

  /** Function return true if given delta is in current time slot
   * Note: This is not trivial
   */
  bool isThisDeltaAwaitCurrentTimeSlot(const rational<int> &inDelta);

  /** This function computes next ctSlot value.
   * This value describe what is next time interval
   * that depends of series of deltas of time series data
   * Note: This is HARD
   */
  const rational<int> &getNextTimeSlot();
};

}  // namespace CRationalStreamMath
