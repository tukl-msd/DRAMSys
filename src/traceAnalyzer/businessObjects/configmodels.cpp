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

#include "configmodels.h"

#include <QJsonDocument>
#include <QJsonObject>

McConfigModel::McConfigModel(const TraceDB& traceFile, QObject* parent) :
    QAbstractTableModel(parent)
{
    QSqlDatabase db = traceFile.getDatabase();
    QString query = "SELECT MCconfig FROM GeneralInfo";
    QSqlQuery sqlQuery = db.exec(query);

    // The whole configuration is stored in a single cell.
    sqlQuery.next();
    QString mcConfigJson = sqlQuery.value(0).toString();

    addAdditionalInfos(traceFile.getGeneralInfo());
    parseJson(mcConfigJson);
}

void McConfigModel::parseJson(const QString& jsonString)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toUtf8());
    QJsonObject mcConfigJson = jsonDocument.object()["mcconfig"].toObject();

    for (auto key : mcConfigJson.keys())
    {
        QJsonValue currentValue = mcConfigJson.value(key);

        if (currentValue.isDouble())
        {
            entries.push_back({key, QString::number(currentValue.toDouble())});
        }
        else if (currentValue.isString())
        {
            entries.push_back({key, currentValue.toString()});
        }
        else if (currentValue.isBool())
        {
            entries.push_back({key, currentValue.toBool() ? "True" : "False"});
        }
    }
}

void McConfigModel::addAdditionalInfos(const GeneralInfo& generalInfo)
{
    auto addEntry = [this](const QString& key, const QString& value) {
        entries.push_back({key, value});
    };

    addEntry("Number of Transactions", QString::number(generalInfo.numberOfTransactions));
    addEntry("Clock period",
             QString::number(generalInfo.clkPeriod) + " " + generalInfo.unitOfTime.toLower());
    addEntry("Length of trace", prettyFormatTime(generalInfo.span.End()));
    addEntry("Window size", QString::number(generalInfo.windowSize));
}

int McConfigModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return entries.size();
}

int McConfigModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return 2;
}

QVariant McConfigModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    auto entry = entries.at(index.row());

    if (index.column() == 0)
        return entry.first;
    else if (index.column() == 1)
        return entry.second;

    return QVariant();
}

QVariant McConfigModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return "Field";
        case 1:
            return "Value";
        default:
            break;
        }
    }

    return QVariant();
}

MemSpecModel::MemSpecModel(const TraceDB& traceFile, QObject* parent) : QAbstractItemModel(parent)
{
    QSqlDatabase db = traceFile.getDatabase();
    QString query = "SELECT Memspec FROM GeneralInfo";
    QSqlQuery sqlQuery = db.exec(query);

    // The whole configuration is stored in a single cell.
    sqlQuery.next();
    QString memSpecJson = sqlQuery.value(0).toString();

    parseJson(memSpecJson);
}

int MemSpecModel::Node::getRow() const
{
    if (!parent)
        return 0;

    const auto& siblings = parent->children;
    const auto siblingsIt =
        std::find_if(siblings.begin(),
                     siblings.end(),
                     [this](const std::unique_ptr<Node>& node) { return node.get() == this; });

    Q_ASSERT(siblingsIt != siblings.end());

    return std::distance(siblings.begin(), siblingsIt);
}

void MemSpecModel::parseJson(const QString& jsonString)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toUtf8());
    QJsonObject memSpecJson = jsonDocument.object()["memspec"].toObject();

    std::function<void(const QJsonObject&, std::unique_ptr<Node>&)> addNodes;
    addNodes = [&addNodes](const QJsonObject& obj, std::unique_ptr<Node>& parentNode)
    {
        for (auto key : obj.keys())
        {
            QJsonValue currentValue = obj.value(key);

            QString value;
            if (currentValue.isDouble())
            {
                value = QString::number(currentValue.toDouble());
            }
            else if (currentValue.isString())
            {
                value = currentValue.toString();
            }
            else if (currentValue.isBool())
            {
                value = currentValue.toBool() ? "True" : "False";
            }

            std::unique_ptr<Node> node =
                std::unique_ptr<Node>(new Node({key, value}, parentNode.get()));
            addNodes(obj[key].toObject(), node);

            parentNode->children.push_back(std::move(node));
        }
    };

    addNodes(memSpecJson, rootNode);
}

int MemSpecModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    const Node* parentNode;

    if (!parent.isValid())
        parentNode = rootNode.get();
    else
        parentNode = static_cast<const Node*>(parent.internalPointer());

    return parentNode->childCount();
}

int MemSpecModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return 2;
}

QVariant MemSpecModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
        return QVariant();

    auto* node = static_cast<const Node*>(index.internalPointer());

    if (index.column() == 0)
        return QVariant(node->data.first);
    else
        return QVariant(node->data.second);
}

QVariant MemSpecModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case 0:
            return "Field";
        case 1:
            return "Value";
        default:
            break;
        }
    }

    return QVariant();
}

QModelIndex MemSpecModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const Node* parentNode;

    if (!parent.isValid())
        parentNode = rootNode.get();
    else
        parentNode = static_cast<const Node*>(parent.internalPointer());

    const Node* node = parentNode->children[row].get();

    return createIndex(row, column, const_cast<Node*>(node));
}

QModelIndex MemSpecModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    const Node* childNode = static_cast<const Node*>(index.internalPointer());
    const Node* parentNode = childNode->parent;

    if (!parentNode || parentNode == rootNode.get())
        return QModelIndex();

    return createIndex(parentNode->getRow(), 0, const_cast<Node*>(parentNode));
}
