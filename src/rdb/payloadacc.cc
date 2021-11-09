#include <iostream>
#include <cstddef> // std::byte
#include <cstring> // std:memcpy
#include <assert.h>

#include "rdb/payloadacc.h"

namespace rdb
{
    template< typename T >
    payLoadAccessor<T>::payLoadAccessor(Descriptor descriptor, T *ptr ) : descriptor(descriptor) , ptr(ptr)
    {
    }

    std::istream &operator>>(std::istream &is, const payLoadAccessor<std::byte> &rhs)
    {
        std::string fieldName;
        is >> fieldName;

        if (is.eof())
        {
            return is;
        }

        Descriptor desc(rhs.descriptor);

        if (desc.Type(fieldName) == "String")
        {
            std::string record;
            std::getline(is >> std::ws, record);
            memset(rhs.ptr + desc.Offset(fieldName), 0, desc.Len(fieldName));
            memcpy(rhs.ptr + desc.Offset(fieldName), record.c_str(), std::min((size_t)desc.Len(fieldName), record.size()));
        }
        else if (desc.Type(fieldName) == "Uint")
        {
            uint data;
            is >> data;
            memcpy(rhs.ptr + desc.Offset(fieldName), &data, sizeof(uint));
        }
        else if (desc.Type(fieldName) == "Int")
        {
            int data;
            is >> data;
            memcpy(rhs.ptr + desc.Offset(fieldName), &data, sizeof(int));
        }
        else if (desc.Type(fieldName) == "Byte")
        {
            int data;
            is >> data;
            unsigned char data8 = static_cast<unsigned char>(data);
            memcpy(rhs.ptr + desc.Offset(fieldName), &data8, sizeof(unsigned char));
        }
        else
        {
            std::cerr << "field not found\n";
        }

        return is;
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
                unsigned char i;
                memcpy( &i , rhs.ptr + offset_ , sizeof(unsigned char) );
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
