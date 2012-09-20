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
CONFIG += qt no_keywords
QT += testlib
CONFIG += link_pkgconfig
PKGCONFIG = glib-2.0 gthread-2.0

VPATH = ../../Src \
		../../Src \
		../../Src/base \
		../../Src/core \
		../../Src/js \
		../../Src/sound \
		../../Src/webbase \
		../../Src/lunaui \
		../../Src/minimalui \
		../../Src/remote \
		../../Src/widgets \
		../../Src/lunaui/virtual-keyboard \
		../../Src/lunaui/launcher \
		../../Src/input

INCLUDEPATH = $$VPATH

DEFINES += QT_WEBOS

QMAKE_CXXFLAGS += -fno-rtti -fno-exceptions -Wall -Werror
QMAKE_CXXFLAGS += -DFIX_FOR_QT
# Override the default (-Wall -W) from g++.conf mkspec (see linux-g++.conf)
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter -Wno-unused-variable -Wno-reorder -Wno-missing-field-initializers -Wno-extra


LIBS += -lcjson -lLunaSysMgrIpc -lLunaKeymaps -lWebKitLuna -llunaservice -lpbnjson_cpp

linux-g++ {
	include(../../desktop.pri)
}

linux-qemux86-g++ {
	include(../../device.pri)
	QMAKE_CXXFLAGS += -fno-strict-aliasing
}

linux-qemuarm-g++ {
    include(../../device.pri)
    QMAKE_CXXFLAGS += -fno-strict-aliasing
}

linux-armv7-g++ {
	include(../../device.pri)
}

linux-armv6-g++ {
	include(../../device.pri)
}

DESTDIR = ./$${BUILD_TYPE}-$${MACHINE_NAME}
OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc

TARGET = sysmgrtst_InputTouch

SOURCES += \
	MallocHooks.cpp \
	Mutex.cpp \
	Thread.cpp \
	TaskBase.cpp \
	AsyncTask.cpp \
	SyncTask.cpp \
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
	DockPositionManager.cpp \
	ApplicationInstaller.cpp \
	WindowManagerBase.cpp \
	WindowServer.cpp \
	FpsHistory.cpp \
	WindowServerLuna.cpp \
	WindowServerMinimal.cpp \
	WindowManagerMinimal.cpp \
	MetaKeyManager.cpp \
	WebPage.cpp \
	WebPageCache.cpp \
	WebAppBase.cpp \
	WindowedWebApp.cpp \
	AlertWebApp.cpp \
	CardWebApp.cpp \
	DashboardWebApp.cpp \
	WebAppFactory.cpp \
	WebAppFactoryLuna.cpp \
	WebAppFactoryMinimal.cpp \
	Rectangle.cpp \
	SingletonTimer.cpp \
	Timer.cpp \
	WebAppManager.cpp \
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
	ScaleImageBresenham.cpp \
	Utils.cpp \
	EncryptionUtil.cpp \
	JsSysObjectWrapper.cpp \
	JsSysObject.cpp \
	JsUtil.cpp \
	JsSysObjectAnimationRunner.cpp \
	SystemService.cpp \
	EventThrottler.cpp \
	EventThrottlerIme.cpp \
	HapticsController.cpp \
	Preferences.cpp \
	NotificationPolicy.cpp \
	PersistentWindowCache.cpp \
	WindowContentTransitionRunner.cpp \
	RoundedCorners.cpp \
	CoreNaviManager.cpp \
	CoreNaviLeds.cpp \
	MemoryWatcher.cpp \
	Localization.cpp \
	SSLSupport.cpp \
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
	Variant.cpp \
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
	CardTransition.cpp \
	NativeAlertManager.cpp \
	EmergencyWindowManager.cpp \
	VolumeControlAlertWindow.cpp \
	ReticleItem.cpp \
	CardWindowManager.cpp \
	OverlayWindowManager.cpp\
	DockWindow.cpp \
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
	DockModeLoadingAnimation.cpp \
	DockWebApp.cpp \
	DockModeLaunchPoint.cpp \
	DockModePositionManager.cpp \
	DockModeMenu.cpp \
	DockModeStatusBar.cpp \
	DockModeClock.cpp \
	CardDropShadowEffect.cpp \
	WebKitKeyMap.cpp \
	PixmapButton.cpp \
	CardGroup.cpp \
	KeyboardMapping.cpp \
    	GestureEventTransition.cpp \
	SingleClickGestureRecognizer.cpp \
	BootupAnimation.cpp \
	ProgressAnimation.cpp \
	CardHostWindow.cpp \
	VirtualKeyboard.cpp \
	KineticScroller.cpp \
	VirtualKeyboardManager.cpp \
	KeyboardFeedbackItem.cpp \
	OverlayNotificationWindowManager.cpp \
	PackageDescription.cpp \
	ServiceDescription.cpp \
	AppDirectRenderingArbitrator.cpp

HEADERS += \
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
	AnimationEquations.h \
	AsyncCaller.h \
	AsyncTask.h \
	EncryptionUtil.h \
	Event.h \
	GraphicsDefs.h \
	KeywordMap.h \
	Mutex.h \
	MutexLocker.h \
	PGSharedWrapper.h \
	PtrArray.h \
	Rectangle.h \
	ScaleImageBresenham.h \
	SingletonTimer.h \
	sptr.h \
	SSLSupport.h \
	SyncTask.h \
	TaskBase.h \
	Thread.h \
	Time.h \
	Timer.h \
	Variant.h \
	JsSysObjectAnimationRunner.h \
	JsSysObject.h \
	JsSysObjectWrapper.h \
	JsUtil.h \
	npapi.h \
	npruntime.h \
	nptypes.h \
	npupp.h \
	ActiveCallBanner.h \
	AlertWebApp.h \
	BannerMessageEventFactory.h \
	BannerMessageHandler.h \
	CardWebApp.h \
	DashboardWebApp.h \
	DockPositionManager.h \
	FullEraseConfirmationWindow.h \
	NewContentIndicatorEventFactory.h \
	NotificationPolicy.h \
	PersistentWindowCache.h \
	WebAppFactoryLuna.h \
	WindowContentTransitionRunner.h \
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
	WebPageCache.h \
	WebPageClient.h \
	WebPage.h \
	WindowedWebApp.h \
	AlertWindow.h \
	CardWindow.h \
	DashboardWindow.h \
	MenuWindow.h \
	CardLoading.h \
	CardTransition.h \
	NativeAlertManager.h \
	EmergencyWindowManager.h \
	VolumeControlAlertWindow.h \
	ReticleItem.h \
	CardWindowManager.h \
	OverlayWindowManager.h \
	OverlayWindowManager_p.h \
	DockWindow.h \
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
	DockModeLoadingAnimation.h \
	DockWebApp.h \
	DockModeLaunchPoint.h \
	DockModePositionManager.h \
	DockModeMenu.h \
	DockModeStatusBar.h \
	DockModeClock.h \
	CardDropShadowEffect.h \
	WebKitKeyMap.h \
	PixmapButton.h \
	CardGroup.h \
	KeyboardMapping.h \
   	GestureEventTransition.h \
	SingleClickGestureRecognizer.h \
	SingleClickGesture.h \
	BootupAnimation.h \
	ProgressAnimation.h \
	CardHostWindow.h \
	VirtualInputMethod.h \
	KineticScroller.h \
	VirtualKeyboard.h \
	VirtualKeyboardManager.h \
	KeyboardFeedbackItem.h \
	LabelProperties.h \
	OverlayNotificationWindowManager.h \
	PackageDescription.h \
	ServiceDescription.h \
	AppDirectRenderingArbitrator.h

####### LAUNCHER ADDITIONS ########
SOURCES += launchercard.cpp \
	launcher.cpp \
	gesturablegraphicsobject.cpp \
	LauncherCardModel.cpp \
	LauncherItem.cpp \
	LauncherItemModel.cpp \
	CardLayout.cpp \
	LauncherModel.cpp \
	LauncherLayout.cpp \
	LauncherSettings.cpp \
	KineticScrollerModel.cpp \
	QHelper.cpp \
	DeleteCardGraphicsItem.cpp

HEADERS += launchercard.h \
	launcher.h \
	gesturablegraphicsobject.h \
	LauncherCardModel.h \
	LauncherItem.h \
	LauncherItemModel.h \
	LauncherItemObject.h \
	CardLayout.h \
	LauncherModel.h \
	LauncherLayout.h \
	LauncherSettings.h \
	KineticScrollerModel.h \
	QHelper.h \
	DeleteCardGraphicsItem.h

SOURCES += sysmgrtst_InputTouch.cpp

contains(CONFIG_BUILD, opengl) {
	QT += opengl
	DEFINES += HAVE_OPENGL
	DEFINES += P_BACKEND=P_BACKEND_SOFT
	LIBS += -lPiranha

	contains(CONFIG_BUILD, texturesharing) {
		DEFINES += HAVE_TEXTURESHARING
		SOURCES += HostWindowDataOpenGLTextureShared.cpp \
					RemoteWindowDataSoftwareTextureShared.cpp \
					RemoteWindowDataSoftware.cpp
		HEADERS += HostWindowDataOpenGLTextureShared.h \
					RemoteWindowDataSoftwareTextureShared.h \
					RemoteWindowDataSoftware.h
		LIBS += -lnapp -lnrwindow
	}
	else {
		SOURCES += HostWindowDataOpenGL.cpp \
					RemoteWindowDataOpenGL.cpp \
					RemoteWindowDataSoftware.cpp
		HEADERS += HostWindowDataOpenGL.h \
					RemoteWindowDataOpenGL.h \
					RemoteWindowDataSoftware.h
	}
}
else {
	DEFINES += P_BACKEND=P_BACKEND_SOFT
	LIBS += -lPiranha
	SOURCES += RemoteWindowDataSoftware.cpp
	HEADERS += RemoteWindowDataSoftware.h
}

contains(CONFIG_BUILD, fb1poweroptimization) {
	DEFINES += FB1_POWER_OPTIMIZATION=1
}

contains(CONFIG_BUILD, directrendering) {
	DEFINES += DIRECT_RENDERING=1
}

