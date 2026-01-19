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

#ifndef TRACEPLOTLINEMODEL_H
#define TRACEPLOTLINEMODEL_H

#include "generalinfo.h"

#include <QAbstractItemModel>
#include <QPushButton>
#include <memory>

class TracePlot;
class CustomLabelScaleDraw;
class QItemSelectionModel;

class AbstractTracePlotLineModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit AbstractTracePlotLineModel(const GeneralInfo& generalInfo, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QModelIndex
    index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    QItemSelectionModel* selectionModel() const;

    enum Role
    {
        TypeRole = Qt::UserRole + 1,
        CollapsedRole
    };
    Q_ENUM(Role)

    enum LineType
    {
        RequestLine,
        ResponseLine,
        CommandBusLine,
        RowCommandBusLine,
        ColumnCommandBusLine,
        DataBusLine,
        PseudoChannel0Line,
        PseudoChannel1Line,
        RankGroup,
        BankLine
    };
    Q_ENUM(LineType)

    struct Node
    {
        struct NodeData
        {
            NodeData() = default;

            NodeData(LineType type, const QString& label) : type(type), label(label) {}

            NodeData(LineType type, const QString& label, unsigned int rank) :
                type(type),
                label(label),
                rank(rank)
            {
            }

            NodeData(LineType type,
                     const QString& label,
                     unsigned int rank,
                     unsigned int group,
                     unsigned int bank) :
                type(type),
                label(label),
                rank(rank),
                group(group),
                bank(bank)
            {
            }

            LineType type;
            QString label;
            unsigned int yVal = 0;

            /**
             * Used to store the collapsed state in the traceplot.
             * The value has only an effect when the type is Type::RankGroup.
             */
            bool collapsed = true;

            /**
             * Only used when the type is Type::BankLine.
             * (Absolute numbering)
             */
            unsigned int rank = 0, group = 0, bank = 0;
        };

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

        static std::shared_ptr<Node> cloneNode(const Node* node, const Node* parent);

        NodeData data;

        const Node* parent = nullptr;
        std::vector<std::shared_ptr<Node>> children;
    };

protected:
    enum class CommandBusType
    {
        SingleCommandBus,
        RowColumnCommandBus
    };

    enum class DataBusType
    {
        LegacyMode,
        PseudoChannelMode
    };

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex& parent) override;
    bool canDropMimeData(const QMimeData* data,
                         Qt::DropAction action,
                         int row,
                         int column,
                         const QModelIndex& parent) const override;
    Qt::DropActions supportedDropActions() const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void addTopLevelNode(std::shared_ptr<Node>&& node);
    void createInitialNodes();
    std::shared_ptr<Node> createRankGroupNode(unsigned int rank) const;

    static QString getLabel(LineType type);
    QString getLabel(unsigned int rank) const;
    QString getLabel(unsigned int rank, unsigned int group, unsigned int bank) const;

    static CommandBusType getCommandBusType(const GeneralInfo& generalInfo);
    static DataBusType getDataBusType(const GeneralInfo& generalInfo);

    static constexpr auto TRACELINE_MIMETYPE = "application/x-tracelinedata";

    QItemSelectionModel* const internalSelectionModel;

    std::shared_ptr<Node> rootNode;

    const unsigned int numberOfRanks;
    const unsigned int groupsPerRank;
    const unsigned int banksPerGroup;
    const unsigned int banksPerRank;

    const CommandBusType commandBusType;
    const DataBusType dataBusType;
};

class AvailableTracePlotLineModel : public AbstractTracePlotLineModel
{
    Q_OBJECT

public:
    explicit AvailableTracePlotLineModel(const GeneralInfo& generalInfo, QObject* parent = nullptr);

public Q_SLOTS:
    void itemsDoubleClicked(const QModelIndex& index);

Q_SIGNALS:
    void returnPressed(const QModelIndexList& indexes);

protected:
    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool eventFilter(QObject* object, QEvent* event) override;
};

class SelectedTracePlotLineModel : public AbstractTracePlotLineModel
{
    Q_OBJECT

public:
    explicit SelectedTracePlotLineModel(const GeneralInfo& generalInfo, QObject* parent = nullptr);

    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    void recreateCollapseButtons(TracePlot* tracePlot, CustomLabelScaleDraw* customLabelScaleDraw);

    std::shared_ptr<Node> getClonedRootNode();
    void setRootNode(std::shared_ptr<Node> node);

public Q_SLOTS:
    void itemsDoubleClicked(const QModelIndex& index);

    void addIndexesFromAvailableModel(const QModelIndexList& indexes);

protected:
    bool eventFilter(QObject* object, QEvent* event) override;

    QVariant
    headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

private:
    std::vector<QPushButton*> collapseButtons;

    friend class TracePlotLineDataSource;
};

/*
 * Simple list that is the true source for the traceplot.
 */
class TracePlotLineDataSource : public QObject
{
    Q_OBJECT

public:
    using TracePlotLine = AbstractTracePlotLineModel::Node;

    explicit TracePlotLineDataSource(SelectedTracePlotLineModel* selectedModel,
                                     QObject* parent = nullptr);

    SelectedTracePlotLineModel* getSelectedModel() const { return selectedModel; };
    std::vector<std::shared_ptr<TracePlotLine>>& getTracePlotLines() { return entries; }

public Q_SLOTS:
    void updateModel();

Q_SIGNALS:
    void modelChanged();

private:
    SelectedTracePlotLineModel* selectedModel;

    std::vector<std::shared_ptr<TracePlotLine>> entries;
};

#endif // TRACEPLOTLINEMODEL_H
