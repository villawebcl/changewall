QT += core gui widgets qml quick dbus

CONFIG += c++20
CONFIG -= app_bundle

TARGET = changewall
TEMPLATE = app

SOURCES += \
    src/commandserver.cpp \
    src/main.cpp \
    src/plasmaintegration.cpp \
    src/profilemanager.cpp \
    src/shortcutmanager.cpp \
    src/transitioncontroller.cpp

HEADERS += \
    src/commandserver.h \
    src/plasmaintegration.h \
    src/profilemanager.h \
    src/shortcutmanager.h \
    src/transitioncontroller.h

RESOURCES += resources/resources.qrc

INCLUDEPATH += /usr/include/KF6 /usr/include/KF6/KGlobalAccel
LIBS += -lKF6GlobalAccel

INSTALLS += target desktop sampleconfig autostart
target.path = /usr/bin
desktop.files = packaging/changewall.desktop
desktop.path = /usr/share/applications
sampleconfig.files = config/config.json
sampleconfig.path = /usr/share/changewall
autostart.files = packaging/changewall-autostart.desktop
autostart.path = /etc/xdg/autostart
