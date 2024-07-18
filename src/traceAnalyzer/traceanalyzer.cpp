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

#include "traceanalyzer.h"
#include "extensionDisclaimer.h"
#include "tracefiletab.h"
#include "ui_traceanalyzer.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

void TraceAnalyzer::setUpStatusBar()
{
    statusLabel = new QLabel(this);
    statusBar()->addWidget(statusLabel);
}

TraceAnalyzer::TraceAnalyzer(QWidget* parent) :
    QMainWindow(parent),
    evaluationTool(pythonCaller),
    ui(new Ui::TraceAnalyzer)
{
    ui->setupUi(this);
    setUpStatusBar();
    ui->traceFileTabs->clear();

    QObject::connect(ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

TraceAnalyzer::TraceAnalyzer(QSet<QString> paths, QWidget* parent) : TraceAnalyzer(parent)
{
    for (const QString& path : paths)
        openTracefileTab(path);
}

TraceAnalyzer::~TraceAnalyzer()
{
    delete ui;
}

void TraceAnalyzer::on_actionOpen_triggered()
{
    QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Open Tracefile"), "../simulator/", tr("Tracefile (*.tdb)"));
    if (paths.isEmpty())
        return;

    for (const QString& path : paths)
        openTracefileTab(path);
}

TraceFileTab* TraceAnalyzer::createTraceFileTab(const QString& path)
{
    auto* traceFileTab = new TraceFileTab(path.toStdString(), pythonCaller, this);

    connect(traceFileTab, &TraceFileTab::statusChanged, this, &TraceAnalyzer::statusChanged);

    return traceFileTab;
}

void TraceAnalyzer::openTracefileTab(const QString& path)
{
    if (openedTraceFiles.contains(path))
        return;

    TraceFileTab* traceFileTab = createTraceFileTab(path);

    ui->traceFileTabs->addTab(traceFileTab, QFileInfo(path).baseName());
    openedTraceFiles.insert(path);

    statusLabel->clear();
}

void TraceAnalyzer::on_menuFile_aboutToShow()
{
    ui->actionOpen->setEnabled(true);
    ui->actionQuit->setEnabled(true);

    bool tabsOpen = ui->traceFileTabs->count() > 0;

    ui->actionSave->setEnabled(tabsOpen);
    ui->actionSave_all->setEnabled(tabsOpen);
    ui->actionReload->setEnabled(tabsOpen);
    ui->actionReload_all->setEnabled(tabsOpen);
    ui->actionExportAsVCD->setEnabled(tabsOpen);
    ui->actionMetrics->setEnabled(tabsOpen);
    ui->actionClose->setEnabled(tabsOpen);
    ui->actionClose_all->setEnabled(tabsOpen);
}

void TraceAnalyzer::closeTab(int index)
{
    auto* traceFileTab = dynamic_cast<TraceFileTab*>(ui->traceFileTabs->widget(index));

    if (traceFileTab->close())
    {
        openedTraceFiles.remove(traceFileTab->getPathToTraceFile());
        ui->traceFileTabs->removeTab(index);
        delete traceFileTab;
    }
}

void TraceAnalyzer::on_traceFileTabs_tabCloseRequested(int index)
{
    closeTab(index);
}

void TraceAnalyzer::on_actionClose_triggered()
{
    closeTab(ui->traceFileTabs->currentIndex());
}

void TraceAnalyzer::on_actionClose_all_triggered()
{
    for (unsigned int i = ui->traceFileTabs->count(); i--;)
        closeTab(i);

    statusLabel->clear();
}

void TraceAnalyzer::reloadTab(int index)
{
    auto* traceFileTab = dynamic_cast<TraceFileTab*>(ui->traceFileTabs->widget(index));

    QString traceFile = traceFileTab->getPathToTraceFile();
    traceTime time = traceFileTab->getCurrentTraceTime();
    traceTime zoomLevel = traceFileTab->getZoomLevel();
    auto rootNode = traceFileTab->saveTraceSelectorState();

    if (!traceFileTab->close())
        return;

    ui->traceFileTabs->removeTab(index);
    delete traceFileTab;

    traceFileTab = createTraceFileTab(traceFile);
    traceFileTab->setZoomLevel(zoomLevel);
    traceFileTab->navigateToTime(time);
    traceFileTab->restoreTraceSelectorState(std::move(rootNode));

    ui->traceFileTabs->insertTab(index, traceFileTab, QFileInfo(traceFile).baseName());
    ui->traceFileTabs->setCurrentIndex(index);
}

void TraceAnalyzer::on_actionReload_triggered()
{
    int index = ui->traceFileTabs->currentIndex();
    reloadTab(index);
    this->statusChanged(QString("Reloaded tab ") + QString::number(index) + " ");
}

void TraceAnalyzer::on_actionReload_all_triggered()
{
    int index = ui->traceFileTabs->currentIndex();

    for (unsigned int i = ui->traceFileTabs->count(); i--;)
        reloadTab(i);

    ui->traceFileTabs->setCurrentIndex(index);

    this->statusChanged(QString("All databases reloaded "));
}

void TraceAnalyzer::on_actionSave_triggered()
{
    auto* traceFileTab = dynamic_cast<TraceFileTab*>(ui->traceFileTabs->currentWidget());
    traceFileTab->commitChangesToDB();

    this->statusChanged(QString("Saved database ") +
                        QFileInfo(traceFileTab->getPathToTraceFile()).baseName() + " ");
}

void TraceAnalyzer::on_actionSave_all_triggered()
{
    for (int index = 0; index < ui->traceFileTabs->count(); index++)
    {
        // Changes in the database files will trigger the file watchers from
        // the TraceFileTab class. They generate signals connected to
        // TraceAnalyzer::statusChanged().
        auto* traceFileTab = dynamic_cast<TraceFileTab*>(ui->traceFileTabs->widget(index));
        traceFileTab->commitChangesToDB();
    }
}

void TraceAnalyzer::on_actionExportAsVCD_triggered()
{
#ifndef EXTENSION_ENABLED
        showExtensionDisclaimerMessageBox();
        return;
#endif

    auto* traceFileTab = dynamic_cast<TraceFileTab*>(ui->traceFileTabs->currentWidget());
    traceFileTab->exportAsVCD();
}

void TraceAnalyzer::statusChanged(const QString& message)
{
    statusLabel->setText(message + QTime::currentTime().toString());
}

void TraceAnalyzer::on_actionMetrics_triggered()
{
#ifndef EXTENSION_ENABLED
        showExtensionDisclaimerMessageBox();
        return;
#endif

    evaluationTool.raise();
    evaluationTool.activateWindow();
    evaluationTool.showAndEvaluateMetrics(openedTraceFiles.values());
}

void TraceAnalyzer::on_actionAbout_triggered()
{
    QMessageBox::about(
        this,
        QStringLiteral("DRAMSys"),
        QStringLiteral(
            "<b>DRAMSys5.0</b> is a flexible DRAM subsystem design space exploration "
            "framework based on SystemC "
            "TLM-2.0. It was developed by the <a "
            "href=\"https://ems.eit.uni-kl.de/en/start/\">Microelectronic Systems "
            "Design Research Group</a>, by <a "
            "href=\"https://www.iese.fraunhofer.de/en.html\">Fraunhofer IESE</a> and by the <a "
            "href=\"https://www.informatik.uni-wuerzburg.de/ce\">Computer Engineering Group</a> at "
            "<a href=\"https://www.uni-wuerzburg.de/en/home\">JMU Würzburg</a>."));
}

void TraceAnalyzer::closeEvent(QCloseEvent* event)
{
    for (int i = 0; i < ui->traceFileTabs->count(); i++)
    {
        QWidget* tab = ui->traceFileTabs->widget(i);
        if (!tab->close())
        {
            event->ignore();
            return;
        }
    }

    event->accept();
}
