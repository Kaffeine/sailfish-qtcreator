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

#include "mertarget.h"
#include "mersdk.h"
#include "merconstants.h"
#include "mersdkmanager.h"
#include "mertoolchain.h"
#include "merqtversion.h"
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtversionfactory.h>
#include <projectexplorer/toolchainmanager.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <QDir>

namespace Mer {
namespace Internal {

using namespace Mer::Constants;

static const struct WrapperScript {
    enum Execution {
        ExecutionStandard,
        ExecutionTypeSb2
    };
    const char* name;
    Execution executionType;
} wrapperScripts[] = {
    { MER_WRAPPER_QMAKE, WrapperScript::ExecutionTypeSb2 },
    { MER_WRAPPER_MAKE, WrapperScript::ExecutionTypeSb2 },
    { MER_WRAPPER_GCC, WrapperScript::ExecutionTypeSb2 },
    { MER_WRAPPER_SPECIFY, WrapperScript::ExecutionStandard },
    { MER_WRAPPER_MB, WrapperScript::ExecutionStandard }
};

MerTarget::MerTarget(MerSdk* mersdk):
    m_sdk(mersdk)
{
}

MerTarget::~MerTarget()
{
}

QString MerTarget::name() const
{
    return m_name;
}

bool MerTarget::fromMap(const QVariantMap &data)
{
    m_name = data.value(QLatin1String(Constants::TARGET_NAME)).toString();
    m_qmakeQuery = data.value(QLatin1String(Constants::QMAKE_DUMP)).toString();
    m_gccMachineDump = data.value(QLatin1String(Constants::GCC_DUMP)).toString();
    return isValid();
}

QVariantMap MerTarget::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(Constants::TARGET_NAME), m_name);
    result.insert(QLatin1String(Constants::QMAKE_DUMP), m_qmakeQuery);
    result.insert(QLatin1String(Constants::GCC_DUMP), m_gccMachineDump);
    return result;
}

bool MerTarget::isValid() const
{
    return !m_name.isEmpty() && !m_qmakeQuery.isEmpty() && !m_gccMachineDump.isEmpty();
}

bool MerTarget::createScripts() const
{
    if (!isValid())
        return false;

    const QString targetPath(MerSdkManager::sdkToolsDirectory()
                              + m_sdk->virtualMachineName() + QLatin1Char('/') + m_name);
    QDir targetDir(targetPath);
    if (!targetDir.exists() && !targetDir.mkpath(targetPath)) {
        qWarning() << "Could not create target directory." << targetDir;
        return false;
    }
    bool result = true;
    for (size_t i = 0; i < sizeof(wrapperScripts) / sizeof(wrapperScripts[0]); ++i)
         result &= createScript(targetPath, i);

    const QString qmakepath = targetPath + QLatin1Char('/') + QLatin1String(Constants::QMAKE_QUERY);
    const QString gccpath = targetPath + QLatin1Char('/') + QLatin1String(Constants::GCC_DUMPMACHINE);

    result &= createCacheFile(qmakepath, m_qmakeQuery);
    result &= createCacheFile(gccpath, m_gccMachineDump);

    return result;
}

void MerTarget::deleteScripts() const
{
    const QString targetPath(MerSdkManager::sdkToolsDirectory()
                              + m_sdk->virtualMachineName() + QLatin1Char('/') + m_name);
    Utils::FileUtils::removeRecursively(Utils::FileName::fromString(targetPath));
}

ProjectExplorer::Kit* MerTarget::createKit() const
{
    if (!isValid())
        return 0;
    const QString sysroot = m_sdk->sharedTargetPath() + QLatin1String("/") + m_name;
    Utils::FileName path = Utils::FileName::fromString(sysroot);
    if (!path.toFileInfo().exists()) {
        qWarning()<< "Sysroot does not exists" << sysroot;
        return 0;
    }
    ProjectExplorer::Kit *k = new ProjectExplorer::Kit();
    k->setAutoDetected(true);
    k->setDisplayName(QString::fromLatin1("%1-%2").arg(m_sdk->virtualMachineName(), m_name));
    k->setIconPath(QLatin1String(Constants::MER_OPTIONS_CATEGORY_ICON));
    k->setValue(Core::Id(Constants::VM_NAME), m_sdk->virtualMachineName());
    ProjectExplorer::SysRootKitInformation::setSysRoot(k, Utils::FileName::fromString(sysroot));
    ProjectExplorer::DeviceTypeKitInformation::setDeviceTypeId(k, Constants::MER_DEVICE_TYPE);
    return k;
}

MerQtVersion* MerTarget::createQtVersion() const
{
    const QString targetDirectory(MerSdkManager::sdkToolsDirectory() + m_sdk->virtualMachineName() + QLatin1Char('/') + m_name);
    const Utils::FileName qmake =
            Utils::FileName::fromString(targetDirectory + QLatin1Char('/') +
                                        QLatin1String(Constants::MER_WRAPPER_QMAKE));
    QtSupport::QtVersionManager *qtvm = QtSupport::QtVersionManager::instance();
    // Is there a qtversion present for this qmake?
    QtSupport::BaseQtVersion *qtv = qtvm->qtVersionForQMakeBinary(qmake);
    if (qtv && !qtv->isValid()) {
        qtvm->removeVersion(qtv);
        qtv = 0;
    }
    if (!qtv)
        qtv = QtSupport::QtVersionFactory::createQtVersionFromQMakePath(qmake, true, targetDirectory);

    QTC_ASSERT(qtv && qtv->type() == QLatin1String(Constants::MER_QT), return 0);

    MerQtVersion *merqtv = static_cast<MerQtVersion *>(qtv);
    const QString vmName = m_sdk->virtualMachineName();
    merqtv->setDisplayName(
                QString::fromLatin1("Qt %1 in %2 %3").arg(qtv->qtVersionString(),
                                                          vmName, m_name));
    merqtv->setVirtualMachineName(vmName);
    merqtv->setTargetName(m_name);
    return merqtv;
}

MerToolChain* MerTarget::createToolChain() const
{
    const QString targetToolsDir(MerSdkManager::sdkToolsDirectory() + m_sdk->virtualMachineName() + QLatin1Char('/') + m_name);
    const Utils::FileName gcc =
            Utils::FileName::fromString(targetToolsDir + QLatin1Char('/') +
                                        QLatin1String(Constants::MER_WRAPPER_GCC));
    ProjectExplorer::ToolChainManager *tcm = ProjectExplorer::ToolChainManager::instance();
    QList<ProjectExplorer::ToolChain *> toolChains = tcm->toolChains();

    foreach (ProjectExplorer::ToolChain *tc, toolChains) {
        if (tc->compilerCommand() == gcc && tc->isAutoDetected()) {
            QTC_ASSERT(tc->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE), return 0);
        }
    }

    MerToolChain* mertoolchain = new MerToolChain(true, gcc);
    const QString vmName = m_sdk->virtualMachineName();
    mertoolchain->setDisplayName(QString::fromLatin1("GCC (%1 %2)").arg(vmName, m_name));
    mertoolchain->setVirtualMachine(vmName);
    mertoolchain->setTargetName(m_name);
    return mertoolchain;
}

bool MerTarget::createScript(const QString &targetPath, int scriptIndex) const
{
    bool ok = false;
    const WrapperScript &wrapperScriptCopy = wrapperScripts[scriptIndex];
    const QFile::Permissions rwrw = QFile::ReadOwner|QFile::ReadUser|QFile::ReadGroup
            |QFile::WriteOwner|QFile::WriteUser|QFile::WriteGroup;
    const QFile::Permissions rwxrwx = rwrw|QFile::ExeOwner|QFile::ExeUser|QFile::ExeGroup;

    QString wrapperScriptCommand = QLatin1String(wrapperScriptCopy.name);
    if (Utils::HostOsInfo::isWindowsHost())
        wrapperScriptCommand.chop(4); // remove the ".cmd"

    QString wrapperBinaryPath = QCoreApplication::applicationDirPath();
    if (Utils::HostOsInfo::isWindowsHost())
        wrapperBinaryPath += QLatin1String("/merssh.exe");
    else if (Utils::HostOsInfo::isMacHost())
        wrapperBinaryPath += QLatin1String("/../Resources/merssh");
    else
        wrapperBinaryPath += QLatin1String("/merssh");

    using namespace Utils;
    const QString allParameters = HostOsInfo::isWindowsHost() ? QLatin1String("%*")
                                                              : QLatin1String("$@");

    const QString scriptCopyPath = targetPath + QLatin1Char('/')
            + QLatin1String(wrapperScriptCopy.name);
    QDir targetDir(targetPath);
    const QString targetName = targetDir.dirName();
    targetDir.cdUp();
    const QString merDevToolsDir = targetDir.canonicalPath();
    QFile script(scriptCopyPath);
    ok = script.open(QIODevice::WriteOnly);
    if (!ok) {
        qWarning() << "Could not open script" << scriptCopyPath;
        return false;
    }
    const QString executionType =
            QLatin1String(wrapperScriptCopy.executionType == WrapperScript::ExecutionStandard ?
                              Constants::MER_EXECUTIONTYPE_STANDARD
                            : Constants::MER_EXECUTIONTYPE_SB2);
    const QString scriptHeader = HostOsInfo::isWindowsHost() ? QLatin1String("@echo off\n")
                                                             : QLatin1String("#!/bin/bash\n");
    const QString scriptContent = scriptHeader
            + QLatin1Char('"') + QDir::toNativeSeparators(wrapperBinaryPath) + QLatin1String("\" ")
            + QLatin1String(Mer::Constants::MERSSH_PARAMETER_SDKTOOLSDIR) + QLatin1String(" \"")
            + merDevToolsDir + QLatin1String("\" ")
            + QLatin1String(Constants::MERSSH_PARAMETER_COMMANDTYPE) + QLatin1Char(' ')
            + executionType + QLatin1Char(' ')
            + QLatin1String(Mer::Constants::MERSSH_PARAMETER_MERTARGET) + QLatin1Char(' ')
            + targetName + QLatin1Char(' ')
            + wrapperScriptCommand + QLatin1Char(' ')
            + allParameters;
    ok = script.write(scriptContent.toUtf8()) != -1;
    if (!ok) {
        qWarning() << "Could not write script" << scriptCopyPath;
        return false;
    }

    ok = QFile::setPermissions(scriptCopyPath, rwxrwx);
    if (!ok) {
        qWarning() << "Could not set file permissions on script" << scriptCopyPath;
        return false;
    }
    return ok;
}

bool MerTarget::createCacheFile(const QString &fileName, const QString &content) const
{
    bool ok = false;

    QFile file(fileName);
    ok = file.open(QIODevice::WriteOnly);
    if (!ok) {
        qWarning() << "Could not open cache file" << fileName;
        return false;
    }

    ok = file.write(content.toUtf8()) != -1;
    if (!ok) {
        qWarning() << "Could not write cache file " << fileName;
        return false;
    }
    file.close();
    return ok;
}

bool MerTarget::operator==(const MerTarget &other) const
{
    return m_name == other.m_name && m_qmakeQuery == other.m_qmakeQuery && m_gccMachineDump == other.m_gccMachineDump;
}

}
}
