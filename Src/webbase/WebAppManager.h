/**
 * @file
 * 
 * Manages running web apps.
 *
 * @author Hewlett-Packard Development Company, L.P.
 * @author tyrok1
 *
 * @section LICENSE
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
 */




#ifndef BROWSERAPPMANAGER_H
#define BROWSERAPPMANAGER_H

#include "Common.h"

#include <map>
#include <list>
#include <string>
#include <QString>

#include <QApplication>

#include "SyncTask.h"
#include "Event.h"
#include "MemoryWatcher.h"
#include "SignalSlot.h"
#include "Timer.h"
#include "Window.h"
#include "WebAppCache.h"
#include "HostBase.h"
#include "lunaservice.h"
#include <PIpcBuffer.h>
#include <PIpcClient.h>
#include <PIpcChannelListener.h>
#include <PIpcMessage.h>


class WebAppBase;
class ProcessBase;
class WindowedWebApp;
class WebKitEventListener;
class PIpcChannel;
class PalmClipboard;
class SharedGlobalProperties;
class SysMgrKeyEvent;
class SysMgrWebBridge;

#define WEB_APP_MGR_IPC_NAME "WebAppManager"

/**
 * Manages running web apps
 */
class WebAppManager : public SyncTask
					, public Trackable
					, public PIpcClient
					, public PIpcChannelListener
{
public:
	/**
	 * Gets this process's WebAppManager instance
	 * 
	 * WebAppManager is designed as a singleton and can only
	 * be constructed once per process.  This method returns
	 * the allowed instance of WebAppManager for this process
	 * and constructs one if one has not yet been created.
	 * 
	 * @return				This process's instance of WebAppManager.
	 */
	static WebAppManager* instance();
	
	/**
	 * Checks whether or not a WebAppManager instance has been constructed for this process
	 * 
	 * Return value indicates whether or not
	 * {@link WebAppManager::instance() WebAppManager::instance()}
	 * has been called yet for this process.
	 * 
	 * @see WebAppManager::instance()
	 * 
	 * @return				Whether or not a WebAppManager instance has been constructed for this process.
	 */
	static bool hasBeenInstanced();
	
	/**
	 * Gets the time zone from the last time the clock was checked
	 * 
	 * @return				System's current time zone.
	 */
	static const std::string getTimeZone();
	
	/**
	 * Shuts down and cleans up this process's instance of WebAppManager
	 * 
	 * Closes all web apps managed by this WebAppManager.
	 * 
	 * @todo Confirm this.
	 */
	virtual ~WebAppManager();
	
	//Documented in parent
	virtual void run();
	
	/**
	 * Gets the device's current orientation
	 * 
	 * Returns which side of the display is currently facing up.
	 * 
	 * @return				Current display orientation.
	 */
	Event::Orientation orientation() const;
	
	/**
	 * Get a list of process information for all running web apps
	 * 
	 * This class manages web apps running on the system.  It tracks
	 * all apps and stores information about them.  This method
	 * returns system information about them, such as their process
	 * IDs.
	 * 
	 * @return				List of information about running web app processes.
	 */
	std::list<const ProcessBase*>	runningApps() ;
	
	/**
	 * Get a list of process information for all running web apps filtered to only those of a certain type
	 * 
	 * This class manages web apps running on the system.  It tracks
	 * all apps and stores information about them.  This method
	 * returns system information about them, such as their process
	 * IDs, but only those of a given type.
	 * 
	 * @param	winType			Type of app to look for.
	 * @return				List of information about running web app processes of the requested type.
	 */
	std::list<const ProcessBase*>	runningApps(Window::Type winType);
	
	/**
	 * Find an app by process ID
	 * 
	 * @param	processID		Process ID of the app to look for.
	 * @return				The app itself, if found.
	 */
	WebAppBase* findApp(const QString& processId);
	
	/**
	 * Find an app by app ID
	 * 
	 * @param	appID			App ID of the app to look for.
	 * @return				The app itself, if found.
	 */
	WebAppBase* findAppById(const QString& appId);
	
	/**
	 * Sets up reporting of stats to Luna
	 * 
	 * Sets up a recurring time interval to report stats from
	 * Palm::MemStats::getJSON to Luna.
	 * 
	 * @see Palm::MemStats::getJSON()
	 */
	void initiateLunaStatsReporting();
	
	/**
	 * Gets a Luna Service handle to talk to the statistics service
	 * 
	 * Gets a handle to the statistics service used to report memory
	 * statistics to Luna.
	 * 
	 * @todo Document this a bit more thoroughly once LSPalmService is documented.
	 * 
	 * @see LSPalmService
	 * 
	 * @return				Handle to the statistics service.
	 */
	LSHandle* getStatsServiceHandle() const;
	
	/**
	 * Adds a banner to indicate that text has been copied to the clipboard
	 * 
	 * @todo Document this more fully once BannerMessageEvent and BannerMessageEventFactory are documented
	 * 
	 * @param	appId			App ID of the app that copied text to the clipboard.
	 */
	void copiedToClipboard(const QString& appId);
	
	/**
	 * Doesn't do anything
	 * 
	 * Doesn't do anything right now.
	 * 
	 * @param	appId			App ID of the app that pasted text from the clipboard.
	 */
	void pastedFromClipboard(const QString& appId);
	
	/**
	 * Close a page after processing events
	 * 
	 * Queues up an event to close the given page so that once the
	 * event queue finishes processing all current events it goes
	 * through and closes all pages which WebAppManager has been
	 * asked to close by then.
	 * 
	 * @param	page			Page to queue up to close.
	 */
	void closePageSoon(SysMgrWebBridge* page);
	
	/**
	 * Request that the given page be closed.
	 */
	void closePageRequest(SysMgrWebBridge* page);
    	
	/**
	 * Dispatch an InterProcess Communication message to the correct spot
	 * 
	 * Dispatches an IPC message to an app or to one of the
	 * onSomething methods of this class depending on the type of
	 * message.
	 * 
	 * @param	msg			IPC message to process.
	 */
	void onMessageReceived(const PIpcMessage& msg);
	
	/**
	 * Logs a message indicating that a remote server disconnected
	 * 
	 * @todo Figure out the purpose for this.
	 * @todo See if this can be removed entirely.
	 */
	void onDisconnected();
	
	/**
	 * Sets options in Palm::WebGlobal
	 * 
	 * @todo Document this once Palm::WebGlobal is publicly documented.
	 * 
	 * @param	flags			Command-line-style flags to process (currently only --timeout_script_execution and --notimeout_script_execution seem to be specifically supported).
	 */
	void setJavascriptFlags( const std::string& flags );
	
	/**
	 * Sends an InterProcess Communication message
	 * 
	 * @todo Document this once PIpcClient and PIpcChannelListener are publicly documented, since it appears this method uses functionality from one of them.
	 * 
	 * @param	msg			IPC message to dispatch.
	 */
	void sendAsyncMessage( PIpcMessage* msg);
	
	//Documented in parent.
	//No idea why this is here.
	using TaskBase::mainLoop;
	
	/**
	 * Lets the bootup screen know that Universal Search is ready
	 * 
	 * The boot process waits until Universal Search is available
	 * before it considers itself "done".  This lets it know that
	 * Universal Search is all set and ready to use.
	 * 
	 * @see WebAppManager::bootFinished()
	 */
	void markUniversalSearchReady();
	
	/**
	 * Finds an app by its process ID and closes it
	 * 
	 * @param	processId		Process ID of the app to close.
	 */
	void closeByProcessId( const std::string& processId );

	// Host Info data cache
	/**
	 * Caches a version of the host device information structure
	 * 
	 * This structure contains information about the device that
	 * LunaSysMgr is running on, and is quite useful for checking
	 * what size the screen is.  By caching it here, it doesn't
	 * have to be looked up every time it's used.
	 * 
	 * This can be retrieved from HostBase::instance()->getInfo().
	 * 
	 * @see HostBase::getInfo()
	 * 
	 * @param	hostInfo		Host device info to cache.
	 */
	const void setHostInfo(const HostInfo *hostInfo);
	
	/**
	 * Gets the cached host device information structure
	 * 
	 * Retrieves the cached HostInfo structure if it has already
	 * been stored using
	 * {@link WebAppManager::setHostInfo() WebAppManager::setHostInfo()}.
	 * 
	 * @see WebAppManager::setHostInfo()
	 * 
	 * @return				Cached host device info.
	 */
	const HostInfo& getHostInfo() const;
	
	/**
	 * Reports to the Luna statistics service that an app has been launched
	 * 
	 * @todo Figure out where this is being used and document this in a bit more detail so we know why it's needed.
	 * 
	 * @param	appId			App ID of the app which has been launched.
	 * @param	processId		Process ID of the app which has been launched.
	 */
	void reportAppLaunched(const QString& appId, const QString& processId);
	
	/**
	 * Reports to the Luna statistics service that an app has been closed
	 * 
	 * @todo Figure out where this is being used and document this in a bit more detail so we know why it's needed.
	 * 
	 * @param	appId			App ID of the app which has been closed.
	 * @param	processId		Process ID of the app which has been closed.
	 */
	void reportAppClosed(const QString& appId, const QString& processId);
	
	/**
	 * Gets the width of the UI in pixels
	 * 
	 * @todo Document this once WebAppManager::onUiDimensionsChanged() is documented.
	 * 
	 * @see WebAppManager::onUiDimensionsChanged()
	 * 
	 * @return				Width of UI.
	 */
	int currentUiWidth() { return m_uiWidth; }
	
	/**
	 * Gets the height of the UI in pixels
	 * 
	 * @todo Document this once WebAppManager::onUiDimensionsChanged() is documented.
	 * 
	 * @see WebAppManager::onUiDimensionsChanged()
	 * 
	 * @return				Height of UI.
	 */
	int currentUiHeight() { return m_uiHeight; }
	
	/**
	 * Returns whether, in the current device orientation, the rotated screen is taller than wide
	 * 
	 * Checks to see, with the host device info structure and
	 * current orientation, if the screen is being held in
	 * such a way that the "vertical" direction is taller
	 * than the "horizontal" direction.
	 * 
	 * @return				true if the device is currently in a portrait orientation, false if in landscape mode.
	 */
	bool isDevicePortraitType() { return m_deviceIsPortraitType; }
	
	/**
	 * Disables/enables caching of apps when they're closed
	 * 
	 * By default under certain circumstances apps can be
	 * cached when they're closed.  This allows you to
	 * disable/enable that functionality.
	 * 
	 * @todo Document this a bit more fully once WebAppBase::freezeInCache() is fully documented.
	 * 
	 * @param	disable			true to disable app caching, false to enable it.
	 */
	void disableAppCaching(bool disable);
	
	/**
	 * Unknown at this time.
	 * 
	 * @todo Document this once SharedGlobalProperties is documented.
	 * 
	 * @see SharedGlobalProperties
	 * 
	 * @return				Unknown at this time.
	 */
	static SharedGlobalProperties* globalProperties();
	
	/**
	 * Unknown at this time
	 * 
	 * @todo See if this is actually used anywhere and for what.
	 * @todo See if we can remove this.
	 * 
	 * @param	val			Unknown at this time.
	 */
	void setInSimulatedMouseEvent(bool val) { m_inSimulatedMouseEvent = val; }
	
	/**
	 * Unknown at this time
	 * 
	 * @todo See if this is actually used anywhere and for what.
	 * @todo See if we can remove this.
	 * 
	 * @return				Unknown at this time.
	 */
	bool inSimulatedMouseEvent() const { return m_inSimulatedMouseEvent; }
	void setActiveAppId(const std::string& id) { m_activeAppId = id; }
	const std::string& getActiveAppId() { return m_activeAppId; }
protected:
	// IPC control message handlers
	/**
	 * Handle an IPC message asking us to open a URL
	 * 
	 * This method is run whenever the Palm InterProcess
	 * Communication hands us a message from another process
	 * asking us to open a URL.
	 * 
	 * @param	url			URL to open.
	 * @param	winType			Type of window to open.
	 * @param	procId			Process ID to use for the new WebPage being opened.
	 * @param	launchingAppId		App ID of the app launching this URL.
	 * @param	launchingProcId		Process ID of the app launching this URL.
	 */
	void onLaunchUrl(const std::string& url, int winType,
		 		     const std::string& appDesc, const std::string& procId,
					 const std::string& args, const std::string& launchingAppId,
					 const std::string& launchingProcId);
	
	/**
	 * Handle an IPC message asking us to launch a URL as a child window of another process
	 * 
	 * This method is run whenever the Palm InterProcess
	 * Communication hands us a message from another process
	 * asking us to open a URL as a child of another window.
	 * 
	 * @todo Document isHeadless parameter if we can find a reason for it to exist.
	 * 
	 * @param	url			URL to open.
	 * @param	winType			Type of window to open.
	 * @param	procId			Process ID to use for the new WebPage being opened.
	 * @param	launchingAppId		App ID of the app launching this URL.
	 * @param	launchingProcId		Process ID of the app launching this URL.
	 * @param	errorCode		Variable (passed by reference) for this method to return an error code into if something goes wrong.
	 * @param	isHeadless		Ignored, at the moment.  Purpose is currently unclear.
	 */
	void onLaunchUrlChild(const std::string& url, int winType,
			 		      const std::string& appDesc, const std::string& procId,
						  const std::string& args, const std::string& launchingAppId,
						  const std::string& launchingProcId, int& errorCode, bool isHeadless);
	
	/**
	 * Finds an app and relaunches it
	 * 
	 * Unsure at the moment why an app would need to be
	 * relaunched.
	 * 
	 * @todo Document this further once WebAppBase::relaunch() is documented.
	 * 
	 * @see WebAppBase::relaunch()
	 * 
	 * @param	procId			Process ID of the app to relaunch.
	 * @param	args			Launch arguments to pass to the app when it relaunches.
	 * @param	launchingAppId		App ID of the app requesting the relaunch.
	 * @param	launchingProcId		Process ID of the app requesting the relaunch.
	 */
	void onRelaunchApp(const std::string& procId, const std::string& args,
			  		   const std::string& launchingAppId, const std::string& launchingProcId);

	//void onInputEvent(int ipcKey, const SysMgrEventWrapper& wrapper);
	
	/**
	 * Run when device orientation has changed
	 * 
	 * Lets all open WebApps know that "up" has changed
	 * direction and that their displays need to rotate to
	 * match the screen's direction.
	 * 
	 * @param	orient			New orientation - one of Event::Orientation_Up, Event::Orientation_Down, Event::Orientation_Left, Event::Orientation_Right.
	 */
	void onSetOrientation(int orient);
	
	/**
	 * Unknown at the moment
	 * 
	 * Both the purpose and function of this are currently unknown.
	 * 
	 * @param	key			Unknown.
	 */
	void onGlobalProperties(int key);
	
	/**
	 * Calls WebAppBase::inspect() for an app
	 * 
	 * Unknown exactly what the function of this is.
	 * 
	 * @todo Document this fully once WebAppBase::inspect() is fully documented.
	 * 
	 * @param	processId		Process ID of the app to inspect.
	 */
	void onInspectByProcessId( const std::string& processId );
	
	/**
	 * Ask MemoryWatcher and the main SysMgr process to perform low memory actions
	 * 
	 * @todo Document this fully once MemoryWatcher::doLowMemActions() and SysMgr's low memory actions are fully documented.
	 * 
	 * @param	allowExpensive		Guessing this is whether or not to spend additional time to clean up memory due to running really low.
	 */
	void performLowMemoryActions( const bool allowExpensive = false );
	
	/**
	 * Missing implementation
	 * 
	 * Can't seem to find an implementation of this.  Without that, it's
	 * unknown what this does.
	 * 
	 * @param	pid			Unknown.
	 * @param	maxMemory		Unknown.
	 * @param	updateFromPid		Unknown.
	 */
	void monitorNativeProcessMemory( const pid_t pid, const int maxMemory, pid_t updateFromPid = 0 );
	
	/**
	 * Clears the global system WebKit cache
	 * 
	 * @see Palm::WebGlobal::clearDOMCache()
	 */
	void clearWebkitCache();
	
	/**
	 * Enables the debugger
	 * 
	 * Current just a wrapper for
	 * {@link Palm::WebGlobal::enableDebugger() Palm::WebGlobal::enableDebugger()}.
	 * 
	 * @see Palm::WebGlobal::enableDebugger()
	 * 
	 * @todo Document this more fully once Palm::WebGlobal::enableDebugger() is publicly documented.
	 */
	void enableDebugger( bool enable );
	
	/**
	 * Handles an IPC message to close all apps
	 * 
	 * If TARGET_DESKTOP is defined, this method deletes
	 * all open apps from memory then runs the garbage
	 * collector to clean up memory.
	 * 
	 * Either way, it then closes down the GLib main loop
	 * and returns.
	 */
	void onShutdownEvent();
	
	/**
	 * Handles an IPC message to close an app
	 * 
	 * Closes all instances of a given app in response
 	 * to something else asking us (via InterProcess
	 * Communication message) to close an app.
	 * 
	 * @param	appId			App ID of the app to close.
	 * @return				true if one or more instances of the given app were open and have now been closed or false if the app was not already open.
	 */
	bool onKillApp(const std::string& appId);
	
	/**
	 * Handles an IPC message to close an app and notify the caller when done
	 * 
	 * Closes all instances of a given app in response
 	 * to something else asking us (via InterProcess
	 * Communication message) to close an app.
	 * 
	 * Differs from
	 * {@link WebAppManager::onKillApp() WebAppManager::onKillApp()}
	 * in that this one returns a result back to the
	 * caller to let them know that, yes, we did close
	 * everything asked of us.
	 * 
	 * @see WebAppManager::onKillApp()
	 * 
	 * @param	appId			App ID of the app to close.
	 * @param	result			Where to store the result of closing apps.  Value will be set to true if one or more instances of the given app were open and have now been closed or false if the app was not already open.
	 */
	void onSyncKillApp(const std::string& appId, bool* result);
	
	/**
	 * Handles an IPC message asking us to launch an app
	 * 
	 * Launches a web app given its App ID.  Pretty
	 * simple.  See
	 * {@link ProcessManager::launch() ProcessManager::launch()}
	 * for more information (it's actually quite
	 * well-documented already.
	 * 
	 * @see ProcessManager::launch()
	 * 
	 * @param	appDescString		App ID of the web app to launch.
	 * @param	params			JSON launch parameters to pass to the new app.
	 * @param	launchingAppId		App ID of the app asking us to launch this new app.
	 * @param	launchingProcId		Process ID of the app asking us to launch this new app (can be NULL).
	 */
	void onProcMgrLaunch(const std::string& appDescString, const std::string& params,
                         const std::string& launchingAppId, const std::string& launchingProcId);
	
	/**
	 * Handles an IPC message asking us to launch an app as a modal child of another app
	 * 
	 * Launches a web app given its App ID.  Differs
	 * from
	 * {@link WebAppManager::onProcMgrLaunch() WebAppManager::onProcMgrLaunch()}
	 * in that the new app is set up as a child of
	 * the given launching parent and is launched
	 * in a modal configuration where it must be
	 * dismissed in some way before returning to
	 * the parent.
	 * 
	 * @see ProcessManager::launchModal()
	 * 
	 * @param	appDescString		App ID of the web app to launch.
	 * @param	params			JSON launch parameters to pass to the new app.
	 * @param	launchingAppId		App ID of the app asking us to launch this new app.
	 * @param	launchingProcId		Process ID of the app asking us to launch this new app (can be NULL).
	 * @param	isHeadless		true to create an app with no visible window, false to create a modal child app.
	 * @param	isParentPdk		Pass in true if the parent is a PDK app instead of a web app, false otherwise.
	 */
	void onProcMgrLaunchChild(const std::string& appDescString, const std::string& params,
	                         const std::string& launchingAppId, const std::string& launchingProcId, bool isHeadless, bool isParentPdk);
	
	/**
	 * Appears to handles an IPC message asking us to launch a headless app at boot time
	 * 
	 * @see ProcessManager::launchBootTimeApp()
	 * 
	 * @todo Document this more fully once ProcessManager::launchBootTimeApp() is fully documented.
	 * @todo Add the missing "e" into the name so it matches ProcessManager.
	 * 
	 * @param	appDescString		App ID of the app to launch.
	 */
	void onProcMgrLaunchBootTimApp(const std::string& appDescString);
	void onListOfRunningAppsRequest(bool includeSysApps);
	void onDeleteHTML5Database(const std::string& domain);

	void inputEvent(int ipcKey, sptr<Event> e);

    void onUiDimensionsChanged(int width, int height);
    void onSuspendWebkitProcess(bool* result);

protected:

	virtual void threadStarting();
	virtual void threadStopping();

	virtual void handleEvent(sptr<Event>);

	virtual void windowedAppAdded(WindowedWebApp *app);
	virtual void windowedAppKeyChanged(WindowedWebApp *app, int oldKey);
	virtual void windowedAppRemoved(WindowedWebApp *app);

private:
	static gboolean BootupTimeoutCallback(gpointer);
	static gboolean BootupIdleCallback(gpointer);
	void bootFinished();

	static bool systemServiceConnectCallback(LSHandle *sh, LSMessage *message, void *ctx);
	static bool activityManagerCallback(LSHandle* sh, LSMessage* message, void* ctx);
	WebAppBase* launchUrlInternal(const std::string& url, Window::Type winType,
								  const std::string& appDesc, const std::string& procId,
								  const std::string& args, const std::string& launchingAppId,
								  const std::string& launchingProcId, int& errorCode, bool launchAsChild, bool ignoreLowMemory=false);
	WebAppBase* launchWithPageInternal(SysMgrWebBridge* page, Window::Type winType, ApplicationDescription* parentDesc);
	void closeAppInternal(WebAppBase* app);
	void setOrientationInternal(Event::Orientation orient);

	void restartHeadLessBootApps();

	void webPageAdded(SysMgrWebBridge* page);
	void webPageRemoved(SysMgrWebBridge* page);
	void shellPageAdded(SysMgrWebBridge* page);

    // IME support
    void launchIme();
	void launchImePopup(const std::string&);

	bool preventAppUnderLowMemory(const std::string& appId, Window::Type winType, ApplicationDescription* appDesc) const;

	static bool sysServicePrefsCallback(LSHandle *lshandle, LSMessage *message, void *ctx);
	static bool displayManagerConnectCallback(LSHandle* sh, LSMessage* message, void* ctx);
	static bool displayManagerCallback(LSHandle* sh, LSMessage* message, void* ctx);	

	void slotMemoryStateChanged(MemoryWatcher::MemState state);

	static gboolean deletePagesCallback(gpointer arg);
	void deletePages();

	void mimeHandoffUrl(const char* mimeType, const char* url, const char* callerId=0);

	WindowedWebApp* appForIpcKey(int key);
	SysMgrWebBridge* takeShellPageForApp(const std::string& appId);

	void addHeadlessAppToWatchList(WebAppBase* app);
	void removeHeadlessAppFromWatchList(WebAppBase* app);
	bool headlessAppWatchCallback();

	WebAppManager();

	virtual void serverConnected(PIpcChannel*);
	virtual void serverDisconnected();

	virtual void setAppWindowProperties(int appIpcKey, WindowProperties &winProp);

	void startGcPowerdActivity();
	void stopGcPowerdActivity();
	bool gcPowerdActivtyTimerCallback();
	bool isAppRunning(const std::string& appId);

    void appDeleted(WebAppBase* app);

	typedef std::pair<std::string, SysMgrWebBridge*> AppIdWebPagePair;
	typedef std::multimap<std::string, SysMgrWebBridge*> AppIdWebPageMap;
	typedef std::map<std::string, SysMgrWebBridge*> AppIdShellPageMap;
	typedef std::list<WebAppBase*> AppList;
	typedef std::list<SysMgrWebBridge*> PageList;
	typedef std::map<WebAppBase*, uint32_t> AppLaunchTimeMap;

	AppIdShellPageMap m_shellPageMap;
	AppList m_appList;
	AppIdWebPageMap m_appPageMap;
	AppLaunchTimeMap m_headlessAppLaunchTimeMap;

	Event::Orientation m_orientation;
	LSPalmService* m_service;
	LSHandle* m_servicePublic;
	LSHandle* m_servicePrivate;
	WebAppBase* m_imePopupApp;
	WebKitEventListener* m_wkEventListener;
	PageList m_pagesToDeleteList;
	bool m_deletingPages;
	Timer<WebAppManager> m_headlessAppWatchTimer;

	bool m_displayOn;
	std::string m_gcPowerdActivityId;
	Timer<WebAppManager> m_gcPowerdActivityTimer;

	typedef std::map<int, WindowedWebApp*> AppWindowMap;
	AppWindowMap m_appWinMap;

	HostInfo hostInfoCache;

	int m_uiWidth, m_uiHeight;
	bool m_deviceIsPortraitType;
	bool m_disableAppCaching;

	bool m_inSimulatedMouseEvent;

    QApplication *m_Application;

    std::string m_activeAppId;

	friend class SysMgrWebBridge;
	friend class SysMgrWebPage;
	friend class WebAppBase;
	friend class WindowedWebApp;
	friend class MemoryWatcher;
	friend class PalmSystem;
	friend class CardWebApp;
	friend class AlertWebApp;
	friend class DashboardWebApp;
	friend class ProcessManager;
};

#endif /* BROWSERAPPMANAGER_H */
