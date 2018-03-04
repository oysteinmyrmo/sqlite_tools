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
    // Tuple, stolen from https://github.com/jorgen/json_tools <-- json_tools is awesome, go give it some love!
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
    }

    template<size_t I, typename ...Ts>
    struct TypeAt
    {
        template<typename T>
        static Internal::Element<I, T> deduce(Internal::Element<I, T>);

        using tuple_impl = Internal::TupleImpl<typename Internal::GenSequence<sizeof...(Ts)>::type, Ts...>;
        using element = decltype(deduce(tuple_impl()));
        using type = typename element::type;
    };

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
    // Tuple end

    enum class Flags : uint8_t
    {
        NONE            =   0x00,
        PRIMARY_KEY     =   0x01,
        DEFAULT         =   0x02,
        NOT_NULL        =   0x04 // Should not be set explicitly, but is set internally based on whether or not a member is of SQLT::Nullable<T> type.
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

        const char *data; // Not null-terminated
        size_t size;
    };

    typedef ColName TableName;
    typedef ColName DatabaseName;
    typedef ColName DatabasePath;

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

        Nullable(const T& val, bool is_null)
            : value(value)
            , is_null(is_null)
        {}

        T value;
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
    bool isPrimaryKey() const { return flagSet(flags, Flags::PRIMARY_KEY); } \
    bool isNotNull() const { return flagSet(flags, Flags::NOT_NULL); } \
    bool hasDefaultValue() const { static_assert(!flagSet(flags, Flags::DEFAULT), "Cannot use Flags::DEFAULT in a column that has no default value."); return false; } \
    std::string DATATYPE() const \
    { \
        std::string dtype = name.toString() + " " + std::string(datatype()); \
        if (isNotNull()) \
            dtype += " NOT NULL"; \
        return dtype; \
    }

#define SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE \
    bool isPrimaryKey() const { return flagSet(flags, Flags::PRIMARY_KEY); } \
    bool isNotNull() const { return flagSet(flags, Flags::NOT_NULL); } \
    bool hasDefaultValue() const { static_assert(flagSet(flags, Flags::DEFAULT), "Must use Flags::DEFAULT in a column that has a default value."); return true; } \
    std::string DATATYPE() const \
    { \
        std::string dtype = name.toString() + " " + std::string(datatype()); \
        if (isNotNull()) \
            dtype += " NOT NULL"; \
        dtype += " DEFAULT " + defaultValueString(); \
        return dtype; \
    }

    template<typename T, typename U>
    struct ColInfo
    {
        ColName name;
        T U::* member;
        Flags flags;
        typedef T type;
    };

    template<typename U>
    struct ColInfo<int, U>
    {
        ColName name;
        int U::* member;
        Flags flags;
        typedef int type;

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

        SQLT_COL_INFO_DEFAULT_FUNCTIONS
        SQLT_COL_INFO_TEXT_FUNCTIONS
    };

    template<typename U>
    struct ColInfo<Nullable<int>, U>
    {
        ColName name;
        Nullable<int> U::* member;
        Flags flags;
        typedef Nullable<int> type;

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

        SQLT_COL_INFO_DEFAULT_FUNCTIONS
        SQLT_COL_INFO_TEXT_FUNCTIONS
    };

    template<typename T, typename U>
    struct ColInfoWithDefault
    {
        ColName name;
        T U::* member;
        T defaultValue;
        Flags flags;
        typedef T type;
    };

    template<typename U>
    struct ColInfoWithDefault<int, U>
    {
        ColName name;
        int U::* member;
        int defaultValue;
        Flags flags;
        typedef int type;

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

        std::string defaultValueString() const { return std::string("\"") + defaultValue + "\""; }
        SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE
        SQLT_COL_INFO_TEXT_FUNCTIONS
    };

    template<typename U>
    struct ColInfoWithDefault<Nullable<int>, U>
    {
        ColName name;
        Nullable<int> U::* member;
        int defaultValue;
        Flags flags;
        typedef Nullable<int> type;

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

        std::string defaultValueString() const { return std::string("\"") + defaultValue + "\""; }
        SQLT_COL_INFO_DEFAULT_FUNCTIONS_WITH_DEFAULT_VALUE
        SQLT_COL_INFO_TEXT_FUNCTIONS
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

    template<typename COL_TUPLE>
    inline size_t primaryKeyCount()
    {
        auto columns = COL_TUPLE::template SQLTBase<COL_TUPLE>::sqlt_static_column_info();
        return SQLT::ColumnTraverser_PrimaryKeyCount<decltype(columns)::size - 1, decltype(columns)>::traverse(columns);
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

    template<typename COL_TUPLE>
    inline std::string createPrimaryKeyStatement()
    {
        std::string statement("");
        size_t pkCount = primaryKeyCount<COL_TUPLE>();
        if (pkCount > 0)
        {
            statement += "PRIMARY KEY(";
            size_t usedPks = 0;
            auto columns = COL_TUPLE::template SQLTBase<COL_TUPLE>::sqlt_static_column_info();
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
        SQLT::ColumnTraverser_CreateTable<0, decltype(columns)::size - 1, decltype(columns)>::traverse(columns, query);
        return query;
    }

    template<typename SQLT_TABLE>
    inline std::string tableName()
    {
        auto tableName = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_table_name();
        return tableName.toString();
    }

    template<typename SQLT_TABLE>
    inline int createTableIfNotExists(sqlite3 *db, char **errMsg)
    {
        std::string query = std::string("CREATE TABLE IF NOT EXISTS ") + tableName<SQLT_TABLE>();
        query += "(";
        query += SQLT::createTableContents<SQLT_TABLE>();
        size_t pkCount = SQLT::primaryKeyCount<SQLT_TABLE>();
        if (pkCount > 0)
        {
            query += ",";
            query += SQLT::createPrimaryKeyStatement<SQLT_TABLE>();
        }
        query += ");";
        return sqlite3_exec(db, query.c_str(), NULL, NULL, errMsg);
    }

    template<typename SQLT_TABLE>
    inline int dropTableIfExists(sqlite3 *db, char **errMsg)
    {
        std::string stmt = std::string("DROP TABLE IF EXISTS ") + tableName<SQLT_TABLE>() + ";";
        return sqlite3_exec(db, stmt.c_str(), NULL, NULL, errMsg);
    }

#define SQLT_COLUMN(member) SQLT::makeColumnInfo(#member, &SQLT_STRUCT_T::member, SQLT::Flags::NONE)
#define SQLT_COLUMN_DEFAULT(member, default) SQLT::makeColumnInfo(#member, &SQLT_STRUCT_T::member, default, SQLT::Flags::DEFAULT)
#define SQLT_COLUMN_PRIMARY_KEY(member) SQLT::makeColumnInfo(#member, &SQLT_STRUCT_T::member, SQLT::Flags::PRIMARY_KEY)
#define SQLT_COLUMN_PRIMARY_KEY_DEFAULT(member, default) SQLT::makeColumnInfo(#member, &SQLT_STRUCT_T::member, default, SQLT::Flags::PRIMARY_KEY | SQLT::Flags::DEFAULT)
#define SQLT_COLUMN_FLAGS(member, flags) SQLT::makeColumnInfo(#member, &SQLT_STRUCT_T::member, flags)
#define SQLT_COLUMN_FLAGS_DEFAULT(member, default, flags) SQLT::makeColumnInfo(#member, &SQLT_STRUCT_T::member, default, flags | SQLT::Flags::DEFAULT)

#define SQLT_TABLE_WITH_NAME(SQLT_TABLE_STRUCT, table_name, ...) \
    template<typename SQLT_STRUCT_T> \
    struct SQLTBase \
    { \
        static const SQLT::TableName &sqlt_static_table_name() \
        { \
            static auto ret = SQLT::makeTableName(table_name); \
            return ret; \
        } \
        using CT = decltype(SQLT::makeTuple(__VA_ARGS__)); \
        static const CT &sqlt_static_column_info() \
        { \
            static auto ret = SQLT::makeTuple(__VA_ARGS__); \
            return ret; \
        } \
    };

#define SQLT_TABLE(SQLT_TABLE_STRUCT, ...) \
    SQLT_TABLE_WITH_NAME(SQLT_TABLE_STRUCT, #SQLT_TABLE_STRUCT, __VA_ARGS__)

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
            return 1 + SQLT::ColumnTraverser<INDEX + 1, SIZE, COL_TUPLE>::columnCount(columns);
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
        SQLT::ColumnTraverser<0, decltype(columns)::size - 1, decltype(columns)>::createColumnNameList(columns, query);
        return query;
    }

    template<typename SQLT_TABLE>
    inline std::string createColumnNameQuestionMarkList()
    {
        std::string query;
        auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
        SQLT::ColumnTraverser<0, decltype(columns)::size - 1, decltype(columns)>::createColumnNameQuestionMarkList(columns, query);
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
        return SQLT::ColumnTraverser<0, decltype(columns)::size - 1, decltype(columns)>::columnCount(columns);
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
            return sqlite3_bind_text(stmt, index, value.c_str(), value.length(), SQLITE_STATIC);
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
            return value.is_null ? sqlite3_bind_null(stmt, index) : sqlite3_bind_text(stmt, index, value.value.c_str(), value.value.length(), SQLITE_STATIC);
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
        static inline int assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, T U::* memberPtr)
        {
            return SQLITE_ERROR;
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

    template<typename U, typename SQLT_TABLE>
    struct SQLiteMemberAssigner<SQLT::Nullable<int>, U, SQLT_TABLE>
    {
        static inline void assignMember(sqlite3_stmt *stmt, int index, SQLT_TABLE& row, SQLT::Nullable<int> U::* memberPtr)
        {
            int dataType = sqlite3_column_type(stmt, index);
            if (dataType == SQLITE_NULL)
            {
                (row.*memberPtr).is_null = true;
                return;
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
                return;
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
                return;
            }
            else
            {
                assert(dataType == SQLITE_TEXT);
                (row.*memberPtr).is_null = false;
                (row.*memberPtr).value = std::string((const char*)sqlite3_column_text(stmt, index));
            }
        }
    };

    template<typename T, typename U, typename SQLT_TABLE>
    inline void assignMember(const ColInfo<T, U>& colInfo, SQLT_TABLE& row, sqlite3_stmt * stmt, int index)
    {
        SQLiteMemberAssigner<T, U, SQLT_TABLE>::assignMember(stmt, index, row, colInfo.member);
    }

    template<typename T, typename U, typename SQLT_TABLE>
    inline void assignMember(const ColInfoWithDefault<T, U>& colInfo, SQLT_TABLE& row, sqlite3_stmt * stmt, int index)
    {
        SQLiteMemberAssigner<T, U, SQLT_TABLE>::assignMember(stmt, index, row, colInfo.member);
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
    };

    template<typename SQLT_TABLE>
    inline int iterateAndBindValues(SQLT_TABLE row, sqlite3_stmt *stmt)
    {
        auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
        return SQLT::SQLiteColumnTraverser<0, decltype(columns)::size - 1, decltype(columns), SQLT_TABLE>::iterateAndBindValues(columns, row, stmt);
    }

    template<typename SQLT_TABLE>
    inline void iterateAndAssignMembers(SQLT_TABLE& row, sqlite3_stmt *stmt)
    {
        auto columns = SQLT_TABLE::template SQLTBase<SQLT_TABLE>::sqlt_static_column_info();
        SQLT::SQLiteColumnTraverser<0, decltype(columns)::size - 1, decltype(columns), SQLT_TABLE>::iterateAndAssignMembers(columns, row, stmt);
    }

    template<typename SQLT_TABLE>
    inline int insert(sqlite3 *db, const std::vector<SQLT_TABLE>& rows)
    {
        int result;
        sqlite3_stmt *stmt;
        const std::string preparedStatement = SQLT::createInsertPreparedStatement<SQLT_TABLE>();

        result = sqlite3_prepare(db, preparedStatement.c_str(), -1, &stmt, NULL);
        if (result != SQLITE_OK)
            return result;

        for (auto const& r : rows)
        {
            sqlite3_reset(stmt);
            result = SQLT::iterateAndBindValues<SQLT_TABLE>(r, stmt);
            if (result != SQLITE_OK)
                return result;

            result = sqlite3_step(stmt);
            if (result != SQLITE_DONE)
                break;
        }

        result = sqlite3_finalize(stmt);
        return result;
    }

    template<typename DB_STRUCT, typename SQLT_TABLE>
    inline int insert(const std::vector<SQLT_TABLE>& rows)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = DB_STRUCT::template SQLTDatabase<DB_STRUCT>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.name.toString().c_str(), &db);
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

    template<typename SQLT_TABLE>
    inline int selectAll(sqlite3 *db, std::vector<SQLT_TABLE> *output)
    {
        int result;
        sqlite3_stmt *stmt;
        const std::string query = std::string("SELECT * FROM ") + SQLT::tableName<SQLT_TABLE>() + ";";
        result = sqlite3_prepare(db, query.c_str(), -1, &stmt, NULL);
        if (result != SQLITE_OK)
            return result;

        output->reserve(50);
        SQLT_TABLE row;
        while (true)
        {
            result = sqlite3_step(stmt);
            if (result == SQLITE_ROW)
            {
                SQLT::iterateAndAssignMembers(row, stmt);
                output->push_back(row);
            }
            else
            {
                break;
            }
        }

        result = sqlite3_finalize(stmt);
        return result;
    }

    template<typename DB_STRUCT, typename SQLT_TABLE>
    inline int selectAll(std::vector<SQLT_TABLE> *rows)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = DB_STRUCT::template SQLTDatabase<DB_STRUCT>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.name.toString().c_str(), &db);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = SQLT::selectAll<SQLT_TABLE>(db, rows);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        return sqlite3_close(db);
    }

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

    // Table info creation.
    template<typename SQLT_TABLE>
    constexpr const TableInfo<SQLT_TABLE> makeTableInfo()
    {
        return TableInfo<SQLT_TABLE>();
    }

    template<typename SQLT_DATABASE>
    struct DatabaseInfo
    {
        DatabaseName name;
        DatabasePath path;
    };

    // Database info creation
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

    template<typename DB_STRUCT>
    inline int createAllTables(sqlite3 *db, char **errMsg)
    {
        auto tables = DB_STRUCT::template SQLTDatabase<DB_STRUCT>::sqlt_static_table_info();
        return SQLiteTableTraverser<0, decltype(tables)::size - 1, decltype(tables)>::iterateAndCreateTables(tables, db, errMsg);
    }

    template<typename DB_STRUCT>
    inline int createAllTables(char **errMsg)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = DB_STRUCT::template SQLTDatabase<DB_STRUCT>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.name.toString().c_str(), &db);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = SQLT::createAllTables<DB_STRUCT>(db, errMsg);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        return sqlite3_close(db);
    }

    template<typename TABLE_TUPLE>
    inline int dropAllTables(sqlite3 *db, char **errMsg)
    {
        auto tables = TABLE_TUPLE::template SQLTDatabase<TABLE_TUPLE>::sqlt_static_table_info();
        return SQLiteTableTraverser<0, decltype(tables)::size - 1, decltype(tables)>::iterateAndDropTables(tables, db, errMsg);
    }

    template<typename DB_STRUCT>
    inline int dropAllTables(char **errMsg)
    {
        int result;
        sqlite3 *db;
        auto dbInfo = DB_STRUCT::template SQLTDatabase<DB_STRUCT>::sqlt_static_database_info();

        result = sqlite3_open(dbInfo.name.toString().c_str(), &db);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        result = SQLT::dropAllTables<DB_STRUCT>(db, errMsg);
        if (result != SQLITE_OK)
        {
            sqlite3_close(db);
            return result;
        }

        return sqlite3_close(db);
    }

#define SQLT_DATABASE_TABLE(database_table) SQLT::makeTableInfo<database_table>()

#define SQLT_DATABASE_WITH_NAME_AND_PATH(database_struct, database_name, database_path, ...) \
    template<typename SQLT_DATABASE_T> \
    struct SQLTDatabase \
    { \
        static const SQLT::DatabaseInfo<database_struct> &sqlt_static_database_info() \
        { \
            static auto ret = SQLT::makeDatabaseInfo<database_struct>(database_name, database_path); \
            return ret; \
        } \
        using DT = decltype(SQLT::makeTuple(__VA_ARGS__)); \
        static const DT &sqlt_static_table_info() \
        { \
            static auto ret = SQLT::makeTuple(__VA_ARGS__); \
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
        {}
    };
}
#endif
