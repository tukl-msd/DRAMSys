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

#ifndef TRANSACTION_H
#define TRANSACTION_H
#include "phases/phase.h"
#include "presentation/tracedrawingproperties.h"
#include "timespan.h"
#include <memory>
#include <vector>

typedef unsigned int ID;

class Transaction
{
private:
    std::vector<std::shared_ptr<Phase>> phases;
    traceTime clk;

public:
    const QString command;
    const uint64_t address;
    const unsigned int dataLength, thread, channel;
    const Timespan span;
    const ID id;

    Transaction(ID id,
                QString command,
                unsigned int address,
                unsigned int dataLength,
                unsigned int thread,
                unsigned int channel,
                Timespan span,
                traceTime clk);

    void draw(QPainter* painter,
              const QwtScaleMap& xMap,
              const QwtScaleMap& yMap,
              const QRectF& canvasRect,
              bool highlight,
              const TraceDrawingProperties& drawingProperties) const;
    void addPhase(const std::shared_ptr<Phase>& phase);

    bool isSelected(Timespan timespan,
                    double yVal,
                    const TraceDrawingProperties& drawingproperties) const;

    const std::vector<std::shared_ptr<Phase>>& Phases() const { return phases; }

public:
    static void setNumTransactions(const unsigned int numTransactions)
    {
        mSNumTransactions = numTransactions;
    }
    static unsigned int getNumTransactions()
    {
        return mSNumTransactions;
    }

private:
    static unsigned int mSNumTransactions;
};

#endif // TRANSACTION_H
