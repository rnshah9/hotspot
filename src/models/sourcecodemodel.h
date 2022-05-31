
/*
    SPDX-FileCopyrightText: Lieven Hey <lieven.hey@kdab.com>
    SPDX-FileCopyrightText: 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "data.h"
#include "disassemblyoutput.h"

#include <QAbstractTableModel>

class SourceCodeModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SourceCodeModel(QObject* parent = nullptr);
    ~SourceCodeModel();

    void setDisassembly(const DisassemblyOutput& disassemblyOutput);

    void clear();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int lineForIndex(const QModelIndex& index);

    enum Columns
    {
        SourceCodeColumn,
        SourceCodeLineNumber,
        Highlight,
        COLUMN_COUNT
    };

    enum CustomRoles
    {
        CostRole = Qt::UserRole,
        TotalCostRole
    };

public slots:
    void updateHighlighting(int line);
    void setCallerCalleeResults(const Data::CallerCalleeResults& results);

private:
    QSet<int> m_validLineNumbers;
    Data::CallerCalleeResults m_callerCalleeResults;
    Data::Costs m_costs;
    int m_numTypes = 0;
    QStringList m_sourceCode;
    int m_lineOffset = 0;
    int m_highlightLine = 0;
};
