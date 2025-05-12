#include "DramATRecorder.h"
#include "DRAMSys/configuration/memspec/MemSpec.h"
#include "DRAMSys/controller/Command.h"
#include "DRAMSys/simulation/SimConfig.h"
#include <sysc/kernel/sc_module.h>
#include <sysc/kernel/sc_simcontext.h>
#include <tlm_core/tlm_2/tlm_generic_payload/tlm_phase.h>

namespace DRAMSys
{

DramATRecorder::DramATRecorder(const sc_core::sc_module_name& name,
                                 const SimConfig& simConfig,
                                 const MemSpec& memSpec,
                                 TlmRecorder& tlmRecorder,
                                 bool enableBandwidth) :
    sc_module(name),
    tlmRecorder(tlmRecorder),
    enableWindowing(simConfig.enableWindowing),
    pseudoChannelMode(memSpec.pseudoChannelMode()),
    ranksPerChannel(memSpec.ranksPerChannel),
    windowSizeTime(simConfig.windowSize * memSpec.tCK),
    numberOfBeatsServed(memSpec.ranksPerChannel, 0),
    activeTimeMultiplier(memSpec.tCK / memSpec.dataRate),
    enableBandwidth(enableBandwidth)
{
    iSocket.register_nb_transport_bw(this, &DramATRecorder::nb_transport_bw);
    tSocket.register_nb_transport_fw(this, &DramATRecorder::nb_transport_fw);
    tSocket.register_b_transport(this, &DramATRecorder::b_transport);
    tSocket.register_transport_dbg(this, &DramATRecorder::transport_dbg);

    if (enableBandwidth && enableWindowing)
    {
        SC_METHOD(recordBandwidth);
        dont_initialize();
        sensitive << windowEvent;

        windowEvent.notify(windowSizeTime);
        nextWindowEventTime = windowSizeTime;
    }
}

tlm::tlm_sync_enum DramATRecorder::nb_transport_fw(tlm::tlm_generic_payload& trans,
                                                    tlm::tlm_phase& phase,
                                                    sc_core::sc_time& delay)
{
    if (enableBandwidth && enableWindowing)
    {
        Command cmd{phase};
        if (cmd.isCasCommand())
        {
            auto rank = ControllerExtension::getRank(trans);
            numberOfBeatsServed[static_cast<std::size_t>(rank)] +=
                ControllerExtension::getBurstLength(trans);
        }
    }

    tlmRecorder.recordPhase(trans, phase, delay);
    return iSocket->nb_transport_fw(trans, phase, delay);
}

tlm::tlm_sync_enum DramATRecorder::nb_transport_bw(tlm::tlm_generic_payload& trans,
                                                    tlm::tlm_phase& phase,
                                                    sc_core::sc_time& delay)
{
    tlmRecorder.recordPhase(trans, phase, delay);
    return tSocket->nb_transport_bw(trans, phase, delay);
}

void DramATRecorder::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
{
    iSocket->b_transport(trans, delay);
}

unsigned int DramATRecorder::transport_dbg(tlm::tlm_generic_payload& trans)
{
    return iSocket->transport_dbg(trans);
}

void DramATRecorder::recordBandwidth()
{
    windowEvent.notify(windowSizeTime);
    nextWindowEventTime += windowSizeTime;

    std::uint64_t totalNumberOfBeatsServed =
        std::accumulate(numberOfBeatsServed.begin(), numberOfBeatsServed.end(), 0);

    uint64_t windowNumberOfBeatsServed = totalNumberOfBeatsServed - lastNumberOfBeatsServed;
    lastNumberOfBeatsServed = totalNumberOfBeatsServed;

    // HBM specific, pseudo channels get averaged
    if (pseudoChannelMode)
        windowNumberOfBeatsServed /= ranksPerChannel;

    sc_core::sc_time windowActiveTime =
        activeTimeMultiplier * static_cast<double>(windowNumberOfBeatsServed);
    double windowAverageBandwidth = windowActiveTime / windowSizeTime;
    tlmRecorder.recordBandwidth(sc_core::sc_time_stamp().to_seconds(), windowAverageBandwidth);
}

} // namespace DRAMSys
