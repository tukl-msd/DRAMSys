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

#include "traceplot.h"
#include "businessObjects/commentmodel.h"
#include "businessObjects/traceplotlinemodel.h"
#include "gototimedialog.h"
#include "tracePlotMouseLabel.h"
#include "tracedrawing.h"
#include "util/clkgrid.h"
#include "util/customlabelscaledraw.h"
#include "util/engineeringScaleDraw.h"

#include <QFileInfo>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QWheelEvent>
#include <cmath>
#include <iostream>
#include <qfiledialog.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_renderer.h>

TracePlot::TracePlot(QWidget* parent) :
    QwtPlot(parent),
    isInitialized(false),
    customLabelScaleDraw(new CustomLabelScaleDraw(drawingProperties.getLabels()))
{
    canvas()->setCursor(Qt::ArrowCursor);
    setUpActions();
}

void TracePlot::init(TraceNavigator* navigator,
                     QScrollBar* scrollBar,
                     TracePlotLineDataSource* tracePlotLineDataSource,
                     CommentModel* commentModel)
{
    Q_ASSERT(isInitialized == false);
    isInitialized = true;

    this->navigator = navigator;
    this->scrollBar = scrollBar;
    this->commentModel = commentModel;
    this->tracePlotLineDataSource = tracePlotLineDataSource;

    installEventFilter(commentModel);

    QObject::connect(commentModel, &CommentModel::dataChanged, this, &TracePlot::commentsChanged);

    QObject::connect(commentModel, &CommentModel::rowsRemoved, this, &TracePlot::commentsChanged);

    QObject::connect(commentModel->selectionModel(),
                     &QItemSelectionModel::selectionChanged,
                     this,
                     &TracePlot::commentsChanged);

    auto selectedTracePlotLineModel = tracePlotLineDataSource->getSelectedModel();
    QObject::connect(selectedTracePlotLineModel,
                     &QAbstractItemModel::rowsInserted,
                     this,
                     &TracePlot::recreateCollapseButtons);
    QObject::connect(selectedTracePlotLineModel,
                     &QAbstractItemModel::rowsRemoved,
                     this,
                     &TracePlot::recreateCollapseButtons);

    connectNavigatorQ_SIGNALS();
    setUpDrawingProperties();
    setUpAxis();
    setUpGrid();
    setUpTracePlotItem();
    setUpZoom();
    setUpQueryEditor();
    mouseLabel = new TracePlotMouseLabel(
        this, navigator->GeneralTraceInfo().clkPeriod, this->mouseDownData.zoomSpan);
    getAndDrawComments();
    setZoomLevel(1000);

    updateScrollbar();
    recreateCollapseButtons();
    dependenciesSubMenu->setEnabled(navigator->TraceFile().checkDependencyTableExists());
    replot();
}

void TracePlot::setUpActions()
{
    insertComment = new QAction("Insert comment", this);
    QObject::connect(insertComment, SIGNAL(triggered()), this, SLOT(on_insertComment()));

    goToTime = new QAction("Go to time", this);
    QObject::connect(goToTime, SIGNAL(triggered()), this, SLOT(on_goToTime()));

    goToTransaction = new QAction("Go to transaction", this);
    QObject::connect(goToTransaction, SIGNAL(triggered()), this, SLOT(on_goToTransaction()));

    deselectAll = new QAction("Deselect all", this);
    QObject::connect(deselectAll, SIGNAL(triggered()), this, SLOT(on_deselectAll()));

    goToPhase = new QAction("Go to phase", this);
    QObject::connect(goToPhase, SIGNAL(triggered()), this, SLOT(on_goToPhase()));

    showQueryEditor = new QAction("Execute query", this);
    showQueryEditor->setShortcut(QKeySequence("ctrl+e"));
    addAction(showQueryEditor);
    QObject::connect(showQueryEditor, SIGNAL(triggered()), this, SLOT(on_executeQuery()));

    selectNextRefresh = new QAction("Select next refresh", this);
    addAction(selectNextRefresh);
    QObject::connect(selectNextRefresh, SIGNAL(triggered()), this, SLOT(on_selectNextRefresh()));

    selectNextActivate = new QAction("Select next activate", this);
    addAction(selectNextActivate);
    QObject::connect(selectNextActivate, SIGNAL(triggered()), this, SLOT(on_selectNextActivate()));

    selectNextPrecharge = new QAction("Select next precharge", this);
    addAction(selectNextPrecharge);
    QObject::connect(
        selectNextPrecharge, SIGNAL(triggered()), this, SLOT(on_selectNextPrecharge()));

    selectNextCommand = new QAction("Select next command", this);
    addAction(selectNextCommand);
    QObject::connect(selectNextCommand, SIGNAL(triggered()), this, SLOT(on_selectNextCommand()));

    //     selectNextActb = new QAction("Select next atcb", this);
    //     selectNextActb->setShortcut(QKeySequence("alt+b"));
    //     addAction(selectNextActb);
    //     QObject::connect(selectNextActb, SIGNAL(triggered()), this,
    //                      SLOT(on_selectNextActb()));
    //
    //     selectNextPreb = new QAction("Select next preb", this);
    //     selectNextPreb->setShortcut(QKeySequence("alt+q"));
    //     addAction(selectNextPreb);
    //     QObject::connect(selectNextPreb, SIGNAL(triggered()), this,
    //                      SLOT(on_selectNextPreb()));
    //
    //     selectNextRefb = new QAction("Select next refb", this);
    //     selectNextRefb->setShortcut(QKeySequence("alt+s"));
    //     addAction(selectNextRefb);
    //     QObject::connect(selectNextRefb, SIGNAL(triggered()), this,
    //                      SLOT(on_selectNextRefb()));

    setColorGroupingPhase = new QAction("Group by Phase", this);
    setColorGroupingPhase->setCheckable(true);
    setColorGroupingPhase->setChecked(true);
    addAction(setColorGroupingPhase);
    QObject::connect(
        setColorGroupingPhase, SIGNAL(triggered()), this, SLOT(on_colorGroupingPhase()));

    setColorGroupingTransaction = new QAction("Group by Transaction", this);
    setColorGroupingTransaction->setCheckable(true);
    addAction(setColorGroupingTransaction);
    QObject::connect(setColorGroupingTransaction,
                     SIGNAL(triggered()),
                     this,
                     SLOT(on_colorGroupingTransaction()));

    setColorGroupingRainbowTransaction =
        new QAction("Group by Transaction - Rainbow Colored", this);
    setColorGroupingRainbowTransaction->setCheckable(true);
    addAction(setColorGroupingRainbowTransaction);
    QObject::connect(setColorGroupingRainbowTransaction,
                     SIGNAL(triggered()),
                     this,
                     SLOT(on_colorGroupingRainbowTransaction()));

    setColorGroupingThread = new QAction("Group by Thread", this);
    setColorGroupingThread->setCheckable(true);
    addAction(setColorGroupingThread);
    QObject::connect(
        setColorGroupingThread, SIGNAL(triggered()), this, SLOT(on_colorGroupingThread()));

    QActionGroup* colorGroupingGroup = new QActionGroup(this);
    colorGroupingGroup->addAction(setColorGroupingPhase);
    colorGroupingGroup->addAction(setColorGroupingTransaction);
    colorGroupingGroup->addAction(setColorGroupingRainbowTransaction);
    colorGroupingGroup->addAction(setColorGroupingThread);

    exportToPdf = new QAction("Export to SVG", this);
    addAction(exportToPdf);
    QObject::connect(exportToPdf, SIGNAL(triggered()), this, SLOT(on_exportToPDF()));

    toggleCollapsedState = new ToggleCollapsedAction(this);
    addAction(toggleCollapsedState);
    QObject::connect(
        toggleCollapsedState, SIGNAL(triggered()), this, SLOT(on_toggleCollapsedState()));
    toggleCollapsedState->setShortcut(Qt::CTRL + Qt::Key_X);

    disabledDependencies = new QAction("Disabled", this);
    selectedDependencies = new QAction("Selected transactions", this);
    allDependencies = new QAction("All transactions", this);
    switchDrawDependencyTextsOption = new QAction("Draw Texts", this);

    disabledDependencies->setCheckable(true);
    selectedDependencies->setCheckable(true);
    allDependencies->setCheckable(true);
    switchDrawDependencyTextsOption->setCheckable(true);

    switchDrawDependencyTextsOption->setChecked(true);
    disabledDependencies->setChecked(true);

    QObject::connect(disabledDependencies,
                     &QAction::triggered,
                     this,
                     [&]()
                     {
                         drawingProperties.drawDependenciesOption.draw = DependencyOption::Disabled;
                         currentTraceTimeChanged();
                     });
    QObject::connect(selectedDependencies,
                     &QAction::triggered,
                     this,
                     [&]()
                     {
                         drawingProperties.drawDependenciesOption.draw = DependencyOption::Selected;
                         currentTraceTimeChanged();
                     });
    QObject::connect(allDependencies,
                     &QAction::triggered,
                     this,
                     [&]()
                     {
                         drawingProperties.drawDependenciesOption.draw = DependencyOption::All;
                         currentTraceTimeChanged();
                     });
    QObject::connect(
        switchDrawDependencyTextsOption,
        &QAction::triggered,
        this,
        [&]()
        {
            if (drawingProperties.drawDependenciesOption.text == DependencyTextOption::Disabled)
            {
                drawingProperties.drawDependenciesOption.text = DependencyTextOption::Enabled;
                switchDrawDependencyTextsOption->setChecked(true);
            }
            else
            {
                drawingProperties.drawDependenciesOption.text = DependencyTextOption::Disabled;
                switchDrawDependencyTextsOption->setChecked(false);
            }
            currentTraceTimeChanged();
        });

    QActionGroup* dependenciesGroup = new QActionGroup(this);
    dependenciesGroup->addAction(disabledDependencies);
    dependenciesGroup->addAction(selectedDependencies);
    dependenciesGroup->addAction(allDependencies);
    dependenciesGroup->addAction(switchDrawDependencyTextsOption);

#ifndef EXTENSION_ENABLED
    dependenciesGroup->setEnabled(false);
#endif

    setUpContextMenu();
}

void TracePlot::setUpContextMenu()
{
    contextMenu = new QMenu(this);
    contextMenu->addActions({deselectAll});

    QMenu* colorGroupingSubMenu = new QMenu("Group by", contextMenu);
    colorGroupingSubMenu->addActions({setColorGroupingPhase,
                                      setColorGroupingTransaction,
                                      setColorGroupingRainbowTransaction,
                                      setColorGroupingThread});
    contextMenu->addMenu(colorGroupingSubMenu);

    dependenciesSubMenu = new QMenu("Show dependencies", contextMenu);
    dependenciesSubMenu->addActions({disabledDependencies,
                                     selectedDependencies,
                                     allDependencies,
                                     switchDrawDependencyTextsOption});
    contextMenu->addMenu(dependenciesSubMenu);

    QMenu* goToSubMenu = new QMenu("Go to", contextMenu);
    goToSubMenu->addActions({goToPhase, goToTransaction, goToTime});
    contextMenu->addMenu(goToSubMenu);

    QMenu* selectSubMenu = new QMenu("Select", contextMenu);
    selectSubMenu->addActions(
        {selectNextRefresh,
         selectNextActivate,
         selectNextPrecharge,
         selectNextCommand /*, selectNextActb, selectNextPreb, selectNextRefb */});
    contextMenu->addMenu(selectSubMenu);

    contextMenu->addActions({showQueryEditor, insertComment, exportToPdf, toggleCollapsedState});
}

void TracePlot::connectNavigatorQ_SIGNALS()
{
    QObject::connect(
        navigator, SIGNAL(currentTraceTimeChanged()), this, SLOT(currentTraceTimeChanged()));
    QObject::connect(navigator,
                     SIGNAL(selectedTransactionsChanged()),
                     this,
                     SLOT(selectedTransactionsChanged()));
}

void TracePlot::setUpDrawingProperties()
{
    connect(this,
            &TracePlot::tracePlotLinesChanged,
            &drawingProperties,
            &TraceDrawingProperties::updateLabels);
    connect(&drawingProperties,
            &TraceDrawingProperties::labelsUpdated,
            this,
            &TracePlot::updateScrollbar);

    drawingProperties.init(tracePlotLineDataSource);

    drawingProperties.textColor = palette().text().color();
    drawingProperties.numberOfRanks = navigator->GeneralTraceInfo().numberOfRanks;
    drawingProperties.numberOfBankGroups = navigator->GeneralTraceInfo().numberOfBankGroups;
    drawingProperties.numberOfBanks = navigator->GeneralTraceInfo().numberOfBanks;
    drawingProperties.banksPerRank = navigator->GeneralTraceInfo().banksPerRank;
    drawingProperties.groupsPerRank = navigator->GeneralTraceInfo().groupsPerRank;
    drawingProperties.banksPerGroup = navigator->GeneralTraceInfo().banksPerGroup;
    drawingProperties.per2BankOffset = navigator->GeneralTraceInfo().per2BankOffset;
}

void TracePlot::setUpQueryEditor()
{
    queryEditor = new QueryEditor(this);
    queryEditor->setWindowFlags(Qt::Window);
    queryEditor->setWindowTitle("Query " +
                                QFileInfo(navigator->TraceFile().getPathToDB()).baseName());
    queryEditor->init(navigator);
}

void TracePlot::setUpTracePlotItem()
{
    tracePlotItem = new TracePlotItem(transactions, *navigator, drawingProperties);
    tracePlotItem->setZ(1);
    tracePlotItem->attach(this);
}

void TracePlot::setUpGrid()
{
    unsigned int clk = navigator->GeneralTraceInfo().clkPeriod;
    QwtPlotGrid* grid = new ClkGrid(clk, GridVisiblityClks * clk);
    grid->setZ(0);
    grid->attach(this);
}

void TracePlot::setUpZoom()
{
    minZoomLevel = minZoomClks * navigator->GeneralTraceInfo().clkPeriod;
    maxZoomLevel = maxZoomClks * navigator->GeneralTraceInfo().clkPeriod;
    textVisibilityZoomLevel = textVisibilityClks * navigator->GeneralTraceInfo().clkPeriod;
    zoomZone = new QwtPlotZoneItem();
    zoomZone->setZ(2);
    zoomZone->attach(this);
    zoomZone->setVisible(false);
}

void TracePlot::updateScrollbar()
{
    // The maximum number of displayed lines determined by the pageStep of the scroll bar.
    const unsigned int maxDisplayedLines = scrollBar->pageStep();

    const int maximum = drawingProperties.getNumberOfDisplayedLines() - maxDisplayedLines - 1;

    if (maximum >= 0)
    {
        scrollBar->setMaximum(maximum);
        scrollBar->show();
    }
    else
        scrollBar->hide();

    verticalScrollbarChanged(scrollBar->value());
}

void TracePlot::verticalScrollbarChanged(int value)
{
    const int yMax = drawingProperties.getNumberOfDisplayedLines() - 1;

    if (scrollBar->isHidden())
    {
        setAxisScale(yLeft, -1, yMax + 1, 1.0);
    }
    else
    {
        setAxisScale(yLeft, scrollBar->maximum() - 1 - value, yMax + 1 - value, 1.0);
    }

    replot();
}

void TracePlot::setUpAxis()
{
    // Set up y axis.
    setAxisScaleDraw(yLeft, customLabelScaleDraw);
    customLabelScaleDraw->setMinimumExtent(135.0);

    // Set up x axis.
    setAxisTitle(xBottom, "Time in ns");
    setAxisScaleDraw(xBottom, new EngineeringScaleDraw);
}

void TracePlot::updateToggleCollapsedAction()
{
    ToggleCollapsedAction::CollapsedState state = getCollapsedState();
    toggleCollapsedState->updateCollapsedState(state);
}

Timespan TracePlot::GetCurrentTimespan()
{
    Timespan span(navigator->CurrentTraceTime() - zoomLevel / 2,
                  navigator->CurrentTraceTime() + zoomLevel / 2);
    if (span.Begin() < 0)
        span.shift(-span.Begin());
    else if (span.End() > navigator->GeneralTraceInfo().span.End())
        span.shift(navigator->GeneralTraceInfo().span.End() - span.End());
    return span;
}

ToggleCollapsedAction::CollapsedState TracePlot::getCollapsedState() const
{
    using CollapsedState = ToggleCollapsedAction::CollapsedState;

    CollapsedState state = CollapsedState::Collapsed;

    unsigned int notCollapsedCount = 0;
    unsigned int rankCount = 0;

    auto selectedModel = tracePlotLineDataSource->getSelectedModel();

    for (int i = 0; i < selectedModel->rowCount(); i++)
    {
        QModelIndex index = selectedModel->index(i, 0);
        auto type = static_cast<AbstractTracePlotLineModel::LineType>(
            selectedModel->data(index, AbstractTracePlotLineModel::TypeRole).toInt());

        if (type != AbstractTracePlotLineModel::RankGroup)
            continue;

        bool isCollapsed =
            selectedModel->data(index, AbstractTracePlotLineModel::CollapsedRole).toBool();
        if (!isCollapsed)
        {
            notCollapsedCount++;
            state = CollapsedState::Mixed;
        }

        rankCount++;
    }

    if (notCollapsedCount == rankCount)
        state = CollapsedState::Expanded;

    return state;
}

void TracePlot::collapseOrExpandAllRanks(ToggleCollapsedAction::CollapseAction collapseAction)
{
    using CollapseAction = ToggleCollapsedAction::CollapseAction;
    auto selectedModel = tracePlotLineDataSource->getSelectedModel();

    for (int i = 0; i < selectedModel->rowCount(); i++)
    {
        QModelIndex index = selectedModel->index(i, 0);
        auto type = static_cast<AbstractTracePlotLineModel::LineType>(
            selectedModel->data(index, AbstractTracePlotLineModel::TypeRole).toInt());

        if (type != AbstractTracePlotLineModel::RankGroup)
            continue;

        switch (collapseAction)
        {
        case CollapseAction::CollapseAllRanks:
            selectedModel->setData(index, true, AbstractTracePlotLineModel::CollapsedRole);
            break;

        case CollapseAction::ExpandAllRanks:
            selectedModel->setData(index, false, AbstractTracePlotLineModel::CollapsedRole);
            break;
        }
    }

    recreateCollapseButtons();
}

void TracePlot::getAndDrawComments()
{
    QList<QModelIndex> selectedRows = commentModel->selectionModel()->selectedRows();

    for (int row = 0; row < commentModel->rowCount(); row++)
    {
        QModelIndex timeIndex =
            commentModel->index(row, static_cast<int>(CommentModel::Column::Time));
        QModelIndex textIndex =
            commentModel->index(row, static_cast<int>(CommentModel::Column::Comment));

        bool selected =
            std::find(selectedRows.begin(), selectedRows.end(), commentModel->index(row, 0)) !=
            selectedRows.end();

        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setLabel(textIndex.data().toString());
        marker->setLabelOrientation(Qt::Vertical);
        marker->setLabelAlignment(Qt::AlignLeft | Qt::AlignBottom);
        marker->setXValue(static_cast<double>(timeIndex.data(Qt::UserRole).toLongLong()));
        marker->setLineStyle(QwtPlotMarker::LineStyle::VLine);
        marker->setLinePen(QColor(selected ? Qt::red : Qt::blue), 2);
        marker->attach(this);
    }
}

CustomLabelScaleDraw* TracePlot::getCustomLabelScaleDraw() const
{
    return customLabelScaleDraw;
}

const TraceDrawingProperties& TracePlot::getDrawingProperties() const
{
    return drawingProperties;
}

void TracePlot::enterZoomMode()
{
    mouseDownData.mouseIsDownForZooming = true;
    mouseLabel->setMode(MouseLabelMode::Timedifference);
    zoomZone->setVisible(true);
    zoomZone->setInterval(mouseDownData.zoomSpan.Begin(), mouseDownData.zoomSpan.End());
}

void TracePlot::exitZoomMode()
{
    mouseDownData.mouseIsDownForZooming = false;
    mouseLabel->setMode(MouseLabelMode::AbsoluteTime);
    zoomZone->setVisible(false);
}
void TracePlot::zoomIn(traceTime zoomCenter)
{
    setZoomLevel(zoomLevel * zoomFactor);
    traceTime time = zoomCenter + (GetCurrentTimespan().Middle() - zoomCenter) * zoomFactor;
    Q_EMIT tracePlotZoomChanged();
    navigator->navigateToTime(time);
}

void TracePlot::zoomOut(traceTime zoomCenter)
{
    setZoomLevel(zoomLevel / zoomFactor);
    Q_EMIT tracePlotZoomChanged();
    navigator->navigateToTime(static_cast<traceTime>(
        zoomCenter + (GetCurrentTimespan().Middle() - zoomCenter) / zoomFactor));
}

traceTime TracePlot::ZoomLevel() const
{
    return zoomLevel;
}

void TracePlot::setZoomLevel(traceTime newZoomLevel)
{
    zoomLevel = newZoomLevel;
    if (zoomLevel < minZoomLevel)
        zoomLevel = minZoomLevel;
    if (zoomLevel > navigator->GeneralTraceInfo().span.timeCovered())
        zoomLevel = navigator->GeneralTraceInfo().span.timeCovered();
    if (zoomLevel > maxZoomLevel)
        zoomLevel = maxZoomLevel;

    if (zoomLevel < textVisibilityZoomLevel)
        drawingProperties.drawText = true;

    if (zoomLevel > textVisibilityZoomLevel)
        drawingProperties.drawText = false;
}

/* Q_SLOTS
 *
 *
 */

void TracePlot::recreateCollapseButtons()
{
    drawingProperties.updateLabels();

    auto selectedModel = tracePlotLineDataSource->getSelectedModel();
    selectedModel->recreateCollapseButtons(this, customLabelScaleDraw);
}

void TracePlot::currentTraceTimeChanged()
{
    bool drawDependencies =
        getDrawingProperties().drawDependenciesOption.draw != DependencyOption::Disabled;

    transactions =
        navigator->TraceFile().getTransactionsInTimespan(GetCurrentTimespan(), drawDependencies);

#ifdef EXTENSION_ENABLED
    if (drawDependencies)
    {
        navigator->TraceFile().updateDependenciesInTimespan(GetCurrentTimespan());
    }
#endif

    setAxisScale(xBottom, GetCurrentTimespan().Begin(), GetCurrentTimespan().End());

    dependenciesSubMenu->setEnabled(navigator->TraceFile().checkDependencyTableExists());

    replot();
}

void TracePlot::selectedTransactionsChanged()
{
    replot();
}

void TracePlot::commentsChanged()
{
    detachItems(QwtPlotItem::Rtti_PlotMarker);
    getAndDrawComments();
    replot();
}

void TracePlot::on_executeQuery()
{
    queryEditor->show();
}

void TracePlot::on_selectNextRefresh()
{
    traceTime time = invTransform(xBottom, contextMenuMouseDown.x());
    navigator->selectNextRefresh(time);
}

void TracePlot::on_selectNextActivate()
{
    traceTime time = invTransform(xBottom, contextMenuMouseDown.x());
    navigator->selectNextActivate(time);
}

void TracePlot::on_selectNextPrecharge()
{
    traceTime time = invTransform(xBottom, contextMenuMouseDown.x());
    navigator->selectNextPrecharge(time);
}

void TracePlot::on_selectNextCommand()
{
    traceTime time = invTransform(xBottom, contextMenuMouseDown.x());
    navigator->selectNextCommand(time);
}

// void TracePlot::on_selectNextActb()
// {
//     navigator->selectNextActb();
// }
//
// void TracePlot::on_selectNextPreb()
// {
//     navigator->selectNextPreb();
// }
//
// void TracePlot::on_selectNextRefb()
// {
//     navigator->selectNextRefb();
// }

void TracePlot::on_colorGroupingPhase()
{
    drawingProperties.colorGrouping = ColorGrouping::PhaseType;
    Q_EMIT(colorGroupingChanged(ColorGrouping::PhaseType));
    replot();
}

void TracePlot::on_colorGroupingTransaction()
{
    drawingProperties.colorGrouping = ColorGrouping::Transaction;
    Q_EMIT(colorGroupingChanged(ColorGrouping::Transaction));
    replot();
}

void TracePlot::on_colorGroupingRainbowTransaction()
{
    drawingProperties.colorGrouping = ColorGrouping::RainbowTransaction;
    Q_EMIT(colorGroupingChanged(ColorGrouping::RainbowTransaction));
    replot();
}

void TracePlot::on_colorGroupingThread()
{
    drawingProperties.colorGrouping = ColorGrouping::Thread;
    Q_EMIT(colorGroupingChanged(ColorGrouping::Thread));
    replot();
}

void TracePlot::on_goToTransaction()
{
    bool ok;
    int maxID = navigator->GeneralTraceInfo().numberOfTransactions;
    int transactionID =
        QInputDialog::getInt(this,
                             "Go to transaction",
                             "Enter transaction ID (1 - " + QString::number(maxID) + ")",
                             0,
                             1,
                             maxID,
                             1,
                             &ok);
    if (ok)
    {
        navigator->clearSelectedTransactions();
        navigator->selectTransaction(transactionID);
    }
}

void TracePlot::on_goToPhase()
{
    bool ok;
    int maxID = navigator->GeneralTraceInfo().numberOfPhases;
    int phaseID = QInputDialog::getInt(this,
                                       "Go to phase",
                                       "Enter phase ID (1 - " + QString::number(maxID) + ")",
                                       0,
                                       1,
                                       maxID,
                                       1,
                                       &ok);

    if (ok)
    {
        navigator->clearSelectedTransactions();
        navigator->selectTransaction(navigator->TraceFile().getTransactionIDFromPhaseID(phaseID));
    }
}

void TracePlot::on_deselectAll()
{
    navigator->clearSelectedTransactions();
}

void TracePlot::on_insertComment()
{
    traceTime time = invTransform(xBottom, contextMenuMouseDown.x());
    commentModel->addComment(time);
}

void TracePlot::on_goToTime()
{
    double goToTime;
    GoToTimeDialog dialog(&goToTime, this);
    int dialogCode = dialog.exec();
    if (dialogCode == QDialog::Accepted)
    {
        traceTime time = static_cast<traceTime>(goToTime) * 1000;
        navigator->navigateToTime(time);
    }
}

void TracePlot::on_exportToPDF()
{
    QwtPlotRenderer renderer;
    QString filename =
        QFileDialog::getSaveFileName(this, "Export to SVG", "", "Portable Document Format(*.svg)");
    if (filename != "")
    {
        QBrush saved = this->canvasBackground();
        this->setCanvasBackground(QBrush(Qt::white));
        renderer.renderDocument(this, filename, "svg", QSizeF(this->widthMM(), this->heightMM()));
        this->setCanvasBackground(QBrush(saved));
    }
}

void TracePlot::on_toggleCollapsedState()
{
    collapseOrExpandAllRanks(toggleCollapsedState->getCollapseAction());
    updateToggleCollapsedAction();
}

/* Keyboard and mouse events
 *
 *
 */

void TracePlot::keyPressEvent(QKeyEvent* keyPressedEvent)
{
    int key = keyPressedEvent->key();
    if (Qt::Key_Control == key)
        keyPressData.ctrlPressed = true;
    else if (Qt::Key_Shift == key)
        keyPressData.shiftPressed = true;
    else if (Qt::Key_Right == key)
        navigator->selectNextTransaction();
    else if (Qt::Key_Left == key)
        navigator->selectPreviousTransaction();
    else if (Qt::Key_Minus == key)
    {
        zoomOut(GetCurrentTimespan().Middle());
    }
    else if (Qt::Key_Plus == key)
    {
        zoomIn(GetCurrentTimespan().Middle());
    }
}

void TracePlot::keyReleaseEvent(QKeyEvent* keyReleasedEvent)
{
    int key = keyReleasedEvent->key();

    if (Qt::Key_Control == key)
        keyPressData.ctrlPressed = false;
    else if (Qt::Key_Shift == key)
    {
        keyPressData.shiftPressed = false;
        exitZoomMode();
        replot();
    }
}

bool TracePlot::eventFilter(QObject* object, QEvent* event)
{
    if (object == canvas())
    {
        switch (event->type())
        {
        case QEvent::Wheel:
        {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            traceTime zoomCenter =
                static_cast<traceTime>(this->invTransform(xBottom, wheelEvent->position().x()));

            (wheelEvent->angleDelta().y() > 0) ? zoomIn(zoomCenter) : zoomOut(zoomCenter);

            return true;
        }
        case QEvent::MouseButtonPress:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            if (mouseEvent->button() == Qt::LeftButton)
            {
                if (keyPressData.shiftPressed)
                {
                    mouseDownData.zoomSpan.setBegin(
                        alignToClk(invTransform(xBottom, mouseEvent->x()),
                                   navigator->GeneralTraceInfo().clkPeriod));
                    mouseDownData.zoomSpan.setEnd(
                        alignToClk(invTransform(xBottom, mouseEvent->x()),
                                   navigator->GeneralTraceInfo().clkPeriod));
                    enterZoomMode();
                }
                else
                {
                    mouseDownData.mouseDownX = mouseEvent->x();
                    mouseDownData.mouseDownTime = GetCurrentTimespan().Middle();
                    mouseDownData.mouseIsDownForDragging = true;
                    canvas()->setCursor(Qt::ClosedHandCursor);
                    SelectTransaction(mouseEvent->x(), mouseEvent->y());
                    SelectComment(mouseEvent->x());
                }
                return true;
            }
            else if (mouseEvent->button() == Qt::RightButton)
            {
                // Also select comments to make it more obvious.
                SelectComment(mouseEvent->x());

                openContextMenu(this->canvas()->mapToGlobal(mouseEvent->pos()), mouseEvent->pos());
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            if (mouseEvent->button() == Qt::LeftButton)
            {
                if (mouseDownData.mouseIsDownForDragging)
                {
                    mouseDownData.mouseIsDownForDragging = false;
                    canvas()->setCursor(Qt::ArrowCursor);
                    return true;
                }
                else if (mouseDownData.mouseIsDownForZooming)
                {
                    exitZoomMode();
                    traceTime newCenter = mouseDownData.zoomSpan.Middle();
                    setZoomLevel(mouseDownData.zoomSpan.timeCovered());
                    navigator->navigateToTime(newCenter);
                    replot();
                    return true;
                }
            }
            break;
        }
        case QEvent::MouseMove:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            if (mouseDownData.mouseIsDownForDragging)
            {
                traceTime deltaTime = invTransform(xBottom, mouseDownData.mouseDownX) -
                                      invTransform(xBottom, mouseEvent->x());
                navigator->navigateToTime(mouseDownData.mouseDownTime + deltaTime);
                return true;
            }
            else if (mouseDownData.mouseIsDownForZooming)
            {
                mouseDownData.zoomSpan.setEnd(alignToClk(invTransform(xBottom, mouseEvent->x()),
                                                         navigator->GeneralTraceInfo().clkPeriod));
                if (mouseDownData.zoomSpan.Begin() < mouseDownData.zoomSpan.End())
                    zoomZone->setInterval(mouseDownData.zoomSpan.Begin(),
                                          mouseDownData.zoomSpan.End());
                else
                    zoomZone->setInterval(mouseDownData.zoomSpan.End(),
                                          mouseDownData.zoomSpan.Begin());

                replot();
            }
            break;
        }
        default:
            break;
        }
    }
    return false;
}

void TracePlot::SelectTransaction(int x, int y) const
{
    double yVal = invTransform(yLeft, y);
    Timespan timespan = hoveredTimespan(x);

    auto selectedTransactions = tracePlotItem->getSelectedTransactions(timespan, yVal);

    if (selectedTransactions.size() > 0)
    {
        if (!keyPressData.ctrlPressed)
            navigator->clearSelectedTransactions();

        navigator->addSelectedTransactions(selectedTransactions);
    }
}

void TracePlot::SelectComment(int x) const
{
    Timespan timespan = hoveredTimespan(x);
    QModelIndex index = commentModel->hoveredComment(timespan);

    if (!index.isValid())
        return;

    if (keyPressData.ctrlPressed)
        commentModel->selectionModel()->setCurrentIndex(
            index, QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
    else
        commentModel->selectionModel()->setCurrentIndex(
            index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

Timespan TracePlot::hoveredTimespan(int x) const
{
    traceTime time = invTransform(xBottom, x);
    traceTime offset = 0.005 * zoomLevel;

    return Timespan(time - offset, time + offset);
}

void TracePlot::openContextMenu(const QPoint& pos, const QPoint& mouseDown)
{
    contextMenuMouseDown = mouseDown;
    Timespan timespan = hoveredTimespan(mouseDown.x());

    if (commentModel->hoveredComment(timespan).isValid())
        commentModel->openContextMenu();
    else
    {
        updateToggleCollapsedAction();
        contextMenu->exec(pos);
    }
}
