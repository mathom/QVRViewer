#-------------------------------------------------
#
# Project created by QtCreator 2016-09-28T00:52:41
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QVRViewer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    vrview.cpp

HEADERS  += mainwindow.h \
    vrview.h \
    modelformats.h

FORMS    += mainwindow.ui

# from http://stackoverflow.com/a/10058744
# Copies the given files to the destination directory
defineTest(copyToDestdir) {
    files = $$1

    for(FILE, files) {
        CONFIG( debug, debug|release ) {
            DDIR = $${OUT_PWD}/debug
        } else {
            DDIR = $${OUT_PWD}/release
        }

        # Replace slashes in paths with backslashes for Windows
        win32:FILE ~= s,/,\\,g
        win32:DDIR ~= s,/,\\,g

        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$FILE) $$quote($$DDIR) $$escape_expand(\\n\\t)
    }

    export(QMAKE_POST_LINK)
}


win32 {
    RC_FILE = win32.rc

    contains(QT_ARCH, i386) {
message("32 bit build")
        LIBS += -L$$PWD/extern/openvr/lib/win32/ \
                -lopenvr_api -lopengl32
        copyToDestdir($${PWD}/extern/openvr/bin/win32/openvr_api.dll)
    } else {
message("64 bit build")
        LIBS += -L$$PWD/extern/openvr/lib/win64/ \
                -lopenvr_api -lopengl32
        copyToDestdir($${PWD}/extern/openvr/bin/win64/openvr_api.dll)
    }
}

INCLUDEPATH += $$PWD/extern/openvr/headers $$PWD/extern/glew/include


# from http://stackoverflow.com/a/25193580
isEmpty(TARGET_EXT) {
    win32 {
        TARGET_CUSTOM_EXT = .exe
    }
    macx {
        TARGET_CUSTOM_EXT = .app
    }
} else {
    TARGET_CUSTOM_EXT = $${TARGET_EXT}
}

win32 {
    DEPLOY_COMMAND = windeployqt
}
macx {
    DEPLOY_COMMAND = macdeployqt
}

CONFIG( debug, debug|release ) {
    # debug
    DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/debug/$${TARGET}$${TARGET_CUSTOM_EXT}))
} else {
    # release
    DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/release/$${TARGET}$${TARGET_CUSTOM_EXT}))
}

QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET} $$escape_expand(\\n\\t)

RESOURCES += \
    resources.qrc

DISTFILES += \
    win32.rc \
    viewer.ico
