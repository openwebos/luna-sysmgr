
TEMPLATE = app

RESOURCES += qwebpage.qrc

CONFIG += qt no_keywords

QT += webkit network

SOURCES = main.cpp sysmgrwebbridge.cpp webappbase.cpp
HEADERS = sysmgrwebbridge.h webappbase.h

QMAKE_CXXFLAGS += -fno-rtti -fno-exceptions

linux-g++-64 {
	include(desktop.pri)
}

linux-armv7-g++|linux-armv6-g++ {
	include(device.pri)
}

MOC_DIR = $$DESTDIR/.moc
OBJECTS_DIR = $$DESTDIR/.obj

TARGET = webtest

LIBS += -lpbnjson_cpp
