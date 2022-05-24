/*
    SPDX-FileCopyrightText: Lieven Hey <lieven.hey@kdab.com>
    SPDX-FileCopyrightText: 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "codedelegate.h"

#include <QMouseEvent>
#include <QPainter>

#include "disassemblymodel.h"

namespace {
QColor backgroundColor(int line, bool isCurrent)
{
    int degrees = (line * 139) % 360;
    return QColor::fromHsv(degrees, 255, 255, isCurrent ? 120 : 40);
}
}

CodeDelegate::CodeDelegate(int lineNumberColumn, int highlightColumn, QObject* parent)
    : QStyledItemDelegate(parent)
{
    m_lineNumberColumn = lineNumberColumn;
    m_highlightColumn = highlightColumn;
}

CodeDelegate::~CodeDelegate() = default;

void CodeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const auto& brush = painter->brush();
    const auto& pen = painter->pen();

    painter->setPen(Qt::NoPen);

    if (option.features & QStyleOptionViewItem::Alternate) {
        // we must handle this ourselves as otherwise the custom background
        // would get painted over with the alternate background color
        painter->setBrush(option.palette.alternateBase());
        painter->drawRect(option.rect);
    }

    const auto model = index.model();

    int sourceLine = model->data(model->index(index.row(), m_lineNumberColumn), Qt::DisplayRole).toInt();

    if (sourceLine) {
        painter->setBrush(backgroundColor(
            sourceLine, model->data(model->index(index.row(), m_highlightColumn), Qt::DisplayRole).toBool()));
        painter->drawRect(option.rect);
    }

    painter->setBrush(brush);
    painter->setPen(pen);

    if (option.features & QStyleOptionViewItem::Alternate) {
        auto o = option;
        o.features &= ~QStyleOptionViewItem::Alternate;
        QStyledItemDelegate::paint(painter, o, index);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

DisassemblyDelegate::DisassemblyDelegate(int lineNumberColumn, int highlightColumn, QObject* parent)
    : CodeDelegate(lineNumberColumn, highlightColumn, parent)
{
}

DisassemblyDelegate::~DisassemblyDelegate() = default;

bool DisassemblyDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& /*option*/,
                                      const QModelIndex& index)
{
    if (event->type() != QMouseEvent::MouseButtonPress)
        return false;

    const QString functionName =
        model->data(model->index(index.row(), DisassemblyModel::LinkedFunctionName), Qt::DisplayRole).toString();
    if (functionName.isEmpty())
        return false;

    int funtionOffset =
        model->data(model->index(index.row(), DisassemblyModel::LinkedFunctionOffset), Qt::DisplayRole).toInt();
    emit gotoFunction(functionName, funtionOffset);

    return true;
}
