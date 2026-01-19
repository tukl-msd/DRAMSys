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

#ifndef ADDRESSDECODER_H
#define ADDRESSDECODER_H

#include "DRAMSys/configuration/memspec/MemSpec.h"
#include "DRAMSys/configuration/json/AddressMapping.h"
#include <vector>
#include <bitset>

namespace DRAMSys
{

struct DecodedAddress
{
    unsigned channel = 0;
    unsigned rank = 0;
    unsigned stack = 0;
    unsigned bankgroup = 0;
    unsigned bank = 0;
    unsigned row = 0;
    unsigned column = 0;
    unsigned byte = 0;
    unsigned burst = 0;

    DecodedAddress(unsigned channel,
                   unsigned rank,
                   unsigned stack,
                   unsigned bankgroup,
                   unsigned bank,
                   unsigned row,
                   unsigned column) :
        channel(channel),
        rank(rank),
        stack(stack),
        bankgroup(bankgroup),
        bank(bank),
        row(row),
        column(column)
    {}

    DecodedAddress() = default;
};

struct AddressComponent {
    AddressComponent() = default;
    explicit AddressComponent(unsigned idx, unsigned length, std::string_view name) :
        idx(idx),
        length(length),
        name(name)
    {
    }
    unsigned idx = 0;
    unsigned length = 0;
    std::string_view name;
};

class AddressDecoder
{
public:
    AddressDecoder(const ::DRAMSys::Config::AddressMapping& addressMapping);

    /**
     * @brief Checks if the decoded address is valid according to the memory specification.
     * 
     * @param decAddr The address to check.
     * @return true if the address is valid, false otherwise.
     */
    [[nodiscard]] static bool isAddressValid(const MemSpec& memSpec, const DecodedAddress& decAddr);

    /**
     * @brief Decodes an address from a transaction payload into its address components.
     * 
     * @param address The encoded address.
     * @return The decoded address.
     */
    [[nodiscard]] DecodedAddress decodeAddress(uint64_t address) const;

    /**
     * @brief Decodes the channel component from an encoded address.
     * 
     * @param encAddr The encoded address.
     * @return The decoded channel number.
     */
    [[nodiscard]] unsigned decodeChannel(uint64_t encAddr) const;

    /**
     * @brief Encodes a DecodedAddress into an address value.
     * 
     * @param decAddr The decoded address to encode.
     * @return The encoded address.
     */
    [[nodiscard]] uint64_t encodeAddress(DecodedAddress decAddr) const;

        /**
     * @brief Checks if all address mapping bits are used and validates compatibility with the memory specification.
     * 
     * @param memSpec The memory specification to check.
     */
    void plausibilityCheck(const MemSpec& memSpec);

    /**
     * @brief Prints the current address mapping configuration.
     */
    void print() const;

private:
    static constexpr unsigned ADDRESS_WIDTH = 64;

    uint64_t highestBitValue;
    uint64_t burstBitMask;
    std::vector<std::bitset<ADDRESS_WIDTH>> mappingMatrix;
    std::vector<std::bitset<ADDRESS_WIDTH>> transposedMappingMatrix;

    AddressComponent byteBits;
    AddressComponent burstBits;
    AddressComponent columnBits;
    AddressComponent bankGroupBits;
    AddressComponent bankBits;
    AddressComponent rowBits;
    AddressComponent pseudochannelBits;
    AddressComponent channelBits;
    AddressComponent rankBits;
    AddressComponent stackBits;

    uint64_t upperBoundAddress;

    static unsigned int getHighestBit(Config::AddressMapping const& addressMapping);

    /**
     * @brief Checks if a given value is a power of two.
     * 
     * @param value The value to check.
     * @return true if the value is a power of two, false otherwise.
     */
    [[nodiscard]] static bool isPowerOfTwo(unsigned value);

    /**
     * @brief Transposes a matrix of 64-bit bitsets.
     * 
     * @param matrix The matrix to transpose.
     * @return The transposed matrix.
     */
    [[nodiscard]] static std::vector<std::bitset<ADDRESS_WIDTH>> transposeMatrix(const std::vector<std::bitset<ADDRESS_WIDTH>>& matrix);

    /**
     * @brief Multiplies a 64-bit vector with a matrix over GF(2).
     * 
     * @param inputVec The input vector to multiply.
     * @param matrix The GF(2) matrix.
     * @return The result of the multiplication as a 64-bit unsinged integer.
     */
    [[nodiscard]] static uint64_t gf2Multiplication(const uint64_t& inputVec, const std::vector<std::bitset<ADDRESS_WIDTH>>& matrix);

    /**
     * @brief Validates that the addressable value matches the value from the memory specification.
     * 
     * @param memSpecValue The value from the memory specification.
     * @param addressableValue The calculated addressable value.
     * @param name The name of the component.
     */
    static void validateAddressableLimit(unsigned memSpecValue, unsigned addressableValue, const std::string& name);

    /**
     * @brief Checks if the address mapping is compatible with the memory specification.
     * 
     * @param memSpec The memory specification.
     */
    void checkMemSpecCompatibility(const MemSpec& memSpec) const;

    /**
     * @brief Checks if the addressable limits for each memory component are sufficient.
     * 
     * @param memSpec The memory specification.
     */
    void checkAddressableLimits(const MemSpec& memSpec) const;

    /**
     * @brief Calculates and checks the number of byte bits required for the memory specification.
     * 
     * @param memSpec The memory specification.
     */
    void checkByteBits(const MemSpec& memSpec) const;

    /**
     * @brief Calculates and checks the number of burst bits required for the memory specification.
     * 
     * @param memSpec The memory specification.
     */
    void checkBurstBits(const MemSpec& memSpec) const;

    /**
     * @brief Checks and handles burst lengths that are not a power of two.
     * 
     * @param memSpec The memory specification.
     */
    void checkBurstLengthBits(const MemSpec& memSpec);
};

} // namespace DRAMSys

#endif // ADDRESSDECODER_H
