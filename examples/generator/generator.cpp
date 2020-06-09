#include <iostream>
#include <fstream>
#include <string.h>
#include <cstdlib>
#include <vector>
#include <limits>       // std::numeric_limits
#include <boost/crc.hpp>
#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>
using namespace std;
using namespace boost ;

const int CRC16_CCITT_R=0x8408;  //https://en.wikipedia.org/wiki/Cyclic_redundancy_check
const int DEFAULT_SIZE_OF_PACK=1;
const int DEFAULT_PACK_COUNT=10;

unsigned int packSize = DEFAULT_SIZE_OF_PACK;
unsigned int packCount = DEFAULT_PACK_COUNT;

vector<unsigned int> data;
vector<unsigned int> remote;

string sOutputFile = "file.binary";
string sInpuFile = "";

unsigned int sawSize = std::numeric_limits<int>::max();
unsigned int rndSize = 0;
unsigned int mulSize = 1;
unsigned int crcPoly = CRC16_CCITT_R;

int main(int argc, char* argv[]) {

	namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()
	("help,h", "show options")
	("addcrc,c", po::value<unsigned int> (&packSize), "count crc after this")
	("datacount,d" , po::value<unsigned int> (&packCount), "count of genrated packs")
	("outfile,f", po::value<string> (&sOutputFile), "outputfilename, default:file.binary")
	("inputfile,k", po::value<string> (&sInpuFile), "get data from inputfile instead of algo" )
	("saw,s" , po::value<unsigned int> (&sawSize) , "algorithm: saw modulo (default:maxint)")
	("rnd,r" , po::value<unsigned int> (&rndSize) , "algorithm: random modulo (default:0)")
	("mul,m" , po::value<unsigned int> (&mulSize) , "algorithm: multiplication (default:1)")
	("crcpoly,e", po::value<unsigned int> (&crcPoly) , "crc polynominal (default:0x8408)")
	("print,p", "show data")
	;
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).
		options(desc).run(), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cerr << argv[0] << " - data generator" << std::endl;
		cerr << "The idea of this program is to generate data with or without crc."<< std::endl;
		cerr << "For testing purposes."<< std::endl;
		cerr << "Algorithm: ((rnd + (j*CRC_packSize + i)) * mul) % saw" << std::endl;
		cerr << "Examples:"<< std::endl;
		cerr << "\t./xgenerator -d 8 -s 30 -p -m 10:"<< std::endl;
		cerr << "\t\t>0 10 20 0 10 20 0 10"<< std::endl;
		cerr << "\t./xgenerator -d 8 -p"<< std::endl;
		cerr << "\t\t>0 1 2 3 4 5 6 7"<< std::endl;
		cerr << "\t./xgenerator -d 8 -p -r 20"<< std::endl;
		cerr << "\t\t>3 7 19 18 17 20 12 19"<< std::endl;
		cerr << "\t./xgenerator -d 2 -p -c 4"<< std::endl;
		cerr << "\t\t0 1 2 3 37368"<< std::endl;
		cerr << "\t\t4 5 6 7 24536"<< std::endl;
		cerr << desc << endl ;
		return system::errc::success;
	}

	auto myfile = std::fstream(sOutputFile, std::ios::out | std::ios::binary);

	if (sInpuFile != "" && vm.count("addcrc")) {

		auto remotefile = std::fstream(sInpuFile, std::ios::in | std::ios::binary);
		unsigned int val ;
		while (remotefile.read((char*)&val,sizeof(unsigned int)))
		{
			remote.push_back(val);
		}
		remotefile.close();
	}

	cerr << endl ;

	assert( remote.size() >= packCount * packSize);

	int cnt=0;
	for(auto j = 0; j < packCount; j++) {

		vector<unsigned int> crcq;

		for(auto i = 0; i < packSize; ++i) {

			auto rndVector = 0;
			unsigned int val ;
			if (sInpuFile != "" && vm.count("addcrc")) {
				val = remote[cnt++];
			} else {
				if (rndSize>0) { rndVector = rand()%rndSize; }
				val = ((rndVector + (j*packSize + i)) * mulSize)%sawSize ; 
			}
			data.push_back(val);
			if (vm.count("addcrc")) { crcq.push_back(val); }
			if (vm.count("print")) { cout << val << " "; }
		}
		
		if (vm.count("addcrc")) {

			boost::crc_basic<16> crcfn(crcPoly, 0x0, 0x0, false, false);
			crcfn.process_bytes(crcq.data(), sizeof(unsigned int) * crcq.size());
			if (vm.count("print")) { cout << crcfn.checksum() << endl ;	}
			data.push_back(crcfn.checksum());
		}
	}

	myfile.write((char*)data.data() , data.size()*sizeof(unsigned int));


	myfile.close();

    cout << "done." << endl ;
    return system::errc::success;
}
