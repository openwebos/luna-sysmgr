# @@@LICENSE
#
#   Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
#   Copyright (c) 2013 LG Electronics
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
STAGING_INCLUDE_DIR = $$(STAGING_INCDIR)

contains(MACHINE_NAME, "qemux86-64") {
    CONFIG_BUILD += nyx webosemulator
}
contains(MACHINE_NAME, "qemux86") {
    CONFIG_BUILD += nyx webosemulator
}
contains(MACHINE_NAME, "qemuarm") {
    CONFIG_BUILD += nyx webosemulator
}
contains(MACHINE_NAME, "qemuarmv7") {
    CONFIG_BUILD += nyx webosemulator
}
contains(MACHINE_NAME, "qemuarmv7a") {
    CONFIG_BUILD += nyx webosemulator
}

contains (CONFIG_BUILD, webosemulator) {
    TARGET_TYPE = TARGET_EMULATOR

    QMAKE_MAKEFILE = Makefile

    BUILD_TYPE = release
    CONFIG -= debug
    CONFIG += release

    LIBS += -lluna-prefs -lPmLogLib -lrolegen

    HEADERS +=  HostArm.h

    #DEFINES += ENABLE_JS_DEBUG_VERBOSE

    LIBS += -Wl,-rpath $$(STAGING_LIBDIR)

    DEFINES += $$TARGET_TYPE HAS_LUNA_PREF=1 QT_PLUGIN QT_STATICPLUGIN

    DEFINES += HAS_QPA

    INCLUDEPATH += \
            $$(STAGING_INCDIR)/glib-2.0 \
            $$(STAGING_INCDIR)/sysmgr-ipc \
            $$(STAGING_INCDIR)/freetype2 \
            $$(STAGING_INCDIR)/PmLogLib/IncsPublic \
            $$(STAGING_INCDIR)/luna-sysmgr-common \
            $$(STAGING_INCDIR)/ime \

    contains(QT_VERSION, "^5.*") {
        DEFINES += USE_KEY_FILTER USE_MOUSE_FILTER
    }

} else {
    warning($$MACHINE_NAME not matched in emulator.pri)
}
