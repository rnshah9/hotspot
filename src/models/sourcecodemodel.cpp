/*
    SPDX-FileCopyrightText: Lieven Hey <lieven.hey@kdab.com>
    SPDX-FileCopyrightText: 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sourcecodemodel.h"
#include <QFile>

SourceCodeModel::SourceCodeModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

SourceCodeModel::~SourceCodeModel() = default;

void SourceCodeModel::clear()
{
    beginResetModel();
    m_sourceCode.clear();
    endResetModel();
}

void SourceCodeModel::setDisassembly(const DisassemblyOutput& disassemblyOutput)
{
    if (disassemblyOutput.sourceFileName.isEmpty())
        return;

    QFile file(disassemblyOutput.sourceFileName);

    if (!file.open(QIODevice::ReadOnly))
        return;

    beginResetModel();

    const auto lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));

    int maxIndex = 0;
    int minIndex = INT_MAX;

    m_validLineNumbers.clear();

    for (const auto& line : disassemblyOutput.disassemblyLines) {
        if (line.sourceCodeLine == 0)
            continue;

        if (line.sourceCodeLine > maxIndex) {
            maxIndex = line.sourceCodeLine;
        }
        if (line.sourceCodeLine < minIndex) {
            minIndex = line.sourceCodeLine;
        }
        m_validLineNumbers.insert(line.sourceCodeLine);
    }

    m_sourceCode.clear();

    // the start / end of a function is defined by { }
    // the following code tries to find ( to include the function signature in the source code
    if (!lines[minIndex - 1].contains(QLatin1Char('('))) {
        for (int i = minIndex - 1; i > minIndex - 6; i--) {
            if (i < 0)
                break;
            if (lines[minIndex - 1].contains(QLatin1Char('(')))
                break;
            minIndex--;
        }
    }

    for (int i = minIndex - 1; i < maxIndex + 1; i++) {
        m_sourceCode.push_back(lines[i]);
    }

    m_lineOffset = minIndex;

    endResetModel();
}

QVariant SourceCodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= COLUMN_COUNT)
        return {};
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};

    if (section == SourceCodeColumn)
        return tr("Source Code");

    if (section == SourceCodeLineNumber)
        return tr("Source Code Line Number");

    if (section == Highlight) {
        return tr("Highlight");
    }

    return {};
}

QVariant SourceCodeModel::data(const QModelIndex& index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent()))
        return {};

    if (index.row() > m_sourceCode.count() || index.row() < 0)
        return {};

    const auto& line = m_sourceCode.at(index.row());

    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        if (index.column() == SourceCodeColumn)
            return line;

        if (index.column() == SourceCodeLineNumber) {
            int line = index.row() + m_lineOffset;
            if (m_validLineNumbers.contains(line))
                return line;
            return 0;
        }

        if (index.column() == Highlight) {
            return index.row() + m_lineOffset == m_highlightLine;
        }
    }
    return {};
}

int SourceCodeModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : COLUMN_COUNT;
}

int SourceCodeModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_sourceCode.count();
}

void SourceCodeModel::updateHighlighting(int line)
{
    m_highlightLine = line;
    emit dataChanged(createIndex(0, Columns::SourceCodeColumn), createIndex(rowCount(), Columns::SourceCodeColumn));
}

int SourceCodeModel::lineForIndex(const QModelIndex& index)
{
    return index.row() + m_lineOffset;
}
