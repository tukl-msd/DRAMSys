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

#include "phase.h"
#include "businessObjects/traceplotlinemodel.h"
#include "businessObjects/transaction.h"
#include "presentation/tracedrawing.h"

#include <cmath>

void Phase::draw(QPainter* painter,
                 const QwtScaleMap& xMap,
                 const QwtScaleMap& yMap,
                 const QRectF& canvasRect,
                 bool highlight,
                 const TraceDrawingProperties& drawingProperties) const
{
    Q_UNUSED(canvasRect);

    QColor color = getColor(drawingProperties);
    painter->setBrush(QBrush(getColor(drawingProperties), getBrushStyle()));

    if (!drawingProperties.drawBorder)
    {
        painter->setPen(color);
    }
    else
    {
        painter->setPen(Qt::black);
    }

    if (highlight)
    {
        QPen pen(Qt::red);
        pen.setWidth(3);
        painter->setPen(pen);
    }

    for (auto yVal : getYVals(drawingProperties))
    {
        drawPhaseSymbol(span.Begin(),
                        span.End(),
                        yVal,
                        drawingProperties.drawText,
                        getPhaseSymbol(),
                        painter,
                        xMap,
                        yMap,
                        drawingProperties.textColor);

        DependencyOptions drawDependenciesOptions = drawingProperties.drawDependenciesOption;
        if (drawDependenciesOptions.draw == DependencyOption::All ||
            (drawDependenciesOptions.draw == DependencyOption::Selected && highlight))
        {
            drawPhaseDependencies(
                span.Begin(), span.End(), yVal, drawingProperties, painter, xMap, yMap);
        }
    }

    for (Timespan spanOnCommandBus : spansOnCommandBus)
    {
        for (const auto& line : drawingProperties.getTracePlotLines())
        {
            if (line->data.type == AbstractTracePlotLineModel::RowCommandBusLine)
            {
                if (isColumnCommand())
                    continue;
            }
            else if (line->data.type == AbstractTracePlotLineModel::ColumnCommandBusLine)
            {
                if (!isColumnCommand())
                    continue;
            }
            else if (line->data.type != AbstractTracePlotLineModel::CommandBusLine)
                continue;

            drawPhaseSymbol(spanOnCommandBus.Begin(),
                            spanOnCommandBus.End(),
                            line->data.yVal,
                            false,
                            PhaseSymbol::Hexagon,
                            painter,
                            xMap,
                            yMap,
                            drawingProperties.textColor);
        }
    }

    if (spanOnDataStrobe.End() != 0)
    {
        for (const auto& line : drawingProperties.getTracePlotLines())
        {
            if (line->data.type == AbstractTracePlotLineModel::PseudoChannel0Line)
            {
                if (rank != 0)
                    continue;
            }
            else if (line->data.type == AbstractTracePlotLineModel::PseudoChannel1Line)
            {
                if (rank != 1)
                    continue;
            }
            else if (line->data.type != AbstractTracePlotLineModel::DataBusLine)
                continue;

            drawPhaseSymbol(spanOnDataStrobe.Begin(),
                            spanOnDataStrobe.End(),
                            line->data.yVal,
                            false,
                            PhaseSymbol::Hexagon,
                            painter,
                            xMap,
                            yMap,
                            drawingProperties.textColor);
        }
    }
}

void Phase::drawPhaseSymbol(traceTime begin,
                            traceTime end,
                            double y,
                            bool drawtext,
                            PhaseSymbol symbol,
                            QPainter* painter,
                            const QwtScaleMap& xMap,
                            const QwtScaleMap& yMap,
                            const QColor& textColor) const
{
    double yVal = yMap.transform(y);
    double symbolHeight = yMap.transform(0) - yMap.transform(hexagonHeight);

    // Increase display size of phases with zero span
    traceTime offset = (begin == end) ? static_cast<traceTime>(0.05 * clk) : 0;

    if (symbol == PhaseSymbol::Hexagon)
    {
        QPoint hexFrom(static_cast<int>(xMap.transform(begin)), static_cast<int>(yVal));
        QPoint hexTo(static_cast<int>(xMap.transform(end + offset)), static_cast<int>(yVal));
        drawHexagon(painter, hexFrom, hexTo, symbolHeight);
    }
    else
    {
        QPoint upperLeft(static_cast<int>(xMap.transform(begin)),
                         static_cast<int>(yVal - symbolHeight / 2));
        QPoint bottomRight(static_cast<int>(xMap.transform(end)),
                           static_cast<int>(yVal + symbolHeight / 2));
        painter->drawRect(QRect(upperLeft, bottomRight));
    }

    if (drawtext)
        drawText(painter,
                 Name(),
                 QPoint(static_cast<int>(xMap.transform(begin)),
                        static_cast<int>(yVal + symbolHeight / 2)),
                 TextPositioning::bottomRight,
                 textColor);
}

void Phase::drawPhaseDependencies(traceTime begin,
                                  traceTime end,
                                  double y,
                                  const TraceDrawingProperties& drawingProperties,
                                  QPainter* painter,
                                  const QwtScaleMap& xMap,
                                  const QwtScaleMap& yMap) const
{
    QPen pen;
    pen.setWidth(2);

    painter->save();
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing);

    double yVal = yMap.transform(y);
    double symbolHeight = yMap.transform(0) - yMap.transform(hexagonHeight);

    traceTime offset = (begin == end) ? static_cast<traceTime>(0.05 * clk) : 0;

    size_t invisibleDeps = 0;

    QPoint depLineTo(static_cast<int>(xMap.transform(begin /* + (end + offset - begin)/4*/)),
                     static_cast<int>(yVal));

#ifdef EXTENSION_ENABLED
    for (const auto& dep : mDependencies)
    {
        if (dep->isVisible())
        {
            if (!dep->draw(depLineTo, drawingProperties, painter, xMap, yMap))
            {
                invisibleDeps += 1;
            }
        }
        else
        {
            invisibleDeps += 1;
        }
    }
#endif

    if (invisibleDeps > 0)
    {
        QPoint invisibleDepsPoint(
            static_cast<int>(xMap.transform(begin + (end + offset - begin) / 2)),
            static_cast<int>(yVal + 0.1 * symbolHeight));
        drawText(painter,
                 QString::number(invisibleDeps),
                 invisibleDepsPoint,
                 TextPositioning::centerCenter);
    }

    painter->restore();
}

std::vector<int> Phase::getYVals(const TraceDrawingProperties& drawingProperties) const
{
    std::vector<int> yVals;

    for (const auto& line : drawingProperties.getTracePlotLines())
    {
        if (line->data.type != AbstractTracePlotLineModel::BankLine)
            continue;

        unsigned int yVal = line->data.yVal;

        unsigned int drawnRank = line->data.rank;
        unsigned int drawnBank = line->data.bank;

        bool shouldBeDrawn = false;

        switch (getGranularity())
        {
        case Granularity::Rankwise:
            shouldBeDrawn = (rank == drawnRank);
            break;
        case Granularity::Groupwise:
            shouldBeDrawn = (rank == drawnRank) && (bank % drawingProperties.banksPerGroup ==
                                                    drawnBank % drawingProperties.banksPerGroup);
            break;
        case Granularity::Bankwise:
            shouldBeDrawn = (bank == drawnBank);
            break;

        case Granularity::TwoBankwise:
            shouldBeDrawn =
                (bank == drawnBank) || ((bank + drawingProperties.per2BankOffset) == drawnBank);
            break;
        }

        if (shouldBeDrawn)
            yVals.push_back(yVal);
    }

    return yVals;
}

std::vector<int> REQ::getYVals(const TraceDrawingProperties& drawingProperties) const
{
    std::vector<int> yVals;

    for (const auto& line : drawingProperties.getTracePlotLines())
    {
        if (line->data.type != AbstractTracePlotLineModel::RequestLine)
            continue;

        yVals.push_back(line->data.yVal);
    }

    return yVals;
}

std::vector<int> RESP::getYVals(const TraceDrawingProperties& drawingProperties) const
{
    std::vector<int> yVals;

    for (const auto& line : drawingProperties.getTracePlotLines())
    {
        if (line->data.type != AbstractTracePlotLineModel::ResponseLine)
            continue;

        yVals.push_back(line->data.yVal);
    }

    return yVals;
}

QColor Phase::getColor(const TraceDrawingProperties& drawingProperties) const
{
    switch (drawingProperties.colorGrouping)
    {
    case ColorGrouping::PhaseType:
        return getPhaseColor();
        break;
    case ColorGrouping::Thread:
        return ColorGenerator::getColor(static_cast<unsigned int>(transaction.lock()->thread));
        break;
    case ColorGrouping::RainbowTransaction:
        return ColorGenerator::getRainbowColored(transaction.lock()->id, ColorName::HSV15);

        break;
    case ColorGrouping::Transaction:
    default:
        return ColorGenerator::getColor(transaction.lock()->id);
    }
}

Qt::BrushStyle Phase::getBrushStyle() const
{
    return Qt::SolidPattern;
}

bool Phase::isSelected(Timespan timespan,
                       double yVal,
                       const TraceDrawingProperties& drawingProperties) const
{
    if (span.overlaps(timespan))
    {
        for (auto lineYVal : getYVals(drawingProperties))
            if (fabs(yVal - lineYVal) <= hexagonHeight / 2)
                return true;
    }

    for (Timespan _span : spansOnCommandBus)
    {
        if (_span.overlaps(timespan))
        {
            for (const auto& line : drawingProperties.getTracePlotLines())
            {
                if (line->data.type == AbstractTracePlotLineModel::RowCommandBusLine)
                {
                    if (isColumnCommand())
                        continue;
                }
                else if (line->data.type == AbstractTracePlotLineModel::ColumnCommandBusLine)
                {
                    if (!isColumnCommand())
                        continue;
                }
                else if (line->data.type != AbstractTracePlotLineModel::CommandBusLine)
                    continue;

                if (fabs(yVal - line->data.yVal) <= hexagonHeight / 2)
                    return true;
            }
        }
    }

    if (spanOnDataStrobe.End() != 0 && spanOnDataStrobe.overlaps(timespan))
    {
        for (const auto& line : drawingProperties.getTracePlotLines())
        {
            if (line->data.type == AbstractTracePlotLineModel::PseudoChannel0Line)
            {
                if (rank != 0)
                    continue;
            }
            else if (line->data.type == AbstractTracePlotLineModel::PseudoChannel1Line)
            {
                if (rank != 1)
                    continue;
            }
            else if (line->data.type != AbstractTracePlotLineModel::DataBusLine)
                continue;

            if (fabs(yVal - line->data.yVal) <= hexagonHeight / 2)
                return true;
        }
    }

    return false;
}

bool Phase::isColumnCommand() const
{
    if (dynamic_cast<const RD*>(this) || dynamic_cast<const RDA*>(this) ||
        dynamic_cast<const WR*>(this) || dynamic_cast<const WRA*>(this))
        return true;
    else
        return false;
}

Phase::PhaseSymbol Phase::getPhaseSymbol() const
{
    return PhaseSymbol::Hexagon;
}

#ifdef EXTENSION_ENABLED
void Phase::addDependency(const std::shared_ptr<PhaseDependency>& dependency)
{
    mDependencies.push_back(dependency);
}
#endif
