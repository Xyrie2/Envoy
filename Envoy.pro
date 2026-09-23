QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG -= debug_and_release

VERSION = 1.0.0
DEFINES += QT_DEPRECATED_WARNINGS

TARGET = Envoy
TEMPLATE = app

RC_ICONS = Resources/app.ico
RESOURCES += Resources/app.qrc

CONFIG(debug, debug|release) {
    BUILD_SUB = debug
} else {
    BUILD_SUB = release
}
BUILD_DIR = $$PWD/Build/$$BUILD_SUB
DESTDIR = $$BUILD_DIR/bin
OBJECTS_DIR = $$BUILD_DIR/obj
MOC_DIR = $$BUILD_DIR/moc
RCC_DIR = $$BUILD_DIR/rcc
UI_DIR = $$BUILD_DIR/ui

INCLUDEPATH += $$PWD/Include $$PWD/Src
DEPENDPATH += $$PWD/Include $$PWD/Src

SOURCES += \
    Src/App/main.cpp \
    Src/App/MainWindow.cpp \
    Src/Core/RadarConfig.cpp \
    Src/Core/RadarProtocol.cpp \
    Src/Core/RadarTrackRules.cpp \
    Src/Core/RadarTrackStore.cpp \
    Src/Core/RadarTrackText.cpp \
    Src/Net/UdpClass.cpp \
    Src/Ui/AppMessageDialog.cpp \
    Src/Ui/AppStatusBar.cpp \
    Src/Ui/ChannelEditDialog.cpp \
    Src/Ui/CollapsibleGroupBox.cpp \
    Src/Ui/ControlPanel.cpp \
    Src/Ui/DisplaySettingsPanel.cpp \
    Src/Ui/FramelessWindow.cpp \
    Src/Ui/MapView.cpp \
    Src/Ui/NetworkSettingsPanel.cpp \
    Src/Ui/TargetListPanel.cpp \
    Src/Ui/TitleBar.cpp \
    Src/Ui/TrackTableController.cpp

HEADERS += \
    Include/App/MainWindow.h \
    Include/Core/RadarConfig.h \
    Include/Core/RadarConstants.h \
    Include/Core/RadarProtocol.h \
    Include/Core/RadarTrackRules.h \
    Include/Core/RadarTrackStore.h \
    Include/Core/RadarTrackText.h \
    Include/Net/UdpClass.h \
    Include/Ui/AppMessageDialog.h \
    Include/Ui/AppStatusBar.h \
    Include/Ui/ChannelEditDialog.h \
    Include/Ui/CollapsibleGroupBox.h \
    Include/Ui/ControlPanel.h \
    Include/Ui/DisplaySettingsPanel.h \
    Include/Ui/FramelessWindow.h \
    Include/Ui/MapGeometry.h \
    Include/Ui/MapView.h \
    Include/Ui/NetworkSettingsPanel.h \
    Include/Ui/TargetListPanel.h \
    Include/Ui/Theme.h \
    Include/Ui/TitleBar.h \
    Include/Ui/TrackTableController.h

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
