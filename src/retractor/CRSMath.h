#pragma once

#include <set>
#include <map>
#include <boost/rational.hpp>
#include <boost/core/noncopyable.hpp>

namespace CRationalStreamMath
{

    using namespace boost ;

    using std::set ;
    using std::map ;

    /**
     * This class is designed to time management and sheduling task
     * Note: This is complicated
     */
    class TimeLine :  private boost::noncopyable
    {

        /** Set of _SORTED_ deltas */
        set< rational<int>> sr;

        /** Here is multiplier for time values */
        map < rational<int>, long > counter ;

        /** Here is actual value of time slot
         *  ctslot = current time slot
         */
        rational<int> ctSlot ;

    public:

        /** This constructor fills inherited set of values filtered
          * During call of constructor list with minimal times is created
          * Times that are multiplicity of other values are removed
          * Prepared list is then taken as argument by getNextTimeSlot function
          */
        TimeLine(const set< rational<int>> inSet) ;

        /** Function return true if given delta is in current time slot
          * Note: This is not trivial
          */
        bool isThisDeltaAwaitCurrentTimeSlot(rational<int> &inDelta) ;

        /** This function computes next ctSlot value.
          * This value describe what is next time interval
          * that depends of series of deltas of time series data
          * Note: This is HARD
          */
        rational<int> &getNextTimeSlot() ;
    };

} // namespace
