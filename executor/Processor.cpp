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
#include "Processor.h"
#include "Buffer.h"

using namespace QS ;

extern "C" qTree coreInstance ;
extern "C" config configInstance ;


namespace QS {


    Processor::Processor()
    {
        using namespace UR ;
        //Ta funkcja inicjuje mape tworzac w cbuff strumienie archiwum
        BOOST_FOREACH ( query q , coreInstance )
        {
            if ( storage.find(q.id) == storage.end() )
                storage[ q.id ] =
                boost::shared_ptr< UR::dbStream >( new dbStream( q.id , q.getSchema() ) ) ;
            else
               ; //strumien o takiej nazwie juz istnieje w systemie

            //inicjacja kontenera dla plikowych zrodel danych
            if ( ! q.filename.empty() )
                gFileMap[ q.id ] = inputDF( q.filename.c_str() , q.lSchema );
        }

    }

    void Processor::saveStreamsToFile( const string filename )
    {
        UR::saveStreamsToFile( filename );
    }

    void Processor::processRows( set < string > inSet )
    {
        bool again( false );

        BOOST_FOREACH ( query q , coreInstance )
        {
            //Odrzucamy wiersze ktore nie wchodza tym razem do przeliczeñ
            if ( inSet.find( q.id ) == inSet.end() )
                continue ;

            //Deklaracje powinny byc obslugiwane inaczej (random i do pliku?)
            //tutaj pobierac dane z pliku? TODO
            if ( q.isDeclaration() )
            {
                if ( q.filename.empty() )
                    continue ;                      //tutaj ida randomy i nie trzeba niczego pobierac z pliku
                else
                    gFileMap[ q.id ].processRow() ; //pobranie kolejnego wiersza z pliku posiadajacego schemat
            }

            //tutaj powinny byc wyliczane wartosci krotek strumienia
            //i zawartosc powinna byc zapisywana do pliku

            int cnt(0) ;
            BOOST_FOREACH( field  & f , q.lSchema )
            {
                bool resultValidFlag ;
                complex< R > value( getCRValue( f , q , resultValidFlag ) ) ;
                if ( resultValidFlag )
                {
                    (*storage[ q.id ])[ cnt ] = value ;
                }
                else
                {
                    again = true ;
                }
                cnt ++ ;
            }

            //Zapisanie wyliczonych danych do cbuffora - na dysk
            storage[ q.id ]->store() ;
        }
    }

    /* retval-out */
    complex< R > Processor::getValueByOffset( 
        const string query_name ,   /* in */
        int schemaOffset ,          /* in */
        int TimeOffset ,            /* in */
        bool & resultValid          /* out */
        )    
    {
        const query & q ( getQuery( query_name ) ) ;
        int i ( 0 );

        if ( schemaOffset >= static_cast<int>( q.lSchema.size() ) )
        {
            TimeOffset += ( schemaOffset + 1 ) % q.lSchema.size()  ;
            schemaOffset %= q.lSchema.size() ;
        }

        BOOST_FOREACH ( QS::field f , q.lSchema )
        {
            if ( schemaOffset == i )
            {
                complex< R > result ;
                if ( TimeOffset == 0 )
                    result = getCRValue( f , q , resultValid ) ;
                else
                {
                    result = boost::get< complex< R > >( storage[ query_name ]->get( TimeOffset )[ i ] );  
                    resultValid = storage[ query_name ]->resultValid ;
                }

                return result ;
            }
            else
                i ++ ;
        }

        throw std::out_of_range("Thrown/Processor.cpp/getValueByOffset/Brak takiego pola w zaptyaniu");
        return complex<R>(0) ;  //Proforma
    }

    string Processor::getRowValue( const string query_name )
    {
        const string SEPARATOR = "\t" ;
        const string INVALID   = "(NA)" ;

        stringstream retVal ;
        BOOST_FOREACH( field  f , getQuery( query_name ).lSchema )
        {
            bool resultValidFlag ;
            complex< R > value( getCRValue( f , getQuery( query_name ) , resultValidFlag ) ) ;
            if ( resultValidFlag )
            {
                if ( value.imag() == 0 )
                {
                    if ( value.real().denominator() == 1 )
                        retVal << value.real().numerator() ;
                    else
                        retVal << value.real() ;
                }
                else
                    retVal << value ;
            }
            else
                retVal << INVALID ;
            retVal << SEPARATOR ;
        }
        return retVal.str() ;
    }



    /* Poniewaz pole wyznaczane jest w ramach pewnego schematu/zapytania 
    musi ono miec dostp do tego zapytania
    dlatego parametrem jest referencja do tego zapytania
    - gownanie rozwiazanie - trzeba to zrobic ladniej
    */
    complex< R > Processor::getCRValue( 
        const field & f ,
        const query & q ,
        bool & resultValid /* return information */
        ) 
    {
        resultValid = true ;        //Ustawiany jest domyslenie - wszystko idzie ok ...

        stack < complex< R > > rStack ;
        complex< R > a , b ;

        BOOST_FOREACH( token tk , f.lProgram )
        {         
            if ( ! resultValid )  //Jesli gdzies po drodze trafimy NULL - wypad z baru, a potem wrocic ponownie za chwilke
                break ;           //Sprawdzic! - TODO

            switch( tk.getTokenCommand() )
            {
            case ADD:
            case SUBTRACT:
            case MULTIPLY:
            case DIVIDE:	

                a = rStack.top() ;
                rStack.pop() ;
                b = rStack.top() ; 
                rStack.pop() ;
            }

            switch( tk.getTokenCommand() )
            {
            case PUSH_VAL:

                rStack.push( tk.getCRValue() );
                break ;

            case ADD:

                rStack.push ( a + b ) ;
                break ;

            case SUBTRACT:

                rStack.push ( a - b ) ;
                break ;

            case MULTIPLY:

                rStack.push ( a * b ) ;
                break ;

            case DIVIDE:	
                //Dzielenie wymiernych liczb urojonych zachowuje siê niezdefiniowanie 
                //- kompilator wylatuje na rationalach
                //http://pl.wikipedia.org/wiki/Liczby_zespolone
                {
                    complex< R > arg1( a ) ;
                    complex< R > arg2( b ) ;

                    R a = arg1.real() ;
                    R b = arg1.imag() ;
                    R c = arg2.real() ;
                    R d = arg2.imag() ;

                    if ( c == 0 && d == 0 )
                    {           
                        complex < R > divVal ( 0 , 0 ); //Proba dzielenia przez zero
                        rStack.push ( divVal ) ;        //zablokowana - ustawian jest flaga 
                        resultValid = false ;           //- resultValid na false
                    }
                    else
                    {
                        complex < R > divVal (
                            ( ( a * c ) + ( b * d ) ) /
                            ( c * c ) + ( d * d )
                            ,
                            ( ( b * c ) - ( a * d ) ) /
                            ( c * c ) + ( d * d )
                            ) ;                
                        rStack.push ( divVal ) ;
                    }
                }
                break ;

            case NEGATE:

                //Procedura negacji na stosie
                a = rStack.top() ;
                rStack.pop() ;
                rStack.push ( - a ) ;
                break ;

            case CALL:

            {
                a = rStack.top() ;
                rStack.pop() ;

                double real = (double) a.real().numerator() / (double) a.real().denominator() ;
                double imag = (double) a.imag().numerator() / (double) a.imag().denominator() ;

                if ( tk.getValue() == "floor" )
                {
                    rStack.push
                            ( complex < R >
                              (
                                  Rationalize( floor ( real ) ) ,
                                  Rationalize( floor ( imag ) )
                                  )
                              );
                }
                else if ( tk.getValue() == "ceil" )
                {
                    rStack.push
                            ( complex < R >
                              (
                                  Rationalize( ceil ( real ) ) ,
                                  Rationalize( ceil ( imag ) )
                                  )
                              );
                }
                else if ( tk.getValue() == "sqrt" )
                {
                    rStack.push
                            ( complex < R >
                              (
                                  Rationalize( sqrt ( real ) ) ,
                                  Rationalize( sqrt ( imag ) )
                                  )
                              );
                }
                else
                    throw std::out_of_range("brak wsparcia dla tego typu funkcji matematycznej");

            }
                break;

            case PUSH_ID3:	//Nazwa_pola
            case PUSH_ID1:	//Nazwa_schematu.Nazwa_pola
            case PUSH_ID2:	//Nazwa_schematu[indeks]
            case PUSH_ID4:	//Nazwa_schematu[indeks,indeks]

                throw std::out_of_range("Thrown/Processor.cpp/getCRValue/Te pole nie wystapi bo jest usuwane w trakcie kompilacji");
                break ;

            case PUSH_ID:	//Nazwa_schematu[indeks,indeks] jak PUSH_ID4
                {
                    assert( tk.getCRValue().real().denominator() == 1 );
                    assert( tk.getCRValue().imag().denominator() == 1 );


                    const int offsetInSchema ( rational_cast<int>( tk.getCRValue().real() ) );
                    const int offsetInTime ( rational_cast<int>(  tk.getCRValue().imag() ) );
                    const string schema( tk.getValue() );

                    //assercja - sprawdzenie (lub na kompilatorze)
                    //odrzucajce proby wywolania martwych petli

                    assert( schema != q.id );

                    rStack.push( 
                        getValueByOffset( schema , offsetInSchema , offsetInTime , resultValid )
                        );
                }

                break ;             

            default:
                throw std::out_of_range("Thrown/Processor.cpp/getCRValue/Trafiono na nieznany token");
            }
        }

        if ( rStack.size() == 0 )
        {
            if ( q.filename.empty() )
                return complex<R>( rand()%2 ) ;    //jesli puste - leca randomy, generatory, itp
            else
                return gFileMap[ q.id ].getCR( f ) ;
        }
        else if ( rStack.size() == 1 ) return rStack.top() ;
        else
            throw std::out_of_range("Thrown/Processor.cpp/getCRValue/Na stosie pozostalo za duzo tokenow");

        return complex<R>(0) ; /* pro forma */
    }

}
