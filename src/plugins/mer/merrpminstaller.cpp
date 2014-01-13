/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "merrpminstaller.h"
#include "meremulatordevice.h"
#include "merconstants.h"
#include <ssh/sshconnection.h>

namespace Mer {
namespace Internal {

MerRpmInstaller::MerRpmInstaller(QObject *parent)
    : AbstractRemoteLinuxPackageInstaller(parent)
{
    connect(this, SIGNAL(stderrData(QString)), SLOT(handleInstallerErrorOutput(QString)));
}

void MerRpmInstaller::installPackage(const ProjectExplorer::IDevice::ConstPtr &deviceConfig, const QString &packageFilePath, bool removePackageFile)
{
    ProjectExplorer::IDevice::Ptr device = deviceConfig->clone();
    if(device->type() != Core::Id(Constants::MER_DEVICE_TYPE_I486))
        return;

    const MerEmulatorDevice* emu = static_cast<const MerEmulatorDevice*>(device.data());
    QSsh::SshConnectionParameters sshParams = emu->sshParametersForUser(device->sshParameters(), QLatin1String("root"));
    device->setSshParameters(sshParams);
    AbstractRemoteLinuxPackageInstaller::installPackage(device,packageFilePath,removePackageFile);
}

void MerRpmInstaller::handleInstallerErrorOutput(const QString &output)
{
    m_installerStderr = output;
}

QString MerRpmInstaller::installCommandLine(const QString &packageFilePath) const
{
    return QLatin1String("rpm -i --force ") + packageFilePath;
}

QString MerRpmInstaller::cancelInstallationCommandLine() const
{
    return QLatin1String("pkill rpm");
}

void MerRpmInstaller::prepareInstallation()
{
    m_installerStderr.clear();
}

QString MerRpmInstaller::errorString() const
{
    return m_installerStderr;
}

} // namespace Internal
} // namespace Mer
