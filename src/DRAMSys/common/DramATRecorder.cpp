#include "DramATRecorder.h"

#include <DRAMSys/controller/Command.h>

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
    if (enableBandwidth && enableWindowing)
    {
        SC_THREAD(recordBandwidth);
    }
}

void DramATRecorder::record(tlm::tlm_generic_payload const& trans,
                            tlm::tlm_phase const& phase,
                            sc_core::sc_time const& delay)
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
}

void DramATRecorder::recordBandwidth()
{
    while (true)
    {
        wait(windowSizeTime);

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
}

} // namespace DRAMSys
