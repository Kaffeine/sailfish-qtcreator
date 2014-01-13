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

#include "mersdkmanager.h"
#include "merconstants.h"
#include "merqtversion.h"
#include "mertoolchain.h"
#include "mervirtualboxmanager.h"
#include "merconnectionmanager.h"
#include "mersdkkitinformation.h"
#include "merplugin.h"
#include "merdevicefactory.h"
#include "mertarget.h"
#include "merdevice.h"
#include "merdevicexmlparser.h"
#include "mertargetkitinformation.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <utils/persistentsettings.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <ssh/sshkeygenerator.h>

#include <QDesktopServices>
#include <QDir>

#ifdef WITH_TESTS
#include <QtTest>
#endif

using namespace Mer::Constants;
using namespace ProjectExplorer;
using namespace QtSupport;
//using namespace RemoteLinux;
using namespace QSsh;
using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerSdkManager *MerSdkManager::m_instance = 0;
bool MerSdkManager::verbose = false;

static Utils::FileName globalSettingsFileName()
{
    QSettings *globalSettings = ExtensionSystem::PluginManager::globalSettings();
    return Utils::FileName::fromString(QFileInfo(globalSettings->fileName()).absolutePath()
                                       + QLatin1String(MER_SDK_FILENAME));
}

static Utils::FileName settingsFileName()
{
     QFileInfo settingsLocation(ExtensionSystem::PluginManager::settings()->fileName());
     return Utils::FileName::fromString(settingsLocation.absolutePath() + QLatin1String(MER_SDK_FILENAME));
}

MerSdkManager::MerSdkManager()
    : m_intialized(false),
      m_writer(0)
{
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), SLOT(storeSdks()));
    connect(KitManager::instance(), SIGNAL(kitsLoaded()), SLOT(initialize()));
    connect(DeviceManager::instance(), SIGNAL(devicesLoaded()), SLOT(updateDevices()));
    connect(DeviceManager::instance(), SIGNAL(deviceListChanged()), SLOT(updateDevices()));
    m_writer = new Utils::PersistentSettingsWriter(settingsFileName(), QLatin1String("MerSDKs"));
    m_instance = this;
    ProjectExplorer::KitManager::instance()->registerKitInformation(new MerSdkKitInformation);
    ProjectExplorer::KitManager::instance()->registerKitInformation(new MerTargetKitInformation);
}

MerSdkManager::~MerSdkManager()
{
    qDeleteAll(m_sdks.values());
    m_instance = 0;
}

void MerSdkManager::initialize()
{
    if (!m_intialized) {
        restore();
        //read kits
        QList<Kit*> kits = merKits();
        QList<MerToolChain*> toolchains = merToolChains();
        QList<MerQtVersion*> qtversions = merQtVersions();
        //cleanup
        foreach (MerToolChain *toolchain, toolchains) {
            const MerSdk *sdk = m_sdks.value(toolchain->virtualMachineName());
            if (sdk && sdk->targetNames().contains(toolchain->targetName()))
                continue;
            qWarning() << "MerToolChain wihout target found. Removing toolchain.";
            ToolChainManager::instance()->deregisterToolChain(toolchain);
        }

        foreach (MerQtVersion *version, qtversions) {
            const MerSdk *sdk = m_sdks.value(version->virtualMachineName());
            if (sdk && sdk->targetNames().contains(version->targetName()))
                continue;
            qWarning() << "MerQtVersion without target found. Removing qtversion.";
            QtVersionManager::instance()->removeVersion(version);
        }

        //remove broken kits
        foreach (Kit *kit, kits) {
            if (!validateKit(kit)) {
                qWarning() << "Broken Mer kit found !. Removing kit.";
                KitManager::instance()->deregisterKit(kit);
            }
        }

        // add new kits if required
        // O(n3)!!
        foreach (MerSdk *sdk, m_sdks) {
            QList<MerTarget> targets = sdk->targets();
            foreach (const MerTarget &target, targets) {
                QList<Kit*> kitsForTarget = MerSdkManager::kitsForTarget(target.name());
                // consider only those that are auto detected.
                bool success = false;
                foreach (const Kit *kit, kitsForTarget) {
                    success = kit->isAutoDetected();
                    if (success)
                        break;
                }
                if (!success)
                    sdk->addTarget(target);
            }
        }

        m_intialized = true;
        updateDevices();
        emit initialized();
    }
}

QList<Kit *> MerSdkManager::merKits() const
{
    QList<Kit*> kits;
    foreach (Kit *kit, KitManager::instance()->kits()) {
        if (isMerKit(kit))
            kits << kit;
    }
    return kits;
}

QList<MerToolChain *> MerSdkManager::merToolChains() const
{
    QList<MerToolChain*> toolchains;
    foreach (ToolChain *toolchain, ToolChainManager::instance()->toolChains()) {
        if (!toolchain->isAutoDetected())
            continue;
        if (toolchain->type() != QLatin1String(Constants::MER_TOOLCHAIN_TYPE))
            continue;
        toolchains << static_cast<MerToolChain*>(toolchain);
    }
    return toolchains;
}

QList<MerQtVersion *> MerSdkManager::merQtVersions() const
{
    QList<MerQtVersion*> qtversions;
    foreach (BaseQtVersion *qtVersion, QtSupport::QtVersionManager::instance()->versions()) {
        if (!qtVersion->isAutodetected())
            continue;
        if (qtVersion->type() != QLatin1String(Constants::MER_QT))
            continue;
        qtversions << static_cast<MerQtVersion*>(qtVersion);
    }
    return qtversions;
}

MerSdkManager *MerSdkManager::instance()
{
    QTC_CHECK(m_instance);
    return m_instance;
}

QList<MerSdk*> MerSdkManager::sdks() const
{
    return m_sdks.values();
}

void MerSdkManager::restore()
{
    //local location
    QList<MerSdk*> sdks = restoreSdks(settingsFileName());
    if(sdks.isEmpty())
        sdks = restoreSdks(globalSettingsFileName());
    foreach (MerSdk *sdk, sdks)
        addSdk(sdk);
}

QList<MerSdk*> MerSdkManager::restoreSdks(const Utils::FileName &fileName)
{
    QList<MerSdk*> result;

    Utils::PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return result;
    QVariantMap data = reader.restoreValues();

    int version = data.value(QLatin1String(MER_SDK_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return result;

    int count = data.value(QLatin1String(MER_SDK_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(MER_SDK_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap merSdkMap = data.value(key).toMap();
        MerSdk *sdk = new MerSdk();
        if (!sdk->fromMap(merSdkMap)) {
             qWarning() << sdk->virtualMachineName()<<"is configured incorrectly...";
        }
        result << sdk;
    }
    return result;
}

void MerSdkManager::storeSdks() const
{
    QVariantMap data;
    data.insert(QLatin1String(MER_SDK_FILE_VERSION_KEY), 1);
    int count = 0;
    foreach (const MerSdk* sdk, m_sdks) {
        if (!sdk->isValid()) {
            qWarning() << sdk->virtualMachineName()<<"is configured incorrectly...";
        }
        QVariantMap tmp = sdk->toMap();
        if (tmp.isEmpty())
            continue;
        data.insert(QString::fromLatin1(MER_SDK_DATA_KEY) + QString::number(count), tmp);
        ++count;
    }
    data.insert(QLatin1String(MER_SDK_COUNT_KEY), count);
    m_writer->save(data, Core::ICore::mainWindow());
}

bool MerSdkManager::isMerKit(const Kit *kit)
{
    if (!kit)
        return false;
    if (!MerSdkKitInformation::sdk(kit))
        return false;

    ToolChain* tc = ToolChainKitInformation::toolChain(kit);
    const Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(kit);
    if (tc && tc->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE))
        return true;
    if (deviceType.isValid() && MerDeviceFactory::canCreate(deviceType))
        return true;

    return false;
}

QString MerSdkManager::targetNameForKit(const Kit *kit)
{
    if (!kit || !isMerKit(kit))
        return QString();
    ToolChain *toolchain = ToolChainKitInformation::toolChain(kit);
    if (toolchain && toolchain->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE)) {
        MerToolChain *mertoolchain = static_cast<MerToolChain *>(toolchain);
        return mertoolchain->targetName();
    }
    return QString();
}

QList<Kit *> MerSdkManager::kitsForTarget(const QString &targetName)
{
    QList<Kit*> kitsForTarget;
    if (targetName.isEmpty())
        return kitsForTarget;
    const QList<Kit*> kits = KitManager::instance()->kits();
    foreach (Kit *kit, kits) {
        if (targetNameForKit(kit) == targetName)
            kitsForTarget << kit;
    }
    return kitsForTarget;
}

QString MerSdkManager::virtualMachineNameForKit(const Kit *kit)
{
    ToolChain *toolchain = ToolChainKitInformation::toolChain(kit);
    if (toolchain && toolchain->type() == QLatin1String(Constants::MER_TOOLCHAIN_TYPE)) {
        MerToolChain *mertoolchain = static_cast<MerToolChain *>(toolchain);
        return mertoolchain->virtualMachineName();
    }
    return QString();
}

bool MerSdkManager::hasMerDevice(Kit *kit)
{
    IDevice::ConstPtr dev = DeviceKitInformation::device(kit);
    if (dev.isNull())
        return false;
    return MerDeviceFactory::canCreate(dev->type());
}

QString MerSdkManager::sdkToolsDirectory()
{
    return QFileInfo(ExtensionSystem::PluginManager::settings()->fileName()).absolutePath() +
            QLatin1String(Constants::MER_SDK_TOOLS);
}

QString MerSdkManager::globalSdkToolsDirectory()
{
    return QFileInfo(ExtensionSystem::PluginManager::globalSettings()->fileName()).absolutePath() +
            QLatin1String(Constants::MER_SDK_TOOLS);
}

bool MerSdkManager::authorizePublicKey(const QString &authorizedKeysPath,
                                       const QString &pubKeyPath,
                                       QString &error)
{
    bool success = false;
    QFileInfo fi(pubKeyPath);
    if (!fi.exists()) {
        error.append(tr("Error: File %1 is missing.").arg(pubKeyPath));
        return success;
    }

    Utils::FileReader pubKeyReader;
    success = pubKeyReader.fetch(pubKeyPath);
    if (!success) {
        error.append(tr("Error: %1").arg(pubKeyReader.errorString()));
        return success;
    }

    const QByteArray publicKey = pubKeyReader.data();

    QDir sshDirectory(QFileInfo(authorizedKeysPath).absolutePath());
    if (!sshDirectory.exists() && !sshDirectory.mkpath(sshDirectory.absolutePath())) {
        error.append(tr("Error: Could not create directory %1").arg(sshDirectory.absolutePath()));
        success = false;
        return success;
    }

    QFileInfo akFi(authorizedKeysPath);
    if (!akFi.exists()) {
        //create new key
        Utils::FileSaver authKeysSaver(authorizedKeysPath, QIODevice::WriteOnly);
        authKeysSaver.write(publicKey);
        success = authKeysSaver.finalize();
        if (!success) {
            error.append(tr("Error: %1").arg(authKeysSaver.errorString()));
            return success;
        }
        QFile::setPermissions(authorizedKeysPath, QFile::ReadOwner|QFile::WriteOwner);
    } else {
        //append
        Utils::FileReader authKeysReader;
        success = authKeysReader.fetch(authorizedKeysPath);
        if (!success) {
            error.append(tr("Error: %1").arg(authKeysReader.errorString()));
            return success;
        }
        success = !authKeysReader.data().contains(publicKey);
        if (!success) {
            error.append(tr("Key already authorized ! \n %1 already in %2").arg(pubKeyPath).arg(authorizedKeysPath));
            return success;
        }
        // File does not contain the public key. Append it to file.
        Utils::FileSaver authorizedKeysSaver(authorizedKeysPath, QIODevice::Append);
        authorizedKeysSaver.write("\n");
        authorizedKeysSaver.write(publicKey);
        success = authorizedKeysSaver.finalize();
        if (!success) {
            error.append(tr("Error: %1").arg(authorizedKeysSaver.errorString()));
            return success;
        }
    }

    return success;
}

bool MerSdkManager::hasSdk(const MerSdk* sdk) const
{
    return m_sdks.contains(sdk->virtualMachineName());
}

// takes ownership
void MerSdkManager::addSdk(MerSdk *sdk)
{
    if (m_sdks.contains(sdk->virtualMachineName()))
        return;
    m_sdks.insert(sdk->virtualMachineName(), sdk);
    connect(sdk, SIGNAL(targetsChanged(QStringList)), this, SIGNAL(sdksUpdated()));
    connect(sdk, SIGNAL(privateKeyChanged(QString)), this, SIGNAL(sdksUpdated()));
    sdk->attach();
    emit sdksUpdated();
}

// pass back the ownership
void MerSdkManager::removeSdk(MerSdk *sdk)
{
    if (!m_sdks.contains(sdk->virtualMachineName()))
        return;
    m_sdks.remove(sdk->virtualMachineName());
    disconnect(sdk, SIGNAL(targetsChanged(QStringList)), this, SIGNAL(sdksUpdated()));
    disconnect(sdk, SIGNAL(privateKeyChanged(QString)), this, SIGNAL(sdksUpdated()));
    sdk->detach();
    emit sdksUpdated();
}

//ownership passed to caller
MerSdk* MerSdkManager::createSdk(const QString &vmName)
{
    MerSdk *sdk = new MerSdk();

    VirtualMachineInfo info = MerVirtualBoxManager::fetchVirtualMachineInfo(vmName);
    sdk->setVirtualMachineName(vmName);
    sdk->setSshPort(info.sshPort);
    sdk->setWwwPort(info.wwwPort);
    //TODO:
    sdk->setHost(QLatin1String(MER_SDK_DEFAULTHOST));
    //TODO:
    sdk->setUserName(QLatin1String(MER_SDK_DEFAULTUSER));
    const QString sshDirectory(QDir::fromNativeSeparators(QDesktopServices::storageLocation(
                                                              QDesktopServices::HomeLocation))+ QLatin1String("/.ssh"));
    sdk->setPrivateKeyFile(QDir::toNativeSeparators(QString::fromLatin1("%1/id_rsa").arg(sshDirectory)));
    sdk->setSharedHomePath(info.sharedHome);
    sdk->setSharedTargetsPath(info.sharedTargets);
    sdk->setSharedConfigPath(info.sharedConfig);
    sdk->setSharedSshPath(info.sharedSsh);
    return sdk;
}


MerSdk* MerSdkManager::sdk(const QString &sdkName) const
{
    return m_sdks.value(sdkName);
}

bool MerSdkManager::validateKit(const Kit *kit)
{
    if (!kit)
        return false;
    ToolChain* toolchain = ToolChainKitInformation::toolChain(kit);
    QtSupport::BaseQtVersion* version = QtSupport::QtKitInformation::qtVersion(kit);
    Core::Id deviceType = DeviceTypeKitInformation::deviceTypeId(kit);
    MerSdk* sdk = MerSdkKitInformation::sdk(kit);

    if (!version || !toolchain || !deviceType.isValid() || !sdk)
        return false;
    if (version->type() != QLatin1String(Constants::MER_QT))
        return false;
    if (toolchain->type() != QLatin1String(Constants::MER_TOOLCHAIN_TYPE))
        return false;
    if (!MerDeviceFactory::canCreate(deviceType))
        return false;

    MerToolChain* mertoolchain = static_cast<MerToolChain*>(toolchain);
    MerQtVersion* merqtversion = static_cast<MerQtVersion*>(version);

    return  sdk->virtualMachineName() ==  mertoolchain->virtualMachineName()
            && sdk->virtualMachineName() ==  merqtversion->virtualMachineName()
            && mertoolchain->targetName() == merqtversion->targetName();
}

bool MerSdkManager::generateSshKey(const QString &privKeyPath, QString &error)
{
    if (QFileInfo(privKeyPath).exists()) {
        error.append(tr("Error: File '%1' exists.\n").arg(privKeyPath));
        return false;
    }

    bool success = true;
    SshKeyGenerator keyGen;
    success = keyGen.generateKeys(SshKeyGenerator::Rsa,
                                  SshKeyGenerator::OpenSsl, 2048,
                                  SshKeyGenerator::DoNotOfferEncryption);
    if (!success) {
        error.append(tr("Error: %1\n").arg(keyGen.error()));
        return false;
    }

    Utils::FileSaver privKeySaver(privKeyPath);
    privKeySaver.write(keyGen.privateKey());
    success = privKeySaver.finalize();
    if (!success) {
        error.append(tr("Error: %1\n").arg(privKeySaver.errorString()));
        return false;
    }

    Utils::FileSaver pubKeySaver(privKeyPath + QLatin1String(".pub"));
    const QByteArray publicKey = keyGen.publicKey();
    pubKeySaver.write(publicKey);
    success = pubKeySaver.finalize();
    if (!success) {
        error.append(tr("Error: %1\n").arg(pubKeySaver.errorString()));
        return false;
    }
    return true;
}

void MerSdkManager::updateDevices()
{
    QList<MerDeviceData> devices;
    int count = DeviceManager::instance()->deviceCount();
    for(int i = 0 ;  i < count; ++i) {
        IDevice::ConstPtr d = DeviceManager::instance()->deviceAt(i);
        if (MerDeviceFactory::canCreate(d->type())) {
            const MerDevice* device = static_cast<const MerDevice*>(d.data());
            MerDeviceData xmlData;
            if(device->type() == Constants::MER_DEVICE_TYPE_ARM ) {
                xmlData.m_ip = device->sshParameters().host;
                xmlData.m_name = device->displayName();
                xmlData.m_type = device->type() == Constants::MER_DEVICE_TYPE_ARM ? QLatin1String("real") : QLatin1String ("vbox");
                xmlData.m_sshKeyPath = device->sshParameters().privateKeyFile;
            } else {
                xmlData.m_index = QString::number(device->index());
                xmlData.m_subNet = device->subnet();
                xmlData.m_mac = device->mac();
                xmlData.m_type = device->type() == Constants::MER_DEVICE_TYPE_ARM ? QLatin1String("real") : QLatin1String ("vbox");
                xmlData.m_sshKeyPath = device->sshParameters().privateKeyFile;
            }
            devices << xmlData;
        }
    }

    foreach(MerSdk* sdk, m_sdks)
    {
        const QString file = sdk->sharedConfigPath() + QLatin1String(Constants::MER_DEVICES_FILENAME);
        MerEngineData xmlData;
        xmlData.m_name = sdk->virtualMachineName();
        xmlData.m_type =  QLatin1String("vbox");
        //hardcoded/magic values on customer regest
        xmlData.m_subNet = QLatin1String("10.220.220");
        if (!file.isEmpty())
            MerDevicesXmlWriter writer(file, devices,xmlData);
    }
}

#ifdef WITH_TESTS

void MerPlugin::verifyTargets(const QString &vm, QStringList expectedKits, QStringList expectedToolChains, QStringList expectedQtVersions)
{
    MerSdkManager *sdkManager = MerSdkManager::instance();
    QList<ProjectExplorer::Kit*> kits = sdkManager->merKits();
    QList<MerToolChain*> toolchains = sdkManager->merToolChains();
    QList<MerQtVersion*> qtversions = sdkManager->merQtVersions();

    foreach (Kit* x, kits) {
        QString target = MerSdkManager::targetNameForKit(x);
        if (expectedKits.contains(target)) {
            expectedKits.removeAll(target);
            continue;
        }
        QFAIL("Unexpected kit created");
    }
    QVERIFY2(expectedKits.isEmpty(), "Expected kits not created");

    foreach (MerToolChain *x, toolchains) {
        QString target = x->targetName();
        QVERIFY2(x->virtualMachineName() == vm, "VirtualMachine name does not match");
        if (expectedToolChains.contains(target)) {
            expectedToolChains.removeAll(target);
            continue;
        }
        QFAIL("Unexpected toolchain created");
    }
    QVERIFY2(expectedToolChains.isEmpty(), "Expected toolchains not created");

    foreach (MerQtVersion *x, qtversions) {
        QString target = x->targetName();
        QVERIFY2(x->virtualMachineName() == vm, "VirtualMachine name does not match");
        if (expectedQtVersions.contains(target)) {
            expectedQtVersions.removeAll(target);
            continue;
        }
        QFAIL("Unexpected qtversion created");
    }
    QVERIFY2(expectedQtVersions.isEmpty(), "Expected qtverion not created");
}

void MerPlugin::testMerSdkManager_data()
{
    QTest::addColumn<QString>("filepath");
    QTest::addColumn<QString>("virtualMachine");
    QTest::addColumn<QStringList>("targets");
    QTest::addColumn<QStringList>("expectedTargets");
    QTest::newRow("testcase1") << "./testcase1" << "TestVM" << QStringList() << (QStringList() << QLatin1String("SailfishOS-i486-x86-1"));
    QTest::newRow("testcase2") << "./testcase2" << "TestVM" << (QStringList() << QLatin1String("SailfishOS-i486-x86-1")) << (QStringList() << QLatin1String("SailfishOS-i486-x86-2"));
    QTest::newRow("testcase3") << "./testcase3" << "TestVM" << (QStringList() << QLatin1String("SailfishOS-i486-x86-1")) << (QStringList());
}

void MerPlugin::testMerSdkManager()
{
    QFETCH(QString, filepath);
    QFETCH(QString, virtualMachine);
    QFETCH(QStringList, targets);
    QFETCH(QStringList, expectedTargets);

    const QString &initFile = filepath + QDir::separator() + QLatin1String("init.xml");
    const QString &targetFile = filepath + QDir::separator() + QLatin1String("targets.xml");
    const QString &runFile = filepath + QDir::separator() + QLatin1String("run.xml");

    QFileInfo initfi(initFile);
    QVERIFY2(initfi.exists(),"Missing init.xml");
    QFileInfo runfi(runFile);
    QVERIFY2(runfi.exists(),"Missing run.xml");

    QStringList initToolChains = targets;
    QStringList initQtVersions = targets;
    QStringList initKits = targets;

    QStringList expectedToolChains = expectedTargets;
    QStringList expectedQtVersions = expectedTargets;
    QStringList expectedKits = expectedTargets;

    foreach (const QString &target, targets) {
        if (target.isEmpty())
            break;
        QVERIFY2(QDir(filepath).mkdir(target), "Could not create fake sysroot");
    }
    foreach (const QString &target, expectedTargets) {
        if (target.isEmpty())
            break;
        QVERIFY2(QDir(filepath).mkdir(target), "Could not create fake sysroot");
    }

    QVERIFY2(QFile::copy(initFile, targetFile), "Could not copy init.xml to target.xml");

    MerSdkManager *sdkManager = MerSdkManager::instance();

    QVERIFY(sdkManager);
    QVERIFY(sdkManager->sdks().isEmpty());
    MerSdk *sdk = sdkManager->createSdk(virtualMachine);
    QVERIFY(sdk);
    QVERIFY(!sdk->isValid());

    sdk->setSharedSshPath(QDir::toNativeSeparators(filepath));
    sdk->setSharedHomePath(QDir::toNativeSeparators(filepath));
    sdk->setSharedTargetsPath(QDir::toNativeSeparators(filepath));

    QVERIFY(sdk->isValid());
    sdkManager->addSdk(sdk);

    QVERIFY(!sdkManager->sdks().isEmpty());

    verifyTargets(virtualMachine, initKits, initToolChains, initQtVersions);

    QVERIFY2(QFile::remove(targetFile), "Could not remove target.xml");
    QVERIFY2(QFile::copy(runFile, targetFile), "Could not copy run.xml to target.xml");

    QSignalSpy spy(sdk, SIGNAL(targetsChanged(QStringList)));
    int i = 0;
    while (spy.count() == 0 && i++ < 50)
        QTest::qWait(200);

    verifyTargets(virtualMachine, expectedKits, expectedToolChains, expectedQtVersions);

    sdkManager->removeSdk(sdk);

    QList<Kit*> kits = sdkManager->merKits();
    QList<MerToolChain*> toolchains = sdkManager->merToolChains();
    QList<MerQtVersion*> qtversions = sdkManager->merQtVersions();

    QVERIFY2(kits.isEmpty(), "Merkit not removed");
    QVERIFY2(toolchains.isEmpty(), "Toolchain not removed");
    QVERIFY2(qtversions.isEmpty(), "QtVersion not removed");
    QVERIFY(sdkManager->sdks().isEmpty());
    //cleanup

    QVERIFY2(QFile::remove(targetFile), "Could not remove target.xml");
    foreach (const QString &target, expectedTargets)
        QDir(filepath).rmdir(target);
    foreach (const QString &target, targets)
        QDir(filepath).rmdir(target);
}
#endif // WITH_TESTS

} // Internal
} // Mer
