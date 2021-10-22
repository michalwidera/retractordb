#include <iostream>
#include <fstream>
#include <assert.h>
#include <map>
#include <vector>
#include <string>
#include <tuple>
#include <initializer_list>

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
            const bool can_not_merge_same_to_same = false;
            assert(can_not_merge_same_to_same);
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
                const bool that_name_aleardy_exist_so_it_will_make_dirty = false;
                assert(that_name_aleardy_exist_so_it_will_make_dirty);
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
        const bool did_not_find_that_record_id_descriptor = false;
        assert(did_not_find_that_record_id_descriptor);
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
        const bool record_not_found_with_that_name = false;
        assert(record_not_found_with_that_name);
        return -1;
    }

    string getRecordType(string name)
    {
        auto pos = getRecordPosition(name);
        return getFieldType(get<type>((*this)[pos]));
    }

    friend ostream &operator<<(ostream &os, const descriptor &desc);
};

ostream &operator<<(ostream &os, const descriptor &desc)
{
    os << "{";
    for (auto const &r : desc)
        os << "\t" << getFieldType(get<type>(r))
           << " " << get<name>(r)
           << getLenIfString(get<type>(r), get<len>(r))
           << endl;
    os << "}[" << desc.getSize() << "]";

    if (desc.fieldNames.size() == 0)
        os << "[dirty]";

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

    descriptor data1{field("Name3", 10, String), field("Name4", 10, String)};

    data1.append({field("Name5z", 10, String)});

    data1.push_back(field("Name", 10, String));
    data1.push_back(field("Len", sizeof(uint), Uint));

    data1 | descriptor("Name2", 10, String) | descriptor("Control", 1, Byte) | descriptor("Len3", sizeof(uint), Uint);

    std::cout << data1 << std::endl;
    ;

    descriptor data2 = descriptor("Name", 10, String) | descriptor("Len3", sizeof(uint), Uint) | descriptor("Control", 1, Byte);

    std::cout << data2 << std::endl;

    std::cout << "Field Control is at " << data2.getRecordPosition("Control") << std::endl;
    std::cout << "Field Control len is " << data2.getRecordLen("Control") << std::endl;
    std::cout << "Field Control type is " << data2.getRecordType("Control") << std::endl;
    std::cout << "Field Control offset is " << data2.getRecordOffset("Control") << std::endl;

    return 0;
}
