/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include "merhardwaredevice.h"

#include "merconstants.h"
#include "merdevicefactory.h"
#include "merhardwaredevicewidget.h"
#include "merhardwaredevicewizardpages.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <remotelinux/remotelinux_constants.h>
#include <ssh/sshconnection.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QProgressDialog>
#include <QTimer>
#include <QWizard>

namespace Mer {
namespace Internal {

MerHardwareDevice::Ptr MerHardwareDevice::create()
{
    return Ptr(new MerHardwareDevice);
}

MerHardwareDevice::Ptr MerHardwareDevice::create(const QString &name,
                                 Origin origin,
                                 Core::Id id)
{
    return Ptr(new MerHardwareDevice(name, origin, id));
}

MerHardwareDevice::MerHardwareDevice(const QString &name,
                     Origin origin,
                     Core::Id id)
    : MerDevice(name, IDevice::Hardware, origin, id),
      m_architecture(ProjectExplorer::Abi::UnknownArchitecture)
{
}

ProjectExplorer::IDevice::Ptr MerHardwareDevice::clone() const
{
    return Ptr(new MerHardwareDevice(*this));
}

MerHardwareDevice::MerHardwareDevice()
{
}

void MerHardwareDevice::fromMap(const QVariantMap &map)
{
    MerDevice::fromMap(map);
    m_architecture = static_cast<ProjectExplorer::Abi::Architecture>(
            map.value(QLatin1String(Constants::MER_DEVICE_ARCHITECTURE),
                ProjectExplorer::Abi::UnknownArchitecture).toInt());
}

QVariantMap MerHardwareDevice::toMap() const
{
    QVariantMap map = MerDevice::toMap();
    map.insert(QLatin1String(Constants::MER_DEVICE_ARCHITECTURE), m_architecture);
    return map;
}

ProjectExplorer::Abi::Architecture MerHardwareDevice::architecture() const
{
    return m_architecture;
}

void MerHardwareDevice::setArchitecture(const ProjectExplorer::Abi::Architecture &architecture)
{
    m_architecture = architecture;
}

ProjectExplorer::IDeviceWidget *MerHardwareDevice::createWidget()
{
    return new MerHardwareDeviceWidget(sharedFromThis());
}


QList<Core::Id> MerHardwareDevice::actionIds() const
{
    QList<Core::Id> ids;
    //ids << Core::Id(Constants::MER_HARDWARE_DEPLOYKEY_ACTION_ID);
    return ids;
}

QString MerHardwareDevice::displayNameForActionId(Core::Id actionId) const
{
    QTC_ASSERT(actionIds().contains(actionId), return QString());

    if (actionId == Constants::MER_HARDWARE_DEPLOYKEY_ACTION_ID)
        return tr("Redeploy SSH Keys");
    return QString();
}


void MerHardwareDevice::executeAction(Core::Id actionId, QWidget *parent)
{
    Q_UNUSED(parent);
    QTC_ASSERT(actionIds().contains(actionId), return);

    if (actionId ==  Constants::MER_HARDWARE_DEPLOYKEY_ACTION_ID) {

        //TODO:
        return;
    }
}

} // Internal
} // Mer
