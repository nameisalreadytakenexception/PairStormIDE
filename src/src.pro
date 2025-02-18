#-------------------------------------------------
#
# Project created by QtCreator 2019-07-22T15:26:16
#
#-------------------------------------------------

QT       += core gui quickwidgets network quick quickcontrols2 sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = src
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++14

SOURCES += \
        main.cpp
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include($$PWD/mainwindow/mainwindow.pri)
include($$PWD/network/network.pri)
include($$PWD/chatwindow/chatwindow.pri)
include($$PWD/documentmanager/documentmanager.pri)
include($$PWD/filemanager/filemanager.pri)
include($$PWD/editor/editor.pri)
include($$PWD/bottompanel/bottompanel.pri)
include($$PWD/settings/settings.pri)
include($$PWD/startpage/startpage.pri)
include($$PWD/projectviewer/projectviewer.pri)
include($$PWD/newfilewizard/newfilewizard.pri)
include($$PWD/logindialog/logindialog.pri)
include($$PWD/splashscreen/splashscreen.pri)
include($$PWD/utils/utils.pri)
include($$PWD/paletteconfigurator/paletteconfigurator.pri)
include($$PWD/databaseaccessor/databaseaccessor.pri)
include($$PWD/settingsconfigurator/settingsconfigurator.pri)
include($$PWD/savefilesdialog/savefilesdialog.pri)
include($$PWD/classgeneration/classgeneration.pri)
include($$PWD/newprojectwizard/newprojectwizard.pri)

RESOURCES += \
    globalresources.qrc
