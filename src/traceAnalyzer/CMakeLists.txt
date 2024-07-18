# Copyright (c) 2020, RPTU Kaiserslautern-Landau
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
#    Matthias Jung
#    Lukas Steiner
#    Derek Christ
#    Iron Prando da Silva

########################################
###           TraceAnalyzer          ###
########################################

project(TraceAnalyzer)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS *.h;*.hpp)

# Add Python3 Dependency:
find_package(Python3 COMPONENTS Development Interpreter)

FetchContent_Declare(
    pybind11
    URL https://github.com/pybind/pybind11/archive/refs/tags/v2.13.1.zip
)

FetchContent_MakeAvailable(pybind11)

# Add QWT Dependency:
find_library(QWT_LIBRARY NAMES "qwt" "qwt-qt5" PATHS
    "$ENV{QWT_HOME}/lib"
    "/opt/homebrew/opt/qwt-qt5/lib"
)
find_path(QWT_INCLUDE_DIRS NAMES "qwt_plot.h" PATHS
    "$ENV{QWT_HOME}/include"
    "/usr/include/qwt-qt5"
    "/usr/include/qwt"
    "/opt/homebrew/opt/qwt-qt5/include"
    "/opt/homebrew/opt/qwt-qt5/lib/qwt.framework/Headers"
)

# Add QT Library:
if (APPLE)
    set(Qt5_DIR "/opt/homebrew/opt/qt@5/lib/cmake/Qt5")
endif(APPLE)
find_package(Qt5 COMPONENTS Core Gui Widgets Sql REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(TraceAnalyzer ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(TraceAnalyzer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${QWT_INCLUDE_DIRS}
)

target_link_libraries(TraceAnalyzer
    PRIVATE
        pybind11::embed
        ${QWT_LIBRARY}
        Qt5::Widgets
        Qt5::Sql
        DRAMSys::util
        DRAMSys::config
)

set(DRAMSYS_TRACE_ANALYZER_EXTENSION_DIR "${CMAKE_SOURCE_DIR}/extensions/apps/traceAnalyzer")

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        DRAMSYS_TRACE_ANALYZER_EXTENSION_DIR="${DRAMSYS_TRACE_ANALYZER_EXTENSION_DIR}"
)

build_source_group()
