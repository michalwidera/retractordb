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
*/
#include "stdafx.h"

#include "CRSMath.h"

namespace CRationalStreamMath
{

TimeLine::TimeLine( const SR & inSet ) :
    ctSlot( 0 )
{
    assert( inSet.size() > 0 );

    BOOST_FOREACH( R val , inSet )
    {
        bool isDivided( false );                // Przelacznik/zatrzask - lapie true jak val jest podzielny przez jakakolwiek liczbe ze zbioru
        BOOST_FOREACH( R x , inSet )            // Zadaniem tej funkcji jest wyznaczenie zbioru takiego ze i.e. {1, 4, 1/2, 3/4} -> { 1/2 , 3/4 }
                if ( x < val )                      // poniewaz 1/2 * 2 == 1 , 1 * 4 == 4 and 1/2 * 2 == 1 ale nie istnieje liczba naturalna taka ze 1/2 * NATURAL == 3/4
        {                                   // czyli usuniecie wszystkich wartosci ktore maja odpowiednik pomnozony przez liczbe naturalna
            if ( R ( rational_cast<int>( x / val ) , 1 ) == ( x / val ) )
                isDivided = true  ;
            if ( R ( rational_cast<int>( val / x ) , 1 ) == ( val / x ) )
                isDivided = true  ;
        }

        if ( isDivided == false )         //Jesli liczba nie jest podzielna zasilamy zbior sr
        {                                 //TYLKO TUTAJ JEST SR.INSERT
            sr.insert( val );             //Tutaj wstawiane sa tylko te delty do zboru sr ktore nie sa delta = delta * n.
            m_counters[ val ] = 1 ;       //inicjacja wartoscia 1
        }

    }
}

bool TimeLine::isThisDeltaAwaitCurrentTimeSlot( const R & inDelta )
{
    R value = ctSlot / inDelta ;
    return ( value.denominator() == 1 ) ;
}

const R & TimeLine::getNextTimeSlot()
{
    assert( sr.size() > 0 );

    //Tutaj odchodzi magia!
    BOOST_FOREACH( R val , sr )         //W konstruktorze ustalil sie zbior delt i indeksow
    {                                   //Najpierw przechodze przez ten zbior
        if ( m_counters[ val ] < 0 )	//jesli ktorakolwiek z wartosci skojarzonych z delta jest wartoscia ujemna (a to stanowi znacznik)
        {								//
            m_counters[ val ] *= -1 ;	//biore i przestawiam znacznik (*-1) ta wartosc jako kolejna do pobrania
            ++ m_counters[ val ] ;	    //biore nastepna ..
        }
    }

    ctSlot = *sr.begin() * m_counters[ *sr.begin() ] ;     //Zakladam ze nastepny slot czasowy to ten pierwszy z brzegu (czyli wartosc begin())

    BOOST_FOREACH( R val , sr )                     //Uwaga - te dwie petle nie moga byc polaczone (latwo)
            if ( ctSlot >  val * m_counters[ val ] )    //nastepnie przechodze przez sloty i wybieram najwiekszy
            ctSlot = val * m_counters[ val ] ;      //Wybierana jest najmniejsza wartosc!

    BOOST_FOREACH( R val , sr )                     //Faza zaznaczenia wybranych do realizacji (przelaczenie na -1)
            if ( val * m_counters[ val ] == ctSlot )    //Tutaj zaznaczam nastepne do wyboru przy kolejnej iterakcji (wysztkie pasujace ze zbioru)
            m_counters[ val ] *= -1 ;               //Ustawiaja znacznik wartosci na -1

    return ctSlot ;
}
}
