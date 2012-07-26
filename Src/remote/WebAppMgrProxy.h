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




#ifndef WEBAPPMGRPROXY_H
#define WEBAPPMGRPROXY_H

#include "Common.h"

#include <PIpcChannelListener.h>
#include <queue>
#include "IpcClientHost.h"
#include "Event.h"
#include "CustomEvents.h"
#include <NewContentIndicatorEvent.h>
#include <BannerMessageEvent.h>
#include "ActiveCallBannerEvent.h"

class PIpcChannel;
class PIpcBuffer;
class PIpcMessage;
class SysMgrKeyEvent;
class HostWindowData;

class WebAppMgrProxy : public IpcClientHost
{
	Q_OBJECT
	
public:
	static void setAppToLaunchUponConnection(char* app);
	static WebAppMgrProxy* connectWebAppMgr(int pid, PIpcChannel* channel);

	static WebAppMgrProxy* instance();
	
	bool connected();
	void onDisconnected();

	virtual ~WebAppMgrProxy();
	
	void sendAsyncMessage(PIpcMessage* msg);
	PIpcChannel* getIpcChannel() const;
	void sendQueuedMessages();
	
	void closeWindow(Window* w);
	void launchUrl(const char* url, Window::Type winType=Window::Type_Card,
			       const char* appDesc="", const char* procId="",
			       const char* params="", const char* launchingAppId="",
			       const char* launchingProcId="");
	void relaunchApp(const char* procId, const char* params, const char* launchingAppId,
					 const char* launchingProcId);
	
	std::string appLaunch(const std::string& appId,
			       		  const std::string& params,
			       		  const std::string& launchingAppId,
			       		  const std::string& launchingProcId,
			       		  std::string& errMsg);
	
	std::string appLaunchModal(const std::string& appId,
				       	  	   const std::string& params,
				       	  	   const std::string& launchingAppId,
				       	  	   const std::string& launchingProcId,
				       	  	   std::string& errMsg,
				       	  	   bool isHeadless,
				       	  	   bool isParentTypePdk);

	std::string	launchBootTimeApp(const char* appId);
	
	std::string launchNativeApp(const ApplicationDescription* desc,
								const std::string& params,
								const std::string& launchingAppId,
								const std::string& launchingProcId,
								std::string& errMsg );

	void inputEvent(Window* win, Event* e);
	void inputQKeyEvent(Window* win, QKeyEvent* event);
	void focusEvent(Window* win, bool focused);
	void resizeEvent(Window* win, int newWidth, int newHeight, bool resizeBuffer=false);
	void uiDimensionsChanged(int width, int height);
	
	void setOrientation(OrientationEvent::Orientation orient);
	OrientationEvent::Orientation orientation() const { return m_orientation; }

	void setGlobalProperties(int key);

	void inspect(const char* procId);
	void clearWebkitCache();
	void setJavascriptFlags(const char* flags);
	void enableDebugger(bool enable);
	void performLowMemoryActions(const bool allowExpensive = false);
	void postShutdownEvent();
	void notifyCompassEnabled(bool enabled);
	void serviceRequestHandler_listRunningApps(bool includeSysApps);
	
	void emitCopy(Window *win);
	void emitCut(Window *win);
	void emitPaste(Window *win);
	void emitSelectAll(Window *win);
	
	void suspendWebKitProcess();
	void resumeWebKitProcess();
	
    // publicizing this because I need to call it from IpcClientHost
    void onReturnedInputEvent(const SysMgrKeyEvent& event);

Q_SIGNALS:

	void signalKeyEventRejected(const SysMgrKeyEvent& event);
	void signalAppLaunchPreventedUnderLowMemory();
	void signalLowMemoryActionsRequested (bool allowExpensive);	

private:

    virtual void onMessageReceived(const PIpcMessage& msg);

    void onPrepareAddWindow(int key, int type, int width, int height);
	void onPrepareAddWindowWithMetaData(int key, int metaDataKey, int type, int width, int height);
    void onPasteToActiveWindow();
    void onBootupFinished();
    void onNewContentEvent(const NewContentIndicatorEventWrapper &wrapper);
    void onBannerMessageEvent(const BannerMessageEventWrapper &wrapper);
	void onEnableDockMode(bool enable);
    void onCancelVibrations();
	void onActiveCallBannerEvent(const ActiveCallBannerEventWrapper &wrapper);
    void onAddPhoneActiveCallBanner(uint32_t ipcHandle,const std::string& iconFile,const std::string& message,uint32_t startTime);
    void onRemovePhoneActiveCallBanner(uint32_t ipcHandle);
    void onUpdatePhoneActiveCallBanner(uint32_t ipcHandle,const std::string& iconFile,const std::string& message,uint32_t resetTime);
    void onApplyLaunchFeedback(int cx, int cy);
    void onListOfRunningAppsResponse(const std::string& runnigAppsJsonArray);
	void onAppLaunchPreventedUnderLowMemory();
    void onLowMemoryActionsRequested(bool allowExpensive);
    void onModalDismissPreCreate(int errorCode);

	Window* createWindowForWebApp(Window::Type winType, HostWindowData* data);
	
	static void webKitDiedCallback(GPid pid, gint status, gpointer data);
	void webKitDied(GPid pid, gint status);

private:
	
	WebAppMgrProxy();
	void clientConnected(int pid, PIpcChannel* channel);

	WebAppMgrProxy(const WebAppMgrProxy&);
	WebAppMgrProxy& operator=(const WebAppMgrProxy&);
	
private:
	std::queue<PIpcMessage*> m_discMsgQueue; // for queueing outgoing messages while not connected
	OrientationEvent::Orientation m_orientation;
	
	PIpcBuffer* m_ipcImgDragBuffer;
	QPixmap*    m_dragPixmap;
};	


#endif /* WEBAPPMGRPROXY_H */
