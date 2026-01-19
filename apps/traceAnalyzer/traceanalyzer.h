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

#ifndef TRACEANALYZER_H
#define TRACEANALYZER_H

#include "evaluationtool.h"
#include <QLabel>
#include <QMainWindow>
#include <QSet>
#include <QString>
#include <vector>

namespace Ui
{
class TraceAnalyzer;
}

class TraceFileTab;

class TraceAnalyzer : public QMainWindow
{
    Q_OBJECT

public:
    explicit TraceAnalyzer(QWidget* parent = nullptr);
    explicit TraceAnalyzer(QSet<QString> paths, QWidget* parent = nullptr);
    ~TraceAnalyzer();

    void setUpStatusBar();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    TraceFileTab* createTraceFileTab(const QString& path);
    void openTracefileTab(const QString& path);
    void reloadTab(int index);
    void closeTab(int index);

    QLabel* statusLabel;
    QSet<QString> openedTraceFiles;
    EvaluationTool evaluationTool;
    PythonCaller pythonCaller;

private Q_SLOTS:
    void on_menuFile_aboutToShow();
    void on_traceFileTabs_tabCloseRequested(int index);

    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSave_all_triggered();
    void on_actionReload_triggered();
    void on_actionReload_all_triggered();
    void on_actionExportAsVCD_triggered();
    void on_actionMetrics_triggered();
    void on_actionClose_triggered();
    void on_actionClose_all_triggered();
    void on_actionAbout_triggered();

public Q_SLOTS:
    void statusChanged(const QString& message);

private:
    Ui::TraceAnalyzer* ui;
};

#endif // TRACEANALYZER_H
