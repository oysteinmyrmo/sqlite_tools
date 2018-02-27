cmake_minimum_required (VERSION 3.0)
project (sqlite_tools)

set(CMAKE_CXX_STANDARD 11)
set(SQLT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()
add_subdirectory(tests)

