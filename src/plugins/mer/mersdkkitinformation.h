/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#ifndef MERSDKKITINFORMATION_H
#define MERSDKKITINFORMATION_H

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitconfigwidget.h>

QT_FORWARD_DECLARE_CLASS(QComboBox);
QT_FORWARD_DECLARE_CLASS(QPushButton);

namespace Mer {
namespace Internal {

class MerSdk;

class  MerSdkKitInformationWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT
public:
    MerSdkKitInformationWidget(ProjectExplorer::Kit *kit);

    QString displayName() const;
    QString toolTip() const;
    void makeReadOnly();
    void refresh();
    bool visibleInKit();

    QWidget *mainWidget() const;
    QWidget *buttonWidget() const;
private:
    int findMerSdk(const MerSdk* sdk) const;
private slots:
    void handleSdksUpdated();
    void handleManageClicked();
    void handleCurrentIndexChanged();
private:
    QComboBox *m_combo;
    QPushButton *m_manageButton;
};


class MerSdkKitInformation : public ProjectExplorer::KitInformation
{
    Q_OBJECT
public:
    explicit MerSdkKitInformation();
    Core::Id dataId() const;
    unsigned int priority() const;
    QVariant defaultValue(ProjectExplorer::Kit *) const;
    QList<ProjectExplorer::Task> validate(const ProjectExplorer::Kit *) const;
    //void fix(ProjectExplorer::Kit *);
    //void setup(ProjectExplorer::Kit *);
    ItemList toUserOutput(ProjectExplorer::Kit *) const ;
    ProjectExplorer::KitConfigWidget *createConfigWidget(ProjectExplorer::Kit *) const ;
    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const;

    static void setSdk(ProjectExplorer::Kit *k, const MerSdk* sdk);
    static MerSdk* sdk(const ProjectExplorer::Kit *k);
};

}
}

#endif // MERSDKKITINFORMATION_H
