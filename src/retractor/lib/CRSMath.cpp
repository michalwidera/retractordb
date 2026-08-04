#include "CRSMath.hpp"

#include "fatalError.hpp"

using namespace CRationalStreamMath;

TimeLine::TimeLine(set<boost::rational<int>> const &inSet) : ctSlot_(0) { rebuild(inSet); }

void TimeLine::updateTimeIntervals(const set<boost::rational<int>> &inSet) { rebuild(inSet); }

void TimeLine::rebuild(const set<boost::rational<int>> &inSet) {
  if (inSet.empty()) FatalError("TimeLine: input interval set must not be empty");
  set<rational<int>> newRates;
  map<rational<int>, long> newCounters;
  for (auto val : inSet) {
    // Latch - catch true if val is divided
    // bu any number from the set
    bool isDivided(false);
    for (auto x : inSet) {
      if (x < val) {
        // Main task of this function is crate set as
        // {1, 4, 1/2, 3/4} -> { 1/2 , 3/4 }
        // because 1/2 * 2 == 1 , 1 * 4 == 4 and 1/2 * 2 == 1
        // but does not exist natural number that 1/2 * NATURAL = 3/4
        // other words: removing all values that have
        // second value multiplicate by natural number
        if (boost::rational<int>(rational_cast<int>(x / val), 1) == (x / val)) isDivided = true;
        if (boost::rational<int>(rational_cast<int>(val / x), 1) == (val / x)) isDivided = true;
      }
    }
    if (!isDivided) {
      // Start each rate at its first occurrence strictly after the current
      // slot. For construction ctSlot_ is zero, so every counter starts at 1.
      const auto elapsed = ctSlot_ / val;
      newRates.insert(val);
      newCounters[val] = static_cast<long>(elapsed.numerator() / elapsed.denominator()) + 1;
    }
  }
  sr_      = std::move(newRates);
  counter_ = std::move(newCounters);
}

bool TimeLine::isThisDeltaAwaitCurrentTimeSlot(const boost::rational<int> &inDelta) {
  if (inDelta == boost::rational<int>(0)) return false;
  boost::rational<int> value = ctSlot_ / inDelta;
  return (value.denominator() == 1);
}

// MAGIC Warning

const boost::rational<int> &TimeLine::getNextTimeSlot() {
  if (sr_.empty()) FatalError("TimeLine::getNextTimeSlot: internal stream rate set is empty");
  // In constructor we were set deltas and indexes
  // Take first value from tje edge
  // even good we can take max rational here.
  ctSlot_ = *sr_.begin() * boost::rational<int>(counter_[*sr_.begin()]);
  // Note: These two loops cannot be mixed together!
  // Find lowest time slot in set
  // time slots are valued delta * counter
  for (auto val : sr_) {
    if (ctSlot_ > val * boost::rational<int>(counter_[val])) ctSlot_ = val * boost::rational<int>(counter_[val]);
  }
  // Increase (+1) lowest time slots
  for (auto val : sr_) {
    if (ctSlot_ == val * boost::rational<int>(counter_[val])) ++counter_[val];
  }
  return ctSlot_;
}
