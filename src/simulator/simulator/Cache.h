/*
 * Copyright (c) 2022, RPTU Kaiserslautern-Landau
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
 *    Lukas Steiner
 *    Derek Christ
 */

#pragma once

#include "MemoryManager.h"

#include <cstdint>
#include <list>
#include <queue>
#include <systemc>
#include <tlm>
#include <tlm_utils/peq_with_cb_and_phase.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

class Cache : public sc_core::sc_module
{
public:
    tlm_utils::simple_initiator_socket<Cache> iSocket;
    tlm_utils::simple_target_socket<Cache> tSocket;

    Cache(const sc_core::sc_module_name& name,
          std::size_t size,
          std::size_t associativity,
          std::size_t lineSize,
          std::size_t mshrDepth,
          std::size_t writeBufferDepth,
          std::size_t maxTargetListSize,
          bool storageEnabled,
          sc_core::sc_time cycleTime,
          std::size_t hitCycles,
          MemoryManager& memoryManager);
    SC_HAS_PROCESS(Cache);

private:
    void peqCallback(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase);

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& fwDelay);
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                       tlm::tlm_phase& phase,
                                       sc_core::sc_time& bwDelay);
    unsigned int transport_dbg(tlm::tlm_generic_payload& trans);

    void fetchLineAndSendEndRequest(tlm::tlm_generic_payload& trans);
    void clearInitiatorBackpressureAndProcessBuffers();
    void sendEndResponseAndFillLine(tlm::tlm_generic_payload& trans);
    void clearTargetBackpressureAndProcessLines(tlm::tlm_generic_payload& trans);

    tlm_utils::peq_with_cb_and_phase<Cache> payloadEventQueue;

    const bool storageEnabled;
    sc_core::sc_time cycleTime;
    const sc_core::sc_time hitLatency;
    const std::size_t size;

    // Lines per set.
    const std::size_t associativity;

    const std::size_t lineSize;
    const std::size_t numberOfSets;
    const std::size_t indexShifts;
    const std::size_t indexMask;
    const std::size_t tagShifts;

    const std::size_t mshrDepth;
    const std::size_t writeBufferDepth;
    const std::size_t maxTargetListSize;

    using index_t = std::uint64_t;
    using tag_t = std::uint64_t;
    using lineOffset_t = std::uint64_t;

    struct CacheLine
    {
        tag_t tag = 0;
        unsigned char* dataPtr = nullptr;
        bool allocated = false;
        bool valid = false;
        bool dirty = false;
        sc_core::sc_time lastAccessTime = sc_core::SC_ZERO_TIME;
    };

    std::vector<std::vector<CacheLine>> lineTable;
    std::vector<uint8_t> dataMemory;

    bool isHit(index_t index, tag_t tag) const;
    bool isHit(std::uint64_t address) const;

    void writeLine(index_t index,
                   tag_t tag,
                   lineOffset_t lineOffset,
                   unsigned int dataLength,
                   const unsigned char* dataPtr);

    void readLine(index_t index,
                  tag_t tag,
                  lineOffset_t lineOffset,
                  unsigned int dataLength,
                  unsigned char* dataPtr);

    CacheLine* evictLine(index_t index);

    std::tuple<index_t, tag_t, lineOffset_t> decodeAddress(std::uint64_t address) const;
    std::uint64_t encodeAddress(index_t index, tag_t tag, lineOffset_t lineOffset = 0) const;

    struct BufferEntry
    {
        index_t index;
        tag_t tag;
        tlm::tlm_generic_payload* trans;

        BufferEntry(index_t index, tag_t tag, tlm::tlm_generic_payload* trans) :
            index(index),
            tag(tag),
            trans(trans)
        {
        }
    };

    struct Mshr
    {
        index_t index;
        tag_t tag;
        std::list<tlm::tlm_generic_payload*> requestList;

        /// Whether the Mshr entry was already issued to the target.
        bool issued = false;

        /// Whether the hit delay was already accounted for.
        /// Used to determine if the next entry in the request list
        /// should be sent out without the hit delay.
        bool hitDelayAccounted = false;

        /// Whether the hit delay is being awaited on.
        /// Used prevent other MSHR targets to wait on the hit
        /// delay when it is already being waited on.
        bool hitDelayStarted = false;

        Mshr(index_t index, tag_t tag, tlm::tlm_generic_payload* request) :
            index(index),
            tag(tag),
            requestList{request}
        {
        }
    };

    std::deque<Mshr> mshrQueue;
    std::deque<BufferEntry> hitQueue;

    using WriteBuffer = std::list<BufferEntry>;
    WriteBuffer writeBuffer;

    uint64_t numberOfHits = 0;
    uint64_t numberOfPrimaryMisses = 0;
    uint64_t numberOfSecondaryMisses = 0;

    std::uint64_t getAlignedAddress(std::uint64_t address) const;

    void processMshrResponse();
    void processWriteBuffer();
    void processHitQueue();
    void processMshrQueue();

    bool tSocketBackpressure = false;

    // Request to the target
    tlm::tlm_generic_payload* requestInProgress = nullptr;

    // Backpressure on initiator
    tlm::tlm_generic_payload* endRequestPending = nullptr;

    sc_core::sc_time lastEndReq = sc_core::sc_max_time();

    void fillLine(tlm::tlm_generic_payload& trans);
    void accessCacheAndSendResponse(tlm::tlm_generic_payload& trans);
    static void allocateLine(CacheLine* line, tag_t tag);

    bool isAllocated(index_t index, tag_t tag) const;
    bool hasBufferSpace() const;

    sc_core::sc_time ceilTime(const sc_core::sc_time& inTime) const;
    sc_core::sc_time ceilDelay(const sc_core::sc_time& inDelay) const;

    MemoryManager& memoryManager;
};
