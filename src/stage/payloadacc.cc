#include <iostream>
#include <cstddef> // std::byte

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

            if (std::get<rtype>(r) == String)
            {
                auto desc = rhs.descriptor;
                auto offset_ = desc.Offset(std::get<rname>(r));
                auto len_ = desc.Len(std::get<rname>(r));
                os << std::string(reinterpret_cast<char *>(rhs.ptr + offset_), len_)  ;
            }
            else
            {
                os << "todo" ;
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
