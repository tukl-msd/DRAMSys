/*
 * Copyright (c) 2021, RPTU Kaiserslautern-Landau
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
 *    Derek Christ
 */

#ifndef COMMENTMODEL_H
#define COMMENTMODEL_H

#include "timespan.h"
#include "tracetime.h"

#include <QAbstractTableModel>
#include <utility>

class QAction;
class QItemSelectionModel;

class CommentModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CommentModel(QObject* parent = nullptr);

    struct Comment
    {
        traceTime time;
        QString text;
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    enum class Column
    {
        Time = 0,
        Comment,
        COLUMNCOUNT
    };

    void openContextMenu();

    QItemSelectionModel* selectionModel() const;

    void addComment(traceTime time);
    void addComment(traceTime time, QString text);

    void removeComment(traceTime time);
    void removeComment(const QModelIndex& index);

    const std::vector<Comment>& getComments() const;

    traceTime getTimeFromIndex(const QModelIndex& index) const;

    QModelIndex hoveredComment(Timespan timespan) const;

public Q_SLOTS:
    void rowDoubleClicked(const QModelIndex& index);

protected:
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    /**
     * The eventFilter is used to delete comments using the delete key.
     */
    bool eventFilter(QObject* object, QEvent* event) override;

Q_SIGNALS:
    void editTriggered(const QModelIndex& index);
    void gotoCommentTriggered(const QModelIndex& index);

private:
    void setUpActions();

    std::vector<Comment> comments;

    QAction* gotoAction;
    QAction* editAction;
    QAction* deleteAction;
    QAction* selectAllAction;
    QAction* deselectAllAction;

    QItemSelectionModel* internalSelectionModel;
};

#endif // COMMENTMODEL_H
