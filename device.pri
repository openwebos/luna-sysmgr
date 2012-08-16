# @@@LICENSE
#
#      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# LICENSE@@@
TARGET_TYPE = TARGET_DEVICE

QMAKE_MAKEFILE = Makefile

BUILD_TYPE = release
CONFIG -= debug
CONFIG += release

LIBS += \
        -lluna-prefs -lPmLogLib -lhid \
        -lmemchute \
        -lmedia-api -lrolegen -lserviceinstall -laffinity \


MACHINE_NAME = $$(MACHINE)

contains(MACHINE_NAME, "chile") {
	DEFINES += MACHINE_CHILE
	CONFIG_BUILD += opengl
}
contains(MACHINE_NAME, "castle") {
	DEFINES += MACHINE_CASTLE
	CONFIG_BUILD += opengl texturesharing fb1poweroptimization directrendering
	CONFIG_BUILD += haptics
}
contains(MACHINE_NAME, "roadrunner") {
	DEFINES += MACHINE_CASTLE MACHINE_ROADRUNNER
	CONFIG_BUILD += opengl texturesharing fb1poweroptimization directrendering 
	CONFIG_BUILD += haptics
}
contains(MACHINE_NAME, "pixie") {
	DEFINES += MACHINE_PIXIE
	CONFIG_BUILD += opengl texturesharing fb1poweroptimization directrendering
	CONFIG_BUILD += haptics
}
contains(MACHINE_NAME, "broadway") {
	DEFINES += MACHINE_BROADWAY
	CONFIG_BUILD += opengl texturesharing directrendering
	CONFIG_BUILD += haptics
	CONFIG_BUILD += qpa
}
contains(MACHINE_NAME, "mantaray") {
	DEFINES += MACHINE_MANTARAY
	CONFIG_BUILD += opengl openglcomposited directrendering
	CONFIG_BUILD += qpa
	CONFIG_BUILD += haptics
}
contains(MACHINE_NAME, "windsornot") {
	DEFINES += MACHINE_WINDSORNOT
	CONFIG_BUILD += opengl openglcomposited directrendering
	CONFIG_BUILD += qpa
}
contains(MACHINE_NAME, "topaz") {
	DEFINES += MACHINE_TOPAZ
        #DEFINES += ENABLE_JS_DEBUG_VERBOSE

###### Uncomment the following for debug building, e.g. for source debugging via eclipse, etc. 
###### Note that this is all really adhoc and quick'n'dirty, and not meant to be a robust method
#	CONFIG -= release
#	CONFIG += debug
#	QMAKE_CXXFLAGS -= $$QMAKE_CXXFLAGS_RELEASE -fomit-frame-pointer -frename-registers -finline-functions
#	QMAKE_CXXFLAGS += -O0 -pg -fno-omit-frame-pointer -fno-rename-registers -fno-inline-functions -fno-exceptions -fno-rtti
#	PALM_CC_OPT =
#	PALM_CXX_OPT =
#	PALM_CXX_EXTRA =
#	QMAKE_LFLAGS -= $$QMAKE_LFLAGS_RELEASE
#	QMAKE_LFLAGS +=  
############

	CONFIG_BUILD += opengl # texturesharing fb1poweroptimization directrendering 
	CONFIG_BUILD += qpa
	CONFIG_BUILD += haptics
}
contains(MACHINE_NAME, "opal") {
	DEFINES += MACHINE_OPAL
	CONFIG_BUILD += opengl texturesharing directrendering
	CONFIG_BUILD += qpa
	CONFIG_BUILD += haptics
}
contains(MACHINE_NAME, "pyramid") {
	DEFINES += MACHINE_PYRAMID
	CONFIG_BUILD += opengl 
	CONFIG_BUILD += qpa
	CONFIG_BUILD += haptics
}
contains(MACHINE_NAME, "qemux86") {
    DEFINES += MACHINE_QEMUX86
    #DEFINES += ENABLE_JS_DEBUG_VERBOSE
    TARGET_TYPE = TARGET_EMULATOR

    # emulator doesn't support these libraries
    LIBS -= -ljemalloc_mt -lpowerd -lmemchute -lhid -lmedia-api
    LIBS += -Wl,-rpath $$(STAGING_LIBDIR)
    SOURCES += SoundPlayerDummy.cpp
    HEADERS += SoundPlayerDummy.h
}

DEFINES += $$TARGET_TYPE HAVE_LUNA_PREF=1 PALM_DEVICE QT_PLUGIN QT_STATICPLUGIN

contains(CONFIG_BUILD, qpa) {
    DEFINES += HAVE_QPA
    LIBS += -lqpalm
}
else {
    HEADERS += 		hiddkbd_qws.h \
			hiddtp_qws.h \
			
    SOURCES +=  	hiddkbd_qws.cpp \
			hiddtp_qws.cpp \
			hiddkbd.cpp \
			hiddtp.cpp
    LIBS += -lnyx
}


HEADERS +=  HostArm.h \
            NyxInputControl.h \
            NyxLedControl.h \

VPATH += Src/input

SOURCES += NyxInputControl.cpp \
           NyxLedControl.cpp \

INCLUDEPATH += \
		$$(STAGING_INCDIR)/glib-2.0 \
		$$(STAGING_INCDIR)/webkit \
		$$(STAGING_INCDIR)/webkit/npapi \
		$$(STAGING_INCDIR)/sysmgr-ipc \
		$$(STAGING_INCDIR)/freetype2 \
		$$(STAGING_INCDIR)/PmLogLib/IncsPublic \
		$$(STAGING_INCDIR)/napp \
		$$(STAGING_INCDIR)/ime \

contains(TARGET_TYPE, TARGET_DEVICE) {
        INCLUDEPATH +=  $$(STAGING_INCDIR)/hid/IncsPublic
        SOURCES += SoundPlayer.cpp
        HEADERS += SoundPlayer.h
}
