#include <string>
#include <vector>
#include <cstdio>

#include <sqlite3/sqlite3.h>
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

        SQLT_TABLE(SomeTable,               // SQLite table name = "SomeTable"
            SQLT_COLUMN_PRIMARY_KEY(id),    // INTEGER PRIMARY KEY NOT NULL
            SQLT_COLUMN(name),              // TEXT NOT NULL
            SQLT_COLUMN(value),             // REAL NOT NULL
            SQLT_COLUMN(description)        // TEXT
        );
    };

    SQLT_DATABASE(Database,                 // SQlite database name = "Database"
        SQLT_DATABASE_TABLE(SomeTable)      // Define the table "SomeTable" in the database "Database"
    );
};

int main()
{
    // Create some data to insert.
    std::vector<Database::SomeTable> rows({
        { 1, "name1", 1.0, { "desc1"  }},
        { 2, "name2", 2.0, { /*NULL*/ }},
        { 3, "name3", 3.0, { "desc3"  }}
    });

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

    // Insert all rows in the data vector.
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

    return 0;
}
