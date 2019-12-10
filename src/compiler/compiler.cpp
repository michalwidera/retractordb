#include "QStruct.h"
#include "SOperations.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>

using boost::lexical_cast;

extern "C" {
    qTree coreInstance ;
}

/** This procedure computes time delays (delata) for generated streams */
string intervalCounter() {
    clog << "intervalCounter()" << endl ;

    while ( true ) {
        bool bOnceAgain( false );
        coreInstance.sort();
        for ( auto & q : coreInstance ) {
            if ( q.lProgram.size() == 0 ) {
                continue ;    /* Declaration */
            }

            if ( q.lProgram.size() == 1  ) {
                if ( q.rInterval == 0 ) {
                    token tInstance( * ( q.lProgram.begin() ) );
                    q.rInterval = coreInstance.getDelta( tInstance.getValue() ) ;
                }
                continue ;	// Just one stream
            }

            assert( q.lProgram.size() == 3 || q.lProgram.size() == 2 ) ;

            // This is shit coded (these size2 i size3) and fast fixed

            bool size3 = ( q.lProgram.size() == 3 );

            list < token > loc = q.lProgram ;

            token t1(*loc.begin()) ;
            if ( size3 ) {
                loc.pop_front();
            }
            token t2(*loc.begin()) ;
            loc.pop_front();
            token op(*loc.begin()) ;
            loc.pop_front();

            boost::rational<int> delta(-1) ;

            switch ( op.getTokenCommand() ) {

                case STREAM_HASH:

                    {
                        boost::rational<int> delta1 = coreInstance.getDelta( t1.getValue() ) ;
                        boost::rational<int> delta2 = coreInstance.getDelta( t2.getValue() ) ;

                        if ( delta1 == 0 || delta2 == 0 ) {
                            bOnceAgain = true ;
                            continue ;
                        } else {
                            delta = deltaHash ( delta1, delta2 ) ;
                        }
                    }
                    break ;

                case STREAM_DEHASH_DIV:

                    {
                        boost::rational<int> delta1 = coreInstance.getDelta( t1.getValue() ) ;
                        boost::rational<int> delta2 = t2.getCRValue() ; //Tutaj nie ma drugiego strumienia tylko od razu argument

                        assert( delta2 != 0 );

                        if ( delta1 == 0 ) {
                            bOnceAgain = true ;
                            continue ;
                        } else {
                            delta = deltaDivMod ( delta1, delta2 ) ;
                        }

                        if ( delta1 > delta ) {
                            throw std::out_of_range("Nie mozna zrobić szybszego div z wolniejszego zrodla");
                        }
                    }
                    break ;

                case STREAM_DEHASH_MOD:

                    {
                        boost::rational<int> delta1 = coreInstance.getDelta( t1.getValue() ) ;
                        boost::rational<int> delta2 = t2.getCRValue() ; 

                        assert( delta2 != 0 );

                        if ( delta1 == 0 ) {
                            bOnceAgain = true ;
                            continue ;
                        } else {
                            delta = deltaDivMod ( delta1, delta2 ) ;
                        }

                        if ( delta1 > delta ) {
                            throw std::out_of_range("Nie mozna zrobić szybszego mod z wolniejszego zrodla");
                        }
                    }
                    break ;

                case STREAM_SUBSTRACT:

                    {
                        boost::rational<int> delta1 = coreInstance.getDelta( t1.getValue() ) ;
                        if ( delta1 == 0 ) {
                            bOnceAgain = true ;
                            continue ;
                        } else {
                            delta = deltaSubstract ( delta1 ) ;
                        }
                    }
                    break ;

                case STREAM_ADD:

                    {
                        boost::rational<int>delta1 = coreInstance.getDelta( t1.getValue() ) ;
                        boost::rational<int>delta2 = coreInstance.getDelta( t2.getValue() ) ;

                        if ( delta1 == 0 || delta2 == 0 ) {
                            bOnceAgain = true ;
                            continue ;
                        } else {
                            delta = deltaAdd ( delta1, delta2 ) ;
                        }
                    }
                    break ;

                case STREAM_AVG:
                case STREAM_MIN:
                case STREAM_MAX:
                case STREAM_SUM:

                // Delta UNCHANGED ! (like time move)

                case STREAM_TIMEMOVE:

                    {
                        boost::rational<int>delta1 = coreInstance.getDelta( t1.getValue() ) ;

                        if ( delta1 == 0 ) {
                            bOnceAgain = true ;
                            continue ;
                        }
                        delta = deltaTimemove ( delta1, 0 ) ;
                    }
                    break ;

                case STREAM_AGSE: {
                        // ->>> check need
                        // core1@(5,3) ->
                        // push_stream core0 -> deltaSrc
                        // push_val 5 -> size_of_window
                        // stream agse 3 -> step_of_window
                        boost::rational<int> deltaSrc = coreInstance.getDelta( t1.getValue() ) ;
                        boost::rational<int> schemaSizeSrc = getQuery( t1.getValue() ).getFieldNamesList().size() ;
                        boost::rational<int> step = abs( t2.getCRValue() );
                        boost::rational<int> windowSize  = abs( op.getCRValue() ) ;

                        assert( windowSize > 0 );
                        assert( step > 0 );

                        if ( t2.getCRValue() < 0 ) { // windowSize < 0
                            delta = deltaSrc ;
                            delta /= schemaSizeSrc ;
                            delta *= windowSize ;
                            delta /= step ;
                        } else {
                            delta = ( deltaSrc / schemaSizeSrc ) * step ;
                        }

                        clog << "Computed delta from agse:" << delta << endl ;
                    }
                    break ;

                default:
                    cerr << op.getStrTokenName() << " ";
                    cerr << op.getCRValue() << " ";
                    cerr << op.getValue() << " ";
                    cerr << endl ;
                    throw std::out_of_range("Undefined token/command on list");

            } //switch ( op.getTokenCommand() )

            assert( delta != -1 );

            q.rInterval = delta;  //Tutaj jest ustalana wartosc delta - wartosc zwrotna

        }  //BOOST_FOREACH ( query & q , coreInstance )

        if ( ! bOnceAgain ) {
            break ;
        } else {
            coreInstance.sort();
        }

    } //while(true)

    coreInstance.sort();
    return string("OK") ;
}

string generateStreamName( string sName1, string sName2, command_id cmd ) {
    string sOperation( "undefinied" );

    switch ( cmd ) {
#define DEF_CASE( _A_ ) case _A_ : sOperation = #_A_ ; break ;
#include "tokendefset.h"
#undef DEF_CASE
    }

    if ( sName2 == "" ) {
        return sOperation + string ( "_" ) + sName1 ;
    } else {
        return sOperation + string ( "_" ) + sName2 + string ( "_" ) + sName1 ;
    }
}

/* Goal of this procedure is to provide stream to canonincal form
TODO: Stream_MAX,MIN,AVG...
*/
string simplifyLProgram() {
    clog << "simplifyLProgram() - start" << endl ;

    coreInstance.sort();
    for (
        qTree::iterator it = coreInstance.begin() ;
        it != coreInstance.end();
        ++it
    ) {
        //Agse pahse optization
        token t0,t1,t2 ;
        for (
            list < token >::iterator it2 = (*it).lProgram.begin() ;
            it2 != (*it).lProgram.end() ;
            it2 ++
        ) {
            t0 = t1 ;
            t1 = t2 ;
            t2 = (*it2);
            if ( t2.getStrTokenName() == "STREAM_AGSE" &&
                t1.getStrTokenName() == "PUSH_VAL" &&
                t0.getStrTokenName() == "PUSH_VAL"
            ) {
                token newVal( t2.getTokenCommand(), t1.getCRValue(), t1.getValue() );
                it2 = (*it).lProgram.erase( it2 );
                it2 -- ;
                it2 = (*it).lProgram.erase( it2 );
                (*it).lProgram.insert( it2, newVal );
                it2 = (*it).lProgram.begin() ;
                break;
            }
        }

        //Otimization phase 2
        if ( (*it).isReductionRequired() ) {

            clog << "simplifyLProgram(): reduction hit" << endl ;

            for (
                list < token >::iterator it2 = (*it).lProgram.begin() ;
                it2 != (*it).lProgram.end() ;
                it2 ++
            ) {
                if (
                    (*it2).getStrTokenName() == "STREAM_TIMEMOVE" ||
                    (*it2).getStrTokenName() == "STREAM_SUBSTRACT"
                ) {
                    query newQuery ;
                    string arg1 ;   //< Name of argument operation
                    command_id cmd ;

                    string affectedField ;

                    {
                        token newVal( *it2 );
                        newQuery.lProgram.push_front( newVal );

                        cmd = ( *it2 ).getTokenCommand() ;

                        clog << "simplifyLProgram(): mv cmd "
                            << ( *it2 ).getStrTokenName()
                            << " "
                            << ( *it2 ).getValue()
                            << endl ;

                        it2 = (*it).lProgram.erase( it2 );
                        it2 -- ;
                    }

                    {
                        token newVal( *it2 );
                        newQuery.lProgram.push_front( newVal );

                        stringstream s ;
                        s << ( *it2 ).getValue() ;
                        arg1 =  string(s.str()) ;

                        clog << "simplifyLProgram(): mv arg "
                            << ( *it2 ).getStrTokenName()
                            << " "
                            << ( *it2 ).getValue()
                            << endl ;

                        affectedField = ( *it2 ).getValue() ;

                        it2 = (*it).lProgram.erase( it2 );
                        it2 -- ;
                    }

                    it2 ++ ;
                    list < token > lSchema;
                    lSchema.push_back( token( PUSH_TSCAN )  );
                    newQuery.lSchema.push_back( field( "*", lSchema, field::BAD, "*" ) ) ;

                    newQuery.id = generateStreamName( arg1, "", cmd );

                    clog << "simplifyLProgram(): generation of " << newQuery.id << endl ;

                    (*it).lProgram.insert( it2, token( PUSH_STREAM, newQuery.id ) );

                    coreInstance.push_back( newQuery ); //After this instruction it loses context

                    it = coreInstance.begin() ;

                    break ;
                } else if (
                    (*it2).getStrTokenName() != "PUSH_STREAM" &&
                    (*it2).getStrTokenName() != "PUSH_VAL"
                ) {
                    query newQuery ;
                    string
                    arg1,   //< Name of first opeartion arguemnt
                    arg2 ;  //< Name of second opeartion arguemnt

                    command_id cmd ;

                    {
                        token newVal( *it2 );
                        newQuery.lProgram.push_front( newVal );

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
                        it2 -- ;
                    }

                    it2 ++ ;
                    list < token > lSchema;
                    lSchema.push_back( token( PUSH_TSCAN )  );
                    newQuery.lSchema.push_back( field( "*", lSchema, field::BAD, "*" ) ) ;
                    newQuery.id = generateStreamName( arg1, arg2, cmd );

                    (*it).lProgram.insert( it2, token( PUSH_STREAM, newQuery.id ) );

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

//Goal of this procedure is to unroll schema bsased of given command
list < field > combine( string sName1, string sName2, token cmd_token ) {
    clog << "combine() - (" << sName1 << "),(" << sName2 << ")" << endl ;

    list < field > lRetVal ;

    command_id cmd = cmd_token.getTokenCommand() ;

    //Merge of schemas for junction of hash type
    if ( cmd == STREAM_HASH ) {
        if (
            getQuery( sName1 ).lSchema.size() !=
            getQuery( sName2 ).lSchema.size()
        ) {
            throw std::invalid_argument("Hash operation needs same schemas on arguments stream");
        }

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

    else if ( cmd == STREAM_DEHASH_DIV ) {
        lRetVal = getQuery( sName1 ).lSchema ;
    }

    else if ( cmd == STREAM_DEHASH_MOD ) {
        lRetVal = getQuery( sName1 ).lSchema ;
    }

    else if ( cmd == STREAM_ADD ) {
        for ( auto f : getQuery( sName1 ).lSchema ) {
            lRetVal.push_back( f );
        }
        for ( auto f : getQuery( sName2 ).lSchema ) {
            lRetVal.push_back( f );
        }
    }

    else if ( cmd == STREAM_SUBSTRACT ) {
        lRetVal = getQuery( sName1 ).lSchema ;
    }

    else if ( cmd == STREAM_TIMEMOVE ) {
        lRetVal = getQuery( sName1 ).lSchema ;
    }

    else if ( cmd == STREAM_AVG ) {
        field intf ;
        intf.setFieldName.insert( "avg" );
        intf.dFieldType = field::RATIONAL ;
        intf.lProgram.push_front( token( PUSH_ID, sName1 ) );
        lRetVal.push_back( intf );
        return lRetVal ;
    }

    else if ( cmd == STREAM_MIN ) {
        field intf ;
        intf.setFieldName.insert( "min" );
        intf.dFieldType = field::RATIONAL ;
        intf.lProgram.push_front( token( PUSH_ID, sName1 ) );
        lRetVal.push_back( intf );
        return lRetVal ;
    }

    else if ( cmd == STREAM_MAX ) {
        field intf ;
        intf.setFieldName.insert( "max" );
        intf.dFieldType = field::RATIONAL ;
        intf.lProgram.push_front( token( PUSH_ID, sName1 ) );
        lRetVal.push_back( intf );
        return lRetVal ;
    }

    else if ( cmd == STREAM_SUM ) {
        field intf ;
        intf.setFieldName.insert( "sum" );
        intf.dFieldType = field::RATIONAL ;
        intf.lProgram.push_front( token( PUSH_ID, sName1 ) );
        lRetVal.push_back( intf );
        return lRetVal ;
    }

    else if ( cmd == STREAM_AGSE ) {
        //Unrolling schema for agse - discussion needed if we need do that this way
        list < field > schema ;
        int windowSize = abs ( rational_cast<int>(cmd_token.getCRValue()) ) ;

        //If winows is negative - reverted schema
        if ( rational_cast<int>(cmd_token.getCRValue()) > 0 ) {
            for ( int i = 0 ; i < windowSize ; i++ ) {
                field intf ;
                intf.setFieldName.insert( sName1 + "_" + lexical_cast<string>(i) );
                intf.dFieldType = field::BAD ;
                schema.push_back( intf );
            }
        } else {
            for ( int i = windowSize-1 ; i >=0  ; i-- ) {
                field intf ;
                intf.setFieldName.insert( sName1 + "_" + lexical_cast<string>(i) );
                intf.dFieldType = field::BAD ;
                schema.push_back( intf );
            }
        }

        lRetVal = schema ;
    } else {
        throw std::invalid_argument("Command stack hits undefinied opeation");
    }


    //Here are added to fields execution methods 
    //by reference to schema position 

    if ( cmd != STREAM_ADD ) {
        int offset( 0 ) ;

        for ( auto & f : lRetVal ) {

            stringstream s ;
            if ( cmd == STREAM_HASH ) {
                s << sName1 ;
            } else if ( cmd == STREAM_TIMEMOVE ) {
                s << sName1 ;
            } else if ( cmd == STREAM_AGSE ) {
                s << sName1 ;
            } else {
                s << sName1 ; //generateStreamName( sName2, sName1, cmd ) ;
            }
            s << "[" ;
            s << offset ++ ;
            s << "]" ;

            if ( f.lProgram.size() > 0 ) {
                f.lProgram.pop_front();
            }
            f.lProgram.push_front( token( PUSH_ID2, s.str() ) );
        }
    } else {
        for ( auto & f : lRetVal ) {
            stringstream s ;
            s << f.getFirstFieldName() ;
            f.lProgram.push_front( token( PUSH_ID3, s.str() ) );
        }
    }

    return lRetVal ;
}

//goal of this procedure is setup of all possible fields name and unroll *
//unfortunatlle algorithm if broken - because does not search backward but next by next
//and some * can be process which have arguments appear as two asterixe
//In such case unrool does not appear and algorithm gets shitin-shitout
string prepareFields() {
    clog << "prepareFields() - start" << endl ;

    coreInstance.tsort();
    for ( auto & q : coreInstance ) {
        for ( auto & t : q.lProgram ) {
            assert( q.lProgram.size() < 4 ) ;
            //fail of this asseration means that all streams are
            //after otimization alerady

            for ( auto & f : q.lSchema ) {

                clog << "prepareFields() getFirstFieldName=" << f.getFirstFieldName() << endl ;

                if ( f.getFirstFieldToken().getTokenCommand() ==  PUSH_TSCAN ) {
                    clog << "prepareFields() found (*)" << endl ;

                    //found! - and now unroll
                    if ( q.lProgram.size() == 1 ) {

                        clog << "prepareFields() size()==1" << endl ;

                        //we assure that on and only token is puhs_stream
                        assert( ( * q.lProgram.begin() ).getTokenCommand() == PUSH_STREAM ) ;

                        //copy list of fields from one to another
                        q.lSchema = getQuery( t.getValue() ).lSchema ;

                        for ( auto & fld : q.lSchema ) {
                            clog << "prepareFields(): fields in source:" << fld.getFirstFieldName() << endl ;
                            for ( auto & tk2 : fld.lProgram ) {

                                string target = tk2.getValue() ;
                                clog << "prepareFields(): " << tk2.getStrTokenName() << " " << target << endl;
                            }
                        }
                        break ;
                    }

                    if ( q.lProgram.size() == 3 ) {

                        clog << "prepareFields() size()==3" << endl ;

                        list < token >::iterator eIt = q.lProgram.begin();

                        string sName1  = ( * eIt ++ ).getValue() ;
                        string sName2  = ( * eIt ++ ).getValue() ;
                        token cmd = ( * eIt ++ ) ;

                        q.lSchema = combine( sName1, sName2, cmd ) ;
                        break ;
                    }

                    if ( q.lProgram.size() == 2 ) {

                        clog << "prepareFields() size()==2" << endl ;

                        list < token >::iterator eIt = q.lProgram.begin();

                        string sName1  = ( * eIt ++ ).getValue() ;
                        string sVar1   = ( * eIt ).getValue() ;
                        token cmd = ( * eIt ++ ) ;

                        q.lSchema = combine( sName1, "", cmd ) ;

                        break ;
                    }
                }
            }
        }
    }
    coreInstance.sort();

    clog << "prepareFields() - stop" << endl ;

    return string("OK") ;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty()) {
        return;
    }
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

/* If in query plan is PUSH_IDX it means that we need to duplicate [_] */
string replicateIDX() {

    for ( auto & q : coreInstance ) { // for each query

        for ( auto & f : q.lSchema ) { // for each field in query

            string schemaName = "" ;
            bool found ( false );

            for ( auto & t : f.lProgram ) { // for each token in query field
                if ( t.getTokenCommand() == PUSH_IDX ) {
                    if ( found ) {
                        if ( q.lSchema.size() != 1 ) {
                            // if _ exist then only one filed can be in query
                            throw std::out_of_range("Only one _ attribute can be in query");
                        }
                    }
                    found = true ;
                    schemaName = t.getValue();
                }
            }

            if ( found ) {
                const int fieldSize =  getQuery( schemaName ).lSchema.size() ;

                assert( fieldSize > 0 );

                list < field > oldFieldSet = q.lSchema ;
                assert ( oldFieldSet.size() == 1);
                field oldField = * q.lSchema.begin() ;
                q.lSchema.clear();

                for ( int i=0 ; i<fieldSize ; i ++ ) {
                    list < token > newLProgram ;
                    for ( auto & t : oldField.lProgram ) {
                        if ( t.getTokenCommand() == PUSH_IDX ) {
                            newLProgram.push_back ( token( PUSH_ID, boost::rational<int> ( i ), t.getValue() ) );
                        } else {
                            newLProgram.push_back ( token( t.getTokenCommand(), t.getCRValue(), t.getValue() ) );
                        }
                    }
                    string toRepalce = oldField.getFieldText() ;

                    replaceAll( toRepalce, "_", lexical_cast<string>(i) ) ;
                    field newField ( schemaName + "_" + lexical_cast<string>(i), newLProgram, oldField.dFieldType, toRepalce );
                    q.lSchema.push_back( newField );
                }
                assert ( q.lSchema.size() == getQuery( schemaName ).lSchema.size() );
                break ;
            }

        }
    }

    return string("OK");
}

/* Purpose of this function is to translate all refrences to fields
to form schema_name[postion, time_offset]
Command method of presentation aims simple data processing
Aim of this procedure is change all of push_idXXX to push_id
note that push_id is closest to push_id4
push_idXXX is searched in all stream program after reduction
*/
string convertReferences() {
    clog << "convertReferences() enter" << endl ;

    boost::regex xprFieldId5("(\\w*)\\[(\\d*)\\]\\[(\\d*)\\]");  //something[1][1]
    boost::regex xprFieldId4("(\\w*)\\[(\\d*)\\,(\\d*)\\]");  //something[1,1]
    boost::regex xprFieldId2("(\\w*)\\[(\\d*)\\]");         //something[1]
    boost::regex xprFieldIdX("(\\w*)\\[_]");                //something[_]
    boost::regex xprFieldId1("(\\w*).(\\w*)");              //something.in_schema
    boost::regex xprFieldId3("(\\w*)");                     //field_fo_corn

    for ( auto & q : coreInstance ) { // for each query
        assert( ! q.isReductionRequired() );

        for ( auto & f : q.lSchema ) {    // for each field in query
            for ( auto & t : f.lProgram ) { // for each token in query field
                const command_id cmd( t.getTokenCommand() );
                const string text( t.getValue() );

                boost::cmatch what;

                clog << "convertReferences() token:" << t.getStrTokenName() << "," ;
                clog << "convertReferences() text :" << text << endl ;

                switch ( cmd ) {
                    case PUSH_ID1:

                        clog << "convertReferences() PUSH_ID1 match - something.in_schema" << endl ;

                        if ( regex_search(text.c_str(),what,xprFieldId1) ) {


                            assert( what.size() == 3 );

                            const string schema( what[1] );
                            const string field( what[2] );

                            //aim of this procedure is found scheamt, next field in schema
                            //and then insert
                            for ( auto & q1 : coreInstance ) {
                                if ( q1.id == schema ) {
                                    int offset1( 0 );
                                    for ( auto & f1 : q1.lSchema ) {
                                        clog << "convertReferences() a.field:" << *f1.setFieldName.begin() << endl ;

                                        if ( f1.setFieldName.find( field ) != f1.setFieldName.end() ) {
                                            t = token( PUSH_ID, boost::rational<int> ( offset1 ), schema ) ;
                                            break ;
                                        } else {
                                            offset1 ++ ;
                                        }
                                    }

                                    if ( offset1 == q1.lSchema.size() ) {
                                        throw std::out_of_range("Failure during refernece converstion - schema exist, no fields");
                                    }

                                    break ;
                                }
                            }
                        } else {
                            throw std::out_of_range("No mach on type conversion ID1");
                        }

                        break ;

                    case PUSH_IDX:

                        clog << "convertReferences() PUSH_IDX match - something[_]" << endl ;

                        if ( regex_search(text.c_str(),what,xprFieldIdX) ) {

                            assert( what.size() == 2 );

                            const string schema( what[1] );

                            t = token( PUSH_IDX, boost::rational<int> ( 0 ), schema ) ;
                        } else {
                            throw std::out_of_range("No mach on type conversion IDX");
                        }

                        break ;
                    case PUSH_ID2:

                        clog << "convertReferences() PUSH_ID2 match - something[1]" << endl ;

                        if ( regex_search(text.c_str(),what,xprFieldId2) ) {

                            assert( what.size() == 3 );

                            const string schema( what[1] );
                            const string sOffset1( what[2] );
                            const int offset1( atoi( sOffset1.c_str() ) ) ;

                            t = token( PUSH_ID, boost::rational<int> ( offset1 ), schema ) ;
                        } else {
                            cerr << xprFieldId2 << endl ;
                            cerr << text.c_str() << endl ;
                            throw std::out_of_range("No mach on type conversion ID2");
                        }

                        break ;

                    case PUSH_ID3:

                        clog << "convertReferences() PUSH_ID3 match - type field_of_corn" << endl ;

                        if ( regex_search(text.c_str(),what,xprFieldId3) ) {

                            assert( what.size() == 2 );
                            const string field( what[1] );

                            list < token >::iterator eIt = q.lProgram.begin();

                            string schema1, schema2 ;
                            command_id cmd ;

                            query *pQ1 ( NULL ), *pQ2 ( NULL );

                            if ( q.lProgram.size() == 1 ) {
                                clog << "convertReferences() size=1" << endl ;
                                schema1  = ( * eIt ).getValue() ;       //push stream - stream
                                cmd = ( * eIt ).getTokenCommand() ;
                                assert( cmd == PUSH_STREAM ) ;

                                pQ1 = & getQuery( schema1 ) ;
                            }
                            if ( q.lProgram.size() == 2 ) {
                                clog << "convertReferences() size=2" << endl ;
                                schema1  = ( * eIt ++ ).getValue() ;    //push stream - stream
                                cmd = ( * eIt ).getTokenCommand() ;     //comand

                                pQ1 = & getQuery( schema1 ) ;
                            }
                            if ( q.lProgram.size() == 3 ) {
                                clog << "convertReferences() size=3" << endl ;
                                schema1  = ( * eIt ++ ).getValue() ;    //push stream - stream
                                schema2  = ( * eIt ++ ).getValue() ;    //push stream - stream
                                cmd = ( * eIt ).getTokenCommand() ;     //comand

                                pQ1 = & getQuery( schema1 ) ;
                                pQ2 = & getQuery( schema2 ) ;
                            }

                            bool bFieldFound( false );

                            int offset1( 0 );
                            if ( pQ1 != NULL ) {
                                offset1 = 0;
                                for( auto & f1 : (*pQ1).lSchema ) {
                                    clog << "convertReferences() b.field:" << *f1.setFieldName.begin() << endl ;

                                    if ( f1.setFieldName.find( field ) != f1.setFieldName.end() ) {
                                        t = token( PUSH_ID, boost::rational<int> ( offset1 ), schema1 ) ;
                                        bFieldFound = true ;
                                    }
                                    offset1 ++ ;
                                }
                            }

                            if ( pQ2 != NULL && bFieldFound == false ) {
                                offset1 = 0 ;
                                for( auto & f2 : (*pQ2).lSchema ) {
                                    clog << "convertReferences() c.field:" << *f2.setFieldName.begin() << endl ;

                                    if ( f2.setFieldName.find( field ) != f2.setFieldName.end() ) {
                                        t = token( PUSH_ID, boost::rational<int> ( offset1 ), schema2 ) ;
                                        bFieldFound = true ;
                                    }
                                    offset1 ++ ;
                                }
                            }

                            if ( bFieldFound == false ) {
                                throw std::logic_error("No field of given name in stream schema");
                            }
                        } else {
                            throw std::out_of_range("No mach on type conversion ID3");
                        }

                        break ;

                    case PUSH_ID4:
                    case PUSH_ID5:

                        clog << "convertReferences() PUSH_ID4/5 match - type something[1,1] || [1][1]" << endl ;

                        if ( regex_search(text.c_str(),what,xprFieldId4) ||
                            regex_search(text.c_str(),what,xprFieldId5)
                        ) {
                            assert( what.size() == 4 );

                            const string schema( what[1] );
                            const string sOffset1( what[2] );
                            const string sOffset2( what[3] );
                            const int offset1( atoi( sOffset1.c_str() ) ) ;
                            const int offset2( atoi( sOffset2.c_str() ) ) ;

                            bool foundSchema( false );
                            for ( auto & q : coreInstance ) {
                                if ( q.id  == schema ) {
                                    foundSchema = true ;
                                    break ;
                                }
                            }
                            if ( !foundSchema ) {
                                clog << "convertReferences() - Field calls nonexist schema - " << text.c_str() << endl ;
                                throw std::logic_error("Field calls nonexist schema - config.log (-g)");
                            }

                            t = token( PUSH_ID, boost::rational<int> ( offset1 + offset2 * q.lSchema.size() ), schema ) ;

                        } else {
                            throw std::out_of_range("No mach on type conversion ID4");
                        }

                        break ;
                }
            }
        }
    }
    clog << "convertReferences() leave" << endl ;

    return string("OK");
}

extern string parser( string sInputFile, string sOutputFile, bool verbose = true);

void dumpInstance( std::string sOutFile, bool verbose = true ) {
    //These object must be const
    const qTree coreInstance2( coreInstance ) ;

    std::ofstream ofs( sOutFile.c_str() );
    boost::archive::text_oarchive oa(ofs);

    oa << coreInstance2 ;

    if ( verbose ) {
        cout << "Dump:      \t" << sOutFile << endl ;
    }
}

std::streambuf *oldbuf = std::clog.rdbuf(clog.rdbuf());

int main(int argc, char* argv[]) {

    try {
        scoped_ptr<ostream> p_ofs;

        namespace po = boost::program_options;

        string sOutputFile ;
        string sInputFile ;

        po::options_description desc("Avaiable options");
        desc.add_options()
        ("help,h", "Show program options")
        ("queryfile,q", po::value<string>(&sInputFile)->default_value("query.txt"), "query set file")
        ("outfile,o", po::value<string>(&sOutputFile)->default_value("query.qry"), "output file")
        ("regtest,t","Regression Test")
        ("log,g","Translation raport - debuglog")
        ("verbose,v", "Diangostic info")
        ;

        po::positional_options_description p;       //Assume that infile is the first option
        p.add("infile", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
        po::notify(vm);

        //Intro
        if (vm.count("verbose")) {
            cerr << argv[0] << " - parser, compiler and qery plans reductor.\n";
        }

        if (vm.count("help")) {
            cout << desc << "\n";
            return 1;
        }

        if (vm.count("log")) {
            p_ofs.reset( new ofstream("compiler.log") ) ;
            if ( ! p_ofs ) {
                throw std::out_of_range("No report (debuglog)");
            }
            clog.rdbuf( p_ofs->rdbuf());
            cout << "Debuglog active\n";
            clog << "Start\n";
        } else {
            clog.rdbuf(NULL);
        }

        if (vm.count("regtest")) {
            int retcode = 0 ;
            cout << "Regression test start" << endl ;
            retcode = parser( "regtest", "testfile", vm.count("verbose")) != "OK" ;
            SOperations_regtest();
            cout << "Regression test end" << endl ;
            clog.rdbuf(oldbuf);
            return retcode;
        }

        string sQueryFile = vm["queryfile"].as< string >() ;
        string sOutFile   = vm["outfile"].as< string >() ;

        //defaults
        string sLinkFile  = "query.lkn" ;
        string sSubsetFile= "query.sub" ;

        //override defaults if filename is matching regexp
        {
            boost::regex filenameRe("(\\w*).(\\w*)");  //filename.extension
            boost::cmatch what;
            if ( regex_match(sQueryFile.c_str(),what,filenameRe) ) {
                string filenameNoExt = string( what[1] ) ;
                sLinkFile = filenameNoExt + ".lkn" ;
                sSubsetFile = filenameNoExt + ".sub" ;
            }
        }

        if (vm.count("verbose")) {
            cout << "In:        \t" << sQueryFile << endl ;
            cout << "Out:       \t" << sOutFile << endl ;
            cout << "Link:      \t" << sLinkFile << endl ;
            cout << "Subset:    \t" << sSubsetFile << endl ;
        }

        //check in query file contains first line subset[beg,end]
        //this sub is to check if we can test different examples
        //and file query.txt can be always in repo
        {
            string str;
            ifstream input( sQueryFile.c_str() );
            getline( input, str ) ;

            boost::cmatch what;

            boost::regex mulitestRe("set\\[(\\d*)\\,(\\d*)\\]");  //multiset[2-7]
            if ( regex_match(str.c_str(),what,mulitestRe) ) {
                sQueryFile = sSubsetFile ;
                ofstream querySubset( sQueryFile.c_str() );

                const int dGivenSetBeg( atoi( string( what[1] ).c_str() ) ) ;
                const int dGivenSetEnd( atoi( string( what[2] ).c_str() ) ) ;

                input.seekg( 0, ios_base::beg );

                int lineNr(0);
                list < string > querySet ;
                while( getline( input, str ) ) {
                    if ( lineNr >= dGivenSetBeg && lineNr <= dGivenSetEnd ) {
                        querySubset << str << "\n" ;
                    }
                    lineNr ++ ;
                }
                cout << "Subset ...\t" << dGivenSetBeg << "-" << dGivenSetEnd << endl ;
            }
        }

        cout << "Compiler " << parser( sQueryFile, sLinkFile, vm.count("verbose")) << endl ;

        //
        // Main Algorithm body
        //

        std::ifstream ifs( sLinkFile.c_str(), std::ios::binary);
        boost::archive::text_iarchive ia(ifs);
        ia >> coreInstance ;

        if ( coreInstance.size() == 0 ) {
            throw std::out_of_range("No queries to process found");
        }

        string sOutFileLog1   = vm["outfile"].as< string >() + ".lg1" ;
        dumpInstance( sOutFileLog1, vm.count("verbose") );

        int dAfterParserSize((int)coreInstance.size());

        string response ;

        response = simplifyLProgram() ;

        string sOutFileLog2   = vm["outfile"].as< string >() + ".lg2" ;
        dumpInstance( sOutFileLog2, vm.count("verbose"));

        response = prepareFields();

        string sOutFileLog3   = vm["outfile"].as< string >() + ".lg3" ;
        dumpInstance( sOutFileLog3, vm.count("verbose"));

        response = intervalCounter() ;

        response = convertReferences();

        response = replicateIDX();

        int dAfterCompilingSize((int)coreInstance.size());

        dumpInstance( sOutFile,vm.count("verbose"));

        int dSize ( dAfterCompilingSize - dAfterParserSize ) ;

        string sSize( "" ) ;

        if ( dSize > 0 ) {
            stringstream s ;
            s << dSize ;
            sSize = string( " + " ) + string( s.str() ) ;
        }

        if (vm.count("verbose")) {
            cout << "Raport:"
                << sQueryFile
                << " => "
                << sOutFile
                << sSize
                << endl ;

            cout << "Stop\n" ;
        }
        clog.rdbuf(oldbuf);

    } catch(std::exception& e) {
        clog.rdbuf(oldbuf);
        cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
