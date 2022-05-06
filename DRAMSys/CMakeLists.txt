# Copyright (c) 2020, Technische Universität Kaiserslautern
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

cmake_minimum_required(VERSION 3.10)

# Project Name:
project(DRAMSys)

option(DRAMSYS_COVERAGE_CHECK "Coverage check of DRAMSys")
option(DRAMSYS_WITH_GEM5 "Build DRAMSys with gem5 coupling")

# Configuration:
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ Version")
set(DCMAKE_SH "CMAKE_SH-NOTFOUND" CACHE STRING "Ignore sh.exe error on Windows")

if(DRAMSYS_COVERAGE_CHECK)
    message("== Coverage check enabled")
    set(GCC_COVERAGE_COMPILE_FLAGS "-g -O0 -coverage -fprofile-arcs -ftest-coverage")
    set(GCC_COVERAGE_LINK_FLAGS "-coverage -lgcov")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
endif()

# Add DRAMSysLibrary:
add_subdirectory(library)

# Add TraceAnalyzer:
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/traceAnalyzer)
    message("== Trace Analyzer included")
    add_subdirectory(traceAnalyzer)
endif()

# Add DRAMSysSimulator:
add_subdirectory(simulator)

# Add DRAMSysgem5
if(DEFINED ENV{GEM5} AND DRAMSYS_WITH_GEM5)
    message("== gem5 coupling included")
    add_subdirectory(gem5)
endif()

