########################################
###              sqlite3             ###
########################################

project(sqlite3 VERSION 3.40.1)


FetchContent_Declare(sqlite3 URL "https://www.sqlite.org/2022/sqlite-amalgamation-3400100.zip")
if (NOT sqlite3_POPULATED)
    FetchContent_Populate(sqlite3)
endif()

add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${sqlite3_SOURCE_DIR}/sqlite3.c ${sqlite3_SOURCE_DIR}/sqlite3.h ${sqlite3_SOURCE_DIR}/sqlite3ext.h )
target_include_directories(${PROJECT_NAME} PUBLIC ${sqlite3_SOURCE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER lib)

### Compile options ###
# refer to https://www.sqlite.org/compile.html

target_compile_definitions(${PROJECT_NAME} PUBLIC SQLITE_ENABLE_RTREE SQLITE_OMIT_LOAD_EXTENSION)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})