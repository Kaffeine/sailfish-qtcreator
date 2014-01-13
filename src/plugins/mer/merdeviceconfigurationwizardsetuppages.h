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

#ifndef MERDEVICECONFIGURATIONWIZARDSETUPPAGES_H
#define MERDEVICECONFIGURATIONWIZARDSETUPPAGES_H

#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <ssh/sshconnection.h>

#include <QWizardPage>

namespace Mer {
namespace Internal {

namespace Ui {
class MerDeviceConfigWizardDeviceTypePage;
class MerDeviceConfigWizardGeneralPage;
class MerDeviceConfigWizardStartPage;
class MerDeviceConfigWizardCheckPreviousKeySetupPage;
class MerDeviceConfigWizardReuseKeysCheckPage;
class MerDeviceConfigWizardKeyCreationPage;
}

struct WizardData
{
    QString configName;
    QString hostName;
    QSsh::SshConnectionParameters::AuthenticationType authType;
    ProjectExplorer::IDevice::MachineType machineType;
    QString privateKeyFilePath;
    QString publicKeyFilePath;
    QString userName;
    QString password;
    QString freePorts;
    int sshPort;
};

class MerDeviceConfigWizardDeviceTypePage : public QWizardPage
{
public:
    explicit MerDeviceConfigWizardDeviceTypePage(QWidget *parent = 0);

    virtual bool isComplete() const;
    ProjectExplorer::IDevice::MachineType machineType() const;

private:
    Ui::MerDeviceConfigWizardDeviceTypePage *m_ui;
};

class MerDeviceConfigWizardGeneralPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit MerDeviceConfigWizardGeneralPage(const WizardData &wizardData, QWidget *parent = 0);

    virtual void initializePage();
    virtual bool isComplete() const;
    QString configName() const;
    QString hostName() const;
    QString userName() const;
    QString password() const;
    QString freePorts() const;
    QSsh::SshConnectionParameters::AuthenticationType authType() const;
    int sshPort() const;

private slots:
    void authTypeChanged();
    void onVirtualMachineChanged(const QString &vmName);

private:
    Ui::MerDeviceConfigWizardGeneralPage *m_ui;
    const WizardData &m_wizardData;
};

class MerDeviceConfigWizardPreviousKeySetupCheckPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit MerDeviceConfigWizardPreviousKeySetupCheckPage(QWidget *parent = 0);

    virtual bool isComplete() const;
    virtual void initializePage();

    bool keyBasedLoginWasSetup() const;
    QString privateKeyFilePath() const;

private slots:
    void handleSelectionChanged();

private:
    Ui::MerDeviceConfigWizardCheckPreviousKeySetupPage *m_ui;
};

class MerDeviceConfigWizardReuseKeysCheckPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit MerDeviceConfigWizardReuseKeysCheckPage(QWidget *parent = 0);

    virtual bool isComplete() const;

    virtual void initializePage();

    bool reuseKeys() const;

    QString privateKeyFilePath() const;
    QString publicKeyFilePath() const;

private slots:
    void handleSelectionChanged();

private:
    Ui::MerDeviceConfigWizardReuseKeysCheckPage *m_ui;
};

class MerDeviceConfigWizardKeyCreationPage : public QWizardPage
{
    Q_OBJECT
public:
    MerDeviceConfigWizardKeyCreationPage(const WizardData &wizardData, QWidget *parent);
    QString privateKeyFilePath() const;
    QString publicKeyFilePath() const;

    virtual void initializePage();
    virtual bool isComplete() const;

private slots:
    void createKeys();
    void authorizeKeys();

private:
    bool saveFile(const QString &filePath, const QByteArray &data);
    void enableInput();

private:
    Ui::MerDeviceConfigWizardKeyCreationPage *m_ui;
    bool m_isComplete;
    const WizardData &m_wizardData;
};

class MerDeviceConfigWizardFinalPage :
        public RemoteLinux::GenericLinuxDeviceConfigurationWizardFinalPage
{
    Q_OBJECT
public:
    MerDeviceConfigWizardFinalPage(const WizardData &wizardData, QWidget *parent);

private slots:
    void startEmulator();

private:
    QString infoText() const;

private:
    const WizardData &m_wizardData;
};

} // Mer
} // Internal

#endif // MERDEVICECONFIGURATIONWIZARDSETUPPAGES_H
