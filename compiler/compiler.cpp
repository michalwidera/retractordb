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

#include "QStruct.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

using namespace QS ;
using boost::lexical_cast;

extern "C" {
qTree coreInstance ;
config configInstance ;
}



R deltaHash( const R & arg1 , const R & arg2 )
{
    clog << "DehashDiv\n" ;

    return ( arg1 * arg2 ) / ( arg1 + arg2 );
}

R deltaDehashDiv( const R & arg1 , const R & arg2 )
{
    clog << "DehashDiv\n" ;

    if ( arg1 == arg2 )
        throw std::out_of_range("Delta are equal in DehashDiv - undefinied.");

    return (arg1*arg2)/abs(arg1-arg2);
}

//
// a[n] = c[n+ceil(((n+1)*delta_a)/delta_b)]
// delta_a = (delta_c*delta_b)/abs(delta_c-delta_b)
//


R deltaDehashMod( const R & arg1 , const R & arg2 )
{
    clog << "DehashMod\n" ;

    if ( arg1 == arg2 )
        throw std::out_of_range("Delta are equal in DehashMod - undefinied.");

    return (arg1*arg2)/abs(arg1-arg2);
}

//
// b[n] = c[n+floor((n*delta_b)/delta_a)]
// delta_b = (delta_c*delta_a)/abs(delta_c-delta_a)
//


R deltaSubstract( const R & arg1 , const R & arg2  )
{
    clog << "Substract\n" ;

    return arg2;
}

R deltaAdd( const R & arg1 , const R & arg2 )
{
    clog << "Add\n" ;

    return min( arg1 , arg2 ) ;
}

R deltaTimemove( const R & arg1 , const R & arg2 )
{
    clog << "Timemove\n" ;

    return arg1 ;
}

/** Ta procedura wyznacza odstepy czasowe (delta) strumieni wygenerowanych */
string intervalCounter()
{
    clog << "intervalCounter()" << endl ;

    //using namespace CRationalStreamMath ;

    while ( true )
    {
        bool bOnceAgain( false );
        coreInstance.sort();
        BOOST_FOREACH ( query & q , coreInstance )      //dla kazdego zapytania
        {
            if ( q.lProgram.size() == 0 )
                continue ;	/* Declaration */

            if ( q.lProgram.size() == 1  )
            {
                if ( q.rInterval == 0 )
                {
                    token tInstance( * ( q.lProgram.begin() ) );
                    q.rInterval = coreInstance.getDelta( tInstance.getValue() ) ;
                }
                continue ;	/* Just one stream */
            }

            assert( q.lProgram.size() == 3 ) ;

            list < token > loc = q.lProgram ;

            token t1(*loc.begin()) ; loc.pop_front();
            token t2(*loc.begin()) ; loc.pop_front();
            token op(*loc.begin()) ; loc.pop_front();

            R delta(-1) ;

            switch ( op.getTokenCommand() )
            {

            case STREAM_HASH:

            {
                R delta1 = coreInstance.getDelta( t1.getValue() ) ;
                R delta2 = coreInstance.getDelta( t2.getValue() ) ;

                if ( delta1 == 0 || delta2 == 0 )
                {
                    bOnceAgain = true ;
                    continue ;
                }
                delta = deltaHash ( delta1 , delta2 ) ;
            }
                break ;

            case STREAM_DEHASH_DIV:

            {
                R delta1 = coreInstance.getDelta( t1.getValue() ) ;
                R delta2 = t2.getCRValue().real() ; //Tutaj nie ma drugiego strumienia tylko od razu argument

                assert( delta2 != 0 );

                if ( delta1 == 0 )
                {
                    bOnceAgain = true ;
                    continue ;
                }
                delta = deltaDehashDiv ( delta1 , delta2 ) ;
            }
                break ;

            case STREAM_DEHASH_MOD:

            {
                R delta1 = coreInstance.getDelta( t1.getValue() ) ;
                R delta2 = t2.getCRValue().real() ; //Tutaj nie ma drugiego strumienia tylko od razu argument

                assert( delta2 != 0 );

                if ( delta1 == 0 )
                {
                    bOnceAgain = true ;
                    continue ;
                }
                delta = deltaDehashMod ( delta1 , delta2 ) ;
            }
                break ;

            case STREAM_SUBSTRACT:

            {
                //R delta1 = coreInstance.getDelta( t1.getValue() ) ; ignored
                R delta1 = coreInstance.getDelta( t1.getValue() ) ;
                if ( delta1 == 0 )
                {
                    bOnceAgain = true ;
                    continue ;
                }
                delta = deltaSubstract ( 0 , delta1 ) ;
            }
                break ;

            case STREAM_ADD:

            {
                R delta1 = coreInstance.getDelta( t1.getValue() ) ;
                R delta2 = coreInstance.getDelta( t2.getValue() ) ;

                if ( delta1 == 0 || delta2 == 0 )
                {
                    bOnceAgain = true ;
                    continue ;
                }
                delta = deltaAdd ( delta1 , delta2 ) ;
            }
                break ;

            case STREAM_TIMEMOVE:

            {
                R delta1 = coreInstance.getDelta( t1.getValue() ) ;

                if ( delta1 == 0 )
                {
                    bOnceAgain = true ;
                    continue ;
                }
                delta = deltaTimemove ( delta1 , 0 ) ;
            }
                break ;

            case STREAM_AGSE:
            {   // ->>> check need
                // core1@(5,3) ->
                // push_stream core0 -> delta_zrodla
                // push_val 5 -> szerokosc_okna
                // stream agse 3 -> skok_okna
                R delta_zrodla =  coreInstance.getDelta( t1.getValue() ) ;
                R dlugosc_schematu_zrodla = getQuery( t1.getValue() ).getSchema().size() ;
                R szerokosc_okna =  t2.getCRValue().real() ;
                R skok_okna =  op.getCRValue().real() ;
                delta = delta_zrodla ;
                delta /= dlugosc_schematu_zrodla ;
                delta *= szerokosc_okna ;
                delta /= skok_okna ;

                clog << "wyznaczona delta z agse:" << delta << endl ;
                /*
                  Wnioskowanie

                .5
                 1 2 3 4 5
                 6 7 8 9 0
                 a a a a a

                5 dlugosc_schematu_zrodla

                delta -> .5 / 5  = 0.1

                 1
                 2
                 3
                 4

                szerokosc_okna = 5

                 1 2 3 4 5
                 6 7 8 9 0

                skok_okna = 2

                 1 2 3 4 5
                 2
                 3 4 5 6 8
                 4
                 5 6 7 8 9
                 6

                  */
            }
                break ;

            default:

                throw std::out_of_range("Niezdefiniowany token/polecenie na liscie");

            } //switch ( op.getTokenCommand() )

            assert( delta != -1 );

            q.rInterval = delta;  //Tutaj jest ustalana wartosc delta - wartosc zwrotna

        }  //BOOST_FOREACH ( query & q , coreInstance )

        if ( ! bOnceAgain )
            break ;
        else
            coreInstance.sort();

    } //while(true)

    coreInstance.sort();
    return string("OK") ;
}

string generateStreamName( string sName1 , string sName2 , command_id cmd )
{
    string sOperation( "undefinied" );

    switch ( cmd )
    {
#define DEF_CASE( _A_ ) case _A_ : sOperation = #_A_ ; break ;
#include "tokendefset.h"
#undef DEF_CASE
    }

    return sOperation + string ( "_" ) + sName2 + string ( "_" ) + sName1 ;
}

/*Celem dzialania tej procedury jest doprowadzenie do kanonicznej postaci
zbioru strumieni danych */
string simplifyLProgram()
{
    clog << "simplifyLProgram() - start" << endl ;

    coreInstance.sort();
    for (
         qTree::iterator it = coreInstance.begin() ;
         it != coreInstance.end();
         ++it
         )
    {
        //optymalizacja faza agse
        token t0,t1,t2 ;
        for (
             list < token >::iterator it2 = (*it).lProgram.begin() ;
             it2 != (*it).lProgram.end() ;
             it2 ++
             )
        {
            t0 = t1 ;
            t1 = t2 ;
            t2 = (*it2);
            if ( t2.getStrTokenName() == "STREAM_AGSE" &&
                 t1.getStrTokenName() == "PUSH_VAL" &&
                 t0.getStrTokenName() == "PUSH_VAL"
                 )
            {
                token newVal( t2.getTokenCommand() , t1.getCRValue() , t1.getValue() );
                it2 = (*it).lProgram.erase( it2 );
                it2 -- ;
                it2 = (*it).lProgram.erase( it2 );
                (*it).lProgram.insert( it2 , newVal );
                it2 = (*it).lProgram.begin() ;
                break;
            }
        }

        //optymalizacja faza 2
        if ( (*it).isReductionRequired() )
        {
            for (
                 list < token >::iterator it2 = (*it).lProgram.begin() ;
                 it2 != (*it).lProgram.end() ;
                 it2 ++
                 )
            {
                if (
                        (*it2).getStrTokenName() != "PUSH_STREAM" &&
                        (*it2).getStrTokenName() != "PUSH_VAL"
                        )
                {
                    query newQuery ;
                    string
                            //oper,   //< Nazwa operacji
                            arg1,   //< Nazwa pierwszego argumentu w operacji
                            arg2 ;  //< Nazwa drugiego argumentu w operacji

                    command_id cmd ;

                    {
                        token newVal( *it2 );
                        newQuery.lProgram.push_front( newVal );

                        //oper = ( *it2 ).getStrTokenName();
                        cmd = ( *it2 ).getTokenCommand() ;

                        it2 = (*it).lProgram.erase( it2 );
                        it2 -- ;
                    }
                    {
                        token newVal( *it2 );
                        newQuery.lProgram.push_front( newVal );

                        stringstream s ;
                        s << ( *it2 ).getValue() ;
                        arg1 =  string(s.str()) ;

                        it2 = (*it).lProgram.erase( it2 );
                        it2 -- ;
                    }
                    {
                        token newVal( *it2 );
                        newQuery.lProgram.push_front( newVal );

                        stringstream s ;
                        s << ( *it2 ).getValue() ;
                        arg2 =  string(s.str()) ;

                        it2 = (*it).lProgram.erase( it2 );
                    }

                    list < token > lSchema;
                    lSchema.push_back( token( PUSH_TSCAN )  );
                    newQuery.lSchema.push_back( field( "*" , lSchema , field::BAD ) ) ;

                    //newQuery.id = oper + string ( "_" ) + arg2 + string ( "_" ) + arg1 ;

                    newQuery.id = generateStreamName( arg1 , arg2 , cmd );

                    (*it).lProgram.insert( it2 , token( PUSH_STREAM , newQuery.id ) );

                    coreInstance.push_back( newQuery );

                    it = coreInstance.begin() ;

                    break ;
                }   //Endif
            }   //Endfor
        }  //Endif
    }  //Endfor

    clog << "simplifyLProgram() - stop" << endl ;

    return string("OK") ;
}

//celem tej procedury jest utworzenie rozwiniecie schematów w oparciu o dana komende
list < field > combine( string sName1 , string sName2 , command_id cmd )
{
    clog << "combine()::" << sName1 << "," << sName2 << endl ;

    list < field > lRetVal ;

    //Merge schematów dla przypadku z³¹czenia typu hash
    if ( cmd == STREAM_HASH )
    {
        if (
                getQuery( sName1 ).lSchema.size() !=
                getQuery( sName2 ).lSchema.size()
                )
            throw std::invalid_argument("hash operation require same schematics");

        //cerr << sName1 << "-" << sName2 << endl ;

        lRetVal = getQuery( sName1 ).lSchema ;

//check this!!!
//        list < field >::iterator it = lRetVal.begin() ;
//        BOOST_FOREACH ( field f1 , getQuery( sName1 ).lSchema )
//        {
//            BOOST_FOREACH( const string & s , f1.setFieldName )
//                    (*it).setFieldName.insert( s ) ;
//            it ++ ;
//        }

    }

    else if ( cmd == STREAM_DEHASH_DIV )
    {
        lRetVal = getQuery( sName1 ).lSchema ;
    }

    else if ( cmd == STREAM_DEHASH_MOD )
    {
        lRetVal = getQuery( sName1 ).lSchema ;
    }

    else if ( cmd == STREAM_ADD )
    {
        lRetVal = getQuery( sName1 ).lSchema ;
        BOOST_FOREACH ( field f , getQuery( sName2 ).lSchema )
                lRetVal.push_back( f );
    }

    else if ( cmd == STREAM_SUBSTRACT )
    {
        lRetVal = getQuery( sName1 ).lSchema ;
    }

    else if ( cmd == STREAM_TIMEMOVE )
    {
        lRetVal = getQuery( sName1 ).lSchema ;
    }

    else if ( cmd == STREAM_AGSE )
    {
        //rozwiniecie schematu dla agse - do dyskusji czy tak to zrobic
        list < field > schema ;
        for ( int i = 0 ; i < lexical_cast<int>(sName2)  ; i++ )
        {
            field intf ;
            intf.setFieldName.insert( sName1 + "_" + lexical_cast<string>(i) );
            schema.push_back( intf );
        }
        lRetVal = schema ;
    }
    else
        throw std::invalid_argument("not supported command on stack");


    //Tutaj sa dodawne do pol metody wykonania poprzez
    //referencje do pozycji wzgledem schematu
    int offset( 0 ) ;
    BOOST_FOREACH ( field & f , lRetVal )
    {
        stringstream s ;
        if ( cmd == STREAM_HASH )
            s << sName1 ;
        else if ( cmd == STREAM_TIMEMOVE )
            s << sName1 ;
        else if ( cmd == STREAM_AGSE )
            s << sName1 ;
        else
            s << generateStreamName( sName2 , sName1 , cmd ) ;
        s << "[" ;
        s << offset ++ ;
        s << "]" ;

        if ( f.lProgram.size() > 0 ) f.lProgram.pop_front();
        f.lProgram.push_front( token( PUSH_ID2 , s.str() ) );
    }

    return lRetVal ;
}


//celem tej procedury jest ustalenie wszystkich mozliwych nazw pol, oraz rozwiniecie *
//niestety algorytm jest skopany - bo nie przeszukuje w gore a kolejno.
//i moga sie trafic * ktore maja jako argumenty dwie gwiazdki i wtedy sie nie rozwija
//a executor glupieje
string prepareFields()
{
    clog << "prepareFields()" << endl ;

    coreInstance.tsort();
    BOOST_FOREACH ( query & q , coreInstance )
    {
        BOOST_FOREACH ( token & t , q.lProgram )
        {
            assert( q.lProgram.size() < 4 ) ;
            //Padniêcie tej assercji oznacza to ze nie wszystkie strumienie
            //sa ju¿ po operacji optymalizacji

            BOOST_FOREACH ( field & f , q.lSchema )
            {
                if ( f.getFirstFieldName() == "*" )
                {
                    //found! - a teraz rozwijamy
                    if ( q.lProgram.size() == 1 )
                    {
                        //upewniamy sie ze jedynym tokenem w takim przypadku jest push_stream
                        assert( ( * q.lProgram.begin() ).getTokenCommand() == PUSH_STREAM ) ;

                        //przepisanie listy pol z jednego do drugiego
                        q.lSchema = getQuery( t.getValue() ).lSchema ;
                        break ;
                    }

                    if ( q.lProgram.size() == 3 )
                    {
                        list < token >::iterator eIt = q.lProgram.begin();

                        string sName1  = ( * eIt ++ ).getValue() ;
                        string sName2  = ( * eIt ++ ).getValue() ;
                        command_id cmd = ( * eIt ++ ).getTokenCommand() ;

                        q.lSchema = combine( sName1 , sName2 , cmd ) ;

                        break ;
                    }

                    if ( q.lProgram.size() == 2 )
                    {
                        list < token >::iterator eIt = q.lProgram.begin();

                        string sName1  = ( * eIt ++ ).getValue() ;
                        command_id cmd = ( * eIt ++ ).getTokenCommand() ;

                        q.lSchema = combine( sName1 , "" , cmd ) ;
                        break ;
                    }
                }
            }
        }
    }
    coreInstance.sort();
    return string("OK") ;
}

/*Celem tej funkcji jest przetlumaczenie wszystkich odowolan do pol
na forme nazwa_schematu[pozycja, ofset czasowy]
ujednolicony sposob prezentacji umozliwia proste dalsze przetwarzanie
celem tej procedury jest zamiana wszystkich wystapien push_idXXX ba push_id
przy czym push_id jest najblizesz znaczeniowo push_id4
push_idXXX jest poszukiwane we wsztstkich programach tworzenia pol po redukcji.
*/
string convertReferences()
{
    clog << "convertReferences()" << endl ;

    boost::regex xprFieldId4("(\\w*)\\[(\\d*)\\,(\\d*)\\]");  //ala_1_ma_koda[1,1]
    boost::regex xprFieldId2("(\\w*)\\[(\\d*)\\]");         //ala_1_ma_koda[1]
    boost::regex xprFieldId1("(\\w*).(\\w*)");              //ala_1_ma_koda.w_schemacie
    boost::regex xprFieldId3("(\\w*)");                     //pole_kukurydzy

    BOOST_FOREACH ( query & q , coreInstance )      //dla kazdego zapytania
    {
        assert( ! q.isReductionRequired() ); //Przed wywolaniem tej funkcji wymagana jest redukcja

        BOOST_FOREACH ( field & f , q.lSchema )     //dla kazdego pola w zapytaniu
        {
            BOOST_FOREACH ( token & t , f.lProgram )//dla kazdego tokenu w polu zapytania
            {
                const command_id cmd( t.getTokenCommand() );
                const string text( t.getValue() );

                boost::cmatch what;

                clog << "cmd:" << t.getStrTokenName() << "," ;
                clog << "text:" << text << endl ;

                switch ( cmd )
                {
                case PUSH_ID1:

                    if ( regex_match(text.c_str(),what,xprFieldId1) )
                    {
                        assert( what.size() == 3 );

                        const string schema( what[1] );
                        const string field( what[2] );

                        //celem tej procedury jest odnalezienie schematu, potem pola w schemacie - a potem wstaiwamy
                        BOOST_FOREACH ( query & q1 , coreInstance )
                        {
                            if ( q1.id == schema )
                            {
                                int offset1( 0 );
                                BOOST_FOREACH ( QS::field & f1 ,  q1.lSchema )
                                {
                                    clog << "a.field:" << *f1.setFieldName.begin() << endl ;

                                    if ( f1.setFieldName.find( field ) != f1.setFieldName.end() )
                                    {
                                        t = token( PUSH_ID , complex< R > ( offset1 , 0 ) , schema ) ;
                                        break ;
                                    }
                                    else
                                        offset1 ++ ;
                                }

                                if ( offset1 == q1.lSchema.size() )
                                    throw std::out_of_range("ref-converstion - schema found, field not found");

                                break ;
                            }
                        }
                    }
                    else
                        throw std::out_of_range("no match in ref-converstion ID1");

                    break ;

                case PUSH_ID2:

                    if ( regex_match(text.c_str(),what,xprFieldId2) )
                    {
                        assert( what.size() == 3 );

                        const string schema( what[1] );
                        const string sOffset1( what[2] );
                        const int offset1( atoi( sOffset1.c_str() ) ) ;

                        t = token( PUSH_ID , complex< R > ( offset1 , 0 ) , schema ) ;
                    }
                    else
                        throw std::out_of_range("no match in ref-converstion ID2");

                    break ;

                case PUSH_ID3:

                    if ( regex_match(text.c_str(),what,xprFieldId3) )
                    {
                        assert( what.size() == 2 );
                        const string field( what[1] );

                        list < token >::iterator eIt = q.lProgram.begin();

                        string schema1 , schema2 ;
                        command_id cmd ;

                        query *pQ1 ( NULL ) , *pQ2 ( NULL );

                        if ( q.lProgram.size() == 1 )
                        {
                            schema1  = ( * eIt ).getValue() ;       //push stream - argument
                            cmd = ( * eIt ).getTokenCommand() ;
                            assert( cmd == PUSH_STREAM ) ;

                            pQ1 = & getQuery( schema1 ) ;
                        }
                        if ( q.lProgram.size() == 2 )
                        {
                            schema1  = ( * eIt ++ ).getValue() ;    //push stream - strumien
                            cmd = ( * eIt ).getTokenCommand() ;     //polecenie

                            pQ1 = & getQuery( schema1 ) ;
                        }
                        if ( q.lProgram.size() == 3 )
                        {
                            schema1  = ( * eIt ++ ).getValue() ;    //push stream - strumien
                            schema2  = ( * eIt ++ ).getValue() ;    //push stream - strumien
                            cmd = ( * eIt ).getTokenCommand() ;     //polecenie

                            pQ1 = & getQuery( schema1 ) ;
                            pQ2 = & getQuery( schema2 ) ;
                        }

                        bool bFieldFound( false );

                        int offset1( 0 );
                        if ( pQ1 != NULL )
                        {
                            offset1 = 0;
                            BOOST_FOREACH( QS::field & f1 , (*pQ1).lSchema )
                            {
                                clog << "b.field:" << *f1.setFieldName.begin() << endl ;

                                if ( f1.setFieldName.find( field ) != f1.setFieldName.end() )
                                {
                                    t = token( PUSH_ID , complex< R > ( offset1 , 0 ) , schema1 ) ;
                                    bFieldFound = true ;
                                }
                                offset1 ++ ;
                            }
                        }

                        if ( pQ2 != NULL && bFieldFound == false )
                        {
                            offset1 = 0 ;
                            BOOST_FOREACH( QS::field & f2 , (*pQ2).lSchema )
                            {
                                clog << "c.field:" << *f2.setFieldName.begin() << endl ;

                                if ( f2.setFieldName.find( field ) != f2.setFieldName.end() )
                                {
                                    t = token( PUSH_ID , complex< R > ( offset1 , 0 ) , schema2 ) ;
                                    bFieldFound = true ;
                                }
                                offset1 ++ ;
                            }
                        }

                        if ( bFieldFound == false )
                            throw std::logic_error("Brak pola o danej nazwie w schemacie strumienia");
                    }
                    else
                        throw std::out_of_range("no match in ref-converstion ID3");

                    break ;

                case PUSH_ID4:

                    if ( regex_match(text.c_str(),what,xprFieldId4) )
                    {
                        assert( what.size() == 4 );

                        const string schema( what[1] );
                        const string sOffset1( what[2] );
                        const string sOffset2( what[3] );
                        const int offset1( atoi( sOffset1.c_str() ) ) ;
                        const int offset2( atoi( sOffset2.c_str() ) ) ;

                        t = token( PUSH_ID , complex< R > ( offset1 , offset2 ) , schema ) ;
                    }
                    else
                        throw std::out_of_range("no match in ref-converstion ID4");

                    break ;
                }
            }
        }
    }

    return string("OK");
}

extern string parser( string sInputFile, string sOutputFile );
extern string parser_v2( string sPlikDanych, string sPlikWynikowy );

void dumpInstance( std::string sOutFile )
{
    //These object must be const
    const config configInstance2( configInstance ) ;
    const qTree coreInstance2( coreInstance ) ;

    std::ofstream ofs( sOutFile.c_str() );
    boost::archive::text_oarchive oa(ofs);

    oa << configInstance2 ;
    oa << coreInstance2 ;

    cerr << "Dump:      \t" << sOutFile << endl ;
}

std::streambuf *oldbuf = NULL ;

int main(int argc, char* argv[])
{
    //Intro
    cerr << "Parser/Compiler - parser and reductor of query plans.\n";
    string revision(  "$Rev: 2 $" );  //This string is svn keyword
    std::replace( revision.begin() , revision.end() , '$' , ' ' );
    revision.erase( std::remove( revision.begin() , revision.end() , ' ' ) , revision.end()) ; //erase-remove idiom
    cerr <<
            "used compiler tag:" + string(COMPILER) +
            "," + revision <<
            ", CTS:" << string( __TIMESTAMP__) <<	//http://msdn2.microsoft.com/en-us/library/b0084kay(VS.80).aspx
            std::endl ;

    try {
        scoped_ptr<ostream> p_ofs;

        namespace po = boost::program_options;

        string sOutputFile ;
        string sInputFile ;

        po::options_description desc("Allowed options");
        desc.add_options()
                ("help,h", "program options")
                ("queryfile,q" , po::value<string>(&sInputFile)->default_value("query.txt"), "query input file")
                ("outfile,o", po::value<string>(&sOutputFile)->default_value("query.qry"), "output data file")
                ("regtest,t","regression test call")
                ("parser2,v","use second parser")
                ("log,g","use log for debug queries")
                ;

        po::positional_options_description p;       //Assume that infile is the first option
        p.add("infile", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            cout << desc << "\n";
            return 1;
        }

        if (vm.count("log"))
        {
            p_ofs.reset( new ofstream("compiler.log") ) ;
            if ( ! p_ofs ) throw std::out_of_range("no log file created");
            clog.rdbuf( p_ofs->rdbuf());
            cout << "Log file active\n";
            clog << "Start\n";
        }
        else
        {
            clog.rdbuf(0);  //metoda na wylaczenie clog
        }

        if (vm.count("regtest"))
        {
            cout << "regression test procedure call" << "\n";
            if ( vm.count("parser2") )
                return ( parser_v2( "regtest" , "regtest" ) != "OK" );
            else
                return ( parser( "regtest" , "regtest" ) != "OK" );
            return 1; //proforma fail
        }

        string sQueryFile = vm["queryfile"].as< string >() ;    //plik z zapytaniamia
        string sOutFile   = vm["outfile"].as< string >() ;      //default query.qry

        //defaults
        string sLinkFile  = "query.lkn" ;                       //ten plik sluzy do laczenia parsera z kompilatorem
        string sSubsetFile= "query.sub"  ;                      //nazwa pliku typu subset

        //override defaults if filename is matching regexp
        {
            boost::regex filenameRe("(\\w*).(\\w*)");  //nazwapliku.rozszerzenie
            boost::cmatch what;
            if ( regex_match(sQueryFile.c_str(),what,filenameRe) )
            {
                string filenameNoExt = string( what[1] ) ;
                sLinkFile = filenameNoExt + ".lkn" ;
                sSubsetFile = filenameNoExt + ".sub" ;
            }
        }

        //sprawdzenie czy query file zawiera w pierwszej linii subset[beg,end]
        //ten sub jest po to zeby dalo sie testowaæ rozne przypadki
        //a plik query.txt byl zawsze w repozytorium
        {
            string str;
            ifstream input( sQueryFile.c_str() );
            getline( input, str ) ;

            boost::cmatch what;

            boost::regex mulitestRe("set\\[(\\d*)\\,(\\d*)\\]");  //multiset[2-7]
            if ( regex_match(str.c_str(),what,mulitestRe) )
            {
                sQueryFile = sSubsetFile ;
                ofstream querySubset( sQueryFile.c_str() );

                const int dGivenSetBeg( atoi( string( what[1] ).c_str() ) ) ;
                const int dGivenSetEnd( atoi( string( what[2] ).c_str() ) ) ;

                input.seekg( 0 , ios_base::beg );

                int lineNr(0);
                list < string > querySet ;
                while( getline( input, str ) )
                {
                    if ( lineNr >= dGivenSetBeg && lineNr <= dGivenSetEnd )
                        querySubset << str << "\n" ;
                    lineNr ++ ;
                }
                cerr << "Subset ...\t" << dGivenSetBeg << "-" << dGivenSetEnd << endl ;
            }
        }
        if ( vm.count("parser2") )
        {
            cerr << "Parsing v2 ...\t" << parser_v2( sQueryFile , sLinkFile ) << endl ;
        }
        else
        {
            cerr << "Parsing v1 ...\t" << parser( sQueryFile , sLinkFile ) << endl ;
        }

        //
        // Main Algorithm body
        //

        std::ifstream ifs( sLinkFile.c_str() , std::ios::binary);
        boost::archive::text_iarchive ia(ifs);
        ia >> configInstance ;
        ia >> coreInstance ;

        string sOutFileLog1   = vm["outfile"].as< string >() + ".lg1" ;      //default query.qry
        dumpInstance( sOutFileLog1 );

        int dAfterParserSize((int)coreInstance.size());

        cerr << "Reduction: \t" << simplifyLProgram() << endl ;

        string sOutFileLog2   = vm["outfile"].as< string >() + ".lg2" ;      //default query.qry
        dumpInstance( sOutFileLog2 );

        cerr << "Intervals: \t" << intervalCounter() << endl ;
        cerr << "Schema:    \t" << prepareFields() << endl ;
        cerr << "References:\t" << convertReferences() << endl ;

        int dAfterCompilingSize((int)coreInstance.size());

        cerr << "Size in:   \t" << dAfterParserSize << endl ;
        cerr << "Size out:  \t" << dAfterCompilingSize << endl ;

        dumpInstance( sOutFile );

        int dSize ( dAfterCompilingSize - dAfterParserSize ) ;

        string sSize( "" ) ;

        if ( dSize > 0 )
        {
            stringstream s ;
            s << dSize ;
            sSize = string( " + " ) + string( s.str() ) ;
        }

        cerr << "Report:"
             << sQueryFile
             << " => "
             << sOutFile
             << sSize
             << endl ;


        clog << "Stop\n" ;

        if (vm.count("log")) clog.rdbuf(0);  //metoda na wylaczenie clog - crash patch
    }
    catch(std::exception& e)
    {
        cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
