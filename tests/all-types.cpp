#include <vector>
#include "assert.h"

#define SQLITE_TOOLS_USE_JSON_TOOLS
#include <sqlite3/sqlite3.h>
#include <json_tools/json_tools.h>
#include <sqlite_tools.h>

const char all_types_data[] = R"json(
[
    {
        "id": 11,
        "num": 1.1,
        "name": "first1",
        "enabled": true,
        "id2": 12,
        "num2": 1.2,
        "name2": "first2",
        "enabled2": false,
        "id3": 13,
        "num3": 1.3,
        "name3": "first3",
        "enabled3": false,
        "id4": 14,
        "num4": 1.4,
        "name4": "first4",
        "enabled4": true
    },
    {
        "id": 21,
        "num": 2.1,
        "name": "second1",
        "enabled": true,
        "id2": 22,
        "num2": 2.2,
        "name2": "second2",
        "enabled2": false,
        "id3": 23,
        "num3": 2.3,
        "name3": "second3",
        "enabled3": true,
        "id4": 24,
        "num4": 2.4,
        "name4": "second4",
        "enabled4": false
    },
    {
        "id": 31,
        "num": 3.1,
        "name": "third1",
        "enabled": false,
        "id2": 32,
        "num2": 3.2,
        "name2": "third2",
        "enabled2": true,
        "id3": null,
        "num3": null,
        "name3": null,
        "enabled3": null,
        "id4": null,
        "num4": null,
        "name4": null,
        "enabled4": null
    }
]
)json";

struct AllTypes
{
    int id;
    double num;
    std::string name;
    bool enabled;

    int id2;
    double num2;
    std::string name2;
    bool enabled2;

    SQLT::Nullable<int> id3;
    SQLT::Nullable<double> num3;
    SQLT::Nullable<std::string> name3;
    SQLT::Nullable<bool> enabled3;

    SQLT::Nullable<int> id4;
    SQLT::Nullable<double> num4;
    SQLT::Nullable<std::string> name4;
    SQLT::Nullable<bool> enabled4;

    JT_STRUCT(
        JT_MEMBER(id),
        JT_MEMBER(num),
        JT_MEMBER(name),
        JT_MEMBER(enabled),
        JT_MEMBER(id2),
        JT_MEMBER(num2),
        JT_MEMBER(name2),
        JT_MEMBER(enabled2),
        JT_MEMBER(id3),
        JT_MEMBER(num3),
        JT_MEMBER(name3),
        JT_MEMBER(enabled3),
        JT_MEMBER(id4),
        JT_MEMBER(num4),
        JT_MEMBER(name4),
        JT_MEMBER(enabled4)
    );

    SQLT_TABLE(AllTypes,                                                // Table name = "AllTypes"
        SQLT_COLUMN_FLAGS(id, SQLT::Flags::PRIMARY_KEY),                // id INTEGER NOT NULL PRIMARY KEY
        SQLT_COLUMN(num),                                               // num REAL NOT NULL
        SQLT_COLUMN(name),                                              // name TEXT NOT NULL
        SQLT_COLUMN(enabled),                                           // enabled INTEGER NOT NULL
        SQLT_COLUMN_FLAGS_DEFAULT(id2, 123, SQLT::Flags::PRIMARY_KEY),  // id2 INTEGER PRIMARY KEY NOT NULL DEFAULT 123
        SQLT_COLUMN_DEFAULT(num2, 1.23),                                // num2 REAL NOT NULL DEFAULT 1.23
        SQLT_COLUMN_DEFAULT(name2, "second"),                           // name2 TEXT NOT NULL DEFAULT "second"
        SQLT_COLUMN_DEFAULT(enabled2, false),                           // enabled2 INTEGER NOT NULL DEFAULT 0
        SQLT_COLUMN_FLAGS(id3, SQLT::Flags::PRIMARY_KEY),               // id3 INTEGER PRIMARY KEY
        SQLT_COLUMN(num3),                                              // num3 REAL
        SQLT_COLUMN(name3),                                             // name3 TEXT
        SQLT_COLUMN(enabled3),                                          // enabled3 INTEGER
        SQLT_COLUMN_FLAGS_DEFAULT(id4, 4, SQLT::Flags::PRIMARY_KEY),    // id4 INTEGER PRIMARY KEY DEFAULT 4
        SQLT_COLUMN_DEFAULT(num4, 4.44),                                // num4 REAL DEFAULT 4.44
        SQLT_COLUMN_DEFAULT(name4, "fourth"),                           // name4 TEXT DEFAULT "fourth"
        SQLT_COLUMN_DEFAULT(enabled4, true)                             // enabled4 INTEGER NOT NULL
    );
};

int main()
{
    SQLT_ASSERT(SQLT::Internal::primaryKeyCount<AllTypes>() == 4);
    SQLT_ASSERT(SQLT::tableName<AllTypes>() == "AllTypes");
    SQLT_ASSERT(SQLT::Internal::columnCount<AllTypes>() == 16);

    sqlite3 *db;
    int rc;
    char* zErrMsg = 0;

    rc = sqlite3_open("alltypes", &db);
    SQLT_ASSERT(rc == SQLITE_OK);

    if (rc)
    {
        fprintf(stderr, "Cannot open db!");
    }
    else
    {
        rc = SQLT::dropTableIfExists<AllTypes>(db, &zErrMsg);
        SQLT_ASSERT(rc == SQLITE_OK);

        rc = SQLT::createTableIfNotExists<AllTypes>(db, &zErrMsg);
        SQLT_ASSERT(rc == SQLITE_OK);

        if (rc == SQLITE_OK)
        {
            std::vector<AllTypes> data;
            JT::ParseContext jtContext(all_types_data);
            jtContext.parseTo(data);
            SQLT_ASSERT(jtContext.error == JT::Error::NoError);

            rc = SQLT::insert(db, data);
            SQLT_ASSERT(rc == SQLITE_OK);

            std::vector<AllTypes> rows;
            rc = SQLT::selectAll(db, &rows);
            SQLT_ASSERT(rc == SQLITE_OK);
            SQLT_ASSERT(rows.size() == 3);

            auto &row = rows[0];
            SQLT_ASSERT(row.id == 11);
            SQLT_FUZZY_ASSERT(row.num, 1.1);
            SQLT_ASSERT(row.name == "first1");
            SQLT_ASSERT(row.enabled == true);
            SQLT_ASSERT(row.id2 == 12);
            SQLT_FUZZY_ASSERT(row.num2, 1.2);
            SQLT_ASSERT(row.name2 == "first2");
            SQLT_ASSERT(row.enabled2 == false);
            SQLT_ASSERT((row.id3.value == 13) && (row.id3.is_null == false));
            SQLT_ASSERT(row.num3.is_null == false);
            SQLT_FUZZY_ASSERT(row.num3.value, 1.3);
            SQLT_ASSERT((row.name3.value == "first3") && (row.name3.is_null == false));
            SQLT_ASSERT((row.enabled3.is_null == false) && (row.enabled3.value == false));
            SQLT_ASSERT((row.id4.value == 14) && (row.id4.is_null == false));
            SQLT_ASSERT(row.num4.is_null == false);
            SQLT_FUZZY_ASSERT(row.num4.value, 1.4);
            SQLT_ASSERT((row.name4.value == "first4") && (row.name4.is_null == false));
            SQLT_ASSERT((row.enabled4.is_null == false) && (row.enabled4.value == true));

            row = rows[1];
            SQLT_ASSERT(row.id == 21);
            SQLT_FUZZY_ASSERT(row.num, 2.1);
            SQLT_ASSERT(row.name == "second1");
            SQLT_ASSERT(row.enabled == true);
            SQLT_ASSERT(row.id2 == 22);
            SQLT_FUZZY_ASSERT(row.num2, 2.2);
            SQLT_ASSERT(row.name2 == "second2");
            SQLT_ASSERT(row.enabled2 == false);
            SQLT_ASSERT((row.id3.value == 23) && (row.id3.is_null == false));
            SQLT_ASSERT(row.num3.is_null == false);
            SQLT_FUZZY_ASSERT(row.num3.value, 2.3);
            SQLT_ASSERT((row.name3.value == "second3") && (row.name3.is_null == false));
            SQLT_ASSERT((row.enabled3.is_null == false) && (row.enabled3.value == true));
            SQLT_ASSERT((row.id4.value == 24) && (row.id4.is_null == false));
            SQLT_ASSERT(row.num4.is_null == false);
            SQLT_FUZZY_ASSERT(row.num4.value, 2.4);
            SQLT_ASSERT((row.name4.value == "second4") && (row.name4.is_null == false));
            SQLT_ASSERT((row.enabled4.is_null == false) && (row.enabled4.value == false));

            row = rows[2];
            SQLT_ASSERT(row.id == 31);
            SQLT_FUZZY_ASSERT(row.num, 3.1);
            SQLT_ASSERT(row.name == "third1");
            SQLT_ASSERT(row.enabled == false);
            SQLT_ASSERT(row.id2 == 32);
            SQLT_FUZZY_ASSERT(row.num2, 3.2);
            SQLT_ASSERT(row.name2 == "third2");
            SQLT_ASSERT(row.enabled2 == true);
            SQLT_ASSERT(row.id3.is_null == true);
            SQLT_ASSERT(row.num3.is_null == true);
            SQLT_ASSERT(row.name3.is_null == true);
            SQLT_ASSERT(row.enabled3.is_null == true);
            SQLT_ASSERT(row.id4.is_null == true);
            SQLT_ASSERT(row.num4.is_null == true);
            SQLT_ASSERT(row.name4.is_null == true);
            SQLT_ASSERT(row.enabled4.is_null == true);
        }
        else
        {
            fprintf(stderr, "SQLite error: %s\n", zErrMsg);
        }
    }

    sqlite3_free(zErrMsg);
    rc = sqlite3_close(db);
    SQLT_ASSERT(rc == SQLITE_OK);

    return 0;
}
