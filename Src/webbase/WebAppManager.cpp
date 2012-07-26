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

#include <string>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <PIpcChannel.h>
#include <PIpcClient.h>
#include <PIpcBuffer.h>

#include "WebAppManager.h"
#include "SystemUiController.h"
#include "ApplicationDescription.h"
#include "CardWebApp.h"
#include "ProcessManager.h"
#include "Localization.h"
#include "Logging.h"
#include "JSONUtils.h"
#include "MemoryWatcher.h"
#include "BannerMessageEventFactory.h"
#include "Settings.h"
#include "WebAppBase.h"
#include "WebAppFactory.h"
#include "WindowedWebApp.h"
#include "Preferences.h"
#include "EventReporter.h"
#include <WebKitEventListener.h>
#include <BackupManager.h>
#include "Utils.h"
#include "Time.h"
#include "ApplicationInstaller.h"
#include "SharedGlobalProperties.h"

#include <algorithm>
#include "cjson/json.h"
#include "lunaservice.h"

#include <QDebug>

#include <nyx/nyx_client.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#ifdef USE_HEAP_PROFILER
#include <google/heap-profiler.h>
#endif

#include "SysMgrWebBridge.h"

// NOTE: the launcher will key and read data with this value. If you change this,
// you will also need to update launcher.
static const char* kReorderClipboardDataKey = "LAUNCHPOINTID";

static const int inputEventType = (Event::User + 1);
static const int pageCloseRequestEventType = (Event::User + 2);

static GSource* s_bootupIdleSrc = 0;
static GSource* s_bootupTimeoutSrc = 0;
enum BootState {
	BootStateUninitialized,
	BootStateWaitingForIdle,
	BootStateWaitingForUniversalSearch,
	BootStateFinished
};

static bool s_universalSearchReady = false;

static BootState s_bootState = BootStateUninitialized;
static const uint32_t s_bootTimeout = 60;
static PIpcChannel *s_ipcChannel = 0;
static PIpcBuffer* s_globalPropsBuffer = 0;

static std::string s_lastCopyClipboardMessageId = std::string();
static std::string s_timeZone = std::string();
static const uint32_t kHeadlessAppAllowedTimeForSoloRunMs = 60000;
static const uint32_t kHeadlessAppWatchTimeoutMs = kHeadlessAppAllowedTimeForSoloRunMs / 5 + 100;

static const int kGcPowerdActivityDuration = 10000;
static const int kGcStartTimeout = 2000;
static const unsigned kGcCpuIdleThreshold = 800; // percentage times 1000
static const unsigned kNumTimesIgnoreGc = 5;

class InputEvent : public Event
{
public:

	InputEvent(int ipcKey, sptr<Event> e)
		: m_event(e) {
		m_ipcKey = ipcKey;
		type = (Event::Type) inputEventType;
	}

	int m_ipcKey;
	sptr<Event> m_event;
};

/*!
 *  Event for indicating the manager that the given page should be closed. 
 *  An example usage of this is the 'window.close()' from javascript.
 *  See also, WindowedWebApp::closeWindowRequest
 */
class PageCloseRequestEvent : public Event {
public:

    PageCloseRequestEvent(SysMgrWebBridge* page) : m_page(page)
    {
        type = (Event::Type) pageCloseRequestEventType;
    }

    SysMgrWebBridge* m_page;
};


static WebAppManager* sInstance = 0;

static const int kLunaStatsReportingIntervalSecs = 5;

static bool PrvGetMemoryStatus(LSHandle* handle, LSMessage* message, void* ctxt);
static bool PrvGetLunaStats(LSHandle* handle, LSMessage* message, void* ctxt);
static bool PrvDoGc(LSHandle* handle, LSMessage* message, void* ctxt);
static gboolean PrvPostLunaStats(gpointer ctxt);
static bool PrvGetSystemTimeCallback(LSHandle* handle, LSMessage* message, void* ctxt);
static bool PrvGetAppEvents(LSHandle* handle, LSMessage* message, void* ctxt);
static bool EnablePiranhaFpsCounter(LSHandle* lsHandle, LSMessage *message, void *user_data);
static bool PrvDumpRenderTree(LSHandle* lsHandle, LSMessage *message, void *user_data);
static bool PrvDumpCompositedTree(LSHandle* lsHandle, LSMessage *message, void *user_data);

#ifdef USE_HEAP_PROFILER
static bool PrvDumpHeapProfiler(LSHandle* lsHandle, LSMessage *message, void *user_data);
#endif

/*
static bool PrvEnableAccelCompositing(Window::Type winType, const std::string& appId)
{
	return ((winType == Window::Type_Card ||
			winType == Window::Type_ChildCard ||
			winType == Window::Type_PIN ||
			winType == Window::Type_ModalChildWindowCard ||
			winType == Window::Type_Emergency) &&
			!Settings::LunaSettings()->appsToDisableAccelCompositing.count(appId));
}
*/

static LSMethod sStatsMethodsPublic[] = {
	{ "getMemoryStatus", PrvGetMemoryStatus },
	{ NULL,       NULL},
};

static LSMethod sStatsMethodsPrivate[] = {
	{ "getStats", PrvGetLunaStats },
	{ "gc", PrvDoGc },
	{ "getAppEvents", PrvGetAppEvents },
	{ "enablePiranhaFpsCounter", EnablePiranhaFpsCounter},
	{ "dumpRenderTree", PrvDumpRenderTree },
	{ "dumpCompositedTree", PrvDumpCompositedTree },
#ifdef USE_HEAP_PROFILER
	{ "dumpHeapProfile", PrvDumpHeapProfiler },
#endif
	{ NULL,       NULL},
};


WebAppManager* WebAppManager::instance()
{
	if (G_UNLIKELY(!sInstance)) {
		new WebAppManager;
	}

	return sInstance;
}

bool WebAppManager::hasBeenInstanced()
{
	return (sInstance != 0);
}

WebAppManager::WebAppManager()
	: SyncTask(g_main_context_default())
	, PIpcClient( "sysmgr", WEB_APP_MGR_IPC_NAME, TaskBase::m_mainLoop)
	, m_wkEventListener(NULL)
	, m_headlessAppWatchTimer(masterTimer(), this, &WebAppManager::headlessAppWatchCallback)
	, m_gcPowerdActivityTimer(masterTimer(), this, &WebAppManager::gcPowerdActivtyTimerCallback)
	, m_displayOn(true)
	, m_disableAppCaching(false)
	, m_inSimulatedMouseEvent(false)
{
    setenv("QT_PLUGIN_PATH", "/usr/plugins", 1);
    setenv("QT_QPA_PLATFORM", "webos", 1);
    setenv("QT_DEBUG_PLUGINS", "1", 1);

    static const char *argv[] = { "./WebAppManager", "-platform", "webos", NULL };

//    static const char *argv[] = { "./WebAppManager", "-platform", "minimal", NULL };

    static int argc = 3;

    m_Application = new QApplication(argc, (char **)argv);
	sInstance = this;
	m_orientation = Event::Orientation_Up;
	m_deletingPages = false;
	m_imePopupApp = 0;
	m_uiWidth  = 0;
	m_uiHeight = 0;
	m_deviceIsPortraitType = true;

    QWebSettings::globalSettings()->setFontFamily(QWebSettings::StandardFont, "Prelude");
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::SerifFont, "Times New Roman");
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::SansSerifFont, "Prelude");
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::FixedFont, "Courier New");
}

WebAppManager::~WebAppManager()
{
	delete m_wkEventListener;
	sInstance = false;
    delete m_Application;
}

void WebAppManager::run()
{
	threadStarting();

	// needs to be initialized once per process
	EventReporter::init(mainLoop());

	markUniversalSearchReady();

	nyx_init();
	
	//g_main_loop_run(mainLoop());
	m_Application->exec();

    nyx_deinit();
    
	threadStopping();

	threadCleanup();

}


void WebAppManager::serverConnected(PIpcChannel* channel)
{
	channel->setListener(this);
}

void WebAppManager::serverDisconnected()
{
	g_critical("%s (%d). Remote server disconnected. Exiting...\n",
	           __PRETTY_FUNCTION__, __LINE__);
	exit(-1);
}

void WebAppManager::windowedAppAdded(WindowedWebApp *app)
{
	if (app && app->getKey()) {
		m_appWinMap[app->getKey()] = app;
	} else {
		g_critical("%s (%d). Failed to add app to the AppWin map.\n",
		           __PRETTY_FUNCTION__, __LINE__);
	}
}

void WebAppManager::windowedAppKeyChanged(WindowedWebApp *app, int oldKey)
{
	if (app && app->getKey()) {
		m_appWinMap.erase(oldKey);
		m_appWinMap[app->getKey()] = app;
	} else {
		g_critical("%s (%d). Failed to add app to the AppWin map.\n",
		           __PRETTY_FUNCTION__, __LINE__);
	}
}

void WebAppManager::windowedAppRemoved(WindowedWebApp *app)
{
	if (app && app->getKey()) {
		m_appWinMap.erase(app->getKey());
	} else {
		g_critical("%s (%d). Failed to remove app from the AppWin map.\n",
		           __PRETTY_FUNCTION__, __LINE__);
	}
}


void WebAppManager::onMessageReceived(const PIpcMessage& msg)
{
	if (msg.routing_id() != MSG_ROUTING_CONTROL) {
		// ROUTED message, forward it to the correct WebApp
		WindowedWebApp *winApp = appForIpcKey(msg.routing_id());
		if (!winApp) {
			g_critical("%s (%d). Failed to find app with key: %d  msgType = 0x%x\n",
			           __PRETTY_FUNCTION__, __LINE__, msg.routing_id(), msg.type());
			return;
		} else {
			winApp->onMessageReceived(msg);
		}
	} else {
			// CONTROL MESSAGE, Handle it here
			bool msgIsOk;

			IPC_BEGIN_MESSAGE_MAP(WebAppManager, msg, msgIsOk)
				IPC_MESSAGE_HANDLER(View_Mgr_LaunchUrl, onLaunchUrl)
				IPC_MESSAGE_HANDLER(View_Mgr_GlobalProperties, onGlobalProperties)
				IPC_MESSAGE_HANDLER(View_Mgr_SetOrientation, onSetOrientation)
				IPC_MESSAGE_HANDLER(View_Mgr_InspectByProcessId, onInspectByProcessId)
				IPC_MESSAGE_HANDLER(View_Mgr_ClearWebkitCache, clearWebkitCache)
				IPC_MESSAGE_HANDLER(View_Mgr_SetJavascriptFlags, setJavascriptFlags)
				IPC_MESSAGE_HANDLER(View_Mgr_EnableDebugger, enableDebugger)
				IPC_MESSAGE_HANDLER(View_Mgr_ShutdownEvent, onShutdownEvent)
				IPC_MESSAGE_HANDLER(View_Mgr_CloseByProcessId, closeByProcessId)
				IPC_MESSAGE_HANDLER(View_Mgr_KillApp, onKillApp)
				IPC_MESSAGE_HANDLER(View_Mgr_SyncKillApp, onSyncKillApp)
				IPC_MESSAGE_HANDLER(View_ProcMgr_Launch, onProcMgrLaunch)
				IPC_MESSAGE_HANDLER(View_ProcMgr_LaunchChild, onProcMgrLaunchChild)
				IPC_MESSAGE_HANDLER(View_ProcMgr_LaunchBootTimeApp, onProcMgrLaunchBootTimApp)
				IPC_MESSAGE_HANDLER(View_Mgr_ListOfRunningAppsRequest, onListOfRunningAppsRequest)
				IPC_MESSAGE_HANDLER(View_Mgr_DeleteHTML5Database, onDeleteHTML5Database)
				IPC_MESSAGE_HANDLER(View_Mgr_PerformLowMemoryActions, performLowMemoryActions)
				IPC_MESSAGE_HANDLER(View_Mgr_UiDimensionsChanged, onUiDimensionsChanged)
				IPC_MESSAGE_HANDLER(View_Mgr_SuspendWebkitProcess, onSuspendWebkitProcess)
				IPC_MESSAGE_UNHANDLED( g_critical("%s (%d). IPC Message Not Handled: (0x%x : 0x%x : 0x%x)",
				                                  __PRETTY_FUNCTION__, __LINE__, msg.routing_id(), msg.type(), msg.message_id()));
			IPC_END_MESSAGE_MAP()
	}
}

void WebAppManager::onDisconnected()
{
	g_critical("%s (%d). Remote server disconnected. Exiting...",
	           __PRETTY_FUNCTION__, __LINE__);
	exit(-1);
}
/* $$$ ## $$$ - Temporarily disabled code to coaelesce events
void WebAppManager::onInputEvent(int ipcKey, const SysMgrEventWrapper& wrapper)
{
	Event *evt = new Event;
	memcpy(&(evt->type), &(wrapper.event->type), sizeof(SysMgrEvent));

	sptr<Event> e = evt;

	// Coaelesce input events
	if (e->type == Event::GestureChange) {


		std::list<sptr<Event> >::reverse_iterator rfirst(m_eventsList.end());
		std::list<sptr<Event> >::reverse_iterator rlast(m_eventsList.begin());

		bool coalesced = false;
		while (rfirst != rlast) {
			sptr<Event> evt = *rfirst;
			if (evt->type == Event::GestureStart) {
				// cannot coalesce
				break;
			}
			else if (e->type == Event::GestureChange) {
				evt->time = e->time;
				evt->x = e->x;
				evt->y = e->y;
				evt->gestureCenterX = e->gestureCenterX;
				evt->gestureCenterY = e->gestureCenterY;
				evt->gestureScale = e->gestureScale;
				evt->gestureRotate = e->gestureRotate;
				coalesced = true;
				break;
			}

			rfirst++;
		}

		if (coalesced) {
			return;
		}
	}

	WindowedWebApp *winApp = appForIpcKey(ipcKey);
	if (!winApp) {
		g_critical("%s (%d). Failed to find app with key: %d\n",
			 __PRETTY_FUNCTION__, __LINE__, ipcKey);
		return;
	} else {
		winApp->inputEvent(e);
	}
}
*/

const std::string WebAppManager::getTimeZone()
{
	return s_timeZone;
}

void WebAppManager::onLaunchUrl(const std::string& url, int winType,
                                const std::string& appDesc, const std::string& procId,
                                const std::string& args, const std::string& launchingAppId,
                                const std::string& launchingProcId)
{
	int errorCode = 0;
	launchUrlInternal(url, static_cast<Window::Type>(winType), appDesc, procId,
	                  args, launchingAppId, launchingProcId, errorCode, false);
}

void WebAppManager::onLaunchUrlChild(const std::string& url, int winType,
                                	 const std::string& appDesc, const std::string& procId,
                                	 const std::string& args, const std::string& launchingAppId,
                                	 const std::string& launchingProcId, int& errorCode, bool isHeadless)
{
	launchUrlInternal(url, static_cast<Window::Type>(winType), appDesc, procId,
	                  args, launchingAppId, launchingProcId, errorCode, true);
}

void WebAppManager::onRelaunchApp(const std::string& procId, const std::string& args,
                                  const std::string& launchingAppId, const std::string& launchingProcId)//, bool launchAsChild)
{
	WebAppBase* app = findApp(QString::fromStdString(procId));
	if (app) {
		if (app->page()->processId() == QString::fromStdString(procId)) {
			app->relaunch(args.c_str(), launchingAppId.c_str(), launchingProcId.c_str());//, launchAsChild);
		}
	}
}

void WebAppManager::onSetOrientation(int orient)
{
	setOrientationInternal(static_cast<Event::Orientation>(orient));
}

void WebAppManager::onGlobalProperties(int key)
{
    if (G_UNLIKELY(s_globalPropsBuffer))
		delete s_globalPropsBuffer;

	s_globalPropsBuffer = PIpcBuffer::attach(key, sizeof(SharedGlobalProperties));
	g_debug("WebAppManager::onGlobalProperties: %d", key);
}


void WebAppManager::onInspectByProcessId( const std::string& processId )
{
	WebAppBase* app = findApp(QString::fromStdString(processId));
	if (app)
		app->page()->inspect();
}

void WebAppManager::clearWebkitCache()
{
//	Palm::WebGlobal::clearDOMCache();
	g_debug("Webkit DOM Cache cleared" );
}

void WebAppManager::setJavascriptFlags( const std::string& flags )
{
	if (flags == "--timeout_script_execution") {
//		Palm::WebGlobal::enableJavaScriptTimeout(true);
	}
	else if (flags == "--notimeout_script_execution") {
//		Palm::WebGlobal::enableJavaScriptTimeout(false);
	}
	else {
//		Palm::WebGlobal::setJavaScriptFlags( flags.c_str() );
	}
}

void WebAppManager::enableDebugger( bool enable )
{
//	Palm::WebGlobal::enableDebugger(enable);
}

void WebAppManager::onShutdownEvent()
{
#if defined(TARGET_DESKTOP)

		for (AppList::const_iterator it = m_appList.begin();
			 it != m_appList.end(); ++it) {
			delete (*it);
		}

//		Palm::WebGlobal::garbageCollectNow();
#endif
		g_main_loop_quit(mainLoop());
		return;
}

bool WebAppManager::onKillApp(const std::string& appId)
{
	std::vector<std::string> pidList;

	bool bInstances = ProcessManager::instance()->getProcIdsOfApp(appId, pidList);
	if (bInstances) {
		//kill off all instances
		for (std::vector<std::string>::iterator pid_it = pidList.begin();pid_it != pidList.end();pid_it++) {
			g_message("Stopping running process %s",(*pid_it).c_str());
			closeByProcessId((*pid_it).c_str());
		}
	}

	return bInstances;
}

void WebAppManager::onSyncKillApp(const std::string& appId, bool* result)
{	
    *result = onKillApp(appId);
}

void WebAppManager::onProcMgrLaunch(const std::string& appDescString, const std::string& params,
                                    const std::string& launchingAppId, const std::string& launchingProcId)
{
	std::string errMsg, procId;

	// FIXME: $$$ find a way to return the process ID to the MainUI process
	procId = ProcessManager::instance()->launch(appDescString, params, launchingAppId, launchingProcId, errMsg);

	if(procId.empty()){
		g_critical("%s (%d). Failed to launch App: error = %s, appDesc = %s\n",
		           __PRETTY_FUNCTION__, __LINE__, errMsg.c_str(), appDescString.c_str());
	}
}

bool WebAppManager::isAppRunning(const std::string& appId)
{
	std::list<const ProcessBase*> apps = runningApps();
	std::list<const ProcessBase*>::iterator itStart = apps.begin();
	std::list<const ProcessBase*>::iterator itEnd = apps.end();

	for(; itStart != itEnd; itStart++) {
		if((*itStart)->appId() == QString::fromStdString(appId)) {
			return true;
		}
	}

	return false;
}

void WebAppManager::onProcMgrLaunchChild(const std::string& appDescString, const std::string& params,
                                    	 const std::string& launchingAppId, const std::string& launchingProcId, bool isHeadless, bool isParentPdk)
{
	// Run a quick check to see if the app to be launched is already running.
	ApplicationDescription* desc = ApplicationDescription::fromJsonString(appDescString.c_str());

	if(!desc) {
		sendAsyncMessage(new ViewHost_ModalDismissedAtPreCreate(SystemUiController::MissingAppDescriptor));
		return;
	}

	if(desc) {
		if(isAppRunning(desc->id())) {
			sendAsyncMessage(new ViewHost_ModalDismissedAtPreCreate(SystemUiController::AnotherInstanceAlreadyRunning));
			delete desc;
			return;
		}
	}

	delete desc;
	desc = 0;

	int errorCode = 0;
	std::string errMsg, procId;
	procId = ProcessManager::instance()->launchModal(appDescString, params, launchingAppId, launchingProcId, errMsg, errorCode, isHeadless, isParentPdk);

	if(procId.empty()){
		sendAsyncMessage(new ViewHost_ModalDismissedAtPreCreate((SystemUiController::ModalWinLaunchErrorReason)errorCode));
		g_critical("%s (%d). Failed to launch App: error = %s, appDesc = %s\n",
		           __PRETTY_FUNCTION__, __LINE__, errMsg.c_str(), appDescString.c_str());
	}
}

void WebAppManager::onProcMgrLaunchBootTimApp(const std::string& appDescString)
{
	std::string errMsg, procId;

	// FIXME: $$$ find a way to return the process ID to the MainUI process
	procId = ProcessManager::instance()->launchBootTimeApp(appDescString);

	if(procId.empty()){
		g_critical("%s (%d). Failed to launch App: error = %s, appDesc = %s\n",
		           __PRETTY_FUNCTION__, __LINE__, errMsg.c_str(), appDescString.c_str());
	}
}

void WebAppManager::onListOfRunningAppsRequest(bool includeSysApps)
{
	json_object* json = 0;
	json_object* array = 0;

	std::vector<ProcessInfo> apps = ProcessManager::instance()->list( includeSysApps );

	json = json_object_new_object();
	if (!json)
		return;
	array = json_object_new_array();
	if (!array) {
		json_object_put(json);
		return;
	}

	for ( std::vector<ProcessInfo>::const_iterator it=apps.begin(); it != apps.end(); it++ )
	{
		json_object* o = json_object_new_object();
		if (!o)
			continue;
		json_object_object_add(o, "id", json_object_new_string(it->appId.toUtf8().constData()));
		json_object_object_add(o, "processid", json_object_new_string(it->processId.toUtf8().constData()));

		json_object_array_add(array, o);
	}
	json_object_object_add(json, "running", array);
	json_object_object_add(json, "returnValue", json_object_new_boolean(true));

	sendAsyncMessage(new ViewHost_ListOfRunningAppsResponse(json_object_to_json_string(json)));

	json_object_put(json);
}

void WebAppManager::onDeleteHTML5Database(const std::string& domain)
{
	g_message("%s: %s", __PRETTY_FUNCTION__, (domain.empty() ? "" : domain.c_str()));
//	Palm::WebGlobal::deleteDatabasesForDomain(domain.c_str());
}

void WebAppManager::inputEvent(int ipcKey, sptr<Event> e)
{
	sptr<Event> evt = new InputEvent(ipcKey, e);
	postEvent(evt);
}

void WebAppManager::closeByProcessId(const std::string& processId)
{
	WebAppBase* app = findApp(QString::fromStdString(processId));
	if (app) {
		app->setKeepAlive(false);
		closeAppInternal(app);
	}
}

void WebAppManager::performLowMemoryActions( const bool allowExpensive )
{
	MemoryWatcher::instance()->doLowMemActions(allowExpensive);

	// request low memory actions from the SysMgr main process
	sendAsyncMessage(new ViewHost_LowMemoryActionsRequested(allowExpensive));
}

void WebAppManager::sendAsyncMessage( PIpcMessage* msg)
{
	m_channel->sendAsyncMessage(msg);
}

void WebAppManager::closePageRequest(SysMgrWebBridge* page)
{
    sptr<Event> closeEvent = new PageCloseRequestEvent(page);
    postEvent(closeEvent);
}

void WebAppManager::handleEvent(sptr<Event> e)
{
	switch ((int)e->type) {
	case (inputEventType): { // mouse, keyboard events
		InputEvent* evt = static_cast<InputEvent*>(e.get());

		// Note : see JsSysObject.cpp:SimulateMouseClick for details on this.
		// Make sure the window is still here -- the PalmSystem object
		// can inject mouse up/down clicks into the queue.
		WindowedWebApp* app = appForIpcKey(evt->m_ipcKey);
		if (app && app->isWindowed())
			app->inputEvent(evt->m_event);
		break;
	}
    case pageCloseRequestEventType:
        {
        PageCloseRequestEvent* closeEvent = static_cast<PageCloseRequestEvent*>(e.get());
        closeAppInternal(closeEvent->m_page->getClient());
        break;
        }
	default:
		break;
	}
}

std::list<const ProcessBase*> WebAppManager::runningApps()
{
	std::list<const ProcessBase*> apps;

	for (AppList::const_iterator it = m_appList.begin();
	     it != m_appList.end(); ++it) {

		apps.push_back(static_cast<const ProcessBase*>((*it)->page()));
	}

	return apps;
}

std::list<const ProcessBase*> WebAppManager::runningApps(Window::Type winType)
{
	std::list<const ProcessBase*> apps;

	for (AppList::const_iterator it = m_appList.begin();
	     it != m_appList.end(); ++it) {

		WebAppBase* app = (*it);

		switch (winType) {
		case (Window::Type_None): {

			if (!app->isWindowed()) {
				apps.push_back(static_cast<const ProcessBase*>(app->page()));
			}

			break;
		}
		default: {

			if (app->isWindowed() &&
				static_cast<WindowedWebApp*>(app)->windowType() == winType) {

				apps.push_back(static_cast<const ProcessBase*>(app->page()));
			}

			break;
		}
		}
	}

	return apps;
}

void WebAppManager::threadStarting()
{
	static bool initialized = false;

	if (!initialized) {
		setpriority(PRIO_PROCESS,getpid(),-1);

		const char* const k_pszPlatformConfigFile = "/etc/palm/browser-platform.conf";
		
//		::PalmBrowserSettings("/etc/palm/browser.conf");
//		if (0 == ::access(k_pszPlatformConfigFile, R_OK))
//		  ::PalmBrowserSettings(k_pszPlatformConfigFile);
		
		if (BackupManager::instance()->init(mainLoop())) {
			sInstance->m_wkEventListener = new WebKitEventListener(BackupManager::instance());
		}
		else {
			g_critical("Unable to initialize backup manager.");
		}
//		Palm::WebGlobal::init(mainLoop(), sInstance->m_wkEventListener);

		// Disable oldgen GCs for 2 mins while booting.  If we finish booting
		// before the 2 mins is up, we'll disable it.
//		Palm::WebGlobal::avoidGC(Palm::WebGlobal::AVOID_OLDGEN, 120000);

		// Disable script timeouts until we have booted.
//		Palm::WebGlobal::enableJavaScriptTimeout(false);

		// FIXME : can locale change without restarting sysmgr?
//		Palm::WebGlobal::setLocale( Preferences::instance()->locale().c_str() );
		initialized = true;

		// start watching memory usage
		MemoryWatcher::instance();

		MemoryWatcher::instance()->signalMemoryStateChanged.
			connect(this, &WebAppManager::slotMemoryStateChanged);

		// Set the extra HTTP header from the carrier
		{
			LSError lserror;
			LSErrorInit(&lserror);

			if (LSRegisterPalmService("com.palm.lunastats", &m_service, &lserror) ) {

				bool r;

				r = LSPalmServiceRegisterCategory(m_service, "/",
												  sStatsMethodsPublic,
												  sStatsMethodsPrivate,
												  NULL, NULL, &lserror);
				if (!r)
					goto Error;

				m_servicePublic = LSPalmServiceGetPrivateConnection(m_service);
				m_servicePrivate = LSPalmServiceGetPrivateConnection(m_service);
				

				r = LSGmainAttachPalmService(m_service, mainLoop(), &lserror);
				if (!r)
					goto Error;

				r = LSCall(m_servicePrivate, "palm://com.palm.lunabus/signal/registerServerStatus",
						   "{\"serviceName\":\"com.palm.systemservice\"}",
						   systemServiceConnectCallback, NULL, NULL, &lserror);
				if (!r)
					goto Error;

				r = LSCall(m_servicePrivate, "palm://com.palm.bus/signal/registerServerStatus",
						   "{\"serviceName\":\"com.palm.display\"}",
						   displayManagerConnectCallback, m_servicePrivate, NULL, &lserror);
				if (!r)
					goto Error;
				
			Error:

				if (!r) {
					LSErrorPrint(&lserror, stderr);
					LSErrorFree(&lserror);
				}
			}

		}
	}

}

template <class T>
bool ValidJsonObject(T jsonObj)
{
	return NULL != jsonObj && !is_error(jsonObj);
}

bool WebAppManager::sysServicePrefsCallback(LSHandle *lshandle, LSMessage *message, void *ctx)
{
/*
	LSError lserror;
	LSErrorInit(&lserror);
	std::string result;
	struct json_object* root = 0;
	struct json_object* carrier = 0;
	struct json_object* textInput = 0;

    // {"x_palm_carrier": string, "x_palm_textinput": { "spellChecking": string, "grammarChecking": string, "shortcutChecking": string}}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_2(REQUIRED(x_palm_carrier, string), NAKED_OBJECT_REQUIRED_3(x_palm_textinput, spellChecking, string, grammarChecking, string, shortcutChecking, string)));

	const char* str = LSMessageGetPayload( message );
	if( !str )
		goto done;

	root = json_tokener_parse( str );
	if( is_error(root) )
	{
		result = "{\"err\":\"malformed JSON\"}";
		goto done;
	}

	carrier = json_object_object_get(root,"x_palm_carrier");
	if( carrier )
	{
		char* carrierCode = json_object_get_string( carrier );
//		Palm::WebGlobal::addAppendedHTTPHeader( "X-Palm-Carrier", carrierCode );
	}
	else {
		g_debug("NO PALM CARRIER in PREFS DB" );
	}

	textInput = json_object_object_get(root, "x_palm_textinput");
	if (ValidJsonObject(textInput)) {

		std::string strProp;

		json_object* prop = json_object_object_get(textInput, "spellChecking");
		if (ValidJsonObject(prop)) {

			strProp = json_object_get_string(prop);
			if (strProp == "disabled")
				PalmBrowserSettings()->checkSpelling = WebKitPalmSettings::DISABLED;
			else if (strProp == "autoCorrect")
				PalmBrowserSettings()->checkSpelling = WebKitPalmSettings::AUTO_CORRECT;
			else if (strProp == "underline")
				PalmBrowserSettings()->checkSpelling = WebKitPalmSettings::UNDERLINE;
			else {
				g_warning("Unknown spellChecking flag: '%s'", strProp.c_str());
			}
		}

		prop = json_object_object_get(textInput, "grammarChecking");
		if (ValidJsonObject(prop)) {

			strProp = json_object_get_string(prop);
			PalmBrowserSettings()->checkGrammar = strProp == "autoCorrect";
		}

		prop = json_object_object_get(textInput, "shortcutChecking");
		if (ValidJsonObject(prop)) {

			strProp = json_object_get_string(prop);
			PalmBrowserSettings()->shortcutChecking = strProp == "autoCorrect";
		}
	}
	else {
		g_warning("Oops, no x_palm_textinput preference");
	}

done:

	if( root && !is_error(root) ) json_object_put( root );
*/
	return true;
}

void WebAppManager::threadStopping()
{

}

WebAppBase* WebAppManager::launchUrlInternal(const std::string& url, Window::Type winType,
                                             const std::string& appDesc, const std::string& _procId,
                                             const std::string& args, const std::string& launchingAppId,
                                             const std::string& launchingProcId, int& errorCode, bool launchAsChild,
                                             bool ignoreLowMemory)
{
	if (G_UNLIKELY(s_bootState == BootStateUninitialized)) {

		if (!s_bootupIdleSrc) {
			s_bootupIdleSrc = g_idle_source_new();
			g_source_set_priority(s_bootupIdleSrc, G_PRIORITY_LOW);
			g_source_set_callback(s_bootupIdleSrc, WebAppManager::BootupIdleCallback, NULL, NULL);
			g_source_attach(s_bootupIdleSrc, g_main_loop_get_context(mainLoop()));
			s_ipcChannel = m_channel;
			s_bootState = BootStateWaitingForIdle;
		}
	}

	// decode the App description and extract the appId
	ApplicationDescription* desc = ApplicationDescription::fromJsonString(appDesc.c_str());
	if (!desc) {
		errorCode = SystemUiController::MissingAppDescriptor;
		g_critical("%s (%d). Failed to decode App Description from JSON string.\n",
		           __PRETTY_FUNCTION__, __LINE__);
		return 0;
	}

	QString appId = QString::fromStdString(desc->id());

	// Launch events for an app may have been queued up. we want to make
	// sure that an app is not launched multiple times

	for (AppList::const_iterator it = m_appList.begin();
	     it != m_appList.end(); ++it) {
//        SysMgrWebBridge* page = (*it)->page();
		if (appId == static_cast<const ProcessBase*>((*it)->page())->appId()) {
			delete desc;
			return 0;
		}
	}


	// Low Memory handling
	if (!ignoreLowMemory && preventAppUnderLowMemory(appId.toStdString(), winType, desc)) {
		errorCode = SystemUiController::InternalError;
		g_warning("%s: Low memory condition Not allowing card app for appId: %s", __PRETTY_FUNCTION__, appId.toUtf8().constData());
		// not enough memory. try to free up some memory and notify the user to close cards
		sendAsyncMessage(new ViewHost_AppLaunchPreventedUnderLowMemory());
		performLowMemoryActions();
		delete desc;
		return 0;
	}

	// -----------------------------------------------------------------------------------------

	std::string procId = _procId;

	if (!procId.size())
		procId = ProcessManager::instance()->processIdFactory();

	WebAppBase* app = WebAppFactory::instance()->createWebApp(winType, m_channel, desc);

	if (winType == Window::Type_None)
		addHeadlessAppToWatchList(app);

	if (G_UNLIKELY(Settings::LunaSettings()->perfTesting)) {
		g_message("SYSMGR PERF: APP START appid: %s, processid: %s, type: %s, time: %d",
				  appId.toUtf8().constData(), procId.c_str(),
				  WebAppFactory::nameForWindowType(winType).toUtf8().constData(),
				  Time::curTimeMs());
	}

	if (app) {
		SysMgrWebBridge* page = new SysMgrWebBridge(winType != Window::Type_None, QUrl(url.c_str()));
		app->setAppDescription(desc);

		static_cast<ProcessBase*>(page)->setProcessId(QString::fromStdString(procId));
		static_cast<ProcessBase*>(page)->setAppId(appId);
		static_cast<ProcessBase*>(page)->setLaunchingAppId(QString::fromStdString(launchingAppId));
		static_cast<ProcessBase*>(page)->setLaunchingProcessId(QString::fromStdString(launchingProcId));

		page->setArgs(args.c_str());

		app->attach(page);

        page->load();

		webPageAdded(page);

		m_appList.push_back(app);
		if (app->isWindowed()) {
			WindowedWebApp* winApp = static_cast<WindowedWebApp*>(app);
			windowedAppAdded(winApp);
			if (!m_displayOn)
				winApp->displayOff();

//			if (page->webkitView() && !PrvEnableAccelCompositing(winType, winApp->appId()))
//				page->webkitView()->setSupportsAcceleratedCompositing(false);
		}

		// pre-create a card if we are launching a headless app for the first time
		// and the launch is coming from an app with a process id
		if(false == launchAsChild) {
			if (winType == Window::Type_None && (launchingAppId.size() > 0 && launchingProcId.size() > 0)) {
				page->createViewForWindowlessPage();
			}
		}
	}
	else {
		delete desc;
	}

	return app;
}

WebAppBase* WebAppManager::launchWithPageInternal(SysMgrWebBridge* page, Window::Type winType, ApplicationDescription* parentDesc)
{
	if (preventAppUnderLowMemory(page->appId().toStdString(), winType, parentDesc)) {
		g_warning("%s: Low memory condition Not allowing card app for appId: %s", __PRETTY_FUNCTION__, page->appId().toUtf8().constData());
		// not enough memory. try to free up some memory and notify the user to close cards
		sendAsyncMessage(new ViewHost_AppLaunchPreventedUnderLowMemory());
		WebAppManager::instance()->performLowMemoryActions();
		return 0;
	}

	WebAppBase* app = WebAppFactory::instance()->createWebApp(winType, page, m_channel, parentDesc);

	if (app) {
		if (parentDesc) {
			std::string appDescString;
			parentDesc->getAppDescriptionString(appDescString);
			ApplicationDescription* appDesc =  ApplicationDescription::fromJsonString(appDescString.c_str());
			app->setAppDescription(appDesc);
		}					   

		std::string processId = ProcessManager::instance()->processIdFactory();
		static_cast<ProcessBase*>(page)->setProcessId(QString::fromStdString(processId));

		app->attach(page);

		m_appList.push_back(app);

		if (app->isWindowed()) {
			WindowedWebApp* winApp = static_cast<WindowedWebApp*>(app);
			windowedAppAdded(winApp);
			if (!m_displayOn)
				winApp->displayOff();

//			if (page->webkitView() && !PrvEnableAccelCompositing(winType, winApp->appId()))
//				page->webkitView()->setSupportsAcceleratedCompositing(false);
		}
	}
	return app;
}

void WebAppManager::closeAppInternal(WebAppBase* app)
{
	QString appId;
	if (app->page())
		appId = app->page()->appId();

	// Should cache this page?

	const std::set<std::string>& appsToKeepAlive = Settings::LunaSettings()->appsToKeepAlive;

	bool cached = false;
	
	if( ( app->isWindowed() && Window::Type_Card == static_cast<WindowedWebApp*>(app)->windowType() )
		&& appsToKeepAlive.find(appId.toStdString()) != appsToKeepAlive.end()
		&& app->keepAlive()
		// Do not cache if we think this was a push-scene:
		&& ( !app->page()->launchingAppId().size() || !app->isChildApp() )
		// Do not cache if the page explicitly says so:
		&& (app->page()->url().isEmpty() || !strstr( app->page()->url().toString().toUtf8().constData(), "nocache=true" ))
		// Do not cache if running first-use:
		&& ( Settings::LunaSettings()->uiType != Settings::UI_MINIMAL )
		// Do not cache a partially loaded page
		&& (app->page()->progress() == 100)
		// Do not cache a page which is shutting down
		&& (!app->page()->isShuttingDown())
		&& !m_disableAppCaching
		)
	{
		app->freezeInCache();
		cached = true;
	}
	else if (m_disableAppCaching) {
		g_message ("%s: App caching disabled, cannot cache app", __PRETTY_FUNCTION__);
	}

	if (cached)
		return;
	
	delete app;

	// ----------------------------------------------------------------------

	std::set<std::string> appsToLaunchAtBoot =
		Settings::LunaSettings()->appsToLaunchAtBoot;
	const std::set<std::string>& appsToKeepAliveUntilMemPressure =
		Settings::LunaSettings()->appsToKeepAliveUntilMemPressure;

	appsToLaunchAtBoot.insert( appsToKeepAliveUntilMemPressure.begin(), appsToKeepAliveUntilMemPressure.end() );

	// Close the headless if this was the last window for the app.
	// But also make sure this was not a boot-time launched app.
	if( appsToLaunchAtBoot.find(appId.toStdString()) == appsToLaunchAtBoot.end() )
	{

		if (m_appPageMap.count(appId.toStdString()) > 1) {
			// Other child pages exist
			return;
		}

		int windowedAppCount = 0;
		WebAppBase* appToClose = 0;
		for (AppList::const_iterator it = m_appList.begin();
		     it != m_appList.end(); ++it) {

			WebAppBase* a = (*it);
			if (a->page() && a->page()->appId() == appId) {

				if (a->isWindowed() || a->isCardApp()) // needed because child cards are non-windowed
					windowedAppCount++;
				else
					appToClose = a;
			}
		}

		if (windowedAppCount == 0 && appToClose
		        && appsToKeepAliveUntilMemPressure.find(appId.toStdString()) == appsToKeepAliveUntilMemPressure.end() ) {
			qDebug() << "Closing headless app with no windows: " << appId;
			delete appToClose;
		}

	}

	if( appsToKeepAliveUntilMemPressure.find(appId.toStdString()) != appsToKeepAliveUntilMemPressure.end() ) {
		qDebug() << "Keeping " << appId << " headless alive ...";
	}
}

Event::Orientation WebAppManager::orientation() const
{
	return m_orientation;
}

void WebAppManager::setOrientationInternal(Event::Orientation orient)
{
	// We currently send only these orientation changes
	switch (orient) {
	case (Event::Orientation_Up):
	case (Event::Orientation_Down):
	case (Event::Orientation_Left):
	case (Event::Orientation_Right):
		break;
	default:
		return;
	}

	if (m_orientation == orient)
		return;

	m_orientation = orient;

	// We iterate over the map because these will have windows
	for (AppWindowMap::iterator it = m_appWinMap.begin();
	     it != m_appWinMap.end(); ++it) {

		WebAppBase* app = static_cast<WebAppBase*>(it->second);
		if (app && app->isWindowed()) {
			WindowedWebApp* windowedApp = static_cast<WindowedWebApp*>(app);
			windowedApp->setOrientation(orient);
		}
	}
}

WebAppBase* WebAppManager::findApp(const QString& processId)
{
	for (AppList::iterator it = m_appList.begin();
	     it != m_appList.end(); ++it) {

		WebAppBase* app = static_cast<WebAppBase*>((*it));
		if (app->page() && app->page()->processId() == processId)
			return app;
	}

	return 0;
}

void WebAppManager::markUniversalSearchReady()
{
	s_universalSearchReady = true;

	g_message("%s: launcher is ready", __PRETTY_FUNCTION__);

	// the system was waiting for the launcher to be ready?
	if (s_bootState == BootStateWaitingForUniversalSearch)
		bootFinished();
}

void WebAppManager::bootFinished()
{
	if (s_bootState == BootStateFinished)
		return;

	g_warning("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ BOOT SEQUENCE COMPLETE");

	s_bootState = BootStateFinished;
	s_universalSearchReady = true;

	// Enable script timeout now that we've booted.
//	Palm::WebGlobal::enableJavaScriptTimeout(true);

	// Done initializing. Re-enable the oldgen GCs:
//	Palm::WebGlobal::resetGCAvoids();

	// Run garbage collection
//	Palm::WebGlobal::garbageCollectNow();

	s_ipcChannel->sendAsyncMessage(new ViewHost_BootupFinished());
	MemoryWatcher::instance()->start();

	if (s_bootupIdleSrc) {
		g_source_destroy(s_bootupIdleSrc);
		g_source_unref(s_bootupIdleSrc);
		s_bootupIdleSrc = 0;
	}

	if (s_bootupTimeoutSrc) {
		g_source_destroy(s_bootupTimeoutSrc);
		g_source_unref(s_bootupTimeoutSrc);
		s_bootupTimeoutSrc = 0;
	}
}

gboolean WebAppManager::BootupTimeoutCallback(gpointer)
{
	g_warning("%s: boot up timed out", __PRETTY_FUNCTION__);
	WebAppManager::instance()->bootFinished();
	return FALSE;
}

gboolean WebAppManager::BootupIdleCallback(gpointer)
{
	static struct timeval tv = {0, 0};

	struct timeval now;
	gettimeofday(&now, 0);

	static int count = 0;
	const int minCount = 3;
	const int maxJitterInMs = 5;

	if (((now.tv_sec - tv.tv_sec) * 1000000 + (now.tv_usec - tv.tv_usec)) / 1000 < maxJitterInMs) {

		tv = now;
		count++;

		if (count >= minCount) {

			if (s_universalSearchReady || Settings::LunaSettings()->uiType == Settings::UI_MINIMAL) {
				WebAppManager::instance()->bootFinished();
			}
			else {
				s_bootState = BootStateWaitingForUniversalSearch;

				g_message("%s: waiting for launcher", __PRETTY_FUNCTION__);

				if (s_bootupIdleSrc) {
					g_source_destroy(s_bootupIdleSrc);
					g_source_unref(s_bootupIdleSrc);
					s_bootupIdleSrc = 0;
				}

				if (!s_bootupTimeoutSrc) {
					GMainLoop* mainLoop = WebAppManager::instance()->mainLoop();
					s_bootupTimeoutSrc = g_timeout_source_new_seconds(s_bootTimeout);
					g_source_set_callback(s_bootupTimeoutSrc, WebAppManager::BootupTimeoutCallback, NULL, NULL);
					g_source_attach(s_bootupTimeoutSrc, g_main_loop_get_context(mainLoop));
				}
			}
			return FALSE;
		}

		return TRUE;
	}

	tv = now;
	count = 0;

	return TRUE;
}

typedef struct AppIdDesc {
	std::string id;
	std::string desc;
} AppIdDesc;

// We also want to "close" headless apps that are kept alive
// because of launch time (e.g., browser). However, for these apps, we do
// *not* want to restart them.
void WebAppManager::restartHeadLessBootApps()
{
	if( !Settings::LunaSettings()->canRestartHeadlessApps )
		return;

	// make sure any keep-alive apps are flushed before closing the headless apps
	disableAppCaching(true);

	std::set<std::string> appsToLaunchAtBoot =
	    Settings::LunaSettings()->appsToLaunchAtBoot;

	// Merge these two lists together so we can share the window counting logic below
	const std::set<std::string>& appsToKeepAliveUntilMemPressure =
	    Settings::LunaSettings()->appsToKeepAliveUntilMemPressure;
	appsToLaunchAtBoot.insert( appsToKeepAliveUntilMemPressure.begin(), appsToKeepAliveUntilMemPressure.end() );

	AppList appsToRelaunch;

	for (AppList::const_iterator it = m_appList.begin(); it != m_appList.end(); ++it) {

		WebAppBase* app = *it;
		SysMgrWebBridge* page = app->page();
		if (!page || app->isWindowed() || app->isCardApp())
			continue;

		QString appId = page->appId();
		int windowCount = 0;

		// FIXME: NxN algo....
		for (AppList::const_iterator iter = m_appList.begin();
		     iter != m_appList.end(); ++iter) {

			WebAppBase* a = (*iter);
			if (a->page() && a->page()->appId() == appId) {

				if (a->isWindowed() || a->isCardApp())
					windowCount++;
			}
		}

		if (windowCount)
			continue;

		if (appsToLaunchAtBoot.find(appId.toStdString()) != appsToLaunchAtBoot.end())
			appsToRelaunch.push_back(app);
	}

	typedef std::pair<AppIdDesc, std::string> AppDescUrlPair;
	typedef std::list<AppDescUrlPair> AppDescUrlPairList;

	AppDescUrlPairList relaunchList;

	for (AppList::const_iterator it = appsToRelaunch.begin();
	     it != appsToRelaunch.end(); ++it) {
		AppIdDesc appInfo;

		appInfo.id   = (*it)->page()->appId().toStdString();
        // TODO: client refers to WebAppBase and we currently have no way to get access to that
//		(*it)->page()->client()->getAppDescription()->getAppDescriptionString(appInfo.desc);

		relaunchList.push_back(AppDescUrlPair(appInfo, (*it)->page()->url().toString().toStdString()));

		g_warning("MemoryWatcher: closing app: %s", appInfo.id.c_str());
		closeAppInternal((*it));
	}

	std::string launchArgs = "{\"launchedAtBoot\":true}";

	for (AppDescUrlPairList::const_iterator it = relaunchList.begin();
	     it != relaunchList.end(); ++it) {

		const AppDescUrlPair& appUrlPair = (*it);
		int errorCode = 0;
		// Do not relaunch headless apps that are kept around to improve launch speed.
		if( appsToKeepAliveUntilMemPressure.find(appUrlPair.first.id) == appsToKeepAliveUntilMemPressure.end() ) {
			launchUrlInternal(appUrlPair.second, Window::Type_None,
			                  appUrlPair.first.desc, std::string(), launchArgs,
			                  std::string(), std::string(), errorCode, false, true);
		}
	}
}

void WebAppManager::webPageAdded(SysMgrWebBridge* page)
{
	for (AppIdWebPageMap::iterator it = m_appPageMap.begin();
		 it != m_appPageMap.end(); ++it) {
		if (it->second == page)
			return;
	}

	m_appPageMap.insert(AppIdWebPagePair(page->appId().toStdString(), page));
}

void WebAppManager::shellPageAdded(SysMgrWebBridge* page)
{
	if (page && m_shellPageMap.find(page->appId().toStdString()) == m_shellPageMap.end()) {
		m_shellPageMap[page->appId().toStdString()] = page;
	}
}

void WebAppManager::webPageRemoved(SysMgrWebBridge* page)
{
	if (!m_deletingPages) {

		// Remove from list of pending delete pages
		PageList::iterator iter = std::find(m_pagesToDeleteList.begin(),
		                                    m_pagesToDeleteList.end(), page);
		if (iter != m_pagesToDeleteList.end())
			m_pagesToDeleteList.erase(iter);
	}

	for (AppIdWebPageMap::iterator it = m_appPageMap.begin();
		 it != m_appPageMap.end(); ++it) {

		if ((*it).second == page) {
			m_appPageMap.erase(it);
			return;
		}
	}
}

bool WebAppManager::preventAppUnderLowMemory(const std::string& appId, Window::Type winType, ApplicationDescription* appDesc) const
{
	const std::set<std::string>& appsToAllowInLowMemory = Settings::LunaSettings()->appsToAllowInLowMemory;
	const std::set<std::string>& appsToLaunchAtBoot = Settings::LunaSettings()->appsToLaunchAtBoot;

	bool allowedByMemWatcher = MemoryWatcher::instance()->allowNewWebAppLaunch();

	// If app is headless, we allow it only if its one of the boot time apps
	// or if its one of the "allow under all conditions" apps
	if (winType == Window::Type_None) {

		if (allowedByMemWatcher)
			return false;

		if ((appsToAllowInLowMemory.find(appId) == appsToAllowInLowMemory.end()) &&
		    (appsToLaunchAtBoot.find(appId) == appsToLaunchAtBoot.end())) {

			g_warning("Not allowing headless app: %s under low/critical memory condition",
					appId.c_str());
			return true;
		}

		return false;
	}


	// Is this a card app? We only prevent card apps
	if (winType != Window::Type_Card)
		return false;

	// Is the app one of the "allow under all conditions"
	if (appsToAllowInLowMemory.find(appId) != appsToAllowInLowMemory.end())
		return false;

	// limit the number of cards we are allowed to open
	const std::list<const ProcessBase*> cardApps = WebAppManager::instance()->runningApps(Window::Type_Card);
	if ((Settings::LunaSettings()->cardLimit > 0) &&
	    ((int) cardApps.size() >= Settings::LunaSettings()->cardLimit)) {
		g_warning("Can not open new card, reached the card limit\n");
		return true;
	}

	// Was it allowed by memWatcher based on current memory usage
	if (allowedByMemWatcher)
		return false;

	return true;
}

void WebAppManager::slotMemoryStateChanged(MemoryWatcher::MemState state)
{
	const char* normalStateStr = "normal";
	const char* lowStateStr = "low";
	const char* criticalStateStr = "critical";

	const char* currStateStr = 0;
	if (state == MemoryWatcher::Critical)
		currStateStr = criticalStateStr;
	else if (state == MemoryWatcher::Low)
		currStateStr = lowStateStr;
	else
		currStateStr = normalStateStr;
		
	gchar* lowMemScript = g_strdup_printf("if (window.Mojo && window.Mojo.lowMemoryNotification) {"
										  " window.Mojo.lowMemoryNotification({state:\"%s\"}); } ",
										  currStateStr);

	for (AppList::const_iterator it = m_appList.begin();
		 it != m_appList.end(); ++it) {

		SysMgrWebBridge* page = (*it)->page();
		if (page &&
			(page->parent() == 0) &&	
			(page->progress() == 100) &&
			(!page->isShuttingDown())) {
			//page->webkitPage()->evaluateScript(lowMemScript);
		}
	}

	g_free(lowMemScript);


	// Post memory state over public bus

	std::string memStateStr;

	switch (state) {
	case MemoryWatcher::Low:
		memStateStr = "low";
		break;
	case MemoryWatcher::Critical:
		memStateStr = "critical";
		break;
	case MemoryWatcher::Normal:
	default:
		memStateStr = "normal";
		break;
	}		
	

	LSError lsError;
	LSErrorInit(&lsError);
	
	json_object* reply = json_object_new_object();
	json_object_object_add(reply, "state", json_object_new_string(memStateStr.c_str()));
		
	if (!LSSubscriptionPost(m_servicePublic, "/", "getMemoryStatus",
							json_object_to_json_string(reply), &lsError))
		LSErrorFree(&lsError);
	
	json_object_put(reply);		
}

void WebAppManager::initiateLunaStatsReporting()
{
	static GSource* src = 0;
	if (!src) {

		src = g_timeout_source_new_seconds(kLunaStatsReportingIntervalSecs);
		g_source_set_callback(src, PrvPostLunaStats, NULL, NULL);
		g_source_attach(src, g_main_loop_get_context(WebAppManager::instance()->mainLoop()));
		g_source_unref(src);
	}
}

LSHandle* WebAppManager::getStatsServiceHandle() const
{
	return m_servicePrivate;
}

bool PrvDoGc(LSHandle* handle, LSMessage* message, void* ctxt)
{
//	Palm::WebGlobal::garbageCollectNow();

	LSError lsError;
	LSErrorInit(&lsError);
	if (!LSMessageReply(handle, message, "{ \"returnValue\":true }", &lsError))
		LSErrorFree(&lsError);

	return true;
}

#ifdef USE_HEAP_PROFILER
static bool PrvDumpHeapProfiler(LSHandle* lsHandle, LSMessage *message,
								void *user_data)
{
    EMPTY_SCHEMA_RETURN(LSHandle, message);

	HeapProfilerDump("period");

	json_object* payload = json_object_new_object();
	json_object_object_add(payload, "returnValue", json_object_new_boolean(true));

	LSError err;
	LSErrorInit(&err);
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(payload), &err))
		LSErrorFree (&err);

	json_object_put(payload);

	return true;
}
#endif // USE_HEAP_PROFILER

bool PrvGetLunaStats(LSHandle* handle, LSMessage* message, void* ctxt)
{
    SUBSCRIBE_SCHEMA_RETURN(handle, message);

	bool ret = false;
	LSError lsError;
	bool subscribed = false;
	std::string jsonStr;
	std::string counters;
    std::string perfStats;
	std::multimap<std::string,std::string> docMap;

	LSErrorInit(&lsError);

	jsonStr = "{ ";

	if (!message) {
		ret = false;
		goto Done;
	}

	if (LSMessageIsSubscription(message)) {

		ret = LSSubscriptionProcess(handle, message, &subscribed, &lsError);
		if (!ret) {
			LSErrorFree(&lsError);
			goto Done;
		}
	}
/*
	if (Palm::MemStats::getJSON(docMap, counters)) {

		// assemble the documents array:
		jsonStr += " \"documents\": [";

		int index = 0;
		std::multimap<std::string,std::string>::const_iterator it;
		for (it=docMap.begin(), index = 0; it != docMap.end(); ++it, ++index) {

			if (index != 0)
				jsonStr += ", ";

			jsonStr += "{ ";
			jsonStr +=  it->second;
			jsonStr += " }";
		}

		jsonStr += " ],\n";

		// assemble the counters frame:
		jsonStr += " \"counters\": {";
		jsonStr += counters;
		jsonStr += " }, ";
	}
*/
/*    if (Palm::WebKitStats::getJSON(perfStats)) {
        jsonStr += " \"perfStats\": ";
        jsonStr += perfStats;
        jsonStr += ", ";
    }*/

	if (subscribed)
		WebAppManager::instance()->initiateLunaStatsReporting();

Done:

	jsonStr += "\"returnValue\":";
	jsonStr += ret ? "true" : "false";
	jsonStr += ", ";

	jsonStr += "\"subscribed\":";
	jsonStr += subscribed ? "true" : "false";
	jsonStr += " }";

	if (!LSMessageReply(handle, message, jsonStr.c_str(), &lsError))
		LSErrorFree(&lsError);

	return true;
}

gboolean PrvPostLunaStats(gpointer ctxt)
{
	LSError lsError;
	std::string jsonStr;
	std::string counters;
	std::multimap<std::string,std::string> docMap;

//	if (!Palm::MemStats::getJSON(docMap, counters))
//		return TRUE;

	LSErrorInit(&lsError);


	jsonStr = "{ ";

	// assemble the documents array:
	jsonStr += " \"documents\": [";

	int index = 0;
	std::multimap<std::string,std::string>::const_iterator it;
	for (it=docMap.begin(), index = 0; it != docMap.end(); ++it, ++index) {

		if (index != 0)
			jsonStr += ", ";

		jsonStr += "{ ";
		jsonStr +=  it->second;
		jsonStr += " }";
	}

	jsonStr += " ],\n";

	// assemble the counters frame:
	jsonStr += " \"counters\": {";
	jsonStr += counters;
	jsonStr += " } ";

    std::string perfStats;
/*    if (Palm::WebKitStats::getJSON(perfStats)) {
        jsonStr += ", \"perfStats\": ";
        jsonStr += perfStats;
    }*/

	jsonStr += " }";

	if (!LSSubscriptionPost(WebAppManager::instance()->getStatsServiceHandle(),
							"/", "getStats", jsonStr.c_str(), &lsError))
		LSErrorFree (&lsError);	

	return TRUE;
}

bool PrvGetAppEvents(LSHandle* handle, LSMessage* message, void* ctxt)
{
    SUBSCRIBE_SCHEMA_RETURN(handle, message);

	bool ret = true;
	LSError lsError;
	bool subscribed = false;
	
	LSErrorInit(&lsError);

	if (!message) {
		ret = false;
		goto Done;
	}

	if (LSMessageIsSubscription(message)) {

		ret = LSSubscriptionProcess(handle, message, &subscribed, &lsError);
		if (!ret) {
			LSErrorFree(&lsError);
			goto Done;
		}
	}

Done:

	std::string jsonStr;
	jsonStr += "\"returnValue\":";
	jsonStr += ret ? "true" : "false";
	jsonStr += ", ";

	jsonStr += "\"subscribed\":";
	jsonStr += subscribed ? "true" : "false";
	jsonStr += " }";

	if (!LSMessageReply(handle, message, jsonStr.c_str(), &lsError))
		LSErrorFree(&lsError);

	return true;	
}

bool PrvDumpRenderTree(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
/*
	std::stringstream retval;
	retval << "{ \"cardViews\":[";

	std::string quotes = "\"";
	std::string replace_quotes = "\\\"";

	std::list<const ProcessBase *> cards = WebAppManager::instance()->runningApps(Window::Type_Card);
	for (std::list<const ProcessBase *>::const_iterator iter = cards.begin(); iter != cards.end(); iter++)
	{
		if (iter != cards.begin())
			retval << ", ";

		CardWebApp *card = static_cast<CardWebApp *>(WebAppManager::instance()->findApp((*iter)->processId()));
 		retval << "{ \"tree\":\"";
		std::string str = card->page()->webkitPage()->mainFrame()->renderTreeDump();

		size_t start = 0;
		while(1) {
  			size_t pos = str.find(quotes, start);
  			if (pos==std::string::npos)
				break;
 			str.replace(pos, quotes.size(), replace_quotes);
  			start = pos + replace_quotes.size();
		}

		retval << str;
		retval << "\" }";
	}

	retval << "], \"status\":true }";

	LSError error;
	LSErrorInit(&error);
	if (!LSMessageReply(lsHandle, message, retval.str().c_str(), &error)) {
		LSErrorFree(&error);
	}
*/
	return true;
}

bool PrvDumpCompositedTree(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
/*
	std::stringstream retval;
	retval << "{ \"cardViews\":[";

	std::list<const ProcessBase *> cards = WebAppManager::instance()->runningApps(Window::Type_Card);
	for (std::list<const ProcessBase *>::const_iterator iter = cards.begin(); iter != cards.end(); iter++)
	{
		if (iter != cards.begin())
			retval << ", ";

		CardWebApp *card = static_cast<CardWebApp *>(WebAppManager::instance()->findApp((*iter)->processId()));
		card->page()->webkitView()->dumpCompositedTree(retval);
	}

	retval << "], \"memStats\":\"";
//	Palm::WebView::dumpMemStats(retval);
	retval << "\", \"status\":true }";

	LSError error;
	LSErrorInit(&error);
	if (!LSMessageReply(lsHandle, message, retval.str().c_str(), &error)) {
		LSErrorFree(&error);
	}
*/
	return true;
}

bool EnablePiranhaFpsCounter(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
/*
	const char* str = LSMessageGetPayload(message);
	if (!str)
		return false;

	struct json_object* root = json_tokener_parse(str);
	struct json_object* label = 0;
	bool failed = true;
	int reset = 0;

	if (!root || is_error(root))
		goto Done;


	// iterate through the elements
	json_object_object_foreach(root, key, val)
	{
		if (strcmp(key, "enable") == 0)
		{
			label = json_object_object_get(root, "enable");

			if (label && json_object_is_type(label, json_type_boolean))
			{
				PSoftContext2D::SetGlobalAttribute(0, json_object_get_boolean(label));
#if defined(DIRECT_RENDERING)
				PGLESContext2D::SetGlobalAttribute(0, json_object_get_boolean(label));
#endif
				failed = false;
			}
		}
		else if (strcmp(key, "reset") == 0)
		{
			if (json_object_is_type(val, json_type_int))
			{
				reset = json_object_get_int(val);
				PSoftContext2D::SetGlobalAttribute(5, reset);
#if defined(DIRECT_RENDERING)
				PGLESContext2D::SetGlobalAttribute(5, reset);
#endif
				failed = false;
			}
		}
		else if (strcmp(key, "dump") == 0)
		{
			PSoftContext2D::SetGlobalAttribute(5, 1);
#if defined(DIRECT_RENDERING)
			PGLESContext2D::SetGlobalAttribute(5, 1);
#endif
			failed = false;
		}
		else
		{
			//Nothing to do.
		}
	}
	
Done:

	json_object* reply = json_object_new_object();
	json_object_object_add(reply, "returnValue", json_object_new_boolean(!failed));

	LSError err;
	LSErrorInit(&err);
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &err))
		LSErrorFree(&err);	

	json_object_put(reply);

	if (!root && !is_error(root))
		json_object_put(root);
*/
	return true;
}

void WebAppManager::copiedToClipboard(const QString& appId)
{
	if (!s_lastCopyClipboardMessageId.empty()) {
		BannerMessageEvent* e = BannerMessageEventFactory::createRemoveMessageEvent(appId.toStdString(),
	                                                                         s_lastCopyClipboardMessageId);
		sendAsyncMessage(new ViewHost_BannerMessageEvent(BannerMessageEventWrapper(e)));
	}

	std::string nullString;
	BannerMessageEvent* e = BannerMessageEventFactory::createAddMessageEvent(appId.toStdString(),
																			 LOCALIZED("Selection Copied"),
																			 "{ }", nullString, nullString,
																			 nullString, -1, false);
	sendAsyncMessage(new ViewHost_BannerMessageEvent(BannerMessageEventWrapper(e)));

	s_lastCopyClipboardMessageId = e->msgId;
}

void WebAppManager::pastedFromClipboard(const QString& appId)
{

}

void WebAppManager::closePageSoon(SysMgrWebBridge* page)
{
	if (!page)
		return;

	if (std::find(m_pagesToDeleteList.begin(), m_pagesToDeleteList.end(), page)
	    != m_pagesToDeleteList.end())
		return;

	m_pagesToDeleteList.push_back(page);

	GSource* src = g_timeout_source_new_seconds(0);
	g_source_set_callback(src, WebAppManager::deletePagesCallback, this, NULL);
	g_source_attach(src, g_main_loop_get_context(mainLoop()));
	g_source_unref(src);
}

gboolean WebAppManager::deletePagesCallback(gpointer arg)
{
	WebAppManager* man = (WebAppManager*) arg;
	man->deletePages();

	return FALSE;
}

void WebAppManager::deletePages()
{
/*
	m_deletingPages = true;

	PageList tmpList = m_pagesToDeleteList;

	for (PageList::iterator it = tmpList.begin();
	     it != tmpList.end(); ++it) {

		SysMgrWebBridge* page = (*it);
		if (page->getClient())
			page->getClient()->close();
		else
		    delete page;
	}

	m_pagesToDeleteList.clear();

	m_deletingPages = false;
*/
}

static bool PrvGetSystemTimeCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
    // {"timezone": string}
    VALIDATE_SCHEMA_AND_RETURN(handle,
                               message,
                               SCHEMA_1(REQUIRED(timezone, string)));

	const char* payload = LSMessageGetPayload(message);
	if (!message)
		return true;

	json_object* label = 0;
	json_object* json = 0;
	std::string newTimeZone;

	label = 0;
	json = json_tokener_parse(payload);
	if (!json || is_error(json))
		goto Done;

	label = json_object_object_get(json, "timezone");
	if (!label || is_error(label))
		goto Done;

	newTimeZone = json_object_get_string(label);

	g_message("PrvGetSystemTimeCallback(): new timezone specified as [%s]\n",newTimeZone.c_str());
	if (newTimeZone != s_timeZone) {

		// timezone has changed
		s_timeZone = newTimeZone;

		setenv("TZ", newTimeZone.c_str(), 1);
		tzset();

		g_message("PrvGetSystemTimeCallback(): timezone changed to [%s]",newTimeZone.c_str());
		g_message("PrvGetSystemTimeCallback(): TZ is now [%s]",getenv("TZ"));
	}

	//Call resetDateCache whenever timezone changes as well as date & time changes.
	g_message("PrvGetSystemTimeCallback(): calling resetDateCache()");
//	Palm::WebGlobal::resetDateCache();

Done:

	if (json && !is_error(json))
		json_object_put(json);

	return true;
}

void WebAppManager::mimeHandoffUrl(const char* mimeType, const char* url, const char* callerId)
{
    if (!mimeType && !url)
		return;

	json_object* json = json_object_new_object();

	if (mimeType && strcasecmp(mimeType, "unknown") != 0)
		json_object_object_add(json, (char*) "mime", json_object_new_string(mimeType));

	if (url)
		json_object_object_add(json, (char*) "target", json_object_new_string(url));

	g_debug("%s:%d Calling applicationManager open with payload: %s",
	        __PRETTY_FUNCTION__, __LINE__, json_object_to_json_string(json));

	LSError lsError;
	LSErrorInit(&lsError);

	if (!LSCallFromApplication(m_servicePrivate, "palm://com.palm.applicationManager/open",
	                           json_object_to_json_string(json), callerId,
	                           NULL, NULL, NULL, &lsError)) {
		g_critical("%s:%d, Failed in LSCall to applicationManager/open: %s",
		           __PRETTY_FUNCTION__, __LINE__, lsError.message);
		LSErrorFree(&lsError);
	}

	json_object_put(json);
}

WindowedWebApp* WebAppManager::appForIpcKey(int key)
{
	if (!key)
		return 0;

	AppWindowMap::const_iterator it = m_appWinMap.find(key);
	if (it != m_appWinMap.end())
		return it->second;

	return 0;
}

SysMgrWebBridge* WebAppManager::takeShellPageForApp(const std::string& appId)
{
	AppIdShellPageMap::iterator it = m_shellPageMap.find(appId);
	SysMgrWebBridge* shellPage = 0;
	if (it != m_shellPageMap.end()) {
		shellPage = it->second;
		m_shellPageMap.erase(it);
	}
	return shellPage;
}

void WebAppManager::addHeadlessAppToWatchList(WebAppBase* app)
{
	AppLaunchTimeMap::const_iterator it = m_headlessAppLaunchTimeMap.find(app);
	if (it != m_headlessAppLaunchTimeMap.end())
		return;

	m_headlessAppLaunchTimeMap[app] = Time::curTimeMs();

	m_headlessAppWatchTimer.stop();
	m_headlessAppWatchTimer.start(kHeadlessAppWatchTimeoutMs, false);
}

void WebAppManager::removeHeadlessAppFromWatchList(WebAppBase* app)
{
	AppLaunchTimeMap::iterator it = m_headlessAppLaunchTimeMap.find(app);
	if (it == m_headlessAppLaunchTimeMap.end())
		return;

	qDebug() << "WATCH: Removing app from watch list: " << app->appId();

	m_headlessAppLaunchTimeMap.erase(it);
	if (m_headlessAppLaunchTimeMap.empty())
		m_headlessAppWatchTimer.stop();
}

bool WebAppManager::headlessAppWatchCallback()
{
	if (m_headlessAppLaunchTimeMap.empty())
		return false;

	if (s_bootState != BootStateFinished)
		return true;

	const std::set<std::string>& appsToLaunchAtBoot = Settings::LunaSettings()->appsToLaunchAtBoot;
	const std::set<std::string>& appsToKeepAlive = Settings::LunaSettings()->appsToKeepAlive;

	AppList appsToDelete;
	for (AppLaunchTimeMap::iterator it = m_headlessAppLaunchTimeMap.begin();
	     it != m_headlessAppLaunchTimeMap.end();) {

		WebAppBase* app = it->first;

		g_message("WATCH: Checking app: %s", app->appId().toAscii().data());

		// this should not happen
		if (!app->page()) {
			appsToDelete.push_back(app);
			m_headlessAppLaunchTimeMap.erase(it++);
			continue;
		}

		std::string appId = app->page()->appId().toStdString();

		if ((appsToLaunchAtBoot.find(appId) != appsToLaunchAtBoot.end()) ||
		    (appsToKeepAlive.find(appId) != appsToKeepAlive.end())) {
			m_headlessAppLaunchTimeMap.erase(it++);
			continue;
		}

		if (m_appPageMap.count(appId) > 1) {
			// other child pages exist. We don't need to track this app.
			// when user closes the child pages, the headless part will get closed
			m_headlessAppLaunchTimeMap.erase(it++);
			continue;
		}

		uint32_t runTime = Time::curTimeMs() - it->second;
		if (runTime > kHeadlessAppAllowedTimeForSoloRunMs) {
			// We have a winner.
			g_message("%s: Marking headless app for closure: %s",
					  __PRETTY_FUNCTION__, appId.c_str());
			appsToDelete.push_back(app);
		}

		++it;
	}

	for (AppList::const_iterator it = appsToDelete.begin();
	     it != appsToDelete.end(); ++it) {
		closeAppInternal((*it));
	}

	if (m_headlessAppLaunchTimeMap.empty())
		return false;

	return true;
}

void WebAppManager::setAppWindowProperties(int appIpcKey, WindowProperties &winProp)
{
	WindowedWebApp *app = appForIpcKey(appIpcKey);

	if(!app)
		return;

	app->setWindowProperties(winProp);
}

const void WebAppManager::setHostInfo(const HostInfo *hostInfo)
{
	memcpy(&hostInfoCache, hostInfo, sizeof(HostInfo));
	m_uiWidth  = hostInfoCache.displayWidth;
	m_uiHeight = hostInfoCache.displayHeight;

	if((Settings::LunaSettings()->homeButtonOrientationAngle == 0) || (Settings::LunaSettings()->homeButtonOrientationAngle == 180)) {
		if(m_uiWidth > m_uiHeight) {
			m_deviceIsPortraitType = false;
		} else {
			m_deviceIsPortraitType = true;
		}
	} else {
		if(m_uiWidth > m_uiHeight) {
			m_deviceIsPortraitType = true;
		} else {
			m_deviceIsPortraitType = false;
		}
	}
}

const HostInfo& WebAppManager::getHostInfo() const
{
	return hostInfoCache;
}

bool WebAppManager::systemServiceConnectCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
	if (!message)
		return true;

    // {"serviceName": string, "connected": boolean}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_2(REQUIRED(serviceName, string), REQUIRED(connected, boolean)));

	const char* payload = LSMessageGetPayload(message);
	if (!payload)
		return true;

	json_object* label = 0;
	json_object* json = 0;
	bool connected = false;
	
	label = 0;		
	json = json_tokener_parse(payload);
	if (!json || is_error(json))
		goto Done;

	label = json_object_object_get(json, "connected");
	if (!label || is_error(label))
		goto Done;
	connected = json_object_get_boolean(label);

	if (connected) {

		LSHandle* service = WebAppManager::instance()->m_servicePrivate;

		LSError lserror;
		LSErrorInit(&lserror);	
		bool r;
		
		// We are connected to the systemservice. call and get the preference values
		r = LSCall(service, "palm://com.palm.systemservice/time/getSystemTime",
				   "{\"subscribe\":true}", PrvGetSystemTimeCallback, NULL, NULL, &lserror);
		if (!r) {
			LSErrorFree(&lserror);
			goto Done;
		}
		
		r = LSCall(service, "palm://com.palm.systemservice/getPreferences", 
					"{ \"subscribe\": true, \"keys\":[\"x_palm_carrier\", \"x_palm_textinput\"]}",
					WebAppManager::sysServicePrefsCallback, NULL, NULL, &lserror);
		if (!r) {
			LSErrorFree(&lserror);
			goto Done;
		}		
	}

Done:

	if (json && !is_error(json))
		json_object_put(json);

	return true;	
}

void WebAppManager::reportAppLaunched(const QString& appId, const QString& processId)
{
	if (!getStatsServiceHandle())
		return;

	json_object* json = json_object_new_object();
	json_object_object_add(json, (char*) "event", json_object_new_string("launch"));
	json_object_object_add(json, (char*) "appId", json_object_new_string(appId.toUtf8().constData()));
	json_object_object_add(json, (char*) "processId", json_object_new_string(processId.toUtf8().constData()));

	char* str = json_object_to_json_string(json);

	LSError lsError;
	LSErrorInit(&lsError);
	
	if (!LSSubscriptionPost(getStatsServiceHandle(),
							"/", "getAppEvents", str, &lsError))
		LSErrorFree (&lsError);	

	json_object_put(json);
}

void WebAppManager::reportAppClosed(const QString& appId, const QString& processId)
{
	if (!getStatsServiceHandle())
		return;

	json_object* json = json_object_new_object();
	json_object_object_add(json, (char*) "event", json_object_new_string("close"));
	json_object_object_add(json, (char*) "appId", json_object_new_string(appId.toUtf8().constData()));
	json_object_object_add(json, (char*) "processId", json_object_new_string(processId.toUtf8().constData()));

	char* str = json_object_to_json_string(json);

	LSError lsError;
	LSErrorInit(&lsError);
	
	if (!LSSubscriptionPost(getStatsServiceHandle(),
							"/", "getAppEvents", str, &lsError))
		LSErrorFree (&lsError);	

	json_object_put(json);    
}

void WebAppManager::disableAppCaching (bool disable)
{
	if (disable) {
		g_message ("%s: flushing WebAppCache", __PRETTY_FUNCTION__);
		WebAppCache::flush();
	}

	m_disableAppCaching = disable;
}

WebAppBase* WebAppManager::findAppById(const QString& appId)
{
	MutexLocker locker(&m_mutex);

	for (AppList::iterator it = m_appList.begin();
	     it != m_appList.end(); ++it) {

		WebAppBase* app = static_cast<WebAppBase*>((*it));

		if (app->page() && app->page()->appId() == appId)
			return app;
	}

	return 0;
}

void WebAppManager::onUiDimensionsChanged(int width, int height)
{
	m_uiWidth  = width;
	m_uiHeight = height;
}

void WebAppManager::onSuspendWebkitProcess(bool* result)
{
	// the SysMgr process is suspending this process. As soon as we return from this function the process will be suspended.

	*result = true;
}

bool WebAppManager::activityManagerCallback(LSHandle* sh, LSMessage* message, void* ctx)
{
    // {"returnValue": boolean, "activityId": integer}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_2(REQUIRED(returnValue, boolean), REQUIRED(activityId, integer)));

	const char* payload = LSMessageGetPayload(message);
	if (!payload)
		return true;

	WebAppBase* app = (WebAppBase*) ctx;
	if (!app || !app->page())
		return true;
	
	json_object* json = 0;
	json_object* label = 0;

	json = json_tokener_parse(payload);
	if (!json || is_error(json))
		return true;

	label = json_object_object_get(json, "returnValue");
	if (!label || is_error(label)) {
		json_object_put(json);
		return true;
	}

	bool success = json_object_get_boolean(label);
	if (!success) {
		json_object_put(json);
		return true;
	}

	label = json_object_object_get(json, "activityId");
	if (!label || is_error(label)) {
		json_object_put(json);
		return true;
	}
	
	int activityId = json_object_get_int(label);
	app->page()->setActivityId(activityId);

	json_object_put(json);
	
	return true;
}

bool WebAppManager::displayManagerConnectCallback(LSHandle* sh, LSMessage* message, void* ctx)
{
    // {"serviceName": string, "connected": boolean}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_2(REQUIRED(serviceName, string), REQUIRED(connected, boolean)));

	const char* payload = LSMessageGetPayload(message);
	if (!message)
		return true;

	json_object* json = json_tokener_parse(payload);
	if (!json || is_error(json)) {
		return true;
	}


	LSHandle* service = (LSHandle*)ctx;

	LSError lserror;
	LSErrorInit(&lserror);

	json_object* value = json_object_object_get(json, "connected");
	if (!value) {
		json_object_put(json);
		return true;
	}

	bool connected = json_object_get_boolean(value);
	if (!connected) {
		json_object_put(json);
		return true;
	}

	bool success = LSCall(service, "palm://com.palm.display/control/status",
						  "{\"subscribe\":true}",
						  WebAppManager::displayManagerCallback,
						  NULL, NULL, &lserror);
	if (!success) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	json_object_put(json);

	return true;    
}

bool WebAppManager::displayManagerCallback(LSHandle* sh, LSMessage* message, void* ctx)
{
    // {"event": string}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_1(REQUIRED(event, string)));

	const char* payload = LSMessageGetPayload(message);
	if (!payload)
		return true;

	json_object* json = 0;
	json_object* label = 0;
	
	json = json_tokener_parse(payload);
	if (!json || is_error(json))
		return true;

	label = json_object_object_get(json, "event");
	if (!label) {
		json_object_put(json);
		return true;
	}

	std::string displayEvent = json_object_get_string(label);
	if (displayEvent == "displayOn") {
		WebAppManager* wam = WebAppManager::instance();
		if (!wam->m_displayOn)  {
			wam->m_displayOn = true;			
			for (AppList::const_iterator it = wam->m_appList.begin();
				 it != wam->m_appList.end(); ++it) {
				WebAppBase* app = (WebAppBase*) *it;
				if (app->isWindowed())
					static_cast<WindowedWebApp*>(app)->displayOn();
			}
		
			wam->stopGcPowerdActivity();
//			Palm::WebGlobal::notifyWake();
		}
	}
	else if (displayEvent == "displayOff") {
		WebAppManager* wam = WebAppManager::instance();
		if (wam->m_displayOn) {
			wam->m_displayOn = false;
			for (AppList::const_iterator it = wam->m_appList.begin();
				 it != wam->m_appList.end(); ++it) {
				WebAppBase* app = (WebAppBase*) *it;
				if (app->isWindowed())
					static_cast<WindowedWebApp*>(app)->displayOff();
			}

			wam->startGcPowerdActivity();
		}
	}

	json_object_put(json);
	
	return true;	
}

void WebAppManager::startGcPowerdActivity()
{
	stopGcPowerdActivity();

	static int s_powerdActivityIndex = 0;
	gchar* activityId = g_strdup_printf("com.palm.lunastats-%d", s_powerdActivityIndex);
	m_gcPowerdActivityId = activityId;
	g_free(activityId);
	
	s_powerdActivityIndex++;
	if (s_powerdActivityIndex < 0)
		s_powerdActivityIndex = 0;

	LSError lsError;
	LSErrorInit(&lsError);

	g_message("%s: starting GC powerd activity: %s", __PRETTY_FUNCTION__, m_gcPowerdActivityId.c_str());

	gchar* params = g_strdup_printf("{\"id\": \"%s\", \"duration_ms\": %d}",
									m_gcPowerdActivityId.c_str(), kGcPowerdActivityDuration);	
	
	bool ret = LSCall(m_servicePrivate, "palm://com.palm.power/com/palm/power/activityStart",
					  params, NULL, NULL, NULL, &lsError);
	g_free(params);

	if (!ret) {
		g_warning("%s: Failed to call powerd activityEnd: %s",
				  __PRETTY_FUNCTION__, lsError.message);
		LSErrorFree(&lsError);
		m_gcPowerdActivityId = std::string();
		return;
	}
		

	m_gcPowerdActivityTimer.start(kGcStartTimeout, true);
}


void WebAppManager::stopGcPowerdActivity()
{
	m_gcPowerdActivityTimer.stop();

	if (m_gcPowerdActivityId.empty())
		return;

	g_message("%s: stopping GC powerd activity: %s", __PRETTY_FUNCTION__, m_gcPowerdActivityId.c_str());
	
	LSError lsError;
	LSErrorInit(&lsError);

	gchar* params = g_strdup_printf("{\"id\": \"%s\"}", m_gcPowerdActivityId.c_str());
	
	bool ret = LSCall(m_servicePrivate, "palm://com.palm.power/com/palm/power/activityEnd",
					  params, NULL, NULL, NULL, &lsError);
	g_free(params);
	m_gcPowerdActivityId = std::string();    

	if (!ret) {
		g_warning("%s: Failed to call powerd activityEnd: %s",
				  __PRETTY_FUNCTION__, lsError.message);
		LSErrorFree(&lsError);
	}
}

bool PrvGetMemoryStatus(LSHandle* handle, LSMessage* message, void* ctxt)
{
    SUBSCRIBE_SCHEMA_RETURN(handle, message);

	bool ret = false;
	LSError lsError;
	bool subscribed = false;
	std::string memStateStr = "normal";
	MemoryWatcher::MemState memState;
	
	LSErrorInit(&lsError);

	if (!message) {
		ret = false;
		goto Done;
	}

	if (LSMessageIsSubscription(message)) {

		ret = LSSubscriptionProcess(handle, message, &subscribed, &lsError);
		if (!ret) {
			LSErrorFree(&lsError);
			goto Done;
		}
	}

	memState = MemoryWatcher::instance()->state();

	switch (memState) {
	case MemoryWatcher::Low:
		memStateStr = "low";
		break;
	case MemoryWatcher::Critical:
		memStateStr = "critical";
		break;
	case MemoryWatcher::Normal:
	default:
		memStateStr = "normal";
		break;
	}

	ret = true;


Done:

	if (ret) {
		json_object* reply = json_object_new_object();
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		json_object_object_add(reply, "subscribed", json_object_new_boolean(subscribed));
		json_object_object_add(reply, "state", json_object_new_string(memStateStr.c_str()));
		
		if (!LSMessageReply(handle, message, json_object_to_json_string(reply),
							&lsError))
			LSErrorFree(&lsError);

		json_object_put(reply);
	}
	else {

		if (!LSMessageReply(handle, message,
							"{\"returnValue\": false}",
							&lsError))
			LSErrorFree(&lsError);
	}

	return true;
}

static long
percentages(int cnt, int *out, long *now, long *old, long *diffs)
{
	register int i;
	register long change;
	register long total_change;
	register long *dp;
	long half_total;

	/* initialization */
	total_change = 0;
	dp = diffs;

	/* calculate changes for each state and the overall change */
	for (i = 0; i < cnt; i++)
	{
		if ((change = *now - *old) < 0)
		{
			/* this only happens when the counter wraps */
			change = (int)
				((unsigned long)*now-(unsigned long)*old);
		}
		total_change += (*dp++ = change);
		*old++ = *now++;
	}

	/* avoid divide by zero potential */
	if (total_change == 0)
	{
		total_change = 1;
	}

	/* calculate percentages based on overall change, rounding up */
	half_total = total_change / 2l;
	for (i = 0; i < cnt; i++)
	{
		*out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
	}

	/* return the total in case the caller wants to use it */
	return(total_change);
}

static inline char *
skip_token(const char *p)
{
	while (isspace(*p)) p++;
	while (*p && !isspace(*p)) p++;
	return (char *)p;
}

static void
read_stat(int *cpu_states, long *cp_time, long *cp_old, long *cp_diff, bool sample_only)
{
	char buffer[4096+1];
	int fd, len;
	char *p;

	if ((fd = open("/proc/stat", O_RDONLY)) != -1)
	{
		if ((len = read(fd, buffer, sizeof(buffer)-1)) > 0)
		{
			buffer[len] = '\0';
			p = skip_token(buffer);                     /* "cpu" */
			cp_time[0] = strtoul(p, &p, 0);
			cp_time[1] = strtoul(p, &p, 0);
			cp_time[2] = strtoul(p, &p, 0);
			cp_time[3] = strtoul(p, &p, 0);

			/* convert cp_time counts to percentages */

			if (!sample_only)
				percentages(4, cpu_states, cp_time, cp_old, cp_diff);
		}
		close(fd);
	}
}

static unsigned
CpuIdle(void)
{
	long cp_time[4];
	long cp_old[4];
	long cp_diff[4];
	int cpu_states[4];

	read_stat(NULL, cp_old, NULL, NULL, true);
	sleep(1);
	read_stat(cpu_states, cp_time, cp_old, cp_diff, false);

	return cpu_states[3];
}

bool WebAppManager::gcPowerdActivtyTimerCallback()
{

	static unsigned s_cancelledGCs = 0;
	if (s_cancelledGCs < kNumTimesIgnoreGc && CpuIdle() < kGcCpuIdleThreshold) {
		s_cancelledGCs++;
		g_message("%s: Skipping gc because CPU too busy", __PRETTY_FUNCTION__);
	}
	else {
		s_cancelledGCs = 0;

		g_message("%s: Calling gc....", __PRETTY_FUNCTION__);

		uint32_t startTime = Time::curTimeMs();
		int gcIterationCount = 1;
/*		while (!Palm::WebGlobal::notifySleep())
			gcIterationCount++;*/
		uint32_t endTime = Time::curTimeMs();

		g_message("%s: Gc took %d iterations and %d ms", __PRETTY_FUNCTION__, gcIterationCount, endTime - startTime);
	}

	stopGcPowerdActivity();
	return false;
}

SharedGlobalProperties* WebAppManager::globalProperties()
{
    return s_globalPropsBuffer ? (SharedGlobalProperties*) s_globalPropsBuffer->data() : 0;
}

void WebAppManager::appDeleted(WebAppBase* app)
{
    if (!app)
        return;

    std::string appId;
    if (app->page())
        appId = app->page()->appId().toStdString();

    m_appList.remove(app);

    if (!appId.empty())
        m_shellPageMap.erase(appId);        
}
