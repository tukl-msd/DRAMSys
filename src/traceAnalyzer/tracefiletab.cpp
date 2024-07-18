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

#include "tracefiletab.h"
#include "businessObjects/pythoncaller.h"
#include "extensionDisclaimer.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>

#include <qwt_plot_canvas.h>

#include <fstream>

TraceFileTab::TraceFileTab(std::string_view traceFilePath,
                           PythonCaller& pythonCaller,
                           QWidget* parent) :
    QWidget(parent),
    traceFilePath(traceFilePath),
    ui(new Ui::TraceFileTab),
    commentModel(new CommentModel(this)),
    navigator(new TraceNavigator(traceFilePath.data(), commentModel, this)),
    mcConfigModel(new McConfigModel(navigator->TraceFile(), this)),
    memSpecModel(new MemSpecModel(navigator->TraceFile(), this)),
    availableRowsModel(new AvailableTracePlotLineModel(navigator->GeneralTraceInfo(), this)),
    selectedRowsModel(new SelectedTracePlotLineModel(navigator->GeneralTraceInfo(), this)),
    tracePlotLineDataSource(new TracePlotLineDataSource(selectedRowsModel, this)),
#ifdef EXTENSION_ENABLED
    depInfosView(new DependencyInfosModel(navigator->TraceFile(), this)),
#endif
    pythonCaller(pythonCaller),
    savingChangesToDB(false)
{
    ui->setupUi(this);

    std::cout << "Opening new tab for \"" << traceFilePath << "\"" << std::endl;

    ui->mcConfigView->setModel(mcConfigModel);
    ui->mcConfigView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->memSpecView->setModel(memSpecModel);
    ui->memSpecView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

#ifdef EXTENSION_ENABLED
    ui->depInfosView->setModel(depInfosView);
    ui->depInfosView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif

    setUpTraceSelector();
    initNavigatorAndItsDependentWidgets();
    setUpFileWatcher(traceFilePath.data());
    setUpTraceplotScrollbar();
    setUpCommentView();

#ifdef EXTENSION_ENABLED
        setUpPossiblePhases();
#else
        addDisclaimer();
#endif

    tracefileChanged();
}

TraceFileTab::~TraceFileTab()
{
    delete ui;
}

void TraceFileTab::commitChangesToDB()
{
    savingChangesToDB = true;
    navigator->commitChangesToDB();
}

void TraceFileTab::exportAsVCD()
{
    std::string filename =
        QFileDialog::getSaveFileName(this, "Export to VCD", "", "VCD files (*.vcd)").toStdString();

    auto dump = PythonCaller::dumpVcd(traceFilePath);

    std::ofstream file(filename);
    file << dump;

    Q_EMIT statusChanged(QString("VCD export finished."));
}

void TraceFileTab::setUpTraceSelector()
{
    ui->availableTreeView->setModel(availableRowsModel);
    ui->availableTreeView->setSelectionModel(availableRowsModel->selectionModel());
    ui->availableTreeView->installEventFilter(availableRowsModel);

    ui->selectedTreeView->setModel(selectedRowsModel);
    ui->selectedTreeView->setSelectionModel(selectedRowsModel->selectionModel());
    ui->selectedTreeView->installEventFilter(selectedRowsModel);

    connect(availableRowsModel,
            &AvailableTracePlotLineModel::returnPressed,
            selectedRowsModel,
            &SelectedTracePlotLineModel::addIndexesFromAvailableModel);

    connect(ui->availableTreeView,
            &QAbstractItemView::doubleClicked,
            availableRowsModel,
            &AvailableTracePlotLineModel::itemsDoubleClicked);
    connect(ui->selectedTreeView,
            &QAbstractItemView::doubleClicked,
            selectedRowsModel,
            &SelectedTracePlotLineModel::itemsDoubleClicked);

    connect(selectedRowsModel,
            &QAbstractItemModel::dataChanged,
            tracePlotLineDataSource,
            &TracePlotLineDataSource::updateModel);
    connect(selectedRowsModel,
            &QAbstractItemModel::rowsInserted,
            tracePlotLineDataSource,
            &TracePlotLineDataSource::updateModel);
    connect(selectedRowsModel,
            &QAbstractItemModel::rowsRemoved,
            tracePlotLineDataSource,
            &TracePlotLineDataSource::updateModel);
}

void TraceFileTab::setUpTraceplotScrollbar()
{
    QObject::connect(ui->traceplotScrollbar,
                     SIGNAL(valueChanged(int)),
                     ui->traceplot,
                     SLOT(verticalScrollbarChanged(int)));
}

void TraceFileTab::initNavigatorAndItsDependentWidgets()
{
    ui->traceplot->init(navigator, ui->traceplotScrollbar, tracePlotLineDataSource, commentModel);

    ui->traceScroller->init(navigator, ui->traceplot, tracePlotLineDataSource);
    connect(this,
            SIGNAL(colorGroupingChanged(ColorGrouping)),
            ui->traceScroller,
            SLOT(colorGroupingChanged(ColorGrouping)));

    ui->selectedTransactionTree->init(navigator);
    // ui->debugMessages->init(navigator,ui->traceplot);

    ui->bandwidthPlot->canvas()->installEventFilter(this);
    ui->powerPlot->canvas()->installEventFilter(this);
    ui->bufferPlot->canvas()->installEventFilter(this);
}

void TraceFileTab::setUpFileWatcher(QString path)
{
    fileWatcher = new QFileSystemWatcher(QStringList(path), this);
    QObject::connect(fileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(tracefileChanged()));
}

void TraceFileTab::setUpCommentView()
{
    ui->commentView->setModel(commentModel);
    ui->commentView->setSelectionModel(commentModel->selectionModel());
    ui->commentView->installEventFilter(commentModel);
    ui->commentView->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(ui->commentView,
                     &QTableView::customContextMenuRequested,
                     commentModel,
                     &CommentModel::openContextMenu);

    QObject::connect(commentModel,
                     &CommentModel::editTriggered,
                     ui->commentView,
                     [=](const QModelIndex& index)
                     {
                         ui->tabWidget->setCurrentWidget(ui->tabComments);
                         ui->commentView->edit(index);
                         ui->commentView->scrollTo(index);
                     });

    QObject::connect(
        ui->commentView, &QTableView::doubleClicked, commentModel, &CommentModel::rowDoubleClicked);
}

void TraceFileTab::addDisclaimer()
{
    // Latency analysis
    auto* latencyDisclaimerLabel = disclaimerLabel();
    ui->latencyLayout->insertWidget(0, latencyDisclaimerLabel);

    ui->latencyAnalysisProgressBar->setEnabled(false);
    ui->startLatencyAnalysis->setEnabled(false);
    ui->latencyPlot->setEnabled(false);
    ui->latencyTreeView->setEnabled(false);

    // Power analysis
    auto* powerDisclaimerLabel = disclaimerLabel();
    ui->powerLayout->insertWidget(0, powerDisclaimerLabel);

    ui->startPowerAnalysis->setEnabled(false);
    ui->powerBox->setEnabled(false);
    ui->bandwidthBox->setEnabled(false);
    ui->bufferBox->setEnabled(false);

    // Dependencies
    auto* dependenciesDisclaimerLabel = disclaimerLabel();
    ui->verticalLayout_depInfos->insertWidget(0, dependenciesDisclaimerLabel);

    ui->depInfoLabel->setEnabled(false);
    ui->depInfosView->setEnabled(false);
    ui->depTabPossiblePhases->setEnabled(false);
    ui->calculateDependencies->setEnabled(false);
}

void TraceFileTab::tracefileChanged()
{
    if (savingChangesToDB == true)
    {
        // Database has changed due to user action (e.g., saving comments).
        // No need to disable the "Save changes to DB" menu.
        savingChangesToDB = false;
        Q_EMIT statusChanged(QString("Changes saved "));
    }
    else
    {
        // External event changed the database file (e.g., the database file
        // was overwritten when running a new test).
        // The "Save changes to DB" menu must be disabled to avoid saving
        // changes to a corrupted or inconsistent file.
        Q_EMIT statusChanged(QString("At least one database has changed on disk "));
    }
    navigator->refreshData();
}

void TraceFileTab::closeEvent(QCloseEvent* event)
{
    if (navigator->existChangesToCommit())
    {
        QMessageBox saveDialog;
        saveDialog.setWindowTitle(QFileInfo(traceFilePath.data()).baseName());
        saveDialog.setText("The trace file has been modified.");
        saveDialog.setInformativeText(
            "Do you want to save your changes?<br><b>Unsaved changes will be lost.</b>");
        saveDialog.setStandardButtons(QMessageBox::Save | QMessageBox::Discard |
                                      QMessageBox::Cancel);
        saveDialog.setDefaultButton(QMessageBox::Save);
        saveDialog.setIcon(QMessageBox::Warning);
        int returnCode = saveDialog.exec();

        switch (returnCode)
        {
        case QMessageBox::Cancel:
            event->ignore();
            break;
        case QMessageBox::Discard:
            event->accept();
            break;
        case QMessageBox::Save:
            commitChangesToDB();
            event->accept();
            break;
        };
    }
    else
        event->accept();
}

traceTime TraceFileTab::getCurrentTraceTime() const
{
    return navigator->CurrentTraceTime();
}

void TraceFileTab::navigateToTime(traceTime time)
{
    navigator->navigateToTime(time);
}

traceTime TraceFileTab::getZoomLevel() const
{
    TracePlot* traceplot = static_cast<TracePlot*>(ui->traceplot);
    return traceplot->ZoomLevel();
}

void TraceFileTab::setZoomLevel(traceTime zoomLevel)
{
    TracePlot* traceplot = static_cast<TracePlot*>(ui->traceplot);
    TraceScroller* tracescroller = static_cast<TraceScroller*>(ui->traceScroller);
    traceplot->setZoomLevel(zoomLevel);
    tracescroller->tracePlotZoomChanged();
}

std::shared_ptr<AbstractTracePlotLineModel::Node> TraceFileTab::saveTraceSelectorState() const
{
    return selectedRowsModel->getClonedRootNode();
}

void TraceFileTab::restoreTraceSelectorState(
    std::shared_ptr<AbstractTracePlotLineModel::Node> rootNode)
{
    selectedRowsModel->setRootNode(std::move(rootNode));
}

bool TraceFileTab::eventFilter(QObject* object, QEvent* event)
{
    if (auto canvas = qobject_cast<QwtPlotCanvas*>(object))
    {
        if (event->type() == QEvent::MouseButtonDblClick)
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

            if (mouseEvent->button() != Qt::LeftButton)
                return false;

            QwtPlot* plot = canvas->plot();

            double realTime = plot->invTransform(QwtPlot::xBottom, mouseEvent->x());

            // Convert from seconds to picoseconds
            traceTime time = realTime * 1000 * 1000 * 1000 * 1000;

            navigator->navigateToTime(time);
            return true;
        }
    }

    return QWidget::eventFilter(object, event);
}
