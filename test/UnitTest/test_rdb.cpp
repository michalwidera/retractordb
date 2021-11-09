#include <gtest/gtest.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <locale>
#include <memory>
#include <sstream>
#include <algorithm>
#include <iomanip>

#include "rdb/dsacc.h"
#include "rdb/desc.h"
#include "rdb/faccfs.h"
#include "rdb/faccposix.h"
#include "rdb/payloadacc.h"

#include "rdb/fainterface.h"

const uint AREA_SIZE = 10;

template <typename T, typename K>
bool test_1()
{
    K binaryAccessor1("testfile-fstream");
    {
        T xData[AREA_SIZE];
        std::memcpy(xData, "test data", AREA_SIZE);

        binaryAccessor1.Write(xData, AREA_SIZE);

        if (strcmp(reinterpret_cast<char *>(xData), "test data") != 0)
            return false;

        T yData[AREA_SIZE];
        binaryAccessor1.Read(yData, AREA_SIZE, 0);

        if (strcmp(reinterpret_cast<char *>(yData), "test data") != 0)
            return false;
    }
    auto statusRemove1 = remove(binaryAccessor1.FileName().c_str());

    return true;
}

template <typename T, typename K>
bool test_2()
{
    K dataStore("testfile-fstream");
    {

        T xData[AREA_SIZE];
        std::memcpy(xData, "test data", AREA_SIZE);

        dataStore.Write(xData, AREA_SIZE);
        dataStore.Write(xData, AREA_SIZE); // Add one extra record

        if (strcmp(reinterpret_cast<char *>(xData), "test data") != 0)
            return false;

        T yData[AREA_SIZE];
        dataStore.Read(yData, AREA_SIZE, 0);

        if (strcmp(reinterpret_cast<char *>(yData), "test data") != 0)
            return false;
    }
    auto statusRemove1 = remove(dataStore.FileName().c_str());

    return true;
}

template <typename T, typename K>
bool test_3()
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

        if(strcmp(reinterpret_cast<char *>(yData), "test aaaa") != 0)
            return false;

        dataStore.Read(yData, AREA_SIZE, AREA_SIZE);

        if (strcmp(reinterpret_cast<char *>(yData), "test xxxx") != 0)
            return false;
    }
    auto statusRemove1 = remove(dataStore.FileName().c_str());

    return true;
}

bool test_descriptor()
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
        if (strcmp(coutstring.str().c_str(), test) != 0)
            return false;
    }

    rdb::Descriptor data2 = rdb::Descriptor("Name", 10, rdb::String) | rdb::Descriptor("Len3", rdb::Uint) | rdb::Descriptor("Control", rdb::Byte);
    {
        std::stringstream coutstring;
        char test[] = "{\tString Name[10]\n\tUint Len3\n\tByte Control\n}";
        coutstring << data2;
        if (strcmp(coutstring.str().c_str(), test) != 0)
            return false;
    }

    if (data2.Position("Control") != 2)
        return false;
    if (data2.Len("Control") != 1)
        return false;
    if (strcmp(data2.Type("Control").c_str(), "Byte") != 0)
        return false;
    if (data2.Offset("Control") != 14)
        return false;

    return true;
}

bool test_descriptor_read()
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
        if(strcmp(coutstring.str().c_str(), test) != 0)
            return false;
    }

    // end cin test

    return true ;
}

bool test_storage()
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
    if (dataDescriptor.GetSize() != sizeof(dataPayload))
        return false;

    dataPayload payload1;

    rdb::DataStorageAccessor dAcc2(dataDescriptor, dataStore);

    if (strcmp(dataStore.FileName().c_str(), "datafile-fstream2") != 0)
        return false;

    std::memcpy(payload1.Name, "test data", AREA_SIZE);
    payload1.TLen = 0x66;
    payload1.Control = 0x22;

    if (payload1.TLen != dAcc2.descriptor.Cast<int>("TLen", payload1.ptr))
        return false;

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
        if (strcmp(coutstring.str().c_str(), "xxxx xxxx") != 0)
            return false;
    }

    {
        std::stringstream coutstring;
        coutstring << std::hex;
        coutstring << dAcc2.descriptor.Cast<int>("TLen", payload3.ptr);
        coutstring << std::dec;
        if (strcmp(coutstring.str().c_str(), "67") != 0)
            return false;
    }

    {
        std::stringstream coutstring;
        coutstring << std::hex;
        coutstring << (uint)dAcc2.descriptor.Cast<uint8_t>("Control", payload3.ptr);
        coutstring << std::dec;
        if (strcmp(coutstring.str().c_str(), "33") != 0)
            return false;
    }

    auto statusRemove = remove(dataStore.FileName().c_str());

    return true;
}

TEST( xrdb, test_descriptor_read )
{
    ASSERT_TRUE( test_descriptor_read() );
}

TEST( xrdb, test_descriptor )
{
    ASSERT_TRUE( test_descriptor() );
}

TEST( xrdb, test_storage )
{
    ASSERT_TRUE( test_storage() );
}

TEST( crdb , genericBinaryFileAccessor_byte )
{
    auto result1 = test_1<std::byte, rdb::genericBinaryFileAccessor<std::byte>>();
    ASSERT_TRUE ( result1 );
    auto result2 = test_2<std::byte, rdb::genericBinaryFileAccessor<std::byte>>();
    ASSERT_TRUE ( result2 );
    auto result3 = test_3<std::byte, rdb::genericBinaryFileAccessor<std::byte>>();
    ASSERT_TRUE ( result3 );
}

TEST( crdb , genericBinaryFileAccessor_char )
{
    auto result1 = test_1<char, rdb::genericBinaryFileAccessor<char>>();
    ASSERT_TRUE ( result1 );
    auto result2 = test_2<char, rdb::genericBinaryFileAccessor<char>>();
    ASSERT_TRUE ( result2 );
    auto result3 = test_3<char, rdb::genericBinaryFileAccessor<char>>();
    ASSERT_TRUE ( result3 );
}

TEST( crdb , posixBinaryFileAccessor_byte )
{
    auto result1 = test_1<std::byte, rdb::posixBinaryFileAccessor<std::byte>>();
    ASSERT_TRUE ( result1 );
    auto result2 = test_2<std::byte, rdb::posixBinaryFileAccessor<std::byte>>();
    ASSERT_TRUE ( result2 );
    auto result3 = test_3<std::byte, rdb::posixBinaryFileAccessor<std::byte>>();
    ASSERT_TRUE ( result3 );
}

TEST( crdb , posixBinaryFileAccessor_char )
{
    auto result1 = test_1<char, rdb::posixBinaryFileAccessor<char>>();
    ASSERT_TRUE ( result1 );
    auto result2 = test_2<char, rdb::posixBinaryFileAccessor<char>>();
    ASSERT_TRUE ( result2 );
    auto result3 = test_3<char, rdb::posixBinaryFileAccessor<char>>();
    ASSERT_TRUE ( result3 );
}