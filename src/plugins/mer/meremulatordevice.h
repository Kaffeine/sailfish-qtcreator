#ifndef MEREMULATORDEVICE_H
#define MEREMULATORDEVICE_H

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

#include <projectexplorer/devicesupport/idevice.h>
#include <QCoreApplication>

namespace Mer {
namespace Internal {

class MerEmulatorDevice : public ProjectExplorer::IDevice
{
    Q_DECLARE_TR_FUNCTIONS(Mer::Internal::MerEmulatorDevice)

public:
    typedef QSharedPointer<MerEmulatorDevice> Ptr;

    static Ptr create();
    ProjectExplorer::IDevice::Ptr clone() const;
    QString displayType() const;
    ProjectExplorer::IDeviceWidget *createWidget();
    QList<Core::Id> actionIds() const;
    QString displayNameForActionId(Core::Id actionId) const;
    void executeAction(Core::Id actionId, QWidget *parent) const;

    void fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    void setVirtualMachine(const QString& machineName);
    QString virtualMachine() const;

    void setSdkName(const QString& sdkName);
    QString sdkName() const;

    void setMac(const QString& mac);
    QString mac() const;

    void setSubnet(const QString& subnet);
    QString subnet() const;

    void setIndex(int index);
    int index() const;
private:
    MerEmulatorDevice();
private:
    QString m_virtualMachine;
    QString m_sdkName;
    QString m_mac;
    QString m_subnet;
    int m_index;

};

}
}

#endif // MEREMULATORDEVICE_H
