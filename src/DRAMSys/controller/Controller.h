/* * Copyright (c) 2019, RPTU Kaiserslautern-Landau
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
 *    Thomas Zimmermann
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "DRAMSys/common/DebugManager.h"
#include "DRAMSys/common/Deserialize.h"
#include "DRAMSys/common/MemoryManager.h"
#include "DRAMSys/common/Serialize.h"
#include "DRAMSys/common/TlmRecorder.h"
#include "DRAMSys/controller/BankMachine.h"
#include "DRAMSys/controller/Command.h"
#include "DRAMSys/controller/McConfig.h"
#include "DRAMSys/controller/checker/CheckerIF.h"
#include "DRAMSys/controller/cmdmux/CmdMuxIF.h"
#include "DRAMSys/controller/powerdown/PowerDownManagerIF.h"
#include "DRAMSys/controller/refresh/RefreshManagerIF.h"
#include "DRAMSys/controller/respqueue/RespQueueIF.h"
#include "DRAMSys/simulation/AddressDecoder.h"
#include "DRAMSys/simulation/SimConfig.h"
#include "DRAMSys/statistics/Group.h"
#include "DRAMSys/statistics/Stat.h"
#include "DRAMSys/statistics/StatProvider.h"

#include <DRAMUtils/memspec/MemSpec.h>

#include <functional>
#include <memory>
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

namespace DRAMSys
{

class Controller : public sc_core::sc_module,
                   public Serialize,
                   public Deserialize,
                   public Statistics::StatProvider
{
public:
    tlm_utils::simple_target_socket<Controller> tSocket{"tSocket"};

    SC_HAS_PROCESS(Controller);
    Controller(const sc_core::sc_module_name& name,
               const McConfig& config,
               const DRAMUtils::MemSpec::MemSpecVariant& memSpecVar,
               const MemSpec& memSpec,
               const SimConfig& simConfig,
               const AddressDecoder& addressDecoder,
               TlmRecorder* tlmRecorder);

    [[nodiscard]] bool idle() const { return totalNumberOfPayloads == 0; }

    void registerIdleCallback(std::function<void()> idleCallback);
    void registerAccessCallback(std::function<void(tlm::tlm_generic_payload&)> accessCallback);
    void registerTraceCallback(std::function<void(tlm::tlm_generic_payload const&,
                                                  tlm::tlm_phase const&,
                                                  sc_core::sc_time const&)> traceCallback);

    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;

    [[nodiscard]] double getAverageBandwidthPerRank(std::size_t rank) const;
    [[nodiscard]] double getAverageBandwidth() const;

    void updateStats() override;
    void resetStats() override;
    Statistics::Group const& getStatGroup() const override { return stats; }

private:
    void end_of_simulation() override;

    virtual tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                               tlm::tlm_phase& phase,
                                               sc_core::sc_time& delay);
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);

    virtual void
    sendToFrontend(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay);

    virtual void controllerMethod();
    void recordBufferDepth();

    const McConfig& config;
    const MemSpec& memSpec;
    const SimConfig& simConfig;
    const AddressDecoder& addressDecoder;
    TlmRecorder* const tlmRecorder;

    std::unique_ptr<SchedulerIF> scheduler;

    /**
     * @brief The gearing determines the divider for the controller frontend frequency.
     * @note If set to 1, the controller is aligned with the command clock (tCK).
    */
    unsigned gearing;

    sc_core::sc_time lastTimeCalled = sc_core::SC_ZERO_TIME;
    const sc_core::sc_time windowSizeTime;
    std::vector<sc_core::sc_time> slidingAverageBufferDepth;
    std::vector<double> windowAverageBufferDepth;

    std::vector<uint64_t> numberOfBeatsServed;
    unsigned totalNumberOfPayloads = 0;
    std::function<void()> idleCallback;
    ControllerVector<Rank, unsigned> ranksNumberOfPayloads;

    ControllerVector<Bank, std::unique_ptr<BankMachine>> bankMachines;
    ControllerVector<Rank, ControllerVector<Bank, BankMachine*>> bankMachinesOnRank;
    std::unique_ptr<CmdMuxIF> cmdMux;
    std::unique_ptr<CheckerIF> checker;
    std::unique_ptr<RespQueueIF> respQueue;
    ControllerVector<Rank, std::unique_ptr<RefreshManagerIF>> refreshManagers;
    ControllerVector<Rank, std::unique_ptr<PowerDownManagerIF>> powerDownManagers;

    std::function<void(tlm::tlm_generic_payload&)> accessCallback;
    std::function<void(
        tlm::tlm_generic_payload const&, tlm::tlm_phase const&, sc_core::sc_time const&)>
        traceCallback;

    uint64_t nextChannelPayloadIDToAppend = 1;

    struct PayloadAndArrival
    {
        tlm::tlm_generic_payload* payload = nullptr;
        sc_core::sc_time arrival = sc_core::sc_max_time();
    } transToAcquire, transToRelease;

    void manageResponses();
    void processNextTransInRespQueue(tlm::tlm_generic_payload* nextTransInRespQueue);
    void processNextChildRespTrans(tlm::tlm_generic_payload* nextTransInRespQueue);
    void releaseTransaction();
    void manageRequests(const sc_core::sc_time& delay);

    sc_core::sc_event beginReqEvent, endRespEvent, controllerEvent, dataResponseEvent;

    MemoryManager memoryManager;

    void createChildTranses(tlm::tlm_generic_payload& parentTrans);

    class Stats : public Statistics::Group
    {
    public:
        Statistics::ScalarStat& numberOfRequests;
        Statistics::ScalarStat& numberOfReadRequests;
        Statistics::ScalarStat& numberOfWriteRequests;
        Statistics::ScalarStat& averageBandwidth;
        Statistics::ScalarStat& averageBandwidthWithoutIdle;
        Statistics::ScalarStat& maximumTheoreticalBandwidth;
        Statistics::ScalarStat& averageUtilization;
        Statistics::ScalarStat& averageUtilizationWithoutIdle;

        class RankStats : public Statistics::Group
        {
        public:
            Statistics::ScalarStat& averageBandwidth;
            Statistics::ScalarStat& averageUtilization;

            RankStats(std::string name, Group* parent);
        };
        std::vector<std::unique_ptr<RankStats>> rankStats;

        Stats(Controller const& controller);
    } stats;

    uint64_t numberOfRequests = 0;
    uint64_t numberOfReadRequests = 0;
    uint64_t numberOfWriteRequests = 0;

    class IdleTimeCollector
    {
    public:
        void start()
        {
            if (!isIdle)
            {
                PRINTDEBUGMESSAGE("IdleTimeCollector", "IDLE start");
                idleStart = sc_core::sc_time_stamp();
                isIdle = true;
            }
        }

        void end()
        {
            if (isIdle)
            {
                PRINTDEBUGMESSAGE("IdleTimeCollector", "IDLE end");
                idleTime += sc_core::sc_time_stamp() - idleStart;
                isIdle = false;
            }
        }

        [[nodiscard]] sc_core::sc_time getIdleTime() const { return idleTime; }

    private:
        bool isIdle = false;
        sc_core::sc_time idleTime = sc_core::SC_ZERO_TIME;
        sc_core::sc_time idleStart;
    } idleTimeCollector;
};

} // namespace DRAMSys

#endif // CONTROLLER_H
