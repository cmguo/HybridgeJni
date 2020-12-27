CONFIG -= qt

TEMPLATE = lib
DEFINES += HYBRIDGEJNI_LIBRARY

CONFIG += c++11

include(../config.pri)

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    hybridgejni.cpp \
    jnichannel.cpp \
    jniclass.cpp \
    jnimeta.cpp \
    jnitransport.cpp \
    jnivariant.cpp

HEADERS += \
    HybridgeJni_global.h \
    hybridgejni.h \
    jnichannel.h \
    jniclass.h \
    jnimeta.h \
    jnitransport.h \
    jnivariant.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

#INCLUDEPATH += "C:\Users\Brandon\AppData\Local\Android\Sdk\ndk-bundle\sources\cxx-stl\llvm-libc++\include"

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Hybridge/release/ -lHybridge
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Hybridge/debug/ -lHybridge
else:unix: LIBS += -L$$OUT_PWD/../Hybridge/ -lHybridge

INCLUDEPATH += $$PWD/../Hybridge
DEPENDPATH += $$PWD/../Hybridge
