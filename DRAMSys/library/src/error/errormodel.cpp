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

#include "errormodel.h"
#include "../common/DebugManager.h"
#include "../simulation/TemperatureController.h"
#include "../simulation/AddressDecoder.h"
#include "../common/dramExtensions.h"

#include <random>
#include <chrono>
#include <bitset>
#include <cmath>
#include <sstream>
#include <fstream>

using namespace sc_core;

errorModel::errorModel(const sc_module_name& name, const Configuration& config,
                       TemperatureController& temperatureController, libDRAMPower *dramPower)
        : sc_module(name), memSpec(*config.memSpec), temperatureController(temperatureController)
{
    this->DRAMPower = dramPower;
    init(config);
}

void errorModel::init(const Configuration& config)
{
    powerAnalysis = config.powerAnalysis;
    thermalSim = config.thermalSimulation;
    // Get Configuration parameters:
    burstLenght = config.memSpec->defaultBurstLength;
    numberOfColumns = config.memSpec->columnsPerRow;
    bytesPerColumn = std::log2(config.memSpec->dataBusWidth);

    // Adjust number of bytes per column dynamically to the selected ecc controller
    //TODO: bytesPerColumn = Configuration::getInstance().adjustNumBytesAfterECC(bytesPerColumn);

    numberOfRows = config.memSpec->rowsPerBank;
    numberOfBitErrorEvents = 0;


    // Initialize the lastRow Access array:
    lastRowAccess = new sc_time[numberOfRows];
    for (unsigned int i = 0; i < numberOfRows; i++)
        lastRowAccess[i] = SC_ZERO_TIME;

    // The name is set when the context is clear.
    contextStr = "";

    // Parse data input:
    parseInputData(config);
    prepareWeakCells();

    // Initialize context variables:
    myChannel = -1;
    myRank = -1;
    myBankgroup = -1;
    myBank = -1;

    // Test 1:
    // If you want to test the function that get the number
    // of bit errors for a given temperature and time
    // uncomment the following lines:
    //
    //std::cout << "MAXTemp:" << maxTemperature << std::endl;
    //std::cout << "MAXTime:" << maxTime << std::endl;
    //getNumberOfFlips(45.0,sc_time(200.0,SC_MS));
    //getNumberOfFlips(45.0,sc_time(190.0,SC_MS));
    //getNumberOfFlips(45.0,sc_time(180.0,SC_MS));
    //getNumberOfFlips(75.0,sc_time(200.0,SC_MS));
    //getNumberOfFlips(75.0,sc_time(190.0,SC_MS));
    //getNumberOfFlips(75.0,sc_time(180.0,SC_MS));
    //getNumberOfFlips(85.0,sc_time(200.0,SC_MS));
    //getNumberOfFlips(85.0,sc_time(190.0,SC_MS));
    //getNumberOfFlips(85.0,sc_time(180.0,SC_MS));
    //getNumberOfFlips(88.0,sc_time(200.0,SC_MS));
    //getNumberOfFlips(88.0,sc_time(190.0,SC_MS));
    //getNumberOfFlips(88.0,sc_time(180.0,SC_MS));
    //getNumberOfFlips(89.0,sc_time(64.0,SC_MS));
    //getNumberOfFlips(89.0,sc_time(64.0,SC_MS));
    //getNumberOfFlips(89.0,sc_time(64.0,SC_MS));

    // Test 2:
    // X X X
    // X 1 1
    // X 1 1
    //weakCells[0].bit = 0;
    //weakCells[0].col = 0;
    //weakCells[0].row = 0;
    //weakCells[0].dependent = true;

    markBitFlips();
}

errorModel::~errorModel()
{
    // Remove all data from the dataMap:
    for (std::map<DecodedAddress, unsigned char *>::iterator it = dataMap.begin();
            it != dataMap.end(); ++it ) {
        delete it->second;
    }
    // Delete all elements from the dataMap:
    dataMap.clear();

    // Remove all data from the lastRowAccess
    delete [] lastRowAccess;

    // Clean errorMap
    errorMap.clear();

    // Clean list of weak cells:
    delete [] weakCells;

    // If an access happened to a bank the numner of error events should be shown:
    if (myChannel != -1 && myBank != -1 && myBankgroup != -1 && myRank != -1 ) {
        std::cout << contextStr
                  << ": Number of Retention Error Events = " << numberOfBitErrorEvents
                  << std::endl;
    }
}

void errorModel::store(tlm::tlm_generic_payload &trans)
{
    // Check wich bits have flipped during the last access and mark them as flipped:
    markBitFlips();

    // Get the key for the dataMap from the transaction's dram extension:
    DramExtension &ext = DramExtension::getExtension(trans);
    DecodedAddress key = DecodedAddress(ext.getChannel().ID(), ext.getRank().ID(),
                                        ext.getBankGroup().ID(), ext.getBank().ID(),
                                        ext.getRow().ID(), ext.getColumn().ID(), 0);
    // Set context:
    setContext(key);

    std::stringstream msg;
    msg << "bank: " << key.bank << " group: " << key.bankgroup << " bytes: " <<
        key.byte << " channel: " << key.channel << " column: " << key.column <<
        " rank: " << key.rank << " row: " << key.row;
    PRINTDEBUGMESSAGE(name(), msg.str());

    // Check if the provided data length is correct:
    assert((bytesPerColumn * burstLenght) == trans.get_data_length());

    PRINTDEBUGMESSAGE(name(), ("Data length: " + std::to_string(trans.get_data_length()) +
                       " bytesPerColumn: " + std::to_string(bytesPerColumn)).c_str());

    // Handle the DRAM burst,
    for (unsigned int i = 0; i < trans.get_data_length(); i += bytesPerColumn) {
        unsigned char *data;

        // Check if address is not already stored:
        if (dataMap.count(key) == 0) {
            // Generate a new data entry
            data = new unsigned char[bytesPerColumn];
        } else { // In case the address was stored before:
            data = dataMap[key];
        }

        // Copy the data from the transaction to the data pointer
        memcpy(data, trans.get_data_ptr() + i, bytesPerColumn);

        // Save part of the burst in the dataMap
        // TODO: Check if we can have double entries, is key unique?
        dataMap.insert(std::pair<DecodedAddress, unsigned char *>(key, data));

        // Reset flipped weak cells in this area, since they are rewritten now
        for (unsigned int j = 0; j < maxNumberOfWeakCells; j++) {
            // If the current written column in a row has a week cell:
            if (weakCells[j].row == key.row && weakCells[j].col == key.column) {
                // If the bit was marked as flipped due to a retention error
                // mark it as unflipped:
                if (weakCells[j].flipped == true) {
                    weakCells[j].flipped = false;
                }
            }
        }

        // The next burst element is handled, therfore the column address must be increased
        key.column++;

        // Check that there is no column overfow:
        std::stringstream msg;
        msg << "key.column is " << key.column << " columnsPerRow is " <<
            numberOfColumns;
        PRINTDEBUGMESSAGE(name(), msg.str());
        assert(key.column <= numberOfColumns);
    }
}

void errorModel::load(tlm::tlm_generic_payload &trans)
{
    // Check wich bits have flipped during the last access and mark them as flipped:
    markBitFlips();

    // Get the key for the dataMap from the transaction's dram extension:
    DramExtension &ext = DramExtension::getExtension(trans);
    DecodedAddress key = DecodedAddress(ext.getChannel().ID(), ext.getRank().ID(),
                                        ext.getBankGroup().ID(), ext.getBank().ID(),
                                        ext.getRow().ID(), ext.getColumn().ID(), 0);

    // Set context:
    setContext(key);

    // Check if the provided data length is correct:
    assert((bytesPerColumn * burstLenght) == trans.get_data_length());

    // Handle the DRAM burst:
    for (unsigned int i = 0; i < trans.get_data_length(); i += bytesPerColumn) {
        // Check if address is not stored:
        if (dataMap.count(key) == 0) {
            SC_REPORT_FATAL("errormodel", "Reading from an empty memory location");
        }

        // Copy the dataMap to the transaction data pointer
        memcpy(trans.get_data_ptr() + i, dataMap[key], bytesPerColumn);

        // The next burst element is handled, therfore the column address must be increased
        key.column++;

        // Check that there is no column overfow:
        assert(key.column <= numberOfColumns);
    }
}

void errorModel::markBitFlips()
{
    double temp = getTemperature();
    for (unsigned int row = 0;
            row < memSpec.rowsPerBank; row++) {
        // If the row has never been accessed ignore it and go to the next one
        if (lastRowAccess[row] != SC_ZERO_TIME) {
            // Get the time interval between now and the last acivate/refresh
            sc_time interval = sc_time_stamp() - lastRowAccess[row];

            // Obtain the number of bit flips for the current temperature and the time interval:
            unsigned int n = getNumberOfFlips(temp, interval);

            // Check if the current row is in the range of bit flips for this interval
            // and temperature, if yes mark it as flipped:
            for (unsigned int i = 0; i < n; i++) {
                // Check if Bit has marked as flipped yet, if yes mark it as flipped
                if (!weakCells[i].flipped && weakCells[i].row == row) {
                    std::stringstream msg;
                    msg << "Maked weakCell[" << i << "] as flipped" << std::endl;
                    PRINTDEBUGMESSAGE(name(), msg.str());

                    weakCells[i].flipped = true;
                }
            }
        }
    }
}

void errorModel::refresh(unsigned int row)
{
    // A refresh is internally composed of ACT and PRE that are executed
    // on all banks, therefore we call the activate method:
    activate(row);
}

void errorModel::activate(unsigned int row)
{
    // Check wich bits have flipped during the last access and mark them as flipped:
    markBitFlips();

    // The Activate command is responsible that an retention error is manifested.
    // Therefore, Flip the bit in the data structure if it is marked as flipped
    // and if it is a one. Transisitons from 0 to 1 are only happening
    // in DRAM with anticells. This behavior is not implemented yet.
    for (unsigned int i = 0; i < maxNumberOfWeakCells; i++) {
        if (weakCells[i].flipped == true && weakCells[i].row == row) {
            // Estimate key to access column data
            DecodedAddress key;
            key.bank = myBank;
            key.bankgroup = myBankgroup;
            key.channel = myChannel;
            key.column = weakCells[i].col;
            key.rank = myRank;
            key.row = row;

            // Byte position in column:
            unsigned int byte = weakCells[i].bit / 8;

            // Bit position in byte:
            unsigned int bitInByte = weakCells[i].bit % 8;

            // Check if the bit is 1 (only 1->0 transitions are supported)
            // DRAMs based on anti cells are not supported yet by this model
            if (getBit(key, byte, bitInByte) == 1) {
                // Prepare bit mask: invert mask and AND it later
                unsigned char mask = pow(2, bitInByte);
                mask = ~mask;

                // Temporal storage for modification:
                unsigned char tempByte;

                if (weakCells[i].dependent == false) {
                    // Load the affected byte to tempByte
                    memcpy(&tempByte, dataMap[key] + byte, 1);

                    // Flip the bit:
                    tempByte = (tempByte & mask);

                    // Output on the Console:
                    std::stringstream msg;
                    msg << "Bit Flipped!"
                        << " row: " << key.row
                        << " col: " << key.column
                        << " bit: " << weakCells[i].bit;
                    PRINTDEBUGMESSAGE(name(), msg.str());

                    numberOfBitErrorEvents++;

                    // Copy the modified byte back to the dataMap:
                    memcpy(dataMap[key] + byte, &tempByte, 1);
                } else { // if(weakCells[i].dependent == true)
                    // Get the neighbourhood of the bit and store it in the
                    // grid variable:
                    //        | 0 1 2 |
                    // grid = | 3 4 5 |
                    //        | 6 7 8 |

                    unsigned int grid[9];

                    grid[0] = getBit(key.row - 1, key.column, byte, bitInByte - 1);
                    grid[1] = getBit(key.row - 1, key.column, byte, bitInByte  );
                    grid[2] = getBit(key.row - 1, key.column, byte, bitInByte + 1);

                    grid[3] = getBit(key.row  , key.column, byte, bitInByte - 1);
                    grid[4] = getBit(key.row  , key.column, byte, bitInByte  );
                    grid[5] = getBit(key.row  , key.column, byte, bitInByte + 1);

                    grid[6] = getBit(key.row + 1, key.column, byte, bitInByte - 1);
                    grid[7] = getBit(key.row + 1, key.column, byte, bitInByte  );
                    grid[8] = getBit(key.row + 1, key.column, byte, bitInByte + 1);

                    unsigned int sum = 0;
                    for (int s = 0; s < 9; s++) {
                        sum += grid[s];
                    }

                    if (sum <= 4) {
                        // Load the affected byte to tempByte
                        memcpy(&tempByte, dataMap[key] + byte, 1);

                        // Flip the bit:
                        tempByte = (tempByte & mask);
                        numberOfBitErrorEvents++;

                        // Copy the modified byte back to the dataMap:
                        memcpy(dataMap[key] + byte, &tempByte, 1);

                        // Output on the Console:
                        std::stringstream msg;
                        msg << "Dependent Bit Flipped!"
                            << " row: " << key.row
                            << " col: " << key.column
                            << " bit: " << weakCells[i].bit
                            << " sum: " << sum << std::endl
                            << grid[0] << grid[1] << grid[2] << std::endl
                            << grid[3] << grid[4] << grid[5] << std::endl
                            << grid[6] << grid[7] << grid[8];
                        PRINTDEBUGMESSAGE(name(), msg.str());
                    } else {
                        // Output on the Console:
                        std::stringstream msg;
                        msg << "Dependent Bit NOT Flipped!"
                            << " row: " << key.row
                            << " col: " << key.column
                            << " bit: " << weakCells[i].bit
                            << " sum: " << sum << std::endl
                            << grid[0] << grid[1] << grid[2] << std::endl
                            << grid[3] << grid[4] << grid[5] << std::endl
                            << grid[6] << grid[7] << grid[8];
                        PRINTDEBUGMESSAGE(name(), msg.str());
                    }
                }
            }
        }
    }

    lastRowAccess[row] = sc_time_stamp();
}

// This method is used to get a bit with a key, usually for independent case:
unsigned int errorModel::getBit(DecodedAddress key, unsigned int byteInColumn,
                                unsigned int bitInByte)
{
    // If the data was not writte by the produce yet it is zero:
    if (dataMap.count(key) == 0) {
        return 0;
    } else { // Return the value of the bit
        unsigned char tempByte;

        // Copy affected byte to a temporal variable:
        memcpy(&tempByte, dataMap[key] + byteInColumn, 1);
        unsigned char mask = pow(2, bitInByte);
        unsigned int result = (tempByte & mask) >> bitInByte;
        std::bitset<8> x(mask);

        std::stringstream msg;
        msg << "mask = " << x
            << " bitInByte = " << bitInByte
            << " tempByte = " << (unsigned int)tempByte
            << " result = " << result;

        PRINTDEBUGMESSAGE(name(), msg.str());

        return result;
    }
}

// This method is used to get neighbourhoods, for the dependent case:
unsigned int errorModel::getBit(int row, int column, int byteInColumn,
                                int bitInByte)
{
    // Border-Exception handling:

    // Switch the byte if bit under/overflow:
    if (bitInByte < 0) {
        byteInColumn--;
        bitInByte = 7;
    } else if (bitInByte >= 8) {
        byteInColumn++;
        bitInByte = 0;
    }

    // Switch the column if byte under/overflow
    if (byteInColumn < 0) {
        column--;
        byteInColumn = bytesPerColumn;
    } else if (byteInColumn >= int(byteInColumn)) {
        column++;
        byteInColumn = 0;
    }

    // If we switch the row we return 0 (culumn under/overflow)
    if (column < 0) {
        return 0;
    } else if (column >= int(numberOfColumns)) {
        return 0;
    }

    // Row over/underflow return 0
    if (row < 0) {
        return 0;
    } else if (row >= int(numberOfRows)) {
        return 0;
    }

    DecodedAddress key;
    key.bank = myBank;
    key.bankgroup = myBankgroup;
    key.channel = myChannel;
    key.rank = myRank;
    key.column = column;
    key.row = row;

    return getBit(key, byteInColumn, bitInByte);
}

double errorModel::getTemperature()
{
    // FIXME
    // make sure the context is set (myChannel has the proper value) before
    // requesting the temperature.
    double temperature = 89;

    if (this->myChannel != -1)
    {
        if (thermalSim && powerAnalysis)
        {
            // TODO
            // check if this is best way to request information to DRAMPower.
            unsigned long long clk_cycles = sc_time_stamp() / memSpec.tCK;
            DRAMPower->calcWindowEnergy(clk_cycles);
            float average_power = (float)DRAMPower->getPower().average_power;
            temperature = temperatureController.getTemperature(this->myChannel, average_power);
        } else {
            temperature = temperatureController.getTemperature(this->myChannel, 0);
        }
    }

    return temperature;
}

void errorModel::parseInputData(const Configuration& config)
{
    std::string fileName = config.errorCSVFile;
    std::ifstream inputFile(fileName);

    if (inputFile.is_open()) {
        std::string line;
        while (std::getline(inputFile, line)) {
            std::istringstream iss(line);
            std::string str_temperature;
            std::string str_retentionTime;
            std::string str_mu_independent;
            std::string str_sigma_independent;
            std::string str_mu_dependent;
            std::string str_sigma_dependent;

            // Parse file:
            iss >> str_temperature
                >> str_retentionTime
                >> str_mu_independent
                >> str_sigma_independent
                >> str_mu_dependent
                >> str_sigma_dependent;

            double temp = std::stod(str_temperature.c_str(), 0);
            sc_time retentionTime    = sc_time(std::stod(str_retentionTime.c_str(), 0),
                                               SC_MS);

            unsigned int mu_independent    = std::stod(str_mu_independent.c_str(), 0);
            unsigned int sigma_independent = std::stod(str_sigma_independent.c_str(), 0);
            unsigned int mu_dependent      = std::stod(str_mu_dependent.c_str(), 0);
            unsigned int sigma_dependent   = std::stod(str_sigma_dependent.c_str(), 0);

            errors e;

            //calculate normal distribution of # of independent errors
            unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::default_random_engine generator(seed);
            std::normal_distribution<double> distribution(mu_independent,
                                                          sigma_independent);
            e.independent = ceil(distribution(generator));

            // calculate normal distribution of # of dependent errors
            unsigned seed2 = std::chrono::system_clock::now().time_since_epoch().count();
            std::default_random_engine generator2(seed2);
            std::normal_distribution<double> distribution2(mu_dependent, sigma_dependent);
            e.dependent = ceil(distribution2(generator2));

            // Store parsed data to the errorMap:
            errorMap[temp][retentionTime] = e;

            std::stringstream msg;
            msg << "Temperature = " << temp
                << " Time = " << retentionTime
                << " independent = " << errorMap[temp][retentionTime].independent
                << " dependent = " << errorMap[temp][retentionTime].dependent;

            PRINTDEBUGMESSAGE(name(), msg.str());
        }
        inputFile.close();
    } else {
        SC_REPORT_FATAL("errormodel", "Cannot open ErrorCSVFile");
    }
}

void errorModel::prepareWeakCells()
{
    // Get the Maxium number of weak cells by iterating over the errorMap:
    maxNumberOfWeakCells = 0;
    maxNumberOfDepWeakCells = 0;
    for ( const auto &i : errorMap ) {
        for ( const auto &j : i.second ) {
            // Get number of dependent weak cells:
            if ( j.second.dependent > maxNumberOfDepWeakCells) {
                maxNumberOfDepWeakCells = j.second.dependent;
            }

            // Get the total number of weak cells (independet + dependent):
            if ( j.second.independent + j.second.dependent > maxNumberOfWeakCells) {
                maxNumberOfWeakCells = j.second.independent + j.second.dependent;
            }
        }
    }

    // Get the highest temperature in the error map:
    maxTemperature = 0;
    for ( const auto &i : errorMap ) {
        if (i.first > maxTemperature) {
            maxTemperature = i.first;
        }
    }

    // Get the highest time in the error map:
    maxTime = SC_ZERO_TIME;
    for ( const auto &i : errorMap ) {
        for ( const auto &j : i.second ) {
            if (j.first > maxTime) {
                maxTime = j.first;
            }
        }
    }

    // Generate weak cells:

    weakCells = new weakCell[maxNumberOfWeakCells];

    for (unsigned int i = 0; i < maxNumberOfWeakCells; i++) {
        unsigned int row, col, bit;

        // Select positions of weak cells randomly, uniformly distributed:
        row = (unsigned int) (rand() % numberOfRows);
        col = (unsigned int) (rand() % numberOfColumns);
        bit = (unsigned int) (rand() % (bytesPerColumn * 8));

        // Test if weak cell has been chosen already before:
        bool found = false;
        for (unsigned int k = 0; k < i; k++) {
            if ((weakCells[k].row == row) && (weakCells[k].col == col)
                    && (weakCells[k].bit == bit)) {
                found = true;
                break;
            }
        }
        // If a cell was already choosen as weak we have to roll the dice again:
        if (found) {
            i--;
        } else {
            weakCells[i].row = row;         // Row in the bank
            weakCells[i].col = col;         // Column in the row
            weakCells[i].bit = bit;         // Bit position in column
            weakCells[i].flipped =
                false;    // Flag whether this position has already flipped
            weakCells[i].dependent =
                false; // init dependency flag with false, dependent cells will be estimated in the next step
        }
    }

    // Generate dependent weak cells:
    for (unsigned int i = 1; i <= maxNumberOfDepWeakCells; i++) {
        unsigned int r = (rand() % maxNumberOfWeakCells);

        // If the dependent weak cell was choosen before roll the dice again:
        if (weakCells[r].dependent) {
            i--;
        } else {
            weakCells[r].dependent = true;
        }

    }

    // Debug output where the weak cells are located:
    for (unsigned int i = 0; i < maxNumberOfWeakCells; i++) {
        std::stringstream msg;
        msg << "row="   << weakCells[i].row
            << " col=" << weakCells[i].col
            << " bit=" << weakCells[i].bit
            << " flip=" << weakCells[i].flipped
            << " dep=" << weakCells[i].dependent;
        PRINTDEBUGMESSAGE(name(), msg.str());
    }
}

// Retrieve number of flipping bits which fits best to temperature input and time since last refresh
unsigned int errorModel::getNumberOfFlips(double temp, sc_time time)
{
    // Check if the provided temperature and retention time are in a valid
    // range that is covered by the input data stored in the errorMap. In case
    // of values out of the valid range the simulation will be aborted.
    if (temp > maxTemperature) {
        SC_REPORT_FATAL("errormodel", "temperature out of range");
    }

    if (time > maxTime) {
        SC_REPORT_FATAL("errormodel", "time out of range");
    }

    // Find nearest temperature:
    double nearestTemperature = 0;
    for ( const auto &i : errorMap ) {
        if (i.first >= temp) { // for worst case reasons we go to the next bin
            nearestTemperature = i.first;
            break;
        }
    }

    // Find nearest time:
    sc_time nearestTime;
    for ( const auto &i : errorMap[nearestTemperature]) {
        if (i.first >= time) { // for worst case reasons we go to the next bin
            nearestTime = i.first;
            break;
        }
    }

    errors e = errorMap[nearestTemperature][nearestTime];

    //std::stringstream msg;
    //msg << "ACT/REF temp:" << temp
    //    << " time:" << time
    //    << " nearestTemp:" << nearestTemperature
    //    << " nearestTime:" << nearestTime
    //    << " ind:" << e.independent
    //    << " dep:" << e.dependent;

    //printDebugMessage(msg.str());

    return e.independent + e.dependent;
}

void errorModel::setContext(DecodedAddress addr)
{
    // This function is called the first store ore load to get the context in
    // which channel, rank or bank the error model is used.
    if (myChannel == -1 && myBank == -1 && myBankgroup == -1 && myRank == -1 ) {
        myChannel = addr.channel;
        myBank = addr.bank;
        myBankgroup = addr.bankgroup;
        myRank = addr.rank;

        contextStr = "Channel_" + std::to_string(myChannel) + "_Bank_" + std::to_string(
                         myBank) + " ";
    }
}
