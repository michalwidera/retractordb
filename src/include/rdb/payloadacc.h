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

        bool hexFormat;

        payLoadAccessor(Descriptor descriptor , T *ptr , bool hexFormat = false );

        template <typename K>
        friend std::istream &operator>>(std::istream &is, const payLoadAccessor<K> &rhs);

        template <typename K>
        friend std::ostream &operator<<(std::ostream &os, const payLoadAccessor<K> &rhs);
    };
}

#endif // STORAGE_RDB_INCLUDE_PAYLOADACC_H_