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
 *    Felipe S. Prado
 *    Matthias Jung
 */

#include "selectmetrics.h"
#include "ui_selectmetrics.h"
#include <QMessageBox>

SelectMetrics::SelectMetrics(QWidget* parent) : QDialog(parent), ui(new Ui::SelectMetrics)
{
    ui->setupUi(this);

    layout = new QVBoxLayout();

    ui->scrollAreaWidgetContents->setLayout(layout);

    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void SelectMetrics::on_okButton_clicked()
{
    if (isThereAnyMetricSelected())
    {
        close();
        Q_EMIT getSelectedMetrics();
    }
    else
    {
        QMessageBox::warning(this, "Warning", "Please select at least one metric");
    }
}

void SelectMetrics::on_clearAllButton_clicked()
{
    for (QCheckBox* checkBox : metrics)
    {
        checkBox->setChecked(false);
    }
}

void SelectMetrics::on_selectAllButton_clicked()
{
    for (QCheckBox* checkBox : metrics)
    {
        checkBox->setChecked(true);
    }
}

bool SelectMetrics::isThereAnyMetricSelected()
{
    for (QCheckBox* checkBox : metrics)
    {
        if (checkBox->isChecked())
            return true;
    }
    return false;
}

void SelectMetrics::setMetrics(std::vector<std::string> metrics)
{
    if (this->metrics.size() != metrics.size())
    {
        for (QCheckBox* checkBox : this->metrics)
        {
            layout->removeWidget(checkBox);
        }

        this->metrics.clear();

        for (std::string metric : metrics)
        {
            QCheckBox* checkBox = new QCheckBox();
            checkBox->setObjectName(QString::fromStdString(metric));
            checkBox->setCheckable(true);
            checkBox->setChecked(true);

            checkBox->setGeometry(10, 25, 100, 17);
            checkBox->setText(QString::fromStdString(metric));
            this->metrics.push_back(checkBox);
            layout->addWidget(checkBox, Qt::AlignLeft);
        }
    }
}

SelectMetrics::~SelectMetrics()
{
    delete ui;
}
