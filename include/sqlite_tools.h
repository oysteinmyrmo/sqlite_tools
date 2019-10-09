/* Copyright © 2018 Øystein Myrmo (oystein.myrmo@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once
#include <cassert>
#include <string>
#include <vector>

#if !defined(SQLITE_OK)
    static_assert(false, "sqlite3.h must be included when using SQLT.");
#endif

namespace SQLT
{
    /**
     * SQLT Internal namespace. Should normally not be referenced externally.
     */
    namespace Internal
    {
        template<size_t...> struct Sequence { using type = Sequence; };

        template<typename A, typename B> struct Merge;
        template<size_t ... Is1, size_t ... Is2>
        struct Merge<Sequence<Is1...>, Sequence<Is2...>>
        {
            using type = Sequence<Is1..., (sizeof...(Is1)+Is2)...>;
        };

        template<size_t size> struct GenSequence;
        template<> struct GenSequence<0> { using type = Sequence<>; };
        template<> struct GenSequence<1> { using type = Sequence<0>; };
        template<size_t size>
        struct GenSequence
        {
            using type = typename Merge<typename GenSequence<size / size_t(2)>::type, typename GenSequence<size - size / size_t(2)>::type>::type;
        };

        template<size_t index, typename T>
        struct Element
        {
            Element()
                : data()
            {}

            Element(const T &t)
                : data(t)
            {}

            using type = T;
            T data;
        };

        template<typename A, typename ...Bs> struct TupleImpl;

        template<size_t ...indices, typename ...Ts>
        struct TupleImpl<Sequence<indices...>, Ts...> : public Element<indices, Ts>...
        {
            TupleImpl()
                : Element<indices, Ts>()...
            {}

            TupleImpl(Ts ...args)
                : Element<indices, Ts>(args)...
            {}
        };

        template<size_t I, typename ...Ts>

        struct TypeAt
        {
            template<typename T>
            static Internal::Element<I, T> deduce(Internal::Element<I, T>);

            using tuple_impl = Internal::TupleImpl<typename Internal::GenSequence<sizeof...(Ts)>::type, Ts...>;
            using element = decltype(deduce(tuple_impl()));
            using type = typename element::type;
        };

        // Tuple, stolen from https://github.com/jorgen/json_tools <-- json_tools is awesome, go give it some love!
        template<typename ...Ts>
        struct Tuple
        {
            Tuple()
                : impl()
            {}

            Tuple(Ts ...args)
                : impl(args...)
            {}

            using Seq = typename Internal::GenSequence<sizeof...(Ts)>::type;
            Internal::TupleImpl<Seq, Ts...> impl;
            static constexpr const size_t size = sizeof...(Ts);

            template<size_t Index>
            const typename TypeAt<Index, Ts...>::type &get() const
            {
                return static_cast<const typename TypeAt<Index, Ts...>::element&>(impl).data;
            }

            template<size_t Index>
            typename TypeAt<Index, Ts...>::type &get()
            {
                return static_cast<typename TypeAt<Index, Ts...>::element&>(impl).data;
            }
        };

        template<size_t I, typename ...Ts>
        struct TypeAt<I, const Tuple<Ts...>>
        {
            template<typename T>
            static Internal::Element<I, T> deduce(Internal::Element<I, T>);

            using tuple_impl = Internal::TupleImpl<typename Internal::GenSequence<sizeof...(Ts)>::type, Ts...>;
            using element = decltype(deduce(tuple_impl()));
            using type = typename element::type;
        };

        template<size_t I, typename ...Ts>
        struct TypeAt<I, Tuple<Ts...>>
        {
            template<typename T>
            static Internal::Element<I, T> deduce(Internal::Element<I, T>);

            using tuple_impl = Internal::TupleImpl<typename Internal::GenSequence<sizeof...(Ts)>::type, Ts...>;
            using element = decltype(deduce(tuple_impl()));
            using type = typename element::type;
        };

        template<>
        struct Tuple<>
        {
            static constexpr const size_t size = 0;
        };

        template<typename ...Ts>
        Tuple<Ts...> makeTuple(Ts ...args)
        {
            return Tuple<Ts...>(args...);
        }
    } // End namespace Interna;

    /**
     * Enum for SQLite flags. Normally it is not needed to use this enum, except if any of the macros SQLT_COLUMN_FLAGS or SQLT_COLUMN_FLAGS_DEFAULT are used explicitly.
     */
    enum class Flags : uint8_t
    {
        NONE        = 0x00,
        PRIMARY_KEY = 0x01,
        DEFAULT     = 0x02,
        NOT_NULL    = 0x04 // Should not be set explicitly, but is set internally based on whether or not a member is of SQLT::Nullable<T> type.
    };

    inline constexpr Flags operator & (Flags l, Flags r)
    {
        return static_cast<Flags>(static_cast<uint8_t>(l) & static_cast<uint8_t>(r));
    }

    inline constexpr Flags operator | (Flags l, Flags r)
    {
        return static_cast<Flags>(static_cast<uint8_t>(l) | static_cast<uint8_t>(r));
    }

    inline constexpr bool operator == (Flags l, Flags r)
    {
        return static_cast<uint8_t>(l) == static_cast<uint8_t>(r);
    }

    inline constexpr bool operator == (Flags l, uint8_t r)
    {
        return static_cast<uint8_t>(l) == r;
    }

    /**
     * SQLT Internal namespace. Should normally not be referenced externally.
     */
    namespace Internal
    {
        inline constexpr bool flagSet(Flags f, Flags check)
        {
            return static_cast<uint8_t>(f & check);
        }

        struct ColName
        {
            template <size_t N>
            constexpr explicit ColName(const char (&data)[N])
                : data(data)
                , size(N-1)
            {}

            std::string toString() const
            {
                return std::string(data, size);
            }

            bool operator==(const std::string& other) const
            {
                return (other.size() == size) && (other.compare(0, size, data) == 0);
            }

            const char *data; // Not null-terminated
            size_t size;
        };

        typedef ColName TableName;
        typedef ColName DatabaseName;
        typedef ColName DatabasePath;
    }

    /**
     * A Nullable type can insert and select NULL values from the database.
     *
     * The Nullable data type can be used for columns that are not marked as "NOT NULL". Columns that use primitive data
     * types like int (INTEGER), double (REAL) or std::string (TEXT) are implicitly marked as "NOT NULL". If you need NULL
     * values for a column, use this type.
     */
    template <typename T>
    struct Nullable
    {
        Nullable()
            : is_null(true)
        {}

        Nullable(const T& value)
            : value(value)
            , is_null(false)
        {}

        Nullable(const T& value, bool is_null)
            : value(value)
            , is_null(is_null)
        {}

        bool operator==(const Nullable<T>& other) const
        {
            return (is_null && other.is_null) || (value == other.value);
        }

        T value;
        bool is_null;
    };

    template <>
    struct Nullable<std::string>
    {
        Nullable()
            : is_null(true)
        {}

        Nullable(const std::string& value)
            : value(value)
            , is_null(false)
        {}

        /// Conversion constructor
        Nullable(const char* convertValue)
            : value(convertValue)
            , is_null(false)
        {}

        Nullable(const std::string& value, bool is_null)
            : value(value)
            , is_null(is_null)
        {}

        Nullable(const char* value, bool is_null)
            : value(value)
            , is_null(is_null)
        {}

        bool operator==(const Nullable<std::string>& other) const
        {
            return (is_null && other.is_null) || (value == other.value);
        }

        std::string value;
        bool is_null;
    };

#define SQLT_COL_INFO_INTEGER_FUNCTIONS \
    static constexpr bool isInteger() { return true; } \
    static constexpr bool isReal() { return false; } \
    static constexpr bool isText() { return false; } \
    static constexpr const char* datatype() { return "INTEGER"; }

#define SQLT_COL_INFO_REAL_FUNCTIONS \
    static constexpr bool isInteger() { return false; } \
    static constexpr bool isReal() { return true; } \
    static constexpr bool isText() { return false; } \
    static constexpr const char* datatype() { return "REAL"; }

#define SQLT_COL_INFO_TEXT_FUNCTIONS \
    static constexpr bool isInteger() { return false; } \
    static constexpr bool isReal() { return false; } \
    static constexpr bool isText() { return true; } \
    static constexpr const char* datatype() { return "TEXT"; }

#define SQLT_COL_INFO_DEFAULT_FUNCTIONS \
    bool isPrimaryKey() const { return SQLT::Internal::flagSet(flags, Flags::PRIMARY_KEY); } \
    bool isNotNull() const { return SQLT::Internal::flagSet(flags, Flags::NOT_NULL); } \
    bool hasDefaultValue() const { static_assert(!SQLT::Internal::flagSet(flags, Flags::DEFAULT), "Cannot use Flags::DEFAULT in a column that has no default value."); return false; } \
    std::string DATATYPE() const \
    { \
        std::string dtype = name.toString() + " " + std::string(datatype()); \
        if (isNotNull()) \
            dtype += " NOT NULL"; \
        return dtype; \
    }

#define SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE \
    bool isPrimaryKey() const { return SQLT::Internal::flagSet(flags, Flags::PRIMARY_KEY); } \
    bool isNotNull() const { return SQLT::Internal::flagSet(flags, Flags::NOT_NULL); } \
    bool hasDefaultValue() const { static_assert(SQLT::Internal::flagSet(flags, Flags::DEFAULT), "Must use Flags::DEFAULT in a column that has a default value."); return true; } \
    std::string DATATYPE() const \
    { \
        std::string dtype = name.toString() + " " + std::string(datatype()); \
        if (isNotNull()) \
            dtype += " NOT NULL"; \
        dtype += " DEFAULT " + defaultValueString(); \
        return dtype; \
    }

    /**
     * SQLT Internal namespace. Should normally not be referenced externally.
     */
    namespace Internal
    {
        template<typename T, typename U>
        struct ColInfo
        {
            ColName name;
            T U::* member;
            Flags flags;
            typedef T type;
            typedef U classType;
        };

        template<typename U>
        struct ColInfo<int, U>
        {
            ColName name;
            int U::* member;
            Flags flags;
            typedef int type;
            typedef U classType;

            SQLT_COL_INFO_DEFAULT_FUNCTIONS
            SQLT_COL_INFO_INTEGER_FUNCTIONS
        };

        template<typename U>
        struct ColInfo<double, U>
        {
            ColName name;
            double U::* member;
            Flags flags;
            typedef double type;
            typedef U classType;

            SQLT_COL_INFO_DEFAULT_FUNCTIONS
            SQLT_COL_INFO_REAL_FUNCTIONS
        };

        template<typename U>
        struct ColInfo<std::string, U>
        {
            ColName name;
            std::string U::* member;
            Flags flags;
            typedef std::string type;
            typedef U classType;

            SQLT_COL_INFO_DEFAULT_FUNCTIONS
            SQLT_COL_INFO_TEXT_FUNCTIONS
        };

        // Fake bools to be an int type since SQLite does not have booleans.
        template<typename U>
        struct ColInfo<bool, U>
        {
            ColName name;
            bool U::* member;
            Flags flags;
            typedef bool type;
            typedef U classType;

            SQLT_COL_INFO_DEFAULT_FUNCTIONS
            SQLT_COL_INFO_INTEGER_FUNCTIONS
        };

        template<typename U>
        struct ColInfo<Nullable<int>, U>
        {
            ColName name;
            Nullable<int> U::* member;
            Flags flags;
            typedef Nullable<int> type;
            typedef U classType;

            SQLT_COL_INFO_DEFAULT_FUNCTIONS
            SQLT_COL_INFO_INTEGER_FUNCTIONS
        };

        template<typename U>
        struct ColInfo<Nullable<double>, U>
        {
            ColName name;
            Nullable<double> U::* member;
            Flags flags;
            typedef Nullable<double> type;
            typedef U classType;

            SQLT_COL_INFO_DEFAULT_FUNCTIONS
            SQLT_COL_INFO_REAL_FUNCTIONS
        };

        template<typename U>
        struct ColInfo<Nullable<std::string>, U>
        {
            ColName name;
            Nullable<std::string> U::* member;
            Flags flags;
            typedef Nullable<std::string> type;
            typedef U classType;

            SQLT_COL_INFO_DEFAULT_FUNCTIONS
            SQLT_COL_INFO_TEXT_FUNCTIONS
        };

        // Fake bools to be an int type since SQLite does not have booleans.
        template<typename U>
        struct ColInfo<Nullable<bool>, U>
        {
            ColName name;
            Nullable<bool> U::* member;
            Flags flags;
            typedef Nullable<bool> type;
            typedef U classType;

            SQLT_COL_INFO_DEFAULT_FUNCTIONS
            SQLT_COL_INFO_INTEGER_FUNCTIONS
        };

        template<typename T, typename U>
        struct ColInfoWithDefault
        {
            ColName name;
            T U::* member;
            T defaultValue;
            Flags flags;
            typedef T type;
            typedef U classType;
        };

        template<typename U>
        struct ColInfoWithDefault<int, U>
        {
            ColName name;
            int U::* member;
            int defaultValue;
            Flags flags;
            typedef int type;
            typedef U classType;

            std::string defaultValueString() const { return std::to_string(defaultValue); }
            SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE
            SQLT_COL_INFO_INTEGER_FUNCTIONS
        };

        template<typename U>
        struct ColInfoWithDefault<double, U>
        {
            ColName name;
            double U::* member;
            double defaultValue;
            Flags flags;
            typedef double type;
            typedef U classType;

            std::string defaultValueString() const { return std::to_string(defaultValue); }
            SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE
            SQLT_COL_INFO_REAL_FUNCTIONS
        };

        template<typename U>
        struct ColInfoWithDefault<std::string, U>
        {
            ColName name;
            std::string U::* member;
            std::string defaultValue;
            Flags flags;
            typedef std::string type;
            typedef U classType;

            std::string defaultValueString() const { return std::string("\"") + defaultValue + "\""; }
            SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE
            SQLT_COL_INFO_TEXT_FUNCTIONS
        };

        // Fake bools to be an int type since SQLite does not have booleans.
        template<typename U>
        struct ColInfoWithDefault<bool, U>
        {
            ColName name;
            bool U::* member;
            bool defaultValue;
            Flags flags;
            typedef bool type;
            typedef U classType;

            std::string defaultValueString() const { return (defaultValue == true) ? "1" : "0"; }
            SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE
            SQLT_COL_INFO_INTEGER_FUNCTIONS
        };

        template<typename U>
        struct ColInfoWithDefault<Nullable<int>, U>
        {
            ColName name;
            Nullable<int> U::* member;
            int defaultValue;
            Flags flags;
            typedef Nullable<int> type;
            typedef U classType;

            std::string defaultValueString() const { return std::to_string(defaultValue); }
            SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE
            SQLT_COL_INFO_INTEGER_FUNCTIONS
        };

        template<typename U>
        struct ColInfoWithDefault<Nullable<double>, U>
        {
            ColName name;
            Nullable<double> U::* member;
            double defaultValue;
            Flags flags;
            typedef Nullable<double> type;
            typedef U classType;

            std::string defaultValueString() const { return std::to_string(defaultValue); }
            SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE
            SQLT_COL_INFO_REAL_FUNCTIONS
        };

        template<typename U>
        struct ColInfoWithDefault<Nullable<std::string>, U>
        {
            ColName name;
            Nullable<std::string> U::* member;
            std::string defaultValue;
            Flags flags;
            typedef Nullable<std::string> type;
            typedef U classType;

            std::string defaultValueString() const { return std::string("\"") + defaultValue + "\""; }
            SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE
            SQLT_COL_INFO_TEXT_FUNCTIONS
        };

        // Fake bools to be an int type since SQLite does not have booleans.
        template<typename U>
        struct ColInfoWithDefault<Nullable<bool>, U>
        {
            ColName name;
            Nullable<bool> U::* member;
            bool defaultValue;
            Flags flags;
            typedef Nullable<bool> type;
            typedef U classType;

            std::string defaultValueString() const { return (defaultValue == true) ? "1" : "0"; }
            SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE
            SQLT_COL_INFO_INTEGER_FUNCTIONS
        };

        // Table name creation.
        template<size_t NAME_SIZE>
        constexpr const TableName makeTableName(const char(&name)[NAME_SIZE])
        {
            return TableName(name);
        }

        // General type without default value.
        template<typename T, typename U, size_t NAME_SIZE>
        constexpr const ColInfo<T, U> makeColumnInfo(const char(&name)[NAME_SIZE], T U::* member, Flags flags)
        {
            return { ColName(name), member, flags | Flags::NOT_NULL };
        }

        // General type with default value.
        template<typename T, typename U, size_t NAME_SIZE>
        constexpr const ColInfoWithDefault<T, U> makeColumnInfo(const char(&name)[NAME_SIZE], T U::* member, T defaultValue, Flags flags)
        {
            return { ColName(name), member, defaultValue, flags | Flags::NOT_NULL };
        }

        // String type with default value.
        template<typename T, typename U, size_t NAME_SIZE>
        constexpr const ColInfoWithDefault<std::string, U> makeColumnInfo(const char(&name)[NAME_SIZE], T U::* member, const char* defaultValue, Flags flags)
        {
            return { ColName(name), member, std::string(defaultValue), flags | Flags::NOT_NULL };
        }

        // General Nullable type specialization.
        template<typename T, typename U, size_t NAME_SIZE>
        constexpr const ColInfo<SQLT::Nullable<T>, U> makeColumnInfo(const char(&name)[NAME_SIZE], SQLT::Nullable<T> U::* member, Flags flags)
        {
            return { ColName(name), member, flags };
        }

        // Nullable type with default value.
        template<typename T, typename U, size_t NAME_SIZE>
        constexpr const ColInfoWithDefault<SQLT::Nullable<T>, U> makeColumnInfo(const char(&name)[NAME_SIZE], SQLT::Nullable<T> U::* member, T defaultValue, Flags flags)
        {
            return { ColName(name), member, defaultValue, flags };
        }

        // Nullable string type with default value.
        template<typename U, size_t NAME_SIZE>
        constexpr const ColInfoWithDefault<SQLT::Nullable<std::string>, U> makeColumnInfo(const char(&name)[NAME_SIZE], SQLT::Nullable<std::string> U::* member, const char* defaultValue, Flags flags)
        {
            return { ColName(name), member, std::string(defaultValue), flags };
        }

        template<typename T1, typename U1, typename T2, typename U2>
        struct MemberPointerComparer
        {
            static inline bool compare(T1 U1::* member1, T2 U2::* member2)
            {
                return false;
            }
        };

        template<typename U1, typename U2>
        struct MemberPointerComparer<int, U1, int, U2>
        {
            static inline bool compare(int U1::* member1, int U2::* member2)
            {
                return member1 == member2;
            }
        };

        template<typename U1, typename U2>
        struct MemberPointerComparer<std::string, U1, std::string, U2>
        {
            static inline bool compare(std::string U1::* member1, std::string U2::* member2)
            {
                return member1 == member2;
            }
        };

        template<typename U1, typename U2>
        struct MemberPointerComparer<double, U1, double, U2>
        {
            static inline bool compare(double U1::* member1, double U2::* member2)
            {
                return member1 == member2;
            }
        };

        template<typename U1, typename U2>
        struct MemberPointerComparer<SQLT::Nullable<int>, U1, SQLT::Nullable<int>, U2>
        {
            static inline bool compare(SQLT::Nullable<int> U1::* member1, SQLT::Nullable<int> U2::* member2)
            {
                return member1 == member2;
            }
        };

        template<typename U1, typename U2>
        struct MemberPointerComparer<SQLT::Nullable<std::string>, U1, SQLT::Nullable<std::string>, U2>
        {
            static inline bool compare(SQLT::Nullable<std::string> U1::* member1, SQLT::Nullable<std::string> U2::* member2)
            {
                return member1 == member2;
            }
        };

        template<typename U1, typename U2>
        struct MemberPointerComparer<SQLT::Nullable<double>, U1, SQLT::Nullable<double>, U2>
        {
            static inline bool compare(SQLT::Nullable<double> U1::* member1, SQLT::Nullable<double> U2::* member2)
            {
                return member1 == member2;
            }
        };

        template<size_t INDEX, size_t SIZE, typename COL_TUPLE, typename T, typename U>
        struct ColumnTraverser_GetColumnName
        {
            static inline std::string traverse(const COL_TUPLE &columns, T U::* member)
            {
                auto col = columns.template get<INDEX>();
                if (MemberPointerComparer<typename decltype(col)::type, typename decltype(col)::classType, T, U>::compare(col.member, member))
                    return col.name.toString();
                return ColumnTraverser_GetColumnName<INDEX + 1, SIZE, COL_TUPLE, T, U>::traverse(columns, member);
            }
        };

        template<size_t INDEX, typename COL_TUPLE, typename T, typename U>
        struct ColumnTraverser_GetColumnName<INDEX, INDEX, COL_TUPLE, T, U>
        {
            static inline std::string traverse(const COL_TUPLE &columns, T U::* member)
            {
                auto col = columns.template get<INDEX>();
                if (MemberPointerComparer<typename decltype(col)::type, typename decltype(col)::classType, T, U>::compare(col.member, member))
                    return col.name.toString();
                return "";
            }
        };

        template<typename SQLT_TABLE, typename T, typename U>
        inline std::string getColumnName(T U::* member)
        {
            auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
            return ColumnTraverser_GetColumnName<0, decltype(columns)::size - 1, decltype(columns), T, U>::traverse(columns, member);
        }

        template<size_t INDEX, typename COL_TUPLE>
        struct ColumnTraverser_GetColumnInfoPosition
        {
            static inline size_t traverse(const COL_TUPLE &columns, const std::string& colName)
            {
                auto& col = columns.template get<INDEX>();
                if (col.name == colName)
                    return INDEX;
                return ColumnTraverser_GetColumnInfoPosition<INDEX - 1, COL_TUPLE>::traverse(columns, colName);
            }
        };

        template<typename COL_TUPLE>
        struct ColumnTraverser_GetColumnInfoPosition<0, COL_TUPLE>
        {
            static inline size_t traverse(const COL_TUPLE &columns, const std::string& colName)
            {
                auto& col = columns.template get<0>();
                if (col.name == colName)
                    return 0;
                return -1; // Underflow on purpose
            }
        };

        template<typename SQLT_TABLE>
        inline size_t getColumnInfoPosition(const std::string& colName)
        {
            auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
            return ColumnTraverser_GetColumnInfoPosition<decltype(columns)::size - 1, decltype(columns)>::traverse(columns, colName);
        }

        template<size_t INDEX, typename COL_TUPLE>
        struct ColumnTraverser_PrimaryKeyCount
        {
            static inline size_t traverse(const COL_TUPLE &columns)
            {
                auto& col = columns.template get<INDEX>();
                size_t pk = col.isPrimaryKey() ? 1 : 0;
                return pk + ColumnTraverser_PrimaryKeyCount<INDEX - 1, COL_TUPLE>::traverse(columns);
            }
        };

        template<typename COL_TUPLE>
        struct ColumnTraverser_PrimaryKeyCount<0, COL_TUPLE>
        {
            static inline size_t traverse(const COL_TUPLE &columns)
            {
                auto& col = columns.template get<0>();
                return col.isPrimaryKey() ? 1 : 0;
            }
        };

        template<typename SQLT_TABLE>
        inline size_t primaryKeyCount()
        {
            auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
            return ColumnTraverser_PrimaryKeyCount<decltype(columns)::size - 1, decltype(columns)>::traverse(columns);
        }

        template<size_t INDEX, size_t SIZE, typename COL_TUPLE>
        struct ColumnTraverser_CreateTablePrimaryKeys
        {
            static inline void traverse(const COL_TUPLE &columns, std::string &statement, size_t pkCount, size_t &usedPks)
            {
                if (usedPks == pkCount)
                    return;

                auto& col = columns.template get<INDEX>();
                if (col.isPrimaryKey())
                {
                    usedPks++;
                    if (usedPks == pkCount)
                    {
                        statement += col.name.toString();
                        return;
                    }
                    statement += col.name.toString() + ",";
                }
                ColumnTraverser_CreateTablePrimaryKeys<INDEX + 1, SIZE, COL_TUPLE>::traverse(columns, statement, pkCount, usedPks);
            }
        };

        template<size_t INDEX, typename COL_TUPLE>
        struct ColumnTraverser_CreateTablePrimaryKeys<INDEX, INDEX, COL_TUPLE>
        {
            // Note: If we end up here, the column in question implicitly *must* be a primary key.
            static inline void traverse(const COL_TUPLE &columns, std::string &statement, size_t pkCount, size_t &usedPks)
            {
                auto& col = columns.template get<INDEX>();
                statement += col.name.toString();
            }
        };

        template<typename SQLT_TABLE>
        inline std::string createPrimaryKeyStatement()
        {
            std::string statement("");
            size_t pkCount = primaryKeyCount<SQLT_TABLE>();
            if (pkCount > 0)
            {
                statement += "PRIMARY KEY(";
                size_t usedPks = 0;
                auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
                ColumnTraverser_CreateTablePrimaryKeys<0, decltype(columns)::size - 1, decltype(columns)>::traverse(columns, statement, pkCount, usedPks);
                statement += ")";
            }
            return statement;
        }

        template<size_t INDEX, size_t SIZE, typename COL_TUPLE>
        struct ColumnTraverser_CreateTable
        {
            static inline void traverse(const COL_TUPLE &columns, std::string& query)
            {
                auto& col = columns.template get<INDEX>();
                query += col.DATATYPE() + ",";
                ColumnTraverser_CreateTable<INDEX + 1, SIZE, COL_TUPLE>::traverse(columns, query);
            }
        };

        template<size_t INDEX, typename COL_TUPLE>
        struct ColumnTraverser_CreateTable<INDEX, INDEX, COL_TUPLE>
        {
            static inline void traverse(const COL_TUPLE &columns, std::string& query)
            {
                auto& col = columns.template get<INDEX>();
                query += col.DATATYPE();
            }
        };

        template<typename COL_TUPLE>
        inline std::string createTableContents()
        {
            std::string query;
            auto columns = COL_TUPLE::template SQLTBase<COL_TUPLE>::sqlt_static_column_info();
            ColumnTraverser_CreateTable<0, decltype(columns)::size - 1, decltype(columns)>::traverse(columns, query);
            return query;
        }

        template<size_t INDEX, size_t SIZE, typename COL_TUPLE>
        struct ColumnTraverser
        {
            static inline void createColumnNameList(const COL_TUPLE &columns, std::string& query)
            {
                auto& col = columns.template get<INDEX>();
                query += col.name.toString() + ",";
                ColumnTraverser<INDEX + 1, SIZE, COL_TUPLE>::createColumnNameList(columns, query);
            }

            static inline void createColumnNameQuestionMarkList(const COL_TUPLE &columns, std::string& query)
            {
                query += "?,";
                ColumnTraverser<INDEX + 1, SIZE, COL_TUPLE>::createColumnNameQuestionMarkList(columns, query);
            }

            static inline void insert(const COL_TUPLE &columns, std::string& query)
            {
                auto& col = columns.template get<INDEX>();
                query += col.DATATYPE() + ",";
                ColumnTraverser<INDEX + 1, SIZE, COL_TUPLE>::insert(columns, query);
            }

            static inline size_t columnCount(const COL_TUPLE& columns)
            {
                return 1 + ColumnTraverser<INDEX + 1, SIZE, COL_TUPLE>::columnCount(columns);
            }
        };

        template<size_t INDEX, typename COL_TUPLE>
        struct ColumnTraverser<INDEX, INDEX, COL_TUPLE>
        {
            static inline void createColumnNameList(const COL_TUPLE &columns, std::string& query)
            {
                auto& col = columns.template get<INDEX>();
                query += col.name.toString();
            }

            static inline void createColumnNameQuestionMarkList(const COL_TUPLE &columns, std::string& query)
            {
                query += "?";
            }

            static inline void insert(const COL_TUPLE &columns, std::string& query)
            {
                auto& col = columns.template get<INDEX>();
                query += col.DATATYPE();
            }

            static inline size_t columnCount(const COL_TUPLE &columns)
            {
                return 1;
            }
        };

        template<typename SQLT_TABLE>
        inline std::string createColumnNameList()
        {
            std::string query;
            auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
            ColumnTraverser<0, decltype(columns)::size - 1, decltype(columns)>::createColumnNameList(columns, query);
            return query;
        }

        template<typename SQLT_TABLE>
        inline std::string createColumnNameQuestionMarkList()
        {
            std::string query;
            auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
            ColumnTraverser<0, decltype(columns)::size - 1, decltype(columns)>::createColumnNameQuestionMarkList(columns, query);
            return query;
        }

        template<typename SQLT_TABLE>
        inline std::string createInsertPreparedStatement()
        {
            std::string query = "INSERT INTO ";
            auto tableName = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_table_name();
            query += tableName.toString();

            query += "(";
            query += createColumnNameList<SQLT_TABLE>();
            query += ") VALUES(";
            query += createColumnNameQuestionMarkList<SQLT_TABLE>();
            query += ");";

            return query;
        }

        template<typename SQLT_TABLE>
        inline size_t columnCount()
        {
            auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
            return ColumnTraverser<0, decltype(columns)::size - 1, decltype(columns)>::columnCount(columns);
        }

        template<typename T>
        struct SQLiteValueBinder
        {
            static inline int bindValue(sqlite3_stmt *stmt, int index, T value)
            {
                assert(false); // Missing bindValue() template specialization for column of type T.
                return SQLITE_ERROR;
            }
        };

        template<>
        struct SQLiteValueBinder<int>
        {
            static inline int bindValue(sqlite3_stmt *stmt, int index, int value)
            {
                return sqlite3_bind_int(stmt, index, value);
            }
        };

        template<>
        struct SQLiteValueBinder<double>
        {
            static inline int bindValue(sqlite3_stmt *stmt, int index, double value)
            {
                return sqlite3_bind_double(stmt, index, value);
            }
        };

        template<>
        struct SQLiteValueBinder<std::string>
        {
            static inline int bindValue(sqlite3_stmt *stmt, int index, const std::string& value)
            {
                return sqlite3_bind_text(stmt, index, value.c_str(), (int)value.length(), SQLITE_STATIC);
            }
        };

        // Fake bools to be an int type since SQLite does not have booleans.
        template<>
        struct SQLiteValueBinder<bool>
        {
            static inline int bindValue(sqlite3_stmt *stmt, int index, bool value)
            {
                return sqlite3_bind_int(stmt, index, value ? 1 : 0);
            }
        };

        template<>
        struct SQLiteValueBinder<SQLT::Nullable<int>>
        {
            static inline int bindValue(sqlite3_stmt *stmt, int index, const SQLT::Nullable<int>& value)
            {
                return value.is_null ? sqlite3_bind_null(stmt, index) : sqlite3_bind_int(stmt, index, value.value);
            }
        };

        template<>
        struct SQLiteValueBinder<SQLT::Nullable<double>>
        {
            static inline int bindValue(sqlite3_stmt *stmt, int index, const SQLT::Nullable<double>& value)
            {
                return value.is_null ? sqlite3_bind_null(stmt, index) : sqlite3_bind_double(stmt, index, value.value);
            }
        };

        template<>
        struct SQLiteValueBinder<SQLT::Nullable<std::string>>
        {
            static inline int bindValue(sqlite3_stmt *stmt, int index, const SQLT::Nullable<std::string>& value)
            {
                return value.is_null ? sqlite3_bind_null(stmt, index) : sqlite3_bind_text(stmt, index, value.value.c_str(), (int)value.value.length(), SQLITE_STATIC);
            }
        };

        // Fake bools to be an int type since SQLite does not have booleans.
        template<>
        struct SQLiteValueBinder<SQLT::Nullable<bool>>
        {
            static inline int bindValue(sqlite3_stmt *stmt, int index, const SQLT::Nullable<bool>& value)
            {
                return value.is_null ? sqlite3_bind_null(stmt, index) : sqlite3_bind_int(stmt, index, value.value ? 1 : 0);
            }
        };

        template<typename T, typename U, typename SQLT_TABLE>
        inline int bindValue(const ColInfo<T, U>& colInfo, const SQLT_TABLE& row, sqlite3_stmt *stmt, int index)
        {
            return SQLiteValueBinder<T>::bindValue(stmt, index, row.*colInfo.member);
        }

        template<typename T, typename U, typename SQLT_TABLE>
        inline int bindValue(const ColInfoWithDefault<T, U>& colInfo, const SQLT_TABLE& row, sqlite3_stmt *stmt, int index)
        {
            return SQLiteValueBinder<T>::bindValue(stmt, index, row.*colInfo.member);
        }

        template<typename T, typename U, typename SQLT_TABLE>
        struct SQLiteMemberAssigner
        {
            static inline void assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, T U::* memberPtr)
            {
                assert(false); // T is not a type that SQLite Tools can use. Must be int, double or std::string
            }
        };

        template<typename U, typename SQLT_TABLE>
        struct SQLiteMemberAssigner<int, U, SQLT_TABLE>
        {
            static inline void assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, int U::* memberPtr)
            {
                row.*memberPtr = sqlite3_column_int(stmt, index);
            }
        };

        template<typename U, typename SQLT_TABLE>
        struct SQLiteMemberAssigner<double, U, SQLT_TABLE>
        {
            static inline void assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, double U::* memberPtr)
            {
                row.*memberPtr = sqlite3_column_double(stmt, index);
            }
        };

        template<typename U, typename SQLT_TABLE>
        struct SQLiteMemberAssigner<std::string, U, SQLT_TABLE>
        {
            static inline void assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, std::string U::* memberPtr)
            {
                row.*memberPtr = std::string((const char*)sqlite3_column_text(stmt, index));
            }
        };

        // Fake bools to be an int type since SQLite does not have booleans.
        template<typename U, typename SQLT_TABLE>
        struct SQLiteMemberAssigner<bool, U, SQLT_TABLE>
        {
            static inline void assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, bool U::* memberPtr)
            {
                row.*memberPtr = (sqlite3_column_int(stmt, index) == 0) ? false : true;
            }
        };

        template<typename U, typename SQLT_TABLE>
        struct SQLiteMemberAssigner<SQLT::Nullable<int>, U, SQLT_TABLE>
        {
            static inline void assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, SQLT::Nullable<int> U::* memberPtr)
            {
                int dataType = sqlite3_column_type(stmt, index);
                if (dataType == SQLITE_NULL)
                {
                    (row.*memberPtr).is_null = true;
                }
                else
                {
                    assert(dataType == SQLITE_INTEGER);
                    (row.*memberPtr).is_null = false;
                    (row.*memberPtr).value = sqlite3_column_int(stmt, index);
                }
            }
        };

        template<typename U, typename SQLT_TABLE>
        struct SQLiteMemberAssigner<SQLT::Nullable<double>, U, SQLT_TABLE>
        {
            static inline void assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, SQLT::Nullable<double> U::* memberPtr)
            {
                int dataType = sqlite3_column_type(stmt, index);
                if (dataType == SQLITE_NULL)
                {
                    (row.*memberPtr).is_null = true;
                }
                else
                {
                    assert(dataType == SQLITE_FLOAT);
                    (row.*memberPtr).is_null = false;
                    (row.*memberPtr).value = sqlite3_column_double(stmt, index);
                }
            }
        };

        template<typename U, typename SQLT_TABLE>
        struct SQLiteMemberAssigner<SQLT::Nullable<std::string>, U, SQLT_TABLE>
        {
            static inline void assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, SQLT::Nullable<std::string> U::* memberPtr)
            {
                int dataType = sqlite3_column_type(stmt, index);
                if (dataType == SQLITE_NULL)
                {
                    (row.*memberPtr).is_null = true;
                }
                else
                {
                    assert(dataType == SQLITE_TEXT);
                    (row.*memberPtr).is_null = false;
                    (row.*memberPtr).value = std::string((const char*)sqlite3_column_text(stmt, index));
                }
            }
        };

        // Fake bools to be an int type since SQLite does not have booleans.
        template<typename U, typename SQLT_TABLE>
        struct SQLiteMemberAssigner<SQLT::Nullable<bool>, U, SQLT_TABLE>
        {
            static inline void assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, SQLT::Nullable<bool> U::* memberPtr)
            {
                int dataType = sqlite3_column_type(stmt, index);
                if (dataType == SQLITE_NULL)
                {
                    (row.*memberPtr).is_null = true;
                }
                else
                {
                    assert(dataType == SQLITE_INTEGER);
                    (row.*memberPtr).is_null = false;
                    (row.*memberPtr).value = sqlite3_column_int(stmt, index);
                }
            }
        };

        template<typename T, typename U, typename SQLT_TABLE>
        inline void assignMember(const ColInfo<T, U>& colInfo, SQLT_TABLE& row, sqlite3_stmt *stmt, int index)
        {
            SQLiteMemberAssigner<T, U, SQLT_TABLE>::assignMember(stmt, index, row, colInfo.member);
        }

        template<typename T, typename U, typename SQLT_TABLE>
        inline void assignMember(const ColInfoWithDefault<T, U>& colInfo, SQLT_TABLE& row, sqlite3_stmt *stmt, int index)
        {
            SQLiteMemberAssigner<T, U, SQLT_TABLE>::assignMember(stmt, index, row, colInfo.member);
        }

        template<typename T>
        struct SQLiteValueAssigner
        {
            static inline void assignValue(T& value, sqlite3_stmt *stmt)
            {
                assert(false); // T is not a type that SQLite Tools can use. Must be int, double or std::string
            }
        };

        template<>
        struct SQLiteValueAssigner<int>
        {
            static inline void assignValue(int& value, sqlite3_stmt *stmt)
            {
                int dataType = sqlite3_column_type(stmt, 0);
                assert(dataType != SQLITE_NULL);
                value = sqlite3_column_int(stmt, 0);
            }
        };

        template<>
        struct SQLiteValueAssigner<double>
        {
            static inline void assignValue(double& value, sqlite3_stmt *stmt)
            {
                int dataType = sqlite3_column_type(stmt, 0);
                assert(dataType != SQLITE_NULL);
                value = sqlite3_column_double(stmt, 0);
            }
        };

        template<>
        struct SQLiteValueAssigner<std::string>
        {
            static inline void assignValue(std::string& value, sqlite3_stmt *stmt)
            {
                int dataType = sqlite3_column_type(stmt, 0);
                assert(dataType != SQLITE_NULL);
                value = std::string((const char*)sqlite3_column_text(stmt, 0));
            }
        };

        template<>
        struct SQLiteValueAssigner<SQLT::Nullable<int>>
        {
            static inline void assignValue(SQLT::Nullable<int>& value, sqlite3_stmt *stmt)
            {
                int dataType = sqlite3_column_type(stmt, 0);
                value.is_null = (dataType == SQLITE_NULL);
                if (!value.is_null)
                {
                    assert(dataType == SQLITE_INTEGER);
                    value.value = sqlite3_column_int(stmt, 0);
                }
            }
        };

        template<>
        struct SQLiteValueAssigner<SQLT::Nullable<double>>
        {
            static inline void assignValue(SQLT::Nullable<double>& value, sqlite3_stmt *stmt)
            {
                int dataType = sqlite3_column_type(stmt, 0);
                value.is_null = (dataType == SQLITE_NULL);
                if (!value.is_null)
                {
                    assert(dataType == SQLITE_FLOAT);
                    value.value = sqlite3_column_double(stmt, 0);
                }
            }
        };

        template<>
        struct SQLiteValueAssigner<SQLT::Nullable<std::string>>
        {
            static inline void assignValue(SQLT::Nullable<std::string>& value, sqlite3_stmt *stmt)
            {
                int dataType = sqlite3_column_type(stmt, 0);
                value.is_null = (dataType == SQLITE_NULL);
                if (!value.is_null)
                {
                    assert(dataType == SQLITE_TEXT);
                    value.value = std::string((const char*)sqlite3_column_text(stmt, 0));
                }
            }
        };

        template<typename T>
        inline void assignValue(T& value, sqlite3_stmt *stmt)
        {
            SQLiteValueAssigner<T>::assignValue(value, stmt);
        }

        template<size_t INDEX, size_t SIZE, typename COL_TUPLE, typename SQLT_TABLE>
        struct SQLiteColumnTraverser
        {
            static inline int iterateAndBindValues(const COL_TUPLE& columns, const SQLT_TABLE& row, sqlite3_stmt *stmt)
            {
                int result = bindValue(columns.template get<INDEX>(), row, stmt, (int)INDEX + 1); // SQLite binds are 1-indexed
                if (result != SQLITE_OK)
                    return result;
                return SQLiteColumnTraverser<INDEX + 1, SIZE, COL_TUPLE, SQLT_TABLE>::iterateAndBindValues(columns, row, stmt);
            }

            static inline void iterateAndAssignMembers(const COL_TUPLE& columns, SQLT_TABLE& row, sqlite3_stmt *stmt)
            {
                assignMember(columns.template get<INDEX>(), row, stmt, (int)INDEX);
                SQLiteColumnTraverser<INDEX + 1, SIZE, COL_TUPLE, SQLT_TABLE>::iterateAndAssignMembers(columns, row, stmt);
            }

            static inline bool iterateAndAssignMembersByColumnName(const COL_TUPLE& columns, SQLT_TABLE& row, sqlite3_stmt *stmt, const char* const colName, int colIndex)
            {
                auto colInfo = columns.template get<INDEX>();
                if (colInfo.name == colName)
                {
                    assignMember(colInfo, row, stmt, colIndex);
                    return true;
                }
                return SQLiteColumnTraverser<INDEX + 1, SIZE, COL_TUPLE, SQLT_TABLE>::iterateAndAssignMembersByColumnName(columns, row, stmt, colName, colIndex);
            }
        };

        template<size_t INDEX, typename COL_TUPLE, typename SQLT_TABLE>
        struct SQLiteColumnTraverser<INDEX, INDEX, COL_TUPLE, SQLT_TABLE>
        {
            static inline int iterateAndBindValues(const COL_TUPLE& columns, const SQLT_TABLE& row, sqlite3_stmt *stmt)
            {
                return bindValue(columns.template get<INDEX>(), row, stmt, (int)INDEX + 1); // SQLite binds are 1-indexed
            }

            static inline void iterateAndAssignMembers(const COL_TUPLE& columns, SQLT_TABLE& row, sqlite3_stmt *stmt)
            {
                assignMember(columns.template get<INDEX>(), row, stmt, (int)INDEX);
            }

            static inline bool iterateAndAssignMembersByColumnName(const COL_TUPLE& columns, SQLT_TABLE& row, sqlite3_stmt *stmt, const char* const colName, int colIndex)
            {
                auto colInfo = columns.template get<INDEX>();
                if (colInfo.name == colName)
                {
                    assignMember(colInfo, row, stmt, colIndex);
                    return true;
                }
                return false;
            }
        };

        template<typename SQLT_TABLE>
        inline int iterateAndBindValues(const SQLT_TABLE& row, sqlite3_stmt *stmt)
        {
            auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
            return SQLiteColumnTraverser<0, decltype(columns)::size - 1, decltype(columns), SQLT_TABLE>::iterateAndBindValues(columns, row, stmt);
        }

        template<typename SQLT_TABLE>
        inline void iterateAndAssignMembers(SQLT_TABLE& row, sqlite3_stmt *stmt)
        {
            auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
            SQLiteColumnTraverser<0, decltype(columns)::size - 1, decltype(columns), SQLT_TABLE>::iterateAndAssignMembers(columns, row, stmt);
        }

        template<typename SQLT_QUERY_STRUCT>
        inline void iterateAndAssignMembersByColumnName(SQLT_QUERY_STRUCT& row, sqlite3_stmt *stmt, const char* const colName, int colIndex)
        {
            auto columns = SQLT_QUERY_STRUCT::template SQLTBase<SQLT_QUERY_STRUCT>::sqlt_static_column_info();
            SQLiteColumnTraverser<0, decltype(columns)::size - 1, decltype(columns), SQLT_QUERY_STRUCT>::iterateAndAssignMembersByColumnName(columns, row, stmt, colName, colIndex);
        }
    } // End namespace Internal

    /**
     * Get the table name for an SQLT table struct.
     *
     * @tparam SQLT_TABLE An SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME.
     * @return The table name for the SQLT table struct template parameter.
     */
    template<typename SQLT_TABLE>
    inline std::string tableName()
    {
        auto tableName = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_table_name();
        return tableName.toString();
    }

    /**
     * Create a table if it does not already exist.
     *
     * @tparam SQLT_TABLE An SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME.
     * @param db The sqlite3 instance to create the table in.
     * @param errMsg The sqlite3 message output.
     * @return The SQLite error code. Will be SQLITE_OK if the table was successfully created or if it already existed.
     *
     * @see SQLT::createAllTables(sqlite3 *db, char **errMsg)
     * @see SQLT::createAllTables(char **errMsg)
     */
    template<typename SQLT_TABLE>
    inline int createTableIfNotExists(sqlite3 *db, char **errMsg)
    {
        std::string query = std::string("CREATE TABLE IF NOT EXISTS ") + tableName<SQLT_TABLE>();
        query += "(";
        query += SQLT::Internal::createTableContents<SQLT_TABLE>();
        size_t pkCount = SQLT::Internal::primaryKeyCount<SQLT_TABLE>();
        if (pkCount > 0)
        {
            query += ",";
            query += SQLT::Internal::createPrimaryKeyStatement<SQLT_TABLE>();
        }
        query += ");";
        return sqlite3_exec(db, query.c_str(), NULL, NULL, errMsg);
    }

    /**
     * Drop a table if it exists.
     *
     * @tparam SQLT_TABLE An SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME.
     * @param db The sqlite3 instance to drop the table in.
     * @param errMsg The sqlite3 message output.
     * @return The SQLite error code. Will be SQLITE_OK if the table was successfully dropped or if it did not exist.
     *
     * @see SQLT::createAllTables(sqlite3 *db, char **errMsg)
     * @see SQLT::createAllTables(char **errMsg)
     */
    template<typename SQLT_TABLE>
    inline int dropTableIfExists(sqlite3 *db, char **errMsg)
    {
        std::string stmt = std::string("DROP TABLE IF EXISTS ") + tableName<SQLT_TABLE>() + ";";
        return sqlite3_exec(db, stmt.c_str(), NULL, NULL, errMsg);
    }

    /**
     * Insert rows into a table.
     *
     * @tparam SQLT_TABLE An SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME.
     * @param db The sqlite3 instance to drop the table in.
     * @param rows The rows to insert into the database.
     * @return The SQLite error code. Will be SQLITE_OK if the rows were successfully inserted.
     *
     * @see SQLT::insert(sqlite3 *db, const SQLT_TABLE& row)
     * @see SQLT::insert(const std::vector<SQLT_TABLE>& rows)
     * @see SQLT::insert(const SQLT_TABLE& row)
     */
    template<typename SQLT_TABLE>
    inline int insert(sqlite3 *db, const std::vector<SQLT_TABLE>& rows)
    {
        int result;
        sqlite3_stmt *stmt;
        const std::string preparedStatement = SQLT::Internal::createInsertPreparedStatement<SQLT_TABLE>();

        result = sqlite3_prepare_v2(db, preparedStatement.c_str(), -1, &stmt, NULL);
        if (result != SQLITE_OK)
            return result;

        for (auto const& r : rows)
        {
            sqlite3_reset(stmt);
            result = SQLT::Internal::iterateAndBindValues<SQLT_TABLE>(r, stmt);
            if (result != SQLITE_OK)
                return result;

            result = sqlite3_step(stmt);
            if (result != SQLITE_DONE)
            {
                sqlite3_finalize(stmt);
                return result;
            }
        }

        result = sqlite3_finalize(stmt);
        return result;
    }

    /**
     * Insert a single row into a table.
     *
     * @tparam SQLT_TABLE An SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME.
     * @param db The sqlite3 instance to drop the table in.
     * @param row The row to insert into the database.
     * @return The SQLite error code. Will be SQLITE_OK if the row was successfully inserted.
     *
     * @see SQLT::insert(sqlite3 *db, const std::vector<SQLT_TABLE>& rows)
     * @see SQLT::insert(const std::vector<SQLT_TABLE>& rows)
     * @see SQLT::insert(const SQLT_TABLE& row)
     */
    template<typename SQLT_TABLE>
    inline int insert(sqlite3 *db, const SQLT_TABLE& row)
    {
        return SQLT::insert(db, std::vector<SQLT_TABLE>({row}));
    }

    /**
     * Insert rows into a table.
     *
     * @tparam SQLT_DB The database to insert into, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @tparam SQLT_TABLE An SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME.
     * @param rows The rows to insert into the database.
     * @return The SQLite error code. Will be SQLITE_OK if the rows were successfully inserted.
     *
     * @see SQLT::insert(sqlite3 *db, const std::vector<SQLT_TABLE>& rows)
     * @see SQLT::insert(sqlite3 *db, const SQLT_TABLE& row)
     * @see SQLT::insert(const SQLT_TABLE& row)
     */
    template<typename SQLT_DB, typename SQLT_TABLE>
    inline int insert(const std::vector<SQLT_TABLE>& rows)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.dbFilePath().c_str(), &db);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = SQLT::insert<SQLT_TABLE>(db, rows);

        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        return sqlite3_close(db);
    }

    /**
     * Insert a single row into a table.
     *
     * @tparam SQLT_DB The database to insert into, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @tparam SQLT_TABLE An SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME.
     * @param row The row to insert into the database.
     * @return The SQLite error code. Will be SQLITE_OK if the row was successfully inserted.
     *
     * @see SQLT::insert(sqlite3 *db, const std::vector<SQLT_TABLE>& rows)
     * @see SQLT::insert(sqlite3 *db, const SQLT_TABLE& row)
     * @see SQLT::insert(const std::vector<SQLT_TABLE>& rows)
     */
    template<typename SQLT_DB, typename SQLT_TABLE>
    inline int insert(const SQLT_TABLE& row)
    {
        return SQLT::insert<SQLT_DB, SQLT_TABLE>(std::vector<SQLT_TABLE>({row}));
    }

    /**
     * Select rows from a custom SQLite query into the corresponding query struct.
     *
     * @tparam SQLT_QUERY_STRUCT An SQLT query struct defined by SQLT_QUERY_RESULT_STRUCT.
     * @param db The sqlite3 instance to select the rows from.
     * @param selectQuery The SQLite SELECT query to perform to select rows into the output vector.
     * @param output The vector to save the results in. Will be resized to approximate_row_count in the process. Is expected to be empty, but the vector will not be cleared.
     * @param approximate_row_count Optional number for initial vector.reserve() call. Should be equal to the expected row count if possible (if such information is available), to avoid unneccesary allocations.
     * @return The SQLite error code. Will be SQLITE_OK if the rows were successfully selected.
     *
     * @see SQLT::select(const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
     * @see SQLT::selectAll(sqlite3 *db, std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
     * @see SQLT::selectAll(std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(sqlite3 *db, T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
     */
    template<typename SQLT_QUERY_STRUCT>
    inline int select(sqlite3 *db, const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
    {
        int result = SQLITE_ERROR;

        sqlite3_stmt *stmt;
        result = sqlite3_prepare_v2(db, selectQuery.c_str(), -1, &stmt, NULL);
        if (result != SQLITE_OK)
            return result;

        if (output->size() < approximate_row_count)
            output->reserve(approximate_row_count);

        SQLT_QUERY_STRUCT row;
        while (true)
        {
            result = sqlite3_step(stmt);
            if (result == SQLITE_ROW)
            {
                int count = sqlite3_column_count(stmt);

                if (count == 0)
                    continue;

                for (int colIndex = 0; colIndex < count; colIndex++)
                {
                    const char* const colName = sqlite3_column_name(stmt, colIndex);
                    Internal::iterateAndAssignMembersByColumnName(row, stmt, colName, colIndex);
                }

                output->emplace_back(row);
            }
            else if (result == SQLITE_DONE)
            {
                break;
            }
            else
            {
                sqlite3_finalize(stmt);
                return result;
            }
        }

        result = sqlite3_finalize(stmt);
        return result;
    }

    /**
     * Select rows from a custom SQLite query into the corresponding query struct.
     *
     * @tparam SQLT_QUERY_STRUCT An SQLT query struct defined by SQLT_QUERY_RESULT_STRUCT.
     * @tparam SQLT_DB The database to select from, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param selectQuery The SQLite SELECT query to perform to select rows into the output vector.
     * @param output The vector to save the results in. Will be resized to approximate_row_count in the process. Is expected to be empty, but the vector will not be cleared.
     * @param approximate_row_count Optional number for initial vector.reserve() call. Should be equal to the expected row count if possible (if such information is available), to avoid unneccesary allocations.
     * @return The SQLite error code. Will be SQLITE_OK if the rows were successfully selected.
     *
     * @see SQLT::select(sqlite3 *db, const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
     * @see SQLT::selectAll(sqlite3 *db, std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
     * @see SQLT::selectAll(std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(sqlite3 *db, T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
     */
    template<typename SQLT_DB, typename SQLT_QUERY_STRUCT>
    inline int select(const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.dbFilePath().c_str(), &db);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = SQLT::select<SQLT_QUERY_STRUCT>(db, selectQuery, output, approximate_row_count);

        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = sqlite3_close(db);
        return result;
    }

    /**
     * Select all rows from a table.
     *
     * @tparam SQLT_TABLE An SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME.
     * @param db The sqlite3 instance to select the rows from.
     * @param output The vector to save the results in. Will be resized to approximate_row_count in the process. Is expected to be empty, but the vector will not be cleared.
     * @param approximate_row_count Optional number for initial vector.reserve() call. Should be equal to or greater than the expected row count, if such information is available, to avoid unneccesary allocations.
     * @return The SQLite error code. Will be SQLITE_OK if the rows were successfully selected.
     *
     * @see SQLT::select(sqlite3 *db, const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
     * @see SQLT::selectAll(std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(sqlite3 *db, T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
     */
    template<typename SQLT_TABLE>
    inline int selectAll(sqlite3 *db, std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
    {
        int result;
        sqlite3_stmt *stmt;
        const std::string query = std::string("SELECT * FROM ") + SQLT::tableName<SQLT_TABLE>() + ";";
        result = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        if (output->size() < approximate_row_count)
            output->reserve(approximate_row_count);

        SQLT_TABLE row;
        while (true)
        {
            result = sqlite3_step(stmt);
            if (result == SQLITE_ROW)
            {
                SQLT::Internal::iterateAndAssignMembers(row, stmt);
                output->emplace_back(row);
            }
            else if (result == SQLITE_DONE)
            {
                break;
            }
            else
            {
                sqlite3_finalize(stmt);
                return result;
            }
        }

        result = sqlite3_finalize(stmt);
        return result;
    }

    /**
     * Select all rows from a table.
     *
     * @tparam SQLT_DB The database to insert into, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @tparam SQLT_TABLE An SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME.
     * @param output The vector to save the results in. Will be resized to approximate_row_count in the process. Is expected to be empty, but the vector will not be cleared.
     * @param approximate_row_count Optional number for initial vector.reserve() call. Should be equal to or greater than the expected row count, if such information is available, to avoid unneccesary allocations.
     * @return The SQLite error code. Will be SQLITE_OK if the rows were successfully selected.
     *
     * @see SQLT::select(sqlite3 *db, const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
     * @see SQLT::selectAll(sqlite3 *db, std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(sqlite3 *db, T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
     */
    template<typename SQLT_DB, typename SQLT_TABLE>
    inline int selectAll(std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.dbFilePath().c_str(), &db);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = SQLT::selectAll<SQLT_TABLE>(db, output, approximate_row_count);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        return sqlite3_close(db);
    }

    /**
     * Select all rows from a table for a given column (i.e. "SELECT member FROM SQLT_TABLE;").
     *
     * @tparam SQLT_DB The database to select from, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @tparam SQLT_TABLE The SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME to select from.
     * @param db The sqlite3 instance to select the rows from.
     * @param member Pointer to the member in the SQLT_TABLE struct to select.
     * @param output The vector to save the results in. Will be resized to approximate_row_count in the process. Is expected to be empty, but the vector will not be cleared.
     * @param approximate_row_count Optional number for initial vector.reserve() call. Should be equal to or greater than the expected row count, if such information is available, to avoid unneccesary allocations.
     * @return The SQLite error code. Will be SQLITE_OK if the rows were successfully selected.
     *
     * @see SQLT::select(sqlite3 *db, const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
     * @see SQLT::selectAll(sqlite3 *db, std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
     * @see SQLT::selectAll(std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
     */
    template<typename SQLT_DB, typename SQLT_TABLE, typename T>
    inline int select(sqlite3 *db, T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
    {
        int result = SQLITE_ERROR;

        if (output->size() < approximate_row_count)
            output->reserve(approximate_row_count);

        std::string colName = SQLT::Internal::getColumnName<SQLT_TABLE>(member);
        if (colName.size())
        {
            std::string tableName = SQLT::tableName<SQLT_TABLE>();
            std::string query = "SELECT " + colName + " FROM " + tableName + ";";

            sqlite3_stmt *stmt;
            result = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
            if (result == SQLITE_OK)
            {
                T selectedValue;
                while (true)
                {
                    result = sqlite3_step(stmt);
                    if (result == SQLITE_ROW)
                    {
                        SQLT::Internal::assignValue(selectedValue, stmt);
                        output->emplace_back(selectedValue);
                    }
                    else if (result == SQLITE_DONE)
                    {
                        break;
                    }
                    else
                    {
                        sqlite3_finalize(stmt);
                        return result;
                    }
                }
            }
            else
            {
                result = sqlite3_finalize(stmt);
                return result;
            }

            result = sqlite3_finalize(stmt);
        }

        return result;
    }

    /**
     * Select all rows from a table for a given column (i.e. "SELECT member FROM SQLT_TABLE;").
     *
     * @tparam SQLT_DB The database to select from, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @tparam SQLT_TABLE The SQLT table struct defined by SQLT_TABLE or SQLT_TABLE_WITH_NAME to select from.
     * @param member Pointer to the member in the SQLT_TABLE struct to select.
     * @param output The vector to save the results in. Will be resized to approximate_row_count in the process. Is expected to be empty, but the vector will not be cleared.
     * @param approximate_row_count Optional number for initial vector.reserve() call. Should be equal to or greater than the expected row count, if such information is available, to avoid unneccesary allocations.
     * @return The SQLite error code. Will be SQLITE_OK if the rows were successfully selected.
     *
     * @see SQLT::select(sqlite3 *db, const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(const std::string& selectQuery, std::vector<SQLT_QUERY_STRUCT> *output, size_t approximate_row_count = 50)
     * @see SQLT::selectAll(sqlite3 *db, std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
     * @see SQLT::selectAll(std::vector<SQLT_TABLE> *output, size_t approximate_row_count = 50)
     * @see SQLT::select(sqlite3 *db, T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
     */
    template<typename SQLT_DB, typename SQLT_TABLE, typename T>
    inline int select(T SQLT_TABLE::* member, std::vector<T> *output, size_t approximate_row_count = 50)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.dbFilePath().c_str(), &db);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = SQLT::select<SQLT_DB>(db, member, output);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        return sqlite3_close(db);
    }

    /**
     * Update rows using a custom SQLite query.
     *
     * @param db The sqlite3 instance to execute the update query on.
     * @param query The SQLite update query to perform. The query is expected to be an UPDATE query (i.e. no results are returned)..
     * @return The SQLite error code. Will be SQLITE_OK if the rows were successfully selected.
     *
     * @see SQLT::seupdate(const std::string& query)
     */
    inline int update(sqlite3 *db, const std::string& query)
    {
        int result = SQLITE_ERROR;

        sqlite3_stmt *stmt;
        result = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
        if (result != SQLITE_OK)
            return result;

        result = sqlite3_step(stmt);
        if (result != SQLITE_DONE)
        {
            sqlite3_finalize(stmt);
            return result;
        }

        result = sqlite3_finalize(stmt);
        return result;
    }

    /**
     * Update rows using a custom SQLite query.
     *
     * @tparam SQLT_DB The database to select from, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param query The SQLite update query to perform. The query is expected to be an UPDATE query (i.e. no results are returned)..
     * @return The SQLite error code. Will be SQLITE_OK if the rows were successfully selected.
     *
     * @see SQLT::update(sqlite3 *db, const std::string& query)
     */
    template<typename SQLT_DB>
    inline int update(const std::string& query)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.dbFilePath().c_str(), &db);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = SQLT::update(db, query);

        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = sqlite3_close(db);
        return result;
    }

    /**
     * SQLT Internal namespace. Should normally not be referenced externally.
     */
    namespace Internal
    {
        template<typename SQLT_TABLE>
        struct TableInfo
        {
            int insert(sqlite3 *db, const std::vector<SQLT_TABLE>& rows) const
            {
                return SQLT::insert(db, rows);
            }

            int createTableIfNotExists(sqlite3 *db, char **errMsg) const
            {
                return SQLT::createTableIfNotExists<SQLT_TABLE>(db, errMsg);
            }

            int dropTableIfExists(sqlite3 *db, char **errMsg) const
            {
                return SQLT::dropTableIfExists<SQLT_TABLE>(db, errMsg);
            }
        };

        template<typename SQLT_TABLE>
        constexpr const TableInfo<SQLT_TABLE> makeTableInfo()
        {
            return TableInfo<SQLT_TABLE>();
        }

        template<typename SQLT_DB>
        struct DatabaseInfo
        {
            DatabaseName name;
            DatabasePath path;
            static std::string overrideDatabasePath; // Used to override the database path on runtime if needed.

            std::string dbFilePath() const
            {
                return DatabaseInfo::overrideDatabasePath.size() ? DatabaseInfo::overrideDatabasePath : (path.toString() + name.toString());
            }
        };

        template<typename SQLT_DB>
        std::string DatabaseInfo<SQLT_DB>::overrideDatabasePath;

        template<typename SQLT_DATABASE, size_t NAME_SIZE, size_t PATH_SIZE>
        constexpr const DatabaseInfo<SQLT_DATABASE> makeDatabaseInfo(const char(&name)[NAME_SIZE], const char(&path)[PATH_SIZE])
        {
            return { DatabaseName(name), DatabasePath(path) };
        }

        template<size_t INDEX, size_t SIZE, typename TABLE_TUPLE>
        struct SQLiteTableTraverser
        {
            static inline int iterateAndCreateTables(const TABLE_TUPLE& tables, sqlite3 *db, char **errMsg)
            {
                int result;
                auto& tableInfo = tables.template get<INDEX>();
                result = tableInfo.createTableIfNotExists(db, errMsg);
                if (result != SQLITE_OK)
                    return result;
                return SQLiteTableTraverser<INDEX + 1, SIZE, TABLE_TUPLE>::iterateAndCreateTables(tables, db, errMsg);
            }

            static inline int iterateAndDropTables(const TABLE_TUPLE& tables, sqlite3 *db, char **errMsg)
            {
                int result;
                auto& tableInfo = tables.template get<INDEX>();
                result = tableInfo.dropTableIfExists(db, errMsg);
                if (result != SQLITE_OK)
                    return result;
                return SQLiteTableTraverser<INDEX + 1, SIZE, TABLE_TUPLE>::iterateAndDropTables(tables, db, errMsg);
            }
        };

        template<size_t SIZE, typename TABLE_TUPLE>
        struct SQLiteTableTraverser<SIZE, SIZE, TABLE_TUPLE>
        {
            static inline int iterateAndCreateTables(const TABLE_TUPLE& tables, sqlite3 *db, char **errMsg)
            {
                auto& tableInfo = tables.template get<SIZE>();
                return tableInfo.createTableIfNotExists(db, errMsg);
            }

            static inline int iterateAndDropTables(const TABLE_TUPLE& tables, sqlite3 *db, char **errMsg)
            {
                auto& tableInfo = tables.template get<SIZE>();
                return tableInfo.dropTableIfExists(db, errMsg);
            }
        };
    } // End namespace Internal

#define SQLT_COLUMN(member) SQLT::Internal::makeColumnInfo(#member, &SQLT_STRUCT_T::member, SQLT::Flags::NONE)
#define SQLT_COLUMN_DEFAULT(member, default) SQLT::Internal::makeColumnInfo(#member, &SQLT_STRUCT_T::member, default, SQLT::Flags::DEFAULT)
#define SQLT_COLUMN_PRIMARY_KEY(member) SQLT::Internal::makeColumnInfo(#member, &SQLT_STRUCT_T::member, SQLT::Flags::PRIMARY_KEY)
#define SQLT_COLUMN_PRIMARY_KEY_DEFAULT(member, default) SQLT::Internal::makeColumnInfo(#member, &SQLT_STRUCT_T::member, default, SQLT::Flags::PRIMARY_KEY | SQLT::Flags::DEFAULT)
#define SQLT_COLUMN_FLAGS(member, flags) SQLT::Internal::makeColumnInfo(#member, &SQLT_STRUCT_T::member, flags)
#define SQLT_COLUMN_FLAGS_DEFAULT(member, default, flags) SQLT::Internal::makeColumnInfo(#member, &SQLT_STRUCT_T::member, default, flags | SQLT::Flags::DEFAULT)

#define SQLT_TABLE_WITH_NAME(SQLT_TABLE_STRUCT, table_name, ...) \
    template<typename SQLT_STRUCT_T> \
    struct SQLTBase \
    { \
        static const SQLT::Internal::TableName &sqlt_static_table_name() \
        { \
            static auto ret = SQLT::Internal::makeTableName(table_name); \
            return ret; \
        } \
        using CT = decltype(SQLT::Internal::makeTuple(__VA_ARGS__)); \
        static const CT &sqlt_static_column_info() \
        { \
            static auto ret = SQLT::Internal::makeTuple(__VA_ARGS__); \
            return ret; \
        } \
    };

#define SQLT_TABLE(SQLT_TABLE_STRUCT, ...) \
    SQLT_TABLE_WITH_NAME(SQLT_TABLE_STRUCT, #SQLT_TABLE_STRUCT, __VA_ARGS__)

#define SQLT_QUERY_RESULT_MEMBER(member) \
    SQLT::Internal::makeColumnInfo(#member, &SQLT_STRUCT_T::member, SQLT::Flags::NONE)

#define SQLT_QUERY_RESULT_STRUCT(SQLT_TABLE_STRUCT, ...) \
    template<typename SQLT_STRUCT_T> \
    struct SQLTBase \
    { \
        using CT = decltype(SQLT::Internal::makeTuple(__VA_ARGS__)); \
        static const CT &sqlt_static_column_info() \
        { \
            static auto ret = SQLT::Internal::makeTuple(__VA_ARGS__); \
            return ret; \
        } \
    };

    /**
     * Open the database.
     *
     * @tparam SQLT_DB The database to open, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param db The sqlite3 instance to open the dtabase for.
     */
    template<typename SQLT_DB>
    inline int open(sqlite3 **db)
    {
        int result = SQLITE_OK;
        auto dbInfo = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_database_info();
        result = sqlite3_open(dbInfo.dbFilePath().c_str(), db);
        if (result != SQLITE_OK)
            sqlite3_close(*db);
        return result;
    }

    /**
     * Close the database.
     *
     * @tparam SQLT_DB The database to close, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param db The sqlite3 instance to close the database for.
     */
    template<typename SQLT_DB>
    inline int close(sqlite3 *db)
    {
        return sqlite3_close(db);
    }

    /**
     * Begin a transaction on the argument database.
     *
     * @tparam SQLT_DB The database to start the transaction for, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param db The sqlite3 instance to start the transaction for.
     */
    template<typename SQLT_DB>
    inline int begin(sqlite3 *db)
    {
		return sqlite3_exec(db, "BEGIN", 0, 0, 0);
    }

    /**
     * Rollback a transaction on the argument database.
     *
     * @tparam SQLT_DB The database to rollback the transaction for, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param db The sqlite3 instance to rollback the transaction for.
     */
    template<typename SQLT_DB>
    inline int rollback(sqlite3 *db)
    {
        return sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
    }

    /**
     * Commit a transaction on the argument database.
     *
     * @tparam SQLT_DB The database to commit the transaction for, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param db The sqlite3 instance to commit the transaction for.
     */
    template<typename SQLT_DB>
    inline int commit(sqlite3 *db)
    {
        return sqlite3_exec(db, "COMMIT", 0, 0, 0);
    }

    /**
     * Explicitly set the path to the SQLite database file on runtime instead of using the one defined on compile time with the macros
     * SQLT_DATABASE, SQLT_DATABASE_WITH_NAME and SQLT_DATABASE_WITH_NAME_AND_PATH.
     *
     * @tparam SQLT_DB The database to insert into, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param dbPath The absolute path to the SQLite database file to read from and write to.
     */
    template<typename SQLT_DB>
    inline void setDatabasePath(const std::string& dbPath)
    {
        auto dbInfo = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_database_info();
        dbInfo.overrideDatabasePath = dbPath;
    }

    /**
     * Create all tables in a database if they do not exist.
     *
     * @tparam SQLT_DB The database to insert into, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param db The sqlite3 instance to create the tables in.
     * @param errMsg The sqlite3 message output.
     * @return The SQLite error code. Will be SQLITE_OK if the tables were successfully created.
     *
     * @see SQLT::createAllTables(char **errMsg)
     */
    template<typename SQLT_DB>
    inline int createAllTables(sqlite3 *db, char **errMsg)
    {
        auto tables = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_table_info();
        return SQLT::Internal::SQLiteTableTraverser<0, decltype(tables)::size - 1, decltype(tables)>::iterateAndCreateTables(tables, db, errMsg);
    }

    /**
     * Create all tables in a database if they do not exist.
     *
     * @tparam SQLT_DB The database to insert into, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param errMsg The sqlite3 message output.
     * @return The SQLite error code. Will be SQLITE_OK if the tables were successfully created.
     *
     * @see SQLT::createAllTables(sqlite3 *db, char **errMsg)
     */
    template<typename SQLT_DB>
    inline int createAllTables(char **errMsg)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.dbFilePath().c_str(), &db);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = SQLT::createAllTables<SQLT_DB>(db, errMsg);

        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        return sqlite3_close(db);
    }

    /**
     * Drop all tables in a database.
     *
     * @tparam SQLT_DB The database to insert into, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param db The sqlite3 instance to drop the tables in.
     * @param errMsg The sqlite3 message output.
     * @return The SQLite error code. Will be SQLITE_OK if the tables were successfully dropped.
     *
     * @see SQLT::dropAllTables(char **errMsg)
     */
    template<typename SQLT_DB>
    inline int dropAllTables(sqlite3 *db, char **errMsg)
    {
        auto tables = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_table_info();
        return SQLT::Internal::SQLiteTableTraverser<0, decltype(tables)::size - 1, decltype(tables)>::iterateAndDropTables(tables, db, errMsg);
    }

    /**
     * Drop all tables in a database.
     *
     * @tparam SQLT_DB The database to insert into, defined by SQLT_DATABASE, SQLT_DATABASE_WITH_NAME or SQLT_DATABASE_WITH_NAME_AND_PATH.
     * @param errMsg The sqlite3 message output.
     * @return The SQLite error code. Will be SQLITE_OK if the tables were successfully dropped.
     *
     * @see SQLT::dropAllTables(sqlite3 *db, char **errMsg)
     */
    template<typename SQLT_DB>
    inline int dropAllTables(char **errMsg)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = SQLT_DB::template SQLTDatabase<SQLT_DB>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.dbFilePath().c_str(), &db);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = SQLT::dropAllTables<SQLT_DB>(db, errMsg);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        return sqlite3_close(db);
    }

#define SQLT_DATABASE_TABLE(database_table) SQLT::Internal::makeTableInfo<database_table>()

#define SQLT_DATABASE_WITH_NAME_AND_PATH(database_struct, database_name, database_path, ...) \
    template<typename SQLT_DATABASE_T> \
    struct SQLTDatabase \
    { \
        static const SQLT::Internal::DatabaseInfo<database_struct> &sqlt_static_database_info() \
        { \
            static auto ret = SQLT::Internal::makeDatabaseInfo<database_struct>(database_name, database_path); \
            return ret; \
        } \
        using DT = decltype(SQLT::Internal::makeTuple(__VA_ARGS__)); \
        static const DT &sqlt_static_table_info() \
        { \
            static auto ret = SQLT::Internal::makeTuple(__VA_ARGS__); \
            return ret; \
        } \
    };

#define SQLT_DATABASE_WITH_NAME(database_struct, database_name, ...) \
    SQLT_DATABASE_WITH_NAME_AND_PATH(database_struct, database_name, "", __VA_ARGS__)

#define SQLT_DATABASE(database_struct, ...) \
    SQLT_DATABASE_WITH_NAME_AND_PATH(database_struct, #database_struct, "", __VA_ARGS__)

}
// END SQLT NAMESPACE

// Note: json_tools.h must be included elsewhere in the application when using SQLITE_TOOLS_USE_JSON_TOOLS
#if defined(SQLITE_TOOLS_USE_JSON_TOOLS)
namespace JT
{
    template<typename T>
    struct TypeHandler<SQLT::Nullable<T>>
    {
        static inline Error unpackToken(SQLT::Nullable<T>& nullable, ParseContext &context)
        {
            if (context.token.value_type == JT::Type::Null)
            {
                nullable.is_null = true;
                return Error::NoError;
            }

            nullable.is_null = false;
            return TypeHandler<T>::unpackToken(nullable.value, context);
        }

        static inline void serializeToken(const SQLT::Nullable<T> nullable, Token &token, Serializer &serializer)
        {
            if (nullable.is_null)
            {
                token.value_type = JT::Type::Null;
                serializer.write(token);
            }
            else
            {
                JT::TypeHandler<T>::serializeToken(nullable.value, token, serializer);
            }
        }
    };
}
#endif
