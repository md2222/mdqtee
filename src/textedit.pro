QT += widgets gui-private network
requires(qtConfig(filedialog))
qtHaveModule(printsupport): QT += printsupport

TEMPLATE        = app
TARGET          = textedit

HEADERS         = textedit.h \
    ccolortoolbutton.h \
    cconfigdialog.h \
    cconfiglist.h \
    cfindtextpanel.h \
    ctabwidget.h \
    ctextedit.h \
    mainwindow.h \
    qzipreader_p.h \
    qzipwriter_p.h
SOURCES         = textedit.cpp \
                  ccolortoolbutton.cpp \
                  cconfigdialog.cpp \
                  cconfiglist.cpp \
                  cfindtextpanel.cpp \
                  ctabwidget.cpp \
                  ctextedit.cpp \
                  main.cpp \
                  mainwindow.cpp

FORMS += \
    mainwindow.ui


RESOURCES += textedit.qrc
build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

include(../../qt-solutions/qtsingleapplication/src/qtsingleapplication.pri)


EXAMPLE_FILES = textedit.qdoc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/richtext/textedit
INSTALLS += target

QMAKE_CXXFLAGS += "-fno-sized-deallocation"

#CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

FORMS += \
    cconfigdialog.ui
