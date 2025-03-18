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
###      DRAMSys::libdramsys         ###
########################################

add_library(libdramsys
    DRAMSys/common/DebugManager.cpp
    DRAMSys/common/TlmRecorder.cpp
    DRAMSys/common/dramExtensions.cpp
    DRAMSys/common/utils.cpp
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
    DRAMSys/controller/ControllerRecordable.cpp
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
    DRAMSys/simulation/AddressDecoder.cpp
    DRAMSys/simulation/Arbiter.cpp
    DRAMSys/simulation/DRAMSys.cpp
    DRAMSys/simulation/Dram.cpp
    DRAMSys/simulation/DramRecordable.cpp
    DRAMSys/simulation/SimConfig.cpp
)

target_include_directories(libdramsys PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(libdramsys PUBLIC cxx_std_17)

target_compile_definitions(libdramsys
    PUBLIC
        $<$<BOOL:${DRAMPower_FOUND}>:DRAMPOWER>
)

target_link_libraries(libdramsys
    PUBLIC
        SystemC::systemc
        DRAMSys::config
        $<$<TARGET_EXISTS:DRAMPower::DRAMPower>:DRAMPower::DRAMPower>
    PRIVATE
        SQLite::SQLite3

)

add_library(DRAMSys::libdramsys ALIAS libdramsys)
