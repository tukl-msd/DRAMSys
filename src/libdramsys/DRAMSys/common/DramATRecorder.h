#ifndef DRAMATRECORDER_H
#define DRAMATRECORDER_H

#include "DRAMSys/configuration/memspec/MemSpec.h"
#include "DRAMSys/simulation/SimConfig.h"
#include "TlmRecorder.h"

#include <sysc/kernel/sc_module.h>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <vector>

namespace DRAMSys
{

class DramATRecorder : public sc_core::sc_module

{
    SC_HAS_PROCESS(DramATRecorder);

public:
    tlm_utils::simple_initiator_socket<DramATRecorder> iSocket;
    tlm_utils::simple_target_socket<DramATRecorder> tSocket;

    DramATRecorder(const sc_core::sc_module_name& name,
                    const SimConfig& simConfig,
                    const MemSpec& memspec,
                    TlmRecorder& tlmRecorder,
                    bool enableBandwidth);
    ~DramATRecorder() = default;

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay);
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& delay);
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);

private:
    TlmRecorder& tlmRecorder;
    const bool enableWindowing;
    const bool pseudoChannelMode;
    const unsigned int ranksPerChannel;
    const sc_core::sc_time windowSizeTime;
    sc_core::sc_event windowEvent;
    sc_core::sc_time nextWindowEventTime;
    std::vector<uint64_t> numberOfBeatsServed;
    uint64_t lastNumberOfBeatsServed = 0;
    const sc_core::sc_time activeTimeMultiplier;

    bool enableBandwidth = false;

    void recordBandwidth();
};

} // namespace DRAMSys

#endif // DRAMATRECORDER_H
