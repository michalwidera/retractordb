#include <iostream>
#include <fstream>
#include <assert.h>
#include <string>
#include <cstring>
#include <locale>
#include <memory>
#include <sstream>
#include <algorithm>

#include "dsacc.h"
#include "desc.h"
#include "faccfs.h"
#include "faccposix.h"
#include "payloadacc.h"

#include "fainterface.h"

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

template <typename T, typename K>
void test_1()
{
    K binaryAccessor1("testfile-fstream");
    {
        T xData[AREA_SIZE];
        std::memcpy(xData, "test data", AREA_SIZE);

        binaryAccessor1.Write(xData, AREA_SIZE);

        assert(strcmp(reinterpret_cast<char *>(xData), "test data") == 0);

        T yData[AREA_SIZE];
        binaryAccessor1.Read(yData, AREA_SIZE, 0);

        assert(strcmp(reinterpret_cast<char *>(yData), "test data") == 0);
    }
    auto statusRemove1 = remove(binaryAccessor1.FileName().c_str());
}

template <typename T, typename K>
void test_2()
{
    K dataStore("testfile-fstream");
    {

        T xData[AREA_SIZE];
        std::memcpy(xData, "test data", AREA_SIZE);

        dataStore.Write(xData, AREA_SIZE);
        dataStore.Write(xData, AREA_SIZE); // Add one extra record

        assert(strcmp(reinterpret_cast<char *>(xData), "test data") == 0);

        T yData[AREA_SIZE];
        dataStore.Read(yData, AREA_SIZE, 0);

        assert(strcmp(reinterpret_cast<char *>(yData), "test data") == 0);
    }
    auto statusRemove1 = remove(dataStore.FileName().c_str());
}

template <typename T, typename K>
void test_3()
{
    K dataStore("testfile-fstream");
    {
        T xData[AREA_SIZE];

        std::memcpy(xData, "test aaaa", AREA_SIZE);
        dataStore.Write(xData, AREA_SIZE);

        std::memcpy(xData, "test bbbb", AREA_SIZE);
        dataStore.Write(xData, AREA_SIZE);

        std::memcpy(xData, "test cccc", AREA_SIZE);
        dataStore.Write(xData, AREA_SIZE);

        std::memcpy(xData, "test xxxx", AREA_SIZE);
        dataStore.Write(xData, AREA_SIZE, AREA_SIZE); // <- Update

        std::memcpy(xData, "test dddd", AREA_SIZE);
        dataStore.Write(xData, AREA_SIZE);

        T yData[AREA_SIZE];

        dataStore.Read(yData, AREA_SIZE, 0);

        assert(strcmp(reinterpret_cast<char *>(yData), "test aaaa") == 0);

        dataStore.Read(yData, AREA_SIZE, AREA_SIZE);

        assert(strcmp(reinterpret_cast<char *>(yData), "test xxxx") == 0);
    }
    auto statusRemove1 = remove(dataStore.FileName().c_str());
}

void test_descriptor()
{
    rdb::Descriptor data1{rdb::field("Name3", 10, rdb::String), rdb::field("Name4", 10, rdb::String)};

    data1.Append({rdb::field("Name5z", 10, rdb::String)});
    data1.Append({rdb::field("Name6z", 10, rdb::String)});

    data1.push_back(rdb::field("Name", 10, rdb::String));
    data1.push_back(rdb::field("TLen", sizeof(uint), rdb::Uint));

    data1 | rdb::Descriptor("Name2", 10, rdb::String) | rdb::Descriptor("Control", rdb::Byte) | rdb::Descriptor("Len3", rdb::Uint);
    {
        std::stringstream coutstring;
        coutstring << data1;
        char test[] = "{\tString Name3[10]\n\tString Name4[10]\n\tString Name5z[10]\n\tString Name6z[10]\n\tString Name[10]\n\tUint TLen\n\tString Name2[10]\n\tByte Control\n\tUint Len3\n}";
        assert(strcmp(coutstring.str().c_str(), test) == 0);
    }

    rdb::Descriptor data2 = rdb::Descriptor("Name", 10, rdb::String) | rdb::Descriptor("Len3", rdb::Uint) | rdb::Descriptor("Control", rdb::Byte);
    {
        std::stringstream coutstring;
        char test[] = "{\tString Name[10]\n\tUint Len3\n\tByte Control\n}";
        coutstring << data2;
        assert(strcmp(coutstring.str().c_str(), test) == 0);
    }

    assert(data2.Position("Control") == 2);
    assert(data2.Len("Control") == 1);
    assert(strcmp(data2.Type("Control").c_str(), "Byte") == 0);
    assert(data2.Offset("Control") == 14);
}

void test_descriptor_read()
{
    //start cin test
    //https://stackoverflow.com/questions/14550187/how-to-put-data-in-cin-from-string
    std::streambuf *orig = std::cin.rdbuf();
    //Note: This mess is intended here in test

    const char *test_string = "{ String Name3[10]\nString Name[10]\nUint Len String Name2 10 Byte Control Uint Len3 }";

    // Test goes here

    rdb::Descriptor data3;

    std::istringstream input(test_string);
    std::cin.rdbuf(input.rdbuf());
    std::cin >> data3;
    // Revert to orginal cin
    std::cin.rdbuf(orig);

    {
        std::stringstream coutstring;
        const char *test = "{\tString Name3[10]\n\tString Name[10]\n\tUint Len\n\tString Name2[10]\n\tByte Control\n\tUint Len3\n}";
        coutstring << data3;
        assert(strcmp(coutstring.str().c_str(), test) == 0);
    }

    // end cin test
}

void test_storage()
{
    rdb::genericBinaryFileAccessor<std::byte> dataStore("datafile-fstream2");

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

    static_assert(sizeof(dataPayload) == 15);

    auto dataDescriptor{rdb::Descriptor("Name", 10, rdb::String) | rdb::Descriptor("Control", rdb::Byte) | rdb::Descriptor("TLen", rdb::Int)};

    // This assert will fail is structure is not packed.
    assert(dataDescriptor.GetSize() == sizeof(dataPayload));

    dataPayload payload1;

    rdb::DataStorageAccessor dAcc2(dataDescriptor, dataStore);

    assert(strcmp(dataStore.FileName().c_str(), "datafile-fstream2") == 0);

    std::memcpy(payload1.Name, "test data", AREA_SIZE);
    payload1.TLen = 0x66;
    payload1.Control = 0x22;

    assert(payload1.TLen == dAcc2.descriptor.Cast<int>("TLen", payload1.ptr));

    dAcc2.Put(payload1.ptr);
    dAcc2.Put(payload1.ptr);
    dAcc2.Put(payload1.ptr);

    dataPayload payload2;
    std::memcpy(payload2.Name, "xxxx xxxx", AREA_SIZE);
    payload2.TLen = 0x67;
    payload2.Control = 0x33;

    dAcc2.Put(payload2.ptr, 1);

    dataPayload payload3;
    dAcc2.Get(payload3.ptr, 1);

    {
        std::stringstream coutstring;
        coutstring << dAcc2.descriptor.ToString("Name", payload3.ptr);
        assert(strcmp(coutstring.str().c_str(), "xxxx xxxx") == 0);
    }

    {
        std::stringstream coutstring;
        coutstring << std::hex;
        coutstring << dAcc2.descriptor.Cast<int>("TLen", payload3.ptr);
        coutstring << std::dec;
        assert(strcmp(coutstring.str().c_str(), "67") == 0);
    }

    {
        std::stringstream coutstring;
        coutstring << std::hex;
        coutstring << (uint)dAcc2.descriptor.Cast<uint8_t>("Control", payload3.ptr);
        coutstring << std::dec;
        assert(strcmp(coutstring.str().c_str(), "33") == 0);
    }

    auto statusRemove = remove(dataStore.FileName().c_str());
}

int main(int argc, char *argv[])
{
    check_debug();

    test_1<std::byte, rdb::genericBinaryFileAccessor<std::byte>>();
    test_2<std::byte, rdb::genericBinaryFileAccessor<std::byte>>();
    test_3<std::byte, rdb::genericBinaryFileAccessor<std::byte>>();

    test_1<char, rdb::genericBinaryFileAccessor<char>>();
    test_2<char, rdb::genericBinaryFileAccessor<char>>();
    test_3<char, rdb::genericBinaryFileAccessor<char>>();

    test_1<std::byte, rdb::posixBinaryFileAccessor<std::byte>>();
    test_2<std::byte, rdb::posixBinaryFileAccessor<std::byte>>();
    test_3<std::byte, rdb::posixBinaryFileAccessor<std::byte>>();

    test_1<char, rdb::posixBinaryFileAccessor<char>>();
    test_2<char, rdb::posixBinaryFileAccessor<char>>();
    test_3<char, rdb::posixBinaryFileAccessor<char>>();

    // -------------------------- Desscriptor Testing

    test_descriptor();

    test_descriptor_read();

    // -------------------------- Storage Testing

    test_storage();

    std::unique_ptr<rdb::genericBinaryFileAccessor<std::byte>> uPtr_store;
    std::unique_ptr<rdb::DataStorageAccessor<std::byte>> uPtr_dacc;
    std::unique_ptr<std::byte[]> uPtr_payload;
    rdb::Descriptor desc;
    std::string file;

    std::string cmd;
    do
    {
        std::cout << ".";
        std::cin >> cmd;
        if (cmd == "exit" || cmd == "quit" || cmd == "q")
        {
            break;
        }
        else if (cmd == "desc")
        {
            std::cout << desc << std::endl;
        }
        else if (cmd == "open")
        {
            std::cin >> file;

            uPtr_store.reset(new rdb::genericBinaryFileAccessor<std::byte>(file.c_str()));

            std::string descriptionFileName(file);
            descriptionFileName.append(".desc");

            std::ifstream streamDescriptorFile(descriptionFileName.c_str());
            std::stringstream buffer;
            buffer << streamDescriptorFile.rdbuf();

            desc.clear();
            buffer >> desc;

            uPtr_dacc.reset(new rdb::DataStorageAccessor<std::byte>(desc, *(uPtr_store.get())));

            uPtr_payload.reset(new std::byte[desc.GetSize()]);

            memset(uPtr_payload.get(), 0, desc.GetSize());

            std::cout << "ok" << std::endl;
        }
        else if (cmd == "create")
        {
            std::cin >> file;

            uPtr_store.reset(new rdb::genericBinaryFileAccessor<std::byte>(file.c_str()));

            std::string sschema;
            std::string object;

            do
            {
                std::cin >> object;
                sschema.append(object);
                sschema.append(" ");
            } while (object.find("}") == std::string::npos);

            std::stringstream scheamStringStream(sschema);

            desc.clear();
            scheamStringStream >> desc;

            uPtr_dacc.reset(new rdb::DataStorageAccessor<std::byte>(desc, *(uPtr_store.get())));

            uPtr_payload.reset(new std::byte[desc.GetSize()]);
            memset(uPtr_payload.get(), 0, desc.GetSize());

            std::cout << "ok\n";
        }
        else if (cmd == "read")
        {
            int record;
            std::cin >> record;

            if (uPtr_store && uPtr_dacc)
            {
                uPtr_dacc->Get(uPtr_payload.get(), record);

                std::cout << "ok\n";
            }
            else
            {
                std::cout << "no connection\n";
            }
        }
        else if (cmd == "set")
        {
            if (uPtr_dacc && desc.size() > 0)
            {
                rdb::payLoadAccessor payload(desc, uPtr_payload.get());

                std::cin >> payload;
            }
            else
            {
                std::cout << "no connection\n";
            }
        }
        else if (cmd == "print")
        {
            if (uPtr_dacc && desc.size() > 0)
            {
                rdb::payLoadAccessor payload(desc, uPtr_payload.get());

                std::cout << payload << std::endl;
            }
            else
            {
                std::cout << "no connection\n";
            }
        }
        else if (cmd == "write")
        {
            if (uPtr_store && uPtr_dacc)
            {
                size_t record;
                std::cin >> record;
                uPtr_dacc->Put(uPtr_payload.get(), record);

                std::cout << "ok\n";
            }
            else
            {
                std::cout << "not connection\n";
            }
        }
        else if (cmd == "append")
        {
            if (uPtr_store && uPtr_dacc)
            {
                uPtr_dacc->Put(uPtr_payload.get());

                std::cout << "ok\n";
            }
            else
            {
                std::cout << "not connection\n";
            }
        }
        else if (cmd == "size")
        {
            std::ifstream in(file.c_str(), std::ifstream::ate | std::ifstream::binary);
            std::cout << int(in.tellg() / desc.GetSize()) << " Record(s)\n";
        }
        else if (cmd == "help")
        {
            std::cout << "exit|quit|q \t\t exit program\n";
            std::cout << "open [file] \t\t open database - create connection\n";
            std::cout << "create [file][schema] \t create database with schema\n";
            std::cout << "desc \t\t\t show schema\n";
            std::cout << "read [n] \t\t read record from database into payload\n";
            std::cout << "write [n] \t\t update record in database from payload\n";
            std::cout << "append \t\t\t append payload record to database\n";
            std::cout << "set [name][value] \t set payload value\n";
            std::cout << "print \t\t\t show payload\n";
            std::cout << "size \t\t\t show database size in records\n";
        }
        else
        {
            std::cout << "?" << std::endl;
        }
    } while (true);

    // use '$xxd datafile-11' to check
    return 0;
}
