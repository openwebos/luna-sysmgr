/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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
#include "IpcClientHost.h"

#include <PIpcChannel.h>
#include <PIpcBuffer.h>
#include <cjson/json.h>
#include <sys/time.h>
#include <sys/resource.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "CardHostWindow.h"
#include "QtHostWindow.h"
#include "HostWindow.h"
#include "IpcServer.h"
#include "SystemUiController.h"
#include "WindowServer.h"
#include "WebAppMgrProxy.h"

static const int kPriorityActive = -1;
static const int kPriorityInactive = 1;

IpcClientHost::IpcClientHost()
	: m_pid(-1)
	, m_processPriority(0)
	, m_clearing(false)
	, m_idleDestroySrc(0)
{
}

IpcClientHost::IpcClientHost(int pid, const std::string& name, PIpcChannel* channel)
	: m_pid(pid)
	, m_processPriority(0)
	, m_name(name)
	, m_clearing(false)
	, m_idleDestroySrc(0)
{
    channel->setListener(this);

	connect(SystemUiController::instance(), SIGNAL(signalMaximizedCardWindowChanged(Window*)),
			this, SLOT(slotMaximizedCardWindowChanged(Window*)));
}

IpcClientHost::~IpcClientHost()
{
	IpcServer::instance()->ipcClientHostQuit(this);

	delete m_channel;
	m_channel = NULL;

	if (m_idleDestroySrc) {
		g_source_destroy(m_idleDestroySrc);
		g_source_unref(m_idleDestroySrc);
	}
}

void IpcClientHost::onMessageReceived(const PIpcMessage& msg)
{
	if (msg.routing_id() != MSG_ROUTING_CONTROL) {
		// ROUTED MESSAGE, forward it to the correct host window
		HostWindow* win = static_cast<HostWindow*>(findWindow(msg.routing_id()));
		if (!win) {
			return;
		}
		win->onMessageReceived(msg);
	} else {
		// CONTROL MESSAGE, Handle it here
		bool msgIsOk;

		IPC_BEGIN_MESSAGE_MAP(IpcClientHost, msg, msgIsOk)
			IPC_MESSAGE_HANDLER(View_Host_ReturnedKeyEvent, onReturnedInputEvent)
			IPC_MESSAGE_HANDLER(ViewHost_PrepareAddWindow, onPrepareAddWindow)
			IPC_MESSAGE_HANDLER(ViewHost_AddWindow, onAddWindow)
			IPC_MESSAGE_HANDLER(ViewHost_RemoveWindow, onRemoveWindow)
			IPC_MESSAGE_HANDLER(ViewHost_SetWindowProperties, onSetWindowProperties)
			IPC_MESSAGE_HANDLER(ViewHost_FocusWindow, onFocusWindow)
			IPC_MESSAGE_HANDLER(ViewHost_UnfocusWindow, onUnfocusWindow)
			IPC_MESSAGE_UNHANDLED( g_critical("%s (%d). IPC Message Not Handled: (0x%x : 0x%x : 0x%x)",
											  __PRETTY_FUNCTION__, __LINE__, msg.routing_id(), msg.type(), msg.message_id()));
		IPC_END_MESSAGE_MAP()
	}
}

void IpcClientHost::onReturnedInputEvent(const SysMgrKeyEvent& event)
{
	WebAppMgrProxy *proxy = WebAppMgrProxy::instance();
	if ( proxy == NULL )
	{
		// what the...?
		return;
	}
	
	// pass the rejected event along
	proxy->onReturnedInputEvent(event);
}


void IpcClientHost::onDisconnected()
{
	g_message("%s (%d): Disconnected", __PRETTY_FUNCTION__, __LINE__);

	m_clearing = true;

	for (WindowMap::const_iterator it = m_winMap.begin(); it != m_winMap.end(); ++it) {
		static_cast<HostWindow*>(it->second)->channelRemoved();
		static_cast<HostWindow*>(it->second)->setClientHost(0);
		WindowServer::instance()->removeWindow(it->second);
	}

	m_winMap.clear();
	m_winSet.clear();
	m_closedWinSet.clear();

	// http://bugreports.qt.nokia.com/browse/QTBUG-18434: deleteLater does
	// not work when used in a g_idle_dispatch
	//deleteLater();
	if (!m_idleDestroySrc) {
		GSource* src = g_idle_source_new();
		g_source_set_callback(src, (GSourceFunc) idleDestroyCallback, this, NULL);
		g_source_attach(src, g_main_loop_get_context(IpcServer::instance()->mainLoop()));
		m_idleDestroySrc = src;
	}
}

void IpcClientHost::onPrepareAddWindow(int key, int type, int width, int height)
{
    bool isNativeQtWindow = (type == (1 << 3));
	// FIXME: Magic numbers
	// 1 << 1: Card
	// 1 << 2: Emergency
	switch (type) {
	case 1 << 2:
		type = Window::Type_Emergency;
		break;
	case 1 << 1:
	default:
		type = Window::Type_Card;
		break;
	}

	HostWindowData* data = HostWindowDataFactory::generate(key, -1, width, height, false);
	if (!data || !data->isValid()) {
		g_critical("%s (%d): Failed to generate HostWindowData for key: %d\n",
				   __PRETTY_FUNCTION__, __LINE__, key);
		delete data;
		return;
	}

	CardHostWindow* win = 0;
    if (isNativeQtWindow)
        win = new QtHostWindow(static_cast<Window::Type>(type), data, this);
    else
        win = new CardHostWindow(static_cast<Window::Type>(type), data, this);

	// Set the appId to be the name.
	if(0 < m_name.length()) {
		win->setAppId(m_name);
	}

	m_winMap[key] = win;
	m_winSet.insert(win);

	g_message("%s (%d): Attached to key: %d, width: %d, height: %d, Window: %p",
	          __PRETTY_FUNCTION__, __LINE__, key, width, height, win);
	
	WindowServer::instance()->prepareAddWindow(win);
}

void IpcClientHost::onAddWindow(int key)
{
	Window* win = findWindow(key);
	if (!win)
		return;

	WindowServer::instance()->addWindow(win);
}

void IpcClientHost::onRemoveWindow(int key)
{
	Window* win = findWindow(key);
	if (!win)
		return;

	g_message("%s (%d): Remove Window with key:%d, window: %p",
	          __PRETTY_FUNCTION__, __LINE__, key, win);
	
	m_winMap.erase(key);
	m_winSet.erase(win);
	m_closedWinSet.erase(win);
	WindowServer::instance()->removeWindow(win);
}

void IpcClientHost::onSetWindowProperties(int key, const std::string& winPropsStr)
{
	Window* win = findWindow(key);
	if (!win)
		return;

	if (winPropsStr.empty())
		return;

	json_object* json = 0;
	json_object* label = 0;
	WindowProperties winProps;

	json = json_tokener_parse(winPropsStr.c_str());
	if (!json || is_error(json))
		return;

	label = json_object_object_get(json, "blockScreenTimeout");
	if (label && json_object_is_type(label, json_type_boolean))
		winProps.setBlockScreenTimeout(json_object_get_boolean(label));

	label = json_object_object_get(json, "subtleLightbar");
	if (label && json_object_is_type(label, json_type_boolean))
		winProps.setSubtleLightbar(json_object_get_boolean(label));

	label = json_object_object_get(json, "fullScreen");
	if (label && json_object_is_type(label, json_type_boolean))
		winProps.setFullScreen(json_object_get_boolean(label));

	label = json_object_object_get(json, "activeTouchpanel");
	if (label && json_object_is_type(label, json_type_boolean))
		winProps.setActiveTouchpanel(json_object_get_boolean(label));

	label = json_object_object_get(json, "alsDisabled");
	if (label && json_object_is_type(label, json_type_boolean))
		winProps.setAlsDisabled(json_object_get_boolean(label));

	label = json_object_object_get(json, "enableCompass");
		if (label && json_object_is_type(label, json_type_boolean))
			winProps.setCompassEvents(json_object_get_boolean(label));

	label = json_object_object_get(json, "enableGyro");
			if (label && json_object_is_type(label, json_type_boolean))
				winProps.setAllowGyroEvents(json_object_get_boolean(label));

	label = json_object_object_get(json, "overlayNotificationsPosition");
	if (label && json_object_is_type(label, json_type_string)) {
		const char* str = json_object_get_string(label);
		if (str) {
			if (strcasecmp(str, "left") == 0)
				winProps.setOverlayNotificationsPosition(WindowProperties::OverlayNotificationsLeft);
			else if (strcasecmp(str, "right") == 0)
				winProps.setOverlayNotificationsPosition(WindowProperties::OverlayNotificationsRight);
			else if (strcasecmp(str, "top") == 0)
				winProps.setOverlayNotificationsPosition(WindowProperties::OverlayNotificationsTop);
			else
				winProps.setOverlayNotificationsPosition(WindowProperties::OverlayNotificationsBottom);
		}
	}

	label = json_object_object_get(json, "suppressBannerMessages");
	if (label && json_object_is_type(label, json_type_boolean))
		winProps.setSuppressBannerMessages(json_object_get_boolean(label));

	label = json_object_object_get(json, "hasPauseUi");
	if (label && json_object_is_type(label, json_type_boolean))
		winProps.setHasPauseUi(json_object_get_boolean(label));

	label = json_object_object_get(json, "suppressGestures");
	if (label && json_object_is_type(label, json_type_boolean))
		winProps.setSuppressGestures(json_object_get_boolean(label));

	label = json_object_object_get(json, "webosDragMode");
	if (label && json_object_is_type(label, json_type_boolean)) {
		winProps.setDashboardManualDragMode(json_object_get_boolean(label));
	}

	label = json_object_object_get(json, "statusBarColor");
	if (label && json_object_is_type(label, json_type_int))
		winProps.setStatusBarColor(json_object_get_int(label));

	label = json_object_object_get(json, "rotationLockMaximized");
	if (label && json_object_is_type(label, json_type_boolean)) {
		winProps.setRotationLockMaximized(json_object_get_boolean(label));
	}

	label = json_object_object_get(json, "allowResizeOnPositiveSpaceChange");
	if (label && json_object_is_type(label, json_type_boolean))
		winProps.setAllowResizeOnPositiveSpaceChange(json_object_get_boolean(label));

	/*label = json_object_object_get(json, "appId");
	if (label && json_object_is_type(label, json_type_string))
	{
		const char* str = json_object_get_string(label);
		if (str)
		{
			// only CardHostWindows get this property set
			CardHostWindow *chw = (CardHostWindow *)win;

			// tell ModalManager all about it
			//ModalManager::instance()->registerCard(chw, str);
		}
	}*/

	json_object_put(json);

	WindowServer::instance()->setWindowProperties(win, winProps);
}

void IpcClientHost::onFocusWindow(int key)
{
	Window* win = findWindow(key);
	if (!win)
		return;

	WindowServer::instance()->focusWindow(win);
}

void IpcClientHost::onUnfocusWindow(int key)
{
	Window* win = findWindow(key);
	if (!win)
		return;

	WindowServer::instance()->unfocusWindow(win);
}

Window* IpcClientHost::findWindow(int key) const
{
	WindowMap::const_iterator it = m_winMap.find(key);
	if (it == m_winMap.end()) {
        g_warning("%s (%d): Failed to find window with key: %d",
				   __PRETTY_FUNCTION__, __LINE__, key);
		return 0;
	}

	return it->second;
}

void IpcClientHost::slotMaximizedCardWindowChanged(Window* w)
{
	bool thisIsMyWindow = false;

	if (w) {
		if (m_winSet.find(w) != m_winSet.end())
			thisIsMyWindow = true;
	}

	if (thisIsMyWindow) {
		if (m_processPriority != kPriorityActive) {
			m_processPriority = kPriorityActive;
			::setpriority(PRIO_PROCESS, m_pid,  m_processPriority);
			g_debug("%s: setting priority of %d to %d", __PRETTY_FUNCTION__, m_pid, m_processPriority);
		}
	}
    else {
		if (m_processPriority != kPriorityInactive) {
			m_processPriority = kPriorityInactive;
			::setpriority(PRIO_PROCESS, m_pid, m_processPriority);
			g_debug("%s: setting priority of %d to %d", __PRETTY_FUNCTION__, m_pid, m_processPriority);
		}
	}
}

void IpcClientHost::closeWindow(Window* w)
{
	for (WindowMap::const_iterator it = m_winMap.begin();
		 it != m_winMap.end(); ++it) {
		if (it->second == w) {
			m_channel->sendAsyncMessage(new View_Close(it->first, w->disableKeepAlive()));
			m_closedWinSet.insert(w);
			break;
		}
    }

	// FIXME: the nuking decision should not be inferred based on window type
	if ((w->type() != Window::Type_Emergency) && (m_closedWinSet == m_winSet)) {
		// All windows closed. Its ok to ask this process to be nuked
		IpcServer::instance()->addProcessToNukeList(m_pid);
	}
}

void IpcClientHost::relaunch()
{
	// Find the first window and maximize it

	WindowMap::const_iterator it = m_winMap.begin();
	if (it == m_winMap.end())
		return;

	Window* w = it->second;
	if (!w)
		return;

	WindowServer::instance()->focusWindow(w);
}

void IpcClientHost::replaceWindowKey(Window* win, int oldKey, int newKey)
{
	g_message("%s:%d window: %p, old: %d, new: %d", __PRETTY_FUNCTION__, __LINE__,
			  win, oldKey, newKey);
	m_winMap.erase(oldKey);
	m_winMap[newKey] = win;
}

void IpcClientHost::windowDeleted(Window* w)
{
	if (m_clearing)
		return;

	if (m_winSet.find(w) == m_winSet.end())
		return;

	g_message("%s (%d): window: %p",
	          __PRETTY_FUNCTION__, __LINE__, w);
	
	m_winSet.erase(w);

	for (WindowMap::iterator it = m_winMap.begin(); it != m_winMap.end(); ++it) {
		if (it->second == w) {
			m_winMap.erase(it);
			break;
		}
	}
}

gboolean IpcClientHost::idleDestroyCallback(gpointer arg)
{
	IpcClientHost* d = (IpcClientHost*) arg;
	delete d;

    return FALSE;
}
