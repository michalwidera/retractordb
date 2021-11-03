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

template <typename L, typename R>
map<R, L>
makeReverse(map<L, R> list)
{
    map<R, L> retVal;
    for (const auto &[key, value] : list)
        retVal[value] = key;
    return retVal;
}

map<string, fieldType> typeDictionary = {{"String", String}, {"Uint", Uint}, {"Byte", Byte}, {"Int", Int}};
map<fieldType, string> typeDictionaryR = makeReverse(typeDictionary);

fieldType getFieldType(string e)
{
    return typeDictionary[e];
}

string getFieldType(fieldType e)
{
    return typeDictionaryR[e];
}

int getFieldLenFromType(fieldType ft)
{
    switch (ft)
    {
    case Uint:
        return sizeof(uint);
    case Int:
        return sizeof(int);
    case Byte:
        return 1;
    case String:
        assert(len != 0 && "String has other method of len-type id");
        break;
    default:
        assert(len != 0 && "Undefined type");
    }
    return 0;
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

    descriptor(fieldName n, fieldType t)
    {
        assert(t != String && "This method does not work for Stings");
        push_back(field(n, getFieldLenFromType(t), t));
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

    int Position(string name)
    {
        if (fieldNames.find(name) != fieldNames.end())
            return fieldNames[name];
        cerr << "[" << name << "]" << endl;
        assert(false && "did not find that record id descriptor:{}");
        return -1;
    }

    int Len(string name)
    {
        auto pos = Position(name);
        return get<len>((*this)[pos]);
    }

    int Offset(string name)
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

    string Type(string name)
    {
        auto pos = Position(name);
        return getFieldType(get<type>((*this)[pos]));
    }

    friend ostream &operator<<(ostream &os, const descriptor &desc);
    friend istream &operator>>(istream &is, descriptor &desc);
};

ostream &operator<<(ostream &os, const descriptor &desc)
{
    os << "{";
    for (auto const &r : desc)
    {
        os << "\t" << getFieldType(get<type>(r))
           << " " << get<name>(r);
        if (get<type>(r) == String)
        {
            os << "[" << to_string(get<len>(r)) << "]";
        };
        os << endl;
    }
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

        if (ft == String)
        {
            is >> len;
        }
        else
        {
            len = getFieldLenFromType(ft);
        }

        desc | descriptor(name, len, ft);

    } while (!is.eof());

    return is;
}

// https://en.cppreference.com/w/cpp/io/ios_base/openmode
// https://stackoverflow.com/questions/15063985/opening-a-binary-output-file-stream-without-truncation

struct binaryFileAccessorInterface
{
    virtual void append_(const byte *ptrData, const uint size) = 0;
    virtual void read_(byte *ptrData, const uint size, const uint position) = 0;
    virtual void update_(const byte *ptrData, const uint size, const uint position) = 0;
    virtual string name_() = 0;

    virtual inline ~binaryFileAccessorInterface(){};
    binaryFileAccessorInterface(){};
};

struct genericBinaryFileAccessor : public binaryFileAccessorInterface
{
    string fileName;

    genericBinaryFileAccessor(string fileName) : fileName(fileName){};

    void append_(const byte *ptrData, const uint size)
    {
        fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(fileName, ios::out | ios::binary | ios::app | ios::ate);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.write(reinterpret_cast<const char*>(ptrData), size);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.close();
    };

    void read_(byte *ptrData, const uint size, const uint position)
    {
        fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(fileName, ios::in | ios::binary);
        assert((myFile.rdstate() & ifstream::failbit) == 0);
        myFile.seekg(position);
        assert((myFile.rdstate() & ifstream::failbit) == 0);
        myFile.read(reinterpret_cast<char*>(ptrData), size);
        assert((myFile.rdstate() & ifstream::failbit) == 0);
        myFile.close();
    };

    void update_(const byte *ptrData, const uint size, const uint position)
    {
        fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(fileName, ios::in | ios::out | ios::binary | ios::ate);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.seekp(position);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.write(reinterpret_cast<const char*>(ptrData), size);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.close();
    };

    string name_() {
        return fileName;
    }
};

map<string, descriptor> schema;

struct fileAccessor
{

    binaryFileAccessorInterface *pAccessor;

    fileAccessor(const descriptor desc, binaryFileAccessorInterface &accessor)
        : pAccessor(&accessor)
    {
        create(desc);
    };

    void update(const byte *outBuffer, const uint offsetFromHead)
    {
        auto size = schema[pAccessor->name_()].getSize();
        pAccessor->update_(outBuffer, size, offsetFromHead * size);
    };

    void read(byte *inBuffer, const uint offsetFromHead)
    {
        auto size = schema[pAccessor->name_()].getSize();
        pAccessor->read_(inBuffer, size, offsetFromHead * size);
    };

    void append(const byte *outBuffer)
    {
        auto size = schema[pAccessor->name_()].getSize();
        pAccessor->append_(outBuffer, size);
    };

    void create(const descriptor desc)
    {
        schema[pAccessor->name_()] = desc;
        fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        string fileDesc(pAccessor->name_());
        fileDesc.append(".desc");

        myFile.open(fileDesc, ios::out);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile << desc;
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.close();
    };
};

// ------------------------------------------------------------------------
// Interface

struct rawAccessor
{

    descriptor *pDesc;
    byte *ptr;
    rawAccessor(descriptor &desc, byte *ptr) : pDesc(&desc),
                                               ptr(ptr)
    {
    }

    rawAccessor() = delete;

    string make_string(const string fieldName)
    {
        auto len = pDesc->Len(fieldName);
        string ret(len, 0);
        memcpy(&ret[0], ptr + pDesc->Offset(fieldName), len);
        return ret;
    };

    template <typename T>
    auto cast(const string fieldName)
    {
        return *(reinterpret_cast<T *>(ptr + pDesc->Offset(fieldName)));
    };
};

int main(int argc, char *argv[])
{

    // Diagnostic code

#ifdef NDEBUG
    cerr << endl;
    cerr << "\x1B[31m";
    cerr << "Release Code form - Failure." << endl;
    cerr << "\033[0m";

    assert(false); // Note this assert will have no effect!

    cerr << endl;
    cerr << "         This is Staging/Testing code." << endl;
    cerr << "         You compiled it as RELEASE - no assert will affect here!" << endl;
    cerr << "         Rebuild code asap with follwoing command:" << endl;
    cerr << "         conan install .. -s build_type=Debug && conan build .." << endl;
    cerr << "         make install && xstage || echo Fail" << endl;

    exit(1);

    // If meet: ndefined reference to `boost::program_options::options_description::options_description
    // There were changes to the <string> class in the C++11 standard which may conflict with versions
    // of the Boost library that were compiled with a non-C++11 compiler (such as G++-4.8).
    // Try recompiling boost or using a version of C++ compiler that was used to compile your Boost libraries.
    // Ref: https://stackoverflow.com/questions/12179154/undefined-reference-to-boostprogram-optionsoptions-descriptionm-default-l

    // Rebuild boost&gtest with conanfile.py compiler settings.
    // conan install .. -s build_type=Debug --build=boost --build=gtest

#else
    // https://stackoverflow.com/questions/4053837/colorizing-text-in-the-console-with-c
    struct check_assert
    {
        bool ok()
        {
            cout << "\x1B[32mDebug Code Form - Ok.\n\033[0m";
            return true;
        }
    } check;
    assert(check.ok()); // This assert show that assert is compiled and works.
                        // Program will show green "Ok." at the end of work if assert is compiled and executed.
#endif

    const uint AREA_SIZE = 10;

    descriptor voidDescriptor({descriptor("Name", 10, String)});
    genericBinaryFileAccessor binaryAccessor("testfile");
    fileAccessor fAcc(voidDescriptor, binaryAccessor);

    {

        byte xData[AREA_SIZE] ;
        memcpy(xData,"test data",AREA_SIZE);
        //                                     0123456789

        binaryAccessor.append_(xData, AREA_SIZE);
        binaryAccessor.append_(xData, AREA_SIZE); // Add one extra record

        assert(strcmp(reinterpret_cast<char*>(xData), "test data") == 0);

        byte yData[AREA_SIZE];
        binaryAccessor.read_(yData, AREA_SIZE, 0);
        cout << endl;
        cout << "[" << yData << "]";
        cout << endl;
        assert(strcmp(reinterpret_cast<char*>(yData), "test data") == 0);
    }

    {
        byte xData[AREA_SIZE];
        memcpy(xData,"test updt",AREA_SIZE);
        //            0123456789

        binaryAccessor.update_(xData, AREA_SIZE, 0);

        byte yData[AREA_SIZE];

        binaryAccessor.read_(yData, AREA_SIZE, 0);
        cout << yData << endl;
        assert(strcmp(reinterpret_cast<char*>(yData), "test updt") == 0);

        binaryAccessor.read_(yData, AREA_SIZE, AREA_SIZE);
        cout << yData << endl;
        assert(strcmp(reinterpret_cast<char*>(yData), "test data") == 0);
    }

    // Revert data to default.
    {
        byte xData[AREA_SIZE];
        memcpy(xData,"test data",AREA_SIZE);
        //            0123456789

        binaryAccessor.update_(xData, AREA_SIZE, 0);

        // update -> data in file

        byte yData[AREA_SIZE];
        binaryAccessor.read_(yData, AREA_SIZE, 0);
        cout << yData << endl;
        assert(strcmp(reinterpret_cast<char*>(yData), "test data") == 0);
    }

    descriptor data1{field("Name3", 10, String), field("Name4", 10, String)};

    data1.append({field("Name5z", 10, String)});

    auto testf = field("Name6z", 10, String);

    data1.append({testf});

    data1.push_back(field("Name", 10, String));
    data1.push_back(field("TLen", sizeof(uint), Uint));

    data1 | descriptor("Name2", 10, String) | descriptor("Control", Byte) | descriptor("Len3", Uint);

    cout << data1 << endl;

    descriptor data2 = descriptor("Name", 10, String) | descriptor("Len3", Uint) | descriptor("Control", Byte);

    cout << data2 << endl;

    cout << "Field Control is at " << data2.Position("Control") << endl;
    cout << "Field Control len is " << data2.Len("Control") << endl;
    cout << "Field Control type is " << data2.Type("Control") << endl;
    cout << "Field Control offset is " << data2.Offset("Control") << endl;

    //start cin test
    //https://stackoverflow.com/questions/14550187/how-to-put-data-in-cin-from-string
    streambuf *orig = cin.rdbuf();
    //Note: This mess is intended here in test

#define TEST_STRING "{ String Name3[10]\nString Name[10]\nUint Len String Name2 10 Byte Control Uint Len3 }"
    istringstream input(TEST_STRING);
    cin.rdbuf(input.rdbuf());
    // Test goes here

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

    cin.imbue(locale(cin.getloc(), unique_ptr<synsugar_is_space>(new synsugar_is_space).release()));
    descriptor data3;

    cout << "START SEND\n"
         << TEST_STRING << "\nEND SEND" << endl;
    cin >> data3;
    cout << "START RCV\n"
         << data3 << "\nEND RCV" << endl;

    // Revert to orginal cin
    std::cin.rdbuf(orig);
    // end cin test

    // ----------------------------------------------------------------------------

    auto dataDescriptor{descriptor("Name", 10, String) | descriptor("Control", Byte) | descriptor("TLen", Int)};

    //This structure is tricky
    //If not aligned - size is 15
    //If aligned - size is 16
    //Note that this should be packed and size should be 15

    union dataPayload
    {
        byte ptr[15];
        struct __attribute__((packed))
        {
            char Name[10];   //10
            uint8_t Control; //1
            int TLen;        //4
        };
    };

    dataPayload payload;

    // This assert will fail is structure is not packed.
    assert(dataDescriptor.getSize() == sizeof(dataPayload));

    genericBinaryFileAccessor binaryAccessor2("datafile-11");
    fileAccessor fAcc2(dataDescriptor,binaryAccessor2);

    cerr << binaryAccessor2.name_() << endl ;

    auto statusRemove = remove(binaryAccessor2.name_().c_str());

    memcpy(payload.Name, "test data", AREA_SIZE);
    payload.TLen = 0x66;
    payload.Control = 0x22;

    rawAccessor dataA(schema["datafile-11"], payload.ptr);

    cout << payload.TLen << endl;
    cout << dataA.cast<int>("TLen") << endl;

    assert(payload.TLen == dataA.cast<int>("TLen"));

    fAcc2.append(payload.ptr);
    fAcc2.append(payload.ptr);
    fAcc2.append(payload.ptr);

    dataPayload payload2;
    memcpy(payload2.Name, "xxxx xxxx",AREA_SIZE);
    payload2.TLen = 0x67;
    payload2.Control = 0x33;

    fAcc2.update(payload2.ptr, 1);

    dataPayload payload3;
    fAcc2.read(payload3.ptr, 1);
    rawAccessor dataB(schema["datafile-11"], payload3.ptr);

    cout << hex;
    cout << "Name:" << dataB.make_string("Name") << endl;
    cout << "Tlen:" << dataB.cast<int>("TLen") << endl;
    cout << "Control:" << (uint)dataB.cast<uint8_t>("Control") << endl;
    cout << dec;

    cout << "use '$xxd datafile-11' to check" << endl;

    return 0;
}
