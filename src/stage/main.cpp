#include <iostream>
#include <fstream>
#include <assert.h>
#include <map>
#include <vector>
#include <string>
#include <tuple>

//https://courses.cs.vt.edu/~cs2604/fall02/binio.html
//https://stackoverflow.com/questions/1658476/c-fopen-vs-open

// Turn off buffering (this must apear before open)
// http://gcc.gnu.org/onlinedocs/libstdc++/manual/streambufs.html#io.streambuf.buffering

using namespace std;

// https://developers.google.com/protocol-buffers/docs/overview#scalar


enum typeInfo {
    String,
    Uint,
    Byte,
    Int
} ;

typedef int recordLen;
typedef string recordName;
typedef tuple <recordName, recordLen, typeInfo> record ;

struct binaryProtocol {
    string fileLocation;
    vector <record> recordStruct;

    // This enables binaryProtocol << record << record chaining

    template< typename T>
    binaryProtocol &operator<<(const T& b) {
        recordStruct.push_back(b);
        return *this;
    }
};

/*

message Person {
    string name[10];
    uint len;
    byte status;
    int cost;
}

binaryprotocol data(String("name",10).Uint("len").Byte("status").Int("cost"));
*/

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

    // test

    binaryProtocol data1;

    data1.recordStruct.push_back(record( "Name" , 10 , String ));
    data1.recordStruct.push_back(record( "Len" , sizeof(uint) , Uint ));

    data1 << record( "Name2" , 10 , String ) << record( "Len2" , sizeof(uint) , Uint ) ;

    for(auto const& value: data1.recordStruct) {
        cout << get<0>(value) << "\t" << get<1>(value) << "\t" << get<2>(value) << endl;
    }
    return 0;
}