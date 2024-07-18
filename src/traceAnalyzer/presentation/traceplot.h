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

#ifndef TRACEPLOT_H
#define TRACEPLOT_H

#include "markerplotitem.h"
#include "queryeditor.h"
#include "tracenavigator.h"
#include "traceplotitem.h"
#include "util/togglecollapsedaction.h"

#include <QAction>
#include <QColor>
#include <QPushButton>
#include <QScrollBar>
#include <QString>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_zoneitem.h>
#include <qwt_scale_draw.h>
#include <vector>

class TracePlotMouseLabel;
class CustomLabelScaleDraw;
class CommentModel;
class TracePlotLineDataSource;

/*  A plot that plots all phases and transactions of a trace file
 *  Y Axis : Bank0,...,Bank7,Trans,CC
 *  X Axis : Time
 *
 */
class TracePlot : public QwtPlot
{
    Q_OBJECT

public:
    TracePlot(QWidget* parent = NULL);
    void init(TraceNavigator* navigator,
              QScrollBar* scrollBar,
              TracePlotLineDataSource* tracePlotLineDataSource,
              CommentModel* commentModel);
    Timespan GetCurrentTimespan();

    traceTime ZoomLevel() const;
    void setZoomLevel(traceTime newZoomLevel);

    CustomLabelScaleDraw* getCustomLabelScaleDraw() const;
    const TraceDrawingProperties& getDrawingProperties() const;

public Q_SLOTS:
    void currentTraceTimeChanged();
    void selectedTransactionsChanged();
    void commentsChanged();
    void verticalScrollbarChanged(int value);
    void updateScrollbar();
    void recreateCollapseButtons();
    void updateToggleCollapsedAction();

Q_SIGNALS:
    void tracePlotZoomChanged();
    void tracePlotLinesChanged();
    void colorGroupingChanged(ColorGrouping colorgrouping);

private Q_SLOTS:
    void on_executeQuery();
    void on_selectNextRefresh();
    void on_selectNextActivate();
    void on_selectNextPrecharge();
    void on_selectNextCommand();
    //     void on_selectNextActb();
    //     void on_selectNextPreb();
    //     void on_selectNextRefb();
    void on_colorGroupingPhase();
    void on_colorGroupingTransaction();
    void on_colorGroupingThread();
    void on_colorGroupingRainbowTransaction();
    void on_goToTransaction();
    void on_goToPhase();
    void on_deselectAll();
    void on_insertComment();
    void on_goToTime();
    void on_exportToPDF();
    void on_toggleCollapsedState();

private:
    bool isInitialized;
    TraceNavigator* navigator;
    TraceDrawingProperties drawingProperties;
    TracePlotItem* tracePlotItem;
    QwtPlotZoneItem* zoomZone;
    std::vector<std::shared_ptr<Transaction>> transactions;
    QueryEditor* queryEditor;
    QMenu* contextMenu;
    QScrollBar* scrollBar;
    CustomLabelScaleDraw* customLabelScaleDraw;

    CommentModel* commentModel;

    TracePlotLineDataSource* tracePlotLineDataSource;

    void setUpTracePlotItem();
    void setUpDrawingProperties();
    void setUpGrid();
    void setUpAxis();
    void setUpZoom();
    void setUpQueryEditor();
    void setUpActions();
    void setUpContextMenu();

    void connectNavigatorQ_SIGNALS();

    void getAndDrawComments();
    ToggleCollapsedAction::CollapsedState getCollapsedState() const;
    void collapseOrExpandAllRanks(ToggleCollapsedAction::CollapseAction collapseAction);

    /* zooming
     *
     */
    traceTime zoomLevel;
    constexpr static int minZoomClks = 10;
    constexpr static int maxZoomClks = 2000;
    constexpr static int GridVisiblityClks = 100;
    constexpr static int textVisibilityClks = 200;
    traceTime minZoomLevel, maxZoomLevel, textVisibilityZoomLevel;

    constexpr static double zoomFactor = 0.8;
    void zoomIn(traceTime zoomCenter);
    void zoomOut(traceTime zoomCenter);
    void exitZoomMode();
    void enterZoomMode();

    /* keyboard an mouse events
     *
     */
    bool eventFilter(QObject* object, QEvent* event);

    QAction* goToTime;
    QAction* goToTransaction;
    QAction* goToPhase;
    QAction* showQueryEditor;
    QAction* insertComment;
    QAction* deselectAll;
    QAction* selectNextRefresh;
    QAction* selectNextActivate;
    QAction* selectNextPrecharge;
    QAction* selectNextCommand;
    //     QAction *selectNextActb;
    //     QAction *selectNextPreb;
    //     QAction *selectNextRefb;
    QAction* setColorGroupingPhase;
    QAction* setColorGroupingTransaction;
    QAction* setColorGroupingThread;
    QAction* setColorGroupingRainbowTransaction;
    QAction* exportToPdf;
    ToggleCollapsedAction* toggleCollapsedState;

    QMenu* dependenciesSubMenu;
    QAction* disabledDependencies;
    QAction* selectedDependencies;
    QAction* allDependencies;
    QAction* switchDrawDependencyTextsOption;

    TracePlotMouseLabel* mouseLabel;

    void openContextMenu(const QPoint& pos, const QPoint& mouseDown);
    QPoint contextMenuMouseDown;
    void SelectTransaction(int x, int y) const;
    void SelectComment(int x) const;
    // const std::vector<std::shared_ptr<Comment>> hoveredComments(traceTime time) const;
    Timespan hoveredTimespan(int x) const;
    void keyPressEvent(QKeyEvent* keyPressedEvent);
    void keyReleaseEvent(QKeyEvent* keyReleasedEvent);

    struct KeyPressData
    {
        bool ctrlPressed, shiftPressed;
        KeyPressData() : ctrlPressed(false), shiftPressed(false) {}
    };

    struct MouseDownData
    {
        traceTime mouseDownTime;
        Timespan zoomSpan;
        int mouseDownX;
        bool mouseIsDownForDragging;
        bool mouseIsDownForZooming;
        MouseDownData() :
            mouseDownTime(0),
            mouseDownX(0),
            mouseIsDownForDragging(false),
            mouseIsDownForZooming(false)
        {
        }
    };

    MouseDownData mouseDownData;
    KeyPressData keyPressData;
};

#endif // TRACEPLOT_H
