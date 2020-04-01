#include <boost/program_options.hpp>
#include <iostream>
#include <sstream>
#include <boost/system/error_code.hpp>
#include <boost/regex.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

#include <fstream>
#include <iostream>

#include <memory>

#include "../share/QStruct.h"

#include "compiler.hpp"

using namespace std;
using namespace boost;

using boost::lexical_cast;

extern string parser( string sInputFile, string sOutputFile, bool verbose = true);

std::streambuf* oldbuf = std::clog.rdbuf(clog.rdbuf());

extern qTree coreInstance ;

int main(int argc, char* argv[]) {
    // Clarification: When gcc has been upgraded to 9.x version some tests fails.
    // Bug appear when data are passing to program via script .sh
    // additional 13 (\r) character was append - this code normalize argv list.
    // C99: The parameters argc and argv and the strings pointed to by the argv array
    // shall be modifiable by the program, and retain their last-stored values
    // between program startup and program termination.
    for ( int i = 0 ; i < argc ;  i ++ ) {
        auto len = strlen( argv[i] ) ;

        if ( len > 0 )
            if ( argv[i][len - 1] == 13 ) {
                argv[i][len - 1] = 0 ;
            }
    }

    try {
        std::unique_ptr<ostream> p_ofs;
        namespace po = boost::program_options;
        string sOutputFile ;
        string sInputFile ;
        po::options_description desc("Avaiable options");
        desc.add_options()
        ("help,h", "Show program options")
        ("queryfile,q", po::value<string>(&sInputFile), "query set file")
        ("outfile,o", po::value<string>(&sOutputFile)->default_value("query.qry"), "output file")
        ("log,g", "Translation raport - debuglog")
        ("verbose,v", "Diangostic info")
        ;
        po::positional_options_description p;       //Assume that infile is the first option
        p.add("queryfile", -1);
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(), vm);
        po::notify(vm);

        if ( vm.count("queryfile") == 0 ) {
            cout << argv[0] << ": fatal error: no input file" << endl ;
            cout << "compilation terminated." << endl ;
            return EPERM ; //ERROR defined in errno-base.h
        }

        if (vm.count("help")) {
            cout << desc << "\n";
            return system::errc::success;
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

        string sQueryFile = vm["queryfile"].as< string >() ;
        string sOutFile   = vm["outfile"].as< string >() ;
        //defaults
        string sLinkFile  = "query.lkn" ;
        string sSubsetFile = "query.sub" ;
        //override defaults if filename is matching regexp
        {
            boost::regex filenameRe("(\\w*).(\\w*)");  //filename.extension
            boost::cmatch what;

            if ( regex_match(sQueryFile.c_str(), what, filenameRe) ) {
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
            ifstream input(sQueryFile.c_str());
            getline(input, str);
            boost::cmatch what;
            boost::regex mulitestRe("set\\[(\\d*)\\,(\\d*)\\]");  //multiset[2-7]

            if (regex_match(str.c_str(), what, mulitestRe)) {
                sQueryFile = sSubsetFile;
                ofstream querySubset(sQueryFile.c_str());
                const int dGivenSetBeg(atoi( string( what[1]).c_str()));
                const int dGivenSetEnd(atoi( string( what[2]).c_str()));
                input.seekg(0, ios_base::beg);
                int lineNr(0);
                list <string> querySet;

                while (getline(input, str)) {
                    if (lineNr >= dGivenSetBeg && lineNr <= dGivenSetEnd) {
                        querySubset << str << "\n";
                    }

                    lineNr++;
                }

                cout << "Subset ...\t" << dGivenSetBeg << "-" << dGivenSetEnd << endl;
            }
        }
        cout << "Input file:" << sQueryFile << endl ;
        cout << "Compile result:" << parser( sQueryFile, sLinkFile, vm.count("verbose")) << endl ;
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
        dumpInstance( sOutFile, vm.count("verbose"));
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
    } catch(std::exception &e) {
        clog.rdbuf(oldbuf);
        cerr << e.what() << "\n";
        return system::errc::interrupted;
    }

    return system::errc::success;
}
