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
#    Thomas Psota

########################################
###      DRAMSys::simulator          ###
########################################

find_package(Threads)

add_library(simulator OBJECT
    simulator/Cache.cpp
    simulator/Simulator.cpp
    simulator/generator/RandomState.cpp
    simulator/generator/SequentialState.cpp
    simulator/generator/TrafficGenerator.cpp
    simulator/hammer/RowHammer.cpp
    simulator/player/StlPlayer.cpp
    simulator/request/RequestIssuer.cpp
    simulator/util.cpp
)

target_include_directories(simulator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(simulator
    PUBLIC
        DRAMSys::DRAMSys
    PRIVATE
        Threads::Threads
)

target_compile_definitions(simulator
    PUBLIC
        DRAMSYS_RESOURCE_DIR="${DRAMSYS_RESOURCE_DIR}"
)

add_library(DRAMSys::simulator ALIAS simulator)

add_executable(dramsys_bin
    main.cpp
)

set_target_properties(dramsys_bin PROPERTIES OUTPUT_NAME "DRAMSys")

target_link_libraries(dramsys_bin
    PRIVATE
        DRAMSys::simulator
)

install(TARGETS dramsys_bin)
