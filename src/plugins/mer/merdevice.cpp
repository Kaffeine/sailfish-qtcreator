/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
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

#include "merdevice.h"

#include "merconstants.h"

namespace Mer {
namespace Internal {

void MerDevice::fromMap(const QVariantMap &map)
{
    IDevice::fromMap(map);
    m_sharedSshPath = map.value(QLatin1String(Constants::MER_DEVICE_SHARED_SSH)).toString();
}

QVariantMap MerDevice::toMap() const
{
    QVariantMap map = IDevice::toMap();
    map.insert(QLatin1String(Constants::MER_DEVICE_SHARED_SSH), m_sharedSshPath);
    return map;
}

void MerDevice::setSharedSshPath(const QString &sshPath)
{
    m_sharedSshPath = sshPath;
}

QString MerDevice::sharedSshPath() const
{
    return m_sharedSshPath;
}

MerDevice::MerDevice()
{
}

MerDevice::MerDevice(const QString &name, Core::Id type, MachineType machineType, Origin origin,
        Core::Id id)
    : RemoteLinux::LinuxDevice(name, type, machineType, origin, id)
{
    setDeviceState(IDevice::DeviceStateUnknown);
}

MerDevice::~MerDevice()
{
}

}
}
