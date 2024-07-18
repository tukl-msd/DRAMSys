/*
 * Copyright (c) 2015, RPTU Kaiserslautern-Landau
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
 *    Janik Schlemminger
 *    Robert Gernhardt
 *    Matthias Jung
 *    Iron Prando da Silva
 */

#include "transaction.h"

#include <utility>

using namespace std;

unsigned int Transaction::mSNumTransactions = 0;

Transaction::Transaction(ID id,
                         QString command,
                         unsigned int address,
                         unsigned int dataLength,
                         unsigned int thread,
                         unsigned int channel,
                         Timespan span,
                         traceTime clk) :
    clk(clk),
    command(std::move(command)),
    address(address),
    dataLength(dataLength),
    thread(thread),
    channel(channel),
    span(span),
    id(id)
{
}

void Transaction::addPhase(const shared_ptr<Phase>& phase)
{
    phases.push_back(phase);
}

void Transaction::draw(QPainter* painter,
                       const QwtScaleMap& xMap,
                       const QwtScaleMap& yMap,
                       const QRectF& canvasRect,
                       bool highlight,
                       const TraceDrawingProperties& drawingProperties) const
{
    for (const shared_ptr<Phase>& phase : phases)
        phase->draw(painter, xMap, yMap, canvasRect, highlight, drawingProperties);
}

bool Transaction::isSelected(Timespan timespan,
                             double yVal,
                             const TraceDrawingProperties& drawingproperties) const
{
    if (span.overlaps(timespan))
    {
        for (const shared_ptr<Phase>& phase : phases)
        {
            if (phase->isSelected(timespan, yVal, drawingproperties))
                return true;
        }
    }

    return false;
}
