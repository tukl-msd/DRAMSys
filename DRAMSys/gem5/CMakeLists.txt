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
#    Lukas Steiner

cmake_minimum_required(VERSION 3.10)

set(GEM5_VARIANT opt) # opt, fast
set(GEM5_ISA ARM) # ARM, X86

# Configuration:
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ Version")
set(DCMAKE_SH="CMAKE_SH-NOTFOUND")

find_library(GEM5_LIBRARY gem5_${GEM5_VARIANT} PATHS $ENV{GEM5}/build/${GEM5_ISA}/)

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../library/src/simulation/DRAMSysRecordable.cpp)
    add_definitions(-DRECORDING)
endif()

add_executable(DRAMSys_gem5 
    main.cpp
    $ENV{GEM5}/util/systemc/gem5_within_systemc/sc_logger.cc
    $ENV{GEM5}/util/systemc/gem5_within_systemc/sc_module.cc
    $ENV{GEM5}/util/systemc/gem5_within_systemc/stats.cc
    $ENV{GEM5}/util/tlm/src/sc_master_port.cc
    $ENV{GEM5}/util/tlm/src/sc_slave_port.cc
    $ENV{GEM5}/util/tlm/src/slave_transactor.cc
    $ENV{GEM5}/util/tlm/src/sc_ext.cc
    $ENV{GEM5}/util/tlm/src/sc_mm.cc
    $ENV{GEM5}/util/tlm/src/sim_control.cc
)

target_include_directories(DRAMSys_gem5
    PRIVATE $ENV{GEM5}/build/${GEM5_ISA}/
    PRIVATE $ENV{GEM5}/util/tlm/examples/slave_port/
    PRIVATE $ENV{GEM5}/util/tlm/examples/common/
    PRIVATE $ENV{GEM5}/util/tlm/src/
    PRIVATE $ENV{GEM5}/util/systemc/gem5_within_systemc/
    PRIVATE ../library/src/
)

target_link_libraries(DRAMSys_gem5 
    PRIVATE DRAMSysLibrary
    PRIVATE ${GEM5_LIBRARY}
)
