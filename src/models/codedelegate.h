/*
    SPDX-FileCopyrightText: Lieven Hey <lieven.hey@kdab.com>
    SPDX-FileCopyrightText: 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QStyledItemDelegate>

class CodeDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    CodeDelegate(int lineNumberColumn, int highlightColumn, QObject* parent = nullptr);
    ~CodeDelegate();

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int m_lineNumberColumn;
    int m_highlightColumn;
};

class DisassemblyDelegate : public CodeDelegate
{
    Q_OBJECT
public:
    DisassemblyDelegate(int lineNumberColumn, int highlightColumn, QObject* parent = nullptr);
    ~DisassemblyDelegate();

    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

signals:
    void gotoFunction(const QString& function, int offset);
};
