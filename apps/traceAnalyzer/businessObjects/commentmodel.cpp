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

#include "commentmodel.h"

#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMenu>

CommentModel::CommentModel(QObject* parent) :
    QAbstractTableModel(parent),
    gotoAction(new QAction("Goto comment", this)),
    editAction(new QAction("Edit comment", this)),
    deleteAction(new QAction("Delete comment", this)),
    selectAllAction(new QAction("Select all comments", this)),
    deselectAllAction(new QAction("Deselect all comments", this)),
    internalSelectionModel(new QItemSelectionModel(this, this))
{
    setUpActions();
}

void CommentModel::setUpActions()
{
    QObject::connect(gotoAction,
                     &QAction::triggered,
                     this,
                     [=]()
                     {
                         const QModelIndexList indexes = internalSelectionModel->selectedRows();
                         for (const QModelIndex& currentIndex : indexes)
                         {
                             emit gotoCommentTriggered(currentIndex);
                         }
                     });

    QObject::connect(editAction,
                     &QAction::triggered,
                     this,
                     [=]()
                     {
                         const QModelIndexList indexes = internalSelectionModel->selectedRows();
                         for (const QModelIndex& currentIndex : indexes)
                             emit editTriggered(
                                 index(currentIndex.row(), static_cast<int>(Column::Comment)));
                     });

    QObject::connect(deleteAction,
                     &QAction::triggered,
                     this,
                     [=]()
                     {
                         const QModelIndexList indexes = internalSelectionModel->selectedRows();
                         for (const QModelIndex& currentIndex : indexes)
                             removeComment(currentIndex);
                     });

    QObject::connect(selectAllAction,
                     &QAction::triggered,
                     this,
                     [=]()
                     {
                         QModelIndex topLeft = index(0, 0);
                         QModelIndex bottomRight = index(rowCount() - 1, columnCount() - 1);
                         internalSelectionModel->select(QItemSelection(topLeft, bottomRight),
                                                        QItemSelectionModel::Select |
                                                            QItemSelectionModel::Rows);
                     });

    QObject::connect(deselectAllAction,
                     &QAction::triggered,
                     internalSelectionModel,
                     &QItemSelectionModel::clearSelection);
}

int CommentModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return comments.size();
}

int CommentModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return static_cast<int>(Column::COLUMNCOUNT);
}

QVariant CommentModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // Qt::UserRole is used to get the raw time without pretty formatting.
    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::UserRole)
        return QVariant();

    const Comment& comment = comments.at(index.row());

    if (role == Qt::UserRole && static_cast<Column>(index.column()) == Column::Time)
        return QVariant(comment.time);

    switch (static_cast<Column>(index.column()))
    {
    case Column::Time:
        return QVariant(prettyFormatTime(comment.time));
    case Column::Comment:
        return QVariant(comment.text);
    default:
        break;
    }

    return QVariant();
}

bool CommentModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    if (role != Qt::EditRole)
        return false;

    if (static_cast<Column>(index.column()) != Column::Comment)
        return false;

    QString newText = value.toString();

    Comment& comment = comments.at(index.row());
    comment.text = newText;

    emit dataChanged(index, index);
    return true;
}

QVariant CommentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (static_cast<Column>(section))
        {
        case Column::Time:
            return "Time";
        case Column::Comment:
            return "Comment";
        default:
            break;
        }
    }

    return QVariant();
}

Qt::ItemFlags CommentModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    if (index.isValid() && index.column() == static_cast<int>(Column::Comment))
        flags |= Qt::ItemIsEditable;

    return flags;
}

void CommentModel::openContextMenu()
{
    if (!internalSelectionModel->hasSelection())
        return;

    QMenu* menu = new QMenu();
    menu->addActions({gotoAction, editAction, deleteAction});
    menu->addSeparator();
    menu->addActions({selectAllAction, deselectAllAction});

    QObject::connect(menu, &QMenu::aboutToHide, [=]() { menu->deleteLater(); });

    menu->popup(QCursor::pos());
}

QItemSelectionModel* CommentModel::selectionModel() const
{
    return internalSelectionModel;
}

void CommentModel::addComment(traceTime time)
{
    auto commentIt = std::find_if(comments.rbegin(),
                                  comments.rend(),
                                  [time](const Comment& comment) { return comment.time <= time; });

    int insertIndex = std::distance(comments.begin(), commentIt.base());

    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    comments.insert(comments.begin() + insertIndex, {time, "Enter comment text..."});
    endInsertRows();

    internalSelectionModel->setCurrentIndex(
        index(insertIndex, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    emit editTriggered(index(insertIndex, 1));
}

void CommentModel::addComment(traceTime time, QString text)
{
    auto commentIt = std::find_if(comments.rbegin(),
                                  comments.rend(),
                                  [time](const Comment& comment) { return comment.time <= time; });

    int insertIndex = std::distance(comments.begin(), commentIt.base());

    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    comments.insert(comments.begin() + insertIndex, {time, text});
    endInsertRows();
}

void CommentModel::removeComment(traceTime time)
{
    auto commentIt = std::find_if(comments.begin(),
                                  comments.end(),
                                  [time](const Comment& comment) { return comment.time == time; });

    if (commentIt == comments.end())
        return;

    int removeIndex = std::distance(comments.begin(), commentIt);

    beginRemoveRows(QModelIndex(), removeIndex, removeIndex);
    comments.erase(commentIt);
    endRemoveRows();
}

void CommentModel::removeComment(const QModelIndex& index)
{
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    comments.erase(comments.begin() + index.row());
    endRemoveRows();
}

const std::vector<CommentModel::Comment>& CommentModel::getComments() const
{
    return comments;
}

traceTime CommentModel::getTimeFromIndex(const QModelIndex& index) const
{
    Q_ASSERT(index.row() >= 0 && comments.size() > static_cast<unsigned>(index.row()));

    return comments[index.row()].time;
}

QModelIndex CommentModel::hoveredComment(Timespan timespan) const
{
    auto commentIt =
        std::find_if(comments.begin(),
                     comments.end(),
                     [timespan](const Comment& comment)
                     { return timespan.Begin() < comment.time && comment.time < timespan.End(); });

    if (commentIt == comments.end())
        return QModelIndex();

    int commentIndex = std::distance(comments.begin(), commentIt);

    return index(commentIndex, 0);
}

bool CommentModel::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object)

    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete)
        {
            const QModelIndexList indexes = internalSelectionModel->selectedRows();
            for (const QModelIndex& currentIndex : indexes)
            {
                removeComment(currentIndex);
            }
        }
    }

    return false;
}

void CommentModel::rowDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    if (static_cast<Column>(index.column()) == Column::Time)
        Q_EMIT gotoCommentTriggered(index);
}
