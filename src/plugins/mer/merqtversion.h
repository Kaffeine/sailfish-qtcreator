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

#ifndef MERQTVERSION_H
#define MERQTVERSION_H

#include <qtsupport/baseqtversion.h>

namespace Mer {
namespace Internal {

class MerQtVersion : public QtSupport::BaseQtVersion
{
public:
    MerQtVersion();
    MerQtVersion(const Utils::FileName &path, bool isAutodetected = false,
                 const QString &autodetectionSource = QString());
    ~MerQtVersion();

    void setVirtualMachineName(const QString &name);
    QString virtualMachineName() const;
    void setTargetName(const QString &name);
    QString targetName() const;

    MerQtVersion *clone() const;

    QString type() const;

    QList<ProjectExplorer::Abi> detectQtAbis() const;

    QString description() const;

    bool supportsShadowBuilds() const;
    QString platformName() const;
    QString platformDisplayName() const;

    QList<ProjectExplorer::Task> validateKit(const ProjectExplorer::Kit *k);
    QVariantMap toMap() const;
    void fromMap(const QVariantMap &data);
    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const;

protected:
    QList<ProjectExplorer::Task> reportIssuesImpl(const QString &proFile,
                                                  const QString &buildDir) const;

    Core::FeatureSet availableFeatures() const;

private:
    QString m_vmName;
    QString m_targetName;
};

} // Internal
} // Mer

#endif // MERQTVERSION_H
