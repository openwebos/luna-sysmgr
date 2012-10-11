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

contains(MACHINE_NAME, "broadway") {
	DEFINES += MACHINE_BROADWAY USE_KEYMAPS PALM_DEVICE
	CONFIG_BUILD += opengl texturesharing directrendering
	CONFIG_BUILD += haptics napp nyx hidlib webosdevice
    LIBS += -lqpalm
}
contains(MACHINE_NAME, "mantaray") {
	DEFINES += MACHINE_MANTARAY USE_KEYMAPS PALM_DEVICE
	CONFIG_BUILD += opengl openglcomposited directrendering
	CONFIG_BUILD += haptics napp nyx hidlib webosdevice
    LIBS += -lqpalm
}
contains(MACHINE_NAME, "windsornot") {
	DEFINES += MACHINE_WINDSORNOT PALM_DEVICE
	CONFIG_BUILD += opengl openglcomposited directrendering 
	CONFIG_BUILD +=  nyx napp hidlib webosdevice
    LIBS += -lqpalm
}
contains(MACHINE_NAME, "topaz") {
	DEFINES += MACHINE_TOPAZ PALM_DEVICE
    # DEFINES += ENABLE_JS_DEBUG_VERBOSE

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

	CONFIG_BUILD += opengl directrendering # texturesharing fb1poweroptimization
	CONFIG_BUILD += affinity haptics napp nyx hidlib webosdevice
    LIBS += -lqpalm
}
contains(MACHINE_NAME, "opal") {
	DEFINES += MACHINE_OPAL PALM_DEVICE
	CONFIG_BUILD += opengl texturesharing directrendering
	CONFIG_BUILD += affinity haptics napp nyx hidlib webosdevice
    LIBS += -lqpalm
}
