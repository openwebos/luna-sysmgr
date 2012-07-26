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




#ifndef WEBPAGE_H
#define WEBPAGE_H

#include "Common.h"

#include <string>
#include <list>
#include <map>
#include <set>

// webkit API (for Palm)
#include <palmwebpage.h>
#include <palmwebpageclient.h>
#include <palmwebviewclient.h>
#include <palmwebsslinfo.h>

#include "ProcessBase.h"
#include "Timer.h"
#include <QVariant>

#include <palmimedefines.h>

class ApplicationDescription;
class Event;
class JsSysObject;
class WebPageClient;
class WebFrame;

namespace Palm {
	class WebGLES2Context;
}

typedef std::map<std::string, QVariant> StringVariantMap;

class WebPage : public Palm::WebViewClient,
				public Palm::WebPageClient,
				public ProcessBase
{
public:

	WebPage(bool createView);
	WebPage(const char* url, bool createView);
	virtual ~WebPage();

	WebPage* parent() const;
	void setParent(WebPage* page);

	void addChild(WebPage* page);
	void removeChild(WebPage* page);

	void createViewForWindowlessPage();

	// Call run after creating the page (if possible, set the
	// client before call this);
	void run();
		
	void setClient(::WebPageClient* c);
	::WebPageClient* client() const;

	const char* url() const { return m_url; }
	Palm::WebView* webkitView() const { return m_webView; }
	Palm::WebPage* webkitPage() const { return m_webPage; }

	void setArgs(const char* args);
		
	bool relaunch(const char* args, const char* launchingAppId, const char* launchingProcId);
	
	void inspect();

	GMainLoop* mainLoop() const;

	uint32_t progress() const { return m_progress; }
	
	bool isEditing() const { return m_manualFocusEnabled ? m_isManualEditorFocused 
                                                         : (m_isEditorFocused || m_isExplicitEditorFocused); }
	const PalmIME::EditorState& editorState() const {
        if (m_manualFocusEnabled)
            return m_manualEditorState;
		else if (m_isEditorFocused)
			return m_editorState;
		else if (m_isExplicitEditorFocused)
			return m_explicitEditorState;
		else
			return m_editorState;
	}

	bool lastAutoCap() const { return m_lastAutoCap; }
	
	void copy();
	void cut();
	void paste();
    void selectAll();
	
	int requestedWidth() const { return m_requestedWidth; }
	int requestedHeight() const { return m_requestedHeight; }

	const StringVariantMap& stageArguments() const { return m_stageArgs; }
	
	void releaseNestedLoopIfNecessary();
	
	void setName(const char* name);
	std::string name() const;
	
	bool isShuttingDown() const { return m_shuttingDown; }

	void setStageReadyPending(bool pending) { m_stageReadyPending = pending; }
	bool stageReadyPending() const { return m_stageReadyPending; }

	void urlQueryToStageArguments(const std::string& urlStr);
	void createAndAttachToApp(ApplicationDescription* appDesc);

	void addNewContentRequestId (const std::string& requestId);
	void removeNewContentRequestId (const std::string& requestId);

	virtual void needTouchEvents(bool);
	bool isTouchEventsNeeded() const { return m_needTouchEvents; }

	int activityId() const;
	void setActivityId(int id);

	void hideSpellingWidget();

	void setComposingText(const char* text);
	void commitComposingText();

	void commitText(const char* text);

	void performEditorAction(PalmIME::FieldAction action);

	void removeInputFocus();

	virtual Palm::WebGLES2Context* createGLES2Context();

    bool isValidFrame(const Palm::WebFrame* frame) const;

    virtual void suspendAppRendering();
    virtual void resumeAppRendering();
    virtual void fastAccelerometerOn(bool enable);

private:

	// Palm::WebViewClient
    virtual void getVirtualWindowSize(int& width, int& height);
    virtual void getWindowSize(int& width, int& height);
	virtual void getScreenSize( int& width, int& height );
    virtual void resizedContents(int newWidth, int newHeight);
    virtual void zoomedContents(double scaleFactor, int newWidth, int newHeight, int newScrollOffsetX, int newScrollOffsetY);
    virtual void invalContents(int x, int y, int width, int height);
    virtual void scrolledContents(int newContentsX, int newContentsY);
	virtual void linkClicked(const char* url);
	virtual void editorFocused(bool focused, int fieldType, int fieldActions);
	virtual void editorFocused(bool focused, int fieldType, int fieldActions, int fieldFlags);
	virtual void editorFocused(bool focused, const PalmIME::EditorState & editorState);
	virtual void explicitEditorFocused(bool focused, const PalmIME::EditorState & editorState);
	virtual void manualEditorFocused(bool focused, const PalmIME::EditorState & editorState);
    virtual void manualFocusEnabled(bool enabled);
    virtual void autoCapEnabled(bool enabled);
	virtual void focused();
	virtual void unfocused();
	virtual Palm::TextCaretType textCaretAppearance() { return Palm::TextCaretNormal; }
	virtual void startDrag(int x, int y, int imgOffsetX, int imgOffsetY, void* dragImageRef, PalmClipboard* sysClipboard);
	virtual void viewportTagParsed(double initialScale, double minimumScale, double maximumScale, int width, int height,
			bool userScalable, bool didUseConstantsForWidth, bool didUseConstantsForHeight) {}
	// FIXME: 8/7/11: remove once webkit changes are in build
	virtual void viewportTagParsed(double initialScale, double minimumScale, double maximumScale,
								   int width, int height, bool userScalable) {}
	virtual void makePointVisible(int x, int y) {}
    
	// Palm::WebPageClient
    virtual const char* getUserAgent();
    virtual void loadStarted();
    virtual void loadStopped();
    virtual void loadProgress(int progress);
    virtual void didFinishDocumentLoad();
	virtual void urlTitleChanged(const char* uri, const char* title);
	virtual void reportError(const char* url, int errCode, const char* msg);
	virtual void jsConsoleMessage(const char* inMsg, int lineNo, const char* inMsgSource);
	virtual void jsConsoleMessage(Palm::MessageLevel level, const char* inMsg, int lineNo, const char* inMsgSource);
	virtual const char* getIdentifier();
	/*
	 * return true if this WebPageClient entity is allowed to use the private lunabus
	 */
	virtual bool isBusPriviledged();
    virtual bool dialogAlert(const char* inMsg);
    virtual bool dialogConfirm(const char* inMsg);
    virtual bool dialogPrompt(const char* inMsg, const char* defaultValue, std::string& result);
    virtual bool dialogUserPassword(const char* inMsg, std::string& userName, std::string& password);
    /*
     * dialogSSLPrompt
     * 
     * return true if things went ok and user presented a response, false otherwise
     */
    virtual bool dialogSSLConfirm(Palm::SSLValidationInfo& sslInfo);
        
    virtual bool popupMenuShow(void* menu, Palm::PopupMenuData* data);
    virtual bool popupMenuHide(void* menu);
	virtual void mimeHandoffUrl(const char* mimeType, const char* url);
	virtual void mimeNotHandled(const char* mimeType, const char* url);
	virtual bool interceptPageNavigation(const char* url, bool isInitialOpen);
	virtual bool shouldHandleScheme(const char* scheme) const;
	virtual bool displayStandaloneImages() const;
	virtual void downloadStart(const char* url) {}
	virtual void downloadProgress(const char* url, unsigned long bytesSoFar, unsigned long estimatedTotalSize) {}
	virtual void downloadError(const char* url, const char* msg) {}
	virtual void downloadFinished(const char* url, const char* mimeType, const char* tmpPathName) {}
	virtual Palm::WebPage* createPage(int width, int height, const char* name, const char* attributes);
	virtual void closePageSoon();
	virtual void statusMessage(const char* msg);
	virtual void updateGlobalHistory(const char* url, bool reload) {}
	virtual void dispatchFailedLoad(const char* domain, int errorCode,
						const char* failingURL, const char* localizedDescription);
	virtual void setMainDocumentError(const char* domain, int errorCode,
			const char* failingURL, const char* localizedDescription) {}

	virtual void pluginFullscreenSpotlightCreate(int, int, int, int, int) {}
	virtual void pluginFullscreenSpotlightRemove() {}

	virtual void copiedToClipboard();
	virtual void pastedFromClipboard();

	virtual bool smartKeySearch(int requestId, const char* query);
	virtual bool smartKeyLearn(const char* word);
	static bool smartKeySearchCallback(struct LSHandle *sh, struct LSMessage *message, void *ctx);

	virtual void needSensorEvents(Palm::SensorType type, bool needEvents);

	virtual void jsObjectCleared();
	
    virtual void frameCreated(Palm::WebFrame* frame);
    virtual void frameDestroyed(Palm::WebFrame* frame);

	virtual void setAppId(const std::string& id);
	virtual void setProcessId(const std::string& id);
	
    /**
     *  Function creates a sensor and returns an Opaque handle to the sensor instance
     *
     *  @param aType        - Type of the sensor to be created
     *  @param aDataCB      - Data callback will be called once data is available
     *  @param aErrCB       - Error callback will be called if there is some error encountered
     *  @param afnDelete    - Sensor Object deletion function
     *  @param pUserData    - User data - Ownership of this pointer is not transferred during this API.
     *  @return a Valid Handle of the sensor
     */
    virtual Palm::SensorHandle createSensor(Palm::SensorType aType, Palm::fnSensorDataCallback aDataCB, Palm::fnSensorErrorCallback aErrCB, Palm::fnSensorHandleDelete* afnDelete, void *pUserData);

    /**
     * Function destroys the particular sensor.
     *
     * @param apHandle - pointer to the handle of the sensor
     * @note apHandle will be invalid after this function call
     */
    virtual void destroySensor(Palm::SensorHandle* apHandle);

    /**
     * Function gets all the supported sensors by the platform
     *
     * @return json array list of all the sensors
     */
    virtual std::string getSupportedSensors();

    /**
     * Function sets the sensor rate for the given sensor
     */
    virtual bool setSensorRate(Palm::SensorHandle aHandle, Palm::SensorRate aRate);

    /**
     * Start/Stop the sensor
     */
    virtual bool startSensor(Palm::SensorHandle aHandle, bool aOn);
private:

	::WebPageClient* m_client;
	Palm::WebView* m_webView;
	Palm::WebPage* m_webPage;
	WebFrame* m_mainFrame;

	char* m_url;
	std::string m_args;
	bool m_waitingForUrl;
	bool m_shuttingDown;	
	uint32_t m_progress;

	bool m_isEditorFocused;
	bool m_isExplicitEditorFocused;
    bool m_isManualEditorFocused;
	PalmIME::EditorState m_editorState;
	PalmIME::EditorState m_explicitEditorState;
    PalmIME::EditorState m_manualEditorState;

    bool m_manualFocusEnabled;

	bool m_lastAutoCap;
	bool m_inRelaunch;

	int m_requestedWidth;
	int m_requestedHeight;

	StringVariantMap m_stageArgs;

	GMainLoop* m_nestedLoop;

	std::string m_identifier;
	std::string m_name;
	
	std::string m_bufferedRelaunchArgs;
	std::string m_bufferedRelaunchLaunchingAppId;
	std::string m_bufferedRelaunchLaunchingProcId;

	bool m_stageReadyPending;

	WebPage* m_parent;
	std::set<WebPage*> m_children;
	std::set<std::string> m_newContentRequestIds;

	bool m_needTouchEvents;
	int m_activiyId;

    std::list<Palm::WebFrame*> m_iFrames;
	
private:

	WebPage(const WebPage&);
	WebPage& operator=(const WebPage&);

    /**
     * Function destroys the particular sensor.
     *
     * @param apHandle - pointer to the handle of the sensor
     * @note apHandle will be invalid after this function call
     */
    static void deleteSensorHandle(Palm::SensorHandle* apHandle);

	friend class WebAppBase;
	friend class WebFrame;
};
	
#endif /* WEBPAGE_H */
