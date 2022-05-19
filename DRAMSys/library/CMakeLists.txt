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
#   Matthias Jung
#   Lukas Steiner
#   Derek Christ

cmake_minimum_required(VERSION 3.10)

# Project Name
project(DRAMSysLibrary)

# Configuration:
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ Version")
set(DCMAKE_SH="CMAKE_SH-NOTFOUND")

# Add DRAMPower:
add_subdirectory(src/common/third_party/DRAMPower)

# Add Configuration
add_subdirectory(src/common/configuration)

# Add SystemC:
if(DEFINED ENV{SYSTEMC_HOME})
    find_library(SYSTEMC_LIBRARY 
        NAMES systemc SnpsVP 
	PATHS $ENV{SYSTEMC_HOME}/lib-$ENV{SYSTEMC_TARGET_ARCH}/ $ENV{SYSTEMC_HOME}/lib-linux64/ $ENV{SYSTEMC_HOME}/lib64 $ENV{SYSTEMC_HOME}/libso-$ENV{COWARE_CXX_COMPILER}/
    )
    message("== Building with external SystemC located in $ENV{SYSTEMC_HOME}")
else()
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Shared Libs")
    add_subdirectory(src/common/third_party/systemc)
    set(SYSTEMC_LIBRARY systemc)
    message("== Building with SystemC submodule")
endif()

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/src/common/third_party/sqlite-amalgamation)
    message("== Database recording included")
    # Add sqlite3 Dependency:
    set(BUILD_ENABLE_RTREE ON CACHE BOOL "Enable R-Tree Feature")
    set(BUILD_ENABLE_RTREE ON)
    add_subdirectory(src/common/third_party/sqlite-amalgamation)

    set(RECORDING_SOURCES
        src/common/TlmRecorder.cpp
        src/controller/ControllerRecordable.cpp
        src/simulation/DRAMSysRecordable.cpp
        src/simulation/dram/DramRecordable.cpp
    )
endif()

add_library(DRAMSysLibrary
    src/common/DebugManager.cpp
    src/common/dramExtensions.cpp
    src/common/tlm2_base_protocol_checker.h
    src/common/utils.cpp
    
    src/configuration/Configuration.cpp 
    src/configuration/TemperatureSimConfig.h
    
    src/configuration/memspec/MemSpec.cpp
    src/configuration/memspec/MemSpecDDR3.cpp
    src/configuration/memspec/MemSpecDDR4.cpp
    src/configuration/memspec/MemSpecDDR5.cpp
    src/configuration/memspec/MemSpecLPDDR4.cpp
    src/configuration/memspec/MemSpecLPDDR5.cpp
    src/configuration/memspec/MemSpecWideIO.cpp
    src/configuration/memspec/MemSpecWideIO2.cpp
    src/configuration/memspec/MemSpecGDDR5.cpp
    src/configuration/memspec/MemSpecGDDR5X.cpp
    src/configuration/memspec/MemSpecGDDR6.cpp
    src/configuration/memspec/MemSpecHBM2.cpp
    src/configuration/memspec/MemSpecSTTMRAM.cpp
    
    src/controller/BankMachine.cpp
    src/controller/Command.cpp
    src/controller/ControllerIF.h
    src/controller/Controller.cpp
    
    src/controller/checker/CheckerIF.h
    src/controller/checker/CheckerDDR3.cpp
    src/controller/checker/CheckerDDR4.cpp
    src/controller/checker/CheckerDDR5.cpp
    src/controller/checker/CheckerLPDDR4.cpp
    src/controller/checker/CheckerLPDDR5.cpp
    src/controller/checker/CheckerWideIO.cpp
    src/controller/checker/CheckerWideIO2.cpp
    src/controller/checker/CheckerGDDR5.cpp
    src/controller/checker/CheckerGDDR5X.cpp
    src/controller/checker/CheckerGDDR6.cpp
    src/controller/checker/CheckerHBM2.cpp
    src/controller/checker/CheckerSTTMRAM.cpp
    
    src/controller/cmdmux/CmdMuxIF.h
    src/controller/cmdmux/CmdMuxOldest.cpp
    src/controller/cmdmux/CmdMuxStrict.cpp
    
    src/controller/powerdown/PowerDownManagerIF.h
    src/controller/powerdown/PowerDownManagerDummy.cpp
    src/controller/powerdown/PowerDownManagerStaggered.cpp
    
    src/controller/refresh/RefreshManagerIF.h
    src/controller/refresh/RefreshManagerDummy.cpp
    src/controller/refresh/RefreshManagerAllBank.cpp
    src/controller/refresh/RefreshManagerPerBank.cpp
    src/controller/refresh/RefreshManagerPer2Bank.cpp
    src/controller/refresh/RefreshManagerSameBank.cpp
    
    src/controller/respqueue/RespQueueIF.h
    src/controller/respqueue/RespQueueFifo.cpp
    src/controller/respqueue/RespQueueReorder.cpp
    
    src/controller/scheduler/SchedulerIF.h
    src/controller/scheduler/SchedulerFifo.cpp
    src/controller/scheduler/SchedulerFrFcfs.cpp
    src/controller/scheduler/SchedulerFrFcfsGrp.cpp
    src/controller/scheduler/SchedulerGrpFrFcfs.cpp
    src/controller/scheduler/SchedulerGrpFrFcfsWm.cpp

    src/controller/scheduler/BufferCounterIF.h
    src/controller/scheduler/BufferCounterBankwise.cpp
    src/controller/scheduler/BufferCounterReadWrite.cpp
    src/controller/scheduler/BufferCounterShared.cpp
    
    src/error/eccbaseclass.cpp
    src/error/ecchamming.cpp
    src/error/errormodel.cpp
    
    src/error/ECC/Bit.cpp
    src/error/ECC/ECC.cpp
    src/error/ECC/Word.cpp
    
    src/simulation/Arbiter.cpp
    src/simulation/AddressDecoder.cpp
    src/simulation/DRAMSys.cpp
    src/simulation/ReorderBuffer.h
    src/simulation/TemperatureController.cpp
    
    src/simulation/dram/Dram.cpp
    src/simulation/dram/DramDDR3.cpp 
    src/simulation/dram/DramDDR4.cpp 
    src/simulation/dram/DramDDR5.cpp
    src/simulation/dram/DramLPDDR4.cpp
    src/simulation/dram/DramLPDDR5.cpp
    src/simulation/dram/DramWideIO.cpp
    src/simulation/dram/DramWideIO2.cpp
    src/simulation/dram/DramGDDR5.cpp
    src/simulation/dram/DramGDDR5X.cpp
    src/simulation/dram/DramGDDR6.cpp
    src/simulation/dram/DramHBM2.cpp
    src/simulation/dram/DramSTTMRAM.cpp

    ${RECORDING_SOURCES}
	
	
    # Simulation Config Files
    resources/simulations/ddr3-example.json
    resources/simulations/ddr3-example2.json
    resources/simulations/ddr3-gem5-se.json
    resources/simulations/ddr4-example.json
    resources/simulations/hbm2-example.json
    resources/simulations/lpddr4-example.json
    resources/simulations/ranktest.json
    resources/simulations/wideio-example.json
    resources/simulations/wideio-thermal.json

    # Address Mapping Config Files
    resources/configs/amconfigs/am_ddr3_4x4Gbx16_dimm_p2KB_brc.json
    resources/configs/amconfigs/am_ddr3_4x4Gbx16_dimm_p2KB_rbc.json
    resources/configs/amconfigs/am_ddr3_8x1Gbx8_dimm_p1KB_brc.json
    resources/configs/amconfigs/am_ddr3_8x1Gbx8_dimm_p1KB_rbc.json
    resources/configs/amconfigs/am_ddr3_8x2Gbx8_dimm_p1KB_brc.json
    resources/configs/amconfigs/am_ddr3_8x2Gbx8_dimm_p1KB_rbc.json
    resources/configs/amconfigs/am_ddr3_x16_brc.json
    resources/configs/amconfigs/am_ddr3_x16_rbc.json
    resources/configs/amconfigs/am_ddr4_8x4Gbx8_dimm_p1KB_brc.json
    resources/configs/amconfigs/am_hbm2_8Gb_pc_brc.json
    resources/configs/amconfigs/am_lpddr4_8Gbx16_brc.json
    resources/configs/amconfigs/am_ranktest.json
    resources/configs/amconfigs/am_wideio2_4x64_4x2Gb_brc.json
    resources/configs/amconfigs/am_wideio2_4x64_4x2Gb_rbc.json
    resources/configs/amconfigs/am_wideio_4x1Gb_brc.json
    resources/configs/amconfigs/am_wideio_4x1Gb_rbc.json
    resources/configs/amconfigs/am_wideio_4x256Mb_brc.json
    resources/configs/amconfigs/am_wideio_4x256Mb_rbc.json
    resources/configs/amconfigs/am_wideio_4x2Gb_brc.json
    resources/configs/amconfigs/am_wideio_4x2Gb_rbc.json
    resources/configs/amconfigs/am_wideio_4x4Gb_brc.json
    resources/configs/amconfigs/am_wideio_4x4Gb_rbc.json
    resources/configs/amconfigs/am_wideio_4x512Mb_brc.json
    resources/configs/amconfigs/am_wideio_4x512Mb_rbc.json

    # Memory Controller Config Files
    resources/configs/mcconfigs/fifo.json
    resources/configs/mcconfigs/fifoStrict.json
    resources/configs/mcconfigs/fr_fcfs_grp.json
    resources/configs/mcconfigs/fr_fcfs.json

    # Memspec Config Files
    resources/configs/memspecs/HBM2.json
    resources/configs/memspecs/JEDEC_256Mb_WIDEIO-200_128bit.json
    resources/configs/memspecs/JEDEC_256Mb_WIDEIO-266_128bit.json
    resources/configs/memspecs/JEDEC_4Gb_DDR4-1866_8bit_A.json
    resources/configs/memspecs/JEDEC_4Gb_DDR4-2400_8bit_A.json
    resources/configs/memspecs/JEDEC_4x64_2Gb_WIDEIO2-400_64bit.json
    resources/configs/memspecs/JEDEC_4x64_2Gb_WIDEIO2-533_64bit.json
    resources/configs/memspecs/JEDEC_8Gb_LPDDR4-3200_16bit.json
    resources/configs/memspecs/memspec_ranktest.json
    resources/configs/memspecs/MICRON_1Gb_DDR2-1066_16bit_H.json
    resources/configs/memspecs/MICRON_1Gb_DDR2-800_16bit_H.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1066_16bit_G_2s.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1066_16bit_G_3s.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1066_16bit_G.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1066_16bit_G_mu.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1066_8bit_G_2s.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1066_8bit_G_3s.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1066_8bit_G.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1066_8bit_G_mu.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1600_8bit_G_2s.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1600_8bit_G_3s.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1600_8bit_G.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1600_8bit_G_less_refresh.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-1600_8bit_G_mu.json
    resources/configs/memspecs/MICRON_1Gb_DDR3-800_8bit_G.json
    resources/configs/memspecs/MICRON_2GB_DDR3-1066_64bit_D_SODIMM.json
    resources/configs/memspecs/MICRON_2GB_DDR3-1066_64bit_G_UDIMM.json
    resources/configs/memspecs/MICRON_2Gb_DDR3-1066_8bit_D_2s.json
    resources/configs/memspecs/MICRON_2Gb_DDR3-1066_8bit_D_3s.json
    resources/configs/memspecs/MICRON_2Gb_DDR3-1066_8bit_D.json
    resources/configs/memspecs/MICRON_2Gb_DDR3-1066_8bit_D_mu.json
    resources/configs/memspecs/MICRON_2GB_DDR3-1333_64bit_D_SODIMM.json
    resources/configs/memspecs/MICRON_2Gb_DDR3-1600_16bit_D_2s.json
    resources/configs/memspecs/MICRON_2Gb_DDR3-1600_16bit_D_3s.json
    resources/configs/memspecs/MICRON_2Gb_DDR3-1600_16bit_D.json
    resources/configs/memspecs/MICRON_2Gb_DDR3-1600_16bit_D_mu.json
    resources/configs/memspecs/MICRON_2GB_DDR3-1600_64bit_G_UDIMM.json
    resources/configs/memspecs/MICRON_2Gb_LPDDR2-1066-S4_16bit_A.json
    resources/configs/memspecs/MICRON_2Gb_LPDDR-266_16bit_A.json
    resources/configs/memspecs/MICRON_2Gb_LPDDR2-800-S4_16bit_A.json
    resources/configs/memspecs/MICRON_2Gb_LPDDR-333_16bit_A.json
    resources/configs/memspecs/MICRON_4Gb_DDR4-1866_8bit_A.json
    resources/configs/memspecs/MICRON_4Gb_DDR4-2400_8bit_A.json
    resources/configs/memspecs/MICRON_4Gb_LPDDR3-1333_32bit_A.json
    resources/configs/memspecs/MICRON_4Gb_LPDDR3-1600_32bit_A.json
    resources/configs/memspecs/MICRON_6Gb_LPDDR4-3200_32bit_A.json
    resources/configs/memspecs/SAMSUNG_K4B1G1646E_1Gb_DDR3-1600_16bit.json
    resources/configs/memspecs/SAMSUNG_K4B4G1646Q_4Gb_DDR3-1066_16bit.json

    # Simulator Config Files
    resources/configs/simulator/ddr3_ecc.json
    resources/configs/simulator/ddr3.json
    resources/configs/simulator/ddr3_gem5_se.json
    resources/configs/simulator/ddr4.json
    resources/configs/simulator/hbm2.json
    resources/configs/simulator/lpddr4.json
    resources/configs/simulator/wideio.json
    resources/configs/simulator/wideio_thermal.json

    # Thermal Simulation Config Files
    resources/configs/thermalsim/config.json
    resources/configs/thermalsim/powerInfo.json
)

if(DEFINED ENV{LIBTHREED_ICE_HOME})
    message("== Thermal simulation available")
    target_compile_definitions(DRAMSysLibrary PRIVATE THERMALSIM)
    target_include_directories(DRAMSysLibrary
        PRIVATE $ENV{LIBTHREED_ICE_HOME}/include/
    )
    find_library(3DICE_LIBRARY NAMES threed-ice-2.2.4 PATHS $ENV{LIBTHREED_ICE_HOME}/lib/)
    target_link_libraries(DRAMSysLibrary
        PRIVATE ${3DICE_LIBRARY}
    )
endif()

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/src/common/third_party/sqlite-amalgamation)
    target_include_directories(DRAMSysLibrary
        PUBLIC src/common/third_party/sqlite-amalgamation/
    )
    target_link_libraries(DRAMSysLibrary
        PRIVATE sqlite3::sqlite3
    )
endif()

# Build:
target_include_directories(DRAMSysLibrary
    PUBLIC src/common/third_party/DRAMPower/src/
    PUBLIC $ENV{SYSTEMC_HOME}/include/
)

if(EXISTS $ENV{SYSTEMC_HOME}/include/tlm/)
    target_include_directories(DRAMSysLibrary
        PUBLIC $ENV{SYSTEMC_HOME}/include/tlm/
    )
endif()

target_link_libraries(DRAMSysLibrary
    PUBLIC ${SYSTEMC_LIBRARY}
    PRIVATE DRAMPower
    PUBLIC DRAMSysConfiguration
)
