#include "Processor.h"
#include "Buffer.h"
#include "SOperations.h"
#include "QStruct.h"

#include <boost/variant.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/lexical_cast.hpp>

//#include <boost/stacktrace.hpp>

extern "C" qTree coreInstance ;

Processor* pProc = NULL ;

extern enum mode {
    XML,
    JSON,
    INFO
} outMode ;


Processor::Processor() {
    //This function initialize map creating archive streams in cbuff
    for ( auto q : coreInstance ) {
        if ( storage.find(q.id) == storage.end() )
            storage[ q.id ] =
                boost::shared_ptr< dbStream >( new dbStream( q.id, q.getFieldNamesList() ) ) ;
        else
            ; //Stream with this name already exist in stystem

        //Container initialization for file type data sources
        if ( ! q.filename.empty() ) {
            gFileMap[ q.id ] = inputDF( q.filename.c_str(), q.lSchema );
        }
    }

    // Initializing fill of context and lenght of data stream
    for ( auto q : coreInstance ) { // For all queries in systes
        vector< number > rowValues ;

        for  ( auto f : q.lSchema ) {
            rowValues.push_back( boost::rational<int> (0) ) ;
        }

        gContextValMap[ q.id ] = rowValues ;
        gContextLenMap[ q.id ] = 0 ;
        gStreamSize[q.id] = -1 ;
    }

    pProc = this ;
}

void Processor::processRows( set < string > inSet ) {
    for ( auto q : coreInstance ) {
        // Drop rows that not come with this to compute
        if ( inSet.find( q.id ) == inSet.end() ) {
            continue ;
        }

        gStreamSize[ q.id ] ++ ;

        //Declareations need to be supported otherwise

        if ( q.isDeclaration() ) {
            if ( q.filename.empty() ) {
                assert( false );
                continue ;    //There are going randoms and no need to acces file
            } else {
                gFileMap[ q.id ].processRow() ;    // Fetch next row form file that have schema
            }
        }

        // here should be computer values of stream tuples
        // computed value shoud be stored in file
        vector <number> forCtx ;
        int cnt(0) ;

        for( auto   &f : q.lSchema ) {
            boost::rational<int> value( computeValue( f, q ) ) ;
            (*storage[ q.id ])[ cnt ] = value ;
            forCtx.push_back( value );
            cnt ++ ;
        }

        gContextValMap[q.id] = forCtx;
        // Store computed values to cbuffer - on disk
        storage[ q.id ]->store() ;
    }
}

string Processor::printRowValue( const string query_name ) {
    using boost::property_tree::ptree;
    ptree pt;
    pt.put("stream", query_name);
    pt.put("count", boost::lexical_cast<std::string>( getQuery( query_name ).lSchema.size() ) );
    int i = 0 ;

    for ( auto n : getRow( query_name, 0)) {
        stringstream retVal ;
        retVal << boost::rational_cast<double> ( boost::get< boost::rational<int>>(n)) ;
        pt.put( boost::lexical_cast<std::string>(i++), retVal.str() );
    }

    stringstream strstream;

    if ( outMode == JSON ) {
        write_json( strstream, pt ) ;
    } else if ( outMode == XML ) {
        write_xml( strstream, pt );
    } else if ( outMode == INFO ) {
        write_info( strstream, pt );
    }

    return strstream.str();
}

vector<number> Processor::getRow( string streamName, int timeOffset ) {
    vector<number> retVal ;
    /*
        if ( timeOffset == 0  && gContextLenMap[streamName] != gStreamSize[streamName]) {
            retVal = gContextValMap[ streamName ] ;
        } else {
            int column = 0;
            dbStream & archive = * ( storage[ streamName ] ) ;
            archive.get( timeOffset );
            for ( auto f : getQuery( streamName ).lSchema ) {
                retVal.push_back( archive.readCache(column++) ) ;
            }
        }
    */
    int column = 0 ;

    for ( auto f : getQuery( streamName ).lSchema ) {
        retVal.push_back( getValue( streamName, timeOffset, column++ ) );
    }

    return retVal ;
}

int Processor::getArgumentOffset( const string &streamName, const string &streamArgument ) {
    int retVal = 0;
    query &q ( getQuery( streamName ) ) ;

    for ( auto t : q.lProgram ) {
        if ( t.getTokenCommand() == STREAM_ADD ) {
            auto it = q.lProgram.begin() ;
            token A = * it ;
            it ++ ;
            token B = * it ;

            if ( A.getValue() == streamArgument ) {
                retVal = 0;
            } else if ( B.getValue() == streamArgument ) {
                retVal = getQuery( A.getValue() ).lSchema.size() ;
            } else {
                cerr << "currnet stream:" << streamName << endl ;
                cerr << "argument:" << streamArgument << endl ;
                cerr << "1st: " << A.getValue() << endl ;
                cerr << "2nd: " << B.getValue() << endl ;
                throw  std::out_of_range("Call to schema that not exist");
            }
        }
    }

    return retVal ;
}

number Processor::getValue( string streamName, int timeOffset, int schemaOffset ) {
    number retval ;
    query &q ( getQuery( streamName ) ) ;
    assert( timeOffset >= 0 );

    if ( schemaOffset >= q.lSchema.size() ) {
        timeOffset += schemaOffset / q.lSchema.size() ;
        schemaOffset %= q.lSchema.size() ;
    }

    if ( timeOffset == 0 && gContextLenMap[streamName] != gStreamSize[streamName] ) {
        retval = gContextValMap[streamName][schemaOffset] ;
    } else {
        dbStream &archive = * ( storage[ streamName ] ) ;
        archive.get(timeOffset - 1);
        retval = archive.readCache(schemaOffset) ;
    }

    return retval;
}

void Processor::updateContext( set < string > inSet ) {
    // Update of context variable
    for ( auto q : coreInstance ) { // For all queries in system

        // Drop off rows that not computed now
        if (inSet.find(q.id) == inSet.end()) {
            continue;
        }

        // If given stream is aledy synchronized with context
        // there is no sens to make computed twice
        if ( gContextLenMap[ q.id ] == gStreamSize[ q.id ] ) {
            continue ;
        }

        vector<number> rowValues;

        if ( q.isDeclaration() ) {
            // If argument is declared -
            // we read source and store in context
            if ( q.filename != "" ) {
                for ( auto f : q.lSchema ) {
                    rowValues.push_back( gFileMap[ q.id ].getCR( f ) );
                }
            } else {
                for ( auto f : q.lSchema ) {
                    rowValues.push_back( boost::rational<int>( rand() % 10, 1 + rand() % 9 ) ) ;
                }
            }
        } else {
            // execution of stream program and store data
            assert ( q.lProgram.size() < 4 );   //we assume that all stream programs are optimized and 1v2v3 size
            assert ( q.lProgram.size() > 0 );   //we assume that optimized streams are ready and filled
            list<token>::iterator it = q.lProgram.begin() ;
            token operation = q.lProgram.back() ;  // Operation is always last element on stack
            token argument1, argument2;            // This arugments are optionally fullfiled
            string streamNameArg ;                 // Same as argument
            int TimeOffset(-1) ;                   // This -1 is intentionally wrong to catch errors
            assert( rowValues.empty() );

            //all stream operations cover
            switch (operation.getTokenCommand()) {
                case PUSH_STREAM :
                    // PUSH_STREAM core0
                    // push stream operation does not have additonal stack arguments
                    streamNameArg = operation.getValue();
                    // check if context have same lengt like field
                    assert (gContextValMap[streamNameArg].size() == getQuery(streamNameArg).lSchema.size());
                    rowValues = gContextValMap[streamNameArg] ;
                    break;

                case STREAM_HASH :
                    // PUSH_STREAM core0
                    // PUSH_STREAM core1
                    // STREAM_HASH
                    assert ( q.lProgram.size() == 3 ) ;
                    // hash operations have two additional arguments of stack
                    // these argument are stream names to HASH operation
                    argument1 = *(it++);
                    argument2 = *(it++);

                    if ( Hash(
                            getQuery( argument1.getValue() ).rInterval,
                            getQuery( argument2.getValue() ).rInterval,
                            gContextLenMap[q.id],
                            TimeOffset ) ) {
                        streamNameArg = argument2.getValue();
                    } else {
                        streamNameArg = argument1.getValue();
                    }

                    TimeOffset = TimeOffset - gStreamSize[ streamNameArg ];
                    assert( TimeOffset >= 0 );
                    assert (gContextValMap[streamNameArg].size() == getQuery( argument1.getValue() ).lSchema.size()) ;
                    assert (gContextValMap[streamNameArg].size() == getQuery( argument2.getValue() ).lSchema.size()) ;
                    rowValues = getRow( streamNameArg, TimeOffset ) ;
                    break;

                case STREAM_ADD:
                    // PUSH_STREAM core0
                    // PUSH_STREAM core1
                    // STREAM_ADD
                    assert ( q.lProgram.size() == 3 ) ;
                    argument1 = *(it++);
                    argument2 = *(it++);
                    assert( streamNameArg == "" );
                    // Checking if ontex have same schema
                    assert (gContextValMap[ argument1.getValue()].size() == getQuery( argument1.getValue() ).lSchema.size()) ;
                    assert (gContextValMap[ argument2.getValue()].size() == getQuery( argument2.getValue() ).lSchema.size()) ;

                    for ( auto f : q.lSchema ) {
                        string schema = f.getFirstFieldToken().getValue() ;

                        if ( isExist(schema) ) {
                            int pos = boost::rational_cast<int> (f.getFirstFieldToken().getCRValue());
                            rowValues.push_back(getValue(schema, 0, pos));
                        } else {
                            rowValues.push_back(computeValue(f, q));
                        }
                    }

                    break;

                case STREAM_AVG:
                    argument1 = *(it++);
                    streamNameArg = argument1.getValue();
                    assert( streamNameArg != "" );
                    {
                        assert ( q.lProgram.size() == 2 ) ;
                        boost::rational <int> ret = 0 ;

                        for ( auto f : getQuery(streamNameArg).lSchema ) {
                            int pos = boost::rational_cast<int> (f.getFirstFieldToken().getCRValue() ) ;
                            string schema = f.getFirstFieldToken().getValue() ;
                            ret = ret + boost::get< boost::rational<int>>( getValue(schema, 0, pos ) );
                        }

                        ret = ret / static_cast<int>(q.lSchema.size())  ;
                        rowValues.push_back( ret );
                    }
                    break ;

                case STREAM_MAX:
                    argument1 = *(it++);
                    streamNameArg = argument1.getValue();
                    assert( streamNameArg != "" );
                    {
                        assert ( q.lProgram.size() == 2 ) ;
                        boost::rational <int> ret = INT_MIN ; /* limits.h */

                        for ( auto f : getQuery(streamNameArg).lSchema ) {
                            int pos = boost::rational_cast<int> (f.getFirstFieldToken().getCRValue() ) ;
                            string schema = f.getFirstFieldToken().getValue() ;
                            boost::rational <int> val =
                                boost::get< boost::rational<int>>( getValue(schema, 0, pos ) );

                            if ( val > ret ) {
                                ret = val ;
                            }
                        }

                        rowValues.push_back( ret );
                    }
                    break ;

                case STREAM_MIN:
                    argument1 = *(it++);
                    streamNameArg = argument1.getValue();
                    assert( streamNameArg != "" );
                    {
                        assert ( q.lProgram.size() == 2 ) ;
                        boost::rational <int> ret = INT_MAX ; /* limits.h */

                        for ( auto f : getQuery(streamNameArg).lSchema ) {
                            int pos = boost::rational_cast<int> (f.getFirstFieldToken().getCRValue() ) ;
                            string schema = f.getFirstFieldToken().getValue() ;
                            boost::rational <int> val =
                                boost::get< boost::rational<int>>( getValue(schema, 0, pos ) );

                            if ( val < ret ) {
                                ret = val ;
                            }
                        }

                        rowValues.push_back( ret );
                    }
                    break ;

                case STREAM_SUM:
                    argument1 = *(it++);
                    streamNameArg = argument1.getValue();
                    assert( streamNameArg != "" );
                    {
                        assert ( q.lProgram.size() == 2 ) ;
                        boost::rational <int> ret = 0 ; /* limits.h */

                        for ( auto f : getQuery(streamNameArg).lSchema ) {
                            int pos = boost::rational_cast<int> (f.getFirstFieldToken().getCRValue() ) ;
                            string schema = f.getFirstFieldToken().getValue() ;
                            boost::rational <int> val =
                                boost::get< boost::rational<int>>( getValue(schema, 0, pos ) );
                            ret += val ;
                        }

                        rowValues.push_back( ret );
                    }
                    break ;

                case STREAM_TIMEMOVE:
                    // PUSH_STREAM core1
                    // STREAM_TIMEMOVE 10
                    assert ( q.lProgram.size() == 2 ) ;
                    argument1 = *(it++);
                    streamNameArg = argument1.getValue();
                    assert( streamNameArg != "" );
                    TimeOffset = rational_cast<int>( operation.getCRValue() );
                    assert ( TimeOffset >= 0 );
                    rowValues = getRow( streamNameArg, TimeOffset );
                    break ;

                case STREAM_DEHASH_DIV :
                case STREAM_DEHASH_MOD:
                    // PUSH_STREAM core0
                    // PUSH_STREAM 2/3
                    // STREAM_DEHASH_DIV
                    {
                        assert ( q.lProgram.size() == 3 ) ;
                        boost::rational<int> rationalArgument;
                        argument1 = *(it++);
                        argument2 = *(it++);
                        streamNameArg = argument1.getValue();
                        assert( streamNameArg != "" );
                        rationalArgument = argument2.getCRValue() ;
                        assert( rationalArgument > 0 );
                        // q.id - name of output stream
                        // gStreamSize[q.id] - count of record in output stream
                        // q.rInterval - delta of output stream (rational) - contains deltaDivMod( core0.delta , rationalArgument )
                        // rationalArgument - (2/3) argument of operation (rational)
                        int position = gStreamSize[q.id] + 1 ;

                        // gStreamSize[q.id] == -1 for zero elements (therefore + 1)

                        if ( operation.getTokenCommand() == STREAM_DEHASH_DIV ) {
                            TimeOffset = Div( rationalArgument, q.rInterval, position );
                        }

                        if ( operation.getTokenCommand() == STREAM_DEHASH_MOD ) {
                            TimeOffset = Mod( rationalArgument, q.rInterval, position );
                        }

                        if ( TimeOffset < 0 ) {
                            //cerr << "@:" << boost::stacktrace::stacktrace() << endl ;
                            //req. higher boost version
                            assert(false);
                        }

                        assert( TimeOffset >= 0 );
                        rowValues = getRow( streamNameArg, TimeOffset );
                    }
                    break ;

                case STREAM_AGSE:
                    // PUSH_STREAM core -> delta_source (argument1)
                    // PUSH_VAL 2 -> window_length (argument2)
                    // STREAM_AGSE 3 -> window_step (operation)
                    {
                        argument1 = *(it++);
                        argument2 = *(it++);
                        string nameSrc = argument1.getValue();
                        string nameOut = q.id ;
                        bool mirror = operation.getCRValue() < 0 ;
                        // step - means step of how long moving window will over data stream
                        // step is counted in tuples/atribute
                        // windowsSize - is length of moving data window
                        int step  = rational_cast<int>( argument2.getCRValue() );
                        assert ( step >= 0 );
                        int windowSize = abs( rational_cast<int>( operation.getCRValue() ) );
                        assert ( windowSize > 0 );
                        boost::rational<int> factor = windowSize / step  ;
                        boost::rational<int> deltaSrc = getQuery( nameSrc ).rInterval ;
                        boost::rational<int> deltaOut = getQuery( nameOut ).rInterval ;
                        int schemaSizeSrc =  getQuery( nameSrc ).lSchema.size() ;
                        int schemaSizeOut =  getQuery( nameOut ).lSchema.size() ;
                        int streamSizeSrc = gContextLenMap[ nameSrc ];
                        int streamSizeOut = gContextLenMap[ nameOut ];
                        int streamSizeSrc_ = gStreamSize[ nameSrc ];
                        int streamSizeOut_ = gStreamSize[ nameOut ];

                        if ( streamSizeOut_ < 0 ) {
                            streamSizeOut_ = 0 ;
                        }

                        if ( streamSizeSrc_ < 0 ) {
                            streamSizeSrc_ = 0 ;
                        }

                        assert ( nameSrc != "" );
                        assert ( windowSize == schemaSizeOut );

                        if ( mirror ) {
                            for ( int i = windowSize - 1 ; i >= 0  ; i -- ) {
                                number ret = getValue(nameSrc, 0, i ) ;
                                rowValues.push_back( ret );
                            }
                        } else {
                            int d =
                                abs (
                                    ( streamSizeOut ) -
                                    agse ( streamSizeSrc * schemaSizeSrc, step )
                                ) ;

                            for ( int i = 0 ; i < windowSize ; i ++ ) {
                                number ret = getValue(nameSrc, 0, i + d ) ;
                                rowValues.push_back( ret );
                            }
                        }

                        assert( rowValues.size() == q.lSchema.size() );
                        assert( gContextValMap[q.id].size() == rowValues.size() );
                    }
                    break;

                case STREAM_SUBSTRACT:
                    // PUSH_STREAM core
                    // STREAM_SUBSTRACT 3_2
                    // This need to be checked - I was tired when I wrote this
                    assert ( q.lProgram.size() == 2 ) ;
                    argument1 = *(it++);
                    streamNameArg = argument1.getValue();
                    assert( streamNameArg != "" );

                    if ( operation.getCRValue() > q.rInterval ) {
                        // Check if parameters are in oposite order
                        TimeOffset = Substract( q.rInterval, operation.getCRValue(), gStreamSize[q.id] );
                        TimeOffset = gContextLenMap[ q.id ] - TimeOffset  ;
                        rowValues = getRow( streamNameArg, TimeOffset );
                    } else {
                        rowValues = gContextValMap[ streamNameArg ] ;
                    }

                    break ;

                default:
                    // Stack hits non supported opertation
                    assert(false);
            }
        }

        // Store to conext computed values
        assert( !rowValues.empty() );
        gContextValMap[ q.id ] = rowValues ;
        gContextLenMap[ q.id ]++ ;
    }
}

number Processor::getValueOfRollup( const query &q, int offset, int timeOffset ) {
    token arg[3];
    const int progSize =  q.lProgram.size() ;
    assert(progSize < 4);
    int ret = 0 ;
    int i = 0;

    for( auto tk : q.lProgram ) {
        arg[i++] = tk ;
    }

    const command_id cmd = arg[progSize - 1].getTokenCommand();
    int TimeOffset(-1) ;                   // This -1 is intentionally wrong - Hash return value

    switch ( cmd ) {
        case PUSH_STREAM:
            return getValue( arg[0].getValue(), timeOffset, offset );

        case STREAM_TIMEMOVE:
            /* signalRow>1 : PUSH_STREAM(signalRow), STREAM_TIMEMOVE(1) */
            return getValue( arg[0].getValue(), timeOffset + rational_cast<int> ( arg[1].getCRValue() ), offset );

        case STREAM_DEHASH_MOD:
        case STREAM_DEHASH_DIV:
            /* signalRow&0.5 : PUSH_STREAM(signalRow), PUSH_VAL(1_2), PUSH_DEHASH_DIV(0) */
            assert(false); //TODO

        case STREAM_SUBSTRACT:
            //TODO: Check
            return getValue( arg[0].getValue(), timeOffset, offset );

        case STREAM_AVG:
        case STREAM_MIN:
        case STREAM_MAX:
        case STREAM_SUM:
            assert(offset == 1);
            return getValue( arg[0].getValue(), timeOffset, offset );

        case STREAM_ADD:
            /* signalRow+source : PUSH_STREAM(signalRow), PUSH_STREAM(source), STREAM_ADD(0) */
            {
                const auto sizeOfFirstSchema = getQuery(arg[0].getValue()).lSchema.size();

                if (offset < sizeOfFirstSchema) {
                    return getValue(arg[0].getValue(), timeOffset, offset);
                } else {
                    return getValue(arg[1].getValue(), timeOffset, offset - sizeOfFirstSchema);
                }
            }

        case STREAM_AGSE:
            assert(false); //TODO
            return number(0) ; /* pro forma */

        case STREAM_HASH:
            // TODO: Check if right hash part is returned here
            {
                const auto timeSeqence = ( gContextLenMap[q.id] - timeOffset ) > 1 ? gContextLenMap[q.id] - timeOffset : 1 ;

                if (Hash(
                        getQuery(arg[0].getValue()).rInterval,
                        getQuery(arg[1].getValue()).rInterval,
                        timeSeqence,
                        TimeOffset)) {
                    return getValue(arg[1].getValue(), TimeOffset, offset);
                } else {
                    return getValue(arg[0].getValue(), TimeOffset, offset);
                }
            }

            assert(false); //TODO
    }

    assert(false); // Unknown operator catched here
    return number(0) ; /* pro forma */
}

/** This function will give info how long is stream argument if argument will be * instead of argument list */
int getSizeOfRollup( const query &q ) {
    token arg[3];
    const int progSize =  q.lProgram.size() ;
    assert(progSize < 4);
    int ret = 0 ;
    int i = 0;

    for( auto tk : q.lProgram ) {
        arg[i++] = tk ;
    }

    if ( progSize == 1 ) {
        return getQuery( arg[0].getValue() ).lSchema.size() ;
    }

    if ( progSize == 2 ) {
        return getQuery( arg[0].getValue() ).lSchema.size() ;
    }

    const command_id cmd = arg[progSize - 1].getTokenCommand();

    if ( progSize == 3 ) {
        switch( cmd ) {
            case STREAM_HASH:
            case STREAM_DEHASH_MOD:
            case STREAM_DEHASH_DIV:
            case STREAM_SUBSTRACT:
            case STREAM_TIMEMOVE:
                return getQuery( arg[0].getValue() ).lSchema.size() ;

            case STREAM_AGSE:
                return abs(rational_cast<int>(arg[2].getCRValue()));

            case STREAM_ADD:
                return getQuery( arg[0].getValue() ).lSchema.size() +
                    getQuery( arg[1].getValue() ).lSchema.size() ;

            case STREAM_AVG:
            case STREAM_MIN:
            case STREAM_MAX:
            case STREAM_SUM:
                return 1 ;
        }
    }

    assert(false);
    return 0; //pro forma
}

boost::rational<int> Processor::computeValue(
    field &f,
    query &q
) {
    bool resultValid = true ;
    stack < boost::rational<int>> rStack ;
    boost::rational<int> a, b ;

    for( auto tk : f.lProgram ) {
        if ( ! resultValid ) { //Is somewe by walk we hit NULL - getoutofhere! - and be back in a while.
            cerr << "result fail" << endl ;
            break ;    //Check it - TODO!
        }

        switch( tk.getTokenCommand() ) {
            case ADD:
            case SUBTRACT:
            case MULTIPLY:
            case DIVIDE:
                a = rStack.top() ;
                rStack.pop() ;
                b = rStack.top() ;
                rStack.pop() ;
        }

        switch( tk.getTokenCommand() ) {
            case PUSH_VAL:
                rStack.push( tk.getCRValue() );
                break ;

            case ADD:
                rStack.push ( b + a ) ;
                break ;

            case SUBTRACT:
                rStack.push ( b - a ) ;
                break ;

            case MULTIPLY:
                rStack.push ( b * a ) ;
                break ;

            case DIVIDE:
                if ( a != 0 ) {
                    rStack.push ( b / a ) ;
                } else {
                    rStack.push( 0 );
                    resultValid = false ;
                }

                break ;

            case NEGATE:
                a = rStack.top() ;
                rStack.pop() ;
                rStack.push ( - a ) ;
                break ;

            case CALL: {
                    double real = (double) a.numerator() / (double) a.denominator() ;

                    if ( tk.getValue() == "floor" ) {
                        a = rStack.top() ;
                        rStack.pop() ;
                        rStack.push ( boost::rational<int>(Rationalize(floor(real))));
                    } else if ( tk.getValue() == "getRowValueceil" ) {
                        a = rStack.top() ;
                        rStack.pop() ;
                        rStack.push ( boost::rational<int>(Rationalize(ceil(real))));
                    } else if ( tk.getValue() == "sqrt" ) {
                        a = rStack.top() ;
                        rStack.pop() ;
                        rStack.push (boost::rational<int>(Rationalize(sqrt(real))));
                    } else if ( tk.getValue() == "count" ) {
                        assert( rStack.size() < 4 );
                        assert( rStack.size() > 0 );
                        /**
                         * Overhere you should see 1,2 or 3 arguments
                         * (x) means count X in schema
                         * (x,y) means count from X to Y in schema
                         * (x,y,t) meanse count X to Y in schemat and t probes back
                         */
                        vector<int> args ;

                        while( !rStack.empty() ) {
                            args.push_back(rational_cast<int>(rStack.top()));
                            rStack.pop();
                        }

                        int from, to, len = 1 ;

                        if ( args.size() == 1 ) {
                            from = to = args[0];
                        }

                        if ( args.size() == 2 ) {
                            from = args[1];
                            to = args[0];
                        }

                        if ( args.size() == 3 ) {
                            from = args[2];
                            to = args[1];
                            len = args[0];
                        }

                        if ( to < from ) {
                            int tempval = from ;
                            to = from ;
                            from = tempval ;
                        }

                        boost::rational <int> ret = 0 ; /* limits.h */

                        for ( int j = 0 ; j < len ; j ++ )
                            for ( int i = 0 ; i < getSizeOfRollup(q) ; i ++ ) {
                                boost::rational <int> val = boost::get< boost::rational<int>>( getValueOfRollup( q, i, j ) );

                                if ( val >= from && val <= to ) {
                                    ret++;
                                }
                            }

                        rStack.push( ret );
                    } else {
                        throw std::out_of_range("No support for this math function - write it SVP");
                    }
                }
                break;

            case PUSH_ID3:  //Field_name
            case PUSH_ID1:  //Schema_name.Field_name
            case PUSH_ID2:  //Schema_name[indeks]
            case PUSH_ID4:  //Schema_name[indeks,indeks]
            case PUSH_ID5:  //Schema_name[indeks][indeks]
            case PUSH_IDX:  //Schema_name[_]
                throw std::out_of_range("Thrown/Processor.cpp/computeValue This field is reducted in compilation");
                break ;

            case PUSH_ID: { //Schema_name[indeks,indeks] like PUSH_ID4
                    int offsetInSchema ( rational_cast<int>(tk.getCRValue()));
                    string argument( tk.getValue() );
                    string outSchema( q.id );
                    // If operation ADD exist - then position schema will be moved
                    // first argument
                    int offsetFromArg = getArgumentOffset( outSchema, argument );
                    number a = getValue(outSchema, 0, offsetInSchema + offsetFromArg);
                    rStack.push( boost::get< boost::rational<int>>(a) );
                }
                break ;

            default:
                throw std::out_of_range("Thrown/Processor.cpp/getCRValue Unknown token");
        }
    }

    if ( rStack.size() == 0 ) {
        int fieldPosition =  q.getFieldIndex(f) ;
        assert( fieldPosition != -1 );
        number retVal = gContextValMap[ q.id ][ fieldPosition ];
        return boost::get< boost::rational<int>>( retVal ) ;
    } else if ( rStack.size() == 1 ) {
        return rStack.top() ;
    } else {
        throw std::out_of_range("Thrown/Processor.cpp/getCRValue too much token left on stack");
    }

    return boost::rational<int>(0) ; /* pro forma */
}

int Processor::getStreamCount( const string query_name ) {
    return gStreamSize[ query_name ];
}
