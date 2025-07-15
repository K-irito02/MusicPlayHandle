QT += core gui widgets sql multimedia network concurrent testlib

CONFIG += c++11

VERSION = 0.1.0
QMAKE_TARGET_COMPANY = Qt6AudioPlayerTeam
QMAKE_TARGET_PRODUCT = Qt6AudioPlayer
QMAKE_TARGET_DESCRIPTION = Qt6AudioPlayerApplication

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

TARGET = musicPlayHandle
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    src/ui/dialogs/AddSongDialog.cpp \
    src/ui/dialogs/CreateTagDialog.cpp \
    src/ui/dialogs/ManageTagDialog.cpp \
    src/ui/dialogs/PlayInterface.cpp \
    src/ui/dialogs/SettingsDialog.cpp \
    src/ui/controllers/AddSongDialogController.cpp \
    src/ui/controllers/ManageTagDialogController.cpp \
    src/ui/controllers/PlayInterfaceController.cpp \
    src/ui/controllers/MainWindowController.cpp \
    src/ui/widgets/taglistitem.cpp \
    src/database/basedao.cpp \
    src/database/songdao.cpp \
    src/database/tagdao.cpp \
    src/database/playlistdao.cpp \
    src/managers/tagmanager.cpp \
    src/managers/playlistmanager.cpp \
    src/core/appconfig.cpp \
    src/core/logger.cpp \
    src/database/databasemanager.cpp \
    src/database/logdao.cpp \
    src/models/song.cpp \
    src/models/tag.cpp \
    src/models/playlist.cpp \
    src/models/playhistory.cpp \
    src/models/errorlog.cpp \
    src/models/systemlog.cpp \
    src/audio/audioengine.cpp \
    src/core/applicationmanager.cpp

HEADERS += \
    mainwindow.h \
    src/managers/tagmanager.h \
    src/managers/playlistmanager.h \
    version.h \
    src/core/appconfig.h \
    src/core/logger.h \
    src/database/databasemanager.h \
    src/database/basedao.h \
    src/database/songdao.h \
    src/database/tagdao.h \
    src/database/playlistdao.h \
    src/database/logdao.h \
    src/models/song.h \
    src/models/tag.h \
    src/models/playlist.h \
    src/models/playhistory.h \
    src/models/errorlog.h \
    src/models/systemlog.h \
    src/audio/audiotypes.h \
    src/audio/audioengine.h \
    src/core/applicationmanager.h \
    src/ui/controllers/MainWindowController.h \
    src/ui/controllers/AddSongDialogController.h \
    src/ui/controllers/PlayInterfaceController.h \
    src/ui/controllers/ManageTagDialogController.h \
    src/ui/widgets/taglistitem.h \
    src/ui/dialogs/AddSongDialog.h \
    src/ui/dialogs/PlayInterface.h \
    src/ui/dialogs/ManageTagDialog.h \
    src/ui/dialogs/SettingsDialog.h \
    src/ui/dialogs/CreateTagDialog.h

FORMS += \
    addSongDialog.ui \
    createtagdialog.ui \
    manageTagDialog.ui \
    playInterface.ui \
    settingsdialog.ui \
    mainwindow.ui

RESOURCES += \
    icon.qrc

TRANSLATIONS += \
    translations/en_US.ts

INCLUDEPATH += \
    . \
    src \
    src/core \
    src/database \
    src/models \
    src/ui \
    src/ui/visualization \
    src/ui/dialogs \
    src/threading \
    src/audio \
    src/managers \
    src/ui/controllers \
    src/utils

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32 {
    # RC_ICONS = images/applicationIcon.ico  # 暂时注释掉，因为只有svg文件
    QMAKE_TARGET_COPYRIGHT = Qt6AudioPlayerTeam
}

macx {
    ICON = images/applicationIcon.icns
    QMAKE_INFO_PLIST = Info.plist
}

unix:!macx {
    desktop.files = musicPlayHandle.desktop
    desktop.path = /usr/share/applications
    icons.files = images/applicationIcon.png
    icons.path = /usr/share/pixmaps
    INSTALLS += desktop icons
}

CONFIG(debug, debug|release) {
    DEFINES += DEBUG_MODE
    DESTDIR = debug
} else {
    DEFINES += RELEASE_MODE
    DESTDIR = release
}

OBJECTS_DIR = $$DESTDIR/obj
MOC_DIR = $$DESTDIR/moc
RCC_DIR = $$DESTDIR/rcc
UI_DIR = $$DESTDIR/ui
