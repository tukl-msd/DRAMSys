/*
 * Copyright (c) 2025, RPTU Kaiserslautern-Landau
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
 *    Johannes Feldmann
 *    Lukas Steiner
 *    Luiza Correa
 *    Derek Christ
 *    Thomas Zimmermann
 */

#include "AddressDecoder.h"
#include "DRAMSys/config/AddressMapping.h"

#include <bitset>
#include <cmath>
#include <iostream>
#include <cstdint>

namespace DRAMSys
{
/********************/
/* Helper Functions */
/********************/
/**
 * @brief Creates a bitmask and stores it in a uint64_t.
 * 
 * @param numBits The number of bits to set to 1.
 * @param startIndex The index of the first bit to set to 1.
 * @return result The uint64_t where the bitmask will be stored.
 */
uint64_t createBitmask(unsigned numBits, unsigned startIndex) {
    // Create the mask by shifting 1's to the correct position
    return ((UINT64_C(1) << numBits) - 1) << startIndex;
}

std::vector<std::bitset<AddressDecoder::ADDRESS_WIDTH>> AddressDecoder::transposeMatrix(const std::vector<std::bitset<ADDRESS_WIDTH>>& matrix) {
    size_t size = matrix.size();
    std::vector<std::bitset<ADDRESS_WIDTH>> transposedMatrix(size);

    for (size_t i = 0; i < size; ++i) {
        for (size_t j = 0; j < ADDRESS_WIDTH; ++j) {
            if (matrix[i].test(j))
            transposedMatrix[j].set(i);
        }
    }
    return transposedMatrix;
}

uint64_t AddressDecoder::gf2Multiplication(const uint64_t& inputVec, const std::vector<std::bitset<ADDRESS_WIDTH>>& matrix)
{
    #if defined(__clang__) || defined(__GNUC__)
        uint64_t result = 0;
        for (size_t i = 0; i < matrix.size(); ++i) {
            uint64_t row = matrix[i].to_ullong();
            uint64_t val = inputVec & row;
            bool parity = __builtin_parityll(val) != 0;
            result |= (uint64_t(parity) << i);
        }
        return result;
    #else
        std::bitset<ADDRESS_WIDTH> resultBits;
        std::bitset<ADDRESS_WIDTH> inputBits(inputVec);

        for (size_t i = 0; i < matrix.size(); ++i) {                
            resultBits[i] = (inputBits & matrix[i]).count() % 2;
        }
        return resultBits.to_ullong();
    #endif

    // Print input, mapping matrix and output in a readable way (useful for debugging)
    // std::cout << "Vec " << ":\t" << std::bitset<ADDRESS_WIDTH>(vector[0]) << std::endl << std::endl;
    // for (size_t i = 0; i < mappingMatrix.size(); ++i) {
    //     std::cout << "Row " << i << ":\t" << mappingMatrix[i] << " | " << resultBits[i] << std::endl;
    // }
}


/****************************/
/* AddressDecoder Functions */
/****************************/

AddressDecoder::AddressDecoder(const DRAMSys::Config::AddressMapping& addressMapping) :
    highestBitValue(getHighestBit(addressMapping)),
    mappingMatrix(std::vector<std::bitset<ADDRESS_WIDTH>>(highestBitValue + 1)),
    upperBoundAddress(static_cast<uint64_t>(std::pow(2, highestBitValue + 1) - 1))
{
    auto addBitsToMatrix =
        [&](std::vector<Config::AddressMapping::BitEntry> const& bits,
            int* rowIndex,
            std::string_view name) -> AddressComponent
    {
        for (auto const& row : bits)
        {
            for (unsigned int bit : row)
            {
                mappingMatrix[*rowIndex][bit] = true;
            }
            (*rowIndex)++;
        }
        // Care: The rowIndex has been changed. We want the lowest bit, so we must subtract the length! 
        return AddressComponent(*rowIndex - bits.size(), bits.size(), name);
    };

    auto entryToVector = [](std::optional<std::vector<Config::AddressMapping::BitEntry>> const& entry) -> std::vector<Config::AddressMapping::BitEntry> {
        std::vector<Config::AddressMapping::BitEntry> bitVector;
        
        if (entry.has_value())
            bitVector = entry.value();

        return bitVector;
    };

    int rowIndex = 0;
    byteBits          = addBitsToMatrix(entryToVector(addressMapping.BYTE_BIT), &rowIndex, "By");
    burstBits         = addBitsToMatrix(entryToVector(addressMapping.BURST_BIT), &rowIndex, "Bu");
    columnBits        = addBitsToMatrix(entryToVector(addressMapping.COLUMN_BIT), &rowIndex, "Co");
    bankGroupBits     = addBitsToMatrix(entryToVector(addressMapping.BANKGROUP_BIT), &rowIndex, "BG");
    bankBits          = addBitsToMatrix(entryToVector(addressMapping.BANK_BIT), &rowIndex, "Ba");
    rowBits           = addBitsToMatrix(entryToVector(addressMapping.ROW_BIT), &rowIndex, "Ro");
    pseudochannelBits = addBitsToMatrix(entryToVector(addressMapping.PSEUDOCHANNEL_BIT), &rowIndex, "PC");
    channelBits       = addBitsToMatrix(entryToVector(addressMapping.CHANNEL_BIT), &rowIndex, "Ch");
    rankBits          = addBitsToMatrix(entryToVector(addressMapping.RANK_BIT), &rowIndex, "Ra");
    stackBits         = addBitsToMatrix(entryToVector(addressMapping.STACK_BIT), &rowIndex, "St");
    transposedMappingMatrix = transposeMatrix(mappingMatrix);

    bankgroupsPerRank = std::lround(std::pow(2, bankGroupBits.length));
    banksPerGroup = std::lround(std::pow(2, bankBits.length));
}

void AddressDecoder::plausibilityCheck(const MemSpec& memSpec)
{
    np2Flag = not allComponentsArePowerOfTwo(memSpec);

    // Check if all address bits are used
    // TODO: Check if every bit occurs ~exactly~ once or just at least once?
    std::bitset<ADDRESS_WIDTH> orBitset(0);
    for (auto bitset: mappingMatrix) {
        orBitset |= bitset;
    }
    

    std::bitset<ADDRESS_WIDTH> mask((1ULL << (highestBitValue + 1)) - 1);
    if (orBitset != mask) {
        SC_REPORT_FATAL("AddressDecoder", "Not all address bits are used exactly once");
    }

    // Check if the addresss mapping is capable of matching the requirements of the memSpec
    checkMemSpecCompatibility(memSpec);
    checkMemorySize(memSpec);
    // Look for dedicated BURST_BITS. If used -> byte-bits and column-based burst-bits are combined 
    if (burstBits.length > 0) {
        if (byteBits.length > 0) {
            SC_REPORT_FATAL("AddressDecoder", "BURST_BITS and BYTE_BITS cannot be used at the same time."); 
        }
        checkBurstBits(memSpec);
    } else {
        checkByteBits(memSpec);
        checkBurstLengthBits(memSpec);
    }
}

bool AddressDecoder::allComponentsArePowerOfTwo(const MemSpec& memSpec) {
    // TODO: What parts do we need to check?
    return isPowerOfTwo(memSpec.numberOfChannels) &&
           isPowerOfTwo(memSpec.stacksPerChannel) &&
           isPowerOfTwo(memSpec.ranksPerChannel) &&
           isPowerOfTwo(memSpec.bankGroupsPerChannel) &&
           isPowerOfTwo(memSpec.banksPerChannel) &&
           isPowerOfTwo(memSpec.devicesPerRank) &&
           isPowerOfTwo(memSpec.rowsPerBank) &&
           isPowerOfTwo(memSpec.columnsPerRow);
}

void AddressDecoder::checkMemorySize(const MemSpec& memSpec) const {
    bool isMemorySizeMismatch = memSpec.getSimMemSizeInBytes() > upperBoundAddress + 1 ||
        (memSpec.getSimMemSizeInBytes() < upperBoundAddress + 1 && !np2Flag);

    if (isMemorySizeMismatch) {
        std::ostringstream o;
            o << "The mapped bits do not match the memory size field (MemSpec size: " 
            << memSpec.getSimMemSizeInBytes() << ", mapping=" << upperBoundAddress + 1 << ")";
        SC_REPORT_FATAL("AddressDecoder", o.str().c_str());
    }
}

void AddressDecoder::checkMemSpecCompatibility(const MemSpec& memSpec) const {
    const unsigned channels = std::lround(std::pow(2, channelBits.length));
    const unsigned pseudochannels = std::lround(std::pow(2, pseudochannelBits.length));
    const unsigned ranks = std::lround(std::pow(2, rankBits.length));
    const unsigned rows = std::lround(std::pow(2, rowBits.length));
    const unsigned columns = std::lround(std::pow(2, columnBits.length));

    const unsigned absoluteBankGroups = bankgroupsPerRank * (ranks * pseudochannels);
    const unsigned absoluteBanks = banksPerGroup * absoluteBankGroups;
    const unsigned effectiveRanksPerChannel = static_cast<uint64_t>(ranks) * static_cast<uint64_t>(pseudochannels);

    // Depending on the NP2 flag we might need to adapt the strictness of the checks
    auto cmp = np2Flag
        ? std::function<bool(uint64_t,uint64_t)>([](uint64_t a, uint64_t b){ return a <= b; })
        : std::function<bool(uint64_t,uint64_t)>([](uint64_t a, uint64_t b){ return a == b; });

    // Check all and collect the errors
    std::vector<std::string> errors;
    const char* rel = np2Flag ? "<=" : "==";
    auto check = [&](const char* field, uint64_t mem, uint64_t map) {
        if (!cmp(mem, map)) {
            std::ostringstream o;
            o << field << ": memSpec=" << mem << ", mapping=" << map
              << " (required: memSpec " << rel << " mapping)";
            errors.push_back(o.str());
        }
    };

    check("numberOfChannels",     memSpec.numberOfChannels,     channels);
    check("ranksPerChannel",      memSpec.ranksPerChannel,      effectiveRanksPerChannel);
    check("bankGroupsPerChannel", memSpec.bankGroupsPerChannel, absoluteBankGroups);
    check("banksPerChannel",      memSpec.banksPerChannel,      absoluteBanks);
    check("rowsPerBank",          memSpec.rowsPerBank,          rows);
    check("columnsPerRow",        memSpec.columnsPerRow,        columns);

    // Handle collected errors
    if (!errors.empty()) {
        std::ostringstream all;
        all << "Unexpected deviations between MemSpec and Address Mapping:\n";
        for (auto& e : errors) all << " - " << e << '\n';
        std::string msg = all.str();
        SC_REPORT_FATAL("AddressDecoder", msg.c_str());
    }
}

void AddressDecoder::checkAddressableLimits(const MemSpec& memSpec) const {
    validateAddressableLimit(memSpec.numberOfChannels, static_cast<unsigned>(std::pow(2, channelBits.length)), "Channel");
    validateAddressableLimit(memSpec.ranksPerChannel, static_cast<unsigned>(std::pow(2, bankBits.length)), "Rank");
    unsigned addressableBankGroups = static_cast<unsigned>(std::pow(2, bankGroupBits.length)) * static_cast<unsigned>(std::pow(2, rankBits.length));
    unsigned absoluteBanks = static_cast<unsigned>(std::pow(2, bankBits.length)) * addressableBankGroups;
    validateAddressableLimit(memSpec.bankGroupsPerChannel, addressableBankGroups, "Bank group");
    validateAddressableLimit(memSpec.banksPerChannel, absoluteBanks, "Bank");
    validateAddressableLimit(memSpec.rowsPerBank, static_cast<unsigned>(std::pow(2, rowBits.length)), "Row");
    validateAddressableLimit(memSpec.columnsPerRow, static_cast<unsigned>(std::pow(2, columnBits.length)), "Column");
}

void AddressDecoder::validateAddressableLimit(unsigned memSpecValue, unsigned addressableValue, const std::string& name) {
    if (memSpecValue > addressableValue || memSpecValue <= (addressableValue >> 1)) {
        SC_REPORT_FATAL("AddressDecoder", (name + " bit mapping does not match the memspec configuration").c_str());
    }
}

void AddressDecoder::checkBurstBits(const MemSpec& memSpec) const {
    unsigned numOfBurstBits = std::ceil(std::log2(memSpec.defaultDataBytesPerBurst));

    if (burstBits.length != numOfBurstBits) {
        SC_REPORT_FATAL("AddressDecoder", 
            ("Burst-bits are not sufficient for this burst size or not continous starting from 0. (defaultBurstLength: " + 
            std::to_string(memSpec.defaultBurstLength) + 
            "B -> number of byte-bits: " + 
            std::to_string(numOfBurstBits) + ")").c_str());
    }
}

void AddressDecoder::checkByteBits(const MemSpec& memSpec) const {
    unsigned bytesPerBeat = memSpec.defaultBytesPerBurst / memSpec.defaultBurstLength;
    unsigned numOfByteBits = std::ceil(std::log2(bytesPerBeat));

    if (!isPowerOfTwo(bytesPerBeat)) {
        SC_REPORT_WARNING("AddressDecoder", 
            ("Bytes per beat are not power of two! \nAssuming " + 
                std::to_string(numOfByteBits) + " reserved byte bits.").c_str());
    }

    if (byteBits.length < numOfByteBits) {
        SC_REPORT_FATAL("AddressDecoder", 
            ("Byte-bits are not continuous starting from 0. (bytesPerBeat: " + 
            std::to_string(bytesPerBeat) + 
            "B -> number of byte-bits: " + 
            std::to_string(numOfByteBits) + ")").c_str());
    }
}

void AddressDecoder::checkBurstLengthBits(const MemSpec& memSpec) {
    unsigned numOfMaxBurstLengthBits = std::ceil(std::log2(memSpec.maxBurstLength));
    burstBitMask = createBitmask(numOfMaxBurstLengthBits, byteBits.length);

    if (!isPowerOfTwo(memSpec.maxBurstLength)) {
        SC_REPORT_WARNING("AddressDecoder", 
            ("Maximum burst length (" + std::to_string(memSpec.maxBurstLength) +
            ") is not power of two! \nAssuming " + 
            std::to_string(numOfMaxBurstLengthBits) + 
            " reserved burst bits.").c_str());
    }

    std::bitset<ADDRESS_WIDTH> burstBitset(((1 << numOfMaxBurstLengthBits) - 1) << columnBits.idx); 
    std::bitset<ADDRESS_WIDTH> columnBitset;
    for (size_t i = 0; i < columnBits.length; i++) {
        columnBitset |= mappingMatrix[columnBits.idx + i];
    }
    if ((columnBits.length < numOfMaxBurstLengthBits) || ((columnBitset & burstBitset) != burstBitset)) {
        SC_REPORT_FATAL("AddressDecoder", 
            ("No continuous column bits for maximum burst length (maximumBurstLength: " +
            std::to_string(memSpec.maxBurstLength) + 
            " -> required number of burst bits: " + 
            std::to_string(numOfMaxBurstLengthBits) + ")").c_str());
    }
}

DecodedAddress AddressDecoder::decodeAddress(uint64_t address) const
{
    uint64_t encAddr = address;
    if (encAddr > upperBoundAddress)
    {
        SC_REPORT_WARNING("AddressDecoder",
                          ("Address " + std::to_string(encAddr) +
                           " out of range (maximum address is " + std::to_string(upperBoundAddress) +
                           ")")
                              .c_str());
    }

    uint64_t result = gf2Multiplication(encAddr, mappingMatrix);

    /**
     * @brief Extracts a specific AddressComponent from the result address.
     */
    auto get_component = [&result](AddressComponent const& component) -> unsigned {
        // Create mask
        uint64_t mask = (1ULL << component.length) - 1;
        // Shift and apply the mask
        return static_cast<unsigned>((result >> component.idx) & mask);
    };

    DecodedAddress decAddr;
    decAddr.channel = get_component(channelBits);
    decAddr.rank = get_component(rankBits);
    decAddr.rank |= get_component(pseudochannelBits);
    decAddr.stack = get_component(stackBits);
    decAddr.bankgroup = get_component(bankGroupBits);
    decAddr.bank = get_component(bankBits);
    decAddr.row = get_component(rowBits);
    decAddr.column= get_component(columnBits);

    // Convert bankgroup and bank to absolute numbering
    decAddr.bankgroup = decAddr.bankgroup + (decAddr.rank * bankgroupsPerRank);
    decAddr.bank = decAddr.bank + (decAddr.bankgroup * banksPerGroup);

    return decAddr;
}

bool AddressDecoder::isAddressValid(const MemSpec& memSpec, const DecodedAddress& decAddr)
{
    // Check all address components for validity
    return (decAddr.channel < memSpec.numberOfChannels) &&
           (decAddr.stack < memSpec.stacksPerChannel) && (decAddr.rank < memSpec.ranksPerChannel) &&
           (decAddr.bankgroup < memSpec.bankGroupsPerChannel) &&
           (decAddr.bank < memSpec.banksPerChannel) && (decAddr.row < memSpec.rowsPerBank) &&
           (decAddr.column < memSpec.columnsPerRow);
}

unsigned AddressDecoder::decodeChannel(uint64_t encAddr) const
{
    if (encAddr > upperBoundAddress)
        SC_REPORT_WARNING("AddressDecoder",
                          ("Address " + std::to_string(encAddr) +
                           " out of range (maximum address is " + std::to_string(upperBoundAddress) +
                           ")")
                              .c_str());

    uint64_t result = gf2Multiplication(encAddr, mappingMatrix);

    /**
     * @brief Extracts a specific AddressComponent from the result address.
     */
    auto get_component = [&result](std::optional<AddressComponent> const& component) -> unsigned {
        if (!component.has_value()) {
            return static_cast<unsigned>(0);
        }
        // Create mask
        uint64_t mask = (1ULL << component->length) - 1;
        // Shift and apply the mask
        return static_cast<unsigned>((result >> component->idx) & mask);
    };

    return get_component(channelBits);
}

uint64_t AddressDecoder::encodeAddress(DecodedAddress decAddr) const
{
    // Convert absolute addressing for bank, bankgroup to relative
    decAddr.bankgroup = decAddr.bankgroup % bankgroupsPerRank;
    decAddr.bank = decAddr.bank % banksPerGroup;

    uint64_t mappedAddr = 0;

    /**
     * @brief Inserts a specific AddressComponent to the mappedAddress.
     */
    auto set_component = [&mappedAddr](std::optional<AddressComponent> const& component, const unsigned int value) -> unsigned {
        if (!component.has_value()) {
            return mappedAddr;
        }
        // Shift and add to mappedAddress
        return static_cast<unsigned>((value << component->idx) | mappedAddr);
    };

    mappedAddr = set_component(channelBits, decAddr.channel);
    mappedAddr = set_component(rankBits, decAddr.rank);
    mappedAddr = set_component(pseudochannelBits, decAddr.rank);
    mappedAddr = set_component(stackBits, decAddr.stack);
    mappedAddr = set_component(bankGroupBits, decAddr.bankgroup);
    mappedAddr = set_component(bankBits, decAddr.bank);
    mappedAddr = set_component(rowBits, decAddr.row);
    mappedAddr = set_component(columnBits, decAddr.column);

    return gf2Multiplication(mappedAddr, transposedMappingMatrix);
}

void AddressDecoder::print() const
{
    std::cout << headline << '\n';
    std::cout << "Used Address Mapping:\n\n";

    auto printBits = [&](const AddressComponent& component) {
        unsigned startIdx = component.idx;
        unsigned length = component.length;

        for (unsigned i = 0; i < length; ++i) {
            std::cout << " " << component.name << " " << std::setw(2) << mappingMatrix[startIdx + i] << '\n';
        }
    };

    if (byteBits.length > 0)
        printBits(byteBits);
    if (burstBits.length > 0)
        printBits(burstBits);
    if (columnBits.length > 0)
        printBits(columnBits);
    if (rowBits.length > 0)
        printBits(rowBits);
    if (bankBits.length > 0)
        printBits(bankBits);
    if (bankGroupBits.length > 0)
        printBits(bankGroupBits);
    if (stackBits.length > 0)
        printBits(stackBits);
    if (rankBits.length > 0)
        printBits(rankBits);
    if (pseudochannelBits.length > 0)
        printBits(pseudochannelBits);
    if (channelBits.length > 0)
        printBits(channelBits);
}

bool AddressDecoder::isPowerOfTwo(unsigned value) {
    return value != 0 && (value & (value - 1)) == 0;
}

unsigned int AddressDecoder::getHighestBit(Config::AddressMapping const& addressMapping)
{
    unsigned int highestBit = std::numeric_limits<unsigned int>::min();
    bool found = false;

    auto checkAndUpdate =
        [&](const std::optional<std::vector<Config::AddressMapping::BitEntry>>& bits)
    {
        if (bits)
        {
            for (const auto& vector : *bits)
            {
                for (const auto& bit : vector)
                {
                    if (bit > highestBit)
                    {
                        highestBit = bit;
                        found = true;
                    }
                }
            }
        }
    };

    checkAndUpdate(addressMapping.BYTE_BIT);
    checkAndUpdate(addressMapping.BURST_BIT);
    checkAndUpdate(addressMapping.COLUMN_BIT);
    checkAndUpdate(addressMapping.ROW_BIT);
    checkAndUpdate(addressMapping.BANK_BIT);
    checkAndUpdate(addressMapping.BANKGROUP_BIT);
    checkAndUpdate(addressMapping.RANK_BIT);
    checkAndUpdate(addressMapping.STACK_BIT);
    checkAndUpdate(addressMapping.PSEUDOCHANNEL_BIT);
    checkAndUpdate(addressMapping.CHANNEL_BIT);

    return found ? highestBit
                 : std::numeric_limits<unsigned int>::min(); // Rückgabe des höchsten Wertes oder
                                                             // des minimalen Wertes
}

} // namespace DRAMSys
