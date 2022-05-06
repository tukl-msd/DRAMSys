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
 */

#ifndef LENGTHCONVERTER_H
#define LENGTHCONVERTER_H

#include <iostream>
#include <utility>
#include <vector>
#include <queue>
#include <set>
#include <unordered_map>
#include <stack>

#include <tlm>
#include <systemc>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/peq_with_cb_and_phase.h>

//TLM_DECLARE_EXTENDED_PHASE(REQ_ARBITRATION);
//TLM_DECLARE_EXTENDED_PHASE(RESP_ARBITRATION);

class LengthConverter : public sc_core::sc_module
{
public:
    tlm_utils::simple_initiator_socket<LengthConverter> iSocket;
    tlm_utils::simple_target_socket<LengthConverter> tSocket;

    LengthConverter(const sc_core::sc_module_name& name, unsigned maxOutputLength, bool storageEnabled);
    SC_HAS_PROCESS(LengthConverter);

private:
    tlm_utils::peq_with_cb_and_phase<LengthConverter> payloadEventQueue;
    void peqCallback(tlm::tlm_generic_payload &payload, const tlm::tlm_phase &phase);

    //std::vector<bool> tSocketIsBusy;
    //std::vector<bool> iSocketIsBusy;

    const unsigned maxOutputLength;
    const bool storageEnabled;

    void createChildTranses(tlm::tlm_generic_payload* parentTrans);

    //std::uint64_t getTargetAddress(std::uint64_t address) const;
    //int getISocketId(std::uint64_t address) const;

    //std::vector<std::queue<tlm::tlm_generic_payload *>> pendingRequests;

    tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload &trans, tlm::tlm_phase &phase,
                                       sc_core::sc_time &fwDelay);
    tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload &payload, tlm::tlm_phase &phase,
                                       sc_core::sc_time &bwDelay);
    unsigned int transport_dbg(tlm::tlm_generic_payload &trans);

    class MemoryManager : public tlm::tlm_mm_interface
    {
    public:
        MemoryManager(bool storageEnabled, unsigned maxDataLength);
        ~MemoryManager() override;
        tlm::tlm_generic_payload* allocate();
        void free(tlm::tlm_generic_payload* payload) override;

    private:
        std::stack<tlm::tlm_generic_payload*> freePayloads;
        bool storageEnabled = false;
        unsigned maxDataLength;
    } memoryManager;

    class ChildExtension : public tlm::tlm_extension<ChildExtension>
    {
    private:
        tlm::tlm_generic_payload* parentTrans;
        explicit ChildExtension(tlm::tlm_generic_payload* parentTrans) : parentTrans(parentTrans) {}

    public:
        //ChildExtension() = delete;

        tlm_extension_base* clone() const override
        {
            return new ChildExtension(parentTrans);
        }

        void copy_from(tlm_extension_base const &ext) override
        {
            const auto& cpyFrom = dynamic_cast<const ChildExtension&>(ext);
            parentTrans = cpyFrom.parentTrans;
        }

        tlm::tlm_generic_payload* getParentTrans()
        {
            return parentTrans;
        }

        static void setExtension(tlm::tlm_generic_payload* childTrans, tlm::tlm_generic_payload* parentTrans)
        {
            auto *extension = childTrans->get_extension<ChildExtension>();

            if (extension != nullptr)
            {
                extension->parentTrans = parentTrans;
            }
            else
            {
                extension = new ChildExtension(parentTrans);
                childTrans->set_auto_extension(extension);
            }
        }

        static bool isChildTrans(const tlm::tlm_generic_payload* trans)
        {
            if (trans->get_extension<ChildExtension>() != nullptr)
                return true;
            else
                return false;
        }

        tlm::tlm_generic_payload* getNextChildTrans()
        {
            return parentTrans->get_extension<ParentExtension>()->getNextChildTrans();
        }

        bool notifyChildTransCompletion()
        {
            return parentTrans->get_extension<ParentExtension>()->notifyChildTransCompletion();
        }
    };

    class ParentExtension : public tlm::tlm_extension<ParentExtension>
    {
    private:
        std::vector<tlm::tlm_generic_payload*> childTranses;
        unsigned nextEndReqChildId = 0;
        unsigned completedChildTranses = 0;
        explicit ParentExtension(std::vector<tlm::tlm_generic_payload*> _childTranses)
            : childTranses(std::move(_childTranses)) {}

    public:
        ParentExtension() = delete;

        tlm_extension_base* clone() const override
        {
            return new ParentExtension(childTranses);
        }

        void copy_from(tlm_extension_base const &ext) override
        {
            const auto& cpyFrom = dynamic_cast<const ParentExtension&>(ext);
            childTranses = cpyFrom.childTranses;
        }

        static bool isParentTrans(const tlm::tlm_generic_payload* trans)
        {
            auto* extension = trans->get_extension<ParentExtension>();
            if (extension != nullptr)
                return !extension->childTranses.empty();
            else
                return false;
        }

        static void setExtension(tlm::tlm_generic_payload* parentTrans, std::vector<tlm::tlm_generic_payload*> childTranses)
        {
            auto* extension = parentTrans->get_extension<ParentExtension>();

            if (extension != nullptr)
            {
                extension->childTranses = std::move(childTranses);
                extension->nextEndReqChildId = 0;
                extension->completedChildTranses = 0;
            }
            else
            {
                extension = new ParentExtension(std::move(childTranses));
                parentTrans->set_auto_extension(extension);
            }
        }

        tlm::tlm_generic_payload* getNextChildTrans()
        {
            if (nextEndReqChildId < childTranses.size())
                return childTranses[nextEndReqChildId++];
            else
                return nullptr;
        }

        bool notifyChildTransCompletion()
        {
            completedChildTranses++;
            return completedChildTranses == childTranses.size();
        }

        void releaseChildTranses()
        {
            std::for_each(childTranses.begin(), childTranses.end(),
                          [](tlm::tlm_generic_payload* childTrans){childTrans->release();});
            childTranses.clear();
        }
    };
};

#endif // LENGTHCONVERTER_H
