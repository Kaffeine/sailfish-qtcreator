/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLPROJECT_H
#define QMLPROJECT_H

#include "qmlprojectmanager.h"
#include "qmlprojectnodes.h"
#include "qmlprojectmanager_global.h"
#include "fileformat/qmlprojectitem.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/filewatcher.h>
#include <coreplugin/ifile.h>

#include <QtCore/QDir>
#include <QtDeclarative/QmlEngine>

namespace QmlJSEditor {
class QmlModelManagerInterface;
}

namespace QmlProjectManager {

class QmlProject;
class QmlRunConfiguration;

namespace Internal {

class QmlMakeStep;

class QmlProjectFile : public Core::IFile
{
    Q_OBJECT

public:
    QmlProjectFile(QmlProject *parent, QString fileName);
    virtual ~QmlProjectFile();

    virtual bool save(const QString &fileName = QString());
    virtual QString fileName() const;

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;

    virtual bool isModified() const;
    virtual bool isReadOnly() const;
    virtual bool isSaveAsAllowed() const;

    virtual void modified(ReloadBehavior *behavior);

private:
    QmlProject *m_project;
    QString m_fileName;
};

class QmlRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit QmlRunConfigurationFactory(QObject *parent = 0);
    virtual ~QmlRunConfigurationFactory();

    // used to show the list of possible additons to a project, returns a list of types
    virtual QStringList availableCreationIds(ProjectExplorer::Project *pro) const;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(const QString &id) const;

    virtual bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    virtual ProjectExplorer::RunConfiguration *create(ProjectExplorer::Project *project, const QString &id);

    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    virtual ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Project *parent, const QVariantMap &map);

    virtual bool canClone(ProjectExplorer::Project *parent, ProjectExplorer::RunConfiguration *source) const;
    virtual ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Project *parent, ProjectExplorer::RunConfiguration *source);
};

class QmlRunControl : public ProjectExplorer::RunControl {
    Q_OBJECT
public:
    explicit QmlRunControl(QmlRunConfiguration *runConfiguration, bool debugMode);
    virtual ~QmlRunControl ();

    // RunControl
    virtual void start();
    virtual void stop();
    virtual bool isRunning() const;

private slots:
    void processExited(int exitCode);
    void slotBringApplicationToForeground(qint64 pid);
    void slotAddToOutputWindow(const QString &line);
    void slotError(const QString & error);

private:
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;

    QString m_executable;
    QStringList m_commandLineArguments;
    bool m_debugMode;
};

class QmlRunControlFactory : public ProjectExplorer::IRunControlFactory {
    Q_OBJECT
public:
    explicit QmlRunControlFactory(QObject *parent = 0);
    virtual ~QmlRunControlFactory();

    // IRunControlFactory
    virtual bool canRun(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode) const;
    virtual ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode);
    virtual QString displayName() const;
    virtual QWidget *configurationWidget(ProjectExplorer::RunConfiguration *runConfiguration);
};

} // namespace Internal

class QMLPROJECTMANAGER_EXPORT QmlProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    QmlProject(Internal::Manager *manager, const QString &filename);
    virtual ~QmlProject();

    QString filesFileName() const;

    virtual QString displayName() const;
    virtual Core::IFile *file() const;
    virtual Internal::Manager *projectManager() const;
    virtual ProjectExplorer::IBuildConfigurationFactory *buildConfigurationFactory() const;

    virtual QList<ProjectExplorer::Project *> dependsOn();

    virtual bool isApplication() const;
    virtual bool hasBuildSettings() const;

    virtual ProjectExplorer::BuildConfigWidget *createConfigWidget();
    virtual QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    virtual Internal::QmlProjectNode *rootProjectNode() const;
    virtual QStringList files(FilesMode fileMode) const;

    enum RefreshOption {
        ProjectFile   = 0x01,
        Files         = 0x02,
        Configuration = 0x04,
        Everything    = ProjectFile | Files | Configuration
    };
    Q_DECLARE_FLAGS(RefreshOptions,RefreshOption)

    void refresh(RefreshOptions options);

    QDir projectDir() const;
    QStringList files() const;

private slots:
    void refreshProjectFile();
    void refreshFiles();

protected:
    virtual void saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer);
    virtual bool restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader);

private:
    // plain format
    void parseProject(RefreshOptions options);
    QStringList convertToAbsoluteFiles(const QStringList &paths) const;

    Internal::Manager *m_manager;
    QString m_fileName;
    Internal::QmlProjectFile *m_file;
    QString m_projectName;
    QmlJSEditor::QmlModelManagerInterface *m_modelManager;

    // plain format
    QStringList m_files;

    // qml based, new format
    QmlEngine m_engine;
    QWeakPointer<QmlProjectItem> m_projectItem;
    ProjectExplorer::FileWatcher *m_fileWatcher;

    Internal::QmlProjectNode *m_rootNode;
};

class QMLPROJECTMANAGER_EXPORT QmlRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class Internal::QmlRunConfigurationFactory;

public:
    QmlRunConfiguration(QmlProject *parent);
    virtual ~QmlRunConfiguration();

    QmlProject *qmlProject() const;

    QString viewerPath() const;
    QStringList viewerArguments() const;
    QString workingDirectory() const;
    uint debugServerPort() const;

    // RunConfiguration
    virtual QWidget *configurationWidget();

    QVariantMap toMap() const;

private slots:
    QString mainScript() const;
    void setMainScript(const QString &scriptFile);
    void onQmlViewerChanged();
    void onQmlViewerArgsChanged();
    void onDebugServerPortChanged();

protected:
    QmlRunConfiguration(QmlProject *parent, QmlRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);

private:
    void ctor();

    QString m_scriptFile;
    QString m_qmlViewerCustomPath;
    QString m_qmlViewerDefaultPath;
    QString m_qmlViewerArgs;
    uint m_debugServerPort;
};

} // namespace QmlProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmlProjectManager::QmlProject::RefreshOptions)

#endif // QMLPROJECT_H
