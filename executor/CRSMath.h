/*Copyright 2010-2012 Streams Lab Michal Widera

  RESTRICTED RIGHTS LEGEND
  Use, duplication or disclosure by any commercial industry (public or
  private), private individual or by any Government Agency without an
  expressed written consent of release from Streams Lab Michal Widera
  is subject to restriction of the Rights.

  Manufacturer is:
      Streams Lab Michal Widera
      Reymonta str. 56/1
      Poland, Zabrze, 41-800
      (+48) 606 995 918

$Rev:: 2             $:  Revision of last commit
$Author:: developer  $:  Author of last commit
$Date:: 2012-07-15 0#$:  Date of last commit
$Id:: CRSMath.h 2 20#$:  Id of last commit
*/
#pragma once

#include <set>
#include <map>

namespace CRationalStreamMath {

	using namespace boost ;

	using std::set ;
	using std::map ;

    /**
	* Ta klasa sluzy do zarzadania czasem dla problemu szeregowania zadan
	* uwaga: Skomplikowane
	*/
    class TimeLine :  private boost::noncopyable
	{ 
        typedef set< R > SR ;
        
        SR sr;                      /**< Zbiór - !posortowanych! Delt */

    	map < R , long > m_counters ;    /**< Tutaj jest mnoznik z dla wartosci czasu */

		R ctSlot ;	/**< Tutaj przechowywana jest wartosc aktualnego slotu czasowego
					ctslot = current time slot */

	public:

		/** Konstruktor wypenia dziedziczony zbior wartosciami odfiltrowanymi 
		w trakcie wywo³ania konstruktora toworzona jest lista zawierajaca
		minimalne czasy, a usuwane sa te ktore sa wielokrotnoscia innych wartosci
		Przygotowana lista jest nastepnie brana pod uwage podczas wywolywania
		getNextTimeSlot	*/
		TimeLine( const SR & inSet ) ;

		/** Funkcja zwraca true jestli dana wartosc delta nalezy do
		aktualnie wykonywanego time-slota 
		to nie jest prosta procedura */
		bool isThisDeltaAwaitCurrentTimeSlot( const R & inDelta ) ;

		/** Ta funkcja wyznacza kolejna wartosc ctSlot 
		wartosc ta okresla jaki jest koleny interwal czasu
		wynikajacy z szeregu delt strumieni danych 
		skomplikowanosc algorytmu - wysoka */
		const R & getNextTimeSlot() ;
	};
}
