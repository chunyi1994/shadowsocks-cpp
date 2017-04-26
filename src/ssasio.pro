TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -Werror
QMAKE_CXXFLAGS +=  -Wno-unused-parameter
QMAKE_CXXFLAGS +=  -Wno-unused-function
LIBS += -lpthread
LIBS += -lboost_system
LIBS += -lboost_filesystem
LIBS += -lcrypto
LIBS += -lcryptopp

SOURCES += main.cpp \
    sslocal.cpp \
    local_conn.cpp \
    utils.cpp \
    encrypt.cpp

HEADERS += \
    sslocal.h \
    local_conn.h \
    logger.h \
    utils.h \
    encrypt.h

