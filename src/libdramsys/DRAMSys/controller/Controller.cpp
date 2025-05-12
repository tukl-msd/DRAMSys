/*
 * Copyright (c) 2019, RPTU Kaiserslautern-Landau
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *    Lukas Steiner
 *    Derek Christ
 *    Marco Mörz
 */

#include "Controller.h"

#include "DRAMSys/common/dramExtensions.h"
#include "DRAMSys/config/McConfig.h"
#include "DRAMSys/controller/checker/CheckerDDR3.h"
#include "DRAMSys/controller/checker/CheckerDDR4.h"
#include "DRAMSys/controller/checker/CheckerGDDR5.h"
#include "DRAMSys/controller/checker/CheckerGDDR5X.h"
#include "DRAMSys/controller/checker/CheckerGDDR6.h"
#include "DRAMSys/controller/checker/CheckerHBM2.h"
#include "DRAMSys/controller/checker/CheckerLPDDR4.h"
#include "DRAMSys/controller/checker/CheckerSTTMRAM.h"
#include "DRAMSys/controller/checker/CheckerWideIO.h"
#include "DRAMSys/controller/checker/CheckerWideIO2.h"
#include "DRAMSys/controller/cmdmux/CmdMuxOldest.h"
#include "DRAMSys/controller/cmdmux/CmdMuxStrict.h"
#include "DRAMSys/controller/powerdown/PowerDownManagerDummy.h"
#include "DRAMSys/controller/powerdown/PowerDownManagerStaggered.h"
#include "DRAMSys/controller/refresh/RefreshManagerAllBank.h"
#include "DRAMSys/controller/refresh/RefreshManagerDummy.h"
#include "DRAMSys/controller/refresh/RefreshManagerPer2Bank.h"
#include "DRAMSys/controller/refresh/RefreshManagerPerBank.h"
#include "DRAMSys/controller/refresh/RefreshManagerSameBank.h"
#include "DRAMSys/controller/respqueue/RespQueueFifo.h"
#include "DRAMSys/controller/respqueue/RespQueueReorder.h"
#include "DRAMSys/controller/scheduler/SchedulerFifo.h"
#include "DRAMSys/controller/scheduler/SchedulerFrFcfs.h"
#include "DRAMSys/controller/scheduler/SchedulerFrFcfsGrp.h"
#include "DRAMSys/controller/scheduler/SchedulerGrpFrFcfs.h"
#include "DRAMSys/controller/scheduler/SchedulerGrpFrFcfsWm.h"

#include <cstdint>
#include <numeric>
#include <string>

#ifdef DDR5_SIM
#include "DRAMSys/controller/checker/CheckerDDR5.h"
#endif
#ifdef LPDDR5_SIM
#include "DRAMSys/controller/checker/CheckerLPDDR5.h"
#endif
#ifdef HBM3_SIM
#include "DRAMSys/controller/checker/CheckerHBM3.h"
#endif

using namespace sc_core;
using namespace tlm;

namespace DRAMSys
{

Controller::Controller(const sc_module_name& name,
                       const McConfig& config,
                       const MemSpec& memSpec,
                       const SimConfig& simConfig,
                       const AddressDecoder& addressDecoder,
                       TlmRecorder* tlmRecorder) :
    sc_module(name),
    config(config),
    memSpec(memSpec),
    simConfig(simConfig),
    addressDecoder(addressDecoder),
    tlmRecorder(tlmRecorder),
    windowSizeTime(simConfig.windowSize * memSpec.tCK),
    nextWindowEventTime(windowSizeTime),
    numberOfBeatsServed(memSpec.ranksPerChannel, 0),
    minBytesPerBurst(memSpec.defaultBytesPerBurst),
    maxBytesPerBurst(memSpec.maxBytesPerBurst)
{
    if (simConfig.databaseRecording && tlmRecorder != nullptr)
    {
        SC_METHOD(recordBufferDepth);
        dont_initialize();
        sensitive << windowEvent;
        windowEvent.notify(windowSizeTime);
    }

    SC_METHOD(controllerMethod);
    sensitive << beginReqEvent << endRespEvent << controllerEvent << dataResponseEvent;

    tSocket.register_nb_transport_fw(this, &Controller::nb_transport_fw);
    tSocket.register_transport_dbg(this, &Controller::transport_dbg);
    tSocket.register_b_transport(this, &Controller::b_transport);
    iSocket.register_nb_transport_bw(this, &Controller::nb_transport_bw);

    idleTimeCollector.start();

    ranksNumberOfPayloads = ControllerVector<Rank, unsigned>(memSpec.ranksPerChannel);

    // reserve buffer for command tuples
    readyCommands.reserve(memSpec.banksPerChannel);

    // instantiate timing checker
    try
    {
        if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecDDR3::id)
        {
            checker = std::make_unique<CheckerDDR3>(dynamic_cast<const MemSpecDDR3&>(memSpec));
        }
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecDDR4::id)
        {
            checker = std::make_unique<CheckerDDR4>(dynamic_cast<const MemSpecDDR4&>(memSpec));
        }
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecWideIO::id)
        {
            checker = std::make_unique<CheckerWideIO>(dynamic_cast<const MemSpecWideIO&>(memSpec));
        }
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecLPDDR4::id)
        {
            checker = std::make_unique<CheckerLPDDR4>(dynamic_cast<const MemSpecLPDDR4&>(memSpec));
        }
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecWideIO2::id)
        {
            checker =
                std::make_unique<CheckerWideIO2>(dynamic_cast<const MemSpecWideIO2&>(memSpec));
        }
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecHBM2::id)
        {
            checker = std::make_unique<CheckerHBM2>(dynamic_cast<const MemSpecHBM2&>(memSpec));
        }
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecGDDR5::id)
        {
            checker = std::make_unique<CheckerGDDR5>(dynamic_cast<const MemSpecGDDR5&>(memSpec));
        }
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecGDDR5X::id)
        {
            checker = std::make_unique<CheckerGDDR5X>(dynamic_cast<const MemSpecGDDR5X&>(memSpec));
        }
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecGDDR6::id)
        {
            checker = std::make_unique<CheckerGDDR6>(dynamic_cast<const MemSpecGDDR6&>(memSpec));
        }
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecSTTMRAM::id)
        {
            checker =
                std::make_unique<CheckerSTTMRAM>(dynamic_cast<const MemSpecSTTMRAM&>(memSpec));
        }
#ifdef DDR5_SIM
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecDDR5::id)
        {
            checker = std::make_unique<CheckerDDR5>(dynamic_cast<const MemSpecDDR5&>(memSpec));
        }
#endif
#ifdef LPDDR5_SIM
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecLPDDR5::id)
        {
            checker = std::make_unique<CheckerLPDDR5>(dynamic_cast<const MemSpecLPDDR5&>(memSpec));
        }
#endif
#ifdef HBM3_SIM
        else if (memSpec.memoryType == DRAMUtils::MemSpec::MemSpecHBM3::id)
        {
            checker = std::make_unique<CheckerHBM3>(dynamic_cast<const MemSpecHBM3&>(memSpec));
        }
#endif
    }
    catch (const std::bad_cast& e)
    {
        SC_REPORT_FATAL(sc_module::name(), "Wrong MemSpec chosen");
    }

    // instantiate scheduler and command mux
    if (config.scheduler == Config::SchedulerType::Fifo)
        scheduler = std::make_unique<SchedulerFifo>(config, memSpec);
    else if (config.scheduler == Config::SchedulerType::FrFcfs)
        scheduler = std::make_unique<SchedulerFrFcfs>(config, memSpec);
    else if (config.scheduler == Config::SchedulerType::FrFcfsGrp)
        scheduler = std::make_unique<SchedulerFrFcfsGrp>(config, memSpec);
    else if (config.scheduler == Config::SchedulerType::GrpFrFcfs)
        scheduler = std::make_unique<SchedulerGrpFrFcfs>(config, memSpec);
    else if (config.scheduler == Config::SchedulerType::GrpFrFcfsWm)
        scheduler = std::make_unique<SchedulerGrpFrFcfsWm>(config, memSpec);

    if (config.cmdMux == Config::CmdMuxType::Oldest)
    {
        if (memSpec.hasRasAndCasBus())
            cmdMux = std::make_unique<CmdMuxOldestRasCas>(memSpec);
        else
            cmdMux = std::make_unique<CmdMuxOldest>(memSpec);
    }
    else if (config.cmdMux == Config::CmdMuxType::Strict)
    {
        if (memSpec.hasRasAndCasBus())
            cmdMux = std::make_unique<CmdMuxStrictRasCas>(memSpec);
        else
            cmdMux = std::make_unique<CmdMuxStrict>(memSpec);
    }

    if (config.respQueue == Config::RespQueueType::Fifo)
        respQueue = std::make_unique<RespQueueFifo>();
    else if (config.respQueue == Config::RespQueueType::Reorder)
        respQueue = std::make_unique<RespQueueReorder>();

    // instantiate bank machines (one per bank)
    if (config.pagePolicy == Config::PagePolicyType::Open)
    {
        for (unsigned bankID = 0; bankID < memSpec.banksPerChannel; bankID++)
            bankMachines.push_back(
                std::make_unique<BankMachineOpen>(config, memSpec, *scheduler, Bank(bankID)));
    }
    else if (config.pagePolicy == Config::PagePolicyType::OpenAdaptive)
    {
        for (unsigned bankID = 0; bankID < memSpec.banksPerChannel; bankID++)
            bankMachines.push_back(std::make_unique<BankMachineOpenAdaptive>(
                config, memSpec, *scheduler, Bank(bankID)));
    }
    else if (config.pagePolicy == Config::PagePolicyType::Closed)
    {
        for (unsigned bankID = 0; bankID < memSpec.banksPerChannel; bankID++)
            bankMachines.push_back(
                std::make_unique<BankMachineClosed>(config, memSpec, *scheduler, Bank(bankID)));
    }
    else if (config.pagePolicy == Config::PagePolicyType::ClosedAdaptive)
    {
        for (unsigned bankID = 0; bankID < memSpec.banksPerChannel; bankID++)
            bankMachines.push_back(std::make_unique<BankMachineClosedAdaptive>(
                config, memSpec, *scheduler, Bank(bankID)));
    }

    bankMachinesOnRank = ControllerVector<Rank, ControllerVector<Bank, BankMachine*>>(
        memSpec.ranksPerChannel, ControllerVector<Bank, BankMachine*>(memSpec.banksPerRank));
    for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
    {
        for (unsigned bankID = 0; bankID < memSpec.banksPerRank; bankID++)
            bankMachinesOnRank[Rank(rankID)][Bank(bankID)] =
                bankMachines[Bank(rankID * memSpec.banksPerRank + bankID)].get();
    }

    // instantiate power-down managers (one per rank)
    if (config.powerDownPolicy == Config::PowerDownPolicyType::NoPowerDown)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
            powerDownManagers.push_back(std::make_unique<PowerDownManagerDummy>());
    }
    else if (config.powerDownPolicy == Config::PowerDownPolicyType::Staggered)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
        {
            powerDownManagers.push_back(std::make_unique<PowerDownManagerStaggered>(
                bankMachinesOnRank[Rank(rankID)], Rank(rankID)));
        }
    }

    // instantiate refresh managers (one per rank)
    if (config.refreshPolicy == Config::RefreshPolicyType::NoRefresh)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
            refreshManagers.push_back(std::make_unique<RefreshManagerDummy>());
    }
    else if (config.refreshPolicy == Config::RefreshPolicyType::AllBank)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
        {
            refreshManagers.push_back(
                std::make_unique<RefreshManagerAllBank>(config,
                                                        memSpec,
                                                        bankMachinesOnRank[Rank(rankID)],
                                                        *powerDownManagers[Rank(rankID)],
                                                        Rank(rankID)));
        }
    }
    else if (config.refreshPolicy == Config::RefreshPolicyType::SameBank)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
        {
            refreshManagers.push_back(
                std::make_unique<RefreshManagerSameBank>(config,
                                                         memSpec,
                                                         bankMachinesOnRank[Rank(rankID)],
                                                         *powerDownManagers[Rank(rankID)],
                                                         Rank(rankID)));
        }
    }
    else if (config.refreshPolicy == Config::RefreshPolicyType::PerBank)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
        {
            // TODO: remove bankMachines in constructor
            refreshManagers.push_back(
                std::make_unique<RefreshManagerPerBank>(config,
                                                        memSpec,
                                                        bankMachinesOnRank[Rank(rankID)],
                                                        *powerDownManagers[Rank(rankID)],
                                                        Rank(rankID)));
        }
    }
    else if (config.refreshPolicy == Config::RefreshPolicyType::Per2Bank)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
        {
            // TODO: remove bankMachines in constructor
            refreshManagers.push_back(
                std::make_unique<RefreshManagerPer2Bank>(config,
                                                         memSpec,
                                                         bankMachinesOnRank[Rank(rankID)],
                                                         *powerDownManagers[Rank(rankID)],
                                                         Rank(rankID)));
        }
    }
    else
        SC_REPORT_FATAL("Controller", "Selected refresh mode not supported!");

    slidingAverageBufferDepth = std::vector<sc_time>(scheduler->getBufferDepth().size());
    windowAverageBufferDepth = std::vector<double>(scheduler->getBufferDepth().size());
}

void Controller::registerIdleCallback(std::function<void()> idleCallback)
{
    this->idleCallback = std::move(idleCallback);
}

void Controller::recordBufferDepth()
{
    windowEvent.notify(windowSizeTime);
    nextWindowEventTime += windowSizeTime;

    for (std::size_t index = 0; index < slidingAverageBufferDepth.size(); index++)
    {
        windowAverageBufferDepth[index] = slidingAverageBufferDepth[index] / windowSizeTime;
        slidingAverageBufferDepth[index] = SC_ZERO_TIME;
    }

    tlmRecorder->recordBufferDepth(sc_time_stamp().to_seconds(), windowAverageBufferDepth);
}

void Controller::controllerMethod()
{
    // Compute and report BufferDepth
    if (simConfig.databaseRecording && simConfig.enableWindowing)
    {
        sc_time timeDiff = sc_time_stamp() - lastTimeCalled;
        lastTimeCalled = sc_time_stamp();
        const std::vector<unsigned>& bufferDepth = scheduler->getBufferDepth();

        for (std::size_t index = 0; index < slidingAverageBufferDepth.size(); index++)
            slidingAverageBufferDepth[index] += bufferDepth[index] * timeDiff;
    }

    if (isFullCycle(sc_time_stamp(), memSpec.tCK))
    {
        // (1) Finish last response (END_RESP) and start new response (BEGIN_RESP)
        manageResponses();

        // (2) Insert new request into scheduler and send END_REQ or use backpressure
        manageRequests(SC_ZERO_TIME);
    }

    // (3) Start refresh and power-down managers to issue requests for the current time
    for (auto& it : refreshManagers)
        it->evaluate();
    for (auto& it : powerDownManagers)
        it->evaluate();

    // (4) Collect all ready commands from BMs, RMs and PDMs

    // clear command buffer
    readyCommands.clear();

    for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
    {
        // (4.1) Check for power-down commands (PDEA/PDEP/SREFEN or PDXA/PDXP/SREFEX)
        Rank rank = Rank(rankID);
        auto commandTuple = powerDownManagers[rank]->getNextCommand();
        if (std::get<CommandTuple::Command>(commandTuple) != Command::NOP)
            readyCommands.emplace_back(commandTuple);
        else
        {
            // (4.2) Check for refresh commands (PREXX or REFXX)
            commandTuple = refreshManagers[rank]->getNextCommand();
            if (std::get<CommandTuple::Command>(commandTuple) != Command::NOP)
                readyCommands.emplace_back(commandTuple);

            // (4.3) Check for bank commands (PREPB, ACT, RD/RDA or WR/WRA)
            for (auto* it : bankMachinesOnRank[rank])
            {
                commandTuple = it->getNextCommand();
                if (std::get<CommandTuple::Command>(commandTuple) != Command::NOP)
                    readyCommands.emplace_back(commandTuple);
            }
        }
    }

    // (5) Select one of the ready commands and issue it to the DRAM
    bool readyCmdBlocked = false;
    if (!readyCommands.empty())
    {
        for (auto& it : readyCommands)
        {
            Command command = std::get<CommandTuple::Command>(it);
            tlm_generic_payload* trans = std::get<CommandTuple::Payload>(it);
            std::get<CommandTuple::Timestamp>(it) =
                checker->timeToSatisfyConstraints(command, *trans);
        }
        auto commandTuple = cmdMux->selectCommand(readyCommands);
        if (commandTuple.has_value()) // can happen with FIFO strict
        {
            Command command = std::get<CommandTuple::Command>(*commandTuple);
            tlm_generic_payload* trans = std::get<CommandTuple::Payload>(*commandTuple);

            Rank rank = ControllerExtension::getRank(*trans);
            Bank bank = ControllerExtension::getBank(*trans);

            if (command.isRankCommand())
            {
                for (auto* it : bankMachinesOnRank[rank])
                    it->update(command);
            }
            else if (command.isGroupCommand())
            {
                for (std::size_t bankID = (static_cast<std::size_t>(bank) % memSpec.banksPerGroup);
                     bankID < memSpec.banksPerRank;
                     bankID += memSpec.banksPerGroup)
                    bankMachinesOnRank[rank][Bank(bankID)]->update(command);
            }
            else if (command.is2BankCommand())
            {
                bankMachines[bank]->update(command);
                bankMachines[Bank(static_cast<std::size_t>(bank) + memSpec.getPer2BankOffset())]
                    ->update(command);
            }
            else // if (isBankCommand(command))
                bankMachines[bank]->update(command);

            refreshManagers[rank]->update(command);
            powerDownManagers[rank]->update(command);
            checker->insert(command, *trans);

            if (command.isCasCommand())
            {
                scheduler->removeRequest(*trans);
                manageRequests(config.thinkDelayFw);
                respQueue->insertPayload(trans,
                                         sc_time_stamp() + config.phyDelayFw +
                                             memSpec.getIntervalOnDataStrobe(command, *trans).end +
                                             config.phyDelayBw + config.thinkDelayBw);

                sc_time triggerTime = respQueue->getTriggerTime();
                if (triggerTime != scMaxTime)
                    dataResponseEvent.notify(triggerTime - sc_time_stamp());

                ranksNumberOfPayloads[rank]--; // TODO: move to a different place?
            }
            if (ranksNumberOfPayloads[rank] == 0)
                powerDownManagers[rank]->triggerEntry();

            sc_time fwDelay = config.phyDelayFw;
            tlm_phase phase = command.toPhase();
            iSocket->nb_transport_fw(*trans, phase, fwDelay);
        }
        else
            readyCmdBlocked = true;
    }

    // (6) Restart bank machines, refresh managers and power-down managers to issue new requests for
    // the future
    sc_time timeForNextTrigger = scMaxTime;
    sc_time localTime;
    for (auto& it : bankMachines)
    {
        it->evaluate();
        auto commandTuple = it->getNextCommand();
        Command command = std::get<CommandTuple::Command>(commandTuple);
        tlm_generic_payload* trans = std::get<CommandTuple::Payload>(commandTuple);
        if (command != Command::NOP)
        {
            localTime = checker->timeToSatisfyConstraints(command, *trans);
            if (!(localTime == sc_time_stamp() && readyCmdBlocked))
                timeForNextTrigger = std::min(timeForNextTrigger, localTime);
        }
    }
    for (auto& it : refreshManagers)
    {
        it->evaluate();
        auto commandTuple = it->getNextCommand();
        Command command = std::get<CommandTuple::Command>(commandTuple);
        tlm_generic_payload* trans = std::get<CommandTuple::Payload>(commandTuple);
        if (command != Command::NOP)
        {
            localTime = checker->timeToSatisfyConstraints(command, *trans);
            if (!(localTime == sc_time_stamp() && readyCmdBlocked))
                timeForNextTrigger = std::min(timeForNextTrigger, localTime);
        }
        else
        {
            timeForNextTrigger = std::min(timeForNextTrigger, it->getTimeForNextTrigger());
        }
    }
    for (auto& it : powerDownManagers)
    {
        it->evaluate();
        auto commandTuple = it->getNextCommand();
        Command command = std::get<CommandTuple::Command>(commandTuple);
        tlm_generic_payload* trans = std::get<CommandTuple::Payload>(commandTuple);
        if (command != Command::NOP)
        {
            localTime = checker->timeToSatisfyConstraints(command, *trans);
            if (!(localTime == sc_time_stamp() && readyCmdBlocked))
                timeForNextTrigger = std::min(timeForNextTrigger, localTime);
        }
    }

    if (timeForNextTrigger != scMaxTime)
        controllerEvent.notify(timeForNextTrigger - sc_time_stamp());
}

tlm_sync_enum
Controller::nb_transport_fw(tlm_generic_payload& trans, tlm_phase& phase, sc_time& delay)
{
    if (phase == BEGIN_REQ)
    {
        transToAcquire.payload = &trans;
        transToAcquire.arrival = sc_time_stamp() + delay + config.thinkDelayFw;
        beginReqEvent.notify(delay + config.thinkDelayFw);
    }
    else if (phase == END_RESP)
    {
        transToRelease.arrival = sc_time_stamp() + delay;
        endRespEvent.notify(delay);
    }
    else
        SC_REPORT_FATAL("Controller",
                        "nb_transport_fw in controller was triggered with unknown phase");

    PRINTDEBUGMESSAGE(name(),
                      "[fw] " + getPhaseName(phase) + " notification in " + delay.to_string());

    return TLM_ACCEPTED;
}

tlm_sync_enum Controller::nb_transport_bw([[maybe_unused]] tlm_generic_payload& trans,
                                          [[maybe_unused]] tlm_phase& phase,
                                          [[maybe_unused]] sc_time& delay)
{
    SC_REPORT_FATAL("Controller", "nb_transport_bw of controller must not be called!");
    return TLM_ACCEPTED;
}

void Controller::b_transport(tlm_generic_payload& trans, sc_time& delay)
{
    iSocket->b_transport(trans, delay);
    delay += trans.is_write() ? config.blockingWriteDelay : config.blockingReadDelay;
}

unsigned int Controller::transport_dbg(tlm_generic_payload& trans)
{
    return iSocket->transport_dbg(trans);
}

void Controller::manageRequests(const sc_time& delay)
{
    if (transToAcquire.payload != nullptr && transToAcquire.arrival <= sc_time_stamp())
    {
        unsigned requiredBufferEntries =
            transToAcquire.payload->get_data_length() / memSpec.maxBytesPerBurst;
        if (scheduler->hasBufferSpace(requiredBufferEntries))
        {
            if (totalNumberOfPayloads == 0)
                idleTimeCollector.end();
            totalNumberOfPayloads++; // seems to be ok

            transToAcquire.payload->acquire();

            // The following logic assumes that transactions are naturally aligned
            const uint64_t address = transToAcquire.payload->get_address();
            const uint64_t dataLength = transToAcquire.payload->get_data_length();
            assert((dataLength & (dataLength - 1)) == 0); // Data length must be a power of 2
            assert(address % dataLength == 0);            // Check if naturally aligned

            if ((address / maxBytesPerBurst) ==
                ((address + transToAcquire.payload->get_data_length() - 1) / maxBytesPerBurst))
            {
                // continuous block of data that can be fetched with a single burst
                DecodedAddress decodedAddress =
                    addressDecoder.decodeAddress(transToAcquire.payload->get_address());
                ControllerExtension::setAutoExtension(
                    *transToAcquire.payload,
                    nextChannelPayloadIDToAppend++,
                    Rank(decodedAddress.rank),
                    Stack(decodedAddress.stack),
                    BankGroup(decodedAddress.bankgroup),
                    Bank(decodedAddress.bank),
                    Row(decodedAddress.row),
                    Column(decodedAddress.column),
                    (transToAcquire.payload->get_data_length() * 8) / memSpec.dataBusWidth);

                Rank rank = Rank(decodedAddress.rank);
                if (ranksNumberOfPayloads[rank] == 0)
                    powerDownManagers[rank]->triggerExit();
                ranksNumberOfPayloads[rank]++;

                scheduler->storeRequest(*transToAcquire.payload);
                Bank bank = Bank(decodedAddress.bank);
                bankMachines[bank]->evaluate();
            }
            else
            {
                createChildTranses(*transToAcquire.payload);
                const std::vector<tlm_generic_payload*>& childTranses =
                    transToAcquire.payload->get_extension<ParentExtension>()->getChildTranses();
                for (auto* childTrans : childTranses)
                {
                    Rank rank = ControllerExtension::getRank(*childTrans);
                    if (ranksNumberOfPayloads[rank] == 0)
                        powerDownManagers[rank]->triggerExit();
                    ranksNumberOfPayloads[rank]++;

                    scheduler->storeRequest(*childTrans);
                    Bank bank = ControllerExtension::getBank(*childTrans);
                    bankMachines[bank]->evaluate();
                }
            }

            transToAcquire.payload->set_response_status(TLM_OK_RESPONSE);
            tlm_phase bwPhase = END_REQ;
            sc_time bwDelay = delay;
            sendToFrontend(*transToAcquire.payload, bwPhase, bwDelay);
            transToAcquire.payload = nullptr;
        }
        else
        {
            PRINTDEBUGMESSAGE(name(), "Total number of payloads exceeded, backpressure!");
        }
    }
}

void Controller::manageResponses()
{
    if (transToRelease.payload != nullptr)
    {
        assert(transToRelease.arrival >= sc_time_stamp());
        if (transToRelease.arrival == sc_time_stamp()) // END_RESP completed
        {
            transToRelease.payload->release();
            transToRelease.payload = nullptr;
            totalNumberOfPayloads--;

            if (totalNumberOfPayloads == 0)
            {
                idleTimeCollector.start();

                if (idleCallback)
                {
                    idleCallback();
                }
            }
        }
        else
            return; // END_RESP not completed
    }

    tlm_generic_payload* nextTransInRespQueue = respQueue->nextPayload();
    if (nextTransInRespQueue != nullptr)
    {
        // Ignore ECC requests
        // TODO in future, use a tagging mechanism to distinguish between normal, ECC and maybe
        // masked requests
        if (nextTransInRespQueue->get_extension<EccExtension>() == nullptr)
        {
            auto rank = ControllerExtension::getRank(*nextTransInRespQueue);
            numberOfBeatsServed[static_cast<std::size_t>(rank)] +=
                ControllerExtension::getBurstLength(*nextTransInRespQueue);
        }

        if (ChildExtension::isChildTrans(*nextTransInRespQueue))
        {
            tlm_generic_payload& parentTrans =
                ChildExtension::getParentTrans(*nextTransInRespQueue);
            if (ParentExtension::notifyChildTransCompletion(parentTrans))
            {
                transToRelease.payload = &parentTrans;
                tlm_phase bwPhase = BEGIN_RESP;
                sc_time bwDelay = SC_ZERO_TIME;

                sendToFrontend(*transToRelease.payload, bwPhase, bwDelay);
                transToRelease.arrival = scMaxTime;
            }
            else
            {
                sc_time triggerTime = respQueue->getTriggerTime();
                if (triggerTime != scMaxTime)
                    dataResponseEvent.notify(triggerTime - sc_time_stamp());
            }
        }
        else
        {
            transToRelease.payload = nextTransInRespQueue;
            tlm_phase bwPhase = BEGIN_RESP;
            sc_time bwDelay = SC_ZERO_TIME;

            sendToFrontend(*transToRelease.payload, bwPhase, bwDelay);
            transToRelease.arrival = scMaxTime;
        }
    }
    else
    {
        sc_time triggerTime = respQueue->getTriggerTime();
        if (triggerTime != scMaxTime)
            dataResponseEvent.notify(triggerTime - sc_time_stamp());
    }
}

void Controller::sendToFrontend(tlm_generic_payload& trans, tlm_phase& phase, sc_time& delay)
{
    tSocket->nb_transport_bw(trans, phase, delay);
}

Controller::MemoryManager::~MemoryManager()
{
    while (!freePayloads.empty())
    {
        tlm_generic_payload* trans = freePayloads.top();
        freePayloads.pop();
        trans->reset();
        delete trans;
    }
}

tlm::tlm_generic_payload& Controller::MemoryManager::allocate()
{
    if (freePayloads.empty())
    {
        return *new tlm_generic_payload(this);
    }

    tlm_generic_payload* result = freePayloads.top();
    freePayloads.pop();
    return *result;
}

void Controller::MemoryManager::free(tlm::tlm_generic_payload* trans)
{
    freePayloads.push(trans);
}

void Controller::createChildTranses(tlm::tlm_generic_payload& parentTrans)
{
    std::vector<tlm_generic_payload*> childTranses;

    uint64_t startAddress = parentTrans.get_address() & ~(maxBytesPerBurst - UINT64_C(1));
    unsigned char* startDataPtr = parentTrans.get_data_ptr();
    unsigned numChildTranses = parentTrans.get_data_length() / maxBytesPerBurst;

    for (unsigned childId = 0; childId < numChildTranses; childId++)
    {
        tlm_generic_payload& childTrans = memoryManager.allocate();
        childTrans.acquire();
        childTrans.set_command(parentTrans.get_command());
        childTrans.set_address(startAddress + childId * maxBytesPerBurst);
        childTrans.set_data_length(maxBytesPerBurst);
        childTrans.set_data_ptr(startDataPtr + childId * maxBytesPerBurst);
        ChildExtension::setExtension(childTrans, parentTrans);
        childTranses.push_back(&childTrans);
    }

    if (startAddress != parentTrans.get_address())
    {
        tlm_generic_payload& firstChildTrans = *childTranses.front();
        firstChildTrans.set_address(firstChildTrans.get_address() + minBytesPerBurst);
        firstChildTrans.set_data_ptr(firstChildTrans.get_data_ptr() + minBytesPerBurst);
        firstChildTrans.set_data_length(minBytesPerBurst);
        tlm_generic_payload& lastChildTrans = memoryManager.allocate();
        lastChildTrans.acquire();
        lastChildTrans.set_command(parentTrans.get_command());
        lastChildTrans.set_address(startAddress + numChildTranses * maxBytesPerBurst);
        lastChildTrans.set_data_length(minBytesPerBurst);
        lastChildTrans.set_data_ptr(startDataPtr + numChildTranses * maxBytesPerBurst);
        ChildExtension::setExtension(lastChildTrans, parentTrans);
        childTranses.push_back(&lastChildTrans);
    }

    for (auto* childTrans : childTranses)
    {
        DecodedAddress decodedAddress = addressDecoder.decodeAddress(childTrans->get_address());
        ControllerExtension::setAutoExtension(*childTrans,
                                              nextChannelPayloadIDToAppend,
                                              Rank(decodedAddress.rank),
                                              Stack(decodedAddress.stack),
                                              BankGroup(decodedAddress.bankgroup),
                                              Bank(decodedAddress.bank),
                                              Row(decodedAddress.row),
                                              Column(decodedAddress.column),
                                              (childTrans->get_data_length() * 8) /
                                                  memSpec.dataBusWidth);
    }
    nextChannelPayloadIDToAppend++;
    ParentExtension::setExtension(parentTrans, std::move(childTranses));
}

void Controller::end_of_simulation()
{
    idleTimeCollector.end();

    std::uint64_t totalNumberOfBeatsServed =
        std::accumulate(numberOfBeatsServed.begin(), numberOfBeatsServed.end(), 0);

    sc_core::sc_time activeTime =
        static_cast<double>(totalNumberOfBeatsServed) / memSpec.dataRate * memSpec.tCK;

    // HBM specific, pseudo channels get averaged
    if (memSpec.pseudoChannelMode())
        activeTime /= memSpec.ranksPerChannel;

    double bandwidth = activeTime / sc_core::sc_time_stamp();
    double bandwidthWoIdle =
        activeTime / (sc_core::sc_time_stamp() - idleTimeCollector.getIdleTime());

    double maxBandwidth = (
        // fCK in GHz e.g. 1 [GHz] (tCK in ps):
        (1000 / memSpec.tCK.to_double())
        // DataRate e.g. 2
        * memSpec.dataRate
        // BusWidth e.g. 8 or 64
        * memSpec.bitWidth
        // Number of devices that form a rank, e.g., 8 on a DDR3 DIMM
        * memSpec.devicesPerRank);

    // HBM specific, one or two pseudo channels per channel
    if (memSpec.pseudoChannelMode())
        maxBandwidth *= memSpec.ranksPerChannel;

    std::cout << std::left << std::setw(24) << name() << std::string("  Total Time:     ")
              << sc_core::sc_time_stamp().to_string() << std::endl;
    std::cout << std::left << std::setw(24) << name() << std::string("  AVG BW:         ")
              << std::fixed << std::setprecision(2) << std::setw(6) << (bandwidth * maxBandwidth)
              << " Gb/s | " << std::setw(6) << (bandwidth * maxBandwidth / 8) << " GB/s | "
              << std::setw(6) << (bandwidth * 100) << " %" << std::endl;

    if (memSpec.ranksPerChannel > 1)
    {
        for (std::size_t i = 0; i < memSpec.ranksPerChannel; i++)
        {
            std::string baseName = memSpec.pseudoChannelMode() ? "pc" : "ra";
            std::string rankName = "." + baseName + std::to_string(i);

            sc_core::sc_time rankActiveTime =
                numberOfBeatsServed[i] * memSpec.tCK / memSpec.dataRate;
            double rankBandwidth = rankActiveTime / sc_core::sc_time_stamp();

            double rankMaxBandwidth = (
                // fCK in GHz e.g. 1 [GHz] (tCK in ps):
                (1000 / memSpec.tCK.to_double())
                // DataRate e.g. 2
                * memSpec.dataRate
                // BusWidth e.g. 8 or 64
                * memSpec.bitWidth
                // Number of devices that form a rank, e.g., 8 on a DDR3 DIMM
                * memSpec.devicesPerRank);

            std::string componentName = name() + rankName;

            std::cout << std::left << std::setw(24) << componentName
                      << std::string("  AVG BW:         ") << std::fixed << std::setprecision(2)
                      << std::setw(6) << (rankBandwidth * rankMaxBandwidth) << " Gb/s | "
                      << std::setw(6) << (rankBandwidth * rankMaxBandwidth / 8) << " GB/s | "
                      << std::setw(6) << (rankBandwidth * 100) << " %" << std::endl;
        }
    }

    std::cout << std::left << std::setw(24) << name() << std::string("  AVG BW\\IDLE:    ")
              << std::fixed << std::setprecision(2) << std::setw(6)
              << (bandwidthWoIdle * maxBandwidth) << " Gb/s | " << std::setw(6)
              << (bandwidthWoIdle * maxBandwidth / 8) << " GB/s | " << std::setw(6)
              << (bandwidthWoIdle * 100) << " %" << std::endl;
    std::cout << std::left << std::setw(24) << name() << std::string("  MAX BW:         ")
              << std::fixed << std::setprecision(2) << std::setw(6) << maxBandwidth << " Gb/s | "
              << std::setw(6) << maxBandwidth / 8 << " GB/s | " << std::setw(6) << 100.0 << " %"
              << std::endl;
}

void Controller::serialize(std::ostream& stream) const
{
    for (auto& refreshManager : refreshManagers)
    {
        refreshManager->serialize(stream);
    }
}

void Controller::deserialize(std::istream& stream)
{
    for (auto& refreshManager : refreshManagers)
    {
        refreshManager->deserialize(stream);
    }
}

} // namespace DRAMSys
