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

#include "traceplotlinemodel.h"
#include "../presentation/traceplot.h"
#include "../presentation/util/customlabelscaledraw.h"

#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMimeData>

AbstractTracePlotLineModel::AbstractTracePlotLineModel(const GeneralInfo& generalInfo,
                                                       QObject* parent) :
    QAbstractItemModel(parent),
    internalSelectionModel(new QItemSelectionModel(this, this)),
    rootNode(std::make_shared<Node>()),
    numberOfRanks(generalInfo.numberOfRanks),
    groupsPerRank(generalInfo.groupsPerRank),
    banksPerGroup(generalInfo.banksPerGroup),
    banksPerRank(generalInfo.banksPerRank),
    commandBusType(getCommandBusType(generalInfo)),
    dataBusType(getDataBusType(generalInfo))
{
    createInitialNodes();
}

AvailableTracePlotLineModel::AvailableTracePlotLineModel(const GeneralInfo& generalInfo,
                                                         QObject* parent) :
    AbstractTracePlotLineModel(generalInfo, parent)
{
}

SelectedTracePlotLineModel::SelectedTracePlotLineModel(const GeneralInfo& generalInfo,
                                                       QObject* parent) :
    AbstractTracePlotLineModel(generalInfo, parent)
{
}

void AbstractTracePlotLineModel::addTopLevelNode(std::shared_ptr<Node>&& node)
{
    rootNode->children.push_back(std::move(node));
}

void AbstractTracePlotLineModel::createInitialNodes()
{
    addTopLevelNode(std::unique_ptr<Node>(
        new Node({LineType::RequestLine, getLabel(LineType::RequestLine)}, rootNode.get())));
    addTopLevelNode(std::unique_ptr<Node>(
        new Node({LineType::ResponseLine, getLabel(LineType::ResponseLine)}, rootNode.get())));

    for (unsigned int rank = 0; rank < numberOfRanks; rank++)
        addTopLevelNode(createRankGroupNode(rank));

    if (commandBusType == CommandBusType::SingleCommandBus)
    {
        addTopLevelNode(std::unique_ptr<Node>(new Node(
            {LineType::CommandBusLine, getLabel(LineType::CommandBusLine)}, rootNode.get())));
    }
    else // commandBusType == CommandBusType::RowColumnCommandBus
    {
        addTopLevelNode(std::unique_ptr<Node>(new Node(
            {LineType::RowCommandBusLine, getLabel(LineType::RowCommandBusLine)}, rootNode.get())));
        addTopLevelNode(std::unique_ptr<Node>(
            new Node({LineType::ColumnCommandBusLine, getLabel(LineType::ColumnCommandBusLine)},
                     rootNode.get())));
    }

    if (dataBusType == DataBusType::LegacyMode)
    {
        addTopLevelNode(std::unique_ptr<Node>(
            new Node({LineType::DataBusLine, getLabel(LineType::DataBusLine)}, rootNode.get())));
    }
    else // dataBusType == DataBusType::PseudoChannelMode
    {
        addTopLevelNode(std::unique_ptr<Node>(
            new Node({LineType::PseudoChannel0Line, getLabel(LineType::PseudoChannel0Line)},
                     rootNode.get())));
        addTopLevelNode(std::unique_ptr<Node>(
            new Node({LineType::PseudoChannel1Line, getLabel(LineType::PseudoChannel1Line)},
                     rootNode.get())));
    }
}

std::shared_ptr<AbstractTracePlotLineModel::Node>
AbstractTracePlotLineModel::createRankGroupNode(unsigned int rank) const
{
    auto rankGroup = std::unique_ptr<Node>(
        new Node({LineType::RankGroup, getLabel(rank), rank}, rootNode.get()));

    for (unsigned int group = 0; group < groupsPerRank; group++)
    {
        for (unsigned int bank = 0; bank < banksPerGroup; bank++)
        {
            unsigned int absoluteRank = rank;
            unsigned int absoluteGroup = group + rank * groupsPerRank;
            unsigned int absoluteBank = bank + rank * banksPerRank + group * banksPerGroup;

            auto bankLine = std::unique_ptr<Node>(new Node({LineType::BankLine,
                                                            getLabel(rank, group, bank),
                                                            absoluteRank,
                                                            absoluteGroup,
                                                            absoluteBank},
                                                           rankGroup.get()));

            rankGroup->children.push_back(std::move(bankLine));
        }
    }

    return rankGroup;
}

int AbstractTracePlotLineModel::rowCount(const QModelIndex& parent) const
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

int AbstractTracePlotLineModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)

    return 1;
}

QVariant AbstractTracePlotLineModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto* node = static_cast<const Node*>(index.internalPointer());

    switch (role)
    {
    case Qt::DisplayRole:
        return node->data.label;

    case Role::TypeRole:
        return QVariant::fromValue(node->data.type);

    case Role::CollapsedRole:
        return node->data.collapsed;
    }

    return QVariant();
}

bool SelectedTracePlotLineModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
        return false;

    auto* node = static_cast<Node*>(index.internalPointer());

    switch (role)
    {
    case Role::CollapsedRole:
        node->data.collapsed = value.toBool();
        emit dataChanged(index, index, {role});
        return true;

    case Qt::DisplayRole:
    case Role::TypeRole:
        // Not allowed
        return false;
    }

    return false;
}

QVariant
AvailableTracePlotLineModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal && section == 0)
        return "Available Items";

    return QVariant();
}

QVariant
SelectedTracePlotLineModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal && section == 0)
    {
        return "Selected Items";
    }

    return QVariant();
}

QModelIndex AbstractTracePlotLineModel::index(int row, int column, const QModelIndex& parent) const
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

QModelIndex AbstractTracePlotLineModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    const Node* childNode = static_cast<const Node*>(index.internalPointer());
    const Node* parentNode = childNode->parent;

    if (!parentNode || parentNode == rootNode.get())
        return QModelIndex();

    return createIndex(parentNode->getRow(), 0, const_cast<Node*>(parentNode));
}

void SelectedTracePlotLineModel::recreateCollapseButtons(TracePlot* tracePlot,
                                                         CustomLabelScaleDraw* customLabelScaleDraw)
{
    // Remove old buttons
    for (auto button : collapseButtons)
    {
        button->hide();
        button->deleteLater();
    }

    collapseButtons.clear();

    for (const auto& node : rootNode->children)
    {
        if (node->data.type != LineType::RankGroup)
            continue;

        QPushButton* collapseButton = new QPushButton(tracePlot);

        unsigned int yVal = [node]()
        {
            if (node->data.collapsed)
                return node->data.yVal;
            else
                return node->children.at(0)->data.yVal;
        }();
        bool isCollapsed = node->data.collapsed;
        QModelIndex nodeIndex = index(node->getRow(), 0);

        auto repositionButton = [=]()
        {
            QPointF point = tracePlot->axisScaleDraw(QwtPlot::yLeft)->labelPosition(yVal);
            collapseButton->setGeometry(point.x(), point.y() - 4, 25, 25);
        };
        repositionButton();

        auto updateLabel = [=]() { collapseButton->setText(isCollapsed ? "+" : "-"); };
        updateLabel();

        auto toggleCollapsed = [=]()
        {
            setData(nodeIndex, !isCollapsed, Role::CollapsedRole);

            recreateCollapseButtons(tracePlot, customLabelScaleDraw);
        };

        // Important: The context of the connection is `collapseButton` as it should be disconnected
        // when the button ceases to exist.
        connect(customLabelScaleDraw,
                &CustomLabelScaleDraw::scaleRedraw,
                collapseButton,
                repositionButton);
        connect(collapseButton, &QPushButton::pressed, this, toggleCollapsed);
        connect(
            collapseButton, &QPushButton::pressed, tracePlot, &TracePlot::recreateCollapseButtons);

        collapseButton->show();
        collapseButtons.push_back(collapseButton);
    }
}

int AbstractTracePlotLineModel::Node::getRow() const
{
    if (!parent)
        return 0;

    const auto& siblings = parent->children;
    const auto siblingsIt =
        std::find_if(siblings.begin(),
                     siblings.end(),
                     [this](const std::shared_ptr<Node>& node) { return node.get() == this; });

    Q_ASSERT(siblingsIt != siblings.end());

    return std::distance(siblings.begin(), siblingsIt);
}

std::shared_ptr<AbstractTracePlotLineModel::Node>
AbstractTracePlotLineModel::Node::cloneNode(const Node* node, const Node* parent)
{
    std::shared_ptr<Node> clonedNode = std::make_shared<Node>(node->data, parent);

    for (const auto& child : node->children)
        clonedNode->children.push_back(cloneNode(child.get(), clonedNode.get()));

    return clonedNode;
}

QString AbstractTracePlotLineModel::getLabel(LineType type)
{
    switch (type)
    {
    case LineType::RequestLine:
        return "REQ";
    case LineType::ResponseLine:
        return "RESP";
    case LineType::CommandBusLine:
        return "Command Bus";
    case LineType::RowCommandBusLine:
        return "Command Bus [R]";
    case LineType::ColumnCommandBusLine:
        return "Command Bus [C]";
    case LineType::DataBusLine:
        return "Data Bus";
    case LineType::PseudoChannel0Line:
        return "Data Bus [PC0]";
    case LineType::PseudoChannel1Line:
        return "Data Bus [PC1]";
    default:
        return "";
    }
}

QString AbstractTracePlotLineModel::getLabel(unsigned int rank) const
{
    std::string_view rankLabel = dataBusType == DataBusType::LegacyMode ? "RA" : "PC";
    return rankLabel.data() + QString::number(rank);
}

QString
AbstractTracePlotLineModel::getLabel(unsigned int rank, unsigned int group, unsigned int bank) const
{
    std::string_view rankLabel = dataBusType == DataBusType::LegacyMode ? "RA" : "PC";
    return rankLabel.data() + QString::number(rank) + " BG" + QString::number(group) + " BA" +
           QString::number(bank);
}

AbstractTracePlotLineModel::CommandBusType
AbstractTracePlotLineModel::getCommandBusType(const GeneralInfo& generalInfo)
{
    if (generalInfo.rowColumnCommandBus)
        return CommandBusType::RowColumnCommandBus;
    else
        return CommandBusType::SingleCommandBus;
}

AbstractTracePlotLineModel::DataBusType
AbstractTracePlotLineModel::getDataBusType(const GeneralInfo& generalInfo)
{
    if (generalInfo.pseudoChannelMode)
        return DataBusType::PseudoChannelMode;
    else
        return DataBusType::LegacyMode;
}

bool SelectedTracePlotLineModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (parent != QModelIndex())
        return false;

    // Note: beginRemoveRows requires [first, last], but erase requires [first, last)
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    rootNode->children.erase(rootNode->children.begin() + row,
                             rootNode->children.begin() + row + count);
    endRemoveRows();

    return true;
}

void AvailableTracePlotLineModel::itemsDoubleClicked(const QModelIndex& index)
{
    QModelIndexList indexList({index});

    emit returnPressed(indexList);
}

void SelectedTracePlotLineModel::itemsDoubleClicked(const QModelIndex& index)
{
    if (index.parent() != QModelIndex())
        return;

    removeRow(index.row(), QModelIndex());
}

bool AvailableTracePlotLineModel::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object)

    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Return)
        {
            const QModelIndexList indexes = internalSelectionModel->selectedRows();

            emit returnPressed(indexes);
        }
        else
        {
            return false;
        }
    }

    return false;
}

bool SelectedTracePlotLineModel::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object)

    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Delete)
        {
            // Note: This implementation requires the selection to be contiguous

            const QModelIndexList indexes = internalSelectionModel->selectedRows();

            if (indexes.count() == 0)
                return true;

            for (const auto& index : indexes)
            {
                // Only remove toplevel indexes
                if (index.parent() != QModelIndex())
                    return true;
            }

            removeRows(indexes.at(0).row(), indexes.size(), QModelIndex());
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}

void SelectedTracePlotLineModel::addIndexesFromAvailableModel(const QModelIndexList& indexes)
{
    for (const auto& index : indexes)
    {
        auto node = static_cast<const Node*>(index.internalPointer());
        auto clonedNode = Node::cloneNode(node, rootNode.get());

        beginInsertRows(QModelIndex(), rootNode->children.size(), rootNode->children.size());
        addTopLevelNode(std::move(clonedNode));
        endInsertRows();
    }
}

QItemSelectionModel* AbstractTracePlotLineModel::selectionModel() const
{
    return internalSelectionModel;
}

QStringList AbstractTracePlotLineModel::mimeTypes() const
{
    QStringList types = QAbstractItemModel::mimeTypes();
    types << TRACELINE_MIMETYPE;

    return types;
}

QMimeData* AbstractTracePlotLineModel::mimeData(const QModelIndexList& indexes) const
{
    QByteArray traceLineData;
    QDataStream dataStream(&traceLineData, QIODevice::WriteOnly);

    for (const auto& index : indexes)
    {
        const Node* node = static_cast<const Node*>(index.internalPointer());

        dataStream << node->data.type << node->data.label << node->data.rank << node->data.group
                   << node->data.bank << node->data.collapsed;
    }

    QMimeData* mimeData = new QMimeData;
    mimeData->setData(TRACELINE_MIMETYPE, traceLineData);

    return mimeData;
}

bool AbstractTracePlotLineModel::canDropMimeData(const QMimeData* data,
                                                 Qt::DropAction action,
                                                 int row,
                                                 int column,
                                                 const QModelIndex& parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(parent);

    if (!data->hasFormat(TRACELINE_MIMETYPE))
        return false;

    if (column > 0)
        return false;

    return true;
}

bool AbstractTracePlotLineModel::dropMimeData(
    const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    bool dropHandled = false;

    int beginRow;

    if (row != -1)
        beginRow = row;
    else
        beginRow = rowCount(QModelIndex());

    if (action == Qt::CopyAction || action == Qt::MoveAction)
    {
        dropHandled = true;

        QByteArray traceLineData = data->data(TRACELINE_MIMETYPE);
        QDataStream dataStream(&traceLineData, QIODevice::ReadOnly);

        std::vector<std::shared_ptr<Node>> droppedNodes;

        while (!dataStream.atEnd())
        {
            LineType type;
            QString label;
            unsigned int rank, group, bank;
            bool isCollapsed;

            dataStream >> type >> label >> rank >> group >> bank >> isCollapsed;

            std::shared_ptr<Node> node;

            if (type == LineType::BankLine)
                node = std::make_shared<Node>(Node::NodeData{type, label, rank, group, bank},
                                              rootNode.get());
            else if (type == LineType::RankGroup)
                node = createRankGroupNode(rank);
            else
                node = std::make_shared<Node>(Node::NodeData{type, label}, rootNode.get());

            if (node->data.type == RankGroup)
                node->data.collapsed = isCollapsed;

            droppedNodes.push_back(std::move(node));
        }

        // Note: beginRemoveRows requires [first, last]
        beginInsertRows(QModelIndex(), beginRow, beginRow + droppedNodes.size() - 1);
        rootNode->children.insert(rootNode->children.begin() + beginRow,
                                  std::make_move_iterator(droppedNodes.begin()),
                                  std::make_move_iterator(droppedNodes.end()));
        endInsertRows();
    }
    else
    {
        dropHandled = QAbstractItemModel::dropMimeData(data, action, row, column, parent);
    }

    return dropHandled;
}

Qt::DropActions AbstractTracePlotLineModel::supportedDropActions() const
{
    return (Qt::MoveAction | Qt::CopyAction);
}

Qt::ItemFlags AbstractTracePlotLineModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (index.isValid())
        return Qt::ItemIsDragEnabled | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}

std::shared_ptr<AbstractTracePlotLineModel::Node> SelectedTracePlotLineModel::getClonedRootNode()
{
    return Node::cloneNode(rootNode.get(), nullptr);
}

void SelectedTracePlotLineModel::setRootNode(std::shared_ptr<AbstractTracePlotLineModel::Node> node)
{
    removeRows(0, rootNode->childCount(), QModelIndex());

    beginInsertRows(QModelIndex(), 0, node->childCount() - 1);
    rootNode = std::move(node);
    endInsertRows();
}

TracePlotLineDataSource::TracePlotLineDataSource(SelectedTracePlotLineModel* selectedModel,
                                                 QObject* parent) :
    selectedModel(selectedModel)
{
    Q_UNUSED(parent)

    updateModel();
}

void TracePlotLineDataSource::updateModel()
{
    entries.clear();

    std::function<void(std::shared_ptr<AbstractTracePlotLineModel::Node> & parent)> addNodes;
    addNodes = [=, &addNodes](std::shared_ptr<AbstractTracePlotLineModel::Node>& parent)
    {
        for (auto& childNode : parent->children)
        {
            if (childNode->data.type == AbstractTracePlotLineModel::RankGroup &&
                !childNode->data.collapsed)
            {
                addNodes(childNode);
                continue; // Don't add the parent node itself when not collapsed.
            }
            entries.push_back(childNode);
        }
    };

    addNodes(selectedModel->rootNode);

    emit modelChanged();
}
