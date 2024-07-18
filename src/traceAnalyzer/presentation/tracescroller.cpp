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
 */

#include "tracescroller.h"
#include "traceplotitem.h"
#include "util/engineeringScaleDraw.h"
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QWheelEvent>
#include <qwt_plot_zoneitem.h>

TraceScroller::TraceScroller(QWidget* parent) :
    QwtPlot(parent),
    isInitialized(false),
    drawingProperties(false,
                      false,
                      {DependencyOption::Disabled, DependencyTextOption::Disabled},
                      ColorGrouping::PhaseType)
{
    setAxisScaleDraw(xBottom, new EngineeringScaleDraw);
    canvas()->setCursor(Qt::ArrowCursor);
    canvasClip = new QwtPlotZoneItem();
    canvasClip->setZ(2);
    canvasClip->attach(this);
}

void TraceScroller::init(TraceNavigator* navigator,
                         TracePlot* tracePlot,
                         TracePlotLineDataSource* tracePlotLineDataSource)
{
    Q_ASSERT(isInitialized == false);
    isInitialized = true;

    this->tracePlotLineDataSource = tracePlotLineDataSource;

    this->navigator = navigator;
    connectNavigatorQ_SIGNALS();

    const CommentModel* commentModel = navigator->getCommentModel();

    QObject::connect(
        commentModel, &CommentModel::dataChanged, this, &TraceScroller::commentsChanged);

    QObject::connect(
        commentModel, &CommentModel::rowsRemoved, this, &TraceScroller::commentsChanged);

    QObject::connect(commentModel->selectionModel(),
                     &QItemSelectionModel::selectionChanged,
                     this,
                     &TraceScroller::commentsChanged);

    QObject::connect(tracePlotLineDataSource,
                     &TracePlotLineDataSource::modelChanged,
                     this,
                     &TraceScroller::updateAxis);

    setUpDrawingProperties();
    setUpAxis();
    setUpTracePlotItem();

    updateAxis();

    getAndDrawComments();

    this->tracePlot = tracePlot;
    QObject::connect(tracePlot, SIGNAL(tracePlotZoomChanged()), this, SLOT(tracePlotZoomChanged()));
    tracePlotZoomChanged();

    QObject::connect(tracePlot,
                     SIGNAL(colorGroupingChanged(ColorGrouping)),
                     this,
                     SLOT(colorGroupingChanged(ColorGrouping)));

    QObject::connect(tracePlot, SIGNAL(tracePlotLinesChanged()), this, SLOT(updateAxis()));
}

void TraceScroller::setUpTracePlotItem()
{
    TracePlotItem* tracePlotItem = new TracePlotItem(transactions, *navigator, drawingProperties);
    tracePlotItem->setZ(1);
    tracePlotItem->attach(this);
}

void TraceScroller::setUpDrawingProperties()
{
    drawingProperties.init(tracePlotLineDataSource);

    drawingProperties.textColor = palette().text().color();
    drawingProperties.numberOfRanks = navigator->GeneralTraceInfo().numberOfRanks;
    drawingProperties.numberOfBankGroups = navigator->GeneralTraceInfo().numberOfBankGroups;
    drawingProperties.numberOfBanks = navigator->GeneralTraceInfo().numberOfBanks;
    drawingProperties.banksPerRank =
        drawingProperties.numberOfBanks / drawingProperties.numberOfRanks;
    drawingProperties.groupsPerRank =
        drawingProperties.numberOfBankGroups / drawingProperties.numberOfRanks;
    drawingProperties.banksPerGroup =
        drawingProperties.numberOfBanks / drawingProperties.numberOfBankGroups;
    drawingProperties.per2BankOffset = navigator->GeneralTraceInfo().per2BankOffset;
}

void TraceScroller::setUpAxis()
{
    axisScaleDraw(yLeft)->enableComponent(QwtAbstractScaleDraw::Labels, false);
    axisScaleDraw(yLeft)->enableComponent(QwtAbstractScaleDraw::Ticks, false);
}

void TraceScroller::updateAxis()
{
    setAxisScale(yLeft, -1, drawingProperties.getNumberOfDisplayedLines(), 1.0);
    replot();
}

void TraceScroller::connectNavigatorQ_SIGNALS()
{
    QObject::connect(
        navigator, SIGNAL(currentTraceTimeChanged()), this, SLOT(currentTraceTimeChanged()));

    QObject::connect(navigator,
                     SIGNAL(selectedTransactionsChanged()),
                     this,
                     SLOT(selectedTransactionsChanged()));
}

Timespan TraceScroller::GetCurrentTimespan()
{
    traceTime deltaOnTracePlot = navigator->GeneralTraceInfo().span.End() - tracePlot->ZoomLevel();
    traceTime deltaOnTraceScroller = navigator->GeneralTraceInfo().span.End() - zoomLevel;

    traceTime newBegin = static_cast<traceTime>(tracePlot->GetCurrentTimespan().Begin() *
                                                (1.0 * deltaOnTraceScroller) / deltaOnTracePlot);
    Timespan span(newBegin, newBegin + zoomLevel);

    if (span.Begin() < 0)
        span.shift(-span.Begin());
    else if (span.End() > navigator->GeneralTraceInfo().span.End())
        span.shift(navigator->GeneralTraceInfo().span.End() - span.End());
    return span;
}

void TraceScroller::getAndDrawComments()
{
    const CommentModel* commentModel = navigator->getCommentModel();
    QList<QModelIndex> selectedRows = commentModel->selectionModel()->selectedRows();

    for (int row = 0; row < commentModel->rowCount(); row++)
    {
        QModelIndex timeIndex =
            commentModel->index(row, static_cast<int>(CommentModel::Column::Time));

        bool selected =
            std::find(selectedRows.begin(), selectedRows.end(), commentModel->index(row, 0)) !=
            selectedRows.end();

        QwtPlotMarker* maker = new QwtPlotMarker();
        maker->setXValue(static_cast<double>(timeIndex.data(Qt::UserRole).toLongLong()));
        maker->setLineStyle(QwtPlotMarker::LineStyle::VLine);
        maker->setLinePen(QColor(selected ? Qt::red : Qt::blue), 2);
        maker->attach(this);
    }
}

/* Q_SLOTS
 *
 *
 */

void TraceScroller::selectedTransactionsChanged()
{
    replot();
}

void TraceScroller::colorGroupingChanged(ColorGrouping colorGrouping)
{
    drawingProperties.colorGrouping = colorGrouping;
    replot();
}

void TraceScroller::currentTraceTimeChanged()
{
    bool drawDependencies =
        drawingProperties.drawDependenciesOption.draw != DependencyOption::Disabled;

    Timespan spanOnTracePlot = tracePlot->GetCurrentTimespan();
    canvasClip->setInterval(spanOnTracePlot.Begin(), spanOnTracePlot.End());
    Timespan span = GetCurrentTimespan();
    transactions = navigator->TraceFile().getTransactionsInTimespan(span, drawDependencies);

#ifdef EXTENSION_ENABLED
    if (drawDependencies)
    {
        navigator->TraceFile().updateDependenciesInTimespan(span);
    }
#endif

    setAxisScale(xBottom, span.Begin(), span.End());
    replot();
}

void TraceScroller::commentsChanged()
{
    detachItems(QwtPlotItem::Rtti_PlotMarker);
    getAndDrawComments();
    replot();
}

void TraceScroller::tracePlotZoomChanged()
{
    zoomLevel = tracePlot->ZoomLevel() * tracePlotEnlargementFactor;
    if (zoomLevel > navigator->GeneralTraceInfo().span.timeCovered())
        zoomLevel = navigator->GeneralTraceInfo().span.timeCovered();
}

bool TraceScroller::eventFilter(QObject* object, QEvent* event)
{
    if (object == canvas())
    {
        static bool clipDragged = false;
        static bool leftMousePressed = false;
        static int mouseDownX = 0;
        static traceTime mouseDownTracePlotTime = 0;

        switch (event->type())
        {
        case QEvent::Wheel:
        {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            traceTime offset;
            int speed = 4;
            (wheelEvent->angleDelta().y() > 0) ? offset = -zoomLevel* speed
                                               : offset = zoomLevel * speed;
            navigator->navigateToTime(navigator->CurrentTraceTime() + offset);
            return true;
        }
        case QEvent::MouseButtonDblClick:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            traceTime time = invTransform(xBottom, mouseEvent->x());
            navigator->navigateToTime(time);
            return true;
        }
        case QEvent::MouseButtonPress:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            if (mouseEvent->button() == Qt::LeftButton)
            {
                canvas()->setCursor(Qt::ClosedHandCursor);
                leftMousePressed = true;
                mouseDownTracePlotTime = tracePlot->GetCurrentTimespan().Middle();
                mouseDownX = mouseEvent->x();
                if (tracePlot->GetCurrentTimespan().contains(
                        invTransform(xBottom, mouseEvent->x())))
                    clipDragged = true;
                else
                    clipDragged = false;
                return true;
            }
            else if (mouseEvent->button() == Qt::RightButton)
            {
                navigator->navigateToTime(
                    static_cast<traceTime>(invTransform(xBottom, mouseEvent->x())));
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                clipDragged = false;
                leftMousePressed = false;
                canvas()->setCursor(Qt::ArrowCursor);
                return true;
            }
            break;
        }
        case QEvent::MouseMove:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (leftMousePressed)
            {
                if (clipDragged)
                {
                    double clipWidth =
                        transform(xBottom, tracePlot->ZoomLevel()) - transform(xBottom, 0);

                    if (mouseEvent->x() < clipWidth / 2)
                    {
                        navigator->navigateToTime(0);
                    }
                    else if (mouseEvent->x() > canvas()->width() - clipWidth / 2)
                    {
                        navigator->navigateToTime(navigator->GeneralTraceInfo().span.End());
                    }
                    else
                    {
                        traceTime time = static_cast<traceTime>(
                            (mouseEvent->x() - clipWidth / 2) / (canvas()->width() - clipWidth) *
                            (navigator->GeneralTraceInfo().span.End() - tracePlot->ZoomLevel()));
                        navigator->navigateToTime(time);
                    }
                }
                else
                {
                    traceTime deltaTime =
                        invTransform(xBottom, mouseDownX) - invTransform(xBottom, mouseEvent->x());
                    navigator->navigateToTime(mouseDownTracePlotTime + deltaTime);
                }
                return true;
            }
        }
        default:
            break;
        }
    }
    return false;
}
