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
#include "desc.h"

//https://courses.cs.vt.edu/~cs2604/fall02/binio.html
//https://stackoverflow.com/questions/1658476/c-fopen-vs-open

// Turn off buffering (this must apear before open)
// http://gcc.gnu.org/onlinedocs/libstdc++/manual/streambufs.html#io.streambuf.buffering

namespace rdb
{

    // ------------------------------------------------------------------------
    // Interface

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

        std::cout << payload.TLen << std::endl;
        std::cout << schema["datafile-11"].Cast<int>("TLen", payload.ptr) << std::endl;

        assert(payload.TLen == schema["datafile-11"].Cast<int>("TLen", payload.ptr));

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

        std::cout << std::hex;
        std::cout << "Name:" << schema["datafile-11"].ToString("Name", payload3.ptr) << std::endl;
        std::cout << "Tlen:" << schema["datafile-11"].Cast<int>("TLen", payload3.ptr) << std::endl;
        std::cout << "Control:" << (uint)schema["datafile-11"].Cast<uint8_t>("Control", payload3.ptr) << std::endl;
        std::cout << std::dec;

        std::cout << "use '$xxd datafile-11' to check" << std::endl;

        return 0;
    }

} // namespace rdb

int main(int argc, char *argv[])
{
    return rdb::main_process();
}