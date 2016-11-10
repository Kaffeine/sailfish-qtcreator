/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "mergeneraloptionswidget.h"
#include "ui_mergeneraloptionswidget.h"

#include "merconstants.h"
#include "merdeployconfiguration.h"
#include "merdeploysteps.h"
#include "mersettings.h"

using namespace Utils;

namespace Mer {
namespace Internal {

MerGeneralOptionsWidget::MerGeneralOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::MerGeneralOptionsWidget)
{
    m_ui->setupUi(this);
    m_ui->rpmValidationInfoLabel->setText(m_ui->rpmValidationInfoLabel->text()
            .arg(MerRpmValidationStep::displayName())
            .arg(MerMb2RpmBuildConfiguration::displayName()));
    m_ui->rpmValidationByDefaultCheckBox->setChecked(MerSettings::rpmValidationByDefault());
    m_ui->benchLocationPathChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_ui->benchLocationPathChooser->setPath(MerSettings::qmlLiveBenchLocation());
    m_ui->benchSyncWorkspaceCheckBox->setChecked(MerSettings::isSyncQmlLiveWorkspaceEnabled());
}

MerGeneralOptionsWidget::~MerGeneralOptionsWidget()
{
    delete m_ui;
}

void MerGeneralOptionsWidget::store()
{
    MerSettings::setRpmValidationByDefault(m_ui->rpmValidationByDefaultCheckBox->isChecked());
    MerSettings::setQmlLiveBenchLocation(m_ui->benchLocationPathChooser->path());
    MerSettings::setSyncQmlLiveWorkspaceEnabled(m_ui->benchSyncWorkspaceCheckBox->isChecked());
}

QString MerGeneralOptionsWidget::searchKeywords() const
{
    QString keywords;
    const QLatin1Char sep(' ');
    QTextStream(&keywords) << sep << m_ui->rpmValidationInfoLabel->text()
                           << sep << m_ui->rpmValidationByDefaultCheckBox->text()
                           << sep << m_ui->qmlLiveGroupBox->title()
                           << sep << m_ui->benchLocationLabel->text()
                           ;
    keywords.remove(QLatin1Char('&'));
    return keywords;
}

} // Internal
} // Mer
