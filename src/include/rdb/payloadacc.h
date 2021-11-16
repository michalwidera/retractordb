#ifndef STORAGE_RDB_INCLUDE_PAYLOADACC_H_
#define STORAGE_RDB_INCLUDE_PAYLOADACC_H_

#include <cstddef> // std::byte

#include "desc.h"

namespace rdb
{
    /**
     * @brief This class define accessing method to payload (memory area)
     *
     * @tparam T Type of stored data - std::byte or char
     */
    template <typename T>
    struct payLoadAccessor
    {
        /**
         * Descriptor of managed payload area
         */
        Descriptor descriptor ;

        /**
         * Pointer to payload
         */
        T *ptr ;

        /**
         * Type of dumped or read numeric formats
         */
        bool hexFormat;

        /**
         * Constructor of payLoadAccessor object
         *
         * @param descriptor descriptor of payload area
         * @param ptr pointer to payload
         * @param hexFormat type of default stored data
         */
        payLoadAccessor(Descriptor descriptor , T *ptr , bool hexFormat = false );

        /**
         * Default constructor is dissalowed
         */
        payLoadAccessor() = delete;

        template <typename K>
        friend std::istream &operator>>(std::istream &is, const payLoadAccessor<K> &rhs);

        template <typename K>
        friend std::ostream &operator<<(std::ostream &os, const payLoadAccessor<K> &rhs);
    };
}

#endif // STORAGE_RDB_INCLUDE_PAYLOADACC_H_