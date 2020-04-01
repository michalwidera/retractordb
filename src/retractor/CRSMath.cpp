#include "CRSMath.h"
#include <assert.h>  // for assert

using namespace CRationalStreamMath ;

TimeLine::TimeLine( const set< boost::rational<int>> inSet ) :
    ctSlot( 0 ) {
    assert( inSet.size() > 0 );

    for( auto val : inSet ) {
        // Latch - catch true if val is divided
        // bu any number from the set
        bool isDivided( false );

        for( auto x : inSet ) {
            if ( x < val ) {
                // Main task of this function is crate set as
                // {1, 4, 1/2, 3/4} -> { 1/2 , 3/4 }
                // because 1/2 * 2 == 1 , 1 * 4 == 4 and 1/2 * 2 == 1
                // but does not exist natural number that 1/2 * NATURAL = 3/4
                // other words: removing all values that have
                // second value muliplicated by natural number
                if ( boost::rational<int> ( rational_cast<int>( x / val ), 1 ) == ( x / val ) ) {
                    isDivided = true  ;
                }

                if ( boost::rational<int> ( rational_cast<int>( val / x ), 1 ) == ( val / x ) ) {
                    isDivided = true  ;
                }
            }
        }

        if ( isDivided == false ) {
            // If number is not divided we add sr set
            // ONLY HERE IS SR.INSERT
            // Here we insert only theses deltas to se set which are not delta = delta * n
            sr.insert( val );
            counter[ val ] = 1 ;
        }
    }
}

bool TimeLine::isThisDeltaAwaitCurrentTimeSlot( boost::rational<int> &inDelta ) {
    boost::rational<int> value = ctSlot / inDelta ;
    return ( value.denominator() == 1 ) ;
}

// MAGIC Warnning

boost::rational<int> &TimeLine::getNextTimeSlot() {
    assert( sr.size() > 0 );
    // In constructor we were set deltas and indexes
    // Take first value from tje edge
    // even good we can take maxrational here.
    ctSlot = *sr.begin() * boost::rational<int>(counter[ *sr.begin() ]) ;

    // Note: These two loops cannot be mixed together!

    // Find lowest time slot in set
    // time slots are valued delta * counter

    for( auto val : sr ) {
        if ( ctSlot > val * boost::rational<int>(counter[ val ]) ) {
            ctSlot = val * boost::rational<int>(counter[ val ]) ;
        }
    }

    // Increase (+1) lowest time slots

    for( auto val : sr ) {
        if ( ctSlot == val * boost::rational<int>(counter[ val ]) ) {
            ++ counter[ val ] ;
        }
    }

    return ctSlot ;
}
