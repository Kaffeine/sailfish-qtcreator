/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#ifndef IOSCONFIGURATIONS_H
#define IOSCONFIGURATIONS_H

#include <projectexplorer/abi.h>
#include <utils/fileutils.h>

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Ios {
namespace Internal {

class IosConfig
{
public:
    IosConfig();
    IosConfig(const QSettings &settings);
    void save(QSettings &settings) const;

    Utils::FileName developerPath;
    bool ignoreAllDevices;
};

class IosConfigurations : public QObject
{
    Q_OBJECT

public:
    static IosConfigurations &instance();
    IosConfig config() const { return m_config; }
    void setConfig(const IosConfig &config);
    Utils::FileName iosToolPath() const;

    QStringList sdkTargets();
    void updateSimulators();
signals:
    void updated();

public slots:
    void updateAutomaticKitList();

private:
    IosConfigurations(QObject *parent);
    void load();
    void save();

    static IosConfigurations *m_instance;
    IosConfig m_config;
    QTimer m_updateAvailableDevices;
};

} // namespace Internal
} // namespace Ios

#endif // IOSCONFIGURATIONS_H
