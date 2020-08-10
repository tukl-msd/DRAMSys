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
#include "refresh/RefreshManagerRankwise.h"
#include "refresh/RefreshManagerDummy.h"
#include "refresh/RefreshManagerBankwise.h"
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

    // instantiate timing checker
    if (memSpec->memoryType == "DDR3")
        checker = new CheckerDDR3();
    else if (memSpec->memoryType == "DDR4")
        checker = new CheckerDDR4();
    else if (memSpec->memoryType == "WIDEIO_SDR")
        checker = new CheckerWideIO();
    else if (memSpec->memoryType == "LPDDR4")
        checker = new CheckerLPDDR4();
    else if (memSpec->memoryType == "WIDEIO2")
        checker = new CheckerWideIO2();
    else if (memSpec->memoryType == "HBM2")
        checker = new CheckerHBM2();
    else if (memSpec->memoryType == "GDDR5")
        checker = new CheckerGDDR5();
    else if (memSpec->memoryType == "GDDR5X")
        checker = new CheckerGDDR5X();
    else if (memSpec->memoryType == "GDDR6")
        checker = new CheckerGDDR6();
    else
        SC_REPORT_FATAL("Controller", "Unsupported DRAM type!");

    // instantiate scheduler and command mux
    if (config.scheduler == "Fifo")
        scheduler = new SchedulerFifo();
    else if (config.scheduler == "FrFcfs")
        scheduler = new SchedulerFrFcfs();
    else if (config.scheduler == "FrFcfsGrp")
        scheduler = new SchedulerFrFcfsGrp();
    else
        SC_REPORT_FATAL("Controller", "Selected scheduler not supported!");

    if (config.cmdMux == "Oldest")
        cmdMux = new CmdMuxOldest();
    else if (config.cmdMux == "Strict")
        cmdMux = new CmdMuxStrict();
    else
        SC_REPORT_FATAL("Controller", "Selected cmdmux not supported!");

    if (config.respQueue == "Fifo")
        respQueue = new RespQueueFifo();
    else if (config.respQueue == "Reorder")
        respQueue = new RespQueueReorder();
    else
        SC_REPORT_FATAL("Controller", "Selected respqueue not supported!");

    // instantiate bank machines (one per bank)
    if (config.pagePolicy == "Open")
    {
        for (unsigned bankID = 0; bankID < memSpec->numberOfBanks; bankID++)
            bankMachines.push_back(new BankMachineOpen(scheduler, checker, Bank(bankID)));
    }
    else if (config.pagePolicy == "OpenAdaptive")
    {
        for (unsigned bankID = 0; bankID < memSpec->numberOfBanks; bankID++)
            bankMachines.push_back(new BankMachineOpenAdaptive(scheduler, checker, Bank(bankID)));
    }
    else if (config.pagePolicy == "Closed")
    {
        for (unsigned bankID = 0; bankID < memSpec->numberOfBanks; bankID++)
            bankMachines.push_back(new BankMachineClosed(scheduler, checker, Bank(bankID)));
    }
    else if (config.pagePolicy == "ClosedAdaptive")
    {
        for (unsigned bankID = 0; bankID < memSpec->numberOfBanks; bankID++)
            bankMachines.push_back(new BankMachineClosedAdaptive(scheduler, checker, Bank(bankID)));
    }
    else
        SC_REPORT_FATAL("Controller", "Selected page policy not supported!");

    for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
    {
        bankMachinesOnRank.push_back(std::vector<BankMachine *>(bankMachines.begin() + rankID * memSpec->banksPerRank,
                bankMachines.begin() + (rankID + 1) * memSpec->banksPerRank));
    }

    // instantiate power-down managers (one per rank)
    if (config.powerDownPolicy == "NoPowerDown")
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
        {
            PowerDownManagerIF *manager = new PowerDownManagerDummy();
            powerDownManagers.push_back(manager);
        }
    }
    else if (config.powerDownPolicy == "Staggered")
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
        {
            PowerDownManagerIF *manager = new PowerDownManagerStaggered(Rank(rankID), checker);
            powerDownManagers.push_back(manager);
        }
    }
    else
        SC_REPORT_FATAL("Controller", "Selected power-down mode not supported!");

    // instantiate refresh managers (one per rank)
    if (config.refreshPolicy == "NoRefresh")
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
            refreshManagers.push_back(new RefreshManagerDummy());
    }
    else if (config.refreshPolicy == "Rankwise")
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
        {
            RefreshManagerIF *manager = new RefreshManagerRankwise
                    (bankMachinesOnRank[rankID], powerDownManagers[rankID], Rank(rankID), checker);
            refreshManagers.push_back(manager);
        }
    }
    else if (config.refreshPolicy == "Bankwise")
    {
        for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
        {
            // TODO: remove bankMachines in constructor
            RefreshManagerIF *manager = new RefreshManagerBankwise
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
    // (1) Release payload if arbiter has accepted the result (finish END_RESP)
    if (payloadToRelease != nullptr && timeToRelease <= sc_time_stamp())
        finishEndResp();

    // (2) Send next result to arbiter (start BEGIN_RESP)
    if (payloadToRelease == nullptr)
        startBeginResp();

    // (3) Insert new request from arbiter into scheduler and restart appropriate BM (finish BEGIN_REQ)
    if (payloadToAcquire != nullptr && timeToAcquire <= sc_time_stamp())
    {
        unsigned bankID = DramExtension::getBank(payloadToAcquire).ID();
        finishBeginReq();
        bankMachines[bankID]->start();
    }

    // (4) Start refresh and power-down managers to issue requests for the current time
    for (auto it : refreshManagers)
        it->start();
    for (auto it : powerDownManagers)
        it->start();

    // (5) Choose one request and send it to DRAM
    std::tuple<Command, tlm_generic_payload *, sc_time> commandTuple;
    std::list<std::tuple<Command, tlm_generic_payload *, sc_time>> readyCommands;
    for (unsigned rankID = 0; rankID < memSpec->numberOfRanks; rankID++)
    {
        // (5.1) Check for power-down commands (PDEA/PDEP/SREFEN or PDXA/PDXP/SREFEX)
        commandTuple = powerDownManagers[rankID]->getNextCommand();
        if (std::get<0>(commandTuple) != Command::NOP)
            readyCommands.push_back(commandTuple);
        else
        {
            // (5.2) Check for refresh commands (PREA/PRE or REFA/REFB)
            commandTuple = refreshManagers[rankID]->getNextCommand();
            if (std::get<0>(commandTuple) != Command::NOP)
                readyCommands.push_back(commandTuple);

            // (5.3) Check for bank commands (PRE, ACT, RD/RDA or WR/WRA)
            for (auto it : bankMachinesOnRank[rankID])
            {
                commandTuple = it->getNextCommand();
                if (std::get<0>(commandTuple) != Command::NOP)
                    readyCommands.push_back(commandTuple);
            }
        }
    }

    bool readyCmdBlocked = false;
    if (!readyCommands.empty())
    {
        commandTuple = cmdMux->selectCommand(readyCommands);
        if (std::get<0>(commandTuple) != Command::NOP) // can happen with FIFO strict
        {
            Rank rank = DramExtension::getRank(std::get<1>(commandTuple));
            BankGroup bankgroup = DramExtension::getBankGroup(std::get<1>(commandTuple));
            Bank bank = DramExtension::getBank(std::get<1>(commandTuple));

            if (isRankCommand(std::get<0>(commandTuple)))
            {
                for (auto it : bankMachinesOnRank[rank.ID()])
                    it->updateState(std::get<0>(commandTuple));
            }
            else
                bankMachines[bank.ID()]->updateState(std::get<0>(commandTuple));

            refreshManagers[rank.ID()]->updateState(std::get<0>(commandTuple));
            powerDownManagers[rank.ID()]->updateState(std::get<0>(commandTuple));
            checker->insert(std::get<0>(commandTuple), rank, bankgroup, bank);

            if (isCasCommand(std::get<0>(commandTuple)))
            {
                scheduler->removeRequest(std::get<1>(commandTuple));
                respQueue->insertPayload(std::get<1>(commandTuple), memSpec->getIntervalOnDataStrobe(std::get<0>(commandTuple)).end);

                sc_time triggerTime = respQueue->getTriggerTime();
                if (triggerTime != sc_max_time())
                    dataResponseEvent.notify(triggerTime - sc_time_stamp());

                ranksNumberOfPayloads[rank.ID()]--;
            }
            if (ranksNumberOfPayloads[rank.ID()] == 0)
                powerDownManagers[rank.ID()]->triggerEntry();

            sendToDram(std::get<0>(commandTuple), std::get<1>(commandTuple));
        }
        else
            readyCmdBlocked = true;
    }

    // (6) Accept request from arbiter if scheduler is not full, otherwise backpressure (start END_REQ)
    if (payloadToAcquire != nullptr && timeToAcquire == sc_max_time())
        startEndReq();

    // (7) Restart bank machines, refresh managers and power-down managers to issue new requests for the future
    // TODO: check if all calls are necessary
    sc_time timeForNextTrigger = sc_max_time();
    for (auto it : bankMachines)
    {
        sc_time localTime = it->start();
        if (!(localTime == sc_time_stamp() && readyCmdBlocked))
            timeForNextTrigger = std::min(timeForNextTrigger, localTime);
    }
    for (auto it : refreshManagers)
        timeForNextTrigger = std::min(timeForNextTrigger, it->start());
    for (auto it : powerDownManagers)
        timeForNextTrigger = std::min(timeForNextTrigger, it->start());

    if (timeForNextTrigger != sc_max_time())
        controllerEvent.notify(timeForNextTrigger - sc_time_stamp());
}

tlm_sync_enum Controller::nb_transport_fw(tlm_generic_payload &trans,
                              tlm_phase &phase, sc_time &delay)
{
    sc_time notificationDelay = delay + Configuration::getInstance().memSpec->tCK;

    if (phase == BEGIN_REQ)
    {
        payloadToAcquire = &trans;
        timeToAcquire = sc_time_stamp() + notificationDelay;
        beginReqEvent.notify(notificationDelay);
    }
    else if (phase == END_RESP)
    {
        timeToRelease = sc_time_stamp() + notificationDelay;
        endRespEvent.notify(notificationDelay);
    }
    else
        SC_REPORT_FATAL("Controller", "nb_transport_fw in controller was triggered with unknown phase");

    PRINTDEBUGMESSAGE(name(), "[fw] " + getPhaseName(phase) + " notification in " +
                      notificationDelay.to_string());

    return TLM_ACCEPTED;
}

tlm_sync_enum Controller::nb_transport_bw(tlm_generic_payload &,
                              tlm_phase &, sc_time &)
{
    SC_REPORT_FATAL("Controller", "nb_transport_bw of controller must not be called");
    return TLM_ACCEPTED;
}

unsigned int Controller::transport_dbg(tlm_generic_payload &trans)
{
    return iSocket->transport_dbg(trans);
}

void Controller::finishBeginReq()
{
    uint64_t id __attribute__((unused)) = DramExtension::getPayloadID(payloadToAcquire);
    PRINTDEBUGMESSAGE(name(), "Payload " + std::to_string(id) + " entered system.");

    if (totalNumberOfPayloads == 0)
        idleTimeCollector.end();
    totalNumberOfPayloads++;

    Rank rank = DramExtension::getRank(payloadToAcquire);
    if (ranksNumberOfPayloads[rank.ID()] == 0)
        powerDownManagers[rank.ID()]->triggerExit();

    ranksNumberOfPayloads[rank.ID()]++;

    scheduler->storeRequest(payloadToAcquire);
    payloadToAcquire->acquire();
    timeToAcquire = sc_max_time();
}

void Controller::startEndReq()
{
    if (scheduler->hasBufferSpace())
    {
        payloadToAcquire->set_response_status(TLM_OK_RESPONSE);
        sendToFrontend(payloadToAcquire, END_REQ);
        payloadToAcquire = nullptr;
    }
    else
        PRINTDEBUGMESSAGE(name(), "Total number of payloads exceeded, backpressure!");
}

void Controller::startBeginResp()
{
    payloadToRelease = respQueue->nextPayload();

    if (payloadToRelease != nullptr)
        sendToFrontend(payloadToRelease, BEGIN_RESP);
    else
    {
        sc_time triggerTime = respQueue->getTriggerTime();
        if (triggerTime != sc_max_time())
            dataResponseEvent.notify(triggerTime - sc_time_stamp());
    }
}

void Controller::finishEndResp()
{
    uint64_t id __attribute__((unused)) = DramExtension::getPayloadID(payloadToRelease);
    PRINTDEBUGMESSAGE(name(), "Payload " + std::to_string(id) + " left system.");

    payloadToRelease->release();
    payloadToRelease = nullptr;
    timeToRelease = sc_max_time();
    numberOfTransactionsServed++;

    totalNumberOfPayloads--;
    if (totalNumberOfPayloads == 0)
        idleTimeCollector.start();
}

void Controller::sendToFrontend(tlm_generic_payload *payload, tlm_phase phase)
{
    sc_time delay = SC_ZERO_TIME;
    tSocket->nb_transport_bw(*payload, phase, delay);
}

void Controller::sendToDram(Command command, tlm_generic_payload *payload)
{
    sc_time delay = SC_ZERO_TIME;
    tlm_phase phase = commandToPhase(command);

    iSocket->nb_transport_fw(*payload, phase, delay);
}
