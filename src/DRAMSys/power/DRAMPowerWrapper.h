#ifndef DRAMPOWERWRAPPER_H
#define DRAMPOWERWRAPPER_H

#include "DRAMSys/common/Deserialize.h"
#include "DRAMSys/common/Serialize.h"

#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>
#include <DRAMPower/simconfig/simconfig.h>

#include <algorithm>


namespace DRAMSys {

template<typename Standard>
class DRAMPowerWrapper : public Deserialize, public Serialize {
// Public type definitions
public:
    using DRAMUtilsMemSpec_t = typename Standard::DRAMUtilsMemSpec_t;
    using MemSpec_t = typename Standard::MemSpec_t;
    using Core_t = typename Standard::Core_t;
    using Interface_t = typename Standard::Interface_t;
    using CalcCore_t = typename Standard::CalcCore_t;
    using CalcInterface_t = typename Standard::CalcInterface_t;

// Public constructors / assignment operators
    DRAMPowerWrapper(const DRAMUtilsMemSpec_t& memSpec, const DRAMPower::config::SimConfig& config)
        : memSpec(memSpec)
        , core(memSpec)
        , interface(memSpec, config)
    {}

// Public member functions:
    void doCommand(const DRAMPower::Command& command) {
        core.doCommand(command);
        interface.doCommand(command);
    }

    void getWindowStats(DRAMPower::timestamp_t timestamp, DRAMPower::SimulationStats& stats) {
        core.getWindowStats(timestamp, stats);
        interface.getWindowStats(timestamp, stats);
    }

    [[nodiscard]] DRAMPower::SimulationStats getWindowStats(DRAMPower::timestamp_t timestamp) {
        DRAMPower::SimulationStats stats;
        getWindowStats(timestamp, stats);
        return stats;
    }

    [[nodiscard]] DRAMPower::timestamp_t getLastCommandTime() {
        return std::max(core.getLastCommandTime(), interface.getLastCommandTime());
    }

    [[nodiscard]] DRAMPower::interface_energy_info_t calcInterfaceEnergy(const DRAMPower::SimulationStats& stats) const {
        CalcInterface_t calculation(memSpec);
        return calculation.calculateEnergy(stats);
    }

    [[nodiscard]] DRAMPower::interface_energy_info_t calcInterfaceEnergy(DRAMPower::timestamp_t timestamp) {
        return calcInterfaceEnergy(getWindowStats(timestamp));
    }

    [[nodiscard]] DRAMPower::energy_t calcCoreEnergy(const DRAMPower::SimulationStats& stats) const {
        CalcCore_t calculation(memSpec);
        return calculation.calcEnergy(stats);
    }

    [[nodiscard]] DRAMPower::energy_t calcCoreEnergy(DRAMPower::timestamp_t timestamp) {
        return calcCoreEnergy(getWindowStats(timestamp));
    }

    [[nodiscard]] double getTotalEnergy(DRAMPower::SimulationStats& stats) const {
        return calcCoreEnergy(stats).total() + calcInterfaceEnergy(stats).total();
    }

    [[nodiscard]] double getTotalEnergy(DRAMPower::timestamp_t timestamp) {
        return calcCoreEnergy(timestamp).total() + calcInterfaceEnergy(timestamp).total();
    }

    [[nodiscard]] Interface_t& getInterface() {
        return interface;
    }

    [[nodiscard]] const Interface_t& getInterface() const {
        return interface;
    }

    [[nodiscard]] Core_t& getCore() {
        return core;
    }

    [[nodiscard]] const Core_t& getCore() const {
        return core;
    }

    [[nodiscard]] bool isSerializable() const {
        return core.isSerializable();
    }

    void deserialize(std::istream& stream) override {
        core.deserialize(stream);
        interface.deserialize(stream);
    }
    
    void serialize(std::ostream& stream) const override {
        core.serialize(stream);
        interface.serialize(stream);
    }

// Private member variables
private:
    MemSpec_t memSpec;
    Core_t core;
    Interface_t interface;
};

} // namespace DRAMSys

#endif /* DRAMPOWERWRAPPER_H */
