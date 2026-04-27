#ifndef DRAMATRECORDER_H
#define DRAMATRECORDER_H

#include <DRAMSys/common/TlmRecorder.h>
#include <DRAMSys/configuration/memspec/MemSpec.h>
#include <DRAMSys/simulation/SimConfig.h>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <vector>

namespace DRAMSys
{

class DramATRecorder : public sc_core::sc_module

{
    SC_HAS_PROCESS(DramATRecorder);

public:
    DramATRecorder(const sc_core::sc_module_name& name,
                   const SimConfig& simConfig,
                   const MemSpec& memspec,
                   TlmRecorder& tlmRecorder,
                   bool enableBandwidth);

    void record(tlm::tlm_generic_payload const& trans,
                tlm::tlm_phase const& phase,
                sc_core::sc_time const& delay);

private:
    TlmRecorder& tlmRecorder;
    const bool enableWindowing;
    const bool pseudoChannelMode;
    const unsigned int ranksPerChannel;
    const sc_core::sc_time windowSizeTime;
    std::vector<uint64_t> numberOfBeatsServed;
    uint64_t lastNumberOfBeatsServed = 0;
    const sc_core::sc_time activeTimeMultiplier;

    bool enableBandwidth = false;

    void recordBandwidth();
};

} // namespace DRAMSys

#endif // DRAMATRECORDER_H
