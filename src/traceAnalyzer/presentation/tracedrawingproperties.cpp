/*
 * Copyright (c) 2021, RPTU Kaiserslautern-Landau
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
 * Author:
 *    Derek Christ
 *    Iron Prando da Silva
 */

#include "tracedrawingproperties.h"
#include "../businessObjects/traceplotlinemodel.h"
#include "util/customlabelscaledraw.h"

TraceDrawingProperties::TraceDrawingProperties(bool drawText,
                                               bool drawBorder,
                                               DependencyOptions drawDependenciesOption,
                                               ColorGrouping colorGrouping) :
    drawText(drawText),
    drawBorder(drawBorder),
    drawDependenciesOption(drawDependenciesOption),
    colorGrouping(colorGrouping)
{
}

void TraceDrawingProperties::init(TracePlotLineDataSource* tracePlotLineDataSource)
{
    this->tracePlotLineDataSource = tracePlotLineDataSource;

    updateLabels();
}

void TraceDrawingProperties::updateLabels()
{
    // Clear hash table, because otherwise not all old values will be overwritten.
    labels->clear();

    // The lowest line starts at the y value of 0.
    int yVal = 0;

    for (auto it = tracePlotLineDataSource->getTracePlotLines().rbegin();
         it != tracePlotLineDataSource->getTracePlotLines().rend();
         ++it)
    {
        auto line = *it;

        labels->operator[](yVal) = line->data.label;
        line->data.yVal = yVal;

        yVal++;
    }

    emit labelsUpdated();
}

std::shared_ptr<QHash<int, QString>> TraceDrawingProperties::getLabels() const
{
    return labels;
}

unsigned int TraceDrawingProperties::getNumberOfDisplayedLines() const
{
    return tracePlotLineDataSource->getTracePlotLines().size();
}
