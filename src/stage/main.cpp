#include <iostream>
#include <assert.h>
#include <string>
#include <cstring>
#include <locale>
#include <memory>
#include <sstream>

#include "dacc.h"
#include "desc.h"
#include "faccfs.h"

const uint AREA_SIZE = 10;

void check_debug()
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
}
///------------------------------
int main(int argc, char *argv[])
{
    check_debug();

    rdb::Descriptor voidDescriptor({rdb::Descriptor("Name", 10, rdb::String)});
    rdb::genericBinaryFileAccessor binaryAccessor("testfile");
    {

        std::byte xData[AREA_SIZE];
        std::memcpy(xData, "test data", AREA_SIZE);
        //             0123456789

        binaryAccessor.Write(xData, AREA_SIZE);
        binaryAccessor.Write(xData, AREA_SIZE); // Add one extra record

        assert(strcmp(reinterpret_cast<char *>(xData), "test data") == 0);

        std::byte yData[AREA_SIZE];
        binaryAccessor.Read(yData, AREA_SIZE, 0);

        std::cout << "[" << reinterpret_cast<char *>(yData) << "]";
        std::cout << "/";
        std::cout << "[" << yData << "]";
        std::cout << std::endl;
        assert(strcmp(reinterpret_cast<char *>(yData), "test data") == 0);
    }

    {
        std::byte xData[AREA_SIZE];
        std::memcpy(xData, "test updt", AREA_SIZE);
        //             0123456789

        binaryAccessor.Write(xData, AREA_SIZE, 0);

        std::byte yData[AREA_SIZE];

        binaryAccessor.Read(yData, AREA_SIZE, 0);

        std::cout << "[" << reinterpret_cast<char *>(yData) << "]";
        std::cout << "/";
        std::cout << "[" << yData << "]";
        std::cout << std::endl;
        assert(strcmp(reinterpret_cast<char *>(yData), "test updt") == 0);

        binaryAccessor.Read(yData, AREA_SIZE, AREA_SIZE);

        std::cout << "[" << reinterpret_cast<char *>(yData) << "]";
        std::cout << "/";
        std::cout << "[" << yData << "]";
        std::cout << std::endl;

        assert(strcmp(reinterpret_cast<char *>(yData), "test data") == 0);
    }

    // Revert data to default.
    {
        std::byte xData[AREA_SIZE];
        std::memcpy(xData, "test data", AREA_SIZE);
        //             0123456789

        binaryAccessor.Write(xData, AREA_SIZE, 0);

        // update -> data in file

        std::byte yData[AREA_SIZE];
        binaryAccessor.Read(yData, AREA_SIZE, 0);

        std::cout << "[" << reinterpret_cast<char *>(yData) << "]";
        std::cout << "/";
        std::cout << "[" << yData << "]";
        std::cout << std::endl;

        assert(strcmp(reinterpret_cast<char *>(yData), "test data") == 0);
    }

    rdb::Descriptor data1{rdb::field("Name3", 10, rdb::String), rdb::field("Name4", 10, rdb::String)};

    data1.Append({rdb::field("Name5z", 10, rdb::String)});
    data1.Append({rdb::field("Name6z", 10, rdb::String)});

    data1.push_back(rdb::field("Name", 10, rdb::String));
    data1.push_back(rdb::field("TLen", sizeof(uint), rdb::Uint));

    data1 | rdb::Descriptor("Name2", 10, rdb::String) | rdb::Descriptor("Control", rdb::Byte) | rdb::Descriptor("Len3", rdb::Uint);

    std::cout << data1 << std::endl;

    rdb::Descriptor data2 = rdb::Descriptor("Name", 10, rdb::String) | rdb::Descriptor("Len3", rdb::Uint) | rdb::Descriptor("Control", rdb::Byte);

    std::cout << data2 << std::endl;

    std::cout << "Field Control is at " << data2.Position("Control") << std::endl;
    std::cout << "Field Control len is " << data2.Len("Control") << std::endl;
    std::cout << "Field Control type is " << data2.Type("Control") << std::endl;
    std::cout << "Field Control offset is " << data2.Offset("Control") << std::endl;

    //start cin test
    //https://stackoverflow.com/questions/14550187/how-to-put-data-in-cin-from-string
    std::streambuf *orig = std::cin.rdbuf();
    //Note: This mess is intended here in test

    const char *test_string = "{ String Name3[10]\nString Name[10]\nUint Len String Name2 10 Byte Control Uint Len3 }";

    std::istringstream input(test_string);
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
    rdb::Descriptor data3;

    std::cout << "START SEND\n" ;
    std::cout << test_string ;
    std::cout << "\nEND SEND" ;
    std::cout << std::endl;

    std::cin >> data3;

    std::cout << "START RCV\n";
    std::cout << data3 ;
    std::cout << "\nEND RCV" ;
    std::cout << std::endl;

    // Revert to orginal cin
    std::cin.rdbuf(orig);
    // end cin test

    // ----------------------------------------------------------------------------

    auto dataDescriptor{rdb::Descriptor("Name", 10, rdb::String) | rdb::Descriptor("Control", rdb::Byte) | rdb::Descriptor("TLen", rdb::Int)};

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

    static_assert(sizeof(dataPayload)==15);

    dataPayload payload;

    // This assert will fail is structure is not packed.
    assert(dataDescriptor.GetSize() == sizeof(dataPayload));

    rdb::genericBinaryFileAccessor binaryAccessor2("datafile-11");
    rdb::DataAccessor dAcc2(dataDescriptor, binaryAccessor2);

    std::cerr << binaryAccessor2.FileName() << std::endl;

    auto statusRemove = remove(binaryAccessor2.FileName().c_str());

    std::memcpy(payload.Name, "test data", AREA_SIZE);
    payload.TLen = 0x66;
    payload.Control = 0x22;

    std::cout << payload.TLen << std::endl;

    std::cout << dAcc2.descriptor.Cast<int>("TLen", payload.ptr) << std::endl;

    assert(payload.TLen == dAcc2.descriptor.Cast<int>("TLen", payload.ptr));

    dAcc2.Put(payload.ptr);
    dAcc2.Put(payload.ptr);
    dAcc2.Put(payload.ptr);

    dataPayload payload2;
    std::memcpy(payload2.Name, "xxxx xxxx", AREA_SIZE);
    payload2.TLen = 0x67;
    payload2.Control = 0x33;

    dAcc2.Put(payload2.ptr, 1);

    dataPayload payload3;
    dAcc2.Get(payload3.ptr, 1);

    std::cout << std::hex;
    std::cout << "Name:" << dAcc2.descriptor.ToString("Name", payload3.ptr) << std::endl;
    std::cout << "Tlen:" << dAcc2.descriptor.Cast<int>("TLen", payload3.ptr) << std::endl;
    std::cout << "Control:" << (uint)dAcc2.descriptor.Cast<uint8_t>("Control", payload3.ptr) << std::endl;
    std::cout << std::dec;

    std::cout << "use '$xxd datafile-11' to check" << std::endl;

    return 0;
}
