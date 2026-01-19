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

#include "evaluationtool.h"
#include "ui_evaluationtool.h"
#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QPainter>
#include <QTextStream>
#include <QtAlgorithms>
#include <iostream>
#include <stdio.h>

EvaluationTool::EvaluationTool(PythonCaller& pythonCaller, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::EvaluationTool),
    pythonCaller(pythonCaller)
{
    ui->setupUi(this);
    traceFilesModel = new QStandardItemModel(this);
    ui->listView->setModel(traceFilesModel);
    selectMetrics = new SelectMetrics(this);
    QObject::connect(selectMetrics, SIGNAL(getSelectedMetrics()), this, SLOT(getSelectedMetrics()));
}

EvaluationTool::~EvaluationTool()
{
    delete ui;
}

void EvaluationTool::showForFiles(QList<QString> paths)
{
    cleanUpUI();
    fillFileList(paths);
    show();
    ui->toolBox->setCurrentIndex(0);
}

void EvaluationTool::showAndEvaluateMetrics(QList<QString> paths)
{
    cleanUpUI();
    fillFileList(paths);
    show();
    ui->toolBox->setCurrentIndex(1);
    selectMetrics->setMetrics(getMetrics());
}

std::vector<std::string> EvaluationTool::getMetrics()
{
    std::vector<std::string> metrics;
    for (int row = 0; row < traceFilesModel->rowCount(); ++row)
    {
        TraceFileItem* item = static_cast<TraceFileItem*>(traceFilesModel->item(row));
        std::vector<std::string> result =
            PythonCaller::availableMetrics(item->getPath().toStdString());
        if (result.size() > metrics.size()) // TODO use std::set
            metrics = result;
    }
    return metrics;
}

void EvaluationTool::cleanUpUI()
{
    traceFilesModel->clear();
    calculatedMetrics.clear();
    ui->traceMetricTreeWidget->clear();
}

void EvaluationTool::fillFileList(QList<QString> paths)
{
    std::sort(paths.begin(),
              paths.end(),
              [](const QString& path1, const QString& path2)
              { return QFileInfo(path1).baseName() < QFileInfo(path2).baseName(); });
    for (const QString& path : paths)
    {
        traceFilesModel->appendRow(new TraceFileItem(path));
    }
}

void EvaluationTool::on_btn_calculateMetrics_clicked()
{
    selectMetrics->raise();
    selectMetrics->activateWindow();
    selectMetrics->show();
}

void EvaluationTool::getSelectedMetrics()
{
    std::vector<long> selectedMetrics;
    for (QCheckBox* metric : selectMetrics->metrics)
    {
        selectedMetrics.push_back(metric->isChecked());
    }
    calculateMetrics(selectedMetrics);
}

void EvaluationTool::calculateMetrics(std::vector<long> selectedMetrics)
{
    ui->traceMetricTreeWidget->clear();
    for (int row = 0; row < traceFilesModel->rowCount(); ++row)
    {
        TraceFileItem* item = static_cast<TraceFileItem*>(traceFilesModel->item(row));
        if (item->checkState() == Qt::Checked)
        {
            TraceCalculatedMetrics result =
                pythonCaller.evaluateMetrics(item->getPath().toStdString(), selectedMetrics);
            calculatedMetrics.push_back(result);
            ui->traceMetricTreeWidget->addTraceMetricResults(result);
        }
    }
    ui->traceMetricTreeWidget->expandAll();
}

EvaluationTool::TraceFileItem::TraceFileItem(const QString& path)
{
    this->path = path;
    setText(QFileInfo(this->path).baseName());
    setCheckable(true);
    setCheckState(Qt::Checked);
    setEditable(false);
}

void EvaluationTool::on_btn_exportCSV_clicked()
{
    if (calculatedMetrics.size() > 0)
    {
        QString filename = QFileDialog::getSaveFileName(
            this, "Export to CSV", "", "Comma separated Values(*.csv)");
        if (filename != "")
        {
            QFile file(filename);
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&file);
            out << calculatedMetrics[0].toCSVHeader() << "\n";
            for (TraceCalculatedMetrics& metrics : calculatedMetrics)
            {
                out << metrics.toCSVLine() << "\n";
            }
            file.close();
        }
    }
}

void EvaluationTool::on_btn_genPlots_clicked()
{
    genPlots();
}

void EvaluationTool::genPlots()
{
    ui->traceMetricTreeWidget->clear();

    if (traceFilesModel->rowCount() == 0)
        return;

    for (int row = 0; row < traceFilesModel->rowCount(); ++row)
    {
        TraceFileItem* item = static_cast<TraceFileItem*>(traceFilesModel->item(row));
        if (item->checkState() == Qt::Checked)
        {
            ui->traceMetricTreeWidget->addTracePlotResults(
                QFileInfo(item->getPath()).baseName(),
                PythonCaller::generatePlots(item->getPath().toStdString()).c_str());
        }
    }
    ui->traceMetricTreeWidget->expandAll();
}
