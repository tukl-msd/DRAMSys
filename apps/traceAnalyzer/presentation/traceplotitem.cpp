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
 */

#include "traceplotitem.h"
#include "tracedrawing.h"
#include "util/colorgenerator.h"
#include <QRect>
#include <algorithm>
#include <qwt_painter.h>
#include <qwt_scale_map.h>
#include <tuple>

using namespace std;
int TracePlotItem::rtti() const
{
    return QwtPlotItem::Rtti_PlotUserItem;
}

void TracePlotItem::draw(QPainter* painter,
                         const QwtScaleMap& xMap,
                         const QwtScaleMap& yMap,
                         const QRectF& canvasRect) const
{
    for (const auto& transaction : transactions)
    {
        bool highlight = navigator.transactionIsSelected(transaction);
        transaction->draw(painter, xMap, yMap, canvasRect, highlight, drawingProperties);
    }
}

vector<shared_ptr<Transaction>> TracePlotItem::getSelectedTransactions(Timespan timespan,
                                                                       double yVal)
{
    vector<shared_ptr<Transaction>> result;

    for (const auto& transaction : transactions)
    {
        if (transaction->isSelected(timespan, yVal, drawingProperties))
            result.push_back(transaction);
    }

    return result;
}
