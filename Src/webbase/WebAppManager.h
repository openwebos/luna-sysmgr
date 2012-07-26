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

class WebAppManager : public SyncTask
					, public Trackable
					, public PIpcClient
					, public PIpcChannelListener
{
public:

	static WebAppManager* instance();
	static bool hasBeenInstanced();
	static const std::string getTimeZone();

	virtual ~WebAppManager();

	virtual void run();

	Event::Orientation orientation() const;

	std::list<const ProcessBase*>	runningApps() ;
	std::list<const ProcessBase*>	runningApps(Window::Type winType);
	WebAppBase* findApp(const QString& processId);
	WebAppBase* findAppById(const QString& appId);

	void initiateLunaStatsReporting();
	LSHandle* getStatsServiceHandle() const;

	void copiedToClipboard(const QString& appId);
	void pastedFromClipboard(const QString& appId);

	void closePageSoon(SysMgrWebBridge* page);

    /*!
        Request that the given page be closed.
    */
    void closePageRequest(SysMgrWebBridge* page);

	void onMessageReceived(const PIpcMessage& msg);
	void onDisconnected();

	void setJavascriptFlags( const std::string& flags );

	void sendAsyncMessage( PIpcMessage* msg);

	using TaskBase::mainLoop;

	void markUniversalSearchReady();

	void closeByProcessId( const std::string& processId );

	// Host Info data cache
	const void setHostInfo(const HostInfo *hostInfo);
	const HostInfo& getHostInfo() const;

	void reportAppLaunched(const QString& appId, const QString& processId);
	void reportAppClosed(const QString& appId, const QString& processId);

	int currentUiWidth() { return m_uiWidth; }
	int currentUiHeight() { return m_uiHeight; }
	bool isDevicePortraitType() { return m_deviceIsPortraitType; }

	void disableAppCaching(bool disable);

	static SharedGlobalProperties* globalProperties();

	void setInSimulatedMouseEvent(bool val) { m_inSimulatedMouseEvent = val; }
	bool inSimulatedMouseEvent() const { return m_inSimulatedMouseEvent; }
	void setActiveAppId(const std::string& id) { m_activeAppId = id; }
	const std::string& getActiveAppId() { return m_activeAppId; }
protected:
	// IPC control message handlers
	void onLaunchUrl(const std::string& url, int winType,
		 		     const std::string& appDesc, const std::string& procId,
					 const std::string& args, const std::string& launchingAppId,
					 const std::string& launchingProcId);
	void onLaunchUrlChild(const std::string& url, int winType,
			 		      const std::string& appDesc, const std::string& procId,
						  const std::string& args, const std::string& launchingAppId,
						  const std::string& launchingProcId, int& errorCode, bool isHeadless);
	void onRelaunchApp(const std::string& procId, const std::string& args,
			  		   const std::string& launchingAppId, const std::string& launchingProcId);

	//void onInputEvent(int ipcKey, const SysMgrEventWrapper& wrapper);
	void onSetOrientation(int orient);
	void onGlobalProperties(int key);
	void onInspectByProcessId( const std::string& processId );
	void performLowMemoryActions( const bool allowExpensive = false );
	void monitorNativeProcessMemory( const pid_t pid, const int maxMemory, pid_t updateFromPid = 0 );
	void clearWebkitCache();
	void enableDebugger( bool enable );
	void onShutdownEvent();
	bool onKillApp(const std::string& appId);
	void onSyncKillApp(const std::string& appId, bool* result);
	void onProcMgrLaunch(const std::string& appDescString, const std::string& params,
                         const std::string& launchingAppId, const std::string& launchingProcId);
	void onProcMgrLaunchChild(const std::string& appDescString, const std::string& params,
	                         const std::string& launchingAppId, const std::string& launchingProcId, bool isHeadless, bool isParentPdk);
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
