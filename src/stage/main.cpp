#include <iostream>
#include <fstream>
#include <assert.h>
#include <map>
#include <vector>
#include <string>
#include <tuple>
#include <boost/type_index.hpp>

//https://courses.cs.vt.edu/~cs2604/fall02/binio.html
//https://stackoverflow.com/questions/1658476/c-fopen-vs-open

// Turn off buffering (this must apear before open)
// http://gcc.gnu.org/onlinedocs/libstdc++/manual/streambufs.html#io.streambuf.buffering

using namespace std;

// https://developers.google.com/protocol-buffers/docs/overview#scalar

enum fieldType
{
    String,
    Uint,
    Byte,
    Int
};

typedef int fieldLen;
typedef string fieldName;
typedef tuple<fieldName, fieldLen, fieldType> field;
enum fieldColumn
{
    name = 0,
    len = 1,
    type = 2
};

struct descriptor : vector<field>
{

    auto &operator|(const descriptor &a)
    {
        if (this != &a)
        {
            insert(end(), a.begin(), a.end());
        }
        else
        {
            assert(false); // it is not allowed to use it following way: data|data
        }
        return *this;
    }

    auto &operator=(const descriptor &a)
    {
        clear();
        insert(end(), a.begin(), a.end());
        return *this;
    }

    descriptor(const descriptor &b)
    {
        *this | b;
    }

    descriptor(fieldName n, fieldLen l, fieldType t)
    {
        push_back(field(n, l, t));
    }

    descriptor()
    {
    }

    uint getSize() const
    {
        uint size = 0;
        for (auto const i : *this)
        {
            size += get<len>(i);
        }
        return size;
    }

    friend ostream &operator<<(ostream &os, const descriptor &desc);
};

string getLenIfString(fieldType e, fieldLen l)
{
    if (e == String)
        return "[" + to_string(l) + "]";
    else
        return "";
}

string getFieldType(fieldType e)
{
    switch (e)
    {
    case String:
        return "String";
    case Uint:
        return "Uint";
    case Byte:
        return "Byte";
    case Int:
        return "Int";
    }
    assert(false);
    return "";
}

ostream &operator<<(ostream &os, const descriptor &desc)
{
    os << "{" ;
    for (auto const &r : desc)
        os << "\t" << getFieldType(get<type>(r))
           << " " << get<name>(r)
           << getLenIfString(get<type>(r), get<len>(r))
           << endl;
    os << "}[" << desc.getSize() << "]";

    return os;
}

void write(char *ptrSrc, uint size)
{
    std::fstream myFile;

    myFile.rdbuf()->pubsetbuf(0, 0);

    myFile.open("data.bin", ios::out | ios::binary | ios::app);
    assert((myFile.rdstate() & std::ifstream::failbit) == 0);
    myFile.write(ptrSrc, size);
    assert((myFile.rdstate() & std::ifstream::failbit) == 0);
    myFile.close();
}

void read(char *ptrOut, uint size)
{
    std::fstream myFile;

    myFile.rdbuf()->pubsetbuf(0, 0);

    myFile.open("data.bin", ios::in | ios::binary);
    assert((myFile.rdstate() & std::ifstream::failbit) == 0);
    myFile.seekg(0);
    myFile.read(ptrOut, size);
    assert((myFile.rdstate() & std::ifstream::failbit) == 0);
    myFile.close();
}

int main(int argc, char *argv[])
{
    const uint AREA_SIZE = 10;

    char x1[AREA_SIZE] = "test data";

    write(x1, AREA_SIZE);

    char x2[10];
    read(x2, AREA_SIZE);
    std::cout << x2 << std::endl;

    descriptor data1;

    data1.push_back(field("Name", 10, String));
    data1.push_back(field("Len", sizeof(uint), Uint));

    data1 | descriptor("Name2", 10, String) | descriptor("Control", sizeof(Byte), Byte) | descriptor("Len3", sizeof(uint), Uint);

    std::cout << data1 << std::endl;;

    //data1 | data1 ;

    std::cout << data1 << std::endl;

    descriptor data2 = descriptor("Name", 10, String) | descriptor("Control", sizeof(Byte), Byte) | descriptor("Len3", sizeof(uint), Uint);

    std::cout << data2 << std::endl;

    return 0;
}
