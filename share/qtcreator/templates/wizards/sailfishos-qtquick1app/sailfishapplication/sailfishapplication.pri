QT += declarative

SOURCES += $$PWD/sailfishapplication.cpp
HEADERS += $$PWD/sailfishapplication.h
INCLUDEPATH += $$PWD


TARGETPATH = /usr/bin
target.path = $$TARGETPATH
DEPLOYMENT_PATH = /opt/sdk/$$TARGET/usr/share/$$TARGET
qml.path =/usr/share/$$TARGET
desktop.path =  /usr/share/$$TARGET/applications

contains(CONFIG, desktop) {
    DEFINES *= DESKTOP
    QT += opengl
}

INSTALLS += target qml desktop

DEFINES += DEPLOYMENT_PATH=\"\\\"\"$${DEPLOYMENT_PATH}/\"\\\"\"

CONFIG += link_pkgconfig
packagesExist(qdeclarative-boostable) {
    message("Building with qdeclarative-boostable support")
    DEFINES += HAS_BOOSTER
    PKGCONFIG += qdeclarative-boostable
} else {
    warning("qdeclarative-boostable not available; startup times will be slower")
}



