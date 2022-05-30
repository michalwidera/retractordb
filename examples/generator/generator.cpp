#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <limits>       // std::numeric_limits
#include <boost/crc.hpp>
#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

using namespace boost ;

const int CRC16_CCITT_R=0x8408;  //https://en.wikipedia.org/wiki/Cyclic_redundancy_check
const int DEFAULT_SIZE_OF_PACK=1;
const int DEFAULT_PACK_COUNT=10;

unsigned int packSize = DEFAULT_SIZE_OF_PACK;
unsigned int packCount = DEFAULT_PACK_COUNT;

std::vector<unsigned int> data;
std::vector<unsigned int> remote;

std::string sOutputFile = "file.binary";
std::string sInpuFile = "";

unsigned int sawSize = std::numeric_limits<int>::max();
unsigned int rndSize = 0;
unsigned int mulSize = 1;
unsigned int crcPoly = CRC16_CCITT_R;

int main(int argc, char* argv[])
{

    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help,h", "show options")
    ("addcrc,c", po::value<unsigned int> (&packSize), "count crc after this")
    ("addsum,u", po::value<unsigned int> (&packSize), "count simple sum after this")
    ("datacount,d", po::value<unsigned int> (&packCount), "count of genrated packs")
    ("outfile,f", po::value<std::string> (&sOutputFile), "outputfilename, default:file.binary")
    ("inputfile,k", po::value<std::string> (&sInpuFile), "get data from inputfile instead of algo" )
    ("saw,s", po::value<unsigned int> (&sawSize), "algorithm: saw modulo (default:maxint)")
    ("rnd,r", po::value<unsigned int> (&rndSize), "algorithm: random modulo (default:0)")
    ("mul,m", po::value<unsigned int> (&mulSize), "algorithm: multiplication (default:1)")
    ("crcpoly,e", po::value<unsigned int> (&crcPoly), "crc polynominal (default:0x8408)")
    ("print,p", "show data")
    ;
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cerr << argv[0] << " - data generator" << std::endl;
        std::cerr << "The idea of this program is to generate data with or without crc."<< std::endl;
        std::cerr << "For testing purposes."<< std::endl;
        std::cerr << "Algorithm: ((rnd + (j*CRC_packSize + i)) * mul) % saw" << std::endl;
        std::cerr << "Examples:"<< std::endl;
        std::cerr << "\t./xgenerator -d 8 -s 30 -p -m 10:"<< std::endl;
        std::cerr << "\t\t>0 10 20 0 10 20 0 10"<< std::endl;
        std::cerr << "\t./xgenerator -d 8 -p"<< std::endl;
        std::cerr << "\t\t>0 1 2 3 4 5 6 7"<< std::endl;
        std::cerr << "\t./xgenerator -d 8 -p -r 20"<< std::endl;
        std::cerr << "\t\t>3 7 19 18 17 20 12 19"<< std::endl;
        std::cerr << "\t./xgenerator -d 2 -p -c 4"<< std::endl;
        std::cerr << "\t\t0 1 2 3 37368"<< std::endl;
        std::cerr << "\t\t4 5 6 7 24536"<< std::endl;
        std::cerr << desc << std::endl ;
        return system::errc::success;
    }

    auto myfile = std::fstream(sOutputFile, std::ios::out | std::ios::binary);

    if (sInpuFile != "" && vm.count("addcrc"))
    {

        auto remotefile = std::fstream(sInpuFile, std::ios::in | std::ios::binary);
        unsigned int val ;
        while (remotefile.read((char*)&val,sizeof(unsigned int)))
        {
            remote.push_back(val);
        }
        remotefile.close();

        assert( remote.size() >= packCount * packSize);
    }

    int crcqCnt=0; //Count of crc that landend in output file
    int sumqCnt=0; //Count of crc that landend in output file
    int cnt=0; //Count of data read from remote file
    for(auto j = 0; j < packCount; j++)
    {

        std::vector<unsigned int> crcq;
        unsigned int sumq = 0;

        for(auto i = 0; i < packSize; ++i)
        {

            auto rndVector = 0;
            unsigned int val ;
            if (sInpuFile != "" && vm.count("addcrc"))
            {
                val = remote[cnt++];
            }
            else
            {
                if (rndSize>0)
                {
                    rndVector = rand()%rndSize;
                }
                val = ((rndVector + (j*packSize + i)) * mulSize)%sawSize ;
            }
            data.push_back(val);
            if (vm.count("addcrc"))
            {
                crcq.push_back(val);
            }
            if (vm.count("print"))
            {
                std::cout << val << " ";
            }

            if (vm.count("addsum"))
            {
                sumq+=val;
            }
        }

        if (vm.count("addcrc"))
        {

            boost::crc_basic<16> crcfn(crcPoly, 0x0, 0x0, false, false);
            crcfn.process_bytes(crcq.data(), sizeof(unsigned int) * crcq.size());
            if (vm.count("print"))
            {
                std::cout << crcfn.checksum() << std::endl ;
            }
            data.push_back(crcfn.checksum());
            crcqCnt++;
        }

        if (vm.count("addsum"))
        {
            if (vm.count("print"))
            {
                std::cout << sumq << std::endl ;
            }
            data.push_back(sumq);
            sumqCnt++;
        }
    }

    if (vm.count("print"))
    {
        std::cout << std::endl ;
    }

    myfile.write((char*)data.data(), data.size()*sizeof(unsigned int));
    myfile.close();

    std::cout << "count:"<< packCount * packSize << std::endl;
    if (vm.count("addcrc"))
    {
        std::cout << "crc cnt:"<< crcqCnt << std::endl;
    }
    if (vm.count("addsum"))
    {
        std::cout << "sum cnt:"<< sumqCnt << std::endl;
    }
    std::cout << "output:" << sOutputFile << std::endl;
    std::cout << "done." << std::endl;
    return system::errc::success;
}
