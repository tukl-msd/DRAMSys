#include "TlmATRecorder.h"

#include "DRAMSys/configuration/memspec/MemSpec.h"
#include "DRAMSys/controller/Command.h"
#include "DRAMSys/simulation/SimConfig.h"
#include <sysc/kernel/sc_module.h>
#include <sysc/kernel/sc_simcontext.h>
#include <tlm_core/tlm_2/tlm_generic_payload/tlm_phase.h>

namespace DRAMSys
{

TlmATRecorder::TlmATRecorder(const sc_core::sc_module_name& name,
                                             const SimConfig& simConfig,
                                             const MemSpec& memSpec,
                                             TlmRecorder& tlmRecorder,
                                             bool enableBandwidth) :
    sc_module(name),
    memSpec(memSpec),
    tlmRecorder(tlmRecorder),
    enableWindowing(simConfig.enableWindowing),
    pseudoChannelMode(memSpec.pseudoChannelMode()),
    ranksPerChannel(memSpec.ranksPerChannel),
    windowSizeTime(simConfig.windowSize * memSpec.tCK),
    numberOfBytesServed(0),
    activeTimeMultiplier(memSpec.tCK / memSpec.dataRate),
    enableBandwidth(enableBandwidth)
{
    iSocket.register_nb_transport_bw(this, &TlmATRecorder::nb_transport_bw);
    tSocket.register_nb_transport_fw(this, &TlmATRecorder::nb_transport_fw);
    tSocket.register_b_transport(this, &TlmATRecorder::b_transport);
    tSocket.register_transport_dbg(this, &TlmATRecorder::transport_dbg);

    if (enableBandwidth && enableWindowing)
    {
        SC_METHOD(recordBandwidth);
        dont_initialize();
        sensitive << windowEvent;

        windowEvent.notify(windowSizeTime);
        nextWindowEventTime = windowSizeTime;
    }
}

tlm::tlm_sync_enum TlmATRecorder::nb_transport_fw(tlm::tlm_generic_payload& trans,
                                                          tlm::tlm_phase& phase,
                                                          sc_core::sc_time& delay)
{
    if (enableBandwidth && enableWindowing)
    {
        if (phase == tlm::BEGIN_REQ && trans.is_write())
            numberOfBytesServed += trans.get_data_length();
    }

    tlmRecorder.recordPhase(trans, phase, delay);
    return iSocket->nb_transport_fw(trans, phase, delay);
}

tlm::tlm_sync_enum TlmATRecorder::nb_transport_bw(tlm::tlm_generic_payload& trans,
                                                          tlm::tlm_phase& phase,
                                                          sc_core::sc_time& delay)
{
    if (enableBandwidth && enableWindowing)
    {
        if (phase == tlm::BEGIN_RESP && trans.is_read())
            numberOfBytesServed += trans.get_data_length();
    }

    tlmRecorder.recordPhase(trans, phase, delay);
    return tSocket->nb_transport_bw(trans, phase, delay);
}

void TlmATRecorder::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
{
    iSocket->b_transport(trans, delay);
}

unsigned int TlmATRecorder::transport_dbg(tlm::tlm_generic_payload& trans)
{
    return iSocket->transport_dbg(trans);
}

void TlmATRecorder::recordBandwidth()
{
    windowEvent.notify(windowSizeTime);
    nextWindowEventTime += windowSizeTime;

    uint64_t windowNumberOfBytesServed = numberOfBytesServed - lastNumberOfBytesServed;
    lastNumberOfBytesServed = numberOfBytesServed;

    // HBM specific, pseudo channels get averaged
    if (pseudoChannelMode)
        windowNumberOfBytesServed /= ranksPerChannel;

    double windowBandwidth =
        static_cast<double>(windowNumberOfBytesServed) / (windowSizeTime.to_seconds());
    tlmRecorder.recordBandwidth(sc_core::sc_time_stamp().to_seconds(), windowBandwidth);
}

} // namespace DRAMSys
