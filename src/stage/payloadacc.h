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

        friend std::ostream &operator<<(std::ostream &os, const payLoadAccessor<std::byte> &rhs);
    };
}

#endif // STORAGE_RDB_INCLUDE_PAYLOADACC_H_