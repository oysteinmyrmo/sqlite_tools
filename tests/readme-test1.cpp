#include <string>
#include <vector>
#include <cstdio>
#include <cassert>

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

int mainWithoutErrorChecks()
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

int mainWithErrorChecks()
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
    result = SQLT::dropAllTables<Database>(&errMsg);
    assert(result == SQLITE_OK);

    // Execute "CREATE TABLE IF NOT EXISTS x(...);" for each defined SQLT_DATABASE_TABLE in the database. All data types are inserted as in the statically defined tables.
    result = SQLT::createAllTables<Database>(&errMsg);
    assert(result == SQLITE_OK);

    // Insert all data into SomeTable.
    result = SQLT::insert<Database>(dataSomeTable);
    assert(result == SQLITE_OK);

    // Insert all data into ManyToManyTable.
    result = SQLT::insert<Database>(dataManyToManyTable);
    assert(result == SQLITE_OK);

    // Insert all data into SomeOtherTable.
    result = SQLT::insert<Database>(dataSomeOtherTable);
    assert(result == SQLITE_OK);

    // SELECT * FROM SomeTable;
    std::vector<Database::SomeTable> rowsSomeTable;
    result = SQLT::selectAll<Database>(&rowsSomeTable); // rowsSomeTable now contains all rows in the table SomeTable.
    assert(result == SQLITE_OK);
    assert(rowsSomeTable.size() == dataSomeTable.size());

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
    assert(result == SQLITE_OK);
    assert(queryOutput.size() == 5);
    assert((queryOutput[0].sometable_id == 3) && (queryOutput[0].someothertable_id == 3));
    assert((queryOutput[1].sometable_id == 1) && (queryOutput[1].someothertable_id == 2));
    assert((queryOutput[2].sometable_id == 2) && (queryOutput[2].someothertable_id == 2));
    assert((queryOutput[3].sometable_id == 1) && (queryOutput[3].someothertable_id == 1));
    assert((queryOutput[4].sometable_id == 3) && (queryOutput[4].someothertable_id == 1));

    return 0;
}

int main()
{
    return mainWithErrorChecks();
}
