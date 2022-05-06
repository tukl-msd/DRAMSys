/*
 * Copyright (c) 2015, Technische Universität Kaiserslautern
 * Copyright (c) 2016, Dresden University of Technology (TU Dresden)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Matthias Jung
 *          Christian Menard
 *          Abdul Mutaal Ahmad
 */

#include <iostream>
#include <string>
#include <cstdlib>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "report_handler.hh"
#include "sc_target.hh"
#include "sim_control.hh"
#include "slave_transactor.hh"
#include "stats.hh"

#include "Configuration.h"
#include "simulation/DRAMSys.h"

#ifdef RECORDING
#include "simulation/DRAMSysRecordable.h"
#endif

using namespace sc_core;
using namespace tlm;

class Gem5SimControlDRAMsys : public Gem5SystemC::Gem5SimControl
{
public:
    Gem5SimControlDRAMsys(std::string configFile) :
        Gem5SystemC::Gem5SimControl("gem5", configFile, 0, "MemoryAccess")
    {}

    virtual void afterSimulate() override
    {
        sc_stop();
    }
};


class AddressOffset : sc_module
{
private:
    unsigned long long int offset;

public:
    tlm_utils::simple_target_socket<AddressOffset> t_socket;
    tlm_utils::simple_initiator_socket<AddressOffset> i_socket;

    AddressOffset(sc_module_name, unsigned long long int o) : offset(o),
        t_socket("t_socket"), i_socket("i_socket")
    {
        t_socket.register_nb_transport_fw(this, &AddressOffset::nb_transport_fw);
        t_socket.register_transport_dbg(this, &AddressOffset::transport_dbg);
        t_socket.register_b_transport(this, &AddressOffset::b_transport);
        i_socket.register_nb_transport_bw(this, &AddressOffset::nb_transport_bw);
    }

    //Forward Interface
    tlm_sync_enum nb_transport_fw(tlm_generic_payload &trans, tlm_phase &phase,
                                       sc_time &delay)
    {
        //std::cout << "NB "<< this->name() <<": " << trans.get_address() << " -" << offset;
        trans.set_address(trans.get_address() - offset);
        //std::cout << " = " << trans.get_address() << std::endl;
        return i_socket->nb_transport_fw(trans, phase, delay);
    }

    unsigned int transport_dbg(tlm_generic_payload &trans)
    {
        // adjust address offset:
        //std::cout << "Debug "<< this->name() <<": " << trans.get_address() << " -" << offset;
        trans.set_address(trans.get_address() - offset);
        //std::cout << " = " << trans.get_address() << std::endl;
        return i_socket->transport_dbg(trans);
    }

    void b_transport(tlm_generic_payload &trans, sc_time &delay)
    {
        // adjust address offset:
        //std::cout << "B "<< this->name() <<": " << trans.get_address() << " -" << offset;
        trans.set_address(trans.get_address() - offset);
        //std::cout << " = " << trans.get_address() << std::endl;
        i_socket->b_transport(trans, delay);
    }

    //Backward Interface
    tlm_sync_enum nb_transport_bw(tlm_generic_payload &trans, tlm_phase &phase,
                                       sc_time &delay)
    {
        //trans.set_address(trans.get_address()+offset);
        return t_socket->nb_transport_bw(trans, phase, delay);
    }

};

std::string pathOfFile(std::string file)
{
    return file.substr(0, file.find_last_of('/'));
}

int sc_main(int argc, char **argv)
{
    SC_REPORT_INFO("sc_main", "Simulation Setup");

    std::string simulationJson;
    std::string gem5ConfigFile;
    std::string resources;
    unsigned int numTransactors;
    std::vector<std::unique_ptr<Gem5SystemC::Gem5SlaveTransactor>> transactors;

    if (argc == 4)
    {
        // Get path of resources:
        resources = pathOfFile(argv[0])
                    + std::string("/../../DRAMSys/library/resources/");

        simulationJson = argv[1];
        gem5ConfigFile = argv[2];
        numTransactors = static_cast<unsigned>(std::stoul(argv[3]));

    }
    else
    {
        SC_REPORT_FATAL("sc_main", "Please provide configuration files and number of ports");
        return EXIT_FAILURE;
    }

    DRAMSysConfiguration::Configuration configLib = DRAMSysConfiguration::from_path(simulationJson, resources);

    // Instantiate DRAMSys:
    std::unique_ptr<DRAMSys> dramSys;

#ifdef RECORDING
    if (configLib.simConfig.databaseRecording.value_or(false))
        dramSys = std::make_unique<DRAMSysRecordable>("DRAMSys", configLib);
    else
#endif
        dramSys = std::make_unique<DRAMSys>("DRAMSys", configLib);

    // Instantiate gem5:
    Gem5SimControlDRAMsys sim_control(gem5ConfigFile);

    // XXX: this code assumes:
    // - for a single port the port name is "transactor"
    // - for multiple ports names are transactor1, transactor2, ..., transactorN
    // Names generated here must match port names used by the gem5 config file, e.g., config.ini
    if (numTransactors == 1)
    {
        transactors.emplace_back(std::make_unique<Gem5SystemC::Gem5SlaveTransactor>("transactor", "transactor"));
        transactors.back().get()->socket.bind(dramSys->tSocket);
        transactors.back().get()->sim_control.bind(sim_control);
    }
    else
    {
        for (unsigned i = 0; i < numTransactors; i++)
        {
            // If there are two or more ports
            unsigned index = i + 1;
            std::string name = "transactor" + std::to_string(index);
            std::string portName = "transactor" + std::to_string(index);
            transactors.emplace_back(std::make_unique<Gem5SystemC::Gem5SlaveTransactor>(name.c_str(), portName.c_str()));
            transactors.back().get()->socket.bind(dramSys->tSocket);
            transactors.back().get()->sim_control.bind(sim_control);
        }
    }

#ifdef CHOICE3
    // If for example two gem5 ports are used (NVM and DRAM) with
    // crazy address offsets:
    Gem5SystemC::Gem5SlaveTransactor dramInterface("transactor1", "transactor1");
    Gem5SystemC::Gem5SlaveTransactor nvmInterface("transactor2", "transactor2");

    AddressOffset nvmOffset("nvmOffset", 0);
    AddressOffset dramOffset("dramOffset", (2147483648 - 67108863)); //+67108863);

    dramInterface.socket.bind(dramOffset.t_socket);
    dramOffset.i_socket.bind(dramSys->tSocket); // ID0
    nvmInterface.socket.bind(nvmOffset.t_socket);
    nvmOffset.i_socket.bind(dramSys->tSocket);

    dramInterface.sim_control.bind(sim_control);
    nvmInterface.sim_control.bind(sim_control);
#endif

    SC_REPORT_INFO("sc_main", "Start of Simulation");

    sc_core::sc_set_stop_mode(SC_STOP_FINISH_DELTA);
    sc_core::sc_start();

    if (!sc_core::sc_end_of_simulation_invoked())
    {
        SC_REPORT_INFO("sc_main", "Simulation stopped without explicit sc_stop()");
        sc_core::sc_stop();
    }

    //for (auto t : transactors)
    //   delete t;

    SC_REPORT_INFO("sc_main", "End of Simulation");

    return EXIT_SUCCESS;
}
