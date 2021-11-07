#include <assert.h>
#include <locale>
#include <iostream>

#include "desc.h"

namespace rdb
{

    template <typename L, typename R>
    std::map<R, L>
    makeReverse(std::map<L, R> list)
    {
        std::map<R, L> retVal;
        for (const auto &[key, value] : list)
            retVal[value] = key;
        return retVal;
    }

    std::map<std::string, FieldType> typeDictionary = {{"String", String}, {"Uint", Uint}, {"Byte", Byte}, {"Int", Int}};
    std::map<FieldType, std::string> typeDictionaryR = makeReverse(typeDictionary);

    FieldType GetFieldType(std::string e)
    {
        return typeDictionary[e];
    }

    std::string GetFieldType(FieldType e)
    {
        return typeDictionaryR[e];
    }

    uint GetFieldLenFromType(FieldType ft)
    {
        switch (ft)
        {
        case Uint:
            return sizeof(uint);
        case Int:
            return sizeof(int);
        case Byte:
            return 1;
        case String:
            assert(len != 0 && "String has other method of len-type id");
            break;
        default:
            assert(len != 0 && "Undefined type");
        }
        return 0;
    }

    Descriptor::Descriptor(std::initializer_list<field> l) : std::vector<field>(l)
    {
    }

    Descriptor::Descriptor(fieldName n, fieldLen l, FieldType t)
    {
        push_back(field(n, l, t));
        UpdateNames();
    }

    Descriptor::Descriptor(fieldName n, FieldType t)
    {
        assert(t != String && "This method does not work for Stings");
        push_back(field(n, GetFieldLenFromType(t), t));
        UpdateNames();
    }

    void Descriptor::Append(std::initializer_list<field> l)
    {
        insert(end(), l.begin(), l.end());
    }

    Descriptor &Descriptor::operator|(const Descriptor &rhs)
    {
        if (this != &rhs)
        {
            insert(end(), rhs.begin(), rhs.end());
        }
        else
        {
            //Descriptor b(*this);
            //insert(end(), b.begin(), b.end());
            assert(false && "can not merge same to same");
            // can't do safe: data | data
            // due one name policy
        }
        UpdateNames();
        return *this;
    }

    Descriptor &Descriptor::operator=(const Descriptor &rhs)
    {
        clear();
        insert(end(), rhs.begin(), rhs.end());
        UpdateNames();
        return *this;
    }

    Descriptor::Descriptor(const Descriptor &init)
    {
        *this | init;
        UpdateNames();
    }

    uint Descriptor::GetSize() const
    {
        uint size = 0;
        for (auto const i : *this)
        {
            size += std::get<len>(i);
        }
        return size;
    }

    bool Descriptor::UpdateNames()
    {
        uint position = 0;
        fieldNames.clear();
        for (auto const i : *this)
        {
            if (fieldNames.find(std::get<fieldName>(i)) == fieldNames.end())
            {
                fieldNames[std::get<fieldName>(i)] = position;
            }
            else
            {
                fieldNames.clear();
                assert(false && "that name aleardy exist so it will make dirty");
                return false;
            }
            position++;
        }
        return true;
    }

    uint Descriptor::Position(std::string name)
    {
        if (fieldNames.find(name) != fieldNames.end())
            return fieldNames[name];

        assert(false && "did not find that record id Descriptor:{}");
        return -1;
    }

    uint Descriptor::Len(const std::string name)
    {
        auto pos = Position(name);
        return std::get<len>((*this)[pos]);
    }

    uint Descriptor::Offset(const std::string name)
    {
        auto offset = 0;
        for (auto const i : *this)
        {
            if (name == std::get<fieldName>(i))
                return offset;
            offset += std::get<len>(i);
        }
        assert(false && "record not found with that name");
        return -1;
    }

    std::string Descriptor::Type(const std::string name)
    {
        auto pos = Position(name);
        return GetFieldType(std::get<type>((*this)[pos]));
    }

    std::ostream &operator<<(std::ostream &os, const Descriptor &rhs)
    {
        os << "{";
        for (auto const &r : rhs)
        {
            os << "\t" << GetFieldType(std::get<type>(r))
               << " " << std::get<name>(r);
            if (std::get<type>(r) == String)
            {
                os << "[" << std::to_string(std::get<len>(r)) << "]";
            };
            os << std::endl;
        }
        os << "}";

        if (rhs.fieldNames.size() == 0)
            os << "[dirty]";

        return os;
    }

    // Look here to explain: https://stackoverflow.com/questions/7302996/changing-the-delimiter-for-cin-c
    struct synsugar_is_space : std::ctype<char>
    {
        synsugar_is_space() : ctype<char>(get_table()) {}
        static mask const *get_table()
        {
            static mask rc[table_size];
            rc['['] = rc[']'] = rc['{'] = rc['}'] = rc[' '] = rc['\n'] = std::ctype_base::space;
            return &rc[0];
        }
    };

    std::istream &operator>>(std::istream &is, Descriptor &rhs)
    {
        is.imbue(std::locale(is.getloc(), std::unique_ptr<synsugar_is_space>(new synsugar_is_space).release()));

        do
        {
            std::string type;
            std::string name;
            int len = 0;
            is >> type;
            if (is.eof())
            {
                break;
            }
            is >> name;

            FieldType ft = GetFieldType(type);

            if (ft == String)
            {
                is >> len;
            }
            else
            {
                len = GetFieldLenFromType(ft);
            }

            rhs | Descriptor(name, len, ft);

        } while (!is.eof());

        return is;
    }

} // namespace rdb