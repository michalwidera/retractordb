#include <iostream>
#include <fstream>
#include <assert.h>
#include <string>
#include <cstring>
#include <filesystem>

#include "rdb/dsacc.h"
#include "rdb/desc.h"
#include "rdb/payloadacc.h"


#include "rdb/faccposix.h"
#include "rdb/faccfs.h"

#define GREEN "\x1B[32m"
#define RED "\x1B[31m"
#define ORANGE "\x1B[33m"
#define BLUE "\x1B[34m"
#define YELLOW "\x1B[93m"
#define RESET "\033[0m"

enum payloadStatusType {
    fetched,
    clean,
    stored,
    changed
};

payloadStatusType payloadStatus(clean) ;

void check_debug()
{
    // Diagnostic code

#ifdef NDEBUG

    std::cerr << RED "Warn: Release Code form.\n" RESET;

    assert(false); // Note this assert will have no effect!

#else
    // https://stackoverflow.com/questions/4053837/colorizing-text-in-the-console-with-c
    struct check_assert
    {
        bool ok()
        {
            std::cout << GREEN "Debug Code Form.\n" RESET;
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
            std::cout << YELLOW << desc << RESET << std::endl;
            continue;
        }
        else if (cmd == "open" || cmd == "ropen")
        {
            std::cin >> file;

            if (!std::filesystem::exists(file))
            {
                std::cout << RED "File does not exist\n" RESET;
                continue;
            }

            uPtr_dacc.reset(new rdb::DataStorageAccessor(file, cmd == "ropen"));

            if ( uPtr_dacc->descriptor.GetSize() == 0 )
            {
                std::cout << RED "File exist, description file missing (.desc)\n" RESET;
                continue;
            }
            desc = uPtr_dacc->descriptor ;

            uPtr_payload.reset(new std::byte[desc.GetSize()]);
            memset(uPtr_payload.get(), 0, desc.GetSize());

            payloadStatus = clean;
        }
        else if (cmd == "create" || cmd == "rcreate" )
        {
            std::cin >> file;

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

            uPtr_dacc.reset(new rdb::DataStorageAccessor(desc, file, cmd == "rcreate" ));

            uPtr_payload.reset(new std::byte[desc.GetSize()]);
            memset(uPtr_payload.get(), 0, desc.GetSize());

            payloadStatus = clean;
        }
        else if (cmd == "read")
        {
            int record;
            std::cin >> record;

            if (!uPtr_dacc)
            {
                std::cout << RED "unconnected\n" RESET;
                continue;
            }

            if (record >= uPtr_dacc->recordsCount)
            {
                std::cout << RED "record out of range\n" RESET;
                continue;
            }

            uPtr_dacc->Get(uPtr_payload.get(), record);
            payloadStatus = fetched;
        }
        else if (cmd == "set")
        {
            if (!uPtr_dacc || desc.size() == 0)
            {
                std::cout << RED "unconnected\n" RESET;
                continue;
            }

            rdb::payLoadAccessor payload(desc, uPtr_payload.get());

            std::cin >> payload;

            payloadStatus = changed;
            continue;
        }
        else if (cmd == "print")
        {
            if (!uPtr_dacc || desc.size() == 0)
            {
                std::cout << RED "unconnected\n" RESET;
                continue;
            }

            rdb::payLoadAccessor payload(desc, uPtr_payload.get());

            std::cout << payload << std::endl;
            continue;
        }
        else if (cmd == "write")
        {
            if (!uPtr_dacc)
            {
                std::cout << RED "unconnected\n" RESET;
                continue;
            }

            size_t record;
            std::cin >> record;

            if (record >= uPtr_dacc->recordsCount)
            {
                std::cout << RED "record out of range - Check append command.\n" RESET;
                continue;
            }

            uPtr_dacc->Put(uPtr_payload.get(), record);
            payloadStatus = stored;
        }
        else if (cmd == "append")
        {
            if (!uPtr_dacc)
            {
                std::cout << RED "unconnected\n" RESET;
                continue;
            }
            uPtr_dacc->Put(uPtr_payload.get());
            payloadStatus = stored;
        }
        else if (cmd == "status")
        {
            if (!uPtr_payload)
            {
                std::cout << RED "unconnected\n" RESET;
                continue;
            }
            switch (payloadStatus)
            {
            case (fetched):
                std::cout << "fetched\n";
                break;
            case (clean):
                std::cout << "clean\n";
                break;
            case (stored):
                std::cout << "stored\n";
                break;
            case (changed):
                std::cout << "changed\n";
                break;
            defualt:
                std::cout << "unknown\n";
            }
        }
        else if (cmd == "size")
        {
            if (!uPtr_dacc)
            {
                std::cout << RED "unconnected\n" RESET;
                continue;
            }

            std::cout << uPtr_dacc->recordsCount << " Record(s)\n";
            continue;
        }
        else if (cmd == "dump")
        {
            if (!uPtr_payload)
            {
                std::cout << RED "unconnected\n" RESET;
                continue;
            }
            auto *ptr = reinterpret_cast<unsigned char *>(uPtr_payload.get());
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
        else if (cmd == "help")
        {
            std::cout << GREEN;
            std::cout << "exit|quit|q \t\t\t exit\n";
            std::cout << "open|ropen [file] \t\t open database - create connection (r-reverse iterator)\n";
            std::cout << "create|rcreate [file][schema] \t create database with schema (r-reverse iterator)\n";
            std::cout << "desc \t\t\t\t show schema\n";
            std::cout << "read [n] \t\t\t read record from database into payload\n";
            std::cout << "write [n] \t\t\t send record to database from payload\n";
            std::cout << "append \t\t\t\t append payload to database\n";
            std::cout << "set [field][value] \t\t set payload field value\n";
            std::cout << "status \t\t\t\t show status of payload\n";
            std::cout << "print \t\t\t\t show payload\n";
            std::cout << "size \t\t\t\t show database size in records\n";
            std::cout << "dump \t\t\t\t show payload memory\n";
            std::cout << RESET;
        }
        else
        {
            std::cout << RED "?\n" RESET;
            continue;
        }
        std::cout << "ok\n";
    } while (true);

    // use '$xxd datafile-11' to check
    return 0;
}