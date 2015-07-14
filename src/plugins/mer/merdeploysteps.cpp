/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
** Contact: http://jolla.com/
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
#include "merconstants.h"
#include "merdeploysteps.h"
#include "merdeployconfiguration.h"
#include "mersdkmanager.h"
#include "mersdkkitinformation.h"
#include "mersettings.h"
#include "mertargetkitinformation.h"
#include "meremulatordevice.h"
#include "mervirtualboxmanager.h"
#include "merrpmvalidationparser.h"
#include <utils/qtcassert.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/variablemanager.h>
#include <QMessageBox>
#include <QTimer>

using namespace ProjectExplorer;
using Core::ICore;

namespace Mer {
namespace Internal {

MerProcessStep::MerProcessStep(ProjectExplorer::BuildStepList *bsl,const Core::Id id)
    :AbstractProcessStep(bsl,id)
{

}

MerProcessStep::MerProcessStep(ProjectExplorer::BuildStepList *bsl, MerProcessStep *bs)
    :AbstractProcessStep(bsl,bs)
{

}

bool MerProcessStep::init()
{
    QmakeProjectManager::QmakeBuildConfiguration *bc = qobject_cast<QmakeProjectManager::QmakeBuildConfiguration*>(buildConfiguration());
    if (!bc)
        bc =  qobject_cast<QmakeProjectManager::QmakeBuildConfiguration*>(target()->activeBuildConfiguration());

    if (!bc) {
        addOutput(tr("Cannot deploy: No active build configuration."),
            ErrorMessageOutput);
        return false;
    }

    const MerSdk *const merSdk = MerSdkKitInformation::sdk(target()->kit());

    if (!merSdk) {
        addOutput(tr("Cannot deploy: Missing MerSdk information in the kit"),ErrorMessageOutput);
        return false;
    }

    const QString target = MerTargetKitInformation::targetName(this->target()->kit());

    if (target.isEmpty()) {
        addOutput(tr("Cannot deploy: Missing MerTarget information in the kit"),ErrorMessageOutput);
        return false;
    }

    IDevice::ConstPtr device = DeviceKitInformation::device(this->target()->kit());

    //TODO: HACK
    if (device.isNull() && dynamic_cast<MerMb2RpmBuildStep *>(this) == 0) {
        addOutput(tr("Cannot deploy: Missing MerDevice information in the kit"),ErrorMessageOutput);
        return false;
    }


    const QString projectDirectory = bc->isShadowBuild() ? bc->rawBuildDirectory().toString() : project()->projectDirectory();
    const QString wrapperScriptsDir =  MerSdkManager::sdkToolsDirectory() + merSdk->virtualMachineName()
            + QLatin1Char('/') + target;
    const QString deployCommand = wrapperScriptsDir + QLatin1Char('/') + QLatin1String(Constants::MER_WRAPPER_DEPLOY);

    ProcessParameters *pp = processParameters();

    Utils::Environment env = bc ? bc->environment() : Utils::Environment::systemEnvironment();

    // By default, MER_SSH_PROJECT_PATH is set to the project explorer variable
    // "%{CurrentProject:Path}". If multiple projects are open, "CurrentProject" may
    // not be the project set as active (i.e. the project being deployed), leading
    // to RPM build looking for files in the wrong project. Instead, MER_SSH_PROJECT_PATH
    // needs to be set to the source directory of the active project. Note that this path
    // is the same, whether or not shadow builds are enabled.
    env.set(QLatin1String(Constants::MER_SSH_PROJECT_PATH), project()->projectDirectory());

    //TODO HACK
    if(!device.isNull())
        env.appendOrSet(QLatin1String(Constants::MER_SSH_DEVICE_NAME),device->displayName());
    pp->setMacroExpander(bc ? bc->macroExpander() : Core::VariableManager::macroExpander());
    pp->setEnvironment(env);
    pp->setWorkingDirectory(projectDirectory);
    pp->setCommand(deployCommand);
    pp->setArguments(arguments());
    return AbstractProcessStep::init();
}

QString MerProcessStep::arguments() const
{
    return m_arguments;
}

void MerProcessStep::setArguments(const QString &arguments)
{
    m_arguments = arguments;
}

MerDeployConfiguration *MerProcessStep::deployConfiguration() const
{
    return qobject_cast<MerDeployConfiguration *>(AbstractProcessStep::deployConfiguration());
}

///////////////////////////////////////////////////////////////////////////////////////////

const Core::Id MerEmulatorStartStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerEmulatorStartStep");
}

QString MerEmulatorStartStep::displayName()
{
    return tr("Start Emulator");
}

MerEmulatorStartStep::MerEmulatorStartStep(BuildStepList *bsl)
    : MerAbstractVmStartStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}

MerEmulatorStartStep::MerEmulatorStartStep(ProjectExplorer::BuildStepList *bsl, MerEmulatorStartStep *bs)
    : MerAbstractVmStartStep(bsl, bs)
{
    setDefaultDisplayName(displayName());
}

bool MerEmulatorStartStep::init()
{
    IDevice::ConstPtr d = DeviceKitInformation::device(this->target()->kit());
    const MerEmulatorDevice* device = dynamic_cast<const MerEmulatorDevice*>(d.data());
    if (!device) {
        setEnabled(false);
        return false;
    }

    setConnection(device->connection());

    return MerAbstractVmStartStep::init();
}

///////////////////////////////////////////////////////////////////////////////////////////

const Core::Id MerMb2RsyncDeployStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerRsyncDeployStep");
}

QString MerMb2RsyncDeployStep::displayName()
{
    return QLatin1String("Rsync");
}

MerMb2RsyncDeployStep::MerMb2RsyncDeployStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--rsync"));
}

MerMb2RsyncDeployStep::MerMb2RsyncDeployStep(ProjectExplorer::BuildStepList *bsl, MerMb2RsyncDeployStep *bs)
    :MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--rsync"));
}

bool MerMb2RsyncDeployStep::init()
{
    return MerProcessStep::init();
}

bool MerMb2RsyncDeployStep::immutable() const
{
    return false;
}

void MerMb2RsyncDeployStep::run(QFutureInterface<bool> &fi)
{
   emit addOutput(tr("Deploying binaries..."), MessageOutput);
   AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerMb2RsyncDeployStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Deploys with rsync."));
    return widget;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

const Core::Id MerLocalRsyncDeployStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerLocalRsyncDeployStep");
}

QString MerLocalRsyncDeployStep::displayName()
{
    return QLatin1String("Local Rsync");
}

MerLocalRsyncDeployStep::MerLocalRsyncDeployStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}

MerLocalRsyncDeployStep::MerLocalRsyncDeployStep(ProjectExplorer::BuildStepList *bsl, MerLocalRsyncDeployStep *bs)
    :MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
}

bool MerLocalRsyncDeployStep::init()
{
    QmakeProjectManager::QmakeBuildConfiguration *bc = qobject_cast<QmakeProjectManager::QmakeBuildConfiguration*>(buildConfiguration());
    if (!bc)
        bc =  qobject_cast<QmakeProjectManager::QmakeBuildConfiguration*>(target()->activeBuildConfiguration());

    if (!bc) {
        addOutput(tr("Cannot deploy: No active build configuration."),
            ErrorMessageOutput);
        return false;
    }

    const MerSdk *const merSdk = MerSdkKitInformation::sdk(target()->kit());

    if (!merSdk) {
        addOutput(tr("Cannot deploy: Missing MerSdk information in the kit"),ErrorMessageOutput);
        return false;
    }

    const QString target = MerTargetKitInformation::targetName(this->target()->kit());

    if (target.isEmpty()) {
        addOutput(tr("Cannot deploy: Missing MerTarget information in the kit"),ErrorMessageOutput);
        return false;
    }

    const QString projectDirectory = bc->isShadowBuild() ? bc->rawBuildDirectory().toString() : project()->projectDirectory();
    const QString deployCommand = QLatin1String("rsync");

    ProcessParameters *pp = processParameters();

    Utils::Environment env = bc ? bc->environment() : Utils::Environment::systemEnvironment();
    pp->setMacroExpander(bc ? bc->macroExpander() : Core::VariableManager::macroExpander());
    pp->setEnvironment(env);
    pp->setWorkingDirectory(projectDirectory);
    pp->setCommand(deployCommand);
    pp->setArguments(arguments());

    return AbstractProcessStep::init();
}

bool MerLocalRsyncDeployStep::immutable() const
{
    return false;
}

void MerLocalRsyncDeployStep::run(QFutureInterface<bool> &fi)
{
   emit addOutput(tr("Deploying binaries..."), MessageOutput);
   AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerLocalRsyncDeployStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Deploys with local installed rsync."));
    widget->setCommandText(tr("Deploy using local installed Rsync"));
    return widget;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////


const Core::Id MerMb2RpmDeployStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerRpmDeployStep");
}

QString MerMb2RpmDeployStep::displayName()
{
    return QLatin1String("RPM");
}

MerMb2RpmDeployStep::MerMb2RpmDeployStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--sdk"));
}


MerMb2RpmDeployStep::MerMb2RpmDeployStep(ProjectExplorer::BuildStepList *bsl, MerMb2RpmDeployStep *bs)
    :MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--sdk"));
}

bool MerMb2RpmDeployStep::init()
{
    return MerProcessStep::init();
}

bool MerMb2RpmDeployStep::immutable() const
{
    return false;
}

void MerMb2RpmDeployStep::run(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Deploying RPM package..."), MessageOutput);
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerMb2RpmDeployStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Deploys RPM package."));
    return widget;
}

//TODO: HACK
////////////////////////////////////////////////////////////////////////////////////////////////////////////


const Core::Id MerMb2RpmBuildStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerRpmBuildStep");
}

QString MerMb2RpmBuildStep::displayName()
{
    return QLatin1String("RPM");
}

MerMb2RpmBuildStep::MerMb2RpmBuildStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}


MerMb2RpmBuildStep::MerMb2RpmBuildStep(ProjectExplorer::BuildStepList *bsl, MerMb2RpmBuildStep *bs)
    :MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
}

bool MerMb2RpmBuildStep::init()
{
    bool success = MerProcessStep::init();
    m_packages.clear();
    const MerSdk *const sdk = MerSdkKitInformation::sdk(target()->kit());
    m_sharedHome = QDir::cleanPath(sdk->sharedHomePath());
    m_sharedSrc = QDir::cleanPath(sdk->sharedSrcPath());

    //hack
    ProcessParameters *pp = processParameters();
    QString deployCommand = pp->command();
    deployCommand.replace(QLatin1String(Constants::MER_WRAPPER_DEPLOY),QLatin1String(Constants::MER_WRAPPER_RPM));
    pp->setCommand(deployCommand);
    return success;
}

bool MerMb2RpmBuildStep::immutable() const
{
    return false;
}
//TODO: This is hack
void MerMb2RpmBuildStep::run(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Building RPM package..."), MessageOutput);
    AbstractProcessStep::run(fi);
}
//TODO: This is hack
void MerMb2RpmBuildStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    //TODO:
    MerProcessStep::processFinished(exitCode, status);
    if(exitCode == 0 && status == QProcess::NormalExit && !m_packages.isEmpty()){
        new RpmInfo(m_packages);
    }
}

QStringList MerMb2RpmBuildStep::packagesFilePath() const
{
    return m_packages;
}

BuildStepConfigWidget *MerMb2RpmBuildStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Builds RPM package."));
    widget->setCommandText(QLatin1String("mb2 rpm"));
    return widget;
}

void MerMb2RpmBuildStep::stdOutput(const QString &line)
{
    QRegExp rexp(QLatin1String("^Wrote: (/.*RPMS.*\\.rpm)"));
    if (rexp.indexIn(line) != -1) {
        QString file = rexp.cap(1);
        //TODO First replace shared home and then shared src (error prone!)
        file.replace(QRegExp(QLatin1String("^/home/mersdk/share")),m_sharedHome);
        file.replace(QRegExp(QLatin1String("^/home/src1")),m_sharedSrc);
        m_packages.append(QDir::toNativeSeparators(file));
    }
    MerProcessStep::stdOutput(line);
}

RpmInfo::RpmInfo(const QStringList& list):
    m_list(list)
{
    QTimer::singleShot(0,this,SLOT(info()));
}

void RpmInfo::info()
{
    QString message(QLatin1String("Following packages have been created:"));
    message.append(QLatin1String("<ul>"));
    foreach(const QString file,m_list) {
        message.append(QLatin1String("<li>"));
        message.append(file);
        message.append(QLatin1String("</li>"));
    }
    message.append(QLatin1String("</ul>"));
    QMessageBox::information(ICore::mainWindow(), tr("Packages created"),message);
    this->deleteLater();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

const Core::Id MerRpmValidationStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerRpmValidationStep");
}

QString MerRpmValidationStep::displayName()
{
    return QLatin1String("RPM Validation");
}

MerRpmValidationStep::MerRpmValidationStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setEnabled(MerSettings::rpmValidationByDefault());
    setDefaultDisplayName(displayName());
}


MerRpmValidationStep::MerRpmValidationStep(ProjectExplorer::BuildStepList *bsl, MerRpmValidationStep *bs)
    :MerProcessStep(bsl,bs)
{
    setEnabled(MerSettings::rpmValidationByDefault());
    setDefaultDisplayName(displayName());
}

bool MerRpmValidationStep::init()
{
    if (!MerProcessStep::init()) {
        return false;
    }

    m_packagingStep = deployConfiguration()->earlierBuildStep<MerMb2RpmBuildStep>(this);
    if (!m_packagingStep) {
        emit addOutput(tr("Cannot validate: No previous \"%1\" step found")
                .arg(MerMb2RpmBuildStep::displayName()),
                ErrorMessageOutput);
        return false;
    }

    setOutputParser(new MerRpmValidationParser);

    return true;
}

bool MerRpmValidationStep::immutable() const
{
    return false;
}

void MerRpmValidationStep::run(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Validating RPM package..."), MessageOutput);

    const QString packageFile = m_packagingStep->packagesFilePath().first();
    if(!packageFile.endsWith(QLatin1String(".rpm"))){
        const QString message((tr("No package to validate found in %1")).arg(packageFile));
        emit addOutput(message, ErrorMessageOutput);
        fi.reportResult(false);
        emit finished();
        return;
    }

    // hack
    ProcessParameters *pp = processParameters();
    QString deployCommand = pp->command();
    deployCommand.replace(QLatin1String(Constants::MER_WRAPPER_DEPLOY),QLatin1String(Constants::MER_WRAPPER_RPMVALIDATION));
    pp->setCommand(deployCommand);
    pp->setArguments(packageFile);

    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerRpmValidationStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Validates RPM package."));
    widget->setCommandText(QLatin1String("rpmvalidation"));
    return widget;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


MerDeployStepWidget::MerDeployStepWidget(MerProcessStep *step)
        : m_step(step)
{
    m_ui.setupUi(this);
    m_ui.commandArgumentsLineEdit->setText(m_step->arguments());
    connect(m_ui.commandArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(commandArgumentsLineEditTextEdited()));
}

QString MerDeployStepWidget::summaryText() const
{
    return QString::fromLatin1("<b>%1:</b> %2").arg(displayName()).arg(m_summaryText);
}

QString MerDeployStepWidget::displayName() const
{
    return m_displayText;
}

void MerDeployStepWidget::commandArgumentsLineEditTextEdited()
{
    m_step->setArguments(m_ui.commandArgumentsLineEdit->text());
}

void MerDeployStepWidget::setCommandText(const QString& commandText)
{
     m_ui.commandLabelEdit->setText(commandText);
}

void MerDeployStepWidget::setDisplayName(const QString& displayText)
{
    m_displayText = displayText;
}

void MerDeployStepWidget::setSummaryText(const QString& summaryText)
{
    m_summaryText = summaryText;
}

QString MerDeployStepWidget::commnadText() const
{
   return  m_ui.commandLabelEdit->text();
}

} // Internal
} // Mer
