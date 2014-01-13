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

#include "mersdk.h"
#include "mertarget.h"
#include "merconstants.h"
#include "mersdkmanager.h"
#include "mertoolchain.h"
#include "merqtversion.h"
#include "mertargetsxmlparser.h"
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionfactory.h>
#include <utils/hostosinfo.h>
#include <QDir>

using namespace Mer::Constants;

namespace Mer {
namespace Internal {

MerSdk::MerSdk(QObject *parent) : QObject(parent)
    , m_autoDetected(false)
    , m_sshPort(-1)
    , m_wwwPort(-1)
    , m_timeout(30)
    , m_headless(false)
{
    connect(&m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(handleTargetsFileChanged(QString)));
}

MerSdk::~MerSdk()
{

}

void MerSdk::setHeadless(bool enabled)
{
    if (m_headless != enabled) {
        m_headless = enabled;
        emit headlessChanged(enabled);
    }
}

bool MerSdk::isHeadless() const
{
    return m_headless;
}

bool MerSdk::isAutoDetected() const
{
    return m_autoDetected;
}

void MerSdk::setAutodetect(bool autoDetected)
{
    m_autoDetected = autoDetected;
}

void MerSdk::setVirtualMachineName(const QString &name)
{
    m_name = name;
}

QString MerSdk::virtualMachineName() const
{
    return m_name;
}

void MerSdk::setSharedHomePath(const QString &homePath)
{
    m_sharedHomePath = homePath;
}

QString MerSdk::sharedHomePath() const
{
    return m_sharedHomePath;
}

void MerSdk::setTargets(const QList<MerTarget> &targets)
{
    if (m_targets != targets) {
        m_targets = targets;
        emit targetsChanged(this->targetNames());
    }
}

QStringList MerSdk::targetNames() const
{
    QStringList result;
    foreach (const MerTarget &target, m_targets)
        result << target.name();
    return result;
}

QList<MerTarget> MerSdk::targets() const
{
    return m_targets;
}

void MerSdk::setSharedTargetsPath(const QString &targetsPath)
{
    m_sharedTargetsPath = targetsPath;
}

QString MerSdk::sharedTargetsPath() const
{
    return m_sharedTargetsPath;
}

void MerSdk::setSharedConfigPath(const QString &configPath)
{
    m_sharedConfigPath = configPath;
}

QString MerSdk::sharedConfigPath() const
{
    return m_sharedConfigPath;
}

void MerSdk::setSharedSshPath(const QString &sshPath)
{
    m_sharedSshPath = sshPath;
}

QString MerSdk::sharedSshPath() const
{
    return m_sharedSshPath;
}

void MerSdk::setSharedSrcPath(const QString &srcPath)
{
    m_sharedSrcPath = srcPath;
}

QString MerSdk::sharedSrcPath() const
{
    return m_sharedSrcPath;
}

void MerSdk::setSshPort(quint16 port)
{
    m_sshPort = port;
}

quint16 MerSdk::sshPort() const
{
    return m_sshPort;
}

void MerSdk::setWwwPort(quint16 port)
{
    m_wwwPort = port;
}

quint16 MerSdk::wwwPort() const
{
    return m_wwwPort;
}

void MerSdk::setPrivateKeyFile(const QString &file)
{
    if (m_privateKeyFile != file) {
        m_privateKeyFile = file;
        emit privateKeyChanged(file);
    }
}

QString MerSdk::privateKeyFile() const
{
    return m_privateKeyFile;
}

void MerSdk::setHost(const QString &host)
{
    m_host = host;
}

QString MerSdk::host() const
{
    return m_host;
}

void MerSdk::setUserName(const QString &username)
{
    m_userName = username;
}

QString MerSdk::userName() const
{
    return m_userName;
}

void MerSdk::setTimeout(int timeout)
{
    m_timeout = timeout;
}

int MerSdk::timeout() const
{
    return m_timeout;
}

// call the install all the targets and track the changes
void MerSdk::attach()
{
    if (!m_watcher.files().isEmpty())
            m_watcher.removePaths(m_watcher.files());
    const QString file = sharedTargetsPath() + QLatin1String(Constants::MER_TARGETS_FILENAME);
    QFileInfo fi(file);
    if (fi.exists()) {
        m_watcher.addPath(file);
        updateTargets();
    } else {
        qWarning() << "Targets file not found: " << file;
        qWarning() << "This will remove all installed targets for" << virtualMachineName();
        updateTargets();
    }
}

// call the uninstall all the targets.
void MerSdk::detach()
{
    disconnect(&m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(handleTargetsFileChanged(QString)));
    if (!m_watcher.files().isEmpty())
            m_watcher.removePaths(m_watcher.files());
    foreach (const MerTarget &target , m_targets) {
        removeTarget(target);
    }
    m_targets.clear();
}

QVariantMap MerSdk::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(VM_NAME), virtualMachineName());
    result.insert(QLatin1String(AUTO_DETECTED), isAutoDetected());
    result.insert(QLatin1String(SHARED_HOME), sharedHomePath());
    result.insert(QLatin1String(SHARED_TARGET), sharedTargetsPath());
    result.insert(QLatin1String(SHARED_CONFIG), sharedConfigPath());
    result.insert(QLatin1String(SHARED_SRC), sharedSrcPath());
    result.insert(QLatin1String(SHARED_SSH), sharedSshPath());
    result.insert(QLatin1String(HOST), host());
    result.insert(QLatin1String(USERNAME), userName());
    result.insert(QLatin1String(PRIVATE_KEY_FILE), privateKeyFile());
    result.insert(QLatin1String(SSH_PORT), QString::number(sshPort()));
    result.insert(QLatin1String(WWW_PORT), QString::number(wwwPort()));
    result.insert(QLatin1String(HEADLESS), isHeadless());
    int count = 0;
    foreach (const MerTarget &target, m_targets) {
        if (!target.isValid())
            qWarning() << target.name()<<"is configured incorrectly...";
        QVariantMap tmp = target.toMap();
        if (tmp.isEmpty())
            continue;
        result.insert(QString::fromLatin1(MER_TARGET_DATA_KEY) + QString::number(count), tmp);
        ++count;
    }
    result.insert(QLatin1String(MER_TARGET_COUNT_KEY), count);
    return result;
}

bool MerSdk::fromMap(const QVariantMap &data)
{
    setVirtualMachineName(data.value(QLatin1String(VM_NAME)).toString());
    setAutodetect(data.value(QLatin1String(AUTO_DETECTED)).toBool());
    setSharedHomePath(data.value(QLatin1String(SHARED_HOME)).toString());
    setSharedTargetsPath(data.value(QLatin1String(SHARED_TARGET)).toString());
    setSharedConfigPath(data.value(QLatin1String(SHARED_CONFIG)).toString());
    setSharedSrcPath(data.value(QLatin1String(SHARED_SRC)).toString());
    setSharedSshPath(data.value(QLatin1String(SHARED_SSH)).toString());
    setHost(data.value(QLatin1String(HOST)).toString());
    setUserName(data.value(QLatin1String(USERNAME)).toString());
    setPrivateKeyFile( data.value(QLatin1String(PRIVATE_KEY_FILE)).toString());
    setSshPort(data.value(QLatin1String(SSH_PORT)).toUInt());
    setWwwPort(data.value(QLatin1String(WWW_PORT)).toUInt());
    setHeadless(data.value(QLatin1String(HEADLESS)).toBool());

    int count = data.value(QLatin1String(MER_TARGET_COUNT_KEY), 0).toInt();
    QList<MerTarget> targets;
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(MER_TARGET_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;
        const QVariantMap targetMap = data.value(key).toMap();
        MerTarget target(this);
        if (!target.fromMap(targetMap))
             qWarning() << target.name() << "is configured incorrectly...";
        targets << target;
    }

    setTargets(targets);

    return isValid();
}

bool MerSdk::isValid() const
{
    return !m_name.isEmpty()
            && !m_sharedHomePath.isEmpty()
            && !m_sharedSshPath.isEmpty()
            && !m_sharedTargetsPath.isEmpty();
}

void MerSdk::updateTargets()
{
    QList<MerTarget> sdkTargets = readTargets(Utils::FileName::fromString(sharedTargetsPath() + QLatin1String(Constants::MER_TARGETS_FILENAME)));
    QList<MerTarget> targetsToInstall;
    QList<MerTarget> targetsToKeep;
    QList<MerTarget> targetsToRemove = m_targets;

    //sort
    foreach (const MerTarget &sdkTarget, sdkTargets) {
        if (targetsToRemove.contains(sdkTarget)) {
            targetsToRemove.removeAll(sdkTarget);
            targetsToKeep<<sdkTarget;
        } else {
            targetsToInstall << sdkTarget;
        }
    }

    //dispatch
    foreach (const MerTarget &sdkTarget, targetsToRemove)
        removeTarget(sdkTarget);
    foreach (const MerTarget &sdkTarget, targetsToInstall)
        addTarget(sdkTarget);

    setTargets(targetsToInstall<<targetsToKeep);
}

void MerSdk::handleTargetsFileChanged(const QString &file)
{
    QTC_ASSERT(file == sharedTargetsPath() + QLatin1String(Constants::MER_TARGETS_FILENAME), return);
    updateTargets();
}

QList<MerTarget> MerSdk::readTargets(const Utils::FileName &fileName)
{
    QList<MerTarget> result;
    MerTargetsXmlReader xmlParser(fileName.toString());
    if (!xmlParser.hasError() && xmlParser.version() > 0) {
        QList<MerTargetData> targetData = xmlParser.targetData();
        foreach (const MerTargetData &data, targetData) {
            MerTarget target(this);
            target.setName(data.name);
            target.setGccDumpMachine(data.gccDumpMachine);
            target.setQmakeQuery(data.qmakeQuery);
            result << target;
        }
    }
    return result;
}

bool MerSdk::addTarget(const MerTarget &target)
{
    if (MerSdkManager::verbose)
        qDebug() << "Installing" << target.name() << "for" << virtualMachineName();
    if (!target.createScripts()) {
        qWarning() << "Failed to create wrapper scripts.";
        return false;
    }

    QScopedPointer<MerToolChain> toolchain(target.createToolChain());
    if (toolchain.isNull())
        return false;
    QScopedPointer<MerQtVersion> version(target.createQtVersion());
    if (version.isNull())
        return false;
    ProjectExplorer::Kit *kit = target.createKit();
    if (!kit)
        return false;

    ProjectExplorer::ToolChainManager::instance()->registerToolChain(toolchain.data());
    QtSupport::QtVersionManager::instance()->addVersion(version.data());
    QtSupport::QtKitInformation::setQtVersion(kit, version.data());
    ProjectExplorer::ToolChainKitInformation::setToolChain(kit, toolchain.data());
    ProjectExplorer::KitManager::instance()->registerKit(kit);
    toolchain.take();
    version.take();
    return true;
}

bool MerSdk::removeTarget(const MerTarget &target)
{
    if (MerSdkManager::verbose)
        qDebug() << "Uninstalling" << target.name() << "for" << virtualMachineName();
    //delete kit
    foreach (ProjectExplorer::Kit *kit, ProjectExplorer::KitManager::instance()->kits()) {
        if (!kit->isAutoDetected())
            continue;
        ProjectExplorer::ToolChain* tc = ProjectExplorer::ToolChainKitInformation::toolChain(kit);
        if (!tc ) {
            continue;
        }
        if (tc->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE)) {
            MerToolChain *mertoolchain = static_cast<MerToolChain*>(tc);
            if (mertoolchain->virtualMachineName() == m_name && mertoolchain->targetName() == target.name()) {
                 QtSupport::BaseQtVersion *v = QtSupport::QtKitInformation::qtVersion(kit);
                 ProjectExplorer::KitManager::instance()->deregisterKit(kit);
                 ProjectExplorer::ToolChainManager::instance()->deregisterToolChain(tc);
                 QTC_ASSERT(v && v->type() == QLatin1String(Constants::MER_QT), continue); //serious bug
                 QtSupport::QtVersionManager::instance()->removeVersion(v);
                 target.deleteScripts();
                 return true;
            }
        }
    }
    return false;
}

} // Internal
} // Mer
