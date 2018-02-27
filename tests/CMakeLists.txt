# SQLT files
set(SQLT_TEST_EXTERNALS "${CMAKE_CURRENT_SOURCE_DIR}/externals")
include_directories("${SQLT_INCLUDE_DIR}")
include_directories("${SQLT_TEST_EXTERNALS}")
set(SQLT_HEADER "${SQLT_INCLUDE_DIR}/sqlite_tools.h")

# SQLite files
set(SQLITE_DIR "${SQLT_TEST_EXTERNALS}/sqlite3")
file(GLOB SQLITE_FILES "${SQLITE_DIR}/*")

# json_tools files
set(JSON_TOOLS_DIR "${SQLT_TEST_EXTERNALS}/json_tools")
set(JT_HEADER "${JSON_TOOLS_DIR}/json_tools.h")

add_executable(all-types assert.h all-types.cpp "${SQLT_HEADER}" "${SQLITE_FILES}" "${JT_HEADER}")

add_test(NAME all-types COMMAND all-types)
