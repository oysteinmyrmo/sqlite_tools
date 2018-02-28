# SQLite Tools

SQLite Tools is a single header only C++11 library to create SQLite databases and insert from and select back to C++ structs. Documentation is yet to come. In the meantime, take a look at the tests for some general use cases.

SQLite Tools really shines when used in combination with JSON Tools: https://github.com/jorgen/json_tools JSON Tools can parse JSON strings into the same structs that SQLite Tools uses to insert data into SQLite.

Simply put:

```
JSON string ----> struct ----> SQLite
              ↑            ↑
          JSON Tools  SQLite Tools
```

