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

#ifndef TRACESCROLLER_H
#define TRACESCROLLER_H

#include "presentation/tracenavigator.h"
#include "traceplot.h"
#include <qwt_plot.h>
#include <qwt_plot_zoneitem.h>
#include <vector>

class TracePlotLineDataSource;

class TraceScroller : public QwtPlot
{
    Q_OBJECT
private:
    std::vector<std::shared_ptr<Transaction>> transactions;
    bool isInitialized;
    TraceNavigator* navigator;
    TracePlot* tracePlot;
    TracePlotLineDataSource* tracePlotLineDataSource;
    constexpr static int tracePlotEnlargementFactor = 4;
    void setUpTracePlotItem();
    void setUpDrawingProperties();
    void setUpAxis();
    void connectNavigatorQ_SIGNALS();

    void getAndDrawComments();
    QwtPlotZoneItem* canvasClip;
    traceTime zoomLevel;
    bool eventFilter(QObject* object, QEvent* event);
    TraceDrawingProperties drawingProperties;

public:
    TraceScroller(QWidget* parent = NULL);
    void init(TraceNavigator* navigator,
              TracePlot* tracePlot,
              TracePlotLineDataSource* tracePlotLineDataSource);
    Timespan GetCurrentTimespan();

public Q_SLOTS:
    void currentTraceTimeChanged();
    void commentsChanged();
    void tracePlotZoomChanged();
    void selectedTransactionsChanged();
    void colorGroupingChanged(ColorGrouping colorgrouping);

private Q_SLOTS:
    void updateAxis();
};

#endif // TraceScroller_H
