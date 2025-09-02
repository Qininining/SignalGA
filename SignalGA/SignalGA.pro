QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# UI
SOURCES += \
    src/main.cpp \
    UI/mainwindow/mainwindow.cpp
HEADERS += \
    UI/mainwindow/mainwindow.h
FORMS += \
    UI/mainwindow/mainwindow.ui
INCLUDEPATH += ./UI/mainwindow

# Data/AcquisitionTask
DEFINES += "DataBaseDIR=$$PWD/Data/output/"
INCLUDEPATH += Data/AcquisitionTask
SOURCES += Data/AcquisitionTask/TaskThreadManager.cpp
HEADERS += Data/AcquisitionTask/TaskThreadManager.h
# Data/DataSaver
INCLUDEPATH += Data/DataSaver
SOURCES += Data/DataSaver/DataSaver.cpp
HEADERS += Data/DataSaver/DataSaver.h

# Drivers/Scanner
INCLUDEPATH += Drivers/Scanner
SOURCES += Drivers/Scanner/Scanner.cpp
HEADERS += Drivers/Scanner/Scanner.h
win32: LIBS += -L$$PWD/Drivers/Scanner/sdk240410/64/ -lScanControl
INCLUDEPATH += Drivers/Scanner/sdk240410      # ScanControl.h
DEPENDPATH += Drivers/Scanner/sdk240410/64    # ScanControl.lib

#SerialPort
QT += serialport
INCLUDEPATH += Drivers/SerialPort
SOURCES += Drivers/SerialPort/SerialCommon.cpp
HEADERS += Drivers/SerialPort/SerialCommon.h
# Force sensor Based on SerialPort
INCLUDEPATH += Drivers/ForceSensor
SOURCES += Drivers/ForceSensor/ForceSensor.cpp
HEADERS += Drivers/ForceSensor/ForceSensor.h



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
