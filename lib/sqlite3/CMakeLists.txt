########################################
###              sqlite3             ###
########################################

FetchContent_Declare(SQLite3
    URL "https://www.sqlite.org/2024/sqlite-amalgamation-3470000.zip"
    OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(SQLite3)

add_library(sqlite3)
target_sources(sqlite3 PRIVATE
    ${sqlite3_SOURCE_DIR}/sqlite3.c
    ${sqlite3_SOURCE_DIR}/sqlite3.h
    ${sqlite3_SOURCE_DIR}/sqlite3ext.h
)

target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})

### Compile options ###
# refer to https://www.sqlite.org/compile.html
target_compile_definitions(sqlite3 PUBLIC SQLITE_ENABLE_RTREE SQLITE_OMIT_LOAD_EXTENSION)

# Use the same name as the FindSQLite3 script of CMake uses
add_library(SQLite::SQLite3 ALIAS sqlite3)
