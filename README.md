# SQLite Tools

SQLite Tools is a single header only C++11 library to create SQLite databases and insert from and select back to C++ structs. The tests are showcasing more functionality than this README does.

## Introduction

Since code speaks better than words, here is an example that statically defines an SQLite database with three tables. The tables are created, data is inserted and selected back into `std::vector`s thereafter. This example is show the same as the test named `readme-test1`.

```c++
#include <sqlite3/sqlite3.h>
#include <sqlite_tools.h>

// Statically define the database with three tables.
struct Database
{
    // Statically define a table with four columns.
    struct SomeTable
    {
        int id;
        std::string name;
        double value;
        SQLT::Nullable<std::string> description;
        SQLT::Nullable<bool> enabled;

        SQLT_TABLE(SomeTable,                   // SQLite table name = "SomeTable"
            SQLT_COLUMN_PRIMARY_KEY(id),        // INTEGER PRIMARY KEY NOT NULL
            SQLT_COLUMN(name),                  // TEXT NOT NULL
            SQLT_COLUMN(value),                 // REAL NOT NULL
            SQLT_COLUMN(description),           // TEXT
            SQLT_COLUMN_DEFAULT(enabled, true)  // INTEGER DEFAULT 1
        );
    };

    // Statically define a table resolving a many-to-many relationship.
    struct ManyToManyTable
    {
        int sometable_id;
        int someothertable_id;

        SQLT_TABLE_WITH_NAME(ManyToManyTable, "mtm_resolver",  // SQLite table name = "mtm_resolver"
            SQLT_COLUMN_PRIMARY_KEY(sometable_id),             // INTEGER NOT NULL
            SQLT_COLUMN_PRIMARY_KEY(someothertable_id)         // INTEGER NOT NULL
                                                               // PRIMARY_KEY(sometable_id, someothertable_id)
        );
    };

    // Statically define another table with three columns.
    struct SomeOtherTable
    {
        int id;
        std::string name;
        SQLT::Nullable<double> amount;

        SQLT_TABLE(SomeOtherTable,        // SQLite table name = "SomeOtherTable"
            SQLT_COLUMN_PRIMARY_KEY(id),  // INTEGER PRIMARY KEY NOT NULL
            SQLT_COLUMN(name),            // TEXT NOT NULL
            SQLT_COLUMN(amount)           // REAL
        );
    };

    SQLT_DATABASE(Database,                    // SQlite database name = "Database"
        SQLT_DATABASE_TABLE(SomeTable),        // Define the table "SomeTable" in the database "Database"
        SQLT_DATABASE_TABLE(ManyToManyTable),  // Define the table "ManyToManyTable" in the database "Database"
        SQLT_DATABASE_TABLE(SomeOtherTable)    // Define the table "SomeOtherTable" in the database "Database"
    );

    // Define a struct that represents the output of a custom SQLite query.
    struct QueryOutput
    {
        int sometable_id;
        std::string sometable_name;
        SQLT::Nullable<bool> sometable_enabled;
        int someothertable_id;
        std::string someothertable_name;
        SQLT::Nullable<double> someothertable_amount;

        SQLT_QUERY_RESULT_STRUCT(QueryOutput,                // Query struct name = "QueryOutput"
            SQLT_QUERY_RESULT_MEMBER(sometable_id),          // Add INTEGER NOT NULL mapped to output column named "sometable_id" in SQLite
            SQLT_QUERY_RESULT_MEMBER(sometable_name),        // Add TEXT NOT NULL mapped to output column named "sometable_name" in SQLite
            SQLT_QUERY_RESULT_MEMBER(sometable_enabled),     // Add INTEGER DEFAULT 1 mapped to output column named "sometable_enabled" in SQLite
            SQLT_QUERY_RESULT_MEMBER(someothertable_id),     // Add INTEGER NOT NULL mapped to output column named "someothertable_id" in SQLite
            SQLT_QUERY_RESULT_MEMBER(someothertable_name),   // Add TEXT NOT NULL mapped to output column named "someothertable_name" in SQLite
            SQLT_QUERY_RESULT_MEMBER(someothertable_amount)  // Add REAL mapped to output column named "someothertable_amount" in SQLite
        );
    };
};

int main()
{
    // Create some data to insert into SomeTable.
    std::vector<Database::SomeTable> dataSomeTable({
        { 1, "name1", 1.0, { "desc1"  }, true         },
        { 2, "name2", 2.0, { /*NULL*/ }, false        },
        { 3, "name3", 3.0, { "desc3"  }, { /*NULL*/ } }
    });

    // Create some data to insert into ManyToManyTable.
    std::vector<Database::ManyToManyTable> dataManyToManyTable({
        {1, 1},
        {1, 2},
        {2, 2},
        {3, 1},
        {3, 3}
    });

    // Create some data to insert into SomeOtherTable.
    std::vector<Database::SomeOtherTable> dataSomeOtherTable({
        { 1, "otherName1", {   1.5    }},
        { 2, "otherName2", {   2.5    }},
        { 3, "otherName3", { /*NULL*/ }},
    });

    char* errMsg;
    int result;

    // Execute "DROP TABLE IF EXISTS x;" for each defined SQLT_DATABASE_TABLE (normally just for testing the same data multiple times).
    SQLT::dropAllTables<Database>(&errMsg);

    // Execute "CREATE TABLE IF NOT EXISTS x(...);" for each defined SQLT_DATABASE_TABLE in the database. All data types are inserted as in the statically defined tables.
    SQLT::createAllTables<Database>(&errMsg);

    // Insert all data.
    SQLT::insert<Database>(dataSomeTable);
    SQLT::insert<Database>(dataManyToManyTable);
    SQLT::insert<Database>(dataSomeOtherTable);

    // SELECT * FROM SomeTable;
    std::vector<Database::SomeTable> rowsSomeTable;
    SQLT::selectAll<Database>(&rowsSomeTable); // rowsSomeTable now contains all rows in the table SomeTable.

    // SELECT name FROM SomeOtherTable;
    std::vector<std::string> namesSomeOtherTable;
    result = SQLT::select<Database>(&Database::SomeTable::name, &namesSomeOtherTable); // namesSomeOtherTable now contains all names in the table SomeOtherTable.
    assert(result == SQLITE_OK);
    assert(namesSomeOtherTable.size() == dataSomeOtherTable.size());

    // Custom query
    const char query[] = R"SQL(
        SELECT  st.id      AS sometable_id,
                st.name    AS sometable_name,
                st.enabled AS sometable_enabled,
                sot.id     AS someothertable_id,
                sot.name   AS someothertable_name,
                sot.amount AS someothertable_amount
        FROM    SomeTable  AS st
        INNER JOIN  mtm_resolver AS mtmr ON mtmr.sometable_id = st.id
        INNER JOIN  SomeOtherTable AS sot ON mtmr.someothertable_id = sot.id
        ORDER BY sot.id DESC, st.id ASC;
    )SQL";

    std::vector<Database::QueryOutput> queryOutput;
    result = SQLT::select<Database>(query, &queryOutput, 5); // queryOutput now contains the resulting rows from the SQLite query (we know that the query will output 5 rows).

    return 0;
}
```

## Data Types

SQLite has 5 data types: `NULL`, `INTEGER`, `REAL`, `TEXT` and `BLOB`. The mapping between C++ data types and SQLite data types in SQLite Tools are as follows:

SQLite|C++
------|---
`NULL`|`SQLT::Nullable<T>` where `T` can be any of the other C++ data types below
`INTEGER`|`int`, `bool` (`bool` automatically maps to `1` and `0` for `true` and `false`)
`REAL`|`double`
`TEXT`|`std::string`
`BLOB`|Not yet supported

## SQLite Tools with JSON Tools

SQLite Tools really shines when used in combination with JSON Tools: https://github.com/jorgen/json_tools JSON Tools can parse JSON strings into the same structs that SQLite Tools uses to insert data into SQLite.

Simply put:

```
JSON string   ----->   struct   ----->   SQLite
                ↑                 ↑
           JT::parseTo()    SQLT::insert()
```

The reverse is of course also possible:

```
JSON string   <-----   struct   <-----   SQLite
                 ↑                 ↑
      JT::serializeStruct()   SQLT::select()
```

Here is a sample similar to the one above, except now we parse a JSON string to our struct for insertion into SQLite, by use of JSON Tools. Then we select the data back from SQLite and serialize it to a JSON string using JSON Tools. This example is the same as the test named `readme-test2`.

```c++
// Note: The SQLITE_TOOLS_USE_JSON_TOOLS define is required to properly parse SQLite Tools types using JSON Tools.
#include <sqlite3/sqlite3.h>
#define SQLITE_TOOLS_USE_JSON_TOOLS
#include <json_tools/json_tools.h>
#include <sqlite_tools.h>

// Statically define the database with a single table.
struct Database
{
    // Statically define the table with four columns.
    struct SomeTable
    {
        int id;
        std::string name;
        double value;
        SQLT::Nullable<std::string> description;
        SQLT::Nullable<bool> enabled;

        // Define SomeTable to be a JSON Tools struct so we can parse JSON strings to SomeTable.
        JT_STRUCT(
            JT_MEMBER(id),
            JT_MEMBER(name),
            JT_MEMBER(value),
            JT_MEMBER(description),
            JT_MEMBER(enabled)
        );

        SQLT_TABLE(SomeTable,               // SQLite table name = "SomeTable"
            SQLT_COLUMN_PRIMARY_KEY(id),    // INTEGER PRIMARY KEY NOT NULL
            SQLT_COLUMN(name),              // TEXT NOT NULL
            SQLT_COLUMN(value),             // REAL NOT NULL
            SQLT_COLUMN(description),       // TEXT
            SQLT_COLUMN(enabled)            // INTEGER
        );
    };

    SQLT_DATABASE(Database,                 // SQlite database name = "Database"
        SQLT_DATABASE_TABLE(SomeTable)      // Define the table "SomeTable" in the database "Database"
    );
};

// The JSON data to insert. Note the missing "description" in the last element; it will be set to null in the struct.
const char jsonData[] = R"json(
[
    {
        "id": 1,
        "name": "name1",
        "value": 1.0,
        "description": "desc1"
    },
    {
        "id": 2,
        "name": "name2",
        "value": 2.0,
        "description": null,
        "enabled": true
    },
    {
        "id": 3,
        "name": "name3",
        "value": 3.0,
        "enabled": false
    }
]
)json";

int main()
{
    // Parse JSON string into structs using JSON Tools.
    JT::ParseContext parseContext(jsonData);
    std::vector<Database::SomeTable> rows;
    parseContext.parseTo(rows);
    assert(parseContext.error == JT::Error::NoError);

    char* errMsg;
    int result;

    // Execute "DROP TABLE IF EXISTS x;" for each defined SQLT_DATABASE_TABLE (normally just for testing the same data multiple times).
    result = SQLT::dropAllTables<Database>(&errMsg);
    if (result != SQLITE_OK)
    {
        fprintf(stderr, "%s", errMsg);
        return 1;
    }

    // Execute "CREATE TABLE IF NOT EXISTS x(...);" for each defined SQLT_DATABASE_TABLE in the database. All data types are inserted as in the statically defined tables.
    result = SQLT::createAllTables<Database>(&errMsg);
    if (result != SQLITE_OK)
    {
        fprintf(stderr, "%s", errMsg);
        return 2;
    }

    // Insert all rows in the rows vector.
    result = SQLT::insert<Database>(rows);
    if (result != SQLITE_OK)
    {
        fprintf(stderr, "Error while inserting rows: %d", result);
        return 3;
    }

    // Select the data back from the database.
    std::vector<Database::SomeTable> selected;
    result = SQLT::selectAll<Database>(&selected);
    if ((result != SQLITE_OK) || (selected.size() != rows.size()))
    {
        fprintf(stderr, "Error while selecting rows. SQLite code: %d, rows size: %zu, selected size: %zu", result, rows.size(), selected.size());
        return 4;
    }

    // Select a single row from the database (i.e. "SELECT name FROM SomeTable;").
    std::vector<decltype(Database::SomeTable::name)> names;
    result = SQLT::select<Database>(&Database::SomeTable::name, &names);
    if ((result != SQLITE_OK) || (names.size() != rows.size()))
    {
        fprintf(stderr, "Error while selecting names. SQLite code: %d, rows size: %zu, names size: %zu", result, rows.size(), names.size());
        return 5;
    }

    // Select a single row from the database (i.e. "SELECT name FROM SomeTable;").
    std::vector<decltype(Database::SomeTable::description)> descriptions;
    result = SQLT::select<Database>(&Database::SomeTable::description, &descriptions);
    if ((result != SQLITE_OK) || (descriptions.size() != rows.size()))
    {
        fprintf(stderr, "Error while selecting descriptions. SQLite code: %d, rows size: %zu, descriptions size: %zu", result, rows.size(), descriptions.size());
        return 6;
    }

    // Serialize the data selected from SQLite back to JSON using JSON Tools. This JSON string is identical to the jsonData string above (i.e. the JSON is identical).
    std::string jsonSelected = JT::serializeStruct(selected);

    return 0;
}
```
