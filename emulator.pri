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

MACHINE_NAME = $$(MACHINE)

contains(MACHINE_NAME, "qemux86") {
    DEFINES += MACHINE_QEMUX86
    CONFIG_BUILD += webosemulator
}
contains(MACHINE_NAME, "qemuarm") {
    DEFINES += MACHINE_QEMUARM
    CONFIG_BUILD += webosemulator
}
contains(MACHINE_NAME, "qemuarmv7") {
    DEFINES += MACHINE_QEMUARM
    CONFIG_BUILD += webosemulator
}
contains(MACHINE_NAME, "qemuarmv7a") {
    DEFINES += MACHINE_QEMUARM
    CONFIG_BUILD += webosemulator
}

contains (CONFIG_BUILD, webosemulator) {
    TARGET_TYPE = TARGET_EMULATOR

    QMAKE_MAKEFILE = Makefile

    BUILD_TYPE = release
    CONFIG -= debug
    CONFIG += release

    LIBS += -lluna-prefs -lPmLogLib -lrolegen 

    #DEFINES += ENABLE_JS_DEBUG_VERBOSE

    # emulator doesn't support these libraries
    LIBS -= -ljemalloc_mt -lpowerd 
    SOURCES += SoundPlayerDummy.cpp
    HEADERS += SoundPlayerDummy.h

    DEFINES += $$TARGET_TYPE HAVE_LUNA_PREF=1 PALM_DEVICE QT_PLUGIN QT_STATICPLUGIN

    DEFINES += HAVE_QPA

    HEADERS +=  HostArm.h \
                NyxInputControl.h \
                NyxLedControl.h \

    VPATH += Src/input

    SOURCES += NyxInputControl.cpp \
               NyxLedControl.cpp \

    INCLUDEPATH += \
            $$(STAGING_INCDIR)/glib-2.0 \
            $$(STAGING_INCDIR)/webkit \
            $$(STAGING_INCDIR)/QtWebKit \
            $$(STAGING_INCDIR)/webkit/npapi \
            $$(STAGING_INCDIR)/sysmgr-ipc \
            $$(STAGING_INCDIR)/freetype2 \
            $$(STAGING_INCDIR)/PmLogLib/IncsPublic \
            $$(STAGING_INCDIR)/napp \
            $$(STAGING_INCDIR)/ime \
} else {
    warning($$MACHINE_NAME not matched in emulator.pri)
}
