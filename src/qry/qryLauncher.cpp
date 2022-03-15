#include <boost/program_options.hpp>
#include <iostream>
#include <sstream>
#include <boost/system/error_code.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/interprocess/ipc/message_queue.hpp>

#include "qry.hpp"

using namespace std ;
using namespace boost ;
using boost::property_tree::ptree;

namespace IPC = boost::interprocess ;

int main(int argc, char* argv[])
{
    // Clarification: When gcc has been upgraded to 9.x version some tests fails.
    // Bug appear when data are passing to program via script .sh
    // additional 13 (\r) character was append - this code normalize argv list.
    // C99: The parameters argc and argv and the strings pointed to by the argv array
    // shall be modifiable by the program, and retain their last-stored values
    // between program startup and program termination.
    for (int i = 0 ; i < argc ;  i ++) {
        auto len = strlen(argv[i]) ;
        if (len > 0)
            if (argv[i][len - 1] == 13)
                argv[i][len - 1] = 0;
    }
    try {
        namespace po = boost::program_options;
        po::options_description desc("Allowed options");
        desc.add_options()
        ("select,s", po::value<string> (&sInputStream), "show this stream")
        ("detail,t", po::value<string> (&sInputStream), "show details of this stream")
        ("tlimitqry,m", po::value<int> (&iTimeLimitCnt)->default_value(0), "limit of elements, 0 - no limit")
        ("hello,l", "diagnostic - hello db world")
        ("json,j", "json communication protocol")
        ("xml,x", "xml communication protocol")
        ("info,o", "info communication protocol (default)")
        ("kill,k", "kill xretractor server")
        ("dir,d", "list of queries")
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
        setbuf(stdout, nullptr);
        if (vm.count("json"))
            setmode("JSON") ;
        if (vm.count("xml"))
            setmode("XML");
        if (vm.count("info"))
            setmode("INFO");
        if (vm.count("graphite"))
            setmode("GRAPHITE") ;
        if (vm.count("raw"))
            setmode("RAW");
        if (vm.count("influxdb"))
            setmode("INFLUXDB") ;
        if (vm.count("help")) {
            cerr << argv[0] << " - xretractor communication tool." << std::endl;
            cerr << desc << endl ;
            return system::errc::success;
        } else if (vm.count("hello"))
            return hello();
        else if (vm.count("kill")) {
            ptree pt = netClient("kill", "") ;
            printf("kill sent.\n");
        } else if (vm.count("dir"))
            dir();
        else if (vm.count("detail")) {
            if (detailShow() == false)
                return system::errc::no_such_file_or_directory ;
        } else if (vm.count("select") && sInputStream != "none") {
            if (select(vm.count("needctrlc")) == false)
                return system::errc::no_such_file_or_directory;
        } else {
            cout << argv[0] << ": fatal error: no argument" << endl ;
            cout << "query receiver terminated." << endl ;
            return EPERM ; //ERROR defined in errno-base.h
        }
    } catch (IPC::interprocess_exception &ex) {
        cerr << ex.what() << endl << "catch client" << endl;
        return system::errc::no_child_process;
    } catch (std::exception &e) {
        cerr << e.what() << endl;
        return system::errc::interrupted;
    }
    cout << "ok." << endl ;
    return system::errc::success;
}
