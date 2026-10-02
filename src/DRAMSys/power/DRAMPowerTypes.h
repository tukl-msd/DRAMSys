#ifndef DRAMPOWERTYPES_H
#define DRAMPOWERTYPES_H

#include "DRAMSys/controller/Command.h"
#include <DRAMPower/command/CmdType.h>

#include <tlm>

namespace DRAMSys {

enum class EfficiencyMode : char {
    NormalMode,
    EfficiencyMode
};

} // namespace DRAMSys

#endif /* POWER_DRAMPOWERTYPES_H */
