/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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

#include <string.h>
#include "WebAppMgrProxy.h"

#include <PIpcChannel.h>
#include <PIpcBuffer.h>
#include <cjson/json.h>
#include <sys/time.h>
#include <sys/resource.h>


#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "Window.h"
#include "CardWindow.h"
#include "DockModeWindow.h"
#include "EventThrottler.h"
#include "EventThrottlerIme.h"
#include "AlertWindow.h"
#include "DashboardWindow.h"
#include "MenuWindow.h"
#include "IpcServer.h"
#include "SystemUiController.h"
#include "WindowServer.h"
//#include "WebAppManager.h"
#include "Settings.h"
#include "ApplicationDescription.h"
#include "ApplicationManager.h"
#include "CoreNaviManager.h"
#include "BannerMessageHandler.h"
#include "Event.h"
#include "Logging.h"
#include "MemoryMonitor.h"
#include "CustomEvents.h"
#include "HostWindowData.h"

static WebAppMgrProxy* s_instance = NULL;
static gchar* s_appToLaunchWhenConnectedStr = NULL;


void WebAppMgrProxy::setAppToLaunchUponConnection(char* app)
{
	s_appToLaunchWhenConnectedStr = app;
}

WebAppMgrProxy* WebAppMgrProxy::connectWebAppMgr(int pid, PIpcChannel* channel)
{
	WebAppMgrProxy* pThis = instance();

	if (!pThis) {
		g_critical("%s (%d) WebAppMgrProxy instance is NULL", __PRETTY_FUNCTION__, __LINE__);
		return NULL;
	}

	if (pThis->connected()) {
		g_critical("%s (%d): ERROR: WebAppMgr instance already Connected!!!",
		           __PRETTY_FUNCTION__, __LINE__);
		return NULL;
	}

	pThis->clientConnected(pid, channel);

	g_child_watch_add_full(G_PRIORITY_HIGH, pid, WebAppMgrProxy::webKitDiedCallback,
	                       pThis, NULL);

	// Launch the SystemUI App
	ApplicationManager* appMgr = ApplicationManager::instance();
	ApplicationDescription* sysUiAppDesc = appMgr ? appMgr->getAppById("com.palm.systemui") : 0;
	if (sysUiAppDesc) {
		std::string backgroundPath = "file://" + Settings::LunaSettings()->lunaSystemPath + "/index.html";
		std::string sysUiDescString;
		sysUiAppDesc->getAppDescriptionString(sysUiDescString);
		pThis->launchUrl(backgroundPath.c_str(), Window::Type_StatusBar,
		                 sysUiDescString.c_str(), "", "{\"launchedAtBoot\":true}");
	}
	else {
		g_critical("Failed to launch System UI application");
	}

	// Launch the Launcher App
	if (Settings::LunaSettings()->uiType == Settings::UI_LUNA) {
		ApplicationDescription* appDesc = appMgr ? appMgr->getAppById("com.palm.launcher") : 0;
		if (appDesc) {
			std::string appDescString;
			appDesc->getAppDescriptionString(appDescString);
			pThis->launchUrl(appDesc->entryPoint().c_str(), Window::Type_Launcher,
			                 appDescString.c_str(), "", "{\"launchedAtBoot\":true}");
		}
		else {
			g_critical("Failed to launch Launcher application");
		}

		// Start the headless Apps
		ApplicationManager::instance()->launchBootTimeApps();
	}
	else if (Settings::LunaSettings()->uiType == Settings::UI_MINIMAL) {

		// The only other app to be launched during minimal mode is phone
		pThis->launchBootTimeApp("com.palm.app.phone");
	}

	// Did user specify an app to launch
	if (s_appToLaunchWhenConnectedStr) {
		std::string errMsg;
		pThis->appLaunch(s_appToLaunchWhenConnectedStr, "", "", "", errMsg);
		s_appToLaunchWhenConnectedStr = NULL;
	}

	pThis->sendQueuedMessages();

	return pThis;
}


WebAppMgrProxy::WebAppMgrProxy()
{
	m_processPriority = 0;
    m_orientation = OrientationEvent::Orientation_Up;
	m_channel = 0;
	m_ipcImgDragBuffer = 0;
	m_dragPixmap = 0;
}

void WebAppMgrProxy::clientConnected(int pid, PIpcChannel* channel)
{
	m_pid = pid;
	m_name = "WebAppManager";//WEB_APP_MGR_IPC_NAME;

	channel->setListener(this);
}

void WebAppMgrProxy::sendQueuedMessages()
{
	// if we have any outgoing messages queued up, send them out
	while (!m_discMsgQueue.empty()) {
		sendAsyncMessage(m_discMsgQueue.front());
		m_discMsgQueue.pop();
	}
}

bool WebAppMgrProxy::connected()
{
	return (0 != m_channel);
}

WebAppMgrProxy* WebAppMgrProxy::instance()
{
	if (G_UNLIKELY(!s_instance)) {
		s_instance = new WebAppMgrProxy();
	}

	return s_instance;
}

void WebAppMgrProxy::onDisconnected()
{
	g_critical("%s (%d): WebAppMgrProxy was Disconnected!!", __PRETTY_FUNCTION__, __LINE__);
	// WebApp Manager was disconnected, so add code here to re-launch it

	// WebKit disconnected (most likely crashed)...
	g_critical("%s (%d): Exiting Sysmgr...", __PRETTY_FUNCTION__, __LINE__);
	exit(0);
	return;

	s_instance = 0;

	IpcClientHost::onDisconnected();
}

void WebAppMgrProxy::webKitDiedCallback(GPid pid, gint status, gpointer data)
{
	WebAppMgrProxy* host = (WebAppMgrProxy*) data;
	host->webKitDied(pid, status);
}

void WebAppMgrProxy::webKitDied(GPid pid, gint status)
{
	g_message("%s: pid: %d, status %d", __PRETTY_FUNCTION__, pid, status);
	IpcServer::instance()->processRemoved(pid, false);
	g_spawn_close_pid(pid);

	g_critical("%s (%d): Exiting Sysmgr...", __PRETTY_FUNCTION__, __LINE__);
	exit(0);
}

WebAppMgrProxy::~WebAppMgrProxy()
{
	IpcServer::instance()->ipcClientHostQuit(this);

	if(m_ipcImgDragBuffer) {
		delete m_ipcImgDragBuffer;
		m_ipcImgDragBuffer = 0;
	}

	if(m_dragPixmap) {
		delete m_dragPixmap;
		m_dragPixmap = 0;
	}

	delete m_channel;
	m_channel = NULL;

	s_instance = NULL;
}

void WebAppMgrProxy::suspendWebKitProcess()
{
	// Send a Synchronous Suspend Request message to the WebAppMgr process
	SystemUiController::instance()->aboutToSendSyncMessage();
	bool result = false;
	// always use a timeout value to avoid deadlock in case of a full IPC buffer
	m_channel->sendSyncSuspendMessage(new View_Mgr_SuspendWebkitProcess(&result), 5000);
}

void WebAppMgrProxy::resumeWebKitProcess()
{
	m_channel->setRemoteIsSuspended(false);
	IpcServer::instance()->resumeProcess(m_pid);
}

void WebAppMgrProxy::sendAsyncMessage(PIpcMessage* msg)
{
	if(connected())	{
		m_channel->sendAsyncMessage(msg);
	} else {
		// queue up the messages while not connected
		m_discMsgQueue.push(msg);
	}
}

PIpcChannel* WebAppMgrProxy::getIpcChannel() const
{
	return m_channel;
}

void WebAppMgrProxy::onMessageReceived(const PIpcMessage& msg)
{
	if (msg.routing_id() != MSG_ROUTING_CONTROL) {
		// ROUTED MESSAGE, forward it to the correct host window
		HostWindow* win = static_cast<HostWindow*>(findWindow(msg.routing_id()));
		if (!win) {
            g_warning("%s (%d). Failed to find app with key: %d  msgType = 0x%x\n",
			           __PRETTY_FUNCTION__, __LINE__, msg.routing_id(), msg.type());
			return;
		}
		win->onMessageReceived(msg);
	} else {
		// CONTROL MESSAGE, Handle it here
		bool msgIsOk;

		IPC_BEGIN_MESSAGE_MAP(WebAppMgrProxy, msg, msgIsOk)
			IPC_MESSAGE_HANDLER(View_Host_ReturnedKeyEvent, onReturnedInputEvent)
			IPC_MESSAGE_HANDLER(ViewHost_PrepareAddWindow, onPrepareAddWindow)
			IPC_MESSAGE_HANDLER(ViewHost_PrepareAddWindowWithMetaData, onPrepareAddWindowWithMetaData)
			IPC_MESSAGE_HANDLER(ViewHost_PasteToActiveWindow, onPasteToActiveWindow)
			IPC_MESSAGE_HANDLER(ViewHost_BootupFinished, onBootupFinished)
			IPC_MESSAGE_HANDLER(ViewHost_NewContentEvent, onNewContentEvent)
			IPC_MESSAGE_HANDLER(ViewHost_BannerMessageEvent, onBannerMessageEvent)
			IPC_MESSAGE_HANDLER(ViewHost_ActiveCallBannerEvent, onActiveCallBannerEvent)
			IPC_MESSAGE_HANDLER(ViewHost_EnableDockMode, onEnableDockMode)
			IPC_MESSAGE_HANDLER(ViewHost_CancelVibrations, onCancelVibrations)
			IPC_MESSAGE_HANDLER(ViewHost_ApplyLaunchFeedback, onApplyLaunchFeedback)
			IPC_MESSAGE_HANDLER(ViewHost_ListOfRunningAppsResponse, onListOfRunningAppsResponse)
			IPC_MESSAGE_HANDLER(ViewHost_AppLaunchPreventedUnderLowMemory, onAppLaunchPreventedUnderLowMemory)
			IPC_MESSAGE_HANDLER(ViewHost_LowMemoryActionsRequested, onLowMemoryActionsRequested)
			IPC_MESSAGE_HANDLER(ViewHost_ModalDismissedAtPreCreate, onModalDismissPreCreate)
			// message not handled, forward it to the base class
			IPC_MESSAGE_UNHANDLED( IpcClientHost::onMessageReceived(msg); )
		IPC_END_MESSAGE_MAP()
	}
}

void WebAppMgrProxy::onReturnedInputEvent(const SysMgrKeyEvent& event)
{
	Q_EMIT signalKeyEventRejected(event);
}

void WebAppMgrProxy::onPrepareAddWindow(int key, int type, int width, int height)
{
	bool hasAlpha = false;
	switch (static_cast<Window::Type>(type)) {
		case (Window::Type_Launcher):
		case (Window::Type_Menu):
		case (Window::Type_Dashboard):
			hasAlpha = true;
			break;

		default:
			break;
	}

	HostWindowData* data = HostWindowDataFactory::generate(key, -1, width, height, hasAlpha);
	if (!data || !data->isValid()) {
		g_critical("%s (%d): Failed to generate HostWindowData for key: %d\n",
				   __PRETTY_FUNCTION__, __LINE__, key);
		delete data;
		return;
	}

	Window* win = createWindowForWebApp(static_cast<Window::Type>(type), data);

	m_winMap[key] = win;
	m_winSet.insert(win);

	g_message("%s (%d): Attached to key: %d, width: %d, height: %d, window: %p",
	          __PRETTY_FUNCTION__, __LINE__, key, width, height, win);
	
	WindowServer::instance()->prepareAddWindow(win);
}

void WebAppMgrProxy::onPrepareAddWindowWithMetaData(int key, int metaDataKey, int type, int width, int height)
{
	bool hasAlpha = false;
	switch (static_cast<Window::Type>(type)) {
		case (Window::Type_Launcher):
		case (Window::Type_Menu):
		case (Window::Type_Dashboard):
			hasAlpha = true;
			break;

		default:
			break;
	}

	HostWindowData* data = HostWindowDataFactory::generate(key, metaDataKey, width, height, hasAlpha);
	if (!data || !data->isValid()) {
		g_critical("%s (%d): Failed to generate HostWindowData for key: %d\n",
				   __PRETTY_FUNCTION__, __LINE__, key);
		delete data;
		return;
	}

	Window* win = createWindowForWebApp(static_cast<Window::Type>(type), data);

	m_winMap[key] = win;
	m_winSet.insert(win);

	g_message("%s (%d): Attached to key: %d, width: %d, height: %d, window: %p",
	          __PRETTY_FUNCTION__, __LINE__, key, width, height, win);
	
	WindowServer::instance()->prepareAddWindow(win);    
}

void WebAppMgrProxy::onPasteToActiveWindow()
{
	if( Window* w = SystemUiController::instance()->activeWindow() )
		emitPaste( w );
}

void WebAppMgrProxy::onBootupFinished()
{
	WindowServer::instance()->bootupFinished();

	// Kick off the SysMgr MemoryMonitor timer
	MemoryMonitor::instance()->start();
}

void WebAppMgrProxy::onNewContentEvent(const NewContentIndicatorEventWrapper &wrapper)
{
	switch (wrapper.event->eventType) {
	case (NewContentIndicatorEvent::Add): {
		CoreNaviManager::instance()->addStandbyRequest(wrapper.event->appId, wrapper.event->requestId);
		break;
	}
	case (NewContentIndicatorEvent::Remove): {
		CoreNaviManager::instance()->removeStandbyRequest(wrapper.event->appId, wrapper.event->requestId);
		break;
	}
	default:
		break;
	}
}

void WebAppMgrProxy::onModalDismissPreCreate(int errorCode)
{
	SystemUiController::instance()->setModalWindowLaunchErrReason((SystemUiController::ModalWinLaunchErrorReason)errorCode);
	SystemUiController::instance()->notifyModalWindowDeactivated();
}

void WebAppMgrProxy::onBannerMessageEvent(const BannerMessageEventWrapper &wrapper)
{
	BannerMessageHandler::instance()->handleBannerMessageEvent(wrapper.event);
}

void WebAppMgrProxy::onActiveCallBannerEvent(const ActiveCallBannerEventWrapper &wrapper)
{
	BannerMessageHandler::instance()->handleActiveCallBannerEvent(wrapper.event);
}

void WebAppMgrProxy::onEnableDockMode(bool enable)
{
	SystemUiController::instance()->enterOrExitDockModeUi(enable);
}

void WebAppMgrProxy::onCancelVibrations()
{
	WindowServer::instance()->cancelVibrations();
}

void WebAppMgrProxy::onApplyLaunchFeedback(int cx, int cy)
{
	WindowServer::instance()->applyLaunchFeedback(cx, cy);
}

void WebAppMgrProxy::onAppLaunchPreventedUnderLowMemory()
{
    Q_EMIT signalAppLaunchPreventedUnderLowMemory();
}

void WebAppMgrProxy::onListOfRunningAppsResponse(const std::string& runnigAppsJsonArray)
{
	// deliver the response data to teh AppManager service
	appManagerCallback_listRunningApps(runnigAppsJsonArray);
}

void WebAppMgrProxy::onLowMemoryActionsRequested(bool allowExpensive)
{
	// the Memory Watcher (part of the WebKit process) has triggered Low Memory recovery actions 
	// (as the result of a low memory situation), so here in the SysMgr process side we should also
	// try to release memory.
	g_debug("%s: Performing Low Memory actions on the SysMgr main process. AllowExpensive = %d", __PRETTY_FUNCTION__, allowExpensive);
	
	Q_EMIT signalLowMemoryActionsRequested(allowExpensive);
}

Window* WebAppMgrProxy::createWindowForWebApp(Window::Type winType, HostWindowData* data)
{
	Window* win = 0;

	switch (winType) {
		case (Window::Type_Launcher): {
			win = new HostWindow(winType, data, this);
			break;
		}
		case (Window::Type_Menu): {
			win = new MenuWindow(data, this);
			break;
		}
		case (Window::Type_ModalChildWindowCard):
		case (Window::Type_Card):
		case (Window::Type_PIN):
		case (Window::Type_Emergency):
		case (Window::Type_ChildCard): {
		 	win = new CardWindow(winType, data, this);
		 	break;
		}
		case (Window::Type_DockModeWindow): {
		 	win = new DockModeWindow(winType, data, this);
		 	break;
		}
		case (Window::Type_PopupAlert):
		case (Window::Type_BannerAlert): {
			win = new AlertWindow(winType, data, this);
			break;
		}
		case (Window::Type_Dashboard): {
			win = new DashboardWindow(data, this);
			break;
		}
		default: {
			win = new HostWindow(winType, data, this);
			break;
		}
	}
	return win;
}

void WebAppMgrProxy::closeWindow(Window* w)
{
	for (WindowMap::const_iterator it = m_winMap.begin(); it != m_winMap.end(); ++it) {
		if (it->second == w) {
			sendAsyncMessage(new View_Close(it->first, w->disableKeepAlive()));
			m_closedWinSet.insert(w);
			break;
		}
	}
}

void WebAppMgrProxy::launchUrl(const char* url, Window::Type winType,
                               const char* appDesc, const char* procId,
                               const char* params, const char* launchingAppId,
                               const char* launchingProcId)
{
	sendAsyncMessage(new View_Mgr_LaunchUrl(url, winType, appDesc, procId, params,
			                                           launchingAppId, launchingProcId));
}

std::string WebAppMgrProxy::appLaunch(const std::string& appId,
                                      const std::string& params,
                                      const std::string& launchingAppId,
                                      const std::string& launchingProcId,
                                      std::string& errMsg)
{
	std::string appIdToLaunch = appId;
	std::string paramsToLaunch = params;
	errMsg.erase();

	ApplicationDescription* desc = ApplicationManager::instance()->getPendingAppById(appIdToLaunch);
	if (!desc)
		desc = ApplicationManager::instance()->getAppById(appIdToLaunch);

	if( !desc )
	{
		errMsg = std::string("\"") + appIdToLaunch + "\" was not found";
		g_debug( "WebAppMgrProxy::appLaunch failed, %s.\n", errMsg.c_str() );
		return "";
	}

	if( appIdToLaunch.empty() ) {
		errMsg = "No appId";
		return "";
	}

	//if execution lock is in place, refuse the launch
	if (desc->canExecute() == false) {
		errMsg = std::string("\"") + appIdToLaunch + "\" has been locked";
		luna_warn("WebAppMgrProxy","appLaunch failed, '%s' has been locked (probably in process of being deleted from the system)",appId.c_str());
		return "";
	}

	// redirect all launch requests for pending applications to app catalog, UNLESS it's a special SUC app
	//TODO: the sucApps thing is done : App Catalog disabled when SUC update is downloading; it's not safe, but it's needed

	if ( (desc->status() != ApplicationDescription::Status_Ready)
		&& (Settings::LunaSettings()->sucApps.find(desc->id()) == Settings::LunaSettings()->sucApps.end())
		)
	{
		desc = ApplicationManager::instance()->getAppById("com.palm.app.swmanager");
		if (desc) {
			appIdToLaunch = desc->id();
			paramsToLaunch = "{}";
		}
		else {
			g_warning("%s: Failed to find app descriptor for com.palm.app.swmanager", __PRETTY_FUNCTION__);
			return "";
		}
	}

	if (desc->type() == ApplicationDescription::Type_Web) {
        // Verify that the app doesn't have a security issue
        if (!desc->securityChecksVerified())
            return "";

        std::string appDescJson;

		desc->getAppDescriptionString(appDescJson);

		// Now forward the launch request to the Process Manager in the WebKit process
		sendAsyncMessage(new View_ProcMgr_Launch(appDescJson, paramsToLaunch, launchingAppId, launchingProcId));

		// FIXME: $$$ Can't get the resulting process ID at this point (asynchronous call)
		return "success";
	} else if (desc->type() == ApplicationDescription::Type_Native ||
			   desc->type() == ApplicationDescription::Type_PDK ||
               desc->type() == ApplicationDescription::Type_Qt) {
        // Verify that the app doesn't have a security issue
        if (!desc->securityChecksVerified())
            return "";
		// Launch Native apps here
		return launchNativeApp(desc, paramsToLaunch, launchingAppId, launchingProcId, errMsg);
	} else if (desc->type() == ApplicationDescription::Type_SysmgrBuiltin)
	{
		if (launchingAppId == "com.palm.launcher")
		{
			desc->startSysmgrBuiltIn(paramsToLaunch);
			return "success";
		}
	}

	g_warning("%s: Attempted to launch application with unknown type", __PRETTY_FUNCTION__);
	return "";
}

std::string WebAppMgrProxy::appLaunchModal(const std::string& appId,
                                      	   const std::string& params,
                                      	   const std::string& launchingAppId,
                                      	   const std::string& launchingProcId,
                                      	   std::string& errMsg, bool isHeadless, bool isParentPdk)
{
	std::string appIdToLaunch = appId;
	std::string paramsToLaunch = params;
	errMsg.erase();

	ApplicationDescription* desc = ApplicationManager::instance()->getPendingAppById(appIdToLaunch);
	if (!desc)
		desc = ApplicationManager::instance()->getAppById(appIdToLaunch);

	if( !desc )
	{
		errMsg = std::string("\"") + appIdToLaunch + "\" was not found";
		g_debug( "WebAppMgrProxy::appLaunch failed, %s.\n", errMsg.c_str() );
		return "";
	}

	if( appIdToLaunch.empty() ) {
		errMsg = "No appId";
		return "";
	}

	//if execution lock is in place, refuse the launch
	if (desc->canExecute() == false) {
		errMsg = std::string("\"") + appIdToLaunch + "\" has been locked";
		luna_warn("WebAppMgrProxy","appLaunch failed, '%s' has been locked (probably in process of being deleted from the system)",appId.c_str());
		return "";
	}

	// redirect all launch requests for pending applications to app catalog
	if (desc->status() != ApplicationDescription::Status_Ready) {
		desc = ApplicationManager::instance()->getAppById("com.palm.app.swmanager");
		if (desc) {
			appIdToLaunch = desc->id();
			paramsToLaunch = "{}";
		}
		else {
			g_warning("%s: Failed to find app descriptor for app catalog", __PRETTY_FUNCTION__);
			return "";
		}
	}

	if (desc->type() == ApplicationDescription::Type_Web) {
		std::string appDescJson;
		desc->getAppDescriptionString(appDescJson);
		sendAsyncMessage(new View_ProcMgr_LaunchChild(appDescJson, paramsToLaunch, launchingAppId, launchingProcId, isHeadless, isParentPdk));
		return "success";
	}

	g_warning("%s: Attempted to launch application with unknown type", __PRETTY_FUNCTION__);
	return "";
}

std::string WebAppMgrProxy::launchNativeApp(const ApplicationDescription* desc,
                                            const std::string& params,
                                            const std::string& launchingAppId,
                                            const std::string& launchingProcId,
                                            std::string& errMsg )
{
	std::string ret;
	// construct the path for launching
	std::string path = desc->entryPoint();
	if (path.find("file://", 0) != std::string::npos)
		path = path.substr(7);

	// assemble the args list. We'll need exactly 3 entries.
	// 1) the path to the exe
	// 2) The sent in "params" value
	// 3) a NULL to terminate the list

	// this has to be a char *, not const char *, because of the 
	// rather unusual use of "char *const argV[]" on the recieving end
	// of the function we're going to call. So we declare it appropriate
	// for the call, and cast as we assign.
	char *argV[3];
	argV[0] = (char *)path.c_str();
	
	if ( params.size() > 0 )
	{
		// send the params
		argV[1] = (char *)params.c_str();
		argV[2] = NULL;
	}
	else
	{
		// no params. Just end the list
		argV[1] = NULL;
	}
	
	int pid = IpcServer::instance()->launchNativeProcess(desc->id(), path.c_str(), argV, desc->type(), desc->runtimeMemoryRequired());
	if (pid <= 0) {
		if (pid < 0) // 0 indicates low memory, -1 indicates launch error
		{
			g_critical("%s: %d Failed to launch native app %s with path: %s",
			           __PRETTY_FUNCTION__, __LINE__,
			           desc->id().c_str(), path.c_str());
			errMsg = "Failed to launch process";
		}
		return std::string();
	}

	char* retStr = 0;
	asprintf(&retStr, "n-%d", pid);

	ret = retStr;
	free(retStr);

	return ret;
}


std::string WebAppMgrProxy::launchBootTimeApp(const char* appId)
{
	if (!appId)
		return "";

	ApplicationDescription* desc = ApplicationManager::instance()->getAppById(appId);
	if (!desc) {
		luna_warn("WebAppMgrProxy", "launch failed, '%s' was not found.", appId);
		return "";
	}

	//if execution lock is in place, refuse the launch
	if (desc->canExecute() == false) {
		luna_warn("WebAppMgrProxy","launch failed, '%s' has been locked (probably in process of being deleted from the system)",appId);
		return "";
	}

	std::string appDescJson;
	desc->getAppDescriptionString(appDescJson);

	sendAsyncMessage(new View_ProcMgr_LaunchBootTimeApp(appDescJson));

	// FIXME: $$$ Can't get the resulting process ID at this point (asynchronous call)
	return "success";
}

void WebAppMgrProxy::inputEvent(Window* win, Event* e)
{
    if (G_UNLIKELY(!win || !win->isIpcWindow())) {
		g_critical("%s (%d): Invoked for a non-IPC Window.",
		           __PRETTY_FUNCTION__, __LINE__);
		return;
	}

    if (EventThrottler::instance()->shouldDropEvent(e))
	    return;

	sendAsyncMessage(new View_InputEvent(static_cast<HostWindow*>(win)->routingId(),
										 SysMgrEventWrapper(e)));
}

void WebAppMgrProxy::inputQKeyEvent(Window* win, QKeyEvent* event)
{
    if(!win || !win->isIpcWindow()) {
		g_critical("%s (%d): Invoked for a non-IPC Window.",
		           __PRETTY_FUNCTION__, __LINE__);
		return;
	}

	if(event) {
		if (event->modifiers() & Qt::MetaModifier) {
			g_message("%s: MetaModifier is set", __PRETTY_FUNCTION__);
		}	
		sendAsyncMessage(new View_KeyEvent(static_cast<HostWindow*>(win)->routingId(), event));
	}
}

void WebAppMgrProxy::focusEvent(Window* win, bool focused)
{
    if(!win || !win->isIpcWindow()) {
		g_critical("%s (%d): Invoked for a non-IPC Window.",
		           __PRETTY_FUNCTION__, __LINE__);
		return;
	}
	sendAsyncMessage(new View_Focus(static_cast<HostWindow*>(win)->routingId(), focused));
}

void WebAppMgrProxy::uiDimensionsChanged(int width, int height)
{
	sendAsyncMessage(new View_Mgr_UiDimensionsChanged(width, height));
}

void WebAppMgrProxy::resizeEvent(Window* win, int newWidth, int newHeight, bool resizeBufer)
{
    if(!win || !win->isIpcWindow()) {
		g_critical("%s (%d): Invoked for a non-IPC Window.",
		           __PRETTY_FUNCTION__, __LINE__);
		return;
	}
	sendAsyncMessage(new View_Resize(static_cast<HostWindow*>(win)->routingId(), newWidth, newHeight, resizeBufer));
}

void WebAppMgrProxy::setOrientation(OrientationEvent::Orientation orient)
{
	m_orientation = orient;
	sendAsyncMessage(new View_Mgr_SetOrientation(static_cast<int>(orient)));
}

void WebAppMgrProxy::setGlobalProperties(int key)
{
    sendAsyncMessage(new View_Mgr_GlobalProperties(key));
}

void WebAppMgrProxy::inspect( const char* procId )
{
	sendAsyncMessage(new View_Mgr_InspectByProcessId(procId));
}

void WebAppMgrProxy::clearWebkitCache()
{
	sendAsyncMessage(new View_Mgr_ClearWebkitCache());
}

void WebAppMgrProxy::setJavascriptFlags( const char* flags )
{
	sendAsyncMessage(new View_Mgr_SetJavascriptFlags(flags));
}

void WebAppMgrProxy::enableDebugger( bool enable )
{
	sendAsyncMessage(new View_Mgr_EnableDebugger(enable));
}

void WebAppMgrProxy::performLowMemoryActions( const bool allowExpensive )
{
	sendAsyncMessage(new View_Mgr_PerformLowMemoryActions(allowExpensive));
}

void WebAppMgrProxy::postShutdownEvent()
{
	sendAsyncMessage(new View_Mgr_ShutdownEvent());
}

void WebAppMgrProxy::emitCopy( Window *win )
{
    if(!win || !win->isIpcWindow()) {
		g_critical("%s (%d): Invoked for a non-IPC Window.",
		           __PRETTY_FUNCTION__, __LINE__);
		return;
	}
	sendAsyncMessage(new View_ClipboardEvent_Copy(static_cast<HostWindow*>(win)->routingId()));
}

void WebAppMgrProxy::emitCut( Window *win )
{
    if(!win || !win->isIpcWindow()) {
		g_critical("%s (%d): Invoked for a non-IPC Window.",
		           __PRETTY_FUNCTION__, __LINE__);
		return;
	}
	sendAsyncMessage(new View_ClipboardEvent_Cut(static_cast<HostWindow*>(win)->routingId()));
}

void WebAppMgrProxy::emitPaste( Window *win )
{
    if(!win || !win->isIpcWindow()) {
		g_critical("%s (%d): Invoked for a non-IPC Window.",
		           __PRETTY_FUNCTION__, __LINE__);
		return;
	}
	sendAsyncMessage(new View_ClipboardEvent_Paste(static_cast<HostWindow*>(win)->routingId()));
}

void WebAppMgrProxy::emitSelectAll( Window *win )
{
    if(!win || !win->isIpcWindow()) {
		g_critical("%s (%d): Invoked for a non-IPC Window.",
		           __PRETTY_FUNCTION__, __LINE__);
		return;
	}
	sendAsyncMessage(new View_SelectAll(static_cast<HostWindow*>(win)->routingId()));
}

void WebAppMgrProxy::notifyCompassEnabled(bool enabled)
{
	//sendAsyncMessage(new View_Mgr_NotifyCompassEnabled(enabled));
}

void WebAppMgrProxy::serviceRequestHandler_listRunningApps(bool includeSysApps)
{
	sendAsyncMessage(new View_Mgr_ListOfRunningAppsRequest(includeSysApps));
}
