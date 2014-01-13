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

#include "merdeploystepfactory.h"
#include "meremulatorstartstep.h"
#include "mersftpdeployconfiguration.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerDeployStepFactory::MerDeployStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

QList<Core::Id> MerDeployStepFactory::availableCreationIds(BuildStepList *parent) const
{
    QList<Core::Id> ids;
    if (!qobject_cast<MerSftpDeployConfiguration *>(parent->parent()))
        return ids;

    ids << MerEmulatorStartStep::stepId();
    return ids;
}

QString MerDeployStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == MerEmulatorStartStep::stepId())
        return MerEmulatorStartStep::stepDisplayName();
    return QString();
}

bool MerDeployStepFactory::canCreate(BuildStepList *parent, const Core::Id id) const
{
    return availableCreationIds(parent).contains(id) && !parent->contains(id);
}

BuildStep *MerDeployStepFactory::create(BuildStepList *parent, const Core::Id id)
{
    if (id == MerEmulatorStartStep::stepId())
        return new MerEmulatorStartStep(parent);
    return 0;
}

bool MerDeployStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, idFromMap(map));
}

BuildStep *MerDeployStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    QTC_ASSERT(canRestore(parent, map), return 0);
    BuildStep * const step = create(parent, idFromMap(map));
    if (!step->fromMap(map)) {
        delete step;
        return 0;
    }
    return step;
}

bool MerDeployStepFactory::canClone(BuildStepList *parent, BuildStep *product) const
{
    return canCreate(parent, product->id());
}

BuildStep *MerDeployStepFactory::clone(BuildStepList *parent, BuildStep *product)
{
    QTC_ASSERT(canClone(parent, product), return 0);
    if (MerEmulatorStartStep * const other = qobject_cast<MerEmulatorStartStep *>(product))
        return new MerEmulatorStartStep(parent, other);

    return 0;
}

} // namespace Internal
} // namespace Mer
