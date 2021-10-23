#include <iostream>
#include <fstream>
#include <assert.h>
#include <map>
#include <vector>
#include <string>
#include <tuple>
#include <initializer_list>
#include <locale>

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

struct synsugar_is_space : std::ctype<char>
{
    synsugar_is_space() : std::ctype<char>(get_table()) {}
    static mask const *get_table()
    {
        static mask rc[table_size];
        rc['['] = std::ctype_base::space;
        rc[']'] = std::ctype_base::space;
        rc['{'] = std::ctype_base::space;
        rc['}'] = std::ctype_base::space;
        rc[' '] = std::ctype_base::space;
        rc['\n'] = std::ctype_base::space;
        return &rc[0];
    }
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
    assert(false && "Didn't find such filed type name as fieldType");
    return "";
}

fieldType getFieldType(string e)
{
    if (e.compare("String") == 0)
        return String;
    if (e.compare("Uint") == 0)
        return Uint;
    if (e.compare("Byte") == 0)
        return Byte;
    if (e.compare("Int") == 0)
        return Int;

    assert(false && "Didn't find such filed type name as string");
    return Byte;
}

struct descriptor : vector<field>
{

    map<string, int> fieldNames;

    descriptor(initializer_list<field> l) : vector(l)
    {
    }

    descriptor(fieldName n, fieldLen l, fieldType t)
    {
        push_back(field(n, l, t));
        updateNames();
    }

    descriptor()
    {
    }

    void append(std::initializer_list<field> l)
    {
        insert(end(), l.begin(), l.end());
    }

    auto &operator|(const descriptor &a)
    {
        if (this != &a)
        {
            insert(end(), a.begin(), a.end());
        }
        else
        {
            //descriptor b(*this);
            //insert(end(), b.begin(), b.end());
            assert(false && "can not merge same to same");
            // can't do safe: data | data
            // due one name policy
        }
        updateNames();
        return *this;
    }

    auto &operator=(const descriptor &a)
    {
        clear();
        insert(end(), a.begin(), a.end());
        updateNames();
        return *this;
    }

    descriptor(const descriptor &b)
    {
        *this | b;
        updateNames();
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

    bool updateNames()
    {
        uint position = 0;
        fieldNames.clear();
        for (auto const i : *this)
        {
            if (fieldNames.find(get<fieldName>(i)) == fieldNames.end())
            {
                fieldNames[get<fieldName>(i)] = position;
            }
            else
            {
                fieldNames.clear();
                assert(false && "that name aleardy exist so it will make dirty");
                return false;
            }
            position++;
        }
        return true;
    }

    int getRecordPosition(string name)
    {
        if (fieldNames.find(name) != fieldNames.end())
            return fieldNames[name];
        assert(false && "did not find that record id descriptor");
        return -1;
    }

    int getRecordLen(string name)
    {
        auto pos = getRecordPosition(name);
        return get<len>((*this)[pos]);
    }

    int getRecordOffset(string name)
    {
        auto offset = 0;
        for (auto const i : *this)
        {
            if (name == get<fieldName>(i))
                return offset;
            offset += get<len>(i);
        }
        assert(false && "record not found with that name");
        return -1;
    }

    string getRecordType(string name)
    {
        auto pos = getRecordPosition(name);
        return getFieldType(get<type>((*this)[pos]));
    }

    friend ostream &operator<<(ostream &os, const descriptor &desc);
    friend istream &operator>>(istream &is, descriptor &desc);
};

ostream &operator<<(ostream &os, const descriptor &desc)
{
    os << "{";
    for (auto const &r : desc)
        os << "\t" << getFieldType(get<type>(r))
           << " " << get<name>(r)
           << getLenIfString(get<type>(r), get<len>(r))
           << endl;
    os << "}";

    if (desc.fieldNames.size() == 0)
        os << "[dirty]";

    return os;
}

istream &operator>>(istream &is, descriptor &desc)
{
    do
    {
        string type;
        string name;
        int len = 0;
        is >> type;
        if (is.eof()){
            break;
        }
        is >> name;
        if (is.eof()){
            break;
        }

        fieldType ft = getFieldType(type);

        if (ft == String)
        {
            is >> len;
        }
        else if (ft == Uint)
        {
            len = sizeof(uint);
        }
        else if (ft == Int)
        {
            len = sizeof(int);
        }
        else if (ft == Byte)
        {
            len = 1;
        }
        else
        {
            assert(len != 0 && "Undefined type");
        }

        desc | descriptor(name, len, ft);
    } while (!is.eof());
    return is;
}

struct binaryStream
{
};

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

    descriptor data1{field("Name3", 10, String), field("Name4", 10, String)};

    data1.append({field("Name5z", 10, String)});

    auto testf = field("Name6z", 10, String);

    data1.append({testf});

    data1.push_back(field("Name", 10, String));
    data1.push_back(field("Len", sizeof(uint), Uint));

    data1 | descriptor("Name2", 10, String) | descriptor("Control", 1, Byte) | descriptor("Len3", sizeof(uint), Uint);

    std::cout << data1 << std::endl;

    descriptor data2 = descriptor("Name", 10, String) | descriptor("Len3", sizeof(uint), Uint) | descriptor("Control", 1, Byte);

    std::cout << data2 << std::endl;

    std::cout << "Field Control is at " << data2.getRecordPosition("Control") << std::endl;
    std::cout << "Field Control len is " << data2.getRecordLen("Control") << std::endl;
    std::cout << "Field Control type is " << data2.getRecordType("Control") << std::endl;
    std::cout << "Field Control offset is " << data2.getRecordOffset("Control") << std::endl;

    cin.imbue(locale(cin.getloc(), new synsugar_is_space));

    descriptor data3;
    std::cin >> data3;
    std::cout << data3 << std::endl;

    return 0;
}
