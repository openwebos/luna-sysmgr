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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <cjson/json.h>

#include <string>
#include <algorithm>
#include <memory>
#include <palmwebglobal.h>
#include <palmwebpage.h>
#include <palmwebview.h>
#include <QUrl>

#include "ApplicationDescription.h"
#include "WebPage.h"
#include "WebPageClient.h"
#include "Settings.h"
#include "WebAppBase.h"
#include "WebAppFactory.h"
#include "WebAppManager.h"
#include "Logging.h"
#include "Utils.h"
#include "Event.h"
#include "Settings.h"
#include "Time.h"
#include "WebFrame.h"

#include "WindowedWebApp.h"

#include "Event.h"
#include "ProcessManager.h"
#include "CmdResourceHandlers.h"
#include "NewContentIndicatorEventFactory.h"

#include "WebKitSensorConnector.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include <openssl/blowfish.h>

//Needs a declaration so that it can be used throughout the file.
StringVariantMap parseStageArguments(const char* argString);


static const int kDefaultPageWidth = 0;
static const int kDefaultPageHeight = 0;

static const char* kLogChannel = "WebPage";

template <class T>
bool ValidJsonObject(T jsonObj)
{
	return (NULL != jsonObj) && (!is_error(jsonObj));		//-coverity-investigation
}


WebPage::WebPage(bool createView)
	: m_client(0)
	, m_webView(0)
	, m_webPage(0)
	, m_url(0)
	, m_waitingForUrl(true)
	, m_shuttingDown(false)
	, m_progress(0)
	, m_isEditorFocused(false)
	, m_isExplicitEditorFocused(false)
    , m_isManualEditorFocused(false)
    , m_manualFocusEnabled(false)
	, m_inRelaunch(false)
	, m_requestedWidth(-1)
	, m_requestedHeight(-1)
	, m_nestedLoop(0)
	, m_stageReadyPending(false)
	, m_parent(0)
	, m_needTouchEvents (false)
	, m_activiyId(-1)
{
	luna_log(kLogChannel, "creating page and waiting for url");

	::memset(&m_editorState, 0, sizeof(PalmIME::EditorState));
	m_editorState.type = PalmIME::FieldType_Text;

	::memset(&m_explicitEditorState, 0, sizeof(PalmIME::EditorState));
	m_explicitEditorState.type = PalmIME::FieldType_Text;

	::memset(&m_manualEditorState, 0, sizeof(PalmIME::EditorState));
	m_manualEditorState.type = PalmIME::FieldType_Text;

	m_lastAutoCap = false;

	if (createView) {
		m_webView = new Palm::WebView(this);
		m_webPage = m_webView->page();
	}
	else
		m_webPage = new Palm::WebPage();

	m_webPage->setClient(this);
	m_mainFrame = new WebFrame(this, m_webPage->mainFrame(), true);
}

WebPage::WebPage(const char* url, bool createView)
	: m_client(0)
	, m_webView(0)
	, m_webPage(0)
	, m_url(0)
	, m_waitingForUrl(false)
	, m_shuttingDown(false)
	, m_progress(0)
	, m_isEditorFocused(false)
	, m_isExplicitEditorFocused(false)
    , m_isManualEditorFocused(false)
    , m_manualFocusEnabled(false)
	, m_inRelaunch(false)
	, m_nestedLoop(0)
	, m_stageReadyPending(false)
	, m_parent(0)
	, m_needTouchEvents(false)
	, m_activiyId(-1)
{
	::memset(&m_editorState, 0, sizeof(PalmIME::EditorState));
	m_editorState.type = PalmIME::FieldType_Text;

	::memset(&m_explicitEditorState, 0, sizeof(PalmIME::EditorState));
	m_explicitEditorState.type = PalmIME::FieldType_Text;

	::memset(&m_manualEditorState, 0, sizeof(PalmIME::EditorState));
	m_manualEditorState.type = PalmIME::FieldType_Text;

	m_lastAutoCap = false;

	if (createView) {
		m_webView = new Palm::WebView(this);
		m_webPage = m_webView->page();
	}
	else
		m_webPage = new Palm::WebPage();

	m_webPage->setClient(this);
	m_mainFrame = new WebFrame(this, m_webPage->mainFrame(), true);
	
	if (url) {
	    m_url = strdup(url);
		urlQueryToStageArguments(m_url);
	}
	else
		m_waitingForUrl = true;
}

WebPage::~WebPage()
{
	m_shuttingDown = true;

	WebAppManager::instance()->webPageRemoved(this);

	if (m_parent)
		m_parent->removeChild(this);
	
	delete m_mainFrame;
	
	if (m_webView)
		delete m_webView;
	else if (m_webPage)
		delete m_webPage;

// 2009/11/11: WebKit team doesn't want us to call explicit GC
//	on page close
//	Palm::WebGlobal::garbageCollectNow();

	// NOTE: Palm::WebView or Palm::WebPage may trigger a method 
	// 	on this which might require m_client to still be valid.	
	m_client = 0;

	m_webView = 0;
	m_webPage = 0;
	
    if (m_url)
		free(m_url);

	for (std::set<WebPage*>::const_iterator it = m_children.begin();
		 it != m_children.end(); ++it) {
		(*it)->setParent(0);
	}

	m_children.clear();

	for (std::set<std::string>::iterator it = m_newContentRequestIds.begin();
		it != m_newContentRequestIds.end(); ++it) {
	    if (!(*it).empty()) {
			g_debug ("Removing new content indicator %s for app %s", appId().c_str(), (*it).c_str());
			NewContentIndicatorEvent* e = NewContentIndicatorEventFactory::createRemoveEvent(appId(), (*it).c_str());
			if (e)
				WebAppManager::instance()->sendAsyncMessage(new ViewHost_NewContentEvent(NewContentIndicatorEventWrapper(e)));
	    }
	}
	m_newContentRequestIds.clear();
}

void WebPage::setClient(::WebPageClient* client)
{
    m_client = client;
	if (!client)
		return;

	if (m_webView) {

		// Resize our window to match the client's window dimensions
		int width, height;
		client->windowSize(width, height);
		m_webView->resize(width, height);

		client->needTouchEvents(m_needTouchEvents);
	}
}

::WebPageClient* WebPage::client() const
{
    return m_client;
}

void WebPage::run()
{
	if (!m_url)
		return;
	
	luna_log(kLogChannel, "Opening page: %s", m_url);
	m_webPage->load(m_url);
}

void WebPage::setArgs(const char* args)
{
	if (args)
		m_args = args;
	else
		m_args = "";

	StringVariantMap stageArgs = parseStageArguments(args);
	m_stageArgs = stageArgs;

	m_mainFrame->setLaunchParams(m_args);
}

void WebPage::startDrag( int x, int y, int imgOffsetX, int imgOffsetY, void* imgRef, PalmClipboard* sysClipboard )
{
	// NOTE: Disabling drag and drop support which was only being used by the launcher.
	// 	All drag and drop is being done natively.
}

void WebPage::releaseNestedLoopIfNecessary()
{
	if( m_nestedLoop )
	{
		g_main_loop_quit(m_nestedLoop);
		g_main_loop_unref(m_nestedLoop);
		m_nestedLoop = 0;
	}
}

bool WebPage::relaunch(const char* args, const char* launchingAppId, const char* launchingProcId)
{
	if (m_shuttingDown)
		return false;
	
	// If the headless app is still loading, buffer this until our progress is complete.
	if( progress() < 100 )
	{
		m_bufferedRelaunchArgs = args;
		m_bufferedRelaunchLaunchingAppId = launchingAppId;
		m_bufferedRelaunchLaunchingProcId = launchingProcId;
		return true;
	}

//	printf("%s: launching appId: %s, launching processId: %s\n",
//		   __PRETTY_FUNCTION__, launchingAppId, launchingProcId);
	
	m_args = args ? args : "";
	setLaunchingAppId(launchingAppId ? launchingAppId : std::string());
	setLaunchingProcessId(launchingProcId ? launchingProcId : std::string());

	m_mainFrame->setLaunchParams(m_args);

	// Relaunching may cause a child page creation. We want to make sure
	// that the launching app id of the child page is set correctly

	m_inRelaunch = true;

	bool ret = m_webPage->evaluateScript("Mojo.relaunch()");

	m_inRelaunch = false;

	return ret;
}

void WebPage::getVirtualWindowSize(int& width, int& height)
{
    if (m_client) {
		m_client->windowSize(width, height);
		return;
	}

	width = 0;
	height = 0;
}

void WebPage::getWindowSize(int& width, int& height)
{
    if (m_client) {
		m_client->windowSize(width, height);
		return;
	}

	width = 0;
	height = 0;
}

void WebPage::getScreenSize(int& width, int& height)
{
    if (m_client) {
		m_client->screenSize(width, height);
		return;
	}

	width = 0;
	height = 0;
}

const char* WebPage::getUserAgent()
{
	// GTK WebKit
    // return "Mozilla/5.0 (X11; U; Linux i686; en-US) AppleWebKit/420+ (KHTML, like Gecko)";

    // iPhone
    //return "Mozilla/5.0 (iPhone; U; CPU like Mac OS X; en) AppleWebKit/420+ (KHTML, like Gecko) Version/3.0 Mobile/1A538a Safari/419";

    // Safari 3.0
    //return "Mozilla/5.0 (X; U; Intel Mac OS X; en) AppleWebKit/522.11.1 (KHTML, like Gecko) Version/3.0.3 Safari/522.12.1";

	
    // Palm Temporary
	// 90210 will become the OS name. (Nova is the current name)
	// 
    return "Mozilla/5.0 (90210/1.0; U; en-US) AppleWebKit/525.13 (KHTML, like Gecko) Version/1.0 Mobile/10027 Safari/525.13";
	
    // Palm Permenanent ****
    //return "Mozilla/5.0 (Nova; U; en-US) AppleWebKit/525.13 (KHTML, like Gecko) Version/1.0 Safari/525.13";    
}

// Obsolete: remove when webkit submission is in effect
void WebPage::editorFocused(bool f, int fieldType, int fieldActions)
{
	editorFocused(f, fieldType, fieldActions, PalmIME::FieldFlags_None);
}

void WebPage::editorFocused(bool f, int fieldType, int fieldActions, int fieldFlags)
{
	PalmIME::EditorState editorState(static_cast<PalmIME::FieldType>(fieldType), static_cast<PalmIME::FieldAction>(fieldActions), static_cast<PalmIME::FieldFlags>(fieldFlags));
	editorFocused(f, editorState);
}

void WebPage::editorFocused(bool f, const PalmIME::EditorState & editorState)
{
	if (m_shuttingDown)
		return;

	m_isEditorFocused = f;
	m_editorState = editorState;

	g_debug("%s (%s): focused: %d", __PRETTY_FUNCTION__, appId().c_str(), f);
	
	if (m_client && !m_manualFocusEnabled) {
		// Prioritize editor Focus over explicit Editor focus
		if (m_isEditorFocused)
			m_client->editorFocusChanged(true, m_editorState);
		else if (m_isExplicitEditorFocused)
			m_client->editorFocusChanged(true, m_explicitEditorState);
		else
			m_client->editorFocusChanged(false, m_editorState);
	}
}

void WebPage::explicitEditorFocused(bool focused, const PalmIME::EditorState & editorState)
{
	if (m_shuttingDown)
		return;

	g_debug("%s (%s): focused: %d", __PRETTY_FUNCTION__, appId().c_str(), focused);
	
	m_isExplicitEditorFocused = focused;
	m_explicitEditorState = editorState;

	if (m_client && !m_manualFocusEnabled) {
		if (m_isEditorFocused)
			m_client->editorFocusChanged(true, m_editorState);
		else if (m_isExplicitEditorFocused)
			m_client->editorFocusChanged(true, m_explicitEditorState);
		else
			m_client->editorFocusChanged(false, m_editorState);
	}    
}

void WebPage::manualEditorFocused(bool focused, const PalmIME::EditorState & editorState)
{
	if (m_shuttingDown || !m_manualFocusEnabled)
		return;

	g_debug("%s (%s): focused: %d", __PRETTY_FUNCTION__, appId().c_str(), focused);
	
	m_isManualEditorFocused = focused;
	m_manualEditorState = editorState;

	if (m_client) {
		m_client->editorFocusChanged(focused, m_manualEditorState);
	}
}

void WebPage::manualFocusEnabled(bool enabled)
{
    if (m_manualFocusEnabled == enabled)
        return;

	g_debug("%s (%s): enabled: %d", __PRETTY_FUNCTION__, appId().c_str(), enabled);
	
    m_manualFocusEnabled = enabled;

    if (!enabled) {

        // prime the auto/explicit editor state
        if (m_isEditorFocused)
            m_client->editorFocusChanged(true, m_editorState);
        else if (m_isExplicitEditorFocused)
            m_client->editorFocusChanged(true, m_explicitEditorState);
        else
            m_client->editorFocusChanged(false, m_editorState);

        if (!m_isExplicitEditorFocused)
            m_client->autoCapEnabled(m_lastAutoCap);
    }
}

void WebPage::autoCapEnabled(bool enabled)
{
    if (!isEditing())
        return;

    if (enabled == m_lastAutoCap)
        return;

	m_lastAutoCap = enabled;

    // auto cap state for fields should affect manual keyboard control
    if (m_client)
        m_client->autoCapEnabled(enabled);
}

void WebPage::resizedContents(int newWidth, int newHeight)
{
	if (m_shuttingDown)
		return;

	if (m_client)
		m_client->resizedContents(newWidth, newHeight);    
}

void WebPage::zoomedContents(double scaleFactor, int newWidth, int newHeight,
							 int newScrollOffsetX, int newScrollOffsetY)
{
	if (m_shuttingDown)
		return;

	if (m_client)
	    m_client->zoomedContents(scaleFactor, newWidth, newHeight,
								 newScrollOffsetX, newScrollOffsetY);
}

void WebPage::invalContents(int x, int y, int width, int height)
{
	if (m_shuttingDown)
		return;

	if (m_client)
		m_client->invalContents(x, y, width, height);
    
}

void WebPage::scrolledContents(int newContentsX, int newContentsY)
{
	if (m_shuttingDown)
		return;

	if (m_client)
		m_client->scrolledContents(newContentsX, newContentsY);
}

void WebPage::loadStarted()
{
	if (m_shuttingDown)
		return;
}

void WebPage::loadStopped()
{
	if (m_shuttingDown)
		return;
	
	if (m_client)
		m_client->loadFinished();
}

void WebPage::didFinishDocumentLoad()
{
	if (m_shuttingDown)
		return;	
}

void WebPage::loadProgress(int progress)
{
	if (m_shuttingDown)
		return;

	m_progress = progress;
	if( 100 == progress )
	{
		if (m_webView)
			m_webView->focus(true);
		
		if (m_client)
			m_client->loadFinished();
		
		if( m_bufferedRelaunchLaunchingAppId.size() )
		{
			relaunch( m_bufferedRelaunchArgs.c_str(),
					m_bufferedRelaunchLaunchingProcId.c_str(), 
					m_bufferedRelaunchLaunchingAppId.c_str() );
			
			m_bufferedRelaunchArgs.clear();
			m_bufferedRelaunchLaunchingProcId.clear();
			m_bufferedRelaunchLaunchingAppId.clear();
		}
	}
}

void WebPage::inspect()
{
	if (m_shuttingDown)
		return;

	m_webPage->inspectRemote(true);
	m_webPage->inspect();
}


void WebPage::urlTitleChanged(const char* uri, const char* title)
{
	if (m_shuttingDown)
		return;
	
	if (m_url) {
		free(m_url);
		m_url = 0;
	}

	if (!uri)
		return;

	m_url = strdup(uri);

	if (m_waitingForUrl) {
		m_waitingForUrl = false;

		// What type of url did we get
		luna_log(kLogChannel, "Got url: %s", m_url);

		// Parse the url
		urlQueryToStageArguments(uri);

		createAndAttachToApp(0);
		if (!m_client)
			closePageSoon();
	}
	
	if (m_client) {
		m_client->titleChanged(title);
		m_client->uriChanged(uri);	
	}
}

void WebPage::reportError(const char* url, int errCode, const char* msg)
{
	if (getenv("DISABLE_JS_LOGGING"))
		return;

	if (!Settings::LunaSettings()->logger_useSyslog)		
		g_log(0, G_LOG_LEVEL_CRITICAL, "{LunaSysMgrJs}: **ERROR** %s ErrorCode=%d %s",	msg, errCode, url);
	else
		luna_syslogV(syslogContextJavascript(), G_LOG_LEVEL_CRITICAL, "**ERROR** %s ErrorCode=%d %s",  msg, errCode, url);
}

void WebPage::jsConsoleMessage(const char* inMsg, int lineNo, const char* inMsgSource)
{
	if (getenv("DISABLE_JS_LOGGING"))
		return;

	if (!Settings::LunaSettings()->logger_useSyslog)		
		g_log(0, G_LOG_LEVEL_MESSAGE, "{LunaSysMgrJS}: %s, %s:%d", inMsg, inMsgSource, lineNo);
	else
		luna_syslogV(syslogContextJavascript(), G_LOG_LEVEL_MESSAGE, "%s, %s:%d", inMsg, inMsgSource, lineNo);
}

void WebPage::jsConsoleMessage(Palm::MessageLevel level, const char* inMsg, int lineNo, const char* inMsgSource)
{
	if (getenv("DISABLE_JS_LOGGING"))
		return;

	GLogLevelFlags logLevel;
	switch(level) {
	case  Palm::TipMessageLevel:
		logLevel = G_LOG_LEVEL_DEBUG;
		break;
	case Palm::LogMessageLevel:
		logLevel = G_LOG_LEVEL_MESSAGE;
		break;
	case Palm::WarningMessageLevel:
		logLevel = G_LOG_LEVEL_WARNING;
		break;
	case Palm::ErrorMessageLevel:
		logLevel = G_LOG_LEVEL_CRITICAL;
		break;
	default:
		logLevel = G_LOG_LEVEL_INFO;
		break;
	}

	if (!Settings::LunaSettings()->logger_useSyslog)		
		g_log(0, logLevel, "{LunaSysMgrJS}: %s: %s, %s:%d", appId().c_str(), inMsg, inMsgSource, lineNo);
	else
		luna_syslogV(syslogContextJavascript(), logLevel, "%s: %s, %s:%d", appId().c_str(), inMsg, inMsgSource, lineNo);
}

bool WebPage::dialogAlert(const char* inMsg)
{
	if (!Settings::LunaSettings()->logger_useSyslog)		
		g_log(0, G_LOG_LEVEL_WARNING, "{LunaSysMgrJS}: %s", inMsg);
	else
		luna_syslog(syslogContextJavascript(), G_LOG_LEVEL_WARNING, inMsg);
	
	return false;
}

bool WebPage::dialogConfirm(const char* inMsg)
{
	return false;        
}

bool WebPage::dialogPrompt(const char* inMsg, const char* defaultValue, std::string& result)
{
    return false;
}

bool WebPage::dialogUserPassword(const char* inMsg, std::string& userName, std::string& result)
{
	return false;    
}

bool WebPage::dialogSSLConfirm(Palm::SSLValidationInfo&)
{
	return false;		//by default, be strict and fail any SSL anomalies. We may want to implement this later though to allow apps to use HTTPS/SSL
}

bool WebPage::popupMenuShow(void* menu, Palm::PopupMenuData* data)
{
	delete data;
	return false;
}

bool WebPage::popupMenuHide(void* menu)
{
	return false;
}

void WebPage::linkClicked(const char* url)
{
    
}

bool WebPage::displayStandaloneImages() const
{
	return true;
}

bool WebPage::shouldHandleScheme(const char* scheme) const
{
	if (NULL == scheme) {
		return false;
	}
	else {
		return !strcasecmp(scheme, "file");
	}
}

StringVariantMap parseStageArguments(const char* argString)
{
	StringVariantMap stageArgs;
    
	if (argString != 0) {
		json_object* root = json_tokener_parse(argString);
		if (root && !is_error(root) && json_object_get_type(root) == json_type_object) {

			json_object_object_foreach(root, key, val) {

				if (!key || !val)
					continue;

				switch (json_object_get_type(val)) {
				case json_type_boolean:
					stageArgs[key] = QVariant((bool) json_object_get_boolean(val));
					break;
				case json_type_int:
					stageArgs[key] = QVariant((int) json_object_get_int(val));
					break;
				case json_type_double:
					stageArgs[key] = QVariant((double) json_object_get_double(val));
					break;
				case json_type_string: {
					std::string s = json_object_get_string(val);
					stageArgs[key] = QVariant(QString::fromUtf8(s.c_str()));
					break;
				}
				default:
					g_warning("%s: Unknown stage argument type: %d",
							  __PRETTY_FUNCTION__, json_object_get_type(val));
					break;
				}
			}

			json_object_put(root);
		}
	}
	return stageArgs;
}

Palm::WebPage* WebPage::createPage(int width, int height, const char* name, const char* attributes)
{
	// first parse the stage arguments so we can tell whether we should fetch a precreated
	// shell page for this app (Card types only)
	StringVariantMap stageArgs = parseStageArguments(attributes);
	StringVariantMap::const_iterator it = stageArgs.find("window");
	std::string windowType;
	bool isModalWindow = false;

	if (it != stageArgs.end())
		windowType = it->second.toString().toStdString();

	if(windowType == "modalwindow")
		isModalWindow = true;

	WebPage* page = 0;
	if (windowType == "card") {

		// see if we already have a pre-created card shell for this app
		page = WebAppManager::instance()->takeShellPageForApp(appId());
		if (page) {

			page->setName(name);

			WebAppManager::instance()->webPageAdded(page);

			if (G_UNLIKELY(Settings::LunaSettings()->perfTesting)) {
				g_message("SYSMGR PERF: APP START appid: %s, processid: %s, type: %s, time: %d",
						  page->appId().c_str(), page->processId().c_str(),
						  WebAppFactory::nameForWindowType(Window::Type_Card).c_str(),
						  Time::curTimeMs());
			}
		}
	}

	if (!page) {

		// this is a child window. will always a view
		page = new WebPage(true);

		page->setAppId(appId()); // set same appid as self
		page->setName(name);

		page->m_requestedWidth = width;
		page->m_requestedHeight = height;

		WebAppManager::instance()->webPageAdded(page);
	
		if ((true == isModalWindow) || (m_inRelaunch && !launchingAppId().empty() && !launchingProcessId().empty())) {
			page->setLaunchingAppId(launchingAppId());
			page->setLaunchingProcessId(launchingProcessId());
		}
		else {
			page->setLaunchingAppId(appId()); // we are launching this app
			page->setLaunchingProcessId(processId());
		}
	}

	WebPage* parent = this;
	if (m_parent)
		parent = m_parent;
	page->setParent(parent);
	
	if (!stageArgs.empty()) {
		g_message("%s: name: %s, stageArguments: %s", __PRETTY_FUNCTION__, name, attributes);

		page->m_waitingForUrl = false;
		page->m_stageArgs = stageArgs;

		page->createAndAttachToApp(m_client ? m_client->getAppDescription() : 0);
		if (!page->m_client)
			page->closePageSoon();
	}

	return page->webkitPage();
}

void WebPage::createViewForWindowlessPage()
{
	// only headless apps should create shell cards
	if (webkitView() != 0)
		return;

	if (m_args.size() > 0) {
		// cross app pushing should not generate a shell page
		if (strstr(m_args.c_str(), "mojoCrossPush") != NULL)
			return;
		// Don't do it launcher explicitly asks for it
		if (strstr(m_args.c_str(), "$disableCardPreLaunch") != NULL)
			return;
	}

	WebPage* shellPage = new WebPage(true);

	shellPage->setAppId(appId());
	shellPage->setLaunchingAppId(launchingAppId());
	shellPage->setLaunchingProcessId(processId());

	shellPage->setParent(this);

	WebAppManager* webMgr = WebAppManager::instance();

	Window::Type winType;

	if (m_client->getAppDescription()->uiRevision() == 2) {
		winType = Window::Type_Card;
	} else {
		winType = Window::Type_Emulated_Card;
	}

	shellPage->m_client = webMgr->launchWithPageInternal(shellPage, winType,
														 m_client ? m_client->getAppDescription() : 0);
	if (!shellPage->m_client)
		shellPage->closePageSoon();
	webMgr->shellPageAdded(shellPage);
}

void WebPage::closePageSoon()
{
	if (m_shuttingDown)
		return;
	
	luna_log(kLogChannel, "Close Window Soon: %p", this);

	WebAppManager::instance()->closePageSoon(this);
}

void WebPage::focused()
{
	// no-op: use PalmSystem.activate
    /* if (m_client)
	 	m_client->focus();
	*/
}

void WebPage::unfocused()
{
	// no-op: use PalmSystem.deactivate
	/* if (m_client)
		m_client->unfocus();
	*/
}

GMainLoop* WebPage::mainLoop() const
{
	return webkit_palm_get_mainloop();
}

void WebPage::statusMessage(const char* message)
{
	if (m_shuttingDown)
		return;

	if (!m_client)
		return;
	
	if (!message || message[0] == 0)
		return;

	m_client->statusMessage(message);
}

void WebPage::dispatchFailedLoad(const char* domain, int errorCode,
			const char* failingURL, const char* localizedDescription)
{
	if (m_shuttingDown)
		return;

	if (m_client) {
		m_client->dispatchFailedLoad(domain, errorCode, failingURL, localizedDescription);
	}
}

const char* WebPage::getIdentifier()
{
	if (m_identifier.empty())
		m_identifier = windowIdentifierFromAppAndProcessId(appId(), processId());
	
	return m_identifier.c_str();
}

bool WebPage::isBusPriviledged() {

#if defined(TARGET_DESKTOP)
	return true;
#endif
	if (m_client == NULL)
		return false;

	ApplicationDescription * appDesc = m_client->getAppDescription();
	if (appDesc == NULL)
		return false;

	if (appDesc->id().empty())
		return false;

	return (appDesc->id().find("com.palm.", 0) == 0);
}

void WebPage::copy()
{
	if (m_webView)
		m_webView->copy();
}

void WebPage::cut()
{
	if (m_webView)
		m_webView->cut();
}

void WebPage::paste()
{
	if (m_webView)
		m_webView->paste();
}

void WebPage::selectAll()
{
    if (m_webView)
        m_webView->selectAll();
}

void WebPage::setComposingText(const char* text)
{
	if (m_webView)
		m_webView->setComposingText(text);
}

void WebPage::commitComposingText()
{
	if (m_webView)
		m_webView->commitComposingText();
}

void WebPage::commitText(const char* text)
{
	if (m_webView)
		m_webView->insertStringAsKeyEvents(text);
}

void WebPage::performEditorAction(PalmIME::FieldAction action)
{
	if (m_webView)
		m_webView->performEditorAction(action);
}

void WebPage::removeInputFocus()
{
	if (m_webView)
		m_webView->removeInputFocus();

	// FIXME: if input focus was explicitly set, turn it off directly. Need
	// a mechanism to remove this type of focus
	if (m_isExplicitEditorFocused) {
		PalmIME::EditorState editorState;
		explicitEditorFocused(false, editorState);
	}

    if (m_manualFocusEnabled && m_isManualEditorFocused) {
        PalmIME::EditorState editorState;
        manualEditorFocused(false, editorState);        
    }
}

void WebPage::mimeHandoffUrl(const char* mimeType, const char* url)
{
	WebAppManager::instance()->mimeHandoffUrl(mimeType, url, getIdentifier());
}


void WebPage::mimeNotHandled(const char* mimeType, const char* url)
{
	mimeHandoffUrl(mimeType, url);
}

bool WebPage::interceptPageNavigation(const char* url, bool isInitialOpen)
{
	return false;
}

void WebPage::setName(const char* name)
{
    m_name = name ? name : "";
}

std::string WebPage::name() const
{
    return m_name;
}

void WebPage::copiedToClipboard()
{
	WebAppManager::instance()->copiedToClipboard(appId());
}

void WebPage::pastedFromClipboard()
{
	WebAppManager::instance()->pastedFromClipboard(appId());
}

/**
 * The context information for our smart key search.
 */
struct SmartKeySearchContext
{
	int requestId;
};

/**
 * Called by luna service when a response to a smart key search is received.
 */
bool WebPage::smartKeySearchCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
	if (!message) {
		return true;
	}

	int requestId = (int)ctx;

	const char* payload = LSMessageGetPayload(message);
	if (!payload)
		return true;

	bool succeeded(false);
	
	json_object* json = json_tokener_parse(payload);

	if (ValidJsonObject(json)) {
		json_object* value = json_object_object_get(json, "returnValue" );
		if( ValidJsonObject(value) && json_object_get_boolean(value) ) {
			value = json_object_object_get(json, "match" );
			if( ValidJsonObject(value) ) {
				Palm::WebGlobal::smartKeySearchResponse(requestId, json_object_get_string(value));
				succeeded = true;
			}
		}
		json_object_put(json);
	}

	if (!succeeded) {
		Palm::WebGlobal::smartKeySearchResponse(requestId, "");
	}

	return true;
}

bool WebPage::smartKeySearch(int requestId, const char* query)
{
	static LSHandle* s_lsHandle;

	if (query == NULL || *query == '\0')
		return Palm::WebGlobal::smartKeySearchResponse(requestId, "");

	if (s_lsHandle == NULL) {
		if (LSRegister(NULL, &s_lsHandle, NULL)) {
			if (!LSGmainAttach(s_lsHandle, mainLoop(), NULL)) {
				LSUnregister(s_lsHandle, NULL);
				s_lsHandle = NULL;
			}
		}
	}

	if (s_lsHandle == NULL) {
		return Palm::WebGlobal::smartKeySearchResponse(requestId, "");
	}

	json_object* payload = json_object_new_object();
	if (!ValidJsonObject(payload)) {
		return Palm::WebGlobal::smartKeySearchResponse(requestId, "");
	}

	json_object_object_add(payload, "query", json_object_new_string(query));

	LSError error;
	LSErrorInit(&error);

	bool succeeded = LSCall(s_lsHandle, "palm://com.palm.smartKey/search", json_object_get_string(payload),
				 smartKeySearchCallback, (void*)requestId, NULL, &error);
	json_object_put(payload);
	if (succeeded) {
		return true;
	}
	else {
		g_warning("Failed querying smartKey service: %s", error.message);
		LSErrorFree(&error);
		return Palm::WebGlobal::smartKeySearchResponse(requestId, "");
	}
}

bool WebPage::smartKeyLearn(const char* word)
{
	static LSHandle* s_lsHandle;

	if (!word || *word == '\0') {
		return false;
	}

	if (s_lsHandle == NULL) {
		if (LSRegister(NULL, &s_lsHandle, NULL)) {
			if (!LSGmainAttach(s_lsHandle, mainLoop(), NULL)) {
				LSUnregister(s_lsHandle, NULL);
				s_lsHandle = NULL;
			}
		}
	}

	if (s_lsHandle == NULL) {
		return false;
	}

	json_object* payload = json_object_new_object();
	if (!ValidJsonObject(payload)) {
		return false;
	}

	json_object_object_add(payload, "word", json_object_new_string(word));

	LSError error;
	LSErrorInit(&error);

	bool succeeded = LSCall(s_lsHandle, "palm://com.palm.smartKey/learn", json_object_get_string(payload),
			NULL, NULL, NULL, &error);

	json_object_put(payload);

	if (!succeeded) {
		g_warning("Failed querying smartKey service: %s", error.message);
		LSErrorFree(&error);
		return false;
	}

	return true;
}

void WebPage::urlQueryToStageArguments(const std::string& urlStr)
{
	QUrl url(urlStr.c_str());
	if (url.hasQuery()) {

		QList<QPair<QString, QString> > queryItems = url.queryItems();
		for (QList<QPair<QString, QString> >::const_iterator it = queryItems.constBegin();
			 it != queryItems.end(); ++it) {
			m_stageArgs[it->first.toStdString()] = it->second;
		}
	}
}

void WebPage::createAndAttachToApp(ApplicationDescription* appDesc)
{
	if (m_client)
		return;

	std::string windowType;
	Window::Type winType = Window::Type_Card;

	StringVariantMap::const_iterator it = m_stageArgs.find("window");
	if (it != m_stageArgs.end())
		windowType = it->second.toString().toStdString();

	if (windowType == "dashboard") {
		luna_log(kLogChannel, "Creating Dashboard App");
		winType = Window::Type_Dashboard;
		m_client = WebAppManager::instance()->launchWithPageInternal(this, winType, appDesc);
	}
	else if (windowType == "banneralert") {
		luna_log(kLogChannel, "Creating Banner Alert");
		winType = Window::Type_BannerAlert;
		m_client = WebAppManager::instance()->launchWithPageInternal(this, winType, appDesc);
	}
	else if (windowType == "popupalert") { 
		luna_log(kLogChannel, "Creating Popup Alert");
		winType = Window::Type_PopupAlert;
		m_client = WebAppManager::instance()->launchWithPageInternal(this, winType, appDesc);
	}
	else if (windowType == "menu") {
		luna_log(kLogChannel, "Creating Menu Window");
		winType = Window::Type_Menu;
		m_client = WebAppManager::instance()->launchWithPageInternal(this, winType, appDesc);
	}
	else if (windowType == "emergency") {
		luna_log(kLogChannel, "Creating Emergency Window");
		if (appId().find("com.palm.", 0) == 0) {
			winType = Window::Type_Emergency;
			m_client = WebAppManager::instance()->launchWithPageInternal(this, winType, appDesc);
		}
		else {
			g_critical("Not allowed to create emergency window from app %s", appId().c_str());
			m_client = 0;
		}
	}
	else if (windowType == "childcard") {
		luna_log(kLogChannel, "Creating Child Card Window");

		// the identifier of the parent window should have been passed down to us through the
		// url query params
			
		std::string parentIdentifier;
		std::string launchingAppId;
		std::string launchingProcessId;

		StringVariantMap::const_iterator it = m_stageArgs.find("parentidentifier");
		if (it != m_stageArgs.end())
			parentIdentifier = it->second.toString().toStdString();
			
		if (parentIdentifier.empty() || !splitWindowIdentifierToAppAndProcessId(parentIdentifier, launchingAppId, launchingProcessId)) {

			// hmmm... no. we just close ourselves
			luna_critical(kLogChannel, "No parent identifier passed for child window creation");
			closePageSoon();
			return;
		}

		setLaunchingAppId(launchingAppId);
		setLaunchingProcessId(launchingProcessId);

		WebAppManager* appMan = WebAppManager::instance();			
		WebAppBase* app = appMan->findApp(launchingProcessId);
		if (app) {
			winType = Window::Type_ChildCard;
			m_client = appMan->launchWithPageInternal(this, winType, appDesc);
		}
	}
	else if (windowType == "pin") {
		luna_log(kLogChannel, "Creating PIN Window");
		winType = Window::Type_PIN;
		m_client = WebAppManager::instance()->launchWithPageInternal(this, winType, appDesc);
	}
	else if (windowType == "dockmode") {
		luna_log(kLogChannel, "Creating Dock mode Window");
		winType = Window::Type_DockModeWindow;
		m_client = WebAppManager::instance()->launchWithPageInternal(this, winType, appDesc);
	}
	else if (windowType == "modalwindow") {
		luna_log(kLogChannel, "Creating modal Window");
		winType = Window::Type_ModalChildWindowCard;
		m_client = WebAppManager::instance()->launchWithPageInternal(this, winType, appDesc);
	}
	else {
		luna_log(kLogChannel, "Creating Card App");
		if (!appDesc || (appDesc->uiRevision() == 2)) {
			winType = Window::Type_Card;
		} else {
			winType = Window::Type_Emulated_Card;
		}
		m_client = WebAppManager::instance()->launchWithPageInternal(this, winType, appDesc);
	}

	if (G_UNLIKELY(Settings::LunaSettings()->perfTesting)) {
		g_message("SYSMGR PERF: APP START appid: %s, processid: %s, type: %s, time: %d",
				  appId().c_str(), processId().c_str(),
				  WebAppFactory::nameForWindowType(winType).c_str(),
				  Time::curTimeMs());
	}
}

WebPage* WebPage::parent() const
{
	return m_parent;    
}

void WebPage::setParent(WebPage* page)
{
	m_parent = page;
	if (m_parent)
		m_parent->addChild(this);
}

void WebPage::addChild(WebPage* page)
{
    m_children.insert(page);
}

void WebPage::removeChild(WebPage* page)
{
	m_children.erase(page);

	if (m_webPage) {
		g_message("%s Parent: %s, %s", __PRETTY_FUNCTION__, m_name.c_str(), page->name().c_str());
		std::string script = "if (Mojo.handleWindowClosed) Mojo.handleWindowClosed(\"";
		script += page->name();
		script += "\");";
		m_webPage->evaluateScript(script.c_str());
	}
}

void WebPage::addNewContentRequestId (const std::string& requestId)
{
    g_debug ("Added requestId %s to page of app %s", requestId.c_str(), appId().c_str());
    m_newContentRequestIds.insert (requestId);
}

void WebPage::removeNewContentRequestId (const std::string& requestId)
{
    g_debug ("Removing requestId %s from page of app %s", requestId.c_str(), appId().c_str());

    m_newContentRequestIds.erase (m_newContentRequestIds.find (requestId));
}

void WebPage::needSensorEvents(Palm::SensorType type, bool needEvents)
{
    if(m_client)
    {
        m_client->enableSensor(type, needEvents);
    }
}

void WebPage::fastAccelerometerOn(bool enable)
{
    if (m_client)
    {
        m_client->fastAccelerometerOn(enable);
    }
}

void WebPage::needTouchEvents (bool needTouchEvents)
{
	if (m_needTouchEvents == needTouchEvents)
		return;

    m_needTouchEvents = needTouchEvents;
	if (m_client)
		m_client->needTouchEvents(needTouchEvents);
}

void WebPage::suspendAppRendering()
{
    if(m_client)
        m_client->suspendAppRendering();
}

void WebPage::resumeAppRendering()
{
    if(m_client)
        m_client->resumeAppRendering();
}

int WebPage::activityId() const
{
    if (m_parent)
		return m_parent->activityId();

	return m_activiyId;
}

void WebPage::setActivityId(int id)
{
	g_message("%s activity Id: %d", appId().c_str(), id);
	m_activiyId = id;    
}

void WebPage::hideSpellingWidget()
{
	if (m_webView)
		m_webView->hideSpellingWidget();
}

Palm::WebGLES2Context* WebPage::createGLES2Context()
{
	return m_client ? m_client->getGLES2Context() : 0;
}

void WebPage::frameCreated(Palm::WebFrame* frame)
{
    m_iFrames.push_back(frame);

	WebFrame* webFrame = new WebFrame(this, frame);
	frame->setClient(webFrame);
}

void WebPage::frameDestroyed(Palm::WebFrame* frame)
{
    std::list<Palm::WebFrame*>::iterator it = std::find(m_iFrames.begin(), m_iFrames.end(), frame);
    if (it != m_iFrames.end())
        m_iFrames.erase(it);

    WebFrameClient* c = frame->client();
	if (c) {
		WebFrame* webFrame = static_cast<WebFrame*>(c);
		delete webFrame;
	}
}

bool WebPage::isValidFrame(const Palm::WebFrame* frame) const
{
    if (m_mainFrame && (frame == m_mainFrame->webkitFrame()))
        return true;

    std::list<Palm::WebFrame*>::const_iterator it = std::find(m_iFrames.begin(), m_iFrames.end(), frame);
    return it != m_iFrames.end();
}

void WebPage::jsObjectCleared()
{
	m_mainFrame->jsObjectCleared();    
}

void WebPage::setAppId(const std::string& id)
{
	m_mainFrame->setAppId(id);
	ProcessBase::setAppId(id);
}

void WebPage::setProcessId(const std::string& id)
{
	m_mainFrame->setProcessId(id);
	ProcessBase::setProcessId(id);    
}

Palm::SensorHandle WebPage::createSensor(Palm::SensorType aType, Palm::fnSensorDataCallback aDataCB, Palm::fnSensorErrorCallback aErrCB, Palm::fnSensorHandleDelete* afnDelete, void* apUserData)
{
    Palm::SensorHandle hSensor = 0;

    //Validate the passed params
    if ((aType < Palm::SensorLast) && (aDataCB) && (aErrCB) && (afnDelete))
    {
        WebKitSensorConnector *pSensor = WebKitSensorConnector::createSensor(aType, aDataCB, aErrCB, apUserData);
        *afnDelete = deleteSensorHandle;
        hSensor = static_cast<void *>(pSensor);
    }

    return hSensor;
}

void WebPage::deleteSensorHandle(Palm::SensorHandle* apHandle)
{
    if ((apHandle) && (*apHandle))
    {
        WebKitSensorConnector *pSensor = static_cast<WebKitSensorConnector*>(*apHandle);
        delete pSensor;
        *apHandle = 0;
    }
}

void WebPage::destroySensor(Palm::SensorHandle* apHandle)
{
    deleteSensorHandle(apHandle);
}

std::string WebPage::getSupportedSensors()
{
    return WebKitSensorConnector::getSupportedSensors();
}

bool WebPage::setSensorRate(Palm::SensorHandle aHandle, Palm::SensorRate aRate)
{
    bool bRetValue = false;

    if (aHandle)
    {
        WebKitSensorConnector *pSensor = static_cast<WebKitSensorConnector*>(aHandle);
        bRetValue = pSensor->setRate(aRate);
    }

    return bRetValue;
}

bool WebPage::startSensor(Palm::SensorHandle aHandle, bool aOn)
{
    bool bRetValue = false;

    if (aHandle)
    {
        WebKitSensorConnector *pSensor = static_cast<WebKitSensorConnector*>(aHandle);

        if (aOn)
        {
            bRetValue = pSensor->on();
        }
        else
        {
            bRetValue = pSensor->off();
        }
    }

    return bRetValue;
}
