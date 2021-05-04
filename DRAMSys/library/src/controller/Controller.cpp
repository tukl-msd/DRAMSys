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
#include "Command.h"
#include "checker/CheckerDDR3.h"
#include "checker/CheckerDDR4.h"
#include "checker/CheckerDDR5.h"
#include "checker/CheckerWideIO.h"
#include "checker/CheckerLPDDR4.h"
#include "checker/CheckerWideIO2.h"
#include "checker/CheckerHBM2.h"
#include "checker/CheckerGDDR5.h"
#include "checker/CheckerGDDR5X.h"
#include "checker/CheckerGDDR6.h"
#include "scheduler/SchedulerFifo.h"
#include "scheduler/SchedulerFrFcfs.h"
#include "scheduler/SchedulerFrFcfsGrp.h"
#include "cmdmux/CmdMuxStrict.h"
#include "cmdmux/CmdMuxOldest.h"
#include "respqueue/RespQueueFifo.h"
#include "respqueue/RespQueueReorder.h"
#include "refresh/RefreshManagerDummy.h"
#include "refresh/RefreshManagerAllBank.h"
#include "refresh/RefreshManagerPerBank.h"
#include "refresh/RefreshManagerSameBank.h"
#include "powerdown/PowerDownManagerStaggered.h"
#include "powerdown/PowerDownManagerDummy.h"

using namespace tlm;

Controller::Controller(sc_module_name name) :
    ControllerIF(name)
{
    SC_METHOD(controllerMethod);
    sensitive << beginReqEvent << endRespEvent << controllerEvent << dataResponseEvent;

    Configuration &config = Configuration::getInstance();
    memSpec = config.memSpec;
    ranksNumberOfPayloads = std::vector<unsigned>(memSpec->numberOfRanks);

    thinkDelayFw = config.thinkDelayFw;
    thinkDelayBw = config.thinkDelayBw;
    phyDelayFw = config.phyDelayFw;
    phyDelayBw = config.phyDelayBw;

    // reserve buffer for command tuples
    readyCommands.reserve(memSpec->numberOfBanks);

    // instantiate timing checker
    if (memSpec->memoryType == MemSpec::MemoryType::DDR3)
        checker = new CheckerDDR3();
    else if (memSpec->memoryType == MemSpec::MemoryType::DDR4)
        checker = new CheckerDDR4();
    else if (memSpec->memoryType == MemSpec::MemoryType::DDR5)
        checker = new CheckerDDR5();
    else if (memSpec->memoryType == MemSpec::MemoryType::WideIO)
        checker = new CheckerWideIO();
    else if (memSpec->memoryType == MemSpec::MemoryType::LPDDR4)
        checker = new CheckerLPDDR4();
    else if (memSpec->memoryType == MemSpec::MemoryType::WideIO2)
        checker = new CheckerWideIO2();
    else if (memSpec->memoryType == MemSpec::MemoryType::HBM2)
        checker = new CheckerHBM2();
    else if (memSpec->memoryType == MemSpec::MemoryType::GDDR5)
        checker = new CheckerGDDR5();
    else if (memSpec->memoryType == MemSpec::MemoryType::GDDR5X)
        checker = new CheckerGDDR5X();
    else if (memSpec->memoryType == MemSpec::MemoryType::GDDR6)
        checker = new CheckerGDDR6();

    // instantiate scheduler and command mux
    if (config.scheduler == Configuration::Scheduler::Fifo)
        scheduler = new SchedulerFifo();
    else if (config.scheduler == Configuration::Scheduler::FrFcfs)
        scheduler = new SchedulerFrFcfs();
    else if (config.scheduler == Configuration::Scheduler::FrFcfsGrp)
        scheduler = new SchedulerFrFcfsGrp();

    if (config.cmdMux == Configuration::CmdMux::Oldest)
    {
        if (memSpec->hasRasAndCasBus())
            cmdMux = new CmdMuxOldestRasCas();
        else
            cmdMux = new CmdMuxOldest();
    }
    else if (config.cmdMux == Configuration::CmdMux::Strict)
    {
        if (memSpec->hasRasAndCasBus())
            cmdMux = new CmdMuxStrictRasCas();
        else
            cmdMux = new CmdMuxStrict();
    }

    if (config.respQueue == Configuration::RespQueue::Fifo)
        respQueue = new RespQueueFifo();
    else if (config.respQueue == Configuration::RespQueue::Reorder)
        respQueue = new RespQueueReorder();

    // instantiate bank machines (one per bank)
    if (config.pagePolicy == Configuration::PagePolicy::Open)
    {
        for (unsigned bankID = 0; bankID < memSpec->numberOfBanks; bankID++)
            bankMachines.push_back(new BankMachineOpen(scheduler, checker, Bank(bankID)));
    }
    else if (config.pagePolicy == Configuration::PagePolicy::OpenAdaptive)
    {
        for (unsigned bankID = 0; bankID < memSpec->numberOfBanks; bankID++)
            bankMachines.push_back(new BankMachineOpenAdaptive(scheduler, checker, Bank(bankID)));
    }
    else if (config.pagePolicy == Configuration::PagePolicy::Closed)
    {
        for (unsigned bankID = 0; bankID < memSpec->numberOfBanks; bankID++)
            bankMachines.push_back(new BankMachineClosed(scheduler, checker, Bank(bankID)));
    }
    else if (config.pagePolicy == Configuration::PagePolicy::ClosedAdaptive)
    {
        for (unsigned bankID = 0; bankID < memSpec->numberOfBanks; bankID++)
            bankMachines.push_back(new BankMachineClosedAdaptive(scheduler, checker, Bank(bankID)));
    }

    for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
    {
        bankMachinesOnRank.push_back(std::vector<BankMachine *>(bankMachines.begin() + rankID * memSpec->banksPerRank,
                bankMachines.begin() + (rankID + 1) * memSpec->banksPerRank));
    }

    // instantiate power-down managers (one per rank)
    if (config.powerDownPolicy == Configuration::PowerDownPolicy::NoPowerDown)
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
        {
            PowerDownManagerIF *manager = new PowerDownManagerDummy();
            powerDownManagers.push_back(manager);
        }
    }
    else if (config.powerDownPolicy == Configuration::PowerDownPolicy::Staggered)
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
        {
            PowerDownManagerIF *manager = new PowerDownManagerStaggered(Rank(rankID), checker);
            powerDownManagers.push_back(manager);
        }
    }

    // instantiate refresh managers (one per rank)
    if (config.refreshPolicy == Configuration::RefreshPolicy::NoRefresh)
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
            refreshManagers.push_back(new RefreshManagerDummy());
    }
    else if (config.refreshPolicy == Configuration::RefreshPolicy::AllBank)
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
        {
            RefreshManagerIF *manager = new RefreshManagerAllBank
                    (bankMachinesOnRank[rankID], powerDownManagers[rankID], Rank(rankID), checker);
            refreshManagers.push_back(manager);
        }
    }
    else if (config.refreshPolicy == Configuration::RefreshPolicy::SameBank)
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
        {
            RefreshManagerIF *manager = new RefreshManagerSameBank
                    (bankMachinesOnRank[rankID], powerDownManagers[rankID], Rank(rankID), checker);
            refreshManagers.push_back(manager);
        }
    }
    else if (config.refreshPolicy == Configuration::RefreshPolicy::PerBank)
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
        {
            // TODO: remove bankMachines in constructor
            RefreshManagerIF *manager = new RefreshManagerPerBank
                    (bankMachinesOnRank[rankID], powerDownManagers[rankID], Rank(rankID), checker);
            refreshManagers.push_back(manager);
        }
    }
    else
        SC_REPORT_FATAL("Controller", "Selected refresh mode not supported!");

    idleTimeCollector.start();
}

Controller::~Controller()
{
    idleTimeCollector.end();

    for (auto it : refreshManagers)
        delete it;
    for (auto it : powerDownManagers)
        delete it;
    for (auto it : bankMachines)
        delete it;
    delete respQueue;
    delete cmdMux;
    delete scheduler;
    delete checker;
}

void Controller::controllerMethod()
{
    // (1) Finish last response (END_RESP) and start new response (BEGIN_RESP)
    manageResponses();

    // (2) Insert new request into scheduler and send END_REQ or use backpressure
    manageRequests(SC_ZERO_TIME);

    // (3) Start refresh and power-down managers to issue requests for the current time
    for (auto it : refreshManagers)
        it->start();
    for (auto it : powerDownManagers)
        it->start();

    // (4) Collect all ready commands from BMs, RMs and PDMs
    CommandTuple::Type commandTuple;
    // clear command buffer
    readyCommands.clear();

    for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
    {
        // (4.1) Check for power-down commands (PDEA/PDEP/SREFEN or PDXA/PDXP/SREFEX)
        commandTuple = powerDownManagers[rankID]->getNextCommand();
        if (std::get<CommandTuple::Command>(commandTuple) != Command::NOP)
            readyCommands.emplace_back(commandTuple);
        else
        {
            // (4.2) Check for refresh commands (PREA/PRE or REFA/REFB)
            commandTuple = refreshManagers[rankID]->getNextCommand();
            if (std::get<CommandTuple::Command>(commandTuple) != Command::NOP)
                readyCommands.emplace_back(commandTuple);

            // (4.3) Check for bank commands (PRE, ACT, RD/RDA or WR/WRA)
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
            BankGroup bankgroup = DramExtension::getBankGroup(payload);
            Bank bank = DramExtension::getBank(payload);

            if (isRankCommand(command))
            {
                for (auto it : bankMachinesOnRank[rank.ID()])
                    it->updateState(command);
            }
            else if (isGroupCommand(command))
            {
                for (unsigned bankID = (bank.ID() % memSpec->banksPerGroup);
                        bankID < memSpec->banksPerRank; bankID += memSpec->banksPerGroup)
                    bankMachinesOnRank[rank.ID()][bankID]->updateState(command);
            }
            else // if (isBankCommand(command))
                bankMachines[bank.ID()]->updateState(command);

            refreshManagers[rank.ID()]->updateState(command);
            powerDownManagers[rank.ID()]->updateState(command);
            checker->insert(command, rank, bankgroup, bank);

            if (isCasCommand(command))
            {
                scheduler->removeRequest(payload);
                manageRequests(thinkDelayFw);
                respQueue->insertPayload(payload, sc_time_stamp()
                        + thinkDelayFw + phyDelayFw
                        + memSpec->getIntervalOnDataStrobe(command).end
                        + phyDelayBw + thinkDelayBw);

                sc_time triggerTime = respQueue->getTriggerTime();
                if (triggerTime != sc_max_time())
                    dataResponseEvent.notify(triggerTime - sc_time_stamp());

                ranksNumberOfPayloads[rank.ID()]--; // TODO: move to a different place?
            }
            if (ranksNumberOfPayloads[rank.ID()] == 0)
                powerDownManagers[rank.ID()]->triggerEntry();

            sendToDram(command, payload, thinkDelayFw + phyDelayFw);
        }
        else
            readyCmdBlocked = true;
    }

    // (6) Restart bank machines, refresh managers and power-down managers to issue new requests for the future
    sc_time timeForNextTrigger = sc_max_time();
    sc_time localTime;
    for (auto it : bankMachines)
    {
        localTime = it->start();
        if (!(localTime == sc_time_stamp() && readyCmdBlocked))
            timeForNextTrigger = std::min(timeForNextTrigger, localTime);
    }
    for (auto it : refreshManagers)
    {
        localTime = it->start();
        if (!(localTime == sc_time_stamp() && readyCmdBlocked))
            timeForNextTrigger = std::min(timeForNextTrigger, localTime);
    }
    for (auto it : powerDownManagers)
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

void Controller::manageRequests(sc_time delay)
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

            scheduler->storeRequest(transToAcquire.payload);
            transToAcquire.payload->acquire();

            Bank bank = DramExtension::getBank(transToAcquire.payload);
            bankMachines[bank.ID()]->start();

            transToAcquire.payload->set_response_status(TLM_OK_RESPONSE);
            sendToFrontend(transToAcquire.payload, END_REQ, delay);
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

            transToRelease.payload->release();
            transToRelease.payload = nullptr;
            numberOfTransactionsServed++;
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
                    sendToFrontend(transToRelease.payload, BEGIN_RESP, memSpec->tCK);
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
            if (transToRelease.time == sc_time_stamp()) // last payload was released in this cycle
                sendToFrontend(transToRelease.payload, BEGIN_RESP, memSpec->tCK);
            else
                sendToFrontend(transToRelease.payload, BEGIN_RESP, SC_ZERO_TIME);

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

void Controller::sendToFrontend(tlm_generic_payload *payload, tlm_phase phase, sc_time delay)
{
    tSocket->nb_transport_bw(*payload, phase, delay);
}

void Controller::sendToDram(Command command, tlm_generic_payload *payload, sc_time delay)
{
    tlm_phase phase = commandToPhase(command);
    iSocket->nb_transport_fw(*payload, phase, delay);
}
