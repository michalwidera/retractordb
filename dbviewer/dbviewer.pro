# -------------------------------------------------
# Project created by QtCreator 2009-05-22T19:10:05
# -------------------------------------------------
TARGET = dbviewer
TEMPLATE = app
DESTDIR = ../../qtcreator-build
INCLUDEPATH += ${BOOST_ROOT}/include/boost-1_50 \
    ../share\
    ../executor \
    ../compiler
LIBS += ${BOOST_ROOT}/lib/libboost_serialization-mgw44-mt-1_50.a
SOURCES += main.cpp \
    mainwindow.cpp \
    ../share/RandomFile.cpp \
    ../share/QStruct.cpp \
    ../executor/Processor.cpp \
    ../share/IPhStrMgr.cpp \
    ../executor/CRSMath.cpp \
    ../share/Buffer.cpp \
    ../executor/SelfTest.cpp \
    ../compiler/parser.cpp \
    ../share/inputFileInstance.cpp
HEADERS += mainwindow.h \
    ../share/RandomFile.h \
    ../share/QStruct.h \
    ../executor/Processor.h \
    ../share/IPhStrMgr.h \
    ../executor/CRSMath.h \
    ../share/Buffer.h \
    ../share/tokendefset.h \
    ../share/stdafx.h \
    ../share/inputFileInstance.h \
    qdebugstream.h
FORMS += mainwindow.ui
RESOURCES += zasoby.qrc
