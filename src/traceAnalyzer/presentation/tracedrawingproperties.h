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
 *    Derek Christ
 *    Iron Prando da Silva
 */

#ifndef TRACECOLLECTIONDRAWINGPROPERTIES_H
#define TRACECOLLECTIONDRAWINGPROPERTIES_H

#include "businessObjects/traceplotlinemodel.h"
#include "tracedrawing.h"
#include <QAbstractProxyModel>
#include <QColor>
#include <QHash>
#include <QString>
#include <map>

enum class ColorGrouping
{
    PhaseType,
    Transaction,
    Thread,
    RainbowTransaction
};

class TracePlotLineDataSource;

enum class DependencyOption
{
    Disabled,
    Selected,
    All
};

enum class DependencyTextOption
{
    Enabled,
    Disabled
};

struct DependencyOptions
{
    DependencyOption draw;
    DependencyTextOption text;
};

class TraceDrawingProperties : public QObject
{
    Q_OBJECT

public:
    bool drawText;
    bool drawBorder;
    DependencyOptions drawDependenciesOption;
    ColorGrouping colorGrouping;
    QColor textColor;

    unsigned int numberOfRanks = 1;
    unsigned int numberOfBankGroups = 1;
    unsigned int numberOfBanks = 1;
    unsigned int banksPerRank = 1;
    unsigned int groupsPerRank = 1;
    unsigned int banksPerGroup = 1;
    unsigned int per2BankOffset = 0;

    TraceDrawingProperties(bool drawText = true,
                           bool drawBorder = true,
                           DependencyOptions drawDependenciesOption =
                               {DependencyOption::Disabled, DependencyTextOption::Enabled},
                           ColorGrouping colorGrouping = ColorGrouping::PhaseType);

    void init(TracePlotLineDataSource* tracePlotLineDataSource);
    void updateLabels();

    unsigned int getNumberOfDisplayedLines() const;

    const std::vector<std::shared_ptr<TracePlotLineDataSource::TracePlotLine>>&
    getTracePlotLines() const
    {
        return tracePlotLineDataSource->getTracePlotLines();
    }

    std::shared_ptr<QHash<int, QString>> getLabels() const;

Q_SIGNALS:
    void labelsUpdated();

private:
    std::shared_ptr<QHash<int, QString>> labels = std::make_shared<QHash<int, QString>>();

    TracePlotLineDataSource* tracePlotLineDataSource;
};

#endif // TRACECOLLECTIONDRAWINGPROPERTIES_H
