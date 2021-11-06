#ifndef STORAGE_RDB_INCLUDE_DESC_H_
#define STORAGE_RDB_INCLUDE_DESC_H_

#include <string>
#include <tuple>
#include <initializer_list>
#include <vector>
#include <map>

namespace rdb
{

    enum FieldType
    {
        String,
        Uint,
        Byte,
        Int
    };

    typedef int fieldLen;
    typedef std::string fieldName;
    typedef std::tuple<fieldName, fieldLen, FieldType> field;

    enum FieldColumn
    {
        name = 0,
        len = 1,
        type = 2
    };

    struct Descriptor : std::vector<field>
    {
        std::map<std::string, int> fieldNames;

        Descriptor(std::initializer_list<field> l);
        Descriptor(fieldName n, fieldLen l, FieldType t);
        Descriptor(fieldName n, FieldType t);
        Descriptor();

        void Append(std::initializer_list<field> l);
        Descriptor &operator|(const Descriptor &rhs);
        Descriptor &operator=(const Descriptor &rhs);
        Descriptor(const Descriptor &init);
        uint GetSize() const;
        bool UpdateNames();
        int Position(std::string name);
        int Len(const std::string name);
        int Offset(const std::string name);
        std::string Type(const std::string name);
        std::string ToString(const std::string name, std::byte *ptr);

        template <typename T>
        auto Cast(const std::string name, std::byte *ptr)
        {
            return *(reinterpret_cast<T *>(ptr + Offset(name)));
        };

        friend std::ostream &operator<<(std::ostream &os, const Descriptor &rhs);
        friend std::istream &operator>>(std::istream &is, Descriptor &rhs);
    };

} // namespace rdb

#endif //STORAGE_RDB_INCLUDE_DESC_H_