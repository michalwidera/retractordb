#include <boost/program_options.hpp>
#include <iostream>
#include <sstream>
#include <boost/system/error_code.hpp>
#include <boost/property_tree/ptree.hpp>

//for: BOOST_FOREACH( cmd_e i, commands | map_keys )
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/map.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/containers/map.hpp>

#include "qry.hpp"

using namespace std ;
using namespace boost ;
using boost::property_tree::ptree;

namespace IPC = boost::interprocess ;

int main(int argc, char* argv[]) {

    // Clarification: When gcc has been upgraded to 9.x version some tests fails.
    // Bug appear when data are passing to program via script .sh
    // additional 13 (\r) character was append - this code normalize argv list.
    // C99: The parameters argc and argv and the strings pointed to by the argv array
    // shall be modifiable by the program, and retain their last-stored values
    // between program startup and program termination.
    for ( int i = 0 ; i < argc ;  i ++ )
    {
        auto len = strlen( argv[i] ) ;
        if ( len > 0 )
            if ( argv[i][len-1] == 13 ) argv[i][len-1] = 0 ;
    }

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
        ("kill,k", "kill xretractor server" )
        ("dir,d", "list of queries" )
        ("graphite,g", "graphite output mode")
        ("influxdb,f", "influxDB output mode")
        ("raw,r", "raw mode (default)")
        ("help,h", "show options")
        ("needctrlc,c", "force ctl+c for stop this tool")
        ;

        po::positional_options_description p;       //Assume that select is the first option
        p.add("select", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
        po::notify(vm);

        setbuf(stdout, NULL);

        if ( vm.count("json") ) {
            setmode("JSON") ;
        }
        if ( vm.count("xml" ) ) {
            setmode("XML");
        }
        if ( vm.count("info") ) {
            setmode("INFO");
        }

        if ( vm.count("graphite") ) {
            setmode("GRAPHITE") ;
        }
        if ( vm.count("raw") ) {
            setmode("RAW");
        }
        if ( vm.count("influxdb") ) {
            setmode("INFLUXDB") ;
        }

        if (vm.count("help")) {

            cerr << argv[0] << " - xretractor communication tool." << std::endl;
            cerr << desc << endl ;
            return system::errc::success;

        } else if (vm.count("hello") ) {

            return hello();

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

                cerr << "not found" << endl ;
                return system::errc::no_such_file_or_directory ;
            }

        } else if (vm.count("select") && sInputStream != "none" ) {
            
            select( vm.count("needctrlc") );

        } else {

            cerr << "use -h" << endl ;
            return system::errc::invalid_argument ;
        }

    } catch(IPC::interprocess_exception &ex) {

        cerr << ex.what() << endl << "catch client" << endl;
        return system::errc::no_child_process;

    } catch(std::exception& e) {

        cerr << e.what() << endl;
        return system::errc::interrupted;

    }

    cout << "ok." << endl ;

    return system::errc::success;
}
