#ifndef DRAMPOWERTYPES_H
#define DRAMPOWERTYPES_H

#include "DRAMSys/controller/Command.h"
#include <DRAMPower/command/CmdType.h>

#include <tlm>

namespace DRAMSys {

enum class EfficencyMode : char {
    NormalMode,
    EfficencyMode
};

[[nodiscard]] inline DRAMPower::CmdType phaseToDRAMPowerCommand(tlm::tlm_phase phase)
{
    // TODO missing DSMEN, DSMEX
    assert(phase >= BEGIN_NOP && phase <= END_SREF);
    static std::array<DRAMPower::CmdType, Command::Type::END_ENUM> phaseOfCommand = {
        DRAMPower::CmdType::NOP,    // 0
        DRAMPower::CmdType::RD,     // 1
        DRAMPower::CmdType::WR,     // 2
        DRAMPower::CmdType::NOP,    // 3
        DRAMPower::CmdType::RDA,    // 4
        DRAMPower::CmdType::WRA,    // 5
        DRAMPower::CmdType::NOP,    // 6
        DRAMPower::CmdType::ACT,    // 7
        DRAMPower::CmdType::PRE,    // 8, PREPB
        DRAMPower::CmdType::REFB,   // 9, REFPB
        DRAMPower::CmdType::NOP,    // 10, RFMPB
        DRAMPower::CmdType::REFP2B, // 11, REFP2B
        DRAMPower::CmdType::NOP,    // 12, RFMP2B
        DRAMPower::CmdType::PRESB,  // 13, PRESB
        DRAMPower::CmdType::REFSB,  // 14, REFSB
        DRAMPower::CmdType::NOP,    // 15, RFMSB
        DRAMPower::CmdType::PREA,   // 16, PREAB
        DRAMPower::CmdType::REFA,   // 17, REFAB
        DRAMPower::CmdType::NOP,    // 18, RFMAB
        DRAMPower::CmdType::PDEA,   // 19
        DRAMPower::CmdType::PDEP,   // 20
        DRAMPower::CmdType::SREFEN, // 21
        DRAMPower::CmdType::PDXA,   // 22
        DRAMPower::CmdType::PDXP,   // 23
        DRAMPower::CmdType::SREFEX  // 24
    };
    return phaseOfCommand[phase - BEGIN_NOP];
}

} // namespace DRAMSys

#endif /* POWER_DRAMPOWERTYPES_H */
