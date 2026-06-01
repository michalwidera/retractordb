#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>
#include <vector>
#include <array>
#include <span>
#include <limits>
#include <random>
#include <boost/crc.hpp>
#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

using namespace boost;

const int CRC16_CCITT_R       = 0x8408;  // https://en.wikipedia.org/wiki/Cyclic_redundancy_check
const int DEFAULT_SIZE_OF_PACK = 1;
const int DEFAULT_PACK_COUNT   = 10;

struct GenParams {
  unsigned int packSize;
  unsigned int sawSize;
  unsigned int rndSize;
  unsigned int mulSize;
  unsigned int crcPoly;
  bool useRemote;
  bool addCrc;
  bool addSum;
  bool print;
};

static void printHelp(const char* progName, const program_options::options_description& desc) {
  std::cerr << progName << " - data generator" << std::endl;
  std::cerr << "The idea of this program is to generate data with or without crc." << std::endl;
  std::cerr << "For testing purposes." << std::endl;
  std::cerr << "Algorithm: ((rnd + (j*CRC_packSize + i)) * mul) % saw" << std::endl;
  std::cerr << "Examples:" << std::endl;
  std::cerr << "\t./xgenerator -d 8 -s 30 -p -m 10:" << std::endl;
  std::cerr << "\t\t>0 10 20 0 10 20 0 10" << std::endl;
  std::cerr << "\t./xgenerator -d 8 -p" << std::endl;
  std::cerr << "\t\t>0 1 2 3 4 5 6 7" << std::endl;
  std::cerr << "\t./xgenerator -d 8 -p -r 20" << std::endl;
  std::cerr << "\t\t>3 7 19 18 17 20 12 19" << std::endl;
  std::cerr << "\t./xgenerator -d 2 -p -c 4" << std::endl;
  std::cerr << "\t\t0 1 2 3 37368" << std::endl;
  std::cerr << "\t\t4 5 6 7 24536" << std::endl;
  std::cerr << desc << std::endl;
}

static std::vector<unsigned int> loadRemoteData(const std::string& filename, unsigned int packCount, unsigned int packSize) {
  std::vector<unsigned int> remote;
  auto remotefile = std::fstream(filename, std::ios::in | std::ios::binary);
  std::array<std::byte, sizeof(unsigned int)> buf{};
  while (remotefile.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(buf.size()))) {
    unsigned int val{};
    std::memcpy(&val, buf.data(), sizeof(val));
    remote.push_back(val);
  }
  assert(remote.size() >= packCount * packSize);
  return remote;
}

static unsigned int computeValue(const GenParams& p, int j, int i) {
  static std::mt19937 rng{std::random_device{}()};
  int rndVector = 0;
  if (p.rndSize > 0) {
    std::uniform_int_distribution<int> dist(0, static_cast<int>(p.rndSize) - 1);
    rndVector = dist(rng);
  }
  return ((rndVector + (j * static_cast<int>(p.packSize) + i)) * p.mulSize) % p.sawSize;
}

static void processPack(const GenParams& p, int j, std::vector<unsigned int>& data,
                        const std::vector<unsigned int>& remote, int& cnt,
                        int& crcqCnt, int& sumqCnt) {
  std::vector<unsigned int> crcq;
  unsigned int sumq = 0;

  for (auto i = 0; i < static_cast<int>(p.packSize); ++i) {
    unsigned int val = p.useRemote ? remote[cnt++] : computeValue(p, j, i);
    data.push_back(val);
    if (p.addCrc) crcq.push_back(val);
    if (p.print) std::cout << val << " ";
    if (p.addSum) sumq += val;
  }

  if (p.addCrc) {
    boost::crc_basic<16> crcfn(p.crcPoly, 0x0, 0x0, false, false);
    crcfn.process_bytes(crcq.data(), sizeof(unsigned int) * crcq.size());
    if (p.print) std::cout << static_cast<unsigned int>(crcfn.checksum()) << std::endl;
    data.push_back(static_cast<unsigned int>(crcfn.checksum()));
    crcqCnt++;
  }

  if (p.addSum) {
    if (p.print) std::cout << sumq << std::endl;
    data.push_back(sumq);
    sumqCnt++;
  }
}

int main(int argc, char* argv[]) {
  unsigned int packSize   = DEFAULT_SIZE_OF_PACK;
  unsigned int packCount  = DEFAULT_PACK_COUNT;
  std::string sOutputFile = "file.binary";
  std::string sInpuFile   = "";
  unsigned int sawSize    = std::numeric_limits<int>::max();
  unsigned int rndSize    = 0;
  unsigned int mulSize    = 1;
  unsigned int crcPoly    = CRC16_CCITT_R;

  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",      "show options")
    ("addcrc,c",    po::value<unsigned int>(&packSize),   "count crc after this")
    ("addsum,u",    po::value<unsigned int>(&packSize),   "count simple sum after this")
    ("datacount,d", po::value<unsigned int>(&packCount),  "count of genrated packs")
    ("outfile,f",   po::value<std::string>(&sOutputFile), "outputfilename, default:file.binary")
    ("inputfile,k", po::value<std::string>(&sInpuFile),   "get data from inputfile instead of algo")
    ("saw,s",       po::value<unsigned int>(&sawSize),    "algorithm: saw modulo (default:maxint)")
    ("rnd,r",       po::value<unsigned int>(&rndSize),    "algorithm: random modulo (default:0)")
    ("mul,m",       po::value<unsigned int>(&mulSize),    "algorithm: multiplication (default:1)")
    ("crcpoly,e",   po::value<unsigned int>(&crcPoly),    "crc polynominal (default:0x8408)")
    ("print,p",     "show data");

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    printHelp(argv[0], desc);
    return system::errc::success;
  }

  const bool useRemote = !sInpuFile.empty() && vm.count("addcrc");
  std::vector<unsigned int> remote;
  if (useRemote) remote = loadRemoteData(sInpuFile, packCount, packSize);

  const GenParams p{packSize, sawSize, rndSize, mulSize, crcPoly,
                    useRemote,
                    static_cast<bool>(vm.count("addcrc")),
                    static_cast<bool>(vm.count("addsum")),
                    static_cast<bool>(vm.count("print"))};

  std::vector<unsigned int> data;
  int crcqCnt = 0;
  int sumqCnt = 0;
  int cnt     = 0;

  for (auto j = 0; j < static_cast<int>(packCount); ++j)
    processPack(p, j, data, remote, cnt, crcqCnt, sumqCnt);

  if (p.print) std::cout << std::endl;

  auto myfile = std::fstream(sOutputFile, std::ios::out | std::ios::binary);
  const auto bytes = std::as_bytes(std::span(data));
  myfile.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size_bytes()));
  myfile.close();

  std::cout << "count:" << packCount * packSize << std::endl;
  if (p.addCrc) std::cout << "crc cnt:" << crcqCnt << std::endl;
  if (p.addSum) std::cout << "sum cnt:" << sumqCnt << std::endl;
  std::cout << "output:" << sOutputFile << std::endl;
  std::cout << "done." << std::endl;
  return system::errc::success;
}
