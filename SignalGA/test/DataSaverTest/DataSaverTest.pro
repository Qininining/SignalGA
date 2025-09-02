QT += core
CONFIG += c++17 console
CONFIG -= app_bundle
TEMPLATE = app

SOURCES += \
    main.cpp \
    ../../Data/DataSaver/DataSaver.cpp

HEADERS += \
    ../../Data/DataSaver/DataSaver.h

INCLUDEPATH += ../../Data/DataSaver

# 输出目录
DESTDIR = ./build
