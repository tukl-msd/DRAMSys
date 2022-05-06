/*
 * Copyright (c) 2019, Technische Universität Kaiserslautern
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
 * Author: Lukas Steiner
 */

#include "Controller.h"

#include "../configuration/Configuration.h"
#include "../common/dramExtensions.h"

#include "checker/CheckerDDR3.h"
#include "checker/CheckerDDR4.h"
#include "checker/CheckerDDR5.h"
#include "checker/CheckerWideIO.h"
#include "checker/CheckerLPDDR4.h"
#include "checker/CheckerLPDDR5.h"
#include "checker/CheckerWideIO2.h"
#include "checker/CheckerHBM2.h"
#include "checker/CheckerGDDR5.h"
#include "checker/CheckerGDDR5X.h"
#include "checker/CheckerGDDR6.h"
#include "checker/CheckerSTTMRAM.h"
#include "scheduler/SchedulerFifo.h"
#include "scheduler/SchedulerFrFcfs.h"
#include "scheduler/SchedulerFrFcfsGrp.h"
#include "scheduler/SchedulerGrpFrFcfs.h"
#include "scheduler/SchedulerGrpFrFcfsWm.h"
#include "cmdmux/CmdMuxStrict.h"
#include "cmdmux/CmdMuxOldest.h"
#include "respqueue/RespQueueFifo.h"
#include "respqueue/RespQueueReorder.h"
#include "refresh/RefreshManagerDummy.h"
#include "refresh/RefreshManagerAllBank.h"
#include "refresh/RefreshManagerPerBank.h"
#include "refresh/RefreshManagerPer2Bank.h"
#include "refresh/RefreshManagerSameBank.h"
#include "powerdown/PowerDownManagerStaggered.h"
#include "powerdown/PowerDownManagerDummy.h"

using namespace sc_core;
using namespace tlm;

Controller::Controller(const sc_module_name& name, const Configuration& config) :
    ControllerIF(name, config), thinkDelayFw(config.thinkDelayFw), thinkDelayBw(config.thinkDelayBw),
    phyDelayFw(config.phyDelayFw), phyDelayBw(config.phyDelayBw)
{
    SC_METHOD(controllerMethod);
    sensitive << beginReqEvent << endRespEvent << controllerEvent << dataResponseEvent;
    
    ranksNumberOfPayloads = std::vector<unsigned>(memSpec.ranksPerChannel);

    // reserve buffer for command tuples
    readyCommands.reserve(memSpec.banksPerChannel);

    // instantiate timing checker
    if (memSpec.memoryType == MemSpec::MemoryType::DDR3)
        checker = std::make_unique<CheckerDDR3>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::DDR4)
        checker = std::make_unique<CheckerDDR4>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::DDR5)
        checker = std::make_unique<CheckerDDR5>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::WideIO)
        checker = std::make_unique<CheckerWideIO>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::LPDDR4)
        checker = std::make_unique<CheckerLPDDR4>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::LPDDR5)
        checker = std::make_unique<CheckerLPDDR5>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::WideIO2)
        checker = std::make_unique<CheckerWideIO2>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::HBM2)
        checker = std::make_unique<CheckerHBM2>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::GDDR5)
        checker = std::make_unique<CheckerGDDR5>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::GDDR5X)
        checker = std::make_unique<CheckerGDDR5X>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::GDDR6)
        checker = std::make_unique<CheckerGDDR6>(config);
    else if (memSpec.memoryType == MemSpec::MemoryType::STTMRAM)
        checker = std::make_unique<CheckerSTTMRAM>(config);

    // instantiate scheduler and command mux
    if (config.scheduler == Configuration::Scheduler::Fifo)
        scheduler = std::make_unique<SchedulerFifo>(config);
    else if (config.scheduler == Configuration::Scheduler::FrFcfs)
        scheduler = std::make_unique<SchedulerFrFcfs>(config);
    else if (config.scheduler == Configuration::Scheduler::FrFcfsGrp)
        scheduler = std::make_unique<SchedulerFrFcfsGrp>(config);
    else if (config.scheduler == Configuration::Scheduler::GrpFrFcfs)
        scheduler = std::make_unique<SchedulerGrpFrFcfs>(config);
    else if (config.scheduler == Configuration::Scheduler::GrpFrFcfsWm)
        scheduler = std::make_unique<SchedulerGrpFrFcfsWm>(config);

    if (config.cmdMux == Configuration::CmdMux::Oldest)
    {
        if (memSpec.hasRasAndCasBus())
            cmdMux = std::make_unique<CmdMuxOldestRasCas>(config);
        else
            cmdMux = std::make_unique<CmdMuxOldest>(config);
    }
    else if (config.cmdMux == Configuration::CmdMux::Strict)
    {
        if (memSpec.hasRasAndCasBus())
            cmdMux = std::make_unique<CmdMuxStrictRasCas>(config);
        else
            cmdMux = std::make_unique<CmdMuxStrict>(config);
    }

    if (config.respQueue == Configuration::RespQueue::Fifo)
        respQueue = std::make_unique<RespQueueFifo>();
    else if (config.respQueue == Configuration::RespQueue::Reorder)
        respQueue = std::make_unique<RespQueueReorder>();

    // instantiate bank machines (one per bank)
    if (config.pagePolicy == Configuration::PagePolicy::Open)
    {
        for (unsigned bankID = 0; bankID < memSpec.banksPerChannel; bankID++)
            bankMachines.emplace_back(std::make_unique<BankMachineOpen>
                    (config, *scheduler, *checker, Bank(bankID)));
    }
    else if (config.pagePolicy == Configuration::PagePolicy::OpenAdaptive)
    {
        for (unsigned bankID = 0; bankID < memSpec.banksPerChannel; bankID++)
            bankMachines.emplace_back(std::make_unique<BankMachineOpenAdaptive>
                    (config, *scheduler, *checker, Bank(bankID)));
    }
    else if (config.pagePolicy == Configuration::PagePolicy::Closed)
    {
        for (unsigned bankID = 0; bankID < memSpec.banksPerChannel; bankID++)
            bankMachines.emplace_back(std::make_unique<BankMachineClosed>
                    (config, *scheduler, *checker, Bank(bankID)));
    }
    else if (config.pagePolicy == Configuration::PagePolicy::ClosedAdaptive)
    {
        for (unsigned bankID = 0; bankID < memSpec.banksPerChannel; bankID++)
            bankMachines.emplace_back(std::make_unique<BankMachineClosedAdaptive>
                    (config, *scheduler, *checker, Bank(bankID)));
    }

    bankMachinesOnRank = std::vector<std::vector<BankMachine*>>(memSpec.ranksPerChannel,
            std::vector<BankMachine*>(memSpec.banksPerRank));
    for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
    {
        for (unsigned bankID = 0; bankID < memSpec.banksPerRank; bankID++)
            bankMachinesOnRank[rankID][bankID] = bankMachines[rankID * memSpec.banksPerRank + bankID].get();
    }

    // instantiate power-down managers (one per rank)
    if (config.powerDownPolicy == Configuration::PowerDownPolicy::NoPowerDown)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
            powerDownManagers.emplace_back(std::make_unique<PowerDownManagerDummy>());
    }
    else if (config.powerDownPolicy == Configuration::PowerDownPolicy::Staggered)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
        {
            powerDownManagers.emplace_back(std::make_unique<PowerDownManagerStaggered>(bankMachinesOnRank[rankID],
                    Rank(rankID), *checker));
        }
    }

    // instantiate refresh managers (one per rank)
    if (config.refreshPolicy == Configuration::RefreshPolicy::NoRefresh)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
            refreshManagers.emplace_back(std::make_unique<RefreshManagerDummy>());
    }
    else if (config.refreshPolicy == Configuration::RefreshPolicy::AllBank)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
        {
            refreshManagers.emplace_back(std::make_unique<RefreshManagerAllBank>
                    (config, bankMachinesOnRank[rankID], *powerDownManagers[rankID].get(), Rank(rankID), *checker));
        }
    }
    else if (config.refreshPolicy == Configuration::RefreshPolicy::SameBank)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
        {
            refreshManagers.emplace_back(std::make_unique<RefreshManagerSameBank>
                    (config, bankMachinesOnRank[rankID], *powerDownManagers[rankID].get(), Rank(rankID), *checker));
        }
    }
    else if (config.refreshPolicy == Configuration::RefreshPolicy::PerBank)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
        {
            // TODO: remove bankMachines in constructor
            refreshManagers.emplace_back(std::make_unique<RefreshManagerPerBank>
                    (config, bankMachinesOnRank[rankID], *powerDownManagers[rankID], Rank(rankID), *checker));
        }
    }
    else if (config.refreshPolicy == Configuration::RefreshPolicy::Per2Bank)
    {
        for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
        {
            // TODO: remove bankMachines in constructor
            refreshManagers.emplace_back(std::make_unique<RefreshManagerPer2Bank>
                    (config, bankMachinesOnRank[rankID], *powerDownManagers[rankID], Rank(rankID), *checker));
        }
    }
    else
        SC_REPORT_FATAL("Controller", "Selected refresh mode not supported!");
}

void Controller::controllerMethod()
{
    // (1) Finish last response (END_RESP) and start new response (BEGIN_RESP)
    manageResponses();

    // (2) Insert new request into scheduler and send END_REQ or use backpressure
    manageRequests(SC_ZERO_TIME);

    // (3) Start refresh and power-down managers to issue requests for the current time
    for (auto& it : refreshManagers)
        it->start();
    for (auto& it : powerDownManagers)
        it->start();

    // (4) Collect all ready commands from BMs, RMs and PDMs
    CommandTuple::Type commandTuple;
    // clear command buffer
    readyCommands.clear();

    for (unsigned rankID = 0; rankID < memSpec.ranksPerChannel; rankID++)
    {
        // (4.1) Check for power-down commands (PDEA/PDEP/SREFEN or PDXA/PDXP/SREFEX)
        commandTuple = powerDownManagers[rankID]->getNextCommand();
        if (std::get<CommandTuple::Command>(commandTuple) != Command::NOP)
            readyCommands.emplace_back(commandTuple);
        else
        {
            // (4.2) Check for refresh commands (PREXX or REFXX)
            commandTuple = refreshManagers[rankID]->getNextCommand();
            if (std::get<CommandTuple::Command>(commandTuple) != Command::NOP)
                readyCommands.emplace_back(commandTuple);

            // (4.3) Check for bank commands (PREPB, ACT, RD/RDA or WR/WRA)
            for (auto it : bankMachinesOnRank[rankID])
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
        commandTuple = cmdMux->selectCommand(readyCommands);
        Command command = std::get<CommandTuple::Command>(commandTuple);
        tlm_generic_payload *payload = std::get<CommandTuple::Payload>(commandTuple);
        if (command != Command::NOP) // can happen with FIFO strict
        {
            Rank rank = DramExtension::getRank(payload);
            Bank bank = DramExtension::getBank(payload);

            if (command.isRankCommand())
            {
                for (auto it : bankMachinesOnRank[rank.ID()])
                    it->updateState(command);
            }
            else if (command.isGroupCommand())
            {
                for (unsigned bankID = (bank.ID() % memSpec.banksPerGroup);
                        bankID < memSpec.banksPerRank; bankID += memSpec.banksPerGroup)
                    bankMachinesOnRank[rank.ID()][bankID]->updateState(command);
            }
            else if (command.is2BankCommand())
            {
                bankMachines[bank.ID()]->updateState(command);
                bankMachines[bank.ID() + memSpec.getPer2BankOffset()]->updateState(command);
            }
            else // if (isBankCommand(command))
                bankMachines[bank.ID()]->updateState(command);

            refreshManagers[rank.ID()]->updateState(command);
            powerDownManagers[rank.ID()]->updateState(command);
            checker->insert(command, *payload);

            if (command.isCasCommand())
            {
                scheduler->removeRequest(*payload);
                manageRequests(thinkDelayFw);
                respQueue->insertPayload(payload, sc_time_stamp()
                        + thinkDelayFw + phyDelayFw
                        + memSpec.getIntervalOnDataStrobe(command, *payload).end
                        + phyDelayBw + thinkDelayBw);

                sc_time triggerTime = respQueue->getTriggerTime();
                if (triggerTime != sc_max_time())
                    dataResponseEvent.notify(triggerTime - sc_time_stamp());

                ranksNumberOfPayloads[rank.ID()]--; // TODO: move to a different place?
            }
            if (ranksNumberOfPayloads[rank.ID()] == 0)
                powerDownManagers[rank.ID()]->triggerEntry();

            sc_time fwDelay = thinkDelayFw + phyDelayFw;
            sendToDram(command, *payload, fwDelay);
        }
        else
            readyCmdBlocked = true;
    }

    // (6) Restart bank machines, refresh managers and power-down managers to issue new requests for the future
    sc_time timeForNextTrigger = sc_max_time();
    sc_time localTime;
    for (auto& it : bankMachines)
    {
        localTime = it->start();
        if (!(localTime == sc_time_stamp() && readyCmdBlocked))
            timeForNextTrigger = std::min(timeForNextTrigger, localTime);
    }
    for (auto& it : refreshManagers)
    {
        localTime = it->start();
        if (!(localTime == sc_time_stamp() && readyCmdBlocked))
            timeForNextTrigger = std::min(timeForNextTrigger, localTime);
    }
    for (auto& it : powerDownManagers)
    {
        localTime = it->start();
        if (!(localTime == sc_time_stamp() && readyCmdBlocked))
            timeForNextTrigger = std::min(timeForNextTrigger, localTime);
    }

    if (timeForNextTrigger != sc_max_time())
        controllerEvent.notify(timeForNextTrigger - sc_time_stamp());
}

tlm_sync_enum Controller::nb_transport_fw(tlm_generic_payload &trans,
                              tlm_phase &phase, sc_time &delay)
{
    if (phase == BEGIN_REQ)
    {
        transToAcquire.payload = &trans;
        transToAcquire.time = sc_time_stamp() + delay;
        beginReqEvent.notify(delay);
    }
    else if (phase == END_RESP)
    {
        transToRelease.time = sc_time_stamp() + delay;
        endRespEvent.notify(delay);
    }
    else
        SC_REPORT_FATAL("Controller", "nb_transport_fw in controller was triggered with unknown phase");

    PRINTDEBUGMESSAGE(name(), "[fw] " + getPhaseName(phase) + " notification in " +
                      delay.to_string());

    return TLM_ACCEPTED;
}

tlm_sync_enum Controller::nb_transport_bw(tlm_generic_payload &,
                              tlm_phase &, sc_time &)
{
    SC_REPORT_FATAL("Controller", "nb_transport_bw of controller must not be called!");
    return TLM_ACCEPTED;
}

unsigned int Controller::transport_dbg(tlm_generic_payload &trans)
{
    return iSocket->transport_dbg(trans);
}

void Controller::manageRequests(const sc_time &delay)
{
    if (transToAcquire.payload != nullptr && transToAcquire.time <= sc_time_stamp())
    {
        if (scheduler->hasBufferSpace())
        {
            NDEBUG_UNUSED(uint64_t id) = DramExtension::getChannelPayloadID(transToAcquire.payload);
            PRINTDEBUGMESSAGE(name(), "Payload " + std::to_string(id) + " entered system.");

            if (totalNumberOfPayloads == 0)
                idleTimeCollector.end();
            totalNumberOfPayloads++;

            Rank rank = DramExtension::getRank(transToAcquire.payload);
            if (ranksNumberOfPayloads[rank.ID()] == 0)
                powerDownManagers[rank.ID()]->triggerExit();

            ranksNumberOfPayloads[rank.ID()]++;

            scheduler->storeRequest(*transToAcquire.payload);
            transToAcquire.payload->acquire();

            Bank bank = DramExtension::getBank(transToAcquire.payload);
            bankMachines[bank.ID()]->start();

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
        assert(transToRelease.time >= sc_time_stamp());
        if (transToRelease.time == sc_time_stamp())
        {
            NDEBUG_UNUSED(uint64_t id) = DramExtension::getChannelPayloadID(transToRelease.payload);
            PRINTDEBUGMESSAGE(name(), "Payload " + std::to_string(id) + " left system.");

            numberOfBeatsServed += DramExtension::getBurstLength(transToRelease.payload);
            transToRelease.payload->release();
            transToRelease.payload = nullptr;
            totalNumberOfPayloads--;

            if (totalNumberOfPayloads == 0)
            {
                idleTimeCollector.start();
            }
            else
            {
                transToRelease.payload = respQueue->nextPayload();

                if (transToRelease.payload != nullptr)
                {
                    // last payload was released in this cycle
                    tlm_phase bwPhase = BEGIN_RESP;
                    sc_time bwDelay = memSpec.tCK;
                    sendToFrontend(*transToRelease.payload, bwPhase, bwDelay);
                    transToRelease.time = sc_max_time();
                }
                else
                {
                    sc_time triggerTime = respQueue->getTriggerTime();
                    if (triggerTime != sc_max_time())
                        dataResponseEvent.notify(triggerTime - sc_time_stamp());
                }
            }
        }
    }
    else
    {
        transToRelease.payload = respQueue->nextPayload();

        if (transToRelease.payload != nullptr)
        {
            tlm_phase bwPhase = BEGIN_RESP;
            sc_time bwDelay;
            if (transToRelease.time == sc_time_stamp()) // last payload was released in this cycle
                bwDelay = memSpec.tCK;
            else
                bwDelay = SC_ZERO_TIME;

            sendToFrontend(*transToRelease.payload, bwPhase, bwDelay);
            transToRelease.time = sc_max_time();
        }
        else
        {
            sc_time triggerTime = respQueue->getTriggerTime();
            if (triggerTime != sc_max_time())
                dataResponseEvent.notify(triggerTime - sc_time_stamp());
        }
    }
}

void Controller::sendToFrontend(tlm_generic_payload& payload, tlm_phase& phase, sc_time& delay)
{
    tSocket->nb_transport_bw(payload, phase, delay);
}

void Controller::sendToDram(Command command, tlm_generic_payload& payload, sc_time& delay)
{
    tlm_phase phase = command.toPhase();
    iSocket->nb_transport_fw(payload, phase, delay);
}
