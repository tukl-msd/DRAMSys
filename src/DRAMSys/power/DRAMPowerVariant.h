#ifndef DRAMPOWERVARIANT_H
#define DRAMPOWERVARIANT_H

#include "DRAMSys/power/DRAMPowerWrapper.h"
#include "DRAMSys/power/DRAMPowerLPDDR6.h"

#include <DRAMPower/standards/ddr4/types.h>
#include <DRAMPower/standards/ddr5/types.h>
#include <DRAMPower/standards/lpddr4/types.h>
#include <DRAMPower/standards/lpddr5/types.h>
#include <DRAMPower/standards/lpddr6/types.h>
#include <DRAMPower/standards/hbm2/types.h>

#include <variant>

namespace DRAMSys
{

using DRAMPowerDDR4 = DRAMPowerWrapper<DRAMPower::DDR4Types>;
using DRAMPowerDDR5 = DRAMPowerWrapper<DRAMPower::DDR5Types>;
using DRAMPowerLPDDR4 = DRAMPowerWrapper<DRAMPower::LPDDR4Types>;
using DRAMPowerLPDDR5 = DRAMPowerWrapper<DRAMPower::LPDDR5Types>;
using DRAMPowerHBM2 = DRAMPowerWrapper<DRAMPower::HBM2Types>;

using DRAMPowerVariant = std::variant<
    DRAMPowerDDR4,
    DRAMPowerDDR5,
    DRAMPowerLPDDR4,
    DRAMPowerLPDDR5,
    DRAMPowerLPDDR6,
    DRAMPowerHBM2
>;

} // namespace DRAMSys

#endif /* DRAMPOWERVARIANT_H */
