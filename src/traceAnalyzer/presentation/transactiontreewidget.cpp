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
 */

#include "transactiontreewidget.h"
#include "data/tracedb.h"
#include <QHeaderView>
#include <memory>
#include <vector>

using namespace std;

TransactionTreeWidget::TransactionTreeWidget(QWidget* parent) :
    QTreeWidget(parent),
    isInitialized(false)
{
    QObject::connect(
        this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(ContextMenuRequested(QPoint)));
    setContextMenuPolicy(Qt::CustomContextMenu);
    goToTransaction = new QAction("Move to", this);
}

void TransactionTreeWidget::init(TraceNavigator* _navigator)
{
    Q_ASSERT(isInitialized == false);
    isInitialized = true;

    this->navigator = _navigator;
    setColumnCount(3);
    setHeaderLabels(QStringList({"Transaction", "Value", "Value"}));
}

void TransactionTreeWidget::AppendTransaction(const shared_ptr<Transaction>& transaction)
{
    QTreeWidgetItem* node =
        new TransactionTreeItem(this, transaction, navigator->GeneralTraceInfo());
    addTopLevelItem(node);
}

void TransactionTreeWidget::ContextMenuRequested(QPoint point)
{
    if (selectedItems().count() > 0 &&
        selectedItems().at(0)->type() ==
            TransactionTreeWidget::TransactionTreeItem::transactionTreeItemType)
    {
        QMenu contextMenu;
        contextMenu.addActions({goToTransaction});
        QAction* selectedContextMenuItems = contextMenu.exec(mapToGlobal(point));

        if (selectedContextMenuItems)
        {
            TransactionTreeItem* item = dynamic_cast<TransactionTreeItem*>(selectedItems().at(0));
            navigator->selectTransaction(item->Id());
        }
    }
}

TransactionTreeWidget::TransactionTreeItem::TransactionTreeItem(
    QTreeWidget* parent,
    const shared_ptr<Transaction>& transaction,
    const GeneralInfo& generalInfo) :
    QTreeWidgetItem(parent, transactionTreeItemType)
{
    this->setText(0, QString::number(transaction->id));
    this->id = transaction->id;

    bool isControllerTransaction = (transaction->thread == generalInfo.controllerThread);

    auto* time = new QTreeWidgetItem({"Timespan"});
    AppendTimespan(time, transaction->span);
    this->addChild(time);
    if (!isControllerTransaction)
    {
        if (transaction->command == "R")
            this->addChild(new QTreeWidgetItem({"Command", "Read"}));
        else // if (transaction->command == "W")
            this->addChild(new QTreeWidgetItem({"Command", "Write"}));
    }
    this->addChild(
        new QTreeWidgetItem({"Length", prettyFormatTime(transaction->span.timeCovered())}));
    if (!isControllerTransaction)
        this->addChild(new QTreeWidgetItem(
            {"Address", QString("0x") + QString::number(transaction->address, 16)}));
    if (!isControllerTransaction)
        this->addChild(
            new QTreeWidgetItem({"Data Length", QString::number(transaction->dataLength)}));
    this->addChild(new QTreeWidgetItem({"Channel", QString::number(transaction->channel)}));
    if (!isControllerTransaction)
        this->addChild(new QTreeWidgetItem({"Thread", QString::number(transaction->thread)}));

    auto* phasesNode = new QTreeWidgetItem(this);
    phasesNode->setText(0, "Phases");
    phasesNode->addChild(new QTreeWidgetItem({"", "Begin", "End"}));

    for (const std::shared_ptr<Phase>& phase : transaction->Phases())
        AppendPhase(phasesNode, *phase);
}

void TransactionTreeWidget::TransactionTreeItem::AppendPhase(QTreeWidgetItem* parent,
                                                             const Phase& phase)
{
    auto* node = new QTreeWidgetItem(parent);
    node->setText(0, phase.Name() + QString(" [") + QString::number(phase.Id()) + QString("]"));

    AppendTimespan(node, phase.Span());

    auto addMapping = [node](std::string_view label, unsigned value)
    {
        auto* mappingNode = new QTreeWidgetItem(node);
        mappingNode->setText(0, label.data());
        mappingNode->setText(1, QString::number(value));
    };

    {
        if (static_cast<int>(phase.getRelevantAttributes() & RelevantAttributes::Rank))
            addMapping("Rank", phase.getRank());

        if (static_cast<int>(phase.getRelevantAttributes() & RelevantAttributes::BankGroup))
            addMapping("Bank Group", phase.getBankGroup());

        if (static_cast<int>(phase.getRelevantAttributes() & RelevantAttributes::Bank))
            addMapping("Bank", phase.getBank());

        if (static_cast<int>(phase.getRelevantAttributes() & RelevantAttributes::Row))
            addMapping("Row", phase.getRow());

        if (static_cast<int>(phase.getRelevantAttributes() & RelevantAttributes::Column))
            addMapping("Column", phase.getColumn());

        if (static_cast<int>(phase.getRelevantAttributes() & RelevantAttributes::BurstLength))
            addMapping("Burst Length", phase.getBurstLength());
    }
}

void TransactionTreeWidget::TransactionTreeItem::AppendTimespan(QTreeWidgetItem* parent,
                                                                const Timespan& timespan)
{
    parent->setText(1, prettyFormatTime(timespan.Begin()));
    parent->setText(2, prettyFormatTime(timespan.End()));
}
