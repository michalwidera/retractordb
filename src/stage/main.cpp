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

enum FieldType
{
    String,
    Uint,
    Byte,
    Int
};

typedef int fieldLen;
typedef string fieldName;
typedef tuple<fieldName, fieldLen, FieldType> field;
enum FieldColumn
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

map<string, FieldType> typeDictionary = {{"String", String}, {"Uint", Uint}, {"Byte", Byte}, {"Int", Int}};
map<FieldType, string> typeDictionaryR = makeReverse(typeDictionary);

FieldType getFieldType(string e)
{
    return typeDictionary[e];
}

string getFieldType(FieldType e)
{
    return typeDictionaryR[e];
}

int getFieldLenFromType(FieldType ft)
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

struct Descriptor : vector<field>
{

    map<string, int> fieldNames;

    Descriptor(initializer_list<field> l) : vector(l)
    {
    }

    Descriptor(fieldName n, fieldLen l, FieldType t)
    {
        push_back(field(n, l, t));
        updateNames();
    }

    Descriptor(fieldName n, FieldType t)
    {
        assert(t != String && "This method does not work for Stings");
        push_back(field(n, getFieldLenFromType(t), t));
        updateNames();
    }

    Descriptor()
    {
    }

    void append(initializer_list<field> l)
    {
        insert(end(), l.begin(), l.end());
    }

    auto &operator|(const Descriptor &rhs)
    {
        if (this != &rhs)
        {
            insert(end(), rhs.begin(), rhs.end());
        }
        else
        {
            //Descriptor b(*this);
            //insert(end(), b.begin(), b.end());
            assert(false && "can not merge same to same");
            // can't do safe: data | data
            // due one name policy
        }
        updateNames();
        return *this;
    }

    auto &operator=(const Descriptor &rhs)
    {
        clear();
        insert(end(), rhs.begin(), rhs.end());
        updateNames();
        return *this;
    }

    Descriptor(const Descriptor &init)
    {
        *this | init;
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
        assert(false && "did not find that record id Descriptor:{}");
        return -1;
    }

    int Len(const string name)
    {
        auto pos = Position(name);
        return get<len>((*this)[pos]);
    }

    int Offset(const string name)
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

    string Type(const string name)
    {
        auto pos = Position(name);
        return getFieldType(get<type>((*this)[pos]));
    }

    friend ostream &operator<<(ostream &os, const Descriptor &desc);
    friend istream &operator>>(istream &is, Descriptor &desc);
};

ostream &operator<<(ostream &os, const Descriptor &rhs)
{
    os << "{";
    for (auto const &r : rhs)
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

    if (rhs.fieldNames.size() == 0)
        os << "[dirty]";

    return os;
}

istream &operator>>(istream &is, Descriptor &rhs)
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

        FieldType ft = getFieldType(type);

        if (ft == String)
        {
            is >> len;
        }
        else
        {
            len = getFieldLenFromType(ft);
        }

        rhs | Descriptor(name, len, ft);

    } while (!is.eof());

    return is;
}

// https://en.cppreference.com/w/cpp/io/ios_base/openmode
// https://stackoverflow.com/questions/15063985/opening-a-binary-output-file-stream-without-truncation

struct BinaryFileAccessorInterface
{
    virtual void append(const byte *ptrData, const uint size) = 0;
    virtual void read(byte *ptrData, const uint size, const uint position) = 0;
    virtual void update(const byte *ptrData, const uint size, const uint position) = 0;
    virtual string fileName() = 0;

    virtual ~BinaryFileAccessorInterface(){};
    BinaryFileAccessorInterface(){};
};

struct genericBinaryFileAccessor : public BinaryFileAccessorInterface
{
    string fileNameStr;

    genericBinaryFileAccessor(string fileName) : fileNameStr(fileName){};

    void append(const byte *ptrData, const uint size) override
    {
        fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(fileName(), ios::out | ios::binary | ios::app | ios::ate);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.write(reinterpret_cast<const char*>(ptrData), size);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.close();
    };

    void read(byte *ptrData, const uint size, const uint position) override
    {
        fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(fileName(), ios::in | ios::binary);
        assert((myFile.rdstate() & ifstream::failbit) == 0);
        myFile.seekg(position);
        assert((myFile.rdstate() & ifstream::failbit) == 0);
        myFile.get(reinterpret_cast<char*>(ptrData), size);
        assert((myFile.rdstate() & ifstream::failbit) == 0);
        myFile.close();
    };

    void update(const byte *ptrData, const uint size, const uint position) override
    {
        fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        myFile.open(fileName(), ios::in | ios::out | ios::binary | ios::ate);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.seekp(position);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.write(reinterpret_cast<const char*>(ptrData), size);
        assert((myFile.rdstate() & ofstream::failbit) == 0);
        myFile.close();
    };

    string fileName() override {
        return fileNameStr;
    }
};

map<string, Descriptor> schema;

struct FileAccessor
{

    BinaryFileAccessorInterface *pAccessor;

    FileAccessor(const Descriptor desc, BinaryFileAccessorInterface &accessor)
        : pAccessor(&accessor)
    {
        Create(desc);
    };

    void Update(const byte *outBuffer, const uint offsetFromHead)
    {
        auto size = schema[pAccessor->fileName()].getSize();
        pAccessor->update(outBuffer, size, offsetFromHead * size);
    };

    void Get(byte *inBuffer, const uint offsetFromHead)
    {
        auto size = schema[pAccessor->fileName()].getSize();
        pAccessor->read(inBuffer, size, offsetFromHead * size);
    };

    void Put(const byte *outBuffer)
    {
        auto size = schema[pAccessor->fileName()].getSize();
        pAccessor->append(outBuffer, size);
    };

    void Create(const Descriptor desc)
    {
        schema[pAccessor->fileName()] = desc;
        fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        string fileDesc(pAccessor->fileName());
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

struct SchemaInterpreter
{

    Descriptor *pDesc;
    byte *ptr;
    SchemaInterpreter(Descriptor &desc, byte *ptr) : pDesc(&desc),
                                               ptr(ptr)
    {
    }

    SchemaInterpreter() = delete;

    // Return a string that contains the copy of the referenced data.
    std::string ToString(const string fieldName) const
    { 
        return std::string(reinterpret_cast<char*>(ptr + pDesc->Offset(fieldName)), pDesc->Len(fieldName)); 
    }

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

    Descriptor voidDescriptor({Descriptor("Name", 10, String)});
    genericBinaryFileAccessor binaryAccessor("testfile");
    FileAccessor fAcc(voidDescriptor, binaryAccessor);

    {

        byte xData[AREA_SIZE] ;
        memcpy(xData,"test data",AREA_SIZE);
        //            0123456789

        binaryAccessor.append(xData, AREA_SIZE);
        binaryAccessor.append(xData, AREA_SIZE); // Add one extra record

        assert(strcmp(reinterpret_cast<char*>(xData), "test data") == 0);

        byte yData[AREA_SIZE];
        binaryAccessor.read(yData, AREA_SIZE, 0);

        cout << "[" << reinterpret_cast<char*>(yData) << "]";
        cout << "/";
        cout << "[" << yData << "]" ;
        cout << endl;
        assert(strcmp(reinterpret_cast<char*>(yData), "test data") == 0);
    }

    {
        byte xData[AREA_SIZE];
        memcpy(xData,"test updt",AREA_SIZE);
        //            0123456789

        binaryAccessor.update(xData, AREA_SIZE, 0);

        byte yData[AREA_SIZE];

        binaryAccessor.read(yData, AREA_SIZE, 0);

        cout << "[" << reinterpret_cast<char*>(yData) << "]";
        cout << "/";
        cout << "[" << yData << "]" ;
        cout << endl;
        assert(strcmp(reinterpret_cast<char*>(yData), "test updt") == 0);

        binaryAccessor.read(yData, AREA_SIZE, AREA_SIZE);

        cout << "[" << reinterpret_cast<char*>(yData) << "]";
        cout << "/";
        cout << "[" << yData << "]" ;
        cout << endl;
    
        assert(strcmp(reinterpret_cast<char*>(yData), "test data") == 0);
    }

    // Revert data to default.
    {
        byte xData[AREA_SIZE];
        memcpy(xData,"test data",AREA_SIZE);
        //            0123456789

        binaryAccessor.update(xData, AREA_SIZE, 0);

        // update -> data in file

        byte yData[AREA_SIZE];
        binaryAccessor.read(yData, AREA_SIZE, 0);

        cout << "[" << reinterpret_cast<char*>(yData) << "]";
        cout << "/";
        cout << "[" << yData << "]" ;
        cout << endl;

        assert(strcmp(reinterpret_cast<char*>(yData), "test data") == 0);
    }

    Descriptor data1{field("Name3", 10, String), field("Name4", 10, String)};

    data1.append({field("Name5z", 10, String)});

    auto testf = field("Name6z", 10, String);

    data1.append({testf});

    data1.push_back(field("Name", 10, String));
    data1.push_back(field("TLen", sizeof(uint), Uint));

    data1 | Descriptor("Name2", 10, String) | Descriptor("Control", Byte) | Descriptor("Len3", Uint);

    cout << data1 << endl;

    Descriptor data2 = Descriptor("Name", 10, String) | Descriptor("Len3", Uint) | Descriptor("Control", Byte);

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
    Descriptor data3;

    cout << "START SEND\n"
         << TEST_STRING << "\nEND SEND" << endl;
    cin >> data3;
    cout << "START RCV\n"
         << data3 << "\nEND RCV" << endl;

    // Revert to orginal cin
    std::cin.rdbuf(orig);
    // end cin test

    // ----------------------------------------------------------------------------

    auto dataDescriptor{Descriptor("Name", 10, String) | Descriptor("Control", Byte) | Descriptor("TLen", Int)};

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
    FileAccessor fAcc2(dataDescriptor,binaryAccessor2);

    cerr << binaryAccessor2.fileName() << endl ;

    auto statusRemove = remove(binaryAccessor2.fileName().c_str());

    memcpy(payload.Name, "test data", AREA_SIZE);
    payload.TLen = 0x66;
    payload.Control = 0x22;

    SchemaInterpreter dataA(schema["datafile-11"], payload.ptr);

    cout << payload.TLen << endl;
    cout << dataA.cast<int>("TLen") << endl;

    assert(payload.TLen == dataA.cast<int>("TLen"));

    fAcc2.Put(payload.ptr);
    fAcc2.Put(payload.ptr);
    fAcc2.Put(payload.ptr);

    dataPayload payload2;
    memcpy(payload2.Name, "xxxx xxxx",AREA_SIZE);
    payload2.TLen = 0x67;
    payload2.Control = 0x33;

    fAcc2.Update(payload2.ptr, 1);

    dataPayload payload3;
    fAcc2.Get(payload3.ptr, 1);
    SchemaInterpreter dataB(schema["datafile-11"], payload3.ptr);

    cout << hex;
    cout << "Name:" << dataB.ToString("Name") << endl;
    cout << "Tlen:" << dataB.cast<int>("TLen") << endl;
    cout << "Control:" << (uint)dataB.cast<uint8_t>("Control") << endl;
    cout << dec;

    cout << "use '$xxd datafile-11' to check" << endl;

    return 0;
}
