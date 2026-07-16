#ifndef DRAMPowerLPDDR6_H
#define DRAMPowerLPDDR6_H

#include "DRAMSys/power/DRAMPowerTypes.h"
#include "DRAMSys/common/Deserialize.h"
#include "DRAMSys/common/Serialize.h"
#include "DRAMSys/common/dramExtensions.h"

#include <DRAMPower/Types.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>
#include <DRAMPower/simconfig/simconfig.h>

#include <DRAMPower/standards/lpddr6/types.h>

#include <algorithm>
#include <systemc>
#include <tlm>


namespace DRAMSys
{

class DRAMPowerLPDDR6 : public Deserialize, public Serialize {
// Public type definitions
public:
    static constexpr std::size_t NUMINTERFACES = 2;
    using Standart_t = DRAMPower::LPDDR6Types;
    using DRAMUtilsMemSpec_t = typename Standart_t::DRAMUtilsMemSpec_t;
    using MemSpec_t = typename Standart_t::MemSpec_t;
    using Core_t = typename Standart_t::Core_t;
    using Interface_t = typename Standart_t::Interface_t;
    using CalcCore_t = typename Standart_t::CalcCore_t;
    using CalcInterface_t = typename Standart_t::CalcInterface_t;

// Public constructors / assignment operators
    DRAMPowerLPDDR6(const DRAMUtilsMemSpec_t& memSpec, const DRAMPower::config::SimConfig& config, EfficencyMode mode = EfficencyMode::NormalMode)
        : memSpec(memSpec)
        , tCK(sc_core::sc_time(memSpec.memtimingspec.tCK, sc_core::SC_SEC))
        , groupsPerRank(memSpec.memarchitecturespec.nbrOfBankGroups)
        , banksPerRank(memSpec.memarchitecturespec.nbrOfBanks)
        , cores{Core_t(memSpec), Core_t(memSpec)}
        , interfaces{Interface_t(memSpec, config), Interface_t(memSpec, config)}
        , mode(mode)
    {}

// Public member functions:
    void setEfficiencyMode(DRAMPower::timestamp_t timestamp, EfficencyMode mode) {
        this->mode = mode;
        Interface_t& interface = interfaces[1];
        switch(mode) {
            case EfficencyMode::NormalMode:
                if (!interface.isEnabled()) interface.enable(timestamp);
                break;
            case EfficencyMode::EfficencyMode:
                if (interface.isEnabled()) interface.disable(timestamp);
                break;
        }
    }

    EfficencyMode getEfficiencyMode() const {
        return mode;
    }

    static constexpr std::size_t getInterfaceCount()  {
        return NUMINTERFACES;
    }

    void doCommand(std::size_t channel, const DRAMPower::LPDDR6Command& command) {
        channel %= NUMINTERFACES;
        cores[channel].doCommand(command);
        switch (mode) {
            case EfficencyMode::NormalMode:
                interfaces[channel].doCommand(command);
                break;
            case EfficencyMode::EfficencyMode:
                interfaces[0].doCommand(command);
                break;
        }
    }

    void doCommand([[maybe_unused]] std::size_t channel,
                     const tlm::tlm_generic_payload& trans,
                     const tlm::tlm_phase& phase,
                     const sc_core::sc_time& delay) {
        auto rank =
            static_cast<std::size_t>(ControllerExtension::getRank(trans)); // relative to the channel
        auto bank_group = static_cast<std::size_t>(
            ControllerExtension::getBankGroup(trans)) % groupsPerRank; // relative to the rank
        auto bank = static_cast<std::size_t>(
            ControllerExtension::getBank(trans)) % banksPerRank; // relative to the rank
        auto dbank = 0;
        auto row = static_cast<std::size_t>(ControllerExtension::getRow(trans));
        auto column = static_cast<std::size_t>(ControllerExtension::getColumn(trans));
        uint64_t cycle = std::lround((sc_core::sc_time_stamp() + delay) / tCK);

        // NOTE:
        // banks are relative to the rank
        // bankgroups are relative to the rank

        DRAMPower::LPDDR6TargetCoordinate target;
        target.bank = bank;
        target.dbank = dbank;
        target.bankGroup = bank_group;
        target.rank = rank;
        target.row = row;
        target.column = column;
        target.subChannel = EfficencyMode::EfficencyMode == mode  ? channel % NUMINTERFACES : 0;

        // TODO read, write data for interface calculation
        uint8_t* data = trans.get_data_ptr();                  // Can be nullptr if no data
        auto datasize = trans.get_data_length() * 8; // Is always set

        DRAMPower::LPDDR6Command command(cycle, phaseToDRAMPowerCommand(phase), target, data, datasize);
        doCommand(channel, command);
    }

    void getWindowStats(DRAMPower::timestamp_t timestamp, DRAMPower::SimulationStats& stats) {
        std::for_each(cores.begin(), cores.end(), [timestamp, &stats](Core_t& core) {
            DRAMPower::SimulationStats localstats{};
            core.getWindowStats(timestamp, localstats);
            stats += localstats;
        });
        std::for_each(interfaces.begin(), interfaces.end(), [timestamp, &stats](Interface_t& interface) {
            DRAMPower::SimulationStats localstats{};
            interface.getWindowStats(timestamp, localstats);
            stats += localstats;
        });
    }

    [[nodiscard]] DRAMPower::SimulationStats getWindowStats(DRAMPower::timestamp_t timestamp) {
        DRAMPower::SimulationStats stats;
        getWindowStats(timestamp, stats);
        return stats;
    }

    [[nodiscard]] DRAMPower::timestamp_t getLastCommandTime() {
        DRAMPower::timestamp_t max = 0;
        std::for_each(cores.begin(), cores.end(), [&max](Core_t& core) {
            max = std::max(max, core.getLastCommandTime());
        });
        std::for_each(interfaces.begin(), interfaces.end(), [&max](Interface_t& interface) {
            max = std::max(max, interface.getLastCommandTime());
        });
        return max;
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

    [[nodiscard]] Interface_t& getInterface(std::size_t channel) {
        return interfaces.at(channel);
    }

    [[nodiscard]] const Interface_t& getInterface(std::size_t channel) const {
        return interfaces.at(channel);
    }

    [[nodiscard]] Core_t& getCore(std::size_t channel) {
        return cores.at(channel);
    }

    [[nodiscard]] const Core_t& getCore(std::size_t channel) const {
        return cores.at(channel);
    }

    [[nodiscard]] bool isSerializable() const {
        return std::all_of(cores.begin(), cores.end(), [](const Core_t& core){
            return core.isSerializable();
        });
    }

    void deserialize(std::istream& stream) override {
        for (auto& core : cores)
        {
            core.deserialize(stream);
        }
        for (auto& interface : interfaces)
        {
            interface.deserialize(stream);
        }
    }
    
    void serialize(std::ostream& stream) const override {
        for (const auto& core : cores)
        {
            core.serialize(stream);
        }
        for (const auto& interface : interfaces)
        {
            interface.serialize(stream);
        }
    }

// Private member variables
private:
    MemSpec_t memSpec;
    sc_core::sc_time tCK;
    uint64_t groupsPerRank{};
    uint64_t banksPerRank{};
    std::array<Core_t, NUMINTERFACES> cores;
    std::array<Interface_t, NUMINTERFACES> interfaces;
    EfficencyMode mode;
};

} // namespace DRAMSys

#endif /* DRAMPowerLPDDR6_H */
