QT       += core gui bluetooth network
WIN32:
CONFIG += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    agent.cpp \
    characteristicinfo.cpp \
    controller.cpp \
    device.cpp \
    deviceinfo.cpp \
    httpsclient.cpp \
    main.cpp \
    mainwindow.cpp \
    service.cpp \
    serviceinfo.cpp

HEADERS += \
    agent.h \
    characteristicinfo.h \
    controller.h \
    device.h \
    deviceinfo.h \
    httpsclient.h \
    mainwindow.h \
    service.h \
    serviceinfo.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
