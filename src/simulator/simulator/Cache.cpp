/*
 * Copyright (c) 2022, Technische Universität Kaiserslautern
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

#include "Cache.h"
#include "MemoryManager.h"

#include <cstring>

using namespace tlm;
using namespace sc_core;

DECLARE_EXTENDED_PHASE(HIT_HANDLING);
DECLARE_EXTENDED_PHASE(MISS_HANDLING);

Cache::Cache(const sc_module_name& name,
             std::size_t size,
             std::size_t associativity,
             std::size_t lineSize,
             std::size_t mshrDepth,
             std::size_t writeBufferDepth,
             std::size_t maxTargetListSize,
             bool storageEnabled,
             sc_core::sc_time cycleTime,
             std::size_t hitCycles,
             MemoryManager& memoryManager) :
    sc_module(name),
    payloadEventQueue(this, &Cache::peqCallback),
    storageEnabled(storageEnabled),
    cycleTime(cycleTime),
    hitLatency(cycleTime * static_cast<double>(hitCycles)),
    size(size),
    associativity(associativity),
    lineSize(lineSize),
    numberOfSets(size / (lineSize * associativity)),
    indexShifts(static_cast<std::size_t>(std::log2(lineSize))),
    indexMask(numberOfSets - 1),
    tagShifts(indexShifts + static_cast<std::size_t>(std::log2(numberOfSets))),
    mshrDepth(mshrDepth),
    writeBufferDepth(writeBufferDepth),
    maxTargetListSize(maxTargetListSize),
    memoryManager(memoryManager)
{
    iSocket.register_nb_transport_bw(this, &Cache::nb_transport_bw);
    tSocket.register_nb_transport_fw(this, &Cache::nb_transport_fw);

    lineTable =
        std::vector<std::vector<CacheLine>>(numberOfSets, std::vector<CacheLine>(associativity));

    if (storageEnabled)
    {
        dataMemory.reserve(size);

        for (std::size_t set = 0; set < lineTable.size(); set++)
        {
            for (std::size_t way = 0; way < lineTable[set].size(); way++)
                lineTable[set][way].dataPtr =
                    dataMemory.data() + set * associativity * lineSize + way * lineSize;
        }
    }
}

tlm_sync_enum Cache::nb_transport_fw(tlm_generic_payload& trans,
                                     tlm_phase& phase,
                                     sc_time& delay) // core side --->
{
    if (phase == BEGIN_REQ)
    {
        if (trans.get_data_length() > lineSize)
        {
            SC_REPORT_FATAL(name(),
                            "Accesses larger than line size in non-blocking mode not supported!");
        }

        trans.acquire();
    }

    // TODO: early completion would be possible
    payloadEventQueue.notify(trans, phase, ceilDelay(delay));

    return TLM_ACCEPTED;
}

tlm_sync_enum Cache::nb_transport_bw(tlm_generic_payload& trans,
                                     tlm_phase& phase,
                                     sc_time& bwDelay) // DRAM side <---
{
    // TODO: early completion would be possible
    payloadEventQueue.notify(trans, phase, ceilDelay(bwDelay));

    return TLM_ACCEPTED;
}

void Cache::peqCallback(tlm_generic_payload& trans, const tlm_phase& phase)
{
    if (phase == BEGIN_REQ) // core side --->
    {
        assert(!endRequestPending);
        fetchLineAndSendEndRequest(trans);
        return;
    }

    if (phase == END_REQ) // <--- DRAM side
    {
        lastEndReq = sc_time_stamp();
        clearInitiatorBackpressureAndProcessBuffers();
        return;
    }

    if (phase == BEGIN_RESP && &trans == requestInProgress) // <--- DRAM side
    {
        // Shortcut, 2 phases in one
        clearInitiatorBackpressureAndProcessBuffers();
        sendEndResponseAndFillLine(trans);
        return;
    }

    if (phase == BEGIN_RESP) // <--- DRAM side
    {
        sendEndResponseAndFillLine(trans);
        return;
    }

    if (phase == END_RESP) // core side --->
    {
        clearTargetBackpressureAndProcessLines(trans);
        return;
    }

    if (phase == HIT_HANDLING) // direct hit, account for the hit delay
    {
        index_t index = 0;
        tag_t tag = 0;
        std::tie(index, tag, std::ignore) = decodeAddress(trans.get_address());

        hitQueue.emplace_back(index, tag, &trans);
        processHitQueue();
    }
    else if (phase == MISS_HANDLING) // fetched MSHR entry available, account for the hit delay
    {
        accessCacheAndSendResponse(trans);

        index_t index = 0;
        tag_t tag = 0;
        std::tie(index, tag, std::ignore) = decodeAddress(trans.get_address());

        auto mshrIt = std::find_if(mshrQueue.begin(),
                                   mshrQueue.end(),
                                   [index, tag](const Mshr& mshr)
                                   { return mshr.index == index && mshr.tag == tag; });

        assert(mshrIt != mshrQueue.end());
        mshrIt->hitDelayAccounted = true;

        if (mshrIt->requestList.empty())
        {
            mshrQueue.erase(mshrIt);

            if (endRequestPending != nullptr && hasBufferSpace())
            {
                payloadEventQueue.notify(*endRequestPending, BEGIN_REQ, SC_ZERO_TIME);
                endRequestPending = nullptr;
            }
        }
    }
    else
    {
        SC_REPORT_FATAL("Cache", "PEQ was triggered with unknown phase");
    }
}

/// Handler for begin request from core side.
void Cache::fetchLineAndSendEndRequest(tlm_generic_payload& trans)
{
    if (hasBufferSpace())
    {
        index_t index = 0;
        tag_t tag = 0;
        std::tie(index, tag, std::ignore) = decodeAddress(trans.get_address());

        auto mshrEntry = std::find_if(mshrQueue.begin(),
                                      mshrQueue.end(),
                                      [index, tag](const Mshr& entry)
                                      { return (index == entry.index) && (tag == entry.tag); });

        if (isHit(index, tag))
        {
            numberOfHits++;

            // Handle hit
            // Account for the 1 cycle accept delay.
            payloadEventQueue.notify(trans, HIT_HANDLING, hitLatency + cycleTime);
        }
        else if (mshrEntry != mshrQueue.end()) // Miss with outstanding previous Miss, noted in MSHR
        {
            numberOfSecondaryMisses++;
            assert(isAllocated(index, tag));

            // A fetch for this cache line is already in progress
            // Add request to the existing Mshr entry

            if (mshrEntry->requestList.size() == maxTargetListSize)
            {
                // Insertion into requestList in mshrEntry not possible.
                endRequestPending = &trans;
                return;
            }

            mshrEntry->requestList.emplace_back(&trans);
        }
        else // Miss without MSHR entry:
        {
            numberOfPrimaryMisses++;
            assert(!isAllocated(index, tag));

            // Cache miss and no fetch in progress.
            // So evict line and allocate empty line.
            auto* evictedLine = evictLine(index);
            if (evictedLine == nullptr)
            {
                // Line eviction not possible.
                endRequestPending = &trans;
                return;
            }

            allocateLine(evictedLine, tag);
            mshrQueue.emplace_back(index, tag, &trans);

            processMshrQueue();
            processWriteBuffer();
        }

        tlm_phase bwPhase = END_REQ;
        sc_time bwDelay = SC_ZERO_TIME;
        tSocket->nb_transport_bw(trans, bwPhase, bwDelay);
    }
    else
    {
        endRequestPending = &trans;
    }
}

/// Handler for end request from DRAM side.
void Cache::clearInitiatorBackpressureAndProcessBuffers()
{
    requestInProgress = nullptr;

    processMshrQueue();
    processWriteBuffer();
}

/// Handler for begin response from DRAM side.
void Cache::sendEndResponseAndFillLine(tlm_generic_payload& trans)
{
    tlm_phase fwPhase = END_RESP;
    sc_time fwDelay = SC_ZERO_TIME;
    iSocket->nb_transport_fw(trans, fwPhase, fwDelay);

    if (trans.is_read())
    {
        fillLine(trans);
        processMshrResponse();
    }

    trans.release();
}

/// Handler for end response from core side.
void Cache::clearTargetBackpressureAndProcessLines(tlm_generic_payload& trans)
{
    trans.release();
    tSocketBackpressure = false;

    processHitQueue();
    processMshrResponse();

    // When the cache currently only handles hits from the hit queue and
    // no other requests to the DRAM side are pending, there is a chance
    // that a endRequestPending will never be served.
    // To circumvent this, pass it into the system again at this point.
    if (endRequestPending != nullptr && hasBufferSpace())
    {
        payloadEventQueue.notify(*endRequestPending, BEGIN_REQ, SC_ZERO_TIME);
        endRequestPending = nullptr;
    }
}

unsigned int Cache::transport_dbg(tlm_generic_payload& trans)
{
    return iSocket->transport_dbg(trans);
}

bool Cache::isHit(index_t index, tag_t tag) const
{
    return std::find_if(lineTable[index].begin(),
                        lineTable[index].end(),
                        [tag](const CacheLine& cacheLine) {
                            return (cacheLine.tag == tag) && cacheLine.valid;
                        }) != lineTable[index].end();
}

bool Cache::isHit(uint64_t address) const
{
    index_t index = 0;
    tag_t tag = 0;
    std::tie(index, tag, std::ignore) = decodeAddress(address);

    return isHit(index, tag);
}

std::tuple<Cache::index_t, Cache::tag_t, Cache::lineOffset_t>
Cache::decodeAddress(uint64_t address) const
{
    return {(address >> indexShifts) & indexMask, address >> tagShifts, address % lineSize};
}

uint64_t
Cache::encodeAddress(Cache::index_t index, Cache::tag_t tag, Cache::lineOffset_t lineOffset) const
{
    return static_cast<uint64_t>(tag << tagShifts) | index << indexShifts | lineOffset;
}

/// Write data to an available cache line, update flags
void Cache::writeLine(index_t index,
                      tag_t tag,
                      lineOffset_t lineOffset,
                      unsigned int dataLength,
                      const unsigned char* dataPtr)
{
    // SC_REPORT_ERROR("cache", "Write to Cache not allowed!");

    CacheLine& currentLine =
        *std::find_if(lineTable[index].begin(),
                      lineTable[index].end(),
                      [tag](const CacheLine& cacheLine) { return cacheLine.tag == tag; });

    assert(currentLine.valid);
    currentLine.lastAccessTime = sc_time_stamp();
    currentLine.dirty = true;

    if (storageEnabled)
        std::copy(dataPtr, dataPtr + dataLength, currentLine.dataPtr + lineOffset);
}

/// Read data from an available cache line, update flags
void Cache::readLine(index_t index,
                     tag_t tag,
                     lineOffset_t lineOffset,
                     unsigned int dataLength,
                     unsigned char* dataPtr)
{
    CacheLine& currentLine =
        *std::find_if(lineTable[index].begin(),
                      lineTable[index].end(),
                      [tag](const CacheLine& cacheLine) { return cacheLine.tag == tag; });

    assert(currentLine.valid);
    currentLine.lastAccessTime = sc_time_stamp();

    if (storageEnabled)
        std::copy(currentLine.dataPtr + lineOffset,
                  currentLine.dataPtr + lineOffset + dataLength,
                  dataPtr);
}

/// Tries to evict oldest line (insert into write memory)
/// Returns the line or a nullptr if not possible
Cache::CacheLine* Cache::evictLine(Cache::index_t index)
{
    CacheLine& oldestLine = *std::min_element(lineTable[index].begin(),
                                              lineTable[index].end(),
                                              [](const CacheLine& lhs, const CacheLine& rhs)
                                              { return lhs.lastAccessTime < rhs.lastAccessTime; });

    if (oldestLine.allocated && !oldestLine.valid)
    {
        // oldestline is allocated but not yet valid -> fetch in progress
        return nullptr;
    }
    if (std::find_if(mshrQueue.begin(),
                     mshrQueue.end(),
                     [index, oldestLine](const Mshr& entry) {
                         return (index == entry.index) && (oldestLine.tag == entry.tag);
                     }) != mshrQueue.end())
    {
        // TODO: solve this in a more clever way
        // There are still entries in mshrQueue to the oldest line -> do not evict it
        return nullptr;
    }
    if (std::find_if(hitQueue.begin(),
                     hitQueue.end(),
                     [index, oldestLine](const BufferEntry& entry) {
                         return (index == entry.index) && (oldestLine.tag == entry.tag);
                     }) != hitQueue.end())
    {
        // TODO: solve this in a more clever way
        // There are still hits in hitQueue to the oldest line -> do not evict it
        return nullptr;
    }

    if (oldestLine.valid && oldestLine.dirty)
    {
        auto& wbTrans = memoryManager.allocate(lineSize);
        wbTrans.acquire();
        wbTrans.set_address(encodeAddress(index, oldestLine.tag));
        wbTrans.set_write();
        wbTrans.set_data_length(lineSize);
        wbTrans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        if (storageEnabled)
            std::copy(oldestLine.dataPtr, oldestLine.dataPtr + lineSize, wbTrans.get_data_ptr());

        writeBuffer.emplace_back(index, oldestLine.tag, &wbTrans);
    }

    oldestLine.allocated = false;
    oldestLine.valid = false;
    oldestLine.dirty = false;

    return &oldestLine;
}

/// Align address to cache line size
uint64_t Cache::getAlignedAddress(uint64_t address) const
{
    return address - (address % lineSize);
}

/// Issue read requests for entries in the MshrQueue to the target
void Cache::processMshrQueue()
{
    if ((requestInProgress == nullptr) && !mshrQueue.empty())
    {
        // Get the first entry that wasn't already issued to the target
        auto mshrIt = std::find_if(
            mshrQueue.begin(), mshrQueue.end(), [](const Mshr& entry) { return !entry.issued; });

        if (mshrIt == mshrQueue.end())
            return;

        // Note: This is the same address for all entries in the requests list
        uint64_t alignedAddress = getAlignedAddress(mshrIt->requestList.front()->get_address());

        index_t index = 0;
        tag_t tag = 0;
        std::tie(index, tag, std::ignore) = decodeAddress(alignedAddress);

        // Search through the writeBuffer in reverse order to get the most recent entry.
        auto writeBufferEntry =
            std::find_if(writeBuffer.rbegin(),
                         writeBuffer.rbegin(),
                         [index, tag](const BufferEntry& entry)
                         { return (index == entry.index) && (tag == entry.tag); });

        if (writeBufferEntry != writeBuffer.rbegin())
        {
            // There is an entry for the required line in the write buffer.
            // Snoop into it and get the data from there instead of the dram.
            mshrIt->issued = true;
            clearInitiatorBackpressureAndProcessBuffers();

            fillLine(*writeBufferEntry->trans);
            processMshrResponse();

            return;
        }

        // Prevents that the cache line will get fetched multiple times from the target
        mshrIt->issued = true;

        auto& fetchTrans = memoryManager.allocate(lineSize);
        fetchTrans.acquire();
        fetchTrans.set_read();
        fetchTrans.set_data_length(lineSize);
        fetchTrans.set_streaming_width(lineSize);
        fetchTrans.set_address(alignedAddress);
        fetchTrans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        tlm_phase fwPhase = BEGIN_REQ;

        // Always account for the cycle as either we just received the BEGIN_REQ
        // or we cleared the backpressure from another END_REQ.
        sc_time fwDelay = cycleTime;

        requestInProgress = &fetchTrans;
        tlm_sync_enum returnValue = iSocket->nb_transport_fw(fetchTrans, fwPhase, fwDelay);

        if (returnValue == tlm::TLM_UPDATED)
        {
            // END_REQ or BEGIN_RESP
            payloadEventQueue.notify(fetchTrans, fwPhase, fwDelay);
        }
        else if (returnValue == tlm::TLM_COMPLETED)
        {
            clearInitiatorBackpressureAndProcessBuffers();

            fillLine(fetchTrans);
            processMshrResponse();

            fetchTrans.release();
        }

        if (endRequestPending != nullptr && hasBufferSpace())
        {
            payloadEventQueue.notify(*endRequestPending, BEGIN_REQ, SC_ZERO_TIME);
            endRequestPending = nullptr;
        }
    }
}

/// Processes writeBuffer (dirty cache line evictions)
void Cache::processWriteBuffer()
{
    if ((requestInProgress == nullptr) && !writeBuffer.empty())
    {
        tlm_generic_payload& wbTrans = *writeBuffer.front().trans;

        tlm_phase fwPhase = BEGIN_REQ;
        sc_time fwDelay = (lastEndReq == sc_time_stamp()) ? cycleTime : SC_ZERO_TIME;

        requestInProgress = &wbTrans;
        tlm_sync_enum returnValue = iSocket->nb_transport_fw(wbTrans, fwPhase, fwDelay);

        if (returnValue == tlm::TLM_UPDATED)
        {
            // END_REQ or BEGIN_RESP
            payloadEventQueue.notify(wbTrans, fwPhase, fwDelay);
        }
        else if (returnValue == tlm::TLM_COMPLETED)
        {
            clearInitiatorBackpressureAndProcessBuffers();
            wbTrans.release();
        }

        if (endRequestPending != nullptr && hasBufferSpace())
        {
            payloadEventQueue.notify(*endRequestPending, BEGIN_REQ, SC_ZERO_TIME);
            endRequestPending = nullptr;
        }

        writeBuffer.pop_front();
    }
}

/// Fill allocated cache line with data from memory
void Cache::fillLine(tlm_generic_payload& trans)
{
    index_t index = 0;
    tag_t tag = 0;
    std::tie(index, tag, std::ignore) = decodeAddress(trans.get_address());

    CacheLine& allocatedLine = *std::find_if(
        lineTable[index].begin(),
        lineTable[index].end(),
        [tag](const CacheLine& cacheLine) { return cacheLine.allocated && cacheLine.tag == tag; });

    allocatedLine.valid = true;
    allocatedLine.dirty = false;

    if (storageEnabled)
        std::copy(trans.get_data_ptr(), trans.get_data_ptr() + lineSize, allocatedLine.dataPtr);
}

/// Make cache access for pending hits
void Cache::processHitQueue()
{
    if (!tSocketBackpressure && !hitQueue.empty())
    {
        auto hit = hitQueue.front();
        hitQueue.pop_front();

        accessCacheAndSendResponse(*hit.trans);
    }
}

/// Access the available cache line and send the response
void Cache::accessCacheAndSendResponse(tlm_generic_payload& trans)
{
    assert(!tSocketBackpressure);
    assert(isHit(trans.get_address()));

    auto [index, tag, lineOffset] = decodeAddress(trans.get_address());

    if (trans.is_read())
        readLine(index, tag, lineOffset, trans.get_data_length(), trans.get_data_ptr());
    else
        writeLine(index, tag, lineOffset, trans.get_data_length(), trans.get_data_ptr());

    tlm_phase bwPhase = BEGIN_RESP;
    sc_time bwDelay = SC_ZERO_TIME;

    trans.set_response_status(tlm::TLM_OK_RESPONSE);

    tlm_sync_enum returnValue = tSocket->nb_transport_bw(trans, bwPhase, bwDelay);
    if (returnValue == tlm::TLM_UPDATED) // TODO tlm_completed
        payloadEventQueue.notify(trans, bwPhase, bwDelay);

    tSocketBackpressure = true;
}

/// Allocates an empty line for later filling (lastAccessTime = sc_max_time())
void Cache::allocateLine(CacheLine* line, tag_t tag)
{
    line->allocated = true;
    line->tag = tag;
    line->lastAccessTime = sc_max_time();
}

/// Checks whether a line with the corresponding tag is already allocated (fetch in progress or
/// already valid)
bool Cache::isAllocated(Cache::index_t index, Cache::tag_t tag) const
{
    return std::find_if(lineTable[index].begin(),
                        lineTable[index].end(),
                        [tag](const CacheLine& cacheLine) {
                            return (cacheLine.tag == tag) && cacheLine.allocated;
                        }) != lineTable[index].end();
}

/// Process oldest hit in mshrQueue, accept pending request from initiator
void Cache::processMshrResponse()
{
    if (!tSocketBackpressure) // TODO: Bedingung eigentlich zu streng, wenn man Hit delay
                              // berücksichtigt.
    {
        const auto hitIt =
            std::find_if(mshrQueue.begin(),
                         mshrQueue.end(),
                         [this](const Mshr& entry) { return isHit(entry.index, entry.tag); });

        // In case there are hits in mshrActive, handle them. Otherwise try again later.
        if (hitIt == mshrQueue.end())
            return;

        // Another MSHR target already started the modeling of the hit delay.
        // Try again later.
        if (hitIt->hitDelayStarted && !hitIt->hitDelayAccounted)
            return;

        // Get the first request in the list and respond to it
        tlm_generic_payload& returnTrans = *hitIt->requestList.front();
        hitIt->requestList.pop_front();

        if (hitIt->hitDelayAccounted)
            accessCacheAndSendResponse(returnTrans);
        else
        {
            hitIt->hitDelayStarted = true;
            payloadEventQueue.notify(returnTrans, MISS_HANDLING, hitLatency);
            return;
        }

        if (hitIt->requestList.empty())
        {
            mshrQueue.erase(hitIt);

            if (endRequestPending != nullptr && hasBufferSpace())
            {
                payloadEventQueue.notify(*endRequestPending, BEGIN_REQ, SC_ZERO_TIME);
                endRequestPending = nullptr;
            }
        }
    }
}

/// Checks whether both mshrActive and writeBuffer have memory space
bool Cache::hasBufferSpace() const
{
    return mshrQueue.size() < mshrDepth && writeBuffer.size() < writeBufferDepth;
}

sc_time Cache::ceilTime(const sc_time& inTime) const
{
    return std::ceil(inTime / cycleTime) * cycleTime;
}

sc_time Cache::ceilDelay(const sc_time& inDelay) const
{
    sc_time inTime = sc_time_stamp() + inDelay;
    sc_time outTime = ceilTime(inTime);
    sc_time outDelay = outTime - sc_time_stamp();
    return outDelay;
}
