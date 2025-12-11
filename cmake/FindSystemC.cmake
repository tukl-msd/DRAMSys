# User must provide SYSTEMC_HOME
# Defines:
#   SystemC_FOUND
#   SystemC_INCLUDE_DIRS
#   SystemC_LIBRARIES
#   SystemC_VERSION (optional)
#   SystemC::systemc (imported target)

if (NOT DEFINED SYSTEMC_HOME)
    message(FATAL_ERROR "SYSTEMC_HOME is not defined. Please set it to your SystemC installation directory.")
endif()

find_library(SystemC_LIBRARY
    NAMES systemc systemc-ar
    HINTS ${SYSTEMC_HOME}
    PATH_SUFFIXES
        lib
        lib64
        lib-linux
        lib-linux64
        lib-macos
    NO_DEFAULT_PATH
)

if (NOT SystemC_LIBRARY)
    message(FATAL_ERROR "SystemC library not found in ${SYSTEMC_HOME}")
endif()

set(SystemC_VERSION "")
set(SystemC_INCLUDE_DIR "${SYSTEMC_HOME}/include")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SystemC
    REQUIRED_VARS SystemC_LIBRARY SystemC_INCLUDE_DIR
    VERSION_VAR SystemC_VERSION
)

mark_as_advanced(SystemC_INCLUDE_DIR SystemC_LIBRARY)

if (SystemC_FOUND)
    add_library(SystemC::systemc UNKNOWN IMPORTED)
    set_target_properties(SystemC::systemc PROPERTIES
        IMPORTED_LOCATION "${SystemC_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SystemC_INCLUDE_DIR}"
    )
endif()
