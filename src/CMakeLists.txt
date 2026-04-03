# Copyright (c) 2021, RPTU Kaiserslautern-Landau
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
#   Derek Christ
#   Thomas Psota

########################################
###        DRAMSys::DRAMSys          ###
########################################

add_library(dramsys
    DRAMSys/DRAMSys.cpp
    DRAMSys/common/DebugManager.cpp
    DRAMSys/common/TlmRecorder.cpp
    DRAMSys/common/TlmATRecorder.cpp
    DRAMSys/common/DramATRecorder.cpp
    DRAMSys/common/dramExtensions.cpp
    DRAMSys/common/utils.cpp
    DRAMSys/common/MemoryManager.cpp
    DRAMSys/configuration/json/DRAMSysConfiguration.cpp
    DRAMSys/configuration/memspec/MemSpec.cpp
    DRAMSys/configuration/memspec/MemSpecDDR3.cpp
    DRAMSys/configuration/memspec/MemSpecDDR4.cpp
    DRAMSys/configuration/memspec/MemSpecGDDR5.cpp
    DRAMSys/configuration/memspec/MemSpecGDDR5X.cpp
    DRAMSys/configuration/memspec/MemSpecGDDR6.cpp
    DRAMSys/configuration/memspec/MemSpecHBM2.cpp
    DRAMSys/configuration/memspec/MemSpecLPDDR4.cpp
    DRAMSys/configuration/memspec/MemSpecSTTMRAM.cpp
    DRAMSys/configuration/memspec/MemSpecWideIO.cpp
    DRAMSys/configuration/memspec/MemSpecWideIO2.cpp
    DRAMSys/controller/BankMachine.cpp
    DRAMSys/controller/Command.cpp
    DRAMSys/controller/Controller.cpp
    DRAMSys/controller/McConfig.cpp
    DRAMSys/controller/checker/CheckerDDR3.cpp
    DRAMSys/controller/checker/CheckerDDR4.cpp
    DRAMSys/controller/checker/CheckerGDDR5.cpp
    DRAMSys/controller/checker/CheckerGDDR5X.cpp
    DRAMSys/controller/checker/CheckerGDDR6.cpp
    DRAMSys/controller/checker/CheckerHBM2.cpp
    DRAMSys/controller/checker/CheckerLPDDR4.cpp
    DRAMSys/controller/checker/CheckerSTTMRAM.cpp
    DRAMSys/controller/checker/CheckerWideIO.cpp
    DRAMSys/controller/checker/CheckerWideIO2.cpp
    DRAMSys/controller/cmdmux/CmdMuxOldest.cpp
    DRAMSys/controller/cmdmux/CmdMuxStrict.cpp
    DRAMSys/controller/powerdown/PowerDownManagerDummy.cpp
    DRAMSys/controller/powerdown/PowerDownManagerStaggered.cpp
    DRAMSys/controller/refresh/RefreshManagerAllBank.cpp
    DRAMSys/controller/refresh/RefreshManagerDummy.cpp
    DRAMSys/controller/refresh/RefreshManagerPer2Bank.cpp
    DRAMSys/controller/refresh/RefreshManagerPerBank.cpp
    DRAMSys/controller/refresh/RefreshManagerSameBank.cpp
    DRAMSys/controller/respqueue/RespQueueFifo.cpp
    DRAMSys/controller/respqueue/RespQueueReorder.cpp
    DRAMSys/controller/scheduler/BufferCounterBankwise.cpp
    DRAMSys/controller/scheduler/BufferCounterReadWrite.cpp
    DRAMSys/controller/scheduler/BufferCounterShared.cpp
    DRAMSys/controller/scheduler/SchedulerFifo.cpp
    DRAMSys/controller/scheduler/SchedulerFrFcfs.cpp
    DRAMSys/controller/scheduler/SchedulerFrFcfsGrp.cpp
    DRAMSys/controller/scheduler/SchedulerGrpFrFcfs.cpp
    DRAMSys/controller/scheduler/SchedulerGrpFrFcfsWm.cpp
    DRAMSys/ecc/InlineEcc.cpp
    DRAMSys/ecc/OrinScheme.cpp
    DRAMSys/ecc/TwoLevelScheme.cpp
    DRAMSys/power/DRAMPowerAdapter.cpp
    DRAMSys/simulation/AddressDecoder.cpp
    DRAMSys/simulation/Arbiter.cpp
    DRAMSys/simulation/Dram.cpp
    DRAMSys/simulation/SimConfig.cpp
)

add_library(DRAMSys::DRAMSys ALIAS dramsys)

set_target_properties(dramsys PROPERTIES
    EXPORT_NAME DRAMSys
)

target_sources(dramsys
PUBLIC
    FILE_SET HEADERS
FILES
    DRAMSys/DRAMSys.h
    DRAMSys/configuration/json/AddressMapping.h
    DRAMSys/configuration/json/DRAMSysConfiguration.h
    DRAMSys/configuration/json/McConfig.h
    DRAMSys/configuration/json/MemSpec.h
    DRAMSys/configuration/json/SimConfig.h
    DRAMSys/configuration/json/TraceSetup.h
)

target_include_directories(dramsys PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(dramsys PUBLIC cxx_std_17)

target_link_libraries(dramsys
    PUBLIC
        SystemC::systemc
        DRAMUtils::DRAMUtils
    PRIVATE
        SQLite3::SQLite3
        fmt::fmt
)

if (DRAMSYS_USE_DRAMPOWER)
    find_package(DRAMPower REQUIRED)
    target_link_libraries(dramsys PUBLIC DRAMPower::DRAMPower)
    target_compile_definitions(dramsys PUBLIC USE_DRAMPOWER)
endif()

if (DRAMSYS_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(
        TARGETS dramsys
        EXPORT  DRAMSysTargets
        FILE_SET HEADERS
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        EXPORT DRAMSysTargets
        FILE DRAMSysTargets.cmake
        NAMESPACE DRAMSys::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DRAMSys
    )

    configure_package_config_file(
        ${CMAKE_SOURCE_DIR}/cmake/DRAMSysConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/DRAMSysConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DRAMSys
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/DRAMSysConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/DRAMSysConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/DRAMSysConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DRAMSys
    )
endif()
