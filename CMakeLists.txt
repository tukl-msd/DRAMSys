# Copyright (c) 2023, RPTU Kaiserslautern-Landau
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors:
#    Thomas Psota
#    Lukas Steiner

###############################################
###                 DRAMSys                 ###
###############################################
cmake_minimum_required(VERSION 3.24.0)

set(PROJECT_NAME "DRAMSys")

project(${PROJECT_NAME} VERSION "5.0")

### Compiler settings ###
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ Standard")
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

### CMake settings ###
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(build_source_group)
include(diagnostics_print)
include(enable_extensions)
include(FetchContent)

if(ENABLE_COVERAGE)
    include(coverage)
endif()

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# Check if standalone build or being included as submodule
get_directory_property(DRAMSYS_IS_SUBMODULE PARENT_DIRECTORY)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### Project settings ###
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "" )

if(NOT DRAMSYS_IS_SUBMODULE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

### DRAMSys directories ###
set(DRAMSYS_SOURCE_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(DRAMSYS_LIBRARY_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(DRAMSYS_TESTS_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/tests")
set(DRAMSYS_RESOURCE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/configs")
set(DRAMSYS_EXTENSIONS_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/extensions")

### Build options ###
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

option(DRAMSYS_VERBOSE_CMAKE_OUTPUT "Show detailed CMake output" OFF)
option(DRAMSYS_BUILD_TESTS "Build DRAMSys unit tests" OFF)
option(DRAMSYS_BUILD_BENCHMARKS "Build DRAMSys benchmarks" OFF)
option(DRAMSYS_BUILD_CLI "Build DRAMSys Command Line Tool" ON)
option(DRAMSYS_BUILD_TRACE_ANALYZER "Build DRAMSys Trace Analyzer" OFF)
option(DRAMSYS_WITH_DRAMPOWER "Build with DRAMPower support enabled." OFF)
option(DRAMSYS_USE_EXTERNAL_SYSTEMC "Use an external SystemC installation." OFF)

###############################################
###           Library Settings              ###
###############################################

### Detect OS threading library ###
find_package(Threads)

### nlohmann_json ###
add_subdirectory(${DRAMSYS_LIBRARY_DIR}/nlohmann_json)

### sqlite3 ###
add_subdirectory(${DRAMSYS_LIBRARY_DIR}/sqlite3)

### GoogleTest ###
if(DRAMSYS_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest
        GIT_TAG release-1.12.1)

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    set_target_properties(gmock PROPERTIES FOLDER lib/gtest)
    set_target_properties(gmock_main PROPERTIES FOLDER lib/gtest)
    set_target_properties(gtest PROPERTIES FOLDER lib/gtest)
    set_target_properties(gtest_main PROPERTIES FOLDER lib/gtest)
endif()

### GoogleBenchmark ###
if(DRAMSYS_BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF)

    FetchContent_Declare(
        benchmark
        URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
    )

    FetchContent_MakeAvailable(benchmark)
endif()

### SystemC ###
if (DRAMSYS_USE_EXTERNAL_SYSTEMC)
    list(APPEND CMAKE_PREFIX_PATH $ENV{SYSTEMC_HOME} /opt/systemc)
    find_package(systemc NAMES SystemCLanguage)
endif()

FetchContent_Declare(
    systemc
    GIT_REPOSITORY https://github.com/accellera-official/systemc.git
    GIT_TAG 2.3.4)

set(DISABLE_COPYRIGHT_MESSAGE True)

# Allow populating of a user-set SystemC package
if (NOT TARGET SystemC::systemc)
    FetchContent_MakeAvailable(systemc)

    # Set include directories to SYSTEM to suppress warnings
    set_target_properties(systemc PROPERTIES SYSTEM TRUE)
else()
    # Set include directories to SYSTEM to suppress warnings
    set_target_properties(SystemC::systemc PROPERTIES SYSTEM TRUE)
endif()


### DRAMPower ###
if (DRAMSYS_WITH_DRAMPOWER)
    FetchContent_Declare(
        DRAMPower
        GIT_REPOSITORY https://github.com/tukl-msd/DRAMPower
        GIT_TAG 9e64a1b)

    FetchContent_MakeAvailable(DRAMPower)
    set_target_properties(DRAMPower PROPERTIES FOLDER lib)
endif ()

###############################################
###           Source Directory              ###
###############################################

add_subdirectory(src/util)
add_subdirectory(src/configuration)
add_subdirectory(src/libdramsys)

if(DRAMSYS_BUILD_CLI)
    add_subdirectory(src/simulator)
endif()

if(DRAMSYS_BUILD_TRACE_ANALYZER)
    add_subdirectory(src/traceAnalyzer)
endif()

dramsys_enable_extensions()

###############################################
###           Test Directory                ###
###############################################

if(DRAMSYS_BUILD_TESTS)
    include(GoogleTest)
    include(CTest)
    add_subdirectory(tests)
endif()

###############################################
###         Benchmark Directory             ###
###############################################

if(DRAMSYS_BUILD_BENCHMARKS)
    add_subdirectory(benches)
endif()
