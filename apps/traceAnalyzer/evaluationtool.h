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
 *    Éder F. Zulian
 *    Felipe S. Prado
 *    Derek Christ
 */

#ifndef EVALUATIONTOOL_H
#define EVALUATIONTOOL_H

#include "selectmetrics.h"

#include "businessObjects/pythoncaller.h"
#include "businessObjects/tracecalculatedmetrics.h"
#include <QList>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QWidget>
#include <vector>

namespace Ui
{
class EvaluationTool;
}

class EvaluationTool : public QWidget
{
    Q_OBJECT

public:
    explicit EvaluationTool(PythonCaller& pythonCaller, QWidget* parent = nullptr);
    ~EvaluationTool();

    void showForFiles(QList<QString> paths);
    void showAndEvaluateMetrics(QList<QString> paths);

private Q_SLOTS:
    void on_btn_calculateMetrics_clicked();
    void on_btn_exportCSV_clicked();
    void on_btn_genPlots_clicked();
    void getSelectedMetrics();

private:
    void fillFileList(QList<QString> paths);
    void cleanUpUI();
    void genPlots();
    void calculateMetrics(std::vector<long> selectedMetrics);
    std::vector<std::string> getMetrics();

    Ui::EvaluationTool* ui;
    QStandardItemModel* traceFilesModel;
    std::vector<TraceCalculatedMetrics> calculatedMetrics;
    SelectMetrics* selectMetrics;

    PythonCaller& pythonCaller;

    class TraceFileItem : public QStandardItem
    {
    public:
        TraceFileItem(const QString& path);
        QString getPath() { return path; }

    private:
        QString path;
    };
};

#endif // EVALUATIONTOOL_H
