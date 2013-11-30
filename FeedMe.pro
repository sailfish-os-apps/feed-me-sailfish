
################## CONFIG ########################

TARGET       = harbour-feedme
TEMPLATE     = app
QT          += core network sql gui qml quick
INCLUDEPATH += /usr/include/sailfishapp

############### SOURCES & CONTENT #####################

SOURCES     += \
    src/main.cpp

HEADERS     += \


OTHER_FILES += \
    harbour-feedme.desktop \
    harbour-feedme.png \
    harbour-feedme.svg \
    rpm/harbour-feedme.yaml \
    qml/harbour-feedme.qml \
    qml/Ajax.js \
    qml/cover/CoverPage.qml \
    qml/pages/StreamPage.qml \
    qml/pages/MainPage.qml \
    qml/pages/ContentPage.qml

RESOURCES   += \
    data.qrc

################## PACKAGING ########################

CONFIG       += link_pkgconfig
PKGCONFIG    += sailfishapp

target.files  = $${TARGET}
target.path   = /usr/bin
desktop.files = $${TARGET}.desktop
desktop.path  = /usr/share/applications
icon.files    = $${TARGET}.png
icon.path     = /usr/share/icons/hicolor/86x86/apps
INSTALLS     += target desktop icon
