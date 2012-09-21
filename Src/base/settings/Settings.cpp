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




#include "Common.h"

#include "Settings.h"

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <math.h>

#include "Utils.h"
#include "Logging.h"

#if !defined(TARGET_DESKTOP)
 #include <lunaprefs.h>
#endif

static const char* kSettingsFile = "/etc/palm/luna.conf";
static const char* kSettingsFilePlatform = "/etc/palm/luna-platform.conf";

#if 0

#define SETTINGS_TRACE(...) \
do { \
    fprintf(stdout, "Settings:: " ); \
    fprintf(stdout, __VA_ARGS__); \
} while (0)

#else

#define SETTINGS_TRACE(...) (void)0

#endif


unsigned long MemStringToBytes( const char* ptr );

Settings* Settings::s_settings = 0;

//static
bool	Settings::validateDownloadPath(const std::string& path) {

	//do not allow /../ in the path. This will avoid complicated parsing to check for valid paths
	if (path.find("..") != std::string::npos)
		return false;

	//the prefix /var or /media has to be anchored at 0
	if ((path.find("/var") == 0 ) || (path.find("/media") == 0))
		return true;

	return false;
}

//TODO: Time to start getting rid of "luna" in visible pathnames
Settings::Settings()
	: lunaAppsPath( "/var/luna/applications/" )
	, pendingAppsPath( "/var/palm/data/com.palm.appInstallService" )
	, appInstallBase( "/media/cryptofs/apps" )
	, appInstallRelative( "usr/palm/applications" )
	, packageInstallBase( "/media/cryptofs/apps" )
	, packageInstallRelative( "usr/palm/packages" )
	, serviceInstallBase( "/media/cryptofs/apps" )
	, serviceInstallRelative( "usr/palm/services" )
	, packageManifestsPath("/media/cryptofs/apps/usr/lib/ipkg/info")
	, downloadPathMedia("/media/internal/downloads")
	, appInstallerTmp("/media/cryptofs/tmp")
	, lunaPresetLaunchPointsPath("/usr/luna/launchpoints/")
	, lunaLaunchPointsPath( "/var/luna/launchpoints/" )
	, lunaSystemPath( "/usr/lib/luna/system/luna-systemui/" )
	, lunaAppLauncherPath( "/usr/lib/luna/system/luna-applauncher/" )
	, lunaSystemResourcesPath( "/usr/palm/sysmgr/images/" )
	, lunaSystemLocalePath( "/usr/palm/sysmgr/localization" )
	, lunaCustomizationLocalePath( "/usr/palm/sysmgr-cust/localization")
	, lunaPrefsPath("/var/luna/preferences/")
	, lunaCmdHandlerPath("/usr/palm/command-resource-handlers.json")
	, lunaCmdHandlerSavedPath("/var/usr/palm/command-resource-handlers-active.json")
	, lunaQmlUiComponentsPath("/usr/palm/sysmgr/uiComponents/")
	, lunaScreenCapturesPath("/media/internal/screencaptures")
	, cardLimit(16)
	, showReticle(true)
	, lunaSystemSoundsPath("/usr/palm/sounds")
	, lunaDefaultAlertSound("alert.wav")
	, lunaDefaultRingtoneSound("phone.wav")
	, lunaSystemSoundAppClose("appclose")
	, lunaSystemSoundScreenCapture("shutter")
	, notificationSoundDuration(5000)
	, lightbarEnabled (false)
	, coreNaviScaler (75)
	, gestureAnimationSpeed (1000)
        , backlightOutdoorScale (250)
        , backlightDimScale (30)
        , backlightDarkScale (10)
	, displayWidth(320)
	, displayHeight(320)
	, displayNumBuffers(3)
	, ledPulseMaxBrightness (100)
	, ledPulseDarkBrightness (50)
	, enableAls(true)
	, disableLocking(false)
	, lockScreenTimeout(5000)
	, maxPenMoveFreq(30)
	, maxPaintLoad(6)				       // number of ms for paint routine
	, maxGestureChangeFreq(30)
	, maxTouchChangeFreq(30)
	, debug_trackInputEvents(false)
	, debug_enabled(false)
	, debug_piranhaDrawColoredOutlines(false)
	, debug_piranhaDisplayFps(false)
	, debug_showGestures(false)
	, debug_doVerboseCrashLogging(false)
    , debug_loopInCrashHandler(false)
	, tapRadius(12)
	, tapRadiusMin(5)
	, tapRadiusSquared(144)
	, tapRadiusShrinkPercent(10)
	, tapRadiusShrinkGranMs(50)
	, tapDoubleClickDuration(300)
    , enableTouchEventsForWebApps(false)
    , homeDoubleClickDuration(70)
	, dragRadiusSquared(64)
	, h_trackball_pixels_per_move(30)
	, v_trackball_pixels_per_move(40)
	, h_accel_rate1(200)
	, v_accel_rate1(200)
	, h_accel_const1(2)
	, v_accel_const1(1)
	, h_accel_rate2(500)
	, v_accel_rate2(500)
	, h_accel_const2(3)
	, v_accel_const2(2)
	, accelFastPollFreq (33)
	, turnOffAccelWhenDimmed(true)
    , logger_useSyslog(true)
	, logger_useTerminal(false)
	, logger_useColor(false)
    , logger_level(G_LOG_LEVEL_WARNING)
	, defaultLanguage("en_US")
	, launcherDefaultPositions("/etc/palm/default-launcher-page-layout.json")
	, launcherCustomPositions("/usr/lib/luna/customization/default-launcher-page-layout.json")
	, quicklaunchDefaultPositions("/usr/palm/default-dock-positions.json")
	, quicklaunchCustomPositions("/usr/lib/luna/customization/default-dock-positions.json")
	, quicklaunchUserPositions("/var/palm/user-dock-positions.json")
	, launcherScrim("/usr/lib/luna/system/luna-applauncher/images/launcher_scrim.png")
        , firstCardLaunch("/var/luna/preferences/used-first-card")
	, atlasEnabled(false)
	, cardGroupingXDistanceFactor(1.0)
	, atlasMemThreshold(0)
	, launcherAtlasStatistics(false)
	, launcherDumpAtlas(false)
	, launcherSideSwipeThreshold(1.2)
	, launcherUsesHwAA(false)
	, launcherRowSpacingAdjust(0)
	, launcherLabelWidthAdjust(0)
	, launcherLabelXPadding(12)
	, launcherIconReorderPositionThreshold(36.0)
	, statusBarTitleMaxWidth(140)
	, dockModePrelaunchAllApps(false)
	, dockModeCloseOnMinimize(true)
	, dockModeCloseOnExit(true)
	, dockModeMaxApps(3)
	, dockModeNightBrightness(1)
	, dockModeDefaultPositions("/etc/palm/default-exhibition-apps.json")
	, dockModeCustomPositions("/usr/lib/luna/customization/default-exhibition-apps.json")
	, dockModeUserPositions("/var/palm/user-exhibition-apps.json")
	, dockModeMenuHeight (400)
	, virtualKeyboardEnabled(false)
	, showNotificationsAtTop(false)
	, virtualCoreNaviEnabled(false)
	, virtualCoreNaviHeight(0)
	, uiType(UI_LUNA)
	, fontBanner("Prelude")
	, fontActiveBanner("Prelude")
	, fontLockWindow("Prelude")
	, fontDockMode("Prelude")
	, fontQuicklaunch("Prelude")
	, fontBootupAnimation("/usr/share/fonts/Prelude-Bold.ttf")
	, fontProgressAnimationBold("/usr/share/fonts/Prelude-Bold.ttf")
	, fontProgressAnimation("Prelude")
	, fontKeyboardKeys("Prelude")
	, fontStatusBar("Prelude")
	, displayUiRotates(false)
	, tabletUi(false)
	, homeButtonOrientationAngle(0)
	, positiveSpaceTopPadding(24)
	, positiveSpaceBottomPadding(24)
	, maximumNegativeSpaceHeightRatio(0.55)
	, activeCardWindowRatio(0.659)
	, nonActiveCardWindowRatio(0.61)
    , ghostCardFinalRatio(0.85)
	, cardGroupRotFactor(90)
	, gapBetweenCardGroups(10)
	, overlayNotificationsHeight(83)
	, splashIconSize(128)
    , enableSplashBackgrounds(true)
	, maxDownloadManagerQueueLength(128)
	, maxDownloadManagerConcurrent(2)
	, maxDownloadManagerRecvSpeed(64 * 1024)
	, showAppStats(false)
	, collectUseStats(true)
	, usePartialKeywordAppSearch(true)
	, scanCalculatesAppSizes(false)
	, uiMainCpuShareLow(512)
	, uiOtherCpuShareLow(128)
	, javaCpuShareLow(128)
	, webCpuShareLow(64)
	, gameCpuShareLow(32)
	, cpuShareDefault(1024)
	, cpuShareDefaultLow(512)
	, allowTurboMode(true)
	, wifiInterfaceName("eth0")
	, wanInterfaceName("ppp0")
	, canRestartHeadlessApps(true)
	, forceSoftwareRendering(false)
	, perfTesting(false)
	, debug_appInstallerCleaner(3)
	, modalWindowWidth(320)
	, modalWindowHeight(480)
	, demoMode(false)
	, logFileName("/var/log/lunasysmgr.log")
    , cardDimmPercentage(0.8f)
    , schemaValidationOption(0)
{

	load(kSettingsFile);
	load(kSettingsFilePlatform);

	postLoad();

}

Settings::~Settings()
{
}

#define KEY_STRING(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs ) { var=(const char*)_vs; g_free(_vs); }\
	else g_error_free(_error); \
}

#define KEY_MEMORY_STRING(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs ) { var=::MemStringToBytes((const char*)_vs); g_free(_vs); }\
	else g_error_free(_error); \
}

#define KEY_BOOLEAN(cat,name,var) \
{\
	gboolean _vb;\
	GError* _error = 0;\
	_vb=g_key_file_get_boolean(keyfile,cat,name,&_error);\
	if( !_error ) { var=_vb; }\
	else g_error_free(_error); \
}

#define KEY_INTEGER(cat,name,var) \
{\
	int _v;\
	GError* _error = 0;\
	_v=g_key_file_get_integer(keyfile,cat,name,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
}

#define KEY_DOUBLE(cat,name,var) \
{\
	double _v;\
	GError* _error = 0;\
	_v=g_key_file_get_double(keyfile,cat,name,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
}


// Parse the total RAM installed out of /proc/meminfo.
static int MeasureTotalRAM()
{
        gchar* buffer;
        gsize sz;
        int memTotal = 0;

        if( !g_file_get_contents( "/proc/meminfo", &buffer, &sz, 0 ) )
                return 0;

        char* ptr = strtok( buffer, ": \x0a\x0d" );
        while( ptr )
        {
                if( !strncmp( ptr,"MemTotal",8) )
                {
                        // next token is the ram
                        if( ( ptr = strtok( 0, ": \x0a\x0d" ) ) )
                        {
                                memTotal = atoi( ptr );
                        }

                        break;
                }

                ptr = strtok( 0, ": \x0a\x0d" );
        }

        g_free(buffer);
        return memTotal;
}

void Settings::load(const char* settingsFile)
{
	GKeyFile* keyfile;
	GKeyFileFlags flags;
	GError* error = 0;

	keyfile = g_key_file_new();
	if(!keyfile)
		return;
	flags = GKeyFileFlags( G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

	if( !g_key_file_load_from_file( keyfile, settingsFile, flags, &error ) )
	{
		g_key_file_free( keyfile );
		if (error) g_error_free(error);
		return ;
	}

	// Fill in with the macros above.
	KEY_STRING("General","ApplicationPath", lunaAppsPath );					// apps path can now be multiple paths, separated by :  (setenv PATH style)
	KEY_STRING("General","AppInstallBase", appInstallBase);	// due to all the churn surrounding moving of apps around, this is now a conf option
	KEY_STRING("General","AppInstallRelative", appInstallRelative);	// due to all the churn surrounding moving of apps around, this is now a conf option
//	KEY_STRING("General","PackageInstallBase", packageInstallBase);

	KEY_STRING("General","PackageInstallRelative", packageInstallRelative);

	KEY_STRING("General","PackageManifestsPath", packageManifestsPath);

	KEY_STRING("General","DownloadPathMedia",downloadPathMedia);
	//validate path, reset to default if necessary
	if (!validateDownloadPath(downloadPathMedia)) {
		downloadPathMedia = "/media/internal/downloads";
	}

	KEY_STRING("General","AppInstallTemp",appInstallerTmp);

	KEY_STRING("General","SystemPath", lunaSystemPath );
	KEY_STRING("General","AppLauncherPath", lunaAppLauncherPath );
	KEY_STRING("General","SystemResourcesPath", lunaSystemResourcesPath );
	KEY_STRING("General","SystemLocalePath", lunaSystemLocalePath );
	KEY_STRING("General","PresetLaunchPointsPath",lunaPresetLaunchPointsPath);
	KEY_STRING("General","LaunchPointsPath", lunaLaunchPointsPath);
	KEY_STRING("General", "PreferencesPath", lunaPrefsPath);
	KEY_STRING("General","UiComponentsPath", lunaQmlUiComponentsPath );
	KEY_STRING("General","ScreenCapturesPath", lunaScreenCapturesPath );

	KEY_BOOLEAN("General", "ShowReticle", showReticle);

	KEY_INTEGER("General", "NotificationSoundDuration", notificationSoundDuration);
	KEY_BOOLEAN("General", "ShowNotificationsAtTop", showNotificationsAtTop);

	KEY_INTEGER( "CoreNavi", "ThrobberBrightnessInLight", ledPulseMaxBrightness);
	KEY_INTEGER( "CoreNavi", "ThrobberBrightnessInDark", ledPulseDarkBrightness);
	KEY_BOOLEAN( "CoreNavi", "EnableLightBar", lightbarEnabled);
	KEY_INTEGER( "CoreNavi", "CoreNaviBrightnessScaler", coreNaviScaler);
	KEY_INTEGER( "CoreNavi", "GestureAnimationSpeedInMs", gestureAnimationSpeed);
    KEY_INTEGER( "CoreNavi", "HomeDoubleClickDuration", homeDoubleClickDuration);

	KEY_INTEGER( "Display", "BrightnessOutdoorScale", backlightOutdoorScale);
	KEY_INTEGER( "Display", "BrightnessDimScale", backlightDimScale);
	KEY_INTEGER( "Display", "BrightnessDarkScale", backlightDarkScale);

	KEY_BOOLEAN( "Display", "EnableALS", enableAls);
	KEY_BOOLEAN( "Display", "TurnOffAccelerometerWhenDimmed", turnOffAccelWhenDimmed);
	KEY_BOOLEAN( "Display", "DisableLocking", disableLocking);
	KEY_INTEGER( "Display", "LockScreenTimeoutMs", lockScreenTimeout);

	KEY_INTEGER( "Memory", "CardLimit", cardLimit );
	KEY_INTEGER( "General","DisplayWidth",displayWidth);
	KEY_INTEGER( "General","DisplayHeight",displayHeight);
	KEY_INTEGER( "General","DisplayNumBuffers", displayNumBuffers);
	KEY_INTEGER("General", "MaxPenMoveFreq", maxPenMoveFreq);
	KEY_INTEGER("General",  "MaxPaintLoad", maxPaintLoad);
	KEY_INTEGER("General", "MaxGestureChangeFreq", maxGestureChangeFreq);
	KEY_INTEGER("General", "MaxTouchChangeFreq", maxTouchChangeFreq);
	KEY_BOOLEAN( "Debug", "WatchPenEvents", debug_trackInputEvents );
	KEY_BOOLEAN( "Debug", "EnableDebugModeByDefault", debug_enabled );
	KEY_BOOLEAN( "Debug", "PiranhaDrawColoredOutlines", debug_piranhaDrawColoredOutlines);
	KEY_BOOLEAN( "Debug", "PiranhaDisplayFps", debug_piranhaDisplayFps);
	KEY_BOOLEAN( "Debug", "ShowGestures", debug_showGestures);

	KEY_BOOLEAN( "Debug", "DoVerboseCrashLogging", debug_doVerboseCrashLogging);
	KEY_BOOLEAN( "Debug", "LoopInCrashHandler", debug_loopInCrashHandler);
	KEY_INTEGER( "Debug", "AppInstallerCleaner",debug_appInstallerCleaner );
	KEY_STRING( "General", "CmdResourceHandlers", lunaCmdHandlerPath );
	KEY_STRING( "General", "CmdResourceHandlersActiveCopy", lunaCmdHandlerSavedPath);
	KEY_STRING( "Fonts", "Banner", fontBanner );
	KEY_STRING( "Fonts", "ActiveBanner", fontActiveBanner );
	KEY_STRING( "Fonts", "LockWindow", fontLockWindow );
	KEY_STRING( "Fonts", "DockMode", fontDockMode );
	KEY_STRING( "Fonts", "Quicklaunch", fontQuicklaunch );
	KEY_STRING( "Fonts", "StatusBar", fontStatusBar );
	KEY_STRING( "Fonts", "KeyboardKeys", fontKeyboardKeys );
	KEY_STRING( "Fonts", "StatusBar", fontStatusBar );

	KEY_INTEGER("TouchEvents", "TapRadiusMax", tapRadius);
	KEY_INTEGER("TouchEvents", "TapRadiusMin", tapRadiusMin);
	KEY_INTEGER("TouchEvents", "TapRadiusShrinkPerc", tapRadiusShrinkPercent);
	KEY_INTEGER("TouchEvents", "TapRadiusShrinkGranMs", tapRadiusShrinkGranMs);
    KEY_BOOLEAN("TouchEvents", "EnableForWebApps", enableTouchEventsForWebApps);

	tapRadiusSquared = tapRadius * tapRadius;

	KEY_INTEGER("TouchEvents", "DoubleClickDuration", tapDoubleClickDuration);
	// Clamp to sensible values
	if (tapDoubleClickDuration < 50)
		tapDoubleClickDuration = 50;
	else if (tapDoubleClickDuration > 2000)
		tapDoubleClickDuration = 2000;

	KEY_INTEGER("VTrackBall", "PixelsPerMoveH", h_trackball_pixels_per_move );
	KEY_INTEGER("VTrackBall", "PixelsPerMoveV", v_trackball_pixels_per_move );
	KEY_INTEGER("VTrackBall", "AccelRateH1", h_accel_rate1 );
	KEY_INTEGER("VTrackBall", "AccelRateV1", v_accel_rate1 );
	KEY_INTEGER("VTrackBall", "AccelConstH1", h_accel_const1 );
	KEY_INTEGER("VTrackBall", "AccelConstV1", v_accel_const1 );
	KEY_INTEGER("VTrackBall", "AccelRateH2", h_accel_rate2 );
	KEY_INTEGER("VTrackBall", "AccelRateV2", v_accel_rate2 );
	KEY_INTEGER("VTrackBall", "AccelConstH2", h_accel_const2 );
	KEY_INTEGER("VTrackBall", "AccelConstV2", v_accel_const2 );

	KEY_STRING("General", "QuickLaunchDefaultPositions", quicklaunchDefaultPositions );
#if defined(TARGET_DESKTOP)
	//If we don't have a home directory, we'll set it to /tmp. Otherwise, this does nothing.
	setenv("HOME", "/tmp", 0);
        const std::string homeFolder = getenv("HOME");
	quicklaunchUserPositions = homeFolder + "/.user-dock-positions.json";
#endif
	KEY_STRING("General", "QuickLaunchUserPositions", quicklaunchUserPositions );
	KEY_INTEGER( "General", "StatusBarTitleMaxWidth", statusBarTitleMaxWidth);

	KEY_BOOLEAN( "DockMode", "DockModePrelaunchAllApps", dockModePrelaunchAllApps);
	KEY_BOOLEAN( "DockMode", "DockModeCloseAppOnMinimize", dockModeCloseOnMinimize);
	KEY_BOOLEAN( "DockMode", "DockModeCloseAppsOnExit", dockModeCloseOnExit);
	KEY_INTEGER( "DockMode", "DockModeMaxApps", dockModeMaxApps);
	KEY_INTEGER( "DockMode", "DockModeNightBrightness", dockModeNightBrightness);
	KEY_INTEGER( "DockMode", "DockModeMenuHeight", dockModeMenuHeight);
	KEY_STRING("DockMode", "DockModeDefaultPositions", quicklaunchDefaultPositions );
#if defined(TARGET_DESKTOP)
	dockModeUserPositions = homeFolder + "/.user-dock-mode-launcher-positions.json";
#endif
	KEY_STRING("DockMode", "DockModeUserPositions", quicklaunchUserPositions );

	KEY_BOOLEAN( "VirtualKeyboard", "VirtualKeyboardEnabled", virtualKeyboardEnabled);

	KEY_BOOLEAN( "VirtualCoreNavi", "VirtualCoreNaviEnabled", virtualCoreNaviEnabled);
	KEY_INTEGER( "VirtualCoreNavi", "VirtualCoreNaviHeight", virtualCoreNaviHeight);

	KEY_DOUBLE("Launcher", "CardSideScrollSwipeThreshold", launcherSideSwipeThreshold);
	KEY_BOOLEAN("Launcher", "UseOGLHardwareAntialias", launcherUsesHwAA);
	KEY_INTEGER("Launcher", "LauncherItemRowSpacingAdjust",launcherRowSpacingAdjust);
	KEY_INTEGER("Launcher", "LauncherLabelWidthAdjust",launcherLabelWidthAdjust);
	KEY_INTEGER("Launcher", "LauncherLabelXPadding",launcherLabelXPadding);
	KEY_DOUBLE("Launcher","LauncherIconReorderPositionThreshold",launcherIconReorderPositionThreshold);

	KEY_BOOLEAN("UI", "DisplayUiRotates", displayUiRotates);
	KEY_BOOLEAN("UI", "TabletUi", tabletUi);
	KEY_INTEGER("UI", "HomeButtonOrientationAngle", homeButtonOrientationAngle);
	KEY_INTEGER("UI", "PositiveSpaceTopPadding", positiveSpaceTopPadding);
	KEY_INTEGER("UI", "PositiveSpaceBottomPadding", positiveSpaceBottomPadding);
	KEY_DOUBLE("UI", "MaximumNegativeSpaceHeightRatio", maximumNegativeSpaceHeightRatio);
	KEY_DOUBLE("UI", "ActiveCardWindowRatio", activeCardWindowRatio);
	KEY_DOUBLE("UI", "NonActiveCardWindowRatio", nonActiveCardWindowRatio);
	KEY_DOUBLE("UI", "GhostCardFinalRatio", ghostCardFinalRatio);
	KEY_INTEGER("UI", "CardGroupRotFactor", cardGroupRotFactor);
	KEY_INTEGER("UI", "GapBetweenCardGroups", gapBetweenCardGroups);
	KEY_INTEGER("UI", "OverlayNotificationsHeight", overlayNotificationsHeight);
	KEY_INTEGER("UI", "SplashIconSize", splashIconSize);
	KEY_BOOLEAN("UI", "EnableSplashBackgrounds", enableSplashBackgrounds);
	KEY_BOOLEAN("UI", "AtlasEnabled", atlasEnabled);

	KEY_INTEGER("UI", "ModalWindowWidth", modalWindowWidth);
	KEY_INTEGER("UI", "ModalWindowHeight", modalWindowHeight);

	KEY_DOUBLE("UI", "CardGroupingXDistanceFactor", cardGroupingXDistanceFactor);
    KEY_DOUBLE("UI", "CardDimmPercentage", cardDimmPercentage);

	KEY_INTEGER("UI", "AtlasMemThreshold", atlasMemThreshold);
	KEY_BOOLEAN("Debug", "LauncherAtlasStatistics", launcherAtlasStatistics);
	KEY_BOOLEAN("Debug", "DumpLauncherAtlas", launcherDumpAtlas);
	if (forceSoftwareRendering) {
		atlasEnabled = false;
		launcherAtlasStatistics = false;
	} else if (atlasEnabled && atlasMemThreshold > 0 && MeasureTotalRAM() < atlasMemThreshold * 1024) {
		g_message("Atlas disabled because physical memory below %dMB threshold\n", atlasMemThreshold);
		atlasEnabled = false;
	}

	KEY_INTEGER("DownloadManager", "MaxQueueLength", maxDownloadManagerQueueLength);
	KEY_INTEGER("DownloadManager", "MaxConcurrent", maxDownloadManagerConcurrent);
	KEY_INTEGER("DownloadManager", "MaxRecvSpeed", maxDownloadManagerRecvSpeed);

	KEY_BOOLEAN( "Demo", "DemoMode", demoMode );

	KEY_BOOLEAN( "Debug", "ShowAppStats", showAppStats );

	KEY_BOOLEAN( "General", "CollectUseStats", collectUseStats );

	KEY_BOOLEAN( "General" , "UsePartialKeywordMatchForAppSearch",usePartialKeywordAppSearch);
	KEY_BOOLEAN( "General" , "ScanCalculatesAppSizes",scanCalculatesAppSizes);

	KEY_INTEGER("KeepAlive", "MaxParked", maxNumParkedApps );

	KEY_INTEGER("CpuShare", "UiMainLow", uiMainCpuShareLow);
	KEY_INTEGER("CpuShare", "UiOtherLow", uiOtherCpuShareLow);
	KEY_INTEGER("CpuShare", "JavaLow", javaCpuShareLow);
	KEY_INTEGER("CpuShare", "WebLow", webCpuShareLow);
	KEY_INTEGER("CpuShare", "GameLow", gameCpuShareLow);
	KEY_INTEGER("CpuShare", "Default", cpuShareDefault);

	KEY_BOOLEAN("AllowTurboMode", "General", allowTurboMode);

	KEY_STRING( "General", "WifiInterfaceName", wifiInterfaceName );
	KEY_STRING( "General", "WanInterfaceName", wanInterfaceName );

	KEY_BOOLEAN( "Memory", "CanRestartHeadlessApps", canRestartHeadlessApps );
	KEY_BOOLEAN( "Debug", "PerformanceLogs", perfTesting);
	KEY_STRING( "General", "LogFileName", logFileName);

    KEY_INTEGER("General", "schemaValidationOption", schemaValidationOption);


	// apps to launch at boot time

	gchar** appsToLaunchAtBootStr =   g_key_file_get_string_list(keyfile, "LaunchAtBoot",
																 "Applications", NULL, NULL);
	if (appsToLaunchAtBootStr) {

		int index = 0;
		appsToLaunchAtBoot.clear();
		while (appsToLaunchAtBootStr[index]) {
			appsToLaunchAtBoot.insert(appsToLaunchAtBootStr[index]);
			SETTINGS_TRACE("App to launch at boot time: %s\n", appsToLaunchAtBootStr[index]);
			++index;
		}

		g_strfreev(appsToLaunchAtBootStr);
	}

	// apps to keep alive
	gchar** appsToKeepAliveStr =   g_key_file_get_string_list(keyfile, "KeepAlive",
																 "Applications", NULL, NULL);
	if (appsToKeepAliveStr) {

		int index = 0;
		appsToKeepAlive.clear();
		while (appsToKeepAliveStr[index]) {
			appsToKeepAlive.insert(appsToKeepAliveStr[index]);
			SETTINGS_TRACE("App to keep alive: %s\n", appsToKeepAliveStr[index]);
			++index;
		}

		g_strfreev(appsToKeepAliveStr);
	}

	// apps to keep alive forever (pinned)
	gchar** appsToKeepAliveForeverStr =   g_key_file_get_string_list(keyfile, "KeepAliveUntilMemPressure",
																 "Applications", NULL, NULL);
	if (appsToKeepAliveForeverStr) {

		int index = 0;
		appsToKeepAliveUntilMemPressure.clear();
		while (appsToKeepAliveForeverStr[index]) {
			appsToKeepAliveUntilMemPressure.insert(appsToKeepAliveForeverStr[index]);
			SETTINGS_TRACE("App to keep alive until memory pressure: %s\n", appsToKeepAliveForeverStr[index]);
			++index;
		}

		g_strfreev(appsToKeepAliveForeverStr);
	}

	// apps to allow under low memory conditions
	gchar** appsToAllowInLowMemoryStr = g_key_file_get_string_list(keyfile, "Memory",
																   "AppsToAllowInLowMemory", NULL, NULL);
	if (appsToAllowInLowMemoryStr) {

		int index = 0;
		appsToAllowInLowMemory.clear();
		while (appsToAllowInLowMemoryStr[index]) {
			appsToAllowInLowMemory.insert(appsToAllowInLowMemoryStr[index]);
			g_message("App to allow in Low memory: %s", appsToAllowInLowMemoryStr[index]);
			++index;
		}

		g_strfreev(appsToAllowInLowMemoryStr);
	}

	// apps with accelerated compositing disabled
	gchar** appsToDisableAccelCompositingStr = g_key_file_get_string_list(keyfile, "AccelCompositingDisabled",
											"Applications", NULL, NULL);
	if (appsToDisableAccelCompositingStr) {
		int index = 0;
		appsToDisableAccelCompositing.clear();
		while (appsToDisableAccelCompositingStr[index]) {
			appsToDisableAccelCompositing.insert(appsToDisableAccelCompositingStr[index]);
			SETTINGS_TRACE("App with accelerated compositing disabled: %s\n", appsToDisableAccelCompositingStr[index]);
			++index;
		}

		g_strfreev(appsToDisableAccelCompositingStr);
	}

	// SUC apps that have special launch privs
	gchar** sucAppsList =   g_key_file_get_string_list(keyfile, "SUCApps",
			"Applications", NULL, NULL);
	if (sucAppsList) {

		int index = 0;
		sucApps.clear();
		while (sucAppsList[index]) {
			sucApps.insert(sucAppsList[index]);
			SETTINGS_TRACE("SUC App with special launch priviledge: %s\n", sucAppsList[index]);
			++index;
		}

		g_strfreev(sucAppsList);
	}

	// ...

	g_key_file_free( keyfile );

	// sanity check on the homeButtonOrientationAngle value:

	if(homeButtonOrientationAngle >= 360)
		homeButtonOrientationAngle = homeButtonOrientationAngle%360;

	if(homeButtonOrientationAngle < -90)
		homeButtonOrientationAngle += 360;

	if((homeButtonOrientationAngle != 0) && (homeButtonOrientationAngle != 90) &&
	   (homeButtonOrientationAngle != 180) && (homeButtonOrientationAngle != 270) &&
	   (homeButtonOrientationAngle != -90))
		homeButtonOrientationAngle = 0;
}

void Settings::postLoad()
{
	//POST-PROCESS lunaAppsPath....there may be multiple paths embedded
	//int splitStringOnKey(std::vector<std::string>& returnSplitSubstrings,const std::string& baseStr,const std::string& delims);
	int numPaths = splitStringOnKey(lunaAppsPaths,lunaAppsPath,":");
	g_warning("Settings::load(): %d application paths defined: ",numPaths);
	std::vector<std::string>::iterator iter = lunaAppsPaths.begin();
	while (iter != lunaAppsPaths.end()) {
		SETTINGS_TRACE("%s ",(*iter).c_str());
		++iter;
	}
	SETTINGS_TRACE("\n");

	//reset the lunaAppsPath (LEGACY compatibility)
	lunaAppsPath = lunaAppsPaths.at(0);

	createNeededFolders();

	// packageInstallBase has to be == to appInstallBase for now (at least in version=blowfish timeframe)
	packageInstallBase = appInstallBase;

	// Piranha flags
}

// Expands "1MB" --> 1048576, "2k" --> 2048, etc.
unsigned long MemStringToBytes( const char* ptr )
{
	char number[32];
	unsigned long r = 0;
	const char* s= ptr;

	while( *ptr && !isalnum(*ptr) ) // skip whitespace
		ptr++;
	s=ptr;

	while( isdigit(*ptr) )
		ptr++;

	strncpy( number, s, (size_t)(ptr-s) );
	number[ptr-s]=0;

	r = (unsigned long)atol(number);
	switch(*ptr)
	{
	case 'M':
		r *= 1024 * 1024; break;
	case 'k':
	case 'K':
		r *= 1024 ; break;
	}

	return r;
}

void Settings::createNeededFolders()
{
	g_mkdir_with_parents(lunaLaunchPointsPath.c_str(), 0755);
	g_mkdir_with_parents(lunaPrefsPath.c_str(), 0755);
	g_mkdir_with_parents(downloadPathMedia.c_str(),0755);
	g_mkdir_with_parents(appInstallerTmp.c_str(),0755);
	g_mkdir_with_parents(packageManifestsPath.c_str(),0755);
	g_mkdir_with_parents((appInstallBase+std::string("/")+appInstallRelative).c_str(),0755);
	g_mkdir_with_parents((packageInstallBase+std::string("/")+packageInstallRelative).c_str(),0755);
	g_mkdir_with_parents("/var/usr/palm",0755);
	g_mkdir_with_parents(lunaScreenCapturesPath.c_str(),0755);
}
