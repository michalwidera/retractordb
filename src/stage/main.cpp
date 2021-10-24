#include <iostream>
#include <fstream>
#include <assert.h>
#include <map>
#include <vector>
#include <string>
#include <tuple>
#include <initializer_list>
#include <locale>
#include <memory>
#include <sstream>

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

// Look here to explain: https://stackoverflow.com/questions/7302996/changing-the-delimiter-for-cin-c
struct synsugar_is_space : ctype<char>
{

    synsugar_is_space() : ctype<char>(get_table()) {}
    static mask const *get_table()
    {
        static mask rc[table_size];
        rc['['] = rc[']'] = rc['{'] = rc['}'] = rc[' '] = rc['\n'] = ctype_base::space;
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

    void append(initializer_list<field> l)
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
        if (is.eof())
        {
            break;
        }
        is >> name;

        fieldType ft = getFieldType(type);

        switch (ft)
        {
        case String:
            is >> len;
            break;
        case Uint:
            len = sizeof(uint);
            break;
        case Int:
            len = sizeof(int);
            break;
        case Byte:
            len = 1;
            break;
        default:
            assert(len != 0 && "Undefined type");
        }

        desc | descriptor(name, len, ft);

    } while (!is.eof());

    return is;
}

//https://en.cppreference.com/w/cpp/io/ios_base/openmode
//https://stackoverflow.com/questions/15063985/opening-a-binary-output-file-stream-without-truncation

void append(char *ptrData, uint size)
{
    fstream myFile;

    myFile.rdbuf()->pubsetbuf(0, 0);

    myFile.open("data.bin", ios::out | ios::binary | ios::app | ios::ate);
    assert((myFile.rdstate() & ofstream::failbit) == 0);
    //myFile.seekp(0,ios::end); <- not needed due ios::ate
    myFile.write(ptrData, size);
    assert((myFile.rdstate() & ofstream::failbit) == 0);
    myFile.close();
}

void read(char *ptrData, uint size, uint position)
{
    fstream myFile;

    myFile.rdbuf()->pubsetbuf(0, 0);

    myFile.open("data.bin", ios::in | ios::binary);
    assert((myFile.rdstate() & ifstream::failbit) == 0);
    myFile.seekg(position);
    assert((myFile.rdstate() & ifstream::failbit) == 0);
    myFile.read(ptrData, size);
    assert((myFile.rdstate() & ifstream::failbit) == 0);
    myFile.close();
}

void update(char *ptrData, uint size, uint position)
{
    fstream myFile;

    myFile.rdbuf()->pubsetbuf(0, 0);

    myFile.open("data.bin",  ios::in | ios::out | ios::binary | ios::ate );
    assert((myFile.rdstate() & ofstream::failbit) == 0);
    myFile.seekp(position);
    assert((myFile.rdstate() & ofstream::failbit) == 0);
    myFile.write(ptrData, size);
    assert((myFile.rdstate() & ofstream::failbit) == 0);
    myFile.close();
}

// ------------------------------------------------------------------------
// Interface

void update(string file, char *outBuffer, uint offsetFromHead)
{
}

void read(string file, char *inBuffer, uint offsetFromHead)
{
}

void append(string file, char *outBuffer)
{
}

int main(int argc, char *argv[])
{
    const uint AREA_SIZE = 10;

    auto status = remove("data.bin");    

    {
        char xData[AREA_SIZE] = "test data";
        //                       0123456789

        append(xData, AREA_SIZE);
        append(xData, AREA_SIZE); // Add one extra record

        assert(strcmp(xData, "test data") == 0);

        char yData[AREA_SIZE];
        read(yData, AREA_SIZE, 0);
        cout << yData << endl;
        assert(strcmp(yData, "test data") == 0);
    }

    {
        char xData[AREA_SIZE] = "test updt";
        //                       0123456789

        update(xData, AREA_SIZE, 0);

        char yData[AREA_SIZE];

        read(yData, AREA_SIZE, 0);
        cout << yData << endl;
        assert(strcmp(yData, "test updt") == 0);

        read(yData, AREA_SIZE, AREA_SIZE);
        cout << yData << endl;
        assert(strcmp(yData, "test data") == 0);
    }

    // Revert data to default.
    {
        char xData[AREA_SIZE] = "test data";
        //                       0123456789

        update(xData, AREA_SIZE, 0);

        // update -> data in file

        char yData[AREA_SIZE];
        read(yData, AREA_SIZE, 0);
        cout << yData << endl;
        assert(strcmp(yData, "test data") == 0);
    }

    descriptor data1{field("Name3", 10, String), field("Name4", 10, String)};

    data1.append({field("Name5z", 10, String)});

    auto testf = field("Name6z", 10, String);

    data1.append({testf});

    data1.push_back(field("Name", 10, String));
    data1.push_back(field("Len", sizeof(uint), Uint));

    data1 | descriptor("Name2", 10, String) | descriptor("Control", 1, Byte) | descriptor("Len3", sizeof(uint), Uint);

    cout << data1 << endl;

    descriptor data2 = descriptor("Name", 10, String) | descriptor("Len3", sizeof(uint), Uint) | descriptor("Control", 1, Byte);

    cout << data2 << endl;

    cout << "Field Control is at " << data2.getRecordPosition("Control") << endl;
    cout << "Field Control len is " << data2.getRecordLen("Control") << endl;
    cout << "Field Control type is " << data2.getRecordType("Control") << endl;
    cout << "Field Control offset is " << data2.getRecordOffset("Control") << endl;

    //start cin test
    //https://stackoverflow.com/questions/14550187/how-to-put-data-in-cin-from-string
    streambuf* orig = cin.rdbuf();
    //Note: This mess is intended here in test
    istringstream input("{ String Name3[10]\nString Name[10]\nUint Len String Name2 10 Byte Control Uint Len3 }");
    cin.rdbuf(input.rdbuf());
    // Test goes here

    cin.imbue(locale(cin.getloc(), unique_ptr<synsugar_is_space>(new synsugar_is_space).release()));
    descriptor data3;
    cin >> data3;
    cout << data3 << endl;

    // Revert to orginal cin
    std::cin.rdbuf(orig);
    // end cin test

    return 0;
}

