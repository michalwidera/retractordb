/*
How xqry terminal works

1. Take a comand line arguemnt ie. xqry list
2. Connect to server
3. Execute command
4. Show result (Show result,....) (wait for ctrl+c)
5. End of process
*/

#include <ctime>
#include <assert.h>
#include <cstdio>
#include <chrono>

#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/asio.hpp>
#include <boost/config.hpp>
#include <boost/thread.hpp>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>
#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/info_parser.hpp>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/find.hpp>

//For data transmision purposes
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/atomic.hpp>

//for: BOOST_FOREACH( cmd_e i, commands | map_keys )
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

#include <boost/assign.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/containers/map.hpp>

// boost::this_process::get_id()
#include <boost/process/environment.hpp>

#include <boost/program_options.hpp>

#include <time.h>

using namespace std ;
using namespace boost ;
using namespace boost::assign;

using boost::property_tree::ptree;

namespace IPC = boost::interprocess ;

//definitions for IPC - maps i strings (most important IPCString i IPCMap)
typedef IPC::managed_shared_memory::segment_manager segment_manager_t;

typedef IPC::allocator<char, segment_manager_t> CharAllocator;
typedef IPC::basic_string<char, std::char_traits<char>,CharAllocator> IPCString;
typedef IPC::allocator<IPCString, segment_manager_t> StringAllocator;

typedef int KeyType;
typedef std::pair<const int, IPCString> ValueType;

typedef IPC::allocator<ValueType, segment_manager_t> ShmemAllocator;
typedef IPC::map<KeyType, IPCString, std::less<KeyType>, ShmemAllocator> IPCMap;

//When server works under valgrid - must be 10 probes x 10ms
constexpr int maxAcceptableFails = 10 ;

boost::lockfree::spsc_queue< ptree, boost::lockfree::capacity< 1024 > > spsc_queue;

int producer_count = 0 ;

boost::atomic<bool> done (false);

std::map <string,ptree> streamTable ;

int iTimeLimitCnt ; // testing purposes - time limit query (-m)

//Default comunication mode INFO
enum mode {
    XML,
    JSON,
    INFO
} outMode(INFO) ;

enum outputFormatMode {
    RAW,
    GRAPHITE,
    INFLUXDB
} outputFormatMode( RAW ) ;

//Graphite nbeed scheamt in format "path.to.data value timestamp"
ptree schema ;
string sInputStream ;

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

//
// Consumer process asynchronusly fetch data from quere and puts on screen/output
//
void consumer() {
    ptree e_value ;
    while (!done) {
        while (spsc_queue.pop(e_value)) {
            string stream = e_value.get("stream", "" )  ;

            using namespace boost::adaptors;

            BOOST_FOREACH( string w, streamTable | map_keys )
            if ( w == stream ) {
                int count = boost::lexical_cast<int> ( e_value.get("count", "" ) );

                if ( outputFormatMode == RAW ) {
                    for ( int i = 0 ; i < count ; i ++ ) {
                        printf( "%s ", e_value.get( boost::lexical_cast<string>(i), "" ).c_str() );
                    }

                    printf("\r\n");
                }

                if ( outputFormatMode == GRAPHITE ) {
                    int i = 0 ;
                    for ( const auto & v : schema.get_child("db.field") ) {
                        printf( "%s.%s %s %llu ",
                            sInputStream.c_str(),
                            v.second.get<std::string>("").c_str(),
                            e_value.get( boost::lexical_cast<string>(i++), "" ).c_str(),
                            (long long) time(NULL)
                        );
                    }
                }

                //https://docs.influxdata.com/influxdb/v1.5/write_protocols/line_protocol_tutorial/
                if ( outputFormatMode == INFLUXDB ) {
                    using namespace std::chrono;
                    int i = 0 ;
                    printf( "%s ", sInputStream.c_str());
                    bool firstValNoComma(true);
                    for ( const auto & v : schema.get_child("db.field") ) {
                        if ( firstValNoComma ) {
                            firstValNoComma = false ;
                        } else {
                            printf( "," );
                        }
                        printf( "%s=%s",
                            v.second.get<std::string>("").c_str(),
                            e_value.get( boost::lexical_cast<string>(i++), "" ).c_str()
                        );
                    }
                    printf( " %ld\n", duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count() );
                }

                //This part is time limited (-m) resposbile
                if ( iTimeLimitCnt > 1 ) {
                    iTimeLimitCnt -- ;
                }
            }
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
    }
    while (spsc_queue.pop(e_value)) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
    }

}


void producer() {
    try {

        string queueName = "brcdbr" + boost::lexical_cast<string>(boost::this_process::get_id()) ;

        IPC::message_queue mq(IPC::open_only ,queueName.c_str() );

        char message[1024];
        unsigned int priority;
        IPC::message_queue::size_type recvd_size = 1024;

        while ( !done ) {

            bool messageReceived = false ;
            while ( ! messageReceived && !done ) {
                messageReceived = mq.try_receive( message, 1024, recvd_size, priority);
                boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
            }
            if ( done ) {
                continue ;
            }
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

            while (!spsc_queue.push( pt )) {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
            }


        }
    } catch(IPC::interprocess_exception &ex) {
        cerr << ex.what() << endl << "catch on producer queue" << endl;
        cerr << "queue:" << "brcdbr" + boost::lexical_cast<string>(boost::this_process::get_id()) << endl ;
        done = true ;
        return ;
    }
}

ptree netClient( string netCommand, string netArgument ) {

    ptree pt_response ;
    ptree pt_request ;

    try {

        IPC::managed_shared_memory mapSegment(IPC::open_only, "AbracadabraShmemMap");
        const ShmemAllocator allocatorShmemMapInstance (mapSegment.get_segment_manager());

        pt_request.put("db.message", netCommand );
        pt_request.put("db.id", boost::this_process::get_id() );

        if ( netArgument != "" ) {
            pt_request.put("db.argument", netArgument );
        }

        //
        //request part
        //

        IPC::message_queue mq
        (IPC::open_only
            ,"AbracadabraQueryQueue"
        );

        std::stringstream request_stream ;

        if ( outMode == JSON ) {
            write_json( request_stream, pt_request ) ;
        }
        if ( outMode == XML ) {
            write_xml( request_stream, pt_request );
        }
        if ( outMode == INFO ) {
            write_info( request_stream, pt_request );
        }

        mq.send( request_stream.str().c_str(), request_stream.str().length(), 0 );

        // Send the request.

        //
        // Response part
        //

        // Read the response status line. The response streambuf will automatically
        // grow to accommodate the entire line. The growth may be limited by passing
        // a maximum size to the streambuf constructor.

        // Check that response is OK.

        std::pair<IPCMap *,std::size_t> ret = mapSegment.find<IPCMap>("MyMap");
        IPCMap * mymap = ret.first ;
        assert( mymap );

        int processId = boost::this_process::get_id() ;

        IPCMap::iterator it = mymap->find( processId ) ;

        int cntr(maxAcceptableFails);
        while ( it == mymap->end() && cntr ) {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
            it = mymap->find( processId ) ;
            --cntr ;
        }

        if ( it == mymap->end() ) {
            printf( "server not found\n" );
            done = true ;
            pt_response.put("error.response", "server not found" );
            return pt_response ;
        }

        std::stringstream strstream;
        strstream << it->second;

        mymap->erase( processId );

        if ( outMode == JSON ) {
            read_json( strstream, pt_response ) ;
        }
        if ( outMode == XML ) {
            read_xml( strstream, pt_response );
        }
        if ( outMode == INFO ) {
            read_info( strstream, pt_response );
        }

    } catch(IPC::interprocess_exception &ex) {
        cerr << ex.what() << endl << "catch IPC server" << endl;
        done = true ;
    } catch (boost::system::system_error& e) {
        cerr << e.what() << endl << "boost system_error" << endl ;
        done = true ;
    } catch (std::exception& e) {
        cerr << e.what() << endl ;
        pt_response.put("error.response", e.what() );
        done = true ;
    }
    return pt_response ;
}

int main(int argc, char* argv[]) {
    try {
        namespace po = boost::program_options;

        po::options_description desc("Allowed options");
        desc.add_options()
        ("select,s", po::value<string>(&sInputStream), "show this stream")
        ("detail,t", po::value<string>(&sInputStream), "show details of this stream")
        ("tlimitqry,m", po::value<int>(&iTimeLimitCnt)->default_value(0), "limit of elements, 0 - no limit" )
        ("hello,l", "diagnostic - hello db world")
        ("json,j", "json communication protocol")
        ("xml,x", "xml communication protocol")
        ("info,o", "info communication protocol (default)")
        ("kill,k", "kill xabracadabra server" )
        ("dir,d", "list of queries" )
        ("graphite,g", "graphite output mode")
        ("influxdb,f", "influxDB output mode")
        ("raw,r", "raw mode (default)")
        ("help,h", "show options")
        ("needctrlc,c", "force ctl+c for stop this tool")
        ;

        po::positional_options_description p;       //Assume that infile is the first option
        p.add("infile", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
        po::notify(vm);

        setbuf(stdout, NULL);

        if ( vm.count("json") ) {
            outMode = JSON ;
        }
        if ( vm.count("xml" ) ) {
            outMode = XML ;
        }
        if ( vm.count("info") ) {
            outMode = INFO ;
        }

        if ( vm.count("graphite") ) {
            outputFormatMode = GRAPHITE ;
        }
        if ( vm.count("raw") ) {
            outputFormatMode = RAW ;
        }
        if ( vm.count("influxdb") ) {
            outputFormatMode = INFLUXDB ;
        }

        if (vm.count("help")) {
            cerr << argv[0] << " - xabracadabra communication tool." << std::endl;
            cerr << "CTS:" << string( __TIMESTAMP__) << std::endl ;
            cerr << "Boost versions:" << BOOST_LIB_VERSION << std::endl ;
            cerr << desc << endl ;
            return 1;
        } else if (vm.count("hello") ) {
            ptree pt = netClient( "hello", "" ) ;
            printf ("snd: hello\n");

            string rcv("fail.") ;

            BOOST_FOREACH(ptree::value_type &v,
                pt) {
                rcv = v.second.get<std::string>("") ;
                printf( "rcv: %s %s\n", v.first.c_str(), rcv.c_str() );
            }

            if ( rcv != "world" ) {
                printf( "%s\n", rcv.c_str() );
                return 1 ;
            }

            // Fail in this part of code could means that server in in json mode
            // and query is going xml. (or otherwise)
            // catched in regressions

        } else if (vm.count("kill") ) {

            ptree pt = netClient( "kill", "" ) ;
            printf ("kill sent.\n");

        } else if (vm.count("dir") ) {

            ptree pt = netClient( "get", "" ) ;

            std::vector<string> vcols = {"", "duration", "size", "count" };
            stringstream ss ;
            for ( auto nName : vcols ) {
                int maxSize = 0 ;
                for ( const auto & v : pt.get_child("db.stream") ) {
                    if ( v.second.get<std::string>(nName).length() > maxSize ) {
                        maxSize = v.second.get<std::string>( nName ).length();
                    }
                }
                ss << "|%" ;
                ss << maxSize ;
                ss << "s";
            };
            ss << "|\n" ;

            for ( const auto & v : pt.get_child("db.stream") ) {
                printf( ss.str().c_str()
                    , v.second.get<std::string>("").c_str()
                    , v.second.get<std::string>("duration").c_str()
                    , v.second.get<std::string>("size").c_str()
                    , v.second.get<std::string>("count").c_str() );
            }

        } else if (vm.count("detail") ) {

            bool found(false);
            ptree pt = netClient( "get", "" ) ;

            cerr << "got answer" << endl ;

            for ( const auto & v : pt.get_child("db.stream") ) {
                if ( sInputStream == v.second.get<std::string>("") ) {
                    found = true ;
                }
            }

            if ( found ) {
                ptree pt = netClient( "detail", sInputStream ) ;

                for ( const auto & v : pt.get_child("db.field") ) {
                    printf( "%s.%s\n", sInputStream.c_str(), v.second.get<std::string>("").c_str() );
                }
            } else {
                printf( "not found\n" );
                return 1 ;
            }

        } else if (vm.count("select") && sInputStream != "none" ) {

            bool found ( false );

            ptree pt = netClient( "get", "" ) ;

            for ( const auto & v : pt.get_child("db.stream") ) {
                if ( sInputStream == v.second.get<std::string>("") ) {
                    streamTable[ sInputStream ] = netClient( "show", sInputStream )  ;
                    found = true ;
                    break ;
                }
            }

            printf( found ? "" : "not found\n" );
            if ( !found ) {
                return 1 ;
            }

            schema = netClient( "detail", sInputStream ) ;

            //
            // Function in this thread will start listner on udp
            //
            boost::thread producer_thread(producer);

            //
            // Function in this thrade will start fetching data from queue
            //
            boost::thread consumer_thread(consumer);

            do {
                if ( vm.count("needctrlc") ) {
                    // If this option appear - any key will not stop process
                } else {
                    if ( _kbhit() ) {
                        break ;
                    }
                }
                if ( done ) {
                    break ;
                }
                if ( iTimeLimitCnt == 1 ) {
                    break ;
                }

                boost::this_thread::sleep_for(boost::chrono::milliseconds(1));

            } while(true);

            if ( iTimeLimitCnt != 1 && !done ) {
                _getch(); //no wait ... feed key from kbhit
            }

            done = true;
            producer_thread.join();
            consumer_thread.join();
        } else {
            cerr << "use -h" << endl ;
            return 1;
        }
    } catch(IPC::interprocess_exception &ex) {
        cerr << ex.what() << endl << "catch client" << endl;
        return 1;
    } catch(std::exception& e) {
        cerr << e.what() << "\n";
        return 1;
    }

    cout << "ok." << endl ;

    return 0;
}
