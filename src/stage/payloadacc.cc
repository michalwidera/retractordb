#include <iostream>
#include <cstddef> // std::byte
#include <cstring> // std:memcpy
#include <assert.h>

#include "payloadacc.h"

namespace rdb
{
    template< typename T >
    payLoadAccessor<T>::payLoadAccessor(Descriptor descriptor, T *ptr ) : descriptor(descriptor) , ptr(ptr)
    {
    }

    std::ostream &operator<<(std::ostream &os, const payLoadAccessor<std::byte> &rhs)
    {
        os << "{";

        for (auto const &r : rhs.descriptor)
        {
            os << "\t" << std::get<rname>(r) << ":" ;

            auto desc = rhs.descriptor;
            auto offset_ = desc.Offset(std::get<rname>(r));

            if (std::get<rtype>(r) == String)
            {
                auto len_ = desc.Len(std::get<rname>(r));
                os << std::string(reinterpret_cast<char *>(rhs.ptr + offset_), len_)  ;
            }
            else if (std::get<rtype>(r) == Int)
            {
                int i;
                memcpy( &i , rhs.ptr + offset_ , sizeof(int) );
                os << i;
            }
            else if (std::get<rtype>(r) == Uint)
            {
                unsigned int i;
                memcpy( &i , rhs.ptr + offset_ , sizeof(unsigned int) );
                os << i;
            }
            else if (std::get<rtype>(r) == Byte)
            {
                uint8_t i;
                memcpy( &i , rhs.ptr + offset_ , sizeof(uint8_t) );
                os << (int)i;
            }
            else
            {
                assert(false && "Unrecognized type");
            }

            os << std::endl;
        }
        os << "}";

        if (rhs.descriptor.fieldNames.size() == 0)
            os << "[dirty]";

        return os;
    }

    template class payLoadAccessor<std::byte>;
    template class payLoadAccessor<char>;
    template class payLoadAccessor<unsigned char>;
}
