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

#ifndef BANKPHASE_H
#define BANKPHASE_H

#ifdef EXTENSION_ENABLED
#include "businessObjects/phases/phasedependency.h"
#endif

#include "businessObjects/timespan.h"
#include "presentation/tracedrawingproperties.h"
#include "presentation/util/colorgenerator.h"
#include <QPainter>
#include <QString>
#include <memory>
#include <qwt_scale_map.h>
#include <utility>
#include <vector>

typedef unsigned int ID;
// enum TextPositioning;
class Transaction;

enum class RelevantAttributes
{
    Rank = 0x01,
    BankGroup = 0x02,
    Bank = 0x04,
    Row = 0x08,
    Column = 0x10,
    BurstLength = 0x20
};

inline RelevantAttributes operator|(RelevantAttributes a, RelevantAttributes b)
{
    return static_cast<RelevantAttributes>(static_cast<int>(a) | static_cast<int>(b));
}

inline RelevantAttributes operator&(RelevantAttributes a, RelevantAttributes b)
{
    return static_cast<RelevantAttributes>(static_cast<int>(a) & static_cast<int>(b));
}

class Phase
{
public:
    Phase(ID id,
          Timespan span,
          Timespan spanOnDataStrobe,
          unsigned int rank,
          unsigned int bankGroup,
          unsigned int bank,
          unsigned int row,
          unsigned int column,
          unsigned int burstLength,
          traceTime clk,
          const std::shared_ptr<Transaction>& transaction,
          std::vector<Timespan> spansOnCommandBus,
          unsigned int groupsPerRank,
          unsigned int banksPerGroup) :
        id(id),
        span(span),
        spanOnDataStrobe(spanOnDataStrobe),
        rank(rank),
        bankGroup(bankGroup),
        bank(bank),
        row(row),
        column(column),
        burstLength(burstLength),
        groupsPerRank(groupsPerRank),
        banksPerGroup(banksPerGroup),
        clk(clk),
        transaction(transaction),
        spansOnCommandBus(std::move(spansOnCommandBus)),
        hexagonHeight(0.6),
        captionPosition(TextPositioning::bottomRight)
    {
    }

    void draw(QPainter* painter,
              const QwtScaleMap& xMap,
              const QwtScaleMap& yMap,
              const QRectF& canvasRect,
              bool highlight,
              const TraceDrawingProperties& drawingProperties) const;
    bool isSelected(Timespan timespan,
                    double yVal,
                    const TraceDrawingProperties& drawingproperties) const;
    bool isColumnCommand() const;

    const Timespan& Span() const { return span; }

    ID Id() const { return id; }

    unsigned int getRank() const { return rank; }

    unsigned int getBankGroup() const { return bankGroup % groupsPerRank; }

    unsigned int getBank() const { return bank % banksPerGroup; }

    unsigned int getRow() const { return row; }

    unsigned int getColumn() const { return column; }

    unsigned int getBurstLength() const { return burstLength; }

    virtual RelevantAttributes getRelevantAttributes() const = 0;

    virtual QString Name() const = 0;

#ifdef EXTENSION_ENABLED
    void addDependency(const std::shared_ptr<PhaseDependency>& dependency);
#endif

protected:
    ID id;
    Timespan span;
    Timespan spanOnDataStrobe;
    unsigned int rank, bankGroup, bank, row, column, burstLength;
    unsigned int groupsPerRank, banksPerGroup;
    traceTime clk;
    std::weak_ptr<Transaction> transaction;
    std::vector<Timespan> spansOnCommandBus;
#ifdef EXTENSION_ENABLED
    std::vector<std::shared_ptr<PhaseDependency>> mDependencies;
#endif

    double hexagonHeight;
    TextPositioning captionPosition;

    enum PhaseSymbol
    {
        Hexagon,
        Rect
    };
    virtual PhaseSymbol getPhaseSymbol() const;
    virtual Qt::BrushStyle getBrushStyle() const;
    virtual QColor getColor(const TraceDrawingProperties& drawingProperties) const;

    virtual QColor getPhaseColor() const = 0;
    virtual std::vector<int> getYVals(const TraceDrawingProperties& drawingProperties) const;
    virtual void drawPhaseSymbol(traceTime begin,
                                 traceTime end,
                                 double y,
                                 bool drawtext,
                                 PhaseSymbol symbol,
                                 QPainter* painter,
                                 const QwtScaleMap& xMap,
                                 const QwtScaleMap& yMap,
                                 const QColor& textColor) const;
    virtual void drawPhaseDependencies(traceTime begin,
                                       traceTime end,
                                       double y,
                                       const TraceDrawingProperties& drawingProperties,
                                       QPainter* painter,
                                       const QwtScaleMap& xMap,
                                       const QwtScaleMap& yMap) const;

    enum class Granularity
    {
        Bankwise,
        TwoBankwise,
        Groupwise,
        Rankwise
    };

    virtual Granularity getGranularity() const { return Granularity::Bankwise; }

    friend class PhaseDependency;
};

class REQ final : public Phase
{
public:
    using Phase::Phase;

protected:
    QColor getPhaseColor() const override { return ColorGenerator::getColor(1); }
    QString Name() const final { return "REQ"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return static_cast<RelevantAttributes>(0);
    }

    std::vector<int> getYVals(const TraceDrawingProperties& drawingProperties) const override;
};

class RESP final : public Phase
{
public:
    using Phase::Phase;

protected:
    QColor getPhaseColor() const override { return ColorGenerator::getColor(1); }
    QString Name() const override { return "RESP"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return static_cast<RelevantAttributes>(0);
    }

    std::vector<int> getYVals(const TraceDrawingProperties& drawingProperties) const override;
};

class PREPB final : public Phase
{
public:
    using Phase::Phase;

protected:
    QColor getPhaseColor() const override { return ColorGenerator::getColor(1); }
    QString Name() const override { return "PREPB"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank;
    }
};

class PRESB final : public Phase
{
public:
    using Phase::Phase;

protected:
    QString Name() const override { return "PRESB"; }
    virtual std::vector<traceTime> getTimesOnCommandBus() const { return {span.Begin()}; }
    QColor getColor(const TraceDrawingProperties& drawingProperties) const override
    {
        Q_UNUSED(drawingProperties)
        return getPhaseColor();
    }
    QColor getPhaseColor() const override { return ColorGenerator::getColor(1); }
    Granularity getGranularity() const override { return Granularity::Groupwise; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::Bank;
    }
};

class PREAB final : public Phase
{
public:
    using Phase::Phase;

protected:
    QString Name() const override { return "PREAB"; }
    virtual std::vector<traceTime> getTimesOnCommandBus() const { return {span.Begin()}; }
    QColor getColor(const TraceDrawingProperties& drawingProperties) const override
    {
        Q_UNUSED(drawingProperties)
        return getPhaseColor();
    }
    QColor getPhaseColor() const override { return ColorGenerator::getColor(10); }
    Granularity getGranularity() const override { return Granularity::Rankwise; }

    RelevantAttributes getRelevantAttributes() const override { return RelevantAttributes::Rank; }
};

class ACT final : public Phase
{
public:
    using Phase::Phase;

protected:
    QColor getPhaseColor() const override { return ColorGenerator::getColor(3); }
    QString Name() const override { return "ACT"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank |
               RelevantAttributes::Row;
    }
};

class RD final : public Phase
{
public:
    using Phase::Phase;

protected:
    QColor getPhaseColor() const override { return ColorGenerator::getColor(4); }
    QString Name() const override { return "RD"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank |
               RelevantAttributes::Column | RelevantAttributes::BurstLength;
    }
};

class RDA final : public Phase
{
public:
    using Phase::Phase;

protected:
    QColor getPhaseColor() const override { return ColorGenerator::getColor(5); }
    QString Name() const override { return "RDA"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank |
               RelevantAttributes::Column | RelevantAttributes::BurstLength;
    }
};

class WR final : public Phase
{
public:
    using Phase::Phase;

protected:
    QColor getPhaseColor() const override { return ColorGenerator::getColor(6); }
    QString Name() const override { return "WR"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank |
               RelevantAttributes::Column | RelevantAttributes::BurstLength;
    }
};

class MWR final : public Phase
{
public:
    using Phase::Phase;

protected:
    QColor getPhaseColor() const override { return ColorGenerator::getColor(6); }
    QString Name() const override { return "MWR"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank |
               RelevantAttributes::Column | RelevantAttributes::BurstLength;
    }
};

class WRA final : public Phase
{
public:
    using Phase::Phase;

protected:
    QColor getPhaseColor() const override { return ColorGenerator::getColor(7); }
    QString Name() const override { return "WRA"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank |
               RelevantAttributes::Column | RelevantAttributes::BurstLength;
    }
};

class MWRA final : public Phase
{
public:
    using Phase::Phase;

protected:
    QColor getPhaseColor() const override { return ColorGenerator::getColor(7); }
    QString Name() const override { return "MWRA"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank |
               RelevantAttributes::Column | RelevantAttributes::BurstLength;
    }
};

class AUTO_REFRESH : public Phase
{
public:
    using Phase::Phase;

protected:
    QString Name() const override { return "REF"; }
    virtual std::vector<traceTime> getTimesOnCommandBus() const { return {span.Begin()}; }
    QColor getColor(const TraceDrawingProperties& drawingProperties) const override
    {
        Q_UNUSED(drawingProperties)
        return getPhaseColor();
    }
    QColor getPhaseColor() const override
    {
        auto phaseColor = QColor(Qt::darkCyan);
        phaseColor.setAlpha(130);
        return phaseColor;
    }
};

class REFAB final : public AUTO_REFRESH
{
public:
    using AUTO_REFRESH::AUTO_REFRESH;

protected:
    QString Name() const override { return "REFAB"; }
    Granularity getGranularity() const override { return Granularity::Rankwise; }

    RelevantAttributes getRelevantAttributes() const override { return RelevantAttributes::Rank; }
};

class RFMAB final : public AUTO_REFRESH
{
public:
    using AUTO_REFRESH::AUTO_REFRESH;

protected:
    QString Name() const override { return "RFMAB"; }
    Granularity getGranularity() const override { return Granularity::Rankwise; }
    QColor getPhaseColor() const override
    {
        auto phaseColor = QColor(Qt::darkRed);
        phaseColor.setAlpha(130);
        return phaseColor;
    }

    RelevantAttributes getRelevantAttributes() const override { return RelevantAttributes::Rank; }
};

class REFPB final : public AUTO_REFRESH
{
public:
    using AUTO_REFRESH::AUTO_REFRESH;

protected:
    QString Name() const override { return "REFPB"; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank;
    }
};

class RFMPB final : public AUTO_REFRESH
{
public:
    using AUTO_REFRESH::AUTO_REFRESH;

protected:
    QString Name() const override { return "RFMPB"; }
    QColor getPhaseColor() const override
    {
        auto phaseColor = QColor(Qt::darkRed);
        phaseColor.setAlpha(130);
        return phaseColor;
    }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank;
    }
};

class REFP2B final : public AUTO_REFRESH
{
public:
    using AUTO_REFRESH::AUTO_REFRESH;

protected:
    QString Name() const override { return "REFP2B"; }
    Granularity getGranularity() const override { return Granularity::TwoBankwise; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank;
    }
};

class RFMP2B final : public AUTO_REFRESH
{
public:
    using AUTO_REFRESH::AUTO_REFRESH;

protected:
    QString Name() const override { return "RFMP2B"; }
    Granularity getGranularity() const override { return Granularity::TwoBankwise; }
    QColor getPhaseColor() const override
    {
        auto phaseColor = QColor(Qt::darkRed);
        phaseColor.setAlpha(130);
        return phaseColor;
    }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank;
    }
};

class REFSB final : public AUTO_REFRESH
{
public:
    using AUTO_REFRESH::AUTO_REFRESH;

protected:
    QString Name() const override { return "REFSB"; }
    Granularity getGranularity() const override { return Granularity::Groupwise; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::Bank;
    }
};

class RFMSB final : public AUTO_REFRESH
{
public:
    using AUTO_REFRESH::AUTO_REFRESH;

protected:
    QString Name() const override { return "RFMSB"; }
    Granularity getGranularity() const override { return Granularity::Groupwise; }
    QColor getPhaseColor() const override
    {
        auto phaseColor = QColor(Qt::darkRed);
        phaseColor.setAlpha(130);
        return phaseColor;
    }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::Bank;
    }
};

class PDNAB : public Phase
{
public:
    using Phase::Phase;
    virtual ~PDNAB() = default;

protected:
    QString Name() const override { return "PDNAB"; }
    Qt::BrushStyle getBrushStyle() const override { return Qt::Dense6Pattern; }
    QColor getColor(const TraceDrawingProperties& drawingProperties) const override
    {
        Q_UNUSED(drawingProperties)
        return getPhaseColor();
    }
    QColor getPhaseColor() const override { return {Qt::black}; }
    Phase::PhaseSymbol getPhaseSymbol() const override { return PhaseSymbol::Rect; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank;
    }
};

class PDNA final : public PDNAB
{
public:
    using PDNAB::PDNAB;

protected:
    QString Name() const override { return "PDNA"; }
    Granularity getGranularity() const override { return Granularity::Rankwise; }

    RelevantAttributes getRelevantAttributes() const override { return RelevantAttributes::Rank; }
};

class PDNPB : public Phase
{
public:
    using Phase::Phase;
    virtual ~PDNPB() = default;

protected:
    QString Name() const override { return "PDNPB"; }
    Qt::BrushStyle getBrushStyle() const override { return Qt::Dense4Pattern; }
    QColor getColor(const TraceDrawingProperties& drawingProperties) const override
    {
        Q_UNUSED(drawingProperties)
        return getPhaseColor();
    }
    QColor getPhaseColor() const override { return {Qt::black}; }
    Phase::PhaseSymbol getPhaseSymbol() const override { return PhaseSymbol::Rect; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank;
    }
};

class PDNP final : public PDNPB
{
public:
    using PDNPB::PDNPB;

protected:
    QString Name() const override { return "PDNP"; }
    Granularity getGranularity() const override { return Granularity::Rankwise; }
    RelevantAttributes getRelevantAttributes() const override { return RelevantAttributes::Rank; }
};

class SREFB : public Phase
{
public:
    using Phase::Phase;
    virtual ~SREFB() = default;

protected:
    QString Name() const override { return "SREFB"; }
    Qt::BrushStyle getBrushStyle() const override { return Qt::Dense1Pattern; }
    QColor getColor(const TraceDrawingProperties& drawingProperties) const override
    {
        Q_UNUSED(drawingProperties)
        return getPhaseColor();
    }
    QColor getPhaseColor() const override { return {Qt::black}; }
    Phase::PhaseSymbol getPhaseSymbol() const override { return PhaseSymbol::Rect; }

    RelevantAttributes getRelevantAttributes() const override
    {
        return RelevantAttributes::Rank | RelevantAttributes::BankGroup | RelevantAttributes::Bank;
    }
};

class SREF : public SREFB
{
public:
    using SREFB::SREFB;

protected:
    QString Name() const override { return "SREF"; }
    Granularity getGranularity() const override { return Granularity::Rankwise; }
    RelevantAttributes getRelevantAttributes() const override { return RelevantAttributes::Rank; }
};

#endif // BANKPHASE_H
