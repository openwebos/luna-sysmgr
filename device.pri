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

########################################################
## Start of the standard assignments across all devices
## Don't change this block !!!!

QMAKE_MAKEFILE = Makefile

BUILD_TYPE = release
CONFIG -= debug
CONFIG += release

LIBS += -lluna-prefs -lPmLogLib -lrolegen

HEADERS += HostArm.h

TARGET_TYPE = TARGET_DEVICE

MACHINE_NAME = $$(MACHINE)

INCLUDEPATH += \
        $$(STAGING_INCDIR)/glib-2.0 \
        $$(STAGING_INCDIR)/webkit \
        $$(STAGING_INCDIR)/QtWebKit \
        $$(STAGING_INCDIR)/webkit/npapi \
        $$(STAGING_INCDIR)/sysmgr-ipc \
        $$(STAGING_INCDIR)/freetype2 \
        $$(STAGING_INCDIR)/PmLogLib/IncsPublic \
        $$(STAGING_INCDIR)/ime \


DEFINES += $$TARGET_TYPE HAS_LUNA_PREF=1 QT_PLUGIN QT_STATICPLUGIN HAS_QPA

## End of standard assignments across all devices
########################################################



########################################################
## Check for known build targets

include(device-known.pri)

##
########################################################



########################################################
## Handle custom configuration for unknown build target

contains(CONFIG_BUILD, webosdevice) {
    ## Known Device
    LIBS += -lserviceinstall
} else {
    warning($$MACHINE_NAME not matched in device-known.pri)

    ##  Set this if you have nyx-modules for your build target (Highly recommended)
    CONFIG_BUILD += nyx

    ##  Set this if you have media-api to handle sound
    # CONFIG_BUILD += mediaapi

    ##  You must have a QPA or can use the standard QPA (change the LIBS value to your QPA library)
    LIBS += -lqpalm

    ##  Activate ServiceInstaller, if available for your build target
    # LIBS += -lserviceinstall

    ##  Set this if you have libnapp and nrwindow available for your build target
    # CONFIG_BUILD += napp

}

## End of custom configuration for unknown build target
########################################################
