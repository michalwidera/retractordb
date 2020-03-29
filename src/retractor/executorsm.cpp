#include "QStruct.h"
#include "CRSMath.h"
#include "Processor.h"

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/chrono.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/info_parser.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/containers/map.hpp>

// boost::this_process::get_id()
#include <boost/process/environment.hpp>

//zeby sie dalo: BOOST_FOREACH( cmd_e i, commands | map_keys )
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>

#include <boost/system/error_code.hpp>

#include <boost/filesystem.hpp>

#include "Buffer.h"

namespace IPC = boost::interprocess ;

// Definicje for IPC purposes - maps i strings (most important IPCString i IPCMap)
typedef IPC::managed_shared_memory::segment_manager segment_manager_t;

typedef IPC::allocator<char, segment_manager_t> CharAllocator;
typedef IPC::basic_string<char, std::char_traits<char>,CharAllocator> IPCString;
typedef IPC::allocator<IPCString, segment_manager_t> StringAllocator;

typedef int KeyType;
typedef std::pair<const int, IPCString> ValueType;

typedef IPC::allocator<ValueType, segment_manager_t> ShmemAllocator;
typedef IPC::map<KeyType, IPCString, std::less<KeyType>, ShmemAllocator> IPCMap;

using namespace CRationalStreamMath ;

// Segment and allocator for string exchange
// IPC::managed_shared_memory strSegment(IPC::open_or_create, "RetractorShmemStr", 65536);
// const StringAllocator allocatorShmemStrInstance (strSegment.get_segment_manager());

// Maps stores realtions processId -> sended stream
std::map < const int, std::string > id2StreamName_Relation ;

std::vector < IPC::message_queue > qset ;

qTree coreInstance ;

extern Processor *pProc ;

// varialbe connected with tlimitqry (-m) parameter
// when it will be set thread will exit by given time (testing purposes)
int iTimeLimitCnt(0) ;

// Default communication mode - xml, by program option can change to json
enum mode {
    XML,
    JSON,
    INFO
} outMode(INFO) ;

typedef boost::property_tree::ptree ptree ;

extern int iTest();

std::map <string,ptree> streamTable ;

set< boost::rational<int> > getListFromCore() {
    set< boost::rational<int> > lstTimeIntervals ;

    for( const auto & it : coreInstance ) {
        lstTimeIntervals.insert( it.rInterval );
    }

    return lstTimeIntervals ;
}

void dumpCore( std::ostream & xout ) {
    xout << "In core:" << endl ;

    xout << "Seqence\tItrval\tQuery id" << endl ;

    for( const auto & it : coreInstance ) {
        xout << getSeqNr( it.id ) << "\t";
        xout << it.rInterval << "\t";
        xout << it.id  ;
        xout << endl ;
    }
}

set < string > getAwaitedStreamsSet(  TimeLine & tl ) {
    set < string > retVal ;

    while ( pProc == NULL ) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
    }

    for( const auto & it : coreInstance ) {
        boost::rational<int> slot = it.rInterval ;
        if ( ! tl.isThisDeltaAwaitCurrentTimeSlot( slot ) ) {
            continue ;
        }

        retVal.insert( it.id );
    }

    return retVal ;
}

void showAwaitedStreams( TimeLine & tl,
    string streamName = "" ) {
    set< string > strSet = getAwaitedStreamsSet( tl );
    for( const auto & str : strSet ) {
        if ( streamName == "" ) {
            cout << "-" << getSeqNr(str) << "-" ;
        } else if ( streamName == str ) {
            cout << "-" << getSeqNr(str) << "-" ;
        }
    }
}

ptree commandProcessor( ptree ptInval ) {
    ptree ptRetval ;

    string command = ptInval.get("db.message", "" ) ;

    try {
        //
        // This command return stream idenifiers
        //
        if ( command == "get" && pProc != NULL ) {
            std::cerr << "get cmd rcv." << std::endl ;

            for ( auto & q : coreInstance ) {
                ptRetval.put( string("db.stream.")+q.id, q.id );
                ptRetval.put( string("db.stream.")+q.id+string(".duration"),boost::lexical_cast<std::string>( q.rInterval ) );
                long recordsCount = 0 ;
                if ( q.isDeclaration() == false ) {
                    recordsCount = streamStoredSize( q.id );
                } else {
                    recordsCount = -1 ;
                }
                int processCount = ( pProc == NULL ) ? 0 : pProc->getStreamCount( q.id ) ;
                ptRetval.put( string("db.stream.")+q.id+string(".size"),boost::lexical_cast<std::string>( recordsCount ) );
                ptRetval.put( string("db.stream.")+q.id+string(".count"),boost::lexical_cast<std::string>( processCount ) );
            }
        }

        //
        // This command return what stream contains of
        //
        if ( command == "detail" && pProc != NULL ) {

            string streamName = ptInval.get("db.argument", "" ) ;

            assert( streamName != "" );

            std::cerr << "got detail " << streamName << " rcv." << std::endl ;

            for ( const auto & s : coreInstance[streamName].getFieldNamesList() ) {
                ptRetval.put( string("db.field.")+s, s );

            }
        }

        //
        // This command will add stream to list of transmited streams
        // there are created next queue with stream for client
        // and map indentifier with this stream
        //
        if ( command == "show"  && pProc != NULL ) {
            string streamName = ptInval.get("db.argument", "" ) ;

            // Probably someone call show w/o stream name
            assert( streamName != "" );

            // check if command present id of process
            assert( ptInval.get("db.id", "" ) != "" );

            // Testing purposes only - get it off after testing
            std::cerr << "got show " << streamName << " rcv." << std::endl ;

            // Here we set that for porcess of given id we send apropriate data stream
            int streamId = boost::lexical_cast<int>(ptInval.get("db.id", "" )) ;

            id2StreamName_Relation[streamId] = streamName ;

            // Create a message_queue
            string queueName = "brcdbr" + ptInval.get("db.id", "") ;

            // let's assuem that we have 1/10 duration
            // that menas 10 elements are going in second
            // so - we need 10 elements for one second buffer
            int maxElements = boost::rational_cast<int> ( 1 / coreInstance[streamName].rInterval );
            maxElements = ( maxElements < 2 ) ? 2 : maxElements ;

            IPC::message_queue mq
            (IPC::open_or_create           //open or crate
                ,queueName.c_str()         //name
                ,maxElements               //max message number
                ,1024                      //max message size
            );
            boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
        }

        //
        // This command stop (kills) server process
        //
        if ( command == "kill") {
            std::cerr << "got kill rcv." << std::endl ;
            iTimeLimitCnt = 1 ;
        }

        //
        // Diagnostic method
        //
        if ( command == "hello" ) {
            cerr << "got hello." << endl ;
            ptRetval.put( string("db"), string("world") );
            cerr << "will reply:" << endl;

            using boost::property_tree::ptree;

            std::stringstream strstream;
            if ( outMode == JSON ) {
                write_json( strstream, ptRetval ) ;
            } else if ( outMode == XML ) {
                write_xml( strstream, ptRetval );
            } else if ( outMode == INFO ) {
                write_info( strstream, ptRetval );
            }
            cerr << strstream.str() << endl; ;
        }
    } catch (const boost::property_tree::ptree_error &e) {
        cerr << "ptree fail:" << e.what() << endl;
    } catch(std::exception& e) {
        cerr << e.what() << endl ;
    }

    return ptRetval ; //sub for a while
}

// Thread procedure
void commmandProcessorLoop() {

    try {
        IPC::message_queue::remove("RetractorQueryQueue");
        IPC::shared_memory_object::remove("RetractorShmemMap");

        // Segment and allogator for map purposes
        IPC::managed_shared_memory mapSegment(IPC::open_or_create, "RetractorShmemMap", 65536);
        const ShmemAllocator allocatorShmemMapInstance (mapSegment.get_segment_manager());

        //Create a message_queue.
        IPC::message_queue mq
        (IPC::open_or_create           //open or crate
            ,"RetractorQueryQueue"   //name
            ,1000                      //max message number
            ,1000                      //max message size
        );

        IPCMap *mymap = mapSegment.construct<IPCMap>("MyMap")      //object name
            (std::less<int>(),allocatorShmemMapInstance);

        //
        // This need to be clean up - There are some mess..
        //
        char message[1000];
        unsigned int priority;
        IPC::message_queue::size_type recvd_size;

        while ( true ) {
            while ( mq.try_receive( message, 1000, recvd_size, priority) ) {

                message[recvd_size] = 0 ;

                std::stringstream strstream;
                strstream << message ;
                memset( message, 0, 1000 );

                ptree pt ;

                if ( outMode == JSON ) {
                    read_json( strstream, pt ) ;
                }
                if ( outMode == XML ) {
                    read_xml( strstream, pt );
                }
                if ( outMode == INFO ) {
                    read_info( strstream, pt );
                }

                ptree pt_retval = commandProcessor( pt );

                int clientProcessId = boost::lexical_cast<int>( pt.get("db.id", "" ) ) ;

                // Sending answer

                std::stringstream response_stream ;

                if ( outMode == JSON ) {
                    write_json( response_stream, pt_retval ) ;
                } else if ( outMode == XML ) {
                    write_xml( response_stream, pt_retval );
                } else if ( outMode == INFO ) {
                    write_info( response_stream, pt_retval );
                }

                IPCString response(allocatorShmemMapInstance);
                response = response_stream.str().c_str();
                mymap->insert(std::pair<KeyType, IPCString>(clientProcessId, response));
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
        }
    } catch(IPC::interprocess_exception &ex) {
        std::cout << ex.what() << std::endl << "catch on server" << std::endl;
    }
}

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int _kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

int _getch() {
    return getchar();
}

int main(int argc, char* argv[]) {

    // Clarification: When gcc has been upgraded to 9.x version some tests fails.
    // Bug appear when data are passing to program via script .sh
    // additional 13 (\r) character was append - this code normalize argv list.
    // C99: The parameters argc and argv and the strings pointed to by the argv array
    // shall be modifiable by the program, and retain their last-stored values
    // between program startup and program termination.

    auto retVal = system::errc::success;

    for ( int i = 0 ; i < argc ;  i ++ )
    {
        auto len = strlen( argv[i] ) ;
        if ( len > 0 )
            if ( argv[i][len-1] == 13 ) argv[i][len-1] = 0 ;
    }

    thread bt(commmandProcessorLoop);  //Sending service in thread

    // This line - delay is ugly fix for slow machine on CI !
    boost::this_thread::sleep_for(boost::chrono::milliseconds(10));  

    try {
        namespace po = boost::program_options;

        string sInputFile ;
        string sQuery ;
        string sDumpFile ;

        po::options_description desc("Supported program options");
        desc.add_options()
        ("help,h", "show help")
        ("infile,i", po::value<string>(&sInputFile)->default_value("query.qry"), "input query plan")
        ("query,q", po::value<string>(&sQuery), "query file" )
        ("display,s", po::value<string>(&sQuery), "process single query" )
        ("dump,d", po::value<string>(&sDumpFile)->default_value("query.dmp"), "dump file name")
        ("tlimitqry,m", po::value<int>(&iTimeLimitCnt)->default_value(0), "query limit, 0 - no limit" )
        ("json,j", "communication mode json")
        ("xml,x", "communication mode xml")
        ("info,o", "communication mode info (default)")
        ("onscreen,e","dump data on screen only")
        ("waterfall,f", "show waterfall mode")
        ("verbose,v", "Dump diagnostic info on screen while work")
        ;

        //Assume that infile is the first option
        po::positional_options_description p;
        p.add("infile", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("verbose")) {
            cerr << argv[0] << " - query plan executor." << std::endl;
        }

        if (vm.count("help")) {
            cout << desc << "\n";
            return system::errc::success;
        }

        if ( !boost::filesystem::exists( sInputFile ) )
        {

            cout << argv[0] << ": fatal error: no input file" << endl ;
            cout << "query processing terminated." << endl ;
            return EPERM ; //ERROR defined in errno-base.h
        }

        if ( vm.count("json") ) {
            outMode = JSON ;
        } else if ( vm.count("xml" ) ) {
            outMode = XML ;
        } else if ( vm.count("info") ) {
            outMode = INFO ;
        }

        if (vm.count("verbose")) {
            cerr << "Input :" << sInputFile << endl ;
        }

        std::ifstream ifs( sInputFile.c_str(), std::ios::binary);

        if ( ! ifs ) {
            cerr << sInputFile << " - no input file" << endl;
            return system::errc::invalid_argument;
        }

        boost::archive::text_iarchive ia(ifs);

        ia >> coreInstance ;

        Processor proc;

        //
        // option query was created for single query testing
        //

        if (vm.count("query")) {

            if (vm.count("verbose")) {
                cerr << "Query :" << sQuery << flush;
            }

            if ( iTimeLimitCnt == 0 ) {
                if (vm.count("verbose")) {
                    cerr << "Press any key to stop." << endl ;
                }
            }

            boost::rational<int> prev_interval(0);
            boost::rational<int> timeSlot( coreInstance.getDelta( sQuery ) );
            boost::rational<int> interval( timeSlot * 1000  ); // miliseconds

            while( ! _kbhit() ) {
                std::cout << timeSlot << std::endl ;
                std::cout << proc.printRowValue( sQuery ) << std::endl ;

                for ( auto s : getQuery( sQuery ).getDepStreamNameList() ) {
                    std::cout << s << std::endl << std::flush ;
                }

                int ms ( rational_cast<int>( interval ) ) ; // miliseconds
                boost::this_thread::sleep_for(boost::chrono::milliseconds(ms));
            }

            _getch(); //no wait ... feed key from kbhit

            return system::errc::success;
        }

        if (vm.count("verbose")) {
            cerr << "Objects:" ;
            dumpCore( cerr );
        }

        TimeLine tl( getListFromCore() );

        //
        // Main loop of data processing
        //

        //
        // When this value is 0 - means we are waiting for key - other way watchdog
        //
        if (vm.count("verbose")) {
            std::cout <<
                ( ( iTimeLimitCnt == 0 ) ?
                    "Press any key to stop."  :
                    "Query limit (-m) waiting for fullfil" )
                << std::endl ;
        }

        boost::rational<int> prev_interval(0);
        while( ! _kbhit() && iTimeLimitCnt != 1 ) {
            if ( iTimeLimitCnt != 0 ) {
                if ( iTimeLimitCnt != 1 ) {
                    iTimeLimitCnt -- ;
                } else {
                    break ;
                }
            }

            //
            // Inner time is counted in miliseconds
            // probably can be increased in faster machines
            //

            const int msInSec = 1000 ;

            boost::rational<int> interval( tl.getNextTimeSlot() * msInSec /* sec->ms */ );
            int period( rational_cast<int> ( interval - prev_interval ) ) ; // miliseconds
            prev_interval = interval ;

            //
            // When you add additional parameter -w numbers of queries appear on screen
            // if additional -w -s str2 (when -s means display) only given one query will appear
            //
            if ( vm.count("waterfall") ) {

                std::cout << period << "\t" ;
                showAwaitedStreams( tl,
                    vm.count("display") ? sQuery : "" );

                std::cout << std::endl ;
            }

            set < string > inSet = getAwaitedStreamsSet(tl) ;
            proc.updateContext( inSet );
            proc.processRows( inSet );

            //
            // Data broadcast - main loop
            //

            for( auto queryName : getAwaitedStreamsSet(tl) ) {
                if (vm.count("onscreen")) {
                    std::cout << proc.printRowValue( queryName ) << endl ;
                } else {
                    std::string row = proc.printRowValue(queryName) ;

                    std::list<int> eraseList ;

                    for ( const auto & element : id2StreamName_Relation ) {

                        if ( element.second == queryName ) {

                            using namespace boost::interprocess;

                            //
                            // Query discovery. queues are created by show command
                            //
                            std::string queueName = "brcdbr" + boost::lexical_cast<std::string>(element.first) ;
                            IPC::message_queue mq(IPC::open_only, queueName.c_str());

                            //
                            // If send queue is full - means no one is listening and queue is going to remove
                            //
                            if ( ! mq.try_send(row.c_str(),row.length(),0) ) {
                                message_queue::remove(queueName.c_str());
                                eraseList.push_back( element.first );
                            }
                        }
                    }

                    //
                    // cleaning form clients map that are not receiving data from queue
                    //
                    for ( const auto & element : eraseList ) {

                        id2StreamName_Relation.erase( element ) ;
                        if (vm.count("verbose")) {

                            cout << "queue erased on timeout, procId=" << element << std::endl;
                        }
                    }
                }
            }

            //
            // Waiting given miliseconds time that is computed
            //
            boost::this_thread::sleep_for(boost::chrono::milliseconds(period));

            //
            // End of loop while( ! _kbhit() )
            //
        }

        //
        // End of main processing loop
        //
        if ( iTimeLimitCnt != 1 ) {

            _getch(); //no wait ... feed key from kbhit
        } else {
            if (vm.count("verbose")) {

                cout << "Query limit (-m) waiting for fullfil" << endl ;
            }
        }

        saveStreamsToFile( sDumpFile );
        if (vm.count("verbose")) {
    
            cout << "Dump  :" << sDumpFile << endl ;
            Dump( std::cout );
        }

    } catch(IPC::interprocess_exception &ex) {

        std::cout << ex.what() << std::endl << "interprocess exception" << std::endl;
        retVal = system::errc::no_child_process;
    } catch(std::exception& e) {

        cerr << e.what() << endl ;
        retVal = system::errc::interrupted;
    }

    bt.interrupt();
    bt.join();

    IPC::shared_memory_object::remove("RetractorShmemMap");
    IPC::message_queue::remove("RetractorQueryQueue");

    for ( const auto & element : id2StreamName_Relation ) {
        std::string queueName = "brcdbr" + boost::lexical_cast<std::string>(element.first) ;
        IPC::message_queue::remove(queueName.c_str());
    }

    return retVal;
}
