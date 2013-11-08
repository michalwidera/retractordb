/*Copyright 2010-2014 Streams Lab Michal Widera

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
        
        SR sr;                      /**< Zbior - !posortowanych! Delt */

    	map < R , long > m_counters ;    /**< Tutaj jest mnoznik z dla wartosci czasu */

		R ctSlot ;	/**< Tutaj przechowywana jest wartosc aktualnego slotu czasowego
					ctslot = current time slot */

	public:

		/** Konstruktor wypenia dziedziczony zbior wartosciami odfiltrowanymi 
		w trakcie wywolania konstruktora toworzona jest lista zawierajaca
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
