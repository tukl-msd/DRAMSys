#ifndef DRAMPOWERWRAPPER_H
#define DRAMPOWERWRAPPER_H

#include "DRAMSys/common/Deserialize.h"
#include "DRAMSys/common/Serialize.h"
#include "DRAMSys/common/dramExtensions.h"
#include "DRAMSys/controller/Command.h"

#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>
#include <DRAMPower/simconfig/simconfig.h>

#include <algorithm>
#include <systemc>
#include <tlm>

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
        , tCK(sc_core::sc_time(memSpec.memtimingspec.tCK, sc_core::SC_SEC))
        , groupsPerRank(memSpec.memarchitecturespec.nbrOfBankGroups)
        , banksPerGroup(memSpec.memarchitecturespec.nbrOfBanks /
                memSpec.memarchitecturespec.nbrOfBankGroups)
        , core(memSpec)
        , interface(memSpec, config)
    {}

// Public member functions:
    void doCommand(const DRAMPower::Command& command) {
        core.doCommand(command);
        interface.doCommand(command);
    }

    void doCommand([[maybe_unused]] std::size_t channel,
                     const tlm::tlm_generic_payload& trans,
                     const tlm::tlm_phase& phase,
                     const sc_core::sc_time& delay) {
        auto rank =
            static_cast<std::size_t>(ControllerExtension::getRank(trans)); // relative to the channel
        auto bank_group_abs = static_cast<std::size_t>(
            ControllerExtension::getBankGroup(trans));             // relative to the channel
        auto bank_group = bank_group_abs - (rank * groupsPerRank); // relative to the rank
        auto bank = static_cast<std::size_t>(ControllerExtension::getBank(trans)) -
                    (bank_group_abs * banksPerGroup); // relative to the bank_group
        auto row = static_cast<std::size_t>(ControllerExtension::getRow(trans));
        auto column = static_cast<std::size_t>(ControllerExtension::getColumn(trans));
        uint64_t cycle = std::lround((sc_core::sc_time_stamp() + delay) / tCK);

        // DRAMPower:
        // banks are relative to the rank
        // bankgroups are relative to the rank
        bank = bank + (bank_group * banksPerGroup);

        DRAMPower::TargetCoordinate target(bank, bank_group, rank, row, column);

        // TODO read, write data for interface calculation
        uint8_t* data = trans.get_data_ptr();                  // Can be nullptr if no data
        auto datasize = trans.get_data_length() * 8; // Is always set

        DRAMPower::Command command(cycle, phaseToDRAMPowerCommand(phase), target, data, datasize);
        doCommand(command);
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
    sc_core::sc_time tCK;
    uint64_t groupsPerRank{};
    uint64_t banksPerGroup{};
    Core_t core;
    Interface_t interface;
};

} // namespace DRAMSys

#endif /* DRAMPOWERWRAPPER_H */
