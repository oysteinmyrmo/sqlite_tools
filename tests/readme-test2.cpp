#include <sqlite3/sqlite3.h>
#include <string>
#include <vector>
#include <cstdio>

// Note: The SQLITE_TOOLS_USE_JSON_TOOLS define is required to properly parse SQLite Tools types using JSON Tools.
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

        // Define SomeTable to be a JSON Tools struct so we can parse JSON strings to SomeTable.
        JT_STRUCT(
            JT_MEMBER(id),
            JT_MEMBER(name),
            JT_MEMBER(value),
            JT_MEMBER(description)
        );

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
        "description": null
    },
    {
        "id": 3,
        "name": "name3",
        "value": 3.0
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

    // Parse the data selected from SQLite back to JSON using JSON Tools. This JSON string is identical to the jsonData string above (i.e. the JSON is identical).
    std::string jsonSelected = JT::serializeStruct(selected);

    return 0;
}
