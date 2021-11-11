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

    template< typename T , typename K >
    constexpr void copyToMemory(std::istream &is, const K &rhs, std::string fieldName)
    {
        T data;
        is >> data;
        Descriptor desc(rhs.descriptor);
        memcpy(rhs.ptr + desc.Offset(fieldName), &data, sizeof(T));
    }

    template <typename K>
    std::istream &operator>>(std::istream &is, const payLoadAccessor<K> &rhs)
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
        else if (desc.Type(fieldName) == "Byte")
        {
            int data;
            is >> data;
            unsigned char data8 = static_cast<unsigned char>(data);
            memcpy(rhs.ptr + desc.Offset(fieldName), &data8, sizeof(unsigned char));
        }
        else if (desc.Type(fieldName) == "Uint")
        {
            copyToMemory<uint, payLoadAccessor<K>>(is, rhs, fieldName);
        }
        else if (desc.Type(fieldName) == "Int")
        {
            copyToMemory<int, payLoadAccessor<K>>(is, rhs, fieldName);
        }
        else if (desc.Type(fieldName) == "Float")
        {
            copyToMemory<float, payLoadAccessor<K>>(is, rhs, fieldName);
        }
        else if (desc.Type(fieldName) == "Double")
        {
            copyToMemory<double, payLoadAccessor<K>>(is, rhs, fieldName);
        }
        else
        {
            std::cerr << "field not found\n";
        }

        return is;
    }

    template< class K >
    std::ostream &operator<<(std::ostream &os, const payLoadAccessor<K> &rhs)
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
            else if (std::get<rtype>(r) == Float)
            {
                float i;
                memcpy( &i , rhs.ptr + offset_ , sizeof(float) );
                os << i;
            }
            else if (std::get<rtype>(r) == Double)
            {
                double i;
                memcpy( &i , rhs.ptr + offset_ , sizeof(double) );
                os << i;
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

    template std::istream &operator>>(std::istream &is, const payLoadAccessor<std::byte> &rhs);
    template std::istream &operator>>(std::istream &is, const payLoadAccessor<char> &rhs);
    template std::istream &operator>>(std::istream &is, const payLoadAccessor<unsigned char> &rhs);

    template std::ostream &operator<<(std::ostream &os, const payLoadAccessor<std::byte> &rhs);
    template std::ostream &operator<<(std::ostream &os, const payLoadAccessor<char> &rhs);
    template std::ostream &operator<<(std::ostream &os, const payLoadAccessor<unsigned char> &rhs);
}
