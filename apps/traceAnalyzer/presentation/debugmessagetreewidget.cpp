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

#include "debugmessagetreewidget.h"
#include <QGuiApplication>
#include <qtooltip.h>

using namespace std;

void DebugMessageTreeWidget::init(TraceNavigator* navigator, TracePlot* traceplot)
{
    Q_ASSERT(isInitialized == false);
    isInitialized = true;
    arrangeUiSettings();
    connect(navigator, SIGNAL(currentTraceTimeChanged()), this, SLOT(currentTraceTimeChanged()));
    connect(
        navigator, SIGNAL(selectedTransactionsChanged()), this, SLOT(selectedTransactionChanged()));
    this->traceplot = traceplot;
    this->navigator = navigator;
    currentTraceTimeChanged();
}

void DebugMessageTreeWidget::arrangeUiSettings()
{
    QFont font = QGuiApplication::font();
    font.setPointSize(10);
    this->setFont(font);
    setColumnCount(2);
    setHeaderLabels(QStringList({"Time", "Message"}));
}

void DebugMessageTreeWidget::selectedTransactionChanged()
{
    if (navigator->hasSelectedTransactions())
    {
        Timespan span = navigator->getSpanCoveredBySelectedTransaction();
        showDebugMessages(navigator->TraceFile().getDebugMessagesInTimespan(span));
    }
    else
    {
        showDebugMessages(
            navigator->TraceFile().getDebugMessagesInTimespan(traceplot->GetCurrentTimespan()));
    }
}

void DebugMessageTreeWidget::currentTraceTimeChanged()
{
    if (!navigator->hasSelectedTransactions())
        showDebugMessages(
            navigator->TraceFile().getDebugMessagesInTimespan(traceplot->GetCurrentTimespan()));
}

void DebugMessageTreeWidget::showDebugMessages(const vector<CommentModel::Comment>& comments)
{
    clear();
    if (comments.empty())
        return;

    traceTime currentTime = -1;
    for (const auto& comment : comments)
    {
        if (currentTime != comment.time)
        {
            addTopLevelItem(new QTreeWidgetItem(
                {prettyFormatTime(comment.time), formatDebugMessage(comment.text)}));
            currentTime = comment.time;
        }
        else
        {
            addTopLevelItem(new QTreeWidgetItem({"", formatDebugMessage(comment.text)}));
        }
    }

    this->resizeColumnToContents(0);
    this->scrollToTop();
}

QString DebugMessageTreeWidget::formatDebugMessage(const QString& message)
{
    QString formattedMessage = message;
    formattedMessage.replace(hexAdressMatcher, "");
    formattedMessage.replace(timeAnnotationMatcher, "");
    formattedMessage.replace("\t", " ");
    return formattedMessage;
}

void DebugMessageTreeWidget::mousePressEvent(QMouseEvent* event)
{
    QTreeWidgetItem* itemUnderCursor = itemAt(event->pos());
    if (itemUnderCursor != NULL)
    {
        QToolTip::showText(this->mapToGlobal(event->pos()), itemUnderCursor->text(1));
    }
}
