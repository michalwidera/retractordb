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

#include "facc.h"

//https://courses.cs.vt.edu/~cs2604/fall02/binio.html
//https://stackoverflow.com/questions/1658476/c-fopen-vs-open

// Turn off buffering (this must apear before open)
// http://gcc.gnu.org/onlinedocs/libstdc++/manual/streambufs.html#io.streambuf.buffering

namespace rdb
{

    enum FieldType
    {
        String,
        Uint,
        Byte,
        Int
    };

    typedef int fieldLen;
    typedef std::string fieldName;
    typedef std::tuple<fieldName, fieldLen, FieldType> field;
    enum FieldColumn
    {
        name = 0,
        len = 1,
        type = 2
    };

    // https://developers.google.com/protocol-buffers/docs/overview#scalar
    template <typename L, typename R>
    std::map<R, L>
    makeReverse(std::map<L, R> list)
    {
        std::map<R, L> retVal;
        for (const auto &[key, value] : list)
            retVal[value] = key;
        return retVal;
    }

    std::map<std::string, FieldType> typeDictionary = {{"String", String}, {"Uint", Uint}, {"Byte", Byte}, {"Int", Int}};
    std::map<FieldType, std::string> typeDictionaryR = makeReverse(typeDictionary);

    FieldType getFieldType(std::string e)
    {
        return typeDictionary[e];
    }

    std::string getFieldType(FieldType e)
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

    struct Descriptor : std::vector<field>
    {

        std::map<std::string, int> fieldNames;

        Descriptor(std::initializer_list<field> l) : std::vector<field>(l)
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

        void append(std::initializer_list<field> l)
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
                size += std::get<len>(i);
            }
            return size;
        }

        bool updateNames()
        {
            uint position = 0;
            fieldNames.clear();
            for (auto const i : *this)
            {
                if (fieldNames.find(std::get<fieldName>(i)) == fieldNames.end())
                {
                    fieldNames[std::get<fieldName>(i)] = position;
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

        int Position(std::string name)
        {
            if (fieldNames.find(name) != fieldNames.end())
                return fieldNames[name];
            std::cerr << "[" << name << "]" << std::endl;
            assert(false && "did not find that record id Descriptor:{}");
            return -1;
        }

        int Len(const std::string name)
        {
            auto pos = Position(name);
            return std::get<len>((*this)[pos]);
        }

        int Offset(const std::string name)
        {
            auto offset = 0;
            for (auto const i : *this)
            {
                if (name == std::get<fieldName>(i))
                    return offset;
                offset += std::get<len>(i);
            }
            assert(false && "record not found with that name");
            return -1;
        }

        std::string Type(const std::string name)
        {
            auto pos = Position(name);
            return getFieldType(std::get<type>((*this)[pos]));
        }

        friend std::ostream &operator<<(std::ostream &os, const Descriptor &rhs);
        friend std::istream &operator>>(std::istream &is, Descriptor &rhs);
    };

    std::ostream &operator<<(std::ostream &os, const Descriptor &rhs)
    {
        os << "{";
        for (auto const &r : rhs)
        {
            os << "\t" << getFieldType(std::get<type>(r))
               << " " << std::get<name>(r);
            if (std::get<type>(r) == String)
            {
                os << "[" << std::to_string(std::get<len>(r)) << "]";
            };
            os << std::endl;
        }
        os << "}";

        if (rhs.fieldNames.size() == 0)
            os << "[dirty]";

        return os;
    }

    std::istream &operator>>(std::istream &is, Descriptor &rhs)
    {
        do
        {
            std::string type;
            std::string name;
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

    // ------------------------------------------------------------------------
    // Interface

    struct SchemaInterpreter
    {

        Descriptor *pDesc;
        std::byte *ptr;
        SchemaInterpreter(Descriptor &desc, std::byte *ptr) : pDesc(&desc),
                                                              ptr(ptr)
        {
        }

        SchemaInterpreter() = delete;

        // Return a string that contains the copy of the referenced data.
        std::string ToString(const std::string fieldName) const
        {
            return std::string(reinterpret_cast<char *>(ptr + pDesc->Offset(fieldName)), pDesc->Len(fieldName));
        }

        template <typename T>
        auto Cast(const std::string fieldName)
        {
            return *(reinterpret_cast<T *>(ptr + pDesc->Offset(fieldName)));
        };
    };

    ///------------------------------

    struct FileAccessor
    {
        BinaryFileAccessorInterface *pAccessor;

        FileAccessor(const Descriptor desc, BinaryFileAccessorInterface &accessor);
        void Update(const std::byte *outBuffer, const uint offsetFromHead);
        void Get(std::byte *inBuffer, const uint offsetFromHead);
        void Put(const std::byte *outBuffer);
        void Create(const Descriptor desc);
    };

    std::map<std::string, Descriptor> schema;

    FileAccessor::FileAccessor(const Descriptor desc, BinaryFileAccessorInterface &accessor)
        : pAccessor(&accessor)
    {
        Create(desc);
    };

    void FileAccessor::Update(const std::byte *outBuffer, const uint offsetFromHead)
    {
        auto size = schema[pAccessor->fileName()].getSize();
        pAccessor->update(outBuffer, size, offsetFromHead * size);
    };

    void FileAccessor::Get(std::byte *inBuffer, const uint offsetFromHead)
    {
        auto size = schema[pAccessor->fileName()].getSize();
        pAccessor->read(inBuffer, size, offsetFromHead * size);
    };

    void FileAccessor::Put(const std::byte *outBuffer)
    {
        auto size = schema[pAccessor->fileName()].getSize();
        pAccessor->append(outBuffer, size);
    };

    void FileAccessor::Create(const Descriptor desc)
    {
        schema[pAccessor->fileName()] = desc;
        std::fstream myFile;

        myFile.rdbuf()->pubsetbuf(0, 0);

        std::string fileDesc(pAccessor->fileName());
        fileDesc.append(".desc");

        myFile.open(fileDesc, std::ios::out);
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile << desc;
        assert((myFile.rdstate() & std::ofstream::failbit) == 0);
        myFile.close();
    };
    ///------------------------------
    int main_process()
    {

        // Diagnostic code

#ifdef NDEBUG
        std::cerr << std::endl;
        std::cerr << "\x1B[31m";
        std::cerr << "Release Code form - Failure.\n";
        std::cerr << "\033[0m";

        assert(false); // Note this assert will have no effect!

        std::cerr << std::endl;
        std::cerr << "         This is Staging/Testing code.\n";
        std::cerr << "         You compiled it as RELEASE - no assert will affect here!\n";
        std::cerr << "         Rebuild code asap with follwoing command:\n";
        std::cerr << "         conan install .. -s build_type=Debug && conan build ..\n";
        std::cerr << "         make install && xstage || echo Fail\n";

        exit(1);

        // If meet: ndefined reference to `boost::program_options::options_description::options_description
        // There were changes to the <string> in the C++11 standard which may conflict with versions
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
                std::cout << "\x1B[32mDebug Code Form - Ok.\n\033[0m";
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

            std::byte xData[AREA_SIZE];
            memcpy(xData, "test data", AREA_SIZE);
            //            0123456789

            binaryAccessor.append(xData, AREA_SIZE);
            binaryAccessor.append(xData, AREA_SIZE); // Add one extra record

            assert(strcmp(reinterpret_cast<char *>(xData), "test data") == 0);

            std::byte yData[AREA_SIZE];
            binaryAccessor.read(yData, AREA_SIZE, 0);

            std::cout << "[" << reinterpret_cast<char *>(yData) << "]";
            std::cout << "/";
            std::cout << "[" << yData << "]";
            std::cout << std::endl;
            assert(strcmp(reinterpret_cast<char *>(yData), "test data") == 0);
        }

        {
            std::byte xData[AREA_SIZE];
            memcpy(xData, "test updt", AREA_SIZE);
            //            0123456789

            binaryAccessor.update(xData, AREA_SIZE, 0);

            std::byte yData[AREA_SIZE];

            binaryAccessor.read(yData, AREA_SIZE, 0);

            std::cout << "[" << reinterpret_cast<char *>(yData) << "]";
            std::cout << "/";
            std::cout << "[" << yData << "]";
            std::cout << std::endl;
            assert(strcmp(reinterpret_cast<char *>(yData), "test updt") == 0);

            binaryAccessor.read(yData, AREA_SIZE, AREA_SIZE);

            std::cout << "[" << reinterpret_cast<char *>(yData) << "]";
            std::cout << "/";
            std::cout << "[" << yData << "]";
            std::cout << std::endl;

            assert(strcmp(reinterpret_cast<char *>(yData), "test data") == 0);
        }

        // Revert data to default.
        {
            std::byte xData[AREA_SIZE];
            memcpy(xData, "test data", AREA_SIZE);
            //            0123456789

            binaryAccessor.update(xData, AREA_SIZE, 0);

            // update -> data in file

            std::byte yData[AREA_SIZE];
            binaryAccessor.read(yData, AREA_SIZE, 0);

            std::cout << "[" << reinterpret_cast<char *>(yData) << "]";
            std::cout << "/";
            std::cout << "[" << yData << "]";
            std::cout << std::endl;

            assert(strcmp(reinterpret_cast<char *>(yData), "test data") == 0);
        }

        Descriptor data1{field("Name3", 10, String), field("Name4", 10, String)};

        data1.append({field("Name5z", 10, String)});

        auto testf = field("Name6z", 10, String);

        data1.append({testf});

        data1.push_back(field("Name", 10, String));
        data1.push_back(field("TLen", sizeof(uint), Uint));

        data1 | Descriptor("Name2", 10, String) | Descriptor("Control", Byte) | Descriptor("Len3", Uint);

        std::cout << data1 << std::endl;

        Descriptor data2 = Descriptor("Name", 10, String) | Descriptor("Len3", Uint) | Descriptor("Control", Byte);

        std::cout << data2 << std::endl;

        std::cout << "Field Control is at " << data2.Position("Control") << std::endl;
        std::cout << "Field Control len is " << data2.Len("Control") << std::endl;
        std::cout << "Field Control type is " << data2.Type("Control") << std::endl;
        std::cout << "Field Control offset is " << data2.Offset("Control") << std::endl;

        //start cin test
        //https://stackoverflow.com/questions/14550187/how-to-put-data-in-cin-from-string
        std::streambuf *orig = std::cin.rdbuf();
        //Note: This mess is intended here in test

#define TEST_STRING "{ String Name3[10]\nString Name[10]\nUint Len String Name2 10 Byte Control Uint Len3 }"
        std::istringstream input(TEST_STRING);
        std::cin.rdbuf(input.rdbuf());
        // Test goes here

        // Look here to explain: https://stackoverflow.com/questions/7302996/changing-the-delimiter-for-cin-c
        struct synsugar_is_space : std::ctype<char>
        {
            synsugar_is_space() : ctype<char>(get_table()) {}
            static mask const *get_table()
            {
                static mask rc[table_size];
                rc['['] = rc[']'] = rc['{'] = rc['}'] = rc[' '] = rc['\n'] = std::ctype_base::space;
                return &rc[0];
            }
        };

        std::cin.imbue(std::locale(std::cin.getloc(), std::unique_ptr<synsugar_is_space>(new synsugar_is_space).release()));
        Descriptor data3;

        std::cout << "START SEND\n"
                  << TEST_STRING << "\nEND SEND" << std::endl;
        std::cin >> data3;
        std::cout << "START RCV\n"
                  << data3 << "\nEND RCV" << std::endl;

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
            std::byte ptr[15];
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
        FileAccessor fAcc2(dataDescriptor, binaryAccessor2);

        std::cerr << binaryAccessor2.fileName() << std::endl;

        auto statusRemove = remove(binaryAccessor2.fileName().c_str());

        memcpy(payload.Name, "test data", AREA_SIZE);
        payload.TLen = 0x66;
        payload.Control = 0x22;

        SchemaInterpreter dataA(schema["datafile-11"], payload.ptr);

        std::cout << payload.TLen << std::endl;
        std::cout << dataA.Cast<int>("TLen") << std::endl;

        assert(payload.TLen == dataA.Cast<int>("TLen"));

        fAcc2.Put(payload.ptr);
        fAcc2.Put(payload.ptr);
        fAcc2.Put(payload.ptr);

        dataPayload payload2;
        memcpy(payload2.Name, "xxxx xxxx", AREA_SIZE);
        payload2.TLen = 0x67;
        payload2.Control = 0x33;

        fAcc2.Update(payload2.ptr, 1);

        dataPayload payload3;
        fAcc2.Get(payload3.ptr, 1);
        SchemaInterpreter dataB(schema["datafile-11"], payload3.ptr);

        std::cout << std::hex;
        std::cout << "Name:" << dataB.ToString("Name") << std::endl;
        std::cout << "Tlen:" << dataB.Cast<int>("TLen") << std::endl;
        std::cout << "Control:" << (uint)dataB.Cast<uint8_t>("Control") << std::endl;
        std::cout << std::dec;

        std::cout << "use '$xxd datafile-11' to check" << std::endl;

        return 0;
    }

} // namespace rdb

int main(int argc, char *argv[])
{
    return rdb::main_process();
}