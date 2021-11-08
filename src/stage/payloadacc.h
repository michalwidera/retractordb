#ifndef STORAGE_RDB_INCLUDE_PAYLOADACC_H_
#define STORAGE_RDB_INCLUDE_PAYLOADACC_H_

#include <cstddef> // std::byte

#include "desc.h"

namespace rdb
{
    template <typename T>
    struct payLoadAccessor
    {
        Descriptor descriptor ;

        T *ptr ;

        payLoadAccessor(Descriptor descriptor , T *ptr );

        /**
         * @brief In case of string types this funcion will get binary representation and convert it to string
         * by accessing name
         *
         * @param name Name of searched field
         * @param ptr Pointer to std::byte table / begininng package
         * @return std::string Returned string from field.
         */
        /*
        std::string ToString(const std::string name)
        {
            return std::string(reinterpret_cast<char *>(ptr + desc.Offset(name)), desc.Len(name));
        }
        */
        /**
         * @brief Reads data from binary package via tuple-data from inner container
         *
         * @tparam T Type that data should be converted (returned)
         * @param name name of given field
         * @param ptr pointer to begininng of package
         * @return auto Value from binary package that corresponds to field from container
         */
        template <typename K>
        auto Cast(const std::string name)
        {
            return *(reinterpret_cast<K *>(ptr + descriptor.Offset(name)));
        };

        friend std::ostream &operator<<(std::ostream &os, const payLoadAccessor<std::byte> &rhs);
    };
}

#endif // STORAGE_RDB_INCLUDE_PAYLOADACC_H_