import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "Mer"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Qt4ProjectManager" }
    Depends { name: "RemoteLinux" }
    Depends { name: "QtcSsh" }
    Depends { name: "Qt"; submodules: ["widgets"] }
    Depends { name: "cpp" }
    cpp.defines: base.concat(["QT_NO_CAST_TO_ASCII", "QT_NO_CAST_FROM_ASCII"])

    files: [
        "meroptionswidget.ui",
        "sdkdetailswidget.ui",
        "sdkselectiondialog.ui",
        "merdeviceconfigwizardgeneralpage.ui",
        "merdeviceconfigwizardcheckpreviouskeysetupcheckpage.ui",
        "merdeviceconfigwizardreusekeyscheckpage.ui",
        "merdeviceconfigwizardkeycreationpage.ui",
        "merdeviceconfigwizarddevicetypepage.ui",
        "merdeviceconfigurationwidget.ui",
        "merplugin.cpp",
        "merqtversion.cpp",
        "merqtversionfactory.cpp",
        "mertoolchain.cpp",
        "meroptionspage.cpp",
        "sdkkitutils.cpp",
        "merdevicefactory.cpp",
        "merdeployconfigurationfactory.cpp",
        "merdeviceconfigurationwizard.cpp",
        "merdeviceconfigurationwizardsetuppages.cpp",
        "merdevice.cpp",
        "messageswindow.cpp",
        "mersdkmanager.cpp",
        "mersdk.cpp",
        "sdktoolchainutils.cpp",
        "sdktargetutils.cpp",
        "mersftpdeployconfiguration.cpp",
        "merrunconfiguration.cpp",
        "merrunconfigurationfactory.cpp",
        "merruncontrolfactory.cpp",
        "sdkscriptsutils.cpp",
        "meroptionswidget.cpp",
        "sdkdetailswidget.cpp",
        "sdkselectiondialog.cpp",
        "mervirtualmachinemanager.cpp",
        "meremulatorstartstep.cpp",
        "merdeploystepfactory.cpp",
        "merdeviceconfigurationwidget.cpp",
        "mervirtualmachinebutton.cpp",
        "mersshparser.cpp",
        "merplugin.h",
        "merconstants.h",
        "merqtversion.h",
        "merqtversionfactory.h",
        "mertoolchain.h",
        "meroptionspage.h",
        "sdkkitutils.h",
        "merdevicefactory.h",
        "mertoolchainfactory.h",
        "merdeployconfigurationfactory.h",
        "merdeviceconfigurationwizard.h",
        "merdeviceconfigurationwizardsetuppages.h",
        "merdevice.h",
        "messageswindow.h",
        "mersdkmanager.h",
        "mersdk.h",
        "sdktoolchainutils.h",
        "sdktargetutils.h",
        "mersftpdeployconfiguration.h",
        "merrunconfiguration.h",
        "merrunconfigurationfactory.h",
        "merruncontrolfactory.h",
        "sdkscriptsutils.h",
        "meroptionswidget.h",
        "sdkdetailswidget.h",
        "sdkselectiondialog.h",
        "mervirtualmachinemanager.h",
        "meremulatorstartstep.h",
        "merdeploystepfactory.h",
        "merdeviceconfigurationwidget.h",
        "mervirtualmachinebutton.h",
        "mersshparser.h",
        "mer.qrc",
        "virtualboxmanager.cpp",
        "virtualboxmanager.h",
        "mer.qrc",
        "jollawelcomepage.cpp",
        "jollawelcomepage.h",
        "mermode.cpp",
        "mermode.h",
        "mermanagementwebview.cpp",
        "mermanagementwebview.h",
        "mermanagementwebview.ui",
        "jollawelcomepage.cpp",
        "jollawelcomepage.h"
    ]
}
