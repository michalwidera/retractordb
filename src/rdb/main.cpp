#include <iostream>
#include <fstream>
#include <assert.h>
#include <string>
#include <cstring>
#include <locale>
#include <memory>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <filesystem>

#include "rdb/dsacc.h"
#include "rdb/desc.h"
#include "rdb/faccfs.h"
#include "rdb/faccposix.h"
#include "rdb/payloadacc.h"

#include "rdb/fainterface.h"

#define GREEN "\x1B[32m"
#define BLACK "\033[0m"
#define RED "\x1B[31m"
#define ORANGE "\x1B[33m"
#define BLUE "\x1B[34m"
#define YELLOW "\x1B[93m"

void check_debug()
{
    // Diagnostic code

#ifdef NDEBUG

    std::cerr << RED "Warn: Release Code form.\n" BLACK;

    assert(false); // Note this assert will have no effect!

#else
    // https://stackoverflow.com/questions/4053837/colorizing-text-in-the-console-with-c
    struct check_assert
    {
        bool ok()
        {
            std::cout << GREEN "Debug Code Form.\n" BLACK;
            return true;
        }
    } check;
    assert(check.ok()); // This assert show that assert is compiled and works.
                        // Program will show green "Ok." at the end of work if assert is compiled and executed.
#endif
}

int main(int argc, char *argv[])
{
    check_debug();

    //#define STORAGE_TYPE rdb::posixBinaryFileAccessor<std::byte>

#define STORAGE_TYPE rdb::genericBinaryFileAccessor<std::byte>

    std::unique_ptr<STORAGE_TYPE> uPtr_store;
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
            std::cout << YELLOW << desc << BLACK << std::endl;
        }
        else if (cmd == "open")
        {
            std::cin >> file;

            if (std::filesystem::exists(file))
            {
                uPtr_store.reset(new STORAGE_TYPE(file.c_str()));

                std::string descriptionFileName(file);
                descriptionFileName.append(".desc");

                if (std::filesystem::exists(descriptionFileName))
                {
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
                else
                {
                    std::cout << RED "File exist, description file missing (.desc)\n" BLACK;
                }
            }
            else
            {
                std::cout << RED "File does not exist\n" BLACK;
            }
        }
        else if (cmd == "create")
        {
            std::cin >> file;

            uPtr_store.reset(new STORAGE_TYPE(file.c_str()));

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
                std::ifstream in(file.c_str(), std::ifstream::ate | std::ifstream::binary);
                auto size = int(in.tellg() / desc.GetSize());

                if (record >= size)
                {
                    std::cout << RED "record out of range\n" BLACK;
                }
                else
                {
                    uPtr_dacc->Get(uPtr_payload.get(), record);
                    std::cout << "ok\n";
                }
            }
            else
            {
                std::cout << RED "unconnected\n" BLACK;
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
                std::cout << RED "unconnected\n" BLACK;
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
                std::cout << RED "unconnected\n" BLACK;
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
                std::cout << RED "unconnected\n" BLACK;
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
                std::cout << RED "unconnected\n" BLACK;
            }
        }
        else if (cmd == "size")
        {
            std::ifstream in(file.c_str(), std::ifstream::ate | std::ifstream::binary);
            std::cout << int(in.tellg() / desc.GetSize()) << " Record(s)\n";
        }
        else if (cmd == "dump")
        {
            auto *ptr = reinterpret_cast<unsigned char *>(uPtr_payload.get());
            if (uPtr_payload)
            {
                for (auto i = 0; i < desc.GetSize(); i++)
                {
                    std::cout << std::hex;
                    std::cout << std::setfill('0');
                    std::cout << std::setw(2);
                    std::cout << static_cast<unsigned>(*(ptr + i));
                    std::cout << std::dec;
                    std::cout << " ";
                };
                std::cout << "\n";
            }
            else
            {
                std::cout << RED "unconnected\n" BLACK;
            }
        }
        else if (cmd == "help")
        {
            std::cout << GREEN;
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
            std::cout << "dump \t\t\t show payload memory\n";
            std::cout << BLACK;
        }
        else
        {
            std::cout << RED "?\n" BLACK;
        }
    } while (true);

    // use '$xxd datafile-11' to check
    return 0;
}
