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

#include "merconstants.h"
#include "merrunconfiguration.h"
#include "merdeployconfiguration.h"
#include "projectexplorer/kitinformation.h"

#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/kitinformation.h>

namespace Mer {
namespace Internal {

MerRunConfiguration::MerRunConfiguration(ProjectExplorer::Target *parent, const Core::Id id,
                                         const QString &proFilePath)
    : RemoteLinux::RemoteLinuxRunConfiguration(parent, id, proFilePath)
{
    ctor();
}

MerRunConfiguration::MerRunConfiguration(ProjectExplorer::Target *parent,
                                         MerRunConfiguration *source)
    : RemoteLinux::RemoteLinuxRunConfiguration(parent, source)
{
    ctor();
}

void MerRunConfiguration::ctor()
{
    connect(target(),SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),this,SIGNAL(enabledChanged()));
}

QString MerRunConfiguration::disabledReason() const
{
    if(m_disabledReason.isEmpty())
        return RemoteLinuxRunConfiguration::disabledReason();
    else
        return m_disabledReason;
}

bool MerRunConfiguration::isEnabled() const
{   
    //TODO Hack

    ProjectExplorer::DeployConfiguration* conf = target()->activeDeployConfiguration();
    if(target()->kit())
    {   
        if(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit()) == Constants::MER_DEVICE_TYPE_ARM
            && conf->id() != MerRpmBuildDeployConfiguration::configurationId()) {
            m_disabledReason = tr("Alpha2 SDK does not support run configuration for arm packages");
            return false;
        }
    }

    return RemoteLinuxRunConfiguration::isEnabled();
}

QString MerRunConfiguration::defaultRemoteExecutableFilePath() const
{
    ProjectExplorer::DeployConfiguration* conf = target()->activeDeployConfiguration();
    if (!conf) return QString();

    QString executable = RemoteLinuxRunConfiguration::defaultRemoteExecutableFilePath();
    if (conf->id() == MerRsyncDeployConfiguration::configurationId()) {
        QString projectName = target()->project()->displayName();
        return QLatin1String("/opt/sdk/") + projectName + executable;
    }

    if (conf->id() == MerRpmDeployConfiguration::configurationId()) {
        //TODO:
        return executable;
    }

    return executable;
}

// TODO: Temporary workaround for lipstick QML issue
// Opens unlock the phone befeore running app
// Remove me !
QString MerRunConfiguration::commandPrefix() const
{
    return QString::fromLatin1("/usr/sbin/mcetool -Don;").append(
                RemoteLinuxRunConfiguration::commandPrefix());
}

} // Internal
} // Mer
