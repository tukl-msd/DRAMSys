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

#ifndef TRACEFILETAB_H
#define TRACEFILETAB_H

#include "ui_tracefiletab.h"

#include "businessObjects/configmodels.h"
#ifdef EXTENSION_ENABLED
#include "businessObjects/dependencymodels.h"
#endif
#include "businessObjects/traceplotlinemodel.h"
#include "presentation/tracenavigator.h"
#include "presentation/traceplot.h"
#include "presentation/tracescroller.h"

#include <QFileSystemWatcher>
#include <QString>
#include <QTreeWidget>
#include <QWidget>

class CommentModel;
class McConfigModel;
class MemSpecModel;
class PythonCaller;

class TraceFileTab : public QWidget
{
    Q_OBJECT

public:
    explicit TraceFileTab(std::string_view traceFilePath,
                          PythonCaller& pythonCaller,
                          QWidget* parent);
    ~TraceFileTab();

    void setUpFileWatcher(QString filename);
    void setUpTraceplotScrollbar();
    void setUpCommentView();
    void setUpTraceSelector();
    void addDisclaimer();

#ifdef EXTENSION_ENABLED
    void setUpPossiblePhases();
#endif

    void initNavigatorAndItsDependentWidgets();
    QString getPathToTraceFile() { return traceFilePath.data(); }
    void commitChangesToDB();
    void exportAsVCD();

    traceTime getCurrentTraceTime() const;
    void navigateToTime(traceTime time);
    traceTime getZoomLevel() const;
    void setZoomLevel(traceTime zoomLevel);

    std::shared_ptr<AbstractTracePlotLineModel::Node> saveTraceSelectorState() const;
    void restoreTraceSelectorState(std::shared_ptr<AbstractTracePlotLineModel::Node> rootNode);

protected:
    void closeEvent(QCloseEvent* event) override;

    /**
     * Used to respond to double click events in the analysis
     * plots to navigate quickly to the corresponding tracetime.
     * May be moved into a seperate class at a later point in time.
     */
    bool eventFilter(QObject* object, QEvent* event) override;

private:
    std::string traceFilePath;
    Ui::TraceFileTab* ui;
    QFileSystemWatcher* fileWatcher;

    CommentModel* commentModel;
    TraceNavigator* navigator;

    McConfigModel* mcConfigModel;
    MemSpecModel* memSpecModel;

    AvailableTracePlotLineModel* availableRowsModel;
    SelectedTracePlotLineModel* selectedRowsModel;
    TracePlotLineDataSource* tracePlotLineDataSource;

#ifdef EXTENSION_ENABLED
    QAbstractItemModel* depInfosView;
#endif

    PythonCaller& pythonCaller;

    void setUpQueryEditor(QString path);
    bool savingChangesToDB;

public Q_SLOTS:
    void tracefileChanged();

Q_SIGNALS:
    void statusChanged(const QString& message);
    void colorGroupingChanged(ColorGrouping colorgrouping);

private Q_SLOTS:
#ifdef EXTENSION_ENABLED
    void on_latencyTreeView_doubleClicked(const QModelIndex& index);
    void on_calculateDependencies_clicked();
    void on_startLatencyAnalysis_clicked();
    void on_startPowerAnalysis_clicked();
#endif
};

#endif // TRACEFILETAB_H
