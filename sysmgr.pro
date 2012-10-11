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
# @@@LICENSE
#
#      Copyright (c) 2010 Hewlett-Packard Development Company, L.P.
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
TEMPLATE = app

CONFIG += qt

TARGET_TYPE = 

ENV_BUILD_TYPE = $$(BUILD_TYPE)
!isEmpty(ENV_BUILD_TYPE) {
	CONFIG -= release debug
	CONFIG += $$ENV_BUILD_TYPE
}

# Prevent conflict with usage of "signal" in other libraries
CONFIG += no_keywords

CONFIG += link_pkgconfig
PKGCONFIG = glib-2.0 gthread-2.0

QT = core gui declarative webkit network

VPATH = \
		./Src \
		./Src/base \
		./Src/base/application \
		./Src/base/hosts \
		./Src/base/visual \
		./Src/base/gesture \
		./Src/base/windowdata \
		./Src/base/settings \
		./Src/core \
		./Src/js \
		./Src/sound \
		./Src/webbase \
		./Src/lunaui \
        ./Src/lunaui/cards \
        ./Src/lunaui/notifications \
        ./Src/lunaui/emergency \
        ./Src/lunaui/lockscreen \
        ./Src/lunaui/dock \
		./Src/minimalui \
		./Src/remote \
		./Src/lunaui/status-bar \
        ./Src/ime \
        ./Src/nyx

####### LAUNCHER ADDITIONS ########
VPATH += ./Src/lunaui/launcher
VPATH += ./Src/lunaui/launcher/gfx
VPATH += ./Src/lunaui/launcher/gfx/debug
VPATH += ./Src/lunaui/launcher/gfx/pixmapobject
VPATH += ./Src/lunaui/launcher/gfx/effects
VPATH += ./Src/lunaui/launcher/gfx/processors
VPATH += ./Src/lunaui/launcher/physics
VPATH += ./Src/lunaui/launcher/physics/motion
VPATH += ./Src/lunaui/launcher/elements
VPATH += ./Src/lunaui/launcher/elements/page
VPATH += ./Src/lunaui/launcher/elements/page/icon_layouts
VPATH += ./Src/lunaui/launcher/elements/bars
VPATH += ./Src/lunaui/launcher/elements/icons
VPATH += ./Src/lunaui/launcher/elements/util
VPATH += ./Src/lunaui/launcher/elements/static
VPATH += ./Src/lunaui/launcher/elements/buttons
VPATH += ./Src/lunaui/launcher/systeminterface
VPATH += ./Src/lunaui/launcher/systeminterface/util
VPATH += ./Src/lunaui/launcher/util
VPATH += ./Src/lunaui/launcher/test

####### LAUNCHER ADDITIONS ########

INCLUDEPATH = $$VPATH

DEFINES += QT_WEBOS

# For shipping version of the code, as opposed to a development build. Set this to 1 late in the process...
DEFINES += SHIPPING_VERSION=0

# Uncomment to compile in trace statements in the code for debugging
# DEFINES += ENABLE_TRACING

# DEFINES += HAVE_CALLGRIND=1

# This allows the use of the % for faster QString concatentation
# See the QString documentation for more information
# DEFINES += QT_USE_FAST_CONCATENATION

# Uncomment this for all QString concatenations using +
# to go through the faster % instead.  Not sure what impact
# this has performance wise or behaviour wise.
# See the QString documentation for more information
# DEFINES += QT_USE_FAST_OPERATOR_PLUS

SOURCES = \
	MallocHooks.cpp \
	Mutex.cpp \
	TaskBase.cpp \
	SyncTask.cpp \
	CpuAffinity.cpp \
	HostBase.cpp \
	KeywordMap.cpp \
	Window.cpp \
	ActiveCallBanner.cpp \
	BannerMessageEventFactory.cpp \
	ApplicationDescription.cpp \
	LaunchPoint.cpp \
	ApplicationManager.cpp \
	CmdResourceHandlers.cpp \
	ApplicationManagerService.cpp \
	BackupManager.cpp \
	WebKitEventListener.cpp \
	ApplicationInstaller.cpp \
	WindowManagerBase.cpp \
	WindowServer.cpp \
	FpsHistory.cpp \
	TouchPlot.cpp \
	WindowServerLuna.cpp \
	WindowServerMinimal.cpp \
	WindowManagerMinimal.cpp \
	MetaKeyManager.cpp \
	WebAppCache.cpp \
	WebAppBase.cpp \
	WindowedWebApp.cpp \
	AlertWebApp.cpp \
	CardWebApp.cpp \
	DashboardWebApp.cpp \
	WebAppFactory.cpp \
	WebAppFactoryLuna.cpp \
	WebAppFactoryMinimal.cpp \
	SingletonTimer.cpp \
	Timer.cpp \
	WebAppManager.cpp \
	WebAppDeferredUpdateHandler.cpp \
	Settings.cpp \
	DisplayManager.cpp \
	DisplayStates.cpp \
	AmbientLightSensor.cpp \
	InputManager.cpp \
	EventReporter.cpp \
	ProcessManager.cpp \
	SystemUiController.cpp \
	BannerMessageHandler.cpp \
	Logging.cpp \
	Utils.cpp \
	Main.cpp \
	JsUtil.cpp \
#	JsSysObjectAnimationRunner.cpp \
	SystemService.cpp \
	EventThrottler.cpp \
	EventThrottlerIme.cpp \
	HapticsController.cpp \
	Preferences.cpp \
	NotificationPolicy.cpp \
	PersistentWindowCache.cpp \
#	WindowContentTransitionRunner.cpp \
	RoundedCorners.cpp \
	CoreNaviManager.cpp \
	CoreNaviLeds.cpp \
	MemoryWatcher.cpp \
	Localization.cpp \
	DeviceInfo.cpp \
	Security.cpp \
	EASPolicyManager.cpp \
	AnimationSettings.cpp \
	MimeSystem.cpp \
	IpcServer.cpp \
	IpcClientHost.cpp \
	WebAppMgrProxy.cpp\
	SuspendBlocker.cpp \
	ApplicationStatus.cpp \
	FullEraseConfirmationWindow.cpp \
	SoundPlayerPool.cpp \
	AsyncCaller.cpp \
	HostWindow.cpp \
	HostWindowData.cpp \
	HostWindowDataSoftware.cpp \
	RemoteWindowData.cpp \
	AlertWindow.cpp \
	CardWindow.cpp \
	DashboardWindow.cpp \
	MenuWindow.cpp \
	CardLoading.cpp \
	NativeAlertManager.cpp \
	EmergencyWindowManager.cpp \
	VolumeControlAlertWindow.cpp \
	ReticleItem.cpp \
	TouchToShareGlow.cpp \
	CardWindowManager.cpp \
	OverlayWindowManager.cpp\
	QuicklaunchLayout.cpp \
	MemoryMonitor.cpp \
	MenuWindowManager.cpp \
	DashboardWindowManager.cpp \
	GraphicsItemContainer.cpp \
	CardWindowManagerStates.cpp \
	DashboardWindowManagerStates.cpp \
	DashboardWindowContainer.cpp \
	BannerWindow.cpp \
	TopLevelWindowManager.cpp \
	ClockWindow.cpp \
	LockWindow.cpp \
	DockModeWindowManager.cpp \
	DockModeWindow.cpp \
	DockWebApp.cpp \
	DockModeLaunchPoint.cpp \
	DockModePositionManager.cpp \
	DockModeAppMenuContainer.cpp \
	DockModeClock.cpp \
	DockModeMenuManager.cpp \
	CardDropShadowEffect.cpp \
#	WebKitKeyMap.cpp \
	CardGroup.cpp \
	KeyboardMapping.cpp \
	SingleClickGestureRecognizer.cpp \
#	BootupAnimation.cpp \
	ProgressAnimation.cpp \
	CardHostWindow.cpp \
	KineticScroller.cpp \
	PackageDescription.cpp \
	ServiceDescription.cpp \
	AppDirectRenderingArbitrator.cpp \
	StatusBar.cpp \
	StatusBarClock.cpp \
	StatusBarBattery.cpp \
	StatusBarTitle.cpp \
	StatusBarServicesConnector.cpp \
	StatusBarIcon.cpp \
	StatusBarInfo.cpp \
	StatusBarItemGroup.cpp \
	StatusBarNotificationArea.cpp \
	SystemMenu.cpp \
	BtDeviceClass.cpp \
	IMEManager.cpp \
	TabletKeyboard.cpp \
	PhoneKeyboard.cpp \
	InputWindowManager.cpp \
	IMEView.cpp \ 
	SysmgrIMEDataInterface.cpp \
	IMEController.cpp \
	IMEPixmap.cpp \
	TabletKeymap.cpp \
	PhoneKeymap.cpp \
	KeyLocationRecorder.cpp \
    VirtualKeyboardPreferences.cpp \
    JSONUtils.cpp \
    GhostCard.cpp \
    WSOverlayScreenShotAnimation.cpp \
	VirtualKeyboard.cpp \
	GlyphCache.cpp \
	CandidateBar.cpp \
	CandidateBarRemote.cpp \
	PalmIMEHelpers.cpp \
    InputClient.cpp \
	QmlAlertWindow.cpp \
    ShortcutsHandler.cpp \
    QtHostWindow.cpp \
    UiNavigationController.cpp \
    SysMgrWebBridge.cpp \
    PalmSystem.cpp \
    NyxSensorConnector.cpp
#    WebKitSensorConnector.cpp

HEADERS = \
	AmbientLightSensor.h \
	AnimationSettings.h \
	ApplicationDescription.h \
	ApplicationInstallerErrors.h \
	ApplicationInstaller.h \
	ApplicationManager.h \
	ApplicationStatus.h \
	BackupManager.h \
	CmdResourceHandlers.h \
	CoreNaviLeds.h \
	CoreNaviManager.h \
	Debug.h \
	DeviceInfo.h \
	DisplayManager.h \
	DisplayStates.h \
	EASPolicyManager.h \
	EventReporter.h \
	EventThrottler.h \
	EventThrottlerIme.h \
	HapticsController.h \
	HostBase.h \
	HostWindow.h \
	HostWindowData.h \
	HostWindowDataSoftware.h \
	RemoteWindowData.h \
	InputManager.h \
	LaunchPoint.h \
	Localization.h \
	Logging.h \
	MetaKeyManager.h \
	MimeSystem.h \
	Preferences.h \
	ProcessManager.h \
	RoundedCorners.h \
	Security.h \
	Settings.h \
	SuspendBlocker.h \
	SystemService.h \
	SystemUiController.h \
	Utils.h \
	WebKitEventListener.h \
	Window.h \
	WindowManagerBase.h \
	WindowServer.h \
	TouchPlot.h \
	AnimationEquations.h \
	AsyncCaller.h \
	Event.h \
	GraphicsDefs.h \
	KeywordMap.h \
	Mutex.h \
	MutexLocker.h \
	PtrArray.h \
	SingletonTimer.h \
	sptr.h \
	SyncTask.h \
	TaskBase.h \
	Time.h \
	Timer.h \
#	JsSysObjectAnimationRunner.h \
	JsUtil.h \
	ActiveCallBanner.h \
	AlertWebApp.h \
	BannerMessageEventFactory.h \
	BannerMessageHandler.h \
	CardWebApp.h \
	DashboardWebApp.h \
	FullEraseConfirmationWindow.h \
	NewContentIndicatorEventFactory.h \
	NotificationPolicy.h \
	PersistentWindowCache.h \
	WebAppFactoryLuna.h \
#	WindowContentTransitionRunner.h \
	WindowServerLuna.h \
	WebAppFactoryMinimal.h \
	WindowManagerMinimal.h \
	WindowServerMinimal.h \
	IpcClientHost.h \
	IpcServer.h \
	WebAppMgrProxy.h \
	SoundPlayer.h \
	SoundPlayerPool.h \
	MemoryWatcher.h \
	ProcessBase.h \
	WebAppBase.h \
	WebAppFactory.h \
	WebAppManager.h \
	WebAppCache.h \
	WindowedWebApp.h \
	AlertWindow.h \
	CardWindow.h \
	DashboardWindow.h \
	MenuWindow.h \
	CardLoading.h \
	NativeAlertManager.h \
	EmergencyWindowManager.h \
	VolumeControlAlertWindow.h \
	ReticleItem.h \
	TouchToShareGlow.h \
	CardWindowManager.h \
	OverlayWindowManager.h \
	OverlayWindowManager_p.h \
	QuicklaunchLayout.h \
	MemoryMonitor.h \
	MenuWindowManager.h \
	DashboardWindowManager.h \
	GraphicsItemContainer.h \
	CardWindowManagerStates.h \
	DashboardWindowManagerStates.h \
	DashboardWindowContainer.h \
	BannerWindow.h \
	TopLevelWindowManager.h \
	ClockWindow.h \
	LockWindow.h \
	DockModeWindowManager.h \
	DockModeWindow.h \
	DockWebApp.h \
	DockModeLaunchPoint.h \
	DockModePositionManager.h \
	DockModeAppMenuContainer.h \
	DockModeClock.h \
	DockModeMenuManager.h \
	CardDropShadowEffect.h \
#	WebKitKeyMap.h \
	CardGroup.h \
	KeyboardMapping.h \
	SingleClickGestureRecognizer.h \
	SingleClickGesture.h \
#	BootupAnimation.h \
	ProgressAnimation.h \
	CardHostWindow.h \
	KineticScroller.h \
	PackageDescription.h \
	ServiceDescription.h \
	AppDirectRenderingArbitrator.h \
	StatusBar.h \
	StatusBarClock.h \
	StatusBarBattery.h \
	StatusBarTitle.h \
	StatusBarServicesConnector.h \
	StatusBarIcon.h \
	StatusBarInfo.h \
	StatusBarItem.h \
	StatusBarItemGroup.h \
	StatusBarNotificationArea.h \
	SystemMenu.h \
	BtDeviceClass.h \
	IMEManager.h \
	InputMethod.h \
	TabletKeyboard.h \
	PhoneKeyboard.h \
	InputWindowManager.h \
	IMEView.h \
	IMEData.h_generator.h \
	IMEData.h \
	SysmgrIMEDataInterface.h \
	IMEDataInterface.h \
	IMEController.h \
	IMEPixmap.h \
	TabletKeymap.h \
	PhoneKeymap.h \
	KeyLocationRecorder.h \
    VirtualKeyboardPreferences.h \
    JSONUtils.h \
    GhostCard.h \
    WSOverlayScreenShotAnimation.h \
	VirtualKeyboard.h \
	GlyphCache.h \
	CandidateBar.h \
	CandidateBarRemote.h \
	PalmIMEHelpers.h \
    InputClient.h \
    QmlInputItem.h \
    CardSmoothEdgeShaderStage.h \
    CardRoundedCornerShaderStage.h \
	QmlAlertWindow.h \
    ShortcutsHandler.h \
    QtHostWindow.h \
    UiNavigationController.h \
    SysMgrWebBridge.h \
    PalmSystem.h \
    NyxSensorCommonTypes.h \
    NyxSensorConnector.h
#    WebKitSensorConnector.h

####### LAUNCHER ADDITIONS ########

SOURCES += dimensionsmain.cpp \
			dimensionslauncher.cpp \
			quicklaunchbar.cpp \
			page.cpp \
			pagemovement.cpp \
			thing.cpp \
			thingpaintable.cpp \
			layoutitem.cpp \
			dimensionsglobal.cpp \
			debugglobal.cpp \
			groupanchoritem.cpp \
			layoutsettings.cpp \
			operationalsettings.cpp \
			dynamicssettings.cpp \
			pixmapobject.cpp \
			pixmap9tileobject.cpp \
			pixmap3htileobject.cpp \
			pixmap3vtileobject.cpp \
			pixmaphugeobject.cpp \
			pixmapjupocobject.cpp \
			pixmapjupocrefobject.cpp \
			pixmapfilmstripobject.cpp \
			pixpager.cpp \
			gfxsettings.cpp \
			pixpagerdebugger.cpp \
			sysmgrdebuggerservice.cpp \
			qtjsonabstract.cpp \
			renderedlabel.cpp \
			scrollableobject.cpp \
			scrollingsurface.cpp \
			scrollinglayoutrenderer.cpp \
			variableanimsignaltransition.cpp \
			linearmotiontransform.cpp \
			frictiontransform.cpp \
			pagetabbar.cpp \
			pagetab.cpp \
			icon.cpp \
			iconcmdevents.cpp \
			icondecorator.cpp \
			iconlayout.cpp \
			alphabeticonlayout.cpp \
			alphabetpage.cpp \
			reorderableiconlayout.cpp \
			reorderablepage.cpp \
			iconreorderanimation.cpp \
			iconlayoutsettings.cpp \
			icongeometrysettings.cpp \
			staticelementsettings.cpp \
			pixmaploader.cpp \
			gfxeffectbase.cpp \
			gfxsepiaeffect.cpp \
			pixbutton.cpp \
			labeledbutton.cpp \
			colorroundrectbutton.cpp \
			pixbuttonsimple.cpp \
			pixbutton2state.cpp \
			horizontaldivider.cpp \
			horizontallabeleddivider.cpp \
			testiconfactory.cpp \
			dotgrid.cpp \
			externalapp.cpp \
			webosapp.cpp \
			appmonitor.cpp \
			iconheap.cpp \
			stringtranslator.cpp \
			appeffector.cpp \
			pagesaver.cpp \
			pagerestore.cpp \
			filenames.cpp \
			blacklist.cpp \
			staticmatchlist.cpp \
			textbox.cpp \
			picturebox.cpp \
			conditionalsignaltransition.cpp \
			propertysettingsignaltransition.cpp \
			timedelaytransition.cpp \
			vcamera.cpp \
			expblur.cpp \
			overlaylayer.cpp \
			safefileops.cpp
			
HEADERS += dimensionsmain.h \
			dimensionslauncher.h \
			quicklaunchbar.h \
			dimensionstypes.h \
			renderopts.h \
			page.h \
			pagemovement.h \
			thing.h \
			thingpaintable.h \
			layoutitem.h \
			dimensionsglobal.h \
			debugglobal.h \
			groupanchoritem.h \
			layoutsettings.h \
			operationalsettings.h \
			dynamicssettings.h \
			pixmapobject.h \
			pixmap9tileobject.h \
			pixmap3htileobject.h \
			pixmap3vtileobject.h \
			pixmaphugeobject.h \
			pixmapjupocobject.h \
			pixmapjupocrefobject.h \
			pixmapfilmstripobject.h \
			pixpager.h \
			gfxsettings.h \
			pixpagerdebugger.h \
			sysmgrdebuggerservice.h \
			qtjsonabstract.h \
			renderedlabel.h \
			scrollableobject.h \
			scrollingsurface.h \
			scrollinglayoutrenderer.h \
			variableanimsignaltransition.h \
			linearmotiontransform.h \
			frictiontransform.h \
			pagetabbar.h \
			pagetab.h \
			icon.h \
			iconcmdevents.h \
			icondecorator.h \
			iconlayout.h \
			alphabeticonlayout.h \
			alphabetpage.h \
			reorderableiconlayout.h \
			reorderablepage.h \
			iconreorderanimation.h \
			iconlayoutsettings.h \
			icongeometrysettings.h \
			staticelementsettings.h \
			pixmaploader.h \
			gfxeffectbase.h \
			gfxsepiaeffect.h \
			pixbutton.h \
			labeledbutton.h \
			colorroundrectbutton.h \
			pixbuttonsimple.h \
			pixbutton2state.h \
			horizontaldivider.h \
			horizontallabeleddivider.h \
			testiconfactory.h \
			dotgrid.h \
			externalapp.h \
			webosapp.h \
			appmonitor.h \
			iconheap.h \
			stringtranslator.h \
			appeffector.h \
			pagesaver.h \
			pagerestore.h \
			filenames.h \
			blacklist.h \
			filterlist.h \
			staticmatchlist.h \
			textbox.h \
			picturebox.h \
			conditionalsignaltransition.h \
			propertysettingsignaltransition.h \
			timedelaytransition.h \
			vcamera.h \
			expblur.h \
			overlaylayer.h \
			safefileops.h
			
####### LAUNCHER ADDITIONS ########


QMAKE_CXXFLAGS += -fno-rtti -fno-exceptions -fvisibility=hidden -fvisibility-inlines-hidden -Wall -fpermissive
QMAKE_CXXFLAGS += -DFIX_FOR_QT
#-DNO_WEBKIT_INIT

# Override the default (-Wall -W) from g++.conf mkspec (see linux-g++.conf)
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-unused-variable -Wno-reorder -Wno-missing-field-initializers -Wno-extra

LIBS += -lcjson -lLunaSysMgrIpc -llunaservice -lpbnjson_cpp -lssl -lsqlite3 -lssl -lcrypto -lnyx

linux-g++ {
    include(desktop.pri)
} else:linux-g++-64 {
    include(desktop.pri)
} else:linux-qemux86-g++ {
	include(emulator.pri)
	QMAKE_CXXFLAGS += -fno-strict-aliasing
} else {
    ## First, check to see if this in an emulator build
    include(emulator.pri)
    contains (CONFIG_BUILD, webosemulator) {
        QMAKE_CXXFLAGS += -fno-strict-aliasing
    } else {
        ## Neither a desktop nor an emulator build, so must be a device
        include(device.pri)
    }
}


contains(CONFIG_BUILD, opengl) {
	QT += opengl
	DEFINES += HAVE_OPENGL
	DEFINES += P_BACKEND=P_BACKEND_SOFT

	contains(CONFIG_BUILD, texturesharing) {
		DEFINES += HAVE_TEXTURESHARING OPENGLCOMPOSITED
		SOURCES += HostWindowDataOpenGLTextureShared.cpp 
					RemoteWindowDataSoftwareTextureShared.cpp \
					RemoteWindowDataSoftwareQt.cpp \
					RemoteWindowDataSoftwareOpenGLComposited.cpp 
		HEADERS += HostWindowDataOpenGLTextureShared.h 
					RemoteWindowDataSoftwareTextureShared.h \
					RemoteWindowDataSoftwareQt.h \
					RemoteWindowDataSoftwareOpenGLComposited.h \
					NAppWindow.h
                #LIBS += -lnapp -lnrwindow
	} else {
		contains(CONFIG_BUILD, openglcomposited) {
			DEFINES += OPENGLCOMPOSITED
			SOURCES += RemoteWindowDataSoftwareOpenGLComposited.cpp
			HEADERS += RemoteWindowDataSoftwareOpenGLComposited.h \
						NAppWindow.h
		}
	
   		SOURCES += HostWindowDataOpenGL.cpp \
	    			RemoteWindowDataSoftwareQt.cpp \
                                RemoteWindowDataOpenGLQt.cpp

   		HEADERS += HostWindowDataOpenGL.h \
	    			RemoteWindowDataSoftwareQt.h \
                                RemoteWindowDataOpenGLQt.h

#    				RemoteWindowDataOpenGL.h \
#    				RemoteWindowDataOpenGL.cpp \
	}
}
else {
	DEFINES += P_BACKEND=P_BACKEND_SOFT
	SOURCES += RemoteWindowDataSoftwareQt.cpp
	HEADERS += RemoteWindowDataSoftwareQt.h
}

contains(CONFIG_BUILD, fb1poweroptimization) {
	DEFINES += FB1_POWER_OPTIMIZATION=1
}

contains(CONFIG_BUILD, directrendering) {
	DEFINES += DIRECT_RENDERING=1
}

contains(CONFIG_BUILD, haptics) {
	DEFINES += HAPTICS=1
}

contains(CONFIG_BUILD, mediaapi) {
    SOURCES += SoundPlayer.cpp
    HEADERS += SoundPlayer.h
    LIBS += -lmedia-api
    DEFINES += HAS_MEDIA_API
} else {
    SOURCES += SoundPlayerDummy.cpp
    HEADERS += SoundPlayerDummy.h
}

contains(CONFIG_BUILD, nyx) {
    HEADERS +=  NyxInputControl.h \
                NyxLedControl.h \

    VPATH += Src/input

    SOURCES += NyxInputControl.cpp \
           NyxLedControl.cpp \

    DEFINES += HAS_NYX
}

contains(CONFIG_BUILD, memchute) {
    LIBS += -lmemchute
    DEFINES += HAS_MEMCHUTE
}

contains(CONFIG_BUILD, hidlib) {
    INCLUDEPATH += $$(STAGING_INCDIR)/hid/IncsPublic
    LIBS += -lhid
    DEFINES += HAS_HIDLIB
}

contains(CONFIG_BUILD, affinity) {
    LIBS += -laffinity
    DEFINES += HAS_AFFINITY
}

contains(CONFIG_BUILD, napp) {
    INCLUDEPATH += $$(STAGING_INCDIR)/napp
    DEFINES += HAS_NAPP
}

DESTDIR = ./$${BUILD_TYPE}-$${MACHINE_NAME}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc

TARGET = LunaSysMgr

# Comment these out to get verbose output
#QMAKE_CXX = @echo Compiling $(@)...; $$QMAKE_CXX
#QMAKE_LINK = @echo Linking $(@)...; $$QMAKE_LINK
#QMAKE_MOC = @echo Mocing $(@)...; $$QMAKE_MOC
