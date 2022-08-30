/*
 * Copyright (c) 2015, Technische Universität Kaiserslautern
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
 * Authors:
 *    Matthias Jung
 */

#ifndef ERRORMODEL_H
#define ERRORMODEL_H

#include <map>

#include <systemc>
#include "../configuration/Configuration.h"
#include "../simulation/AddressDecoder.h"
#include "../simulation/TemperatureController.h"

class libDRAMPower;

class errorModel : public sc_core::sc_module
{
public:
    errorModel(const sc_core::sc_module_name& name, const Configuration& config,
               TemperatureController& temperatureController, libDRAMPower* dramPower = nullptr);
    ~errorModel() override;

    // Access Methods:
    void store(tlm::tlm_generic_payload &trans);
    void load(tlm::tlm_generic_payload &trans);
    void refresh(unsigned int row);
    void activate(unsigned int row);
    void setTemperature(double t);
    double getTemperature();

private:
    void init(const Configuration& config);
    bool powerAnalysis;
    libDRAMPower *DRAMPower;
    bool thermalSim;
    TemperatureController& temperatureController;
    const MemSpec& memSpec;
    // Configuration Parameters:
    unsigned int burstLenght;
    unsigned int numberOfColumns;
    unsigned int bytesPerColumn;
    unsigned int numberOfRows;

    // context:
    std::string contextStr;

    // Online Parameters:
    unsigned int numberOfBitErrorEvents;

    // Private Methods:
    void parseInputData(const Configuration& config);
    void prepareWeakCells();
    void markBitFlips();
    unsigned int getNumberOfFlips(double temp, sc_core::sc_time time);
    void setContext(DecodedAddress addr);
    unsigned int getBit(DecodedAddress key, unsigned int byte,
                        unsigned int bitInByte);
    unsigned int getBit(int row, int column, int byteInColumn, int bitInByte);

    // Input related data structures:

    struct errors {
        double independent;
        double dependent;
    };

    //    temperature          time     number of errors
    //         |                 |        |
    std::map<double, std::map<sc_core::sc_time, errors> > errorMap;

    unsigned int maxNumberOfWeakCells;
    unsigned int maxNumberOfDepWeakCells;
    double maxTemperature;
    sc_core::sc_time maxTime;

    // Storage of weak cells:
    struct weakCell {
        unsigned int row;
        unsigned int col;
        unsigned int bit;
        bool flipped;
        bool dependent;
    };

    weakCell *weakCells;

    // To use a map for storing the data a comparing function must be defined
    struct DecodedAddressComparer
    {
        bool operator() (const DecodedAddress &first ,
                         const DecodedAddress &second) const
        {
            if (first.row == second.row)
                return first.column < second.column;
            else
                return first.row < second.row;
        }
    };

    // The data structure stores complete column accesses
    // A DRAM burst will be splitted up in several column accesses
    // e.g. BL=4 means that 4 elements will be added to the dataMap!
    std::map<DecodedAddress, unsigned char *, DecodedAddressComparer> dataMap;

    // An array to save when the last ACT/REF to a row happened:
    sc_core::sc_time *lastRowAccess;

    // Context Variables (will be written by the first dram access)
    int myChannel;
    int myRank;
    int myBankgroup;
    int myBank;
};

#endif // ERRORMODEL_H
