#ifndef STORAGE_RDB_INCLUDE_DESC_H_
#define STORAGE_RDB_INCLUDE_DESC_H_

#include <string>
#include <tuple>
#include <initializer_list>
#include <vector>
#include <map>

namespace rdb
{

    // https://developers.google.com/protocol-buffers/docs/overview#scalar
    // https://doc.rust-lang.org/book/ch03-02-data-types.html

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

    /**
     * @brief Structure resposible for mapping types into binary struct
     */
    struct Descriptor : std::vector<field>
    {
        std::map<std::string, int> fieldNames;

        Descriptor(std::initializer_list<field> l);
        Descriptor(fieldName n, fieldLen l, FieldType t);
        Descriptor(fieldName n, FieldType t);
        Descriptor() = default;

        void Append(std::initializer_list<field> l);
        Descriptor &operator|(const Descriptor &rhs);
        Descriptor &operator=(const Descriptor &rhs);
        Descriptor(const Descriptor &init);
        uint GetSize() const;
        bool UpdateNames();
        uint Position(std::string name);
        uint Len(const std::string name);
        uint Offset(const std::string name);
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