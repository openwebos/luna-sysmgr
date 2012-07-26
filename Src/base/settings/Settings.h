/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */




#ifndef __Settings_h__
#define __Settings_h__

#include "Common.h"

#include <string>
#include <vector>
#include <set>
#include <glib.h>
#include <QtGlobal>

class Settings
{
public:

	enum UiType {
		UI_LUNA = 0,
		UI_MINIMAL
	};

	std::string			lunaAppsPath; 		// default >> /var/luna/applications/
	std::vector<std::string>	lunaAppsPaths;	// additional paths for applications...lunaAppsPath will be vector[0]
	std::string			pendingAppsPath; // default >> /var/palm/data/com.palm.appInstallService
	std::string			appInstallBase;		//default >> /media/cryptofs/apps
	std::string			appInstallRelative;		// default >> usr/palm/applications
	std::string			packageInstallBase;		// default >> /media/cryptofs/apps				// MUST BE EQUAL TO appInstallBase UNTIL FURTHER NOTICE!
	std::string			packageInstallRelative;		//default >> usr/palm/packages
	std::string			serviceInstallBase;		// default >> /media/cryptofs/apps
	std::string			serviceInstallRelative;		//default >> usr/palm/services
	std::string			packageManifestsPath;			//default >> /media/cryptofs/apps/usr/lib/ipkg/info
	std::string			downloadPathMedia;	//default >> /media/internal/downloads
	std::string			appInstallerTmp;	//default >> /media/internal/.tmp
	std::string			lunaPresetLaunchPointsPath;		//default >> /usr/luna/launchpoints			//for launchpoints that will never change (carrier cust stuff etc)
	std::string         lunaLaunchPointsPath; // default >> /var/luna/launchpoints/
	std::string			lunaSystemPath;		// default >> /usr/lib/luna/system/luna-systemui/
	std::string			lunaAppLauncherPath;	// default >> /usr/lib/luna/system/luna-applauncher/
	std::string			lunaSystemResourcesPath;	// default >> /usr/lib/luna/system/luna-systemui/images/
	std::string			lunaSystemLocalePath;	// default >> /usr/palm/sysmgr/localization
	std::string         lunaCustomizationLocalePath; // default >> /usr/palm/sysmgr-cust/localization
	std::string         lunaPrefsPath;      // default >> /var/luna/preferences
	std::string			lunaCmdHandlerPath;	// default >> /usr/palm/command-resource-handlers.json
	std::string			lunaCmdHandlerSavedPath;	//default >> /var/usr/palm/command-resource-handlers-active.json
	std::string			lunaQmlUiComponentsPath;	//default >> /usr/palm/sysmgr/uiComponents
	std::string                     lunaScreenCapturesPath;         // default >> /media/internal/screencaptures

	int					cardLimit; // -1 to disable
	std::set<std::string>	appsToAllowInLowMemory;

	bool				showReticle;

	std::string			lunaSystemSoundsPath;     // default >> /usr/palm/sounds
	std::string         lunaDefaultAlertSound;    // default >> /usr/palm/sounds/alert.wav
	std::string         lunaDefaultRingtoneSound; // default >> /usr/palm/sounds/phone.wav

	std::string         lunaSystemSoundAppOpen;
	std::string         lunaSystemSoundAppClose;
	std::string         lunaSystemSoundTap;
	std::string         lunaSystemSoundButtonDown;
	std::string         lunaSystemSoundButtonUp;
	std::string         lunaSystemSoundScreenLock;
	std::string         lunaSystemSoundScreenUnlock;
	std::string			lunaSystemSoundScreenCapture;

	int					notificationSoundDuration;

	int                 lightbarEnabled;
	int                 coreNaviScaler;
	int		    gestureAnimationSpeed;

	int                 backlightOutdoorScale;
	int                 backlightDimScale;
	int                 backlightDarkScale;
	
	int					displayWidth;
	int					displayHeight;
	int                 displayNumBuffers;

	// parameters to control led pulsing
	int             ledPulseMaxBrightness;
	int             ledPulseDarkBrightness;
	
	// ALS
	bool            enableAls;
	bool		disableLocking;
	int		lockScreenTimeout;

    // Parameters to control pen event throttling
    int maxPenMoveFreq;
    int maxPaintLoad;

    // Parameters to control gesture event throttling
    int maxGestureChangeFreq;

	// Parameters to control touch event throttling
	int maxTouchChangeFreq;


	bool				debug_trackInputEvents;
	bool				debug_enabled;
	bool                debug_piranhaDrawColoredOutlines;
	bool                debug_piranhaDisplayFps;
	bool				debug_showGestures;
	bool	            debug_doVerboseCrashLogging;
	bool	            debug_loopInCrashHandler;

	int					tapRadius;
	int					tapRadiusMin;
	int					tapRadiusSquared;
	int					tapRadiusShrinkPercent;
	int					tapRadiusShrinkGranMs;
	int                 tapDoubleClickDuration; // msec.
    bool                enableTouchEventsForWebApps;

    int                 homeDoubleClickDuration; // time to wait for a second Home button press

	int					dragRadiusSquared;

	int					h_trackball_pixels_per_move;
	int					v_trackball_pixels_per_move;
    int h_accel_rate1;
    int v_accel_rate1;
    int h_accel_const1;
    int v_accel_const1;
    int h_accel_rate2;
    int v_accel_rate2;
    int h_accel_const2;
    int v_accel_const2;

    int accelFastPollFreq;
	bool turnOffAccelWhenDimmed;

    bool                logger_useSyslog;
	bool				logger_useTerminal;
	bool				logger_useColor;
    int                 logger_level;

	std::string         defaultLanguage;

	std::string         launcherDefaultPositions;
	std::string         launcherCustomPositions;
	std::string			quicklaunchDefaultPositions;
	std::string			quicklaunchCustomPositions;
	std::string 		quicklaunchUserPositions;
	std::string         launcherScrim;
	std::string         firstCardLaunch;
	bool                atlasEnabled;
	double					cardGroupingXDistanceFactor;
	int                 atlasMemThreshold;
	bool                launcherAtlasStatistics;
	bool                launcherDumpAtlas;

	double				launcherSideSwipeThreshold;
	bool				launcherUsesHwAA;
	int					launcherRowSpacingAdjust;
	int					launcherLabelWidthAdjust;
	int					launcherLabelXPadding;
	double				launcherIconReorderPositionThreshold;
	int                 	statusBarTitleMaxWidth;

	bool 			dockModePrelaunchAllApps;
	bool 			dockModeCloseOnMinimize;
	bool 			dockModeCloseOnExit;
	unsigned int        	dockModeMaxApps;
	unsigned char		dockModeNightBrightness;
	std::string 		dockModeDefaultPositions;
	std::string		dockModeCustomPositions;
	std::string 		dockModeUserPositions;
	unsigned int		dockModeMenuHeight;
	
	bool				virtualKeyboardEnabled;

	bool				virtualCoreNaviEnabled;
	unsigned int        virtualCoreNaviHeight;

	std::set<std::string> appsToLaunchAtBoot;
	std::set<std::string> appsToKeepAlive;
	std::set<std::string> appsToKeepAliveUntilMemPressure;
	std::set<std::string> appsToDisableAccelCompositing;
	int					maxNumParkedApps;

	std::set<std::string> sucApps;

	UiType				uiType;

	std::string			fontBanner;
	std::string 		fontActiveBanner;
	std::string			fontLockWindow;
	std::string			fontDockMode;
	std::string			fontQuicklaunch;
	std::string			fontBootupAnimation;
	std::string			fontProgressAnimationBold;
	std::string			fontProgressAnimation;
	std::string			fontKeyboardKeys;
	std::string			fontStatusBar;

	
	bool displayUiRotates;
	bool tabletUi;
	int  homeButtonOrientationAngle;

	int positiveSpaceTopPadding;
	int positiveSpaceBottomPadding;
	float maximumNegativeSpaceHeightRatio;
	qreal activeCardWindowRatio;
	qreal nonActiveCardWindowRatio;
    qreal ghostCardFinalRatio;
	int cardGroupRotFactor;
	int gapBetweenCardGroups;
	int overlayNotificationsHeight;
	int splashIconSize;
    bool enableSplashBackgrounds;

	unsigned int maxDownloadManagerQueueLength;
	int maxDownloadManagerConcurrent;
	unsigned int maxDownloadManagerRecvSpeed;

	bool demoMode;

	bool showAppStats;

	bool collectUseStats;

	bool	usePartialKeywordAppSearch;
	//...

	bool	scanCalculatesAppSizes;

	int uiMainCpuShareLow;
	int uiOtherCpuShareLow;
	int javaCpuShareLow;
	int webCpuShareLow;
	int gameCpuShareLow;
	int cpuShareDefault;
	int cpuShareDefaultLow;
	int modalWindowWidth;
	int modalWindowHeight;

	bool allowTurboMode;

	std::string			wifiInterfaceName; 		// default >> eth0
	std::string			wanInterfaceName; 		// default >> ppp0

	bool canRestartHeadlessApps;

	bool forceSoftwareRendering;
	bool perfTesting;

	int debug_appInstallerCleaner;
	std::string                     logFileName;            // default >> /var/log/lunasysmgr.log

    float cardDimmPercentage;
    int schemaValidationOption;

	static inline Settings*  LunaSettings() {
		if (G_LIKELY(s_settings))
			return s_settings;

		s_settings = new Settings();
		return s_settings;
	}


	void createNeededFolders();

private:
	void load(const char* settingsFile);
	void postLoad();
	Settings();
	~Settings();

	static bool validateDownloadPath(const std::string& path);

	static Settings* s_settings;
};

#endif // Settings

