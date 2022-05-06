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
#    Derek Christ

cmake_minimum_required(VERSION 3.10)

# Project Name
project(DRAMSysSimulator)

# Configuration:
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ Version")
set(DCMAKE_SH="CMAKE_SH-NOTFOUND")

find_package(Threads REQUIRED)

add_executable(DRAMSys
    main.cpp
    ExampleInitiator.h
    MemoryManager.cpp
    StlPlayer.cpp
    TrafficGenerator.cpp
    TrafficInitiator.cpp
    TraceSetup.cpp
    LengthConverter.cpp
)

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../library/src/simulation/DRAMSysRecordable.cpp)
    target_compile_definitions(DRAMSys PRIVATE RECORDING)
endif()

target_include_directories(DRAMSys
    PUBLIC ../library/src/
)

target_link_libraries(DRAMSys
    PRIVATE DRAMSysLibrary
    PRIVATE Threads::Threads
)
