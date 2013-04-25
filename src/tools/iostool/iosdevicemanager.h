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
#ifndef IOSMANAGER_H
#define IOSMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

namespace Ios {
namespace Internal {
class IosDeviceManagerPrivate;
} // namespace Internal

class IosDeviceManager : public QObject
{
    Q_OBJECT
public:
    typedef QMap<QString,QString> Dict;
    enum OpStatus {
        Success = 0,
        Warning = 1,
        Failure = 2
    };
    enum AppOp {
        None = 0,
        Install = 1,
        Run = 2,
        InstallAndRun = 3
    };

    static IosDeviceManager *instance();
    bool watchDevices();
    void requestAppOp(const QString &bundlePath, const QStringList &extraArgs, AppOp appOp,
                      const QString &deviceId, int timeout = 1000);
    void requestDeviceInfo(const QString &deviceId, int timeout = 1000);
    int processGdbServer(int fd);
    QStringList errors();
signals:
    void deviceAdded(const QString &deviceId);
    void deviceRemoved(const QString &deviceId);
    void isTransferringApp(const QString &bundlePath, const QString &deviceId, int progress,
                           const QString &info);
    void didTransferApp(const QString &bundlePath, const QString &deviceId,
                        Ios::IosDeviceManager::OpStatus status);
    void didStartApp(const QString &bundlePath, const QString &deviceId,
                        Ios::IosDeviceManager::OpStatus status, int gdbFd);
    void deviceInfo(const QString &deviceId, const Ios::IosDeviceManager::Dict &info);
    void appOutput(const QString &output);
    void errorMsg(const QString &msg);
private slots:
    void checkPendingLookups();
private:
    friend class Internal::IosDeviceManagerPrivate;
    IosDeviceManager(QObject *parent = 0);
    Internal::IosDeviceManagerPrivate *d;
};

} // namespace Ios

#endif // IOSMANAGER_H
