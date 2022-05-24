/*
    SPDX-FileCopyrightText: Darya Knysh <d.knysh@nips.ru>
    SPDX-FileCopyrightText: Milian Wolff <milian.wolff@kdab.com>
    SPDX-FileCopyrightText: 2016-2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "resultsdisassemblypage.h"
#include "ui_resultsdisassemblypage.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTextStream>
#include <QStandardItemModel>
#include <QStandardPaths>

#include <KRecursiveFilterProxyModel>

#include "parsers/perf/perfparser.h"
#include "resultsutil.h"

#include "data.h"
#include "models/codedelegate.h"
#include "models/costdelegate.h"
#include "models/disassemblymodel.h"
#include "models/hashmodel.h"
#include "models/sourcecodemodel.h"
#include "models/topproxy.h"
#include "models/treemodel.h"

#include <QDebug>

ResultsDisassemblyPage::ResultsDisassemblyPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ResultsDisassemblyPage)
    , m_disassemblyModel(new DisassemblyModel(this))
    , m_sourceCodeModel(new SourceCodeModel(this))
    , m_costDelegate(new CostDelegate(DisassemblyModel::CostRole, DisassemblyModel::TotalCostRole, this))
    , m_disassemblyDelegate(
          new DisassemblyDelegate(DisassemblyModel::SourceCodeLine, DisassemblyModel::Highlight, this))
    , m_sourceCodeDelegate(new CodeDelegate(SourceCodeModel::SourceCodeLineNumber, SourceCodeModel::Highlight, this))
{
    ui->setupUi(this);
    ui->assemblyView->setModel(m_disassemblyModel);
    ui->assemblyView->setMouseTracking(true);
    ui->sourceCodeView->setModel(m_sourceCodeModel);
    ui->sourceCodeView->setMouseTracking(true);

    auto updateFromDisassembly = [this](const QModelIndex& index) {
        int line = m_disassemblyModel->lineForIndex(index);
        m_disassemblyModel->updateHighlighting(line);
        m_sourceCodeModel->updateHighlighting(line);
    };

    auto updateFromSource = [this](const QModelIndex& index) {
        int line = m_sourceCodeModel->lineForIndex(index);
        m_disassemblyModel->updateHighlighting(line);
        m_sourceCodeModel->updateHighlighting(line);
    };

    connect(ui->assemblyView, &QTreeView::entered, this, updateFromDisassembly);

    connect(ui->sourceCodeView, &QTreeView::entered, this, updateFromSource);

    connect(ui->sourceCodeView, qOverload<const QModelIndex&>(&QTreeView::entered), this, [this] {
        ui->sourceCodeView->update();
        ui->assemblyView->update();
    });
    connect(ui->assemblyView, qOverload<const QModelIndex&>(&QTreeView::entered), this, [this] {
        ui->sourceCodeView->update();
        ui->assemblyView->update();
    });

    connect(m_disassemblyDelegate, &DisassemblyDelegate::gotoFunction, this,
            [this](const QString& functionName, int offset) {
                if (m_curSymbol.symbol == functionName) {
                    ui->assemblyView->scrollTo(m_disassemblyModel->findIndexWithOffset(offset),
                                          QAbstractItemView::ScrollHint::PositionAtTop);
                } else {
                    const auto symbol = std::find_if(
                        m_callerCalleeResults.entries.keyBegin(), m_callerCalleeResults.entries.keyEnd(),
                        [functionName](const Data::Symbol& symbol) { return symbol.symbol == functionName; });

                    if (symbol != m_callerCalleeResults.entries.keyEnd()) {
                        setSymbol(*symbol);
                        showDisassembly();
                    }
                }
            });
}

ResultsDisassemblyPage::~ResultsDisassemblyPage() = default;

void ResultsDisassemblyPage::clear()
{
    m_disassemblyModel->clear();
    m_sourceCodeModel->clear();
}

void ResultsDisassemblyPage::setupAsmViewModel()
{
    ui->sourceCodeView->header()->setStretchLastSection(false);
    ui->sourceCodeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    ui->assemblyView->header()->setStretchLastSection(false);
    ui->assemblyView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->assemblyView->hideColumn(DisassemblyModel::LinkedFunctionName);
    ui->assemblyView->hideColumn(DisassemblyModel::LinkedFunctionOffset);
    ui->assemblyView->hideColumn(DisassemblyModel::SourceCodeLine);
    ui->assemblyView->hideColumn(DisassemblyModel::Highlight);

    ui->sourceCodeView->hideColumn(SourceCodeModel::SourceCodeLineNumber);
    ui->sourceCodeView->hideColumn(SourceCodeModel::Highlight);

    ui->assemblyView->setSelectionMode(QTreeView::NoSelection);

    ui->assemblyView->setItemDelegateForColumn(0, m_disassemblyDelegate);
    ui->sourceCodeView->setItemDelegateForColumn(0, m_sourceCodeDelegate);
    for (int col = 1; col < m_disassemblyModel->columnCount(); col++) {
        ui->assemblyView->setColumnWidth(col, 100);
        ui->assemblyView->header()->setSectionResizeMode(col, QHeaderView::Interactive);
        ui->assemblyView->setItemDelegateForColumn(col, m_costDelegate);
    }
}

void ResultsDisassemblyPage::showDisassembly()
{
    // Show empty tab when selected symbol is not valid
    if (m_curSymbol.symbol.isEmpty()) {
        clear();
    }

    // TODO: add the ability to configure the arch <-> objdump mapping somehow in the settings
    const auto objdump = [this]() {
        if (!m_objdump.isEmpty())
            return m_objdump;

        if (m_arch.startsWith(QLatin1String("armv8")) || m_arch.startsWith(QLatin1String("aarch64"))) {
            return QStringLiteral("aarch64-linux-gnu-objdump");
        }
        const auto isArm = m_arch.startsWith(QLatin1String("arm"));
        return isArm ? QStringLiteral("arm-linux-gnueabi-objdump") : QStringLiteral("objdump");
    };

    showDisassembly(DisassemblyOutput::disassemble(objdump(), m_arch, m_curSymbol));
}

void ResultsDisassemblyPage::showDisassembly(const DisassemblyOutput& disassemblyOutput)
{
    m_disassemblyModel->clear();
    m_sourceCodeModel->clear();

    const auto& entry = m_callerCalleeResults.entry(m_curSymbol);

    ui->symbolLabel->setText(tr("Disassembly for symbol:  %1").arg(Util::formatSymbol(m_curSymbol)));
    // don't set tooltip on symbolLabel, as that will be called internally and then get overwritten
    setToolTip(Util::formatTooltip(entry.id, m_curSymbol, m_callerCalleeResults.selfCosts,
                                   m_callerCalleeResults.inclusiveCosts));

    if (!disassemblyOutput) {
        ui->errorMessage->setText(disassemblyOutput.errorMessage);
        ui->errorMessage->show();
        return;
    }

    ui->errorMessage->hide();

    m_disassemblyModel->setDisassembly(disassemblyOutput);
    m_sourceCodeModel->setDisassembly(disassemblyOutput);

    setupAsmViewModel();
}

void ResultsDisassemblyPage::setSymbol(const Data::Symbol& symbol)
{
    m_curSymbol = symbol;
}

void ResultsDisassemblyPage::setCostsMap(const Data::CallerCalleeResults& callerCalleeResults)
{
    m_callerCalleeResults = callerCalleeResults;
    m_disassemblyModel->setResults(m_callerCalleeResults);
}

void ResultsDisassemblyPage::setObjdump(const QString& objdump)
{
    m_objdump = objdump;
}

void ResultsDisassemblyPage::setArch(const QString& arch)
{
    m_arch = arch.trimmed().toLower();
}
