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

)

target_include_directories(sqlite3
    PUBLIC
        $<BUILD_INTERFACE:${sqlite3_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/sqlite3>
)

### Compile options ###
# refer to https://www.sqlite.org/compile.html
target_compile_definitions(sqlite3 PUBLIC SQLITE_ENABLE_RTREE SQLITE_OMIT_LOAD_EXTENSION)

# Use the same name as the FindSQLite3 script of CMake uses
add_library(SQLite3::SQLite3 ALIAS sqlite3)

if (DRAMSYS_INSTALL)
    include(GNUInstallDirs)

    install(
        TARGETS sqlite3
        EXPORT DRAMSysTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBRARYDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBRARYDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        FILES
            ${sqlite3_SOURCE_DIR}/sqlite3.h
            ${sqlite3_SOURCE_DIR}/sqlite3ext.h
        DESTINATION include/sqlite3
    )
endif()
