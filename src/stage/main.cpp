#include <iostream>
#include <fstream>
#include <assert.h>
#include <map>
#include <vector>
#include <string>

//https://courses.cs.vt.edu/~cs2604/fall02/binio.html
//https://stackoverflow.com/questions/1658476/c-fopen-vs-open

// Turn off buffering (this must apear before open)
// http://gcc.gnu.org/onlinedocs/libstdc++/manual/streambufs.html#io.streambuf.buffering

using namespace std;

// https://developers.google.com/protocol-buffers/docs/overview#scalar

typedef uint ProtocolTypeSize;

class FileAggregator {
    string fileLocation;
    vector < uint > recordStruct;
};


/*

message Person {
    string name[10];
    uint len;
    byte status;
    int cost;
}

binaryprotocol data = String("name",10) | Uint("len") | Byte("status") | Int("cost");
*/

class String {

}

void write( char * ptrSrc , uint size )
{
    std::fstream myFile;

    myFile.rdbuf()->pubsetbuf(0, 0);

    myFile.open("data.bin", ios::out | ios::binary | ios::app );
    assert((myFile.rdstate() & std::ifstream::failbit ) == 0);
    myFile.write(ptrSrc, size);
    assert((myFile.rdstate() & std::ifstream::failbit ) == 0);
    myFile.close();
}

void read(char * ptrOut, uint size )
{
    std::fstream myFile;

    myFile.rdbuf()->pubsetbuf(0, 0);

    myFile.open("data.bin", ios::in | ios::binary );
    assert((myFile.rdstate() & std::ifstream::failbit ) == 0);
    myFile.seekg(0);
    myFile.read(ptrOut, size);
    assert((myFile.rdstate() & std::ifstream::failbit ) == 0);
    myFile.close();
}

int main(int argc, char* argv[])
{
    std::cout << "File stage" << std::endl;

    const uint AREA_SIZE = 10 ;

    char x1[AREA_SIZE] = "test data";

    write( x1 , AREA_SIZE );

    char x2[10];
    read(x2,AREA_SIZE);
    std::cout << x2 << std::endl;

    return 0;
}