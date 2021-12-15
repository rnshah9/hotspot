/*
    SPDX-FileCopyrightText: Lieven Hey <lieven.hey@kdab.com>
    SPDX-FileCopyrightText: 2022 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "diffdialog.h"
#include "ui_diffdialog.h"

DiffDialog::DiffDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DiffDialog)
{
    ui->setupUi(this);
    const auto filterString = QLatin1String("perf*.data\nperf.data*");
    ui->file_a->setFilter(filterString);
    ui->file_b->setFilter(filterString);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
}

DiffDialog::~DiffDialog() = default;

QString DiffDialog::fileA() const
{
    return ui->file_a->url().path();
}

QString DiffDialog::fileB() const
{
    return ui->file_b->url().path();
}
