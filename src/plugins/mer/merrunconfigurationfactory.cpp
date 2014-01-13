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

#include "merrunconfigurationfactory.h"
#include "merconstants.h"
#include "merrunconfiguration.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4project.h>

#include <utils/qtcassert.h>

using namespace Mer::Constants;

namespace Mer {
namespace Internal {

namespace {
QString pathFromId(const Core::Id id)
{
    const QString idStr = id.toString();
    if (idStr.startsWith(QLatin1String(MER_RUNCONFIGURATION_PREFIX)))
        return idStr.mid(QString::fromLatin1(MER_RUNCONFIGURATION_PREFIX).size());

    return QString();
}
} // Anonymous

MerRunConfigurationFactory::MerRunConfigurationFactory(QObject *parent)
    : ProjectExplorer::IRunConfigurationFactory(parent)
{
}

QList<Core::Id> MerRunConfigurationFactory::availableCreationIds(
        ProjectExplorer::Target *parent) const
{
    QList<Core::Id> ids;
    if (!canHandle(parent))
        return ids;

    Qt4ProjectManager::Qt4Project *qt4Project =
            qobject_cast<Qt4ProjectManager::Qt4Project *>(parent->project());
    if (!qt4Project)
        return ids;

    QStringList proFiles =
            qt4Project->applicationProFilePathes(QLatin1String(MER_RUNCONFIGURATION_PREFIX));
    //TODO::foreach (const QString &pf, proFiles)
    //    ids << Core::Id(pf);
    return ids;
}

QString MerRunConfigurationFactory::displayNameForId(const Core::Id id) const
{
    const QString path = pathFromId(id);
    if (path.isEmpty())
        return QString();

    if (id.toString().startsWith(QLatin1String(MER_RUNCONFIGURATION_PREFIX)))
        return tr("%1 on Mer Device").arg(QFileInfo(path).completeBaseName());

    return QString();
}

bool MerRunConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;

    Qt4ProjectManager::Qt4Project *qt4Project =
            qobject_cast<Qt4ProjectManager::Qt4Project *>(parent->project());
    if (!qt4Project)
        return false;

    return qt4Project->hasApplicationProFile(pathFromId(id));
}

ProjectExplorer::RunConfiguration *MerRunConfigurationFactory::doCreate(
        ProjectExplorer::Target *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;

    if (id.toString().startsWith(QLatin1String(MER_RUNCONFIGURATION_PREFIX))){
        return new MerRunConfiguration(parent, id, pathFromId(id));
    }

    return 0;
}

bool MerRunConfigurationFactory::canRestore(ProjectExplorer::Target *parent,
                                            const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;

    return ProjectExplorer::idFromMap(map).toString().startsWith(
                QLatin1String(MER_RUNCONFIGURATION_PREFIX));
}

ProjectExplorer::RunConfiguration *MerRunConfigurationFactory::doRestore(
        ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    Core::Id id = ProjectExplorer::idFromMap(map);
    ProjectExplorer::RunConfiguration *rc = new MerRunConfiguration(parent, id, pathFromId(id));
    QTC_ASSERT(rc, return 0);
    if (rc->fromMap(map))
        return rc;

    delete rc;
    return 0;
}

bool MerRunConfigurationFactory::canClone(ProjectExplorer::Target *parent,
                                          ProjectExplorer::RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration *MerRunConfigurationFactory::clone(
        ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    MerRunConfiguration *old = qobject_cast<MerRunConfiguration *>(source);

    if (!old)
        return 0;

    return new MerRunConfiguration(parent, old);
}

bool MerRunConfigurationFactory::canHandle(ProjectExplorer::Target *t) const
{
    return ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(t->kit()) == MER_DEVICE_TYPE;
}

} // Internal
} // Mer
