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

#ifndef CONFIGMODELS_H
#define CONFIGMODELS_H

#include "data/tracedb.h"

#ifdef EXTENSION_ENABLED
#include "businessObjects/phases/dependencyinfos.h"
#endif

#include <QAbstractTableModel>
#include <utility>
#include <vector>

class McConfigModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit McConfigModel(const TraceDB& traceFile, QObject* parent = nullptr);

protected:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    /**
     * Parses the json file and adds json entries to the entries vector.
     * In case of failure, nothing is added and therefore the model
     * will stay empty.
     */
    void parseJson(const QString& jsonString);

    /**
     * Add additional infos about the tracefile which were
     * previously displayed in the fileDescription widget.
     */
    void addAdditionalInfos(const GeneralInfo& generalInfo);

    std::vector<std::pair<QString, QString>> entries;
};

class MemSpecModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit MemSpecModel(const TraceDB& traceFile, QObject* parent = nullptr);

protected:
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;

private:
    /**
     * Parses the json file and adds json entries to the entries vector.
     * In case of failure, nothing is added and therefore the model
     * will stay empty.
     */
    void parseJson(const QString& jsonString);

    struct Node
    {
        using NodeData = std::pair<QString, QString>;

        /**
         * Constructor only used for the root node that does not contain any data.
         */
        Node() = default;

        Node(NodeData data, const Node* parent) : data(data), parent(parent) {}

        /**
         * Gets the row relative to its parent.
         */
        int getRow() const;
        int childCount() const { return children.size(); }

        NodeData data;

        const Node* parent = nullptr;
        std::vector<std::unique_ptr<Node>> children;
    };

    std::unique_ptr<Node> rootNode = std::unique_ptr<Node>(new Node);
};

#endif // CONFIGMODELS_H
