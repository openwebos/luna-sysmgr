/**
 * @file
 * 
 * Holds information about processes launched by LunaSysMgr
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

/**
 * Represents a WebKit-based web page
 * 
 * This class seems to mostly just aggregate functionality from
 * other classes into a single convenient point.  For the most part
 * it adds very little functionality.
 * 
 * Does not contain display management and rendering functionality.
 * 
 * @see WebPageClient
 * 
 * @todo There is a lot of functionality in this class that relies on other classes which are currently undocumented or not publicly documented.  When some of them are, please revisit this and document it more fully.
 */
class WebPage : public Palm::WebViewClient,
				public Palm::WebPageClient,
				public ProcessBase
{
public:
	/**
	 * Constructs a WebKit-based web page object
	 * 
	 * Initializes dependencies, creates a
	 * {@link WebFrame WebFrame}, and waits for a URL to load
	 * into it.
	 * 
	 * @see WebFrame
	 * 
	 * @todo Clarify this a bit once Palm::WebView is publicly documented.
	 * 
	 * @param	createView			Whether or not to create a new Palm::WebView for this page (since that class is currently undocumented within this project, this needs to be clarified a bit).
	 */
	WebPage(bool createView);
	
	/**
	 * Constructs a WebKit-based web page object
	 * 
	 * Initializes dependencies, creates a
	 * {@link WebFrame WebFrame}, and sets the URL to load.
	 * 
	 * @see WebFrame
	 * 
	 * @todo Clarify this a bit once Palm::WebView is publicly documented.
	 * 
	 * @param	url				URL to load once WebPage::run() is called.
	 * @param	createView			Whether or not to create a new Palm::WebView for this page (since that class is currently undocumented within this project, this needs to be clarified a bit).
	 */
	WebPage(const char* url, bool createView);
	
	/**
	 * Cleans up this WebPage
	 * 
	 * - Lets WebAppManager know that it's closing.
	 * - Cleans up the view and/or the Palm::WebPage class (which is currently undocumented in this project).
	 * - Disconnects all child pages (pages opened from links in this page) from this one.
	 * - Clears all "page loading" indicators.
	 * 
	 * @todo Clarify this a bit once Palm::WebPage is publicly documented.
	 */
	virtual ~WebPage();
	
	/**
	 * Gets the WebPage that opened this one
	 * 
	 * When you click a link in a page and it opens a new card,
	 * the page with the link is the parent and the new card is
	 * the child of it.  This tracks the parent instance from
	 * the child.
	 * 
	 * @return					WebPage instance which opened this one.
	 */
	WebPage* parent() const;
	
	/**
	 * Sets the WebPage that opened this one
	 * 
	 * When you click a link in a page and it opens a new card,
	 * the page with the link is the parent and the new card is
	 * the child of it.  This tracks the parent instance from
	 * the child.
	 * 
	 * @param	page				WebPage instance which opened this one.
	 */
	void setParent(WebPage* page);
	
	/**
	 * Adds a child WebPage to our current page
	 * 
	 * When you click a link in a page and it opens a new card,
	 * the page with the link is the parent and the new card is
	 * the child of it.  This tracks each child instance that
	 * was opened from our current instance (the parent page).
	 * 
	 * @param	page				New child page to add to the list of pages which were opened from links in this page.
	 */
	void addChild(WebPage* page);
	
	/**
	 * Removes a child WebPage from our current page
	 * 
	 * When you click a link in a page and it opens a new card,
	 * the page with the link is the parent and the new card is
	 * the child of it.  This removes a child from the list of
	 * pages that were opened from our current instance (the
	 * parent page).
	 * 
	 * @param	page				Child page to remove from the list of pages which were opened from links in this page.
	 */
	void removeChild(WebPage* page);
	
	/**
	 * @todo Document this - it's currently confusing as all get-out looking at the implementation.
	 */
	void createViewForWindowlessPage();
	
	/**
	 * Starts the current URL loading
	 * 
	 * Loads the current URL into the Palm::WebPage instance
	 * associated with this class instance.
	 * 
	 * Call run after creating the page (if possible, set the
	 * client before call this);
	 * 
	 * @todo Document this further once Palm::WebPage is publicly documented.
	 */
	void run();
	
	/**
	 * Set the WebPageClient which is responsible for displaying this WebPage
	 * 
	 * WebPage is responsible for loading and processing WebKit-
	 * based web pages.  WebPageClient is responsible for
	 * interacting with the display (handling resizing,
	 * orientation changes, scrolling, zooming, etc.).  This
	 * allows you to connect the two.
	 * 
	 * @see WebPageClient
	 * 
	 * @param	c				WebPageClient instance responsible for handling changes/interaction with the display.
	 */
	void setClient(::WebPageClient* c);
	
	/**
	 * Gets the WebPageClient which manages display changes for this instance
	 * 
	 * WebPage is responsible for loading and processing WebKit-
	 * based web pages.  WebPageClient is responsible for
	 * interacting with the display (handling resizing,
	 * orientation changes, scrolling, zooming, etc.).  This
	 * allows you to see which WebPageClient is currently
	 * connected to this WebPage instance.
	 * 
	 * @see WebPageClient
	 * 
	 * @return					The WebPageClient which is handling display changes for this instance.
	 */
	::WebPageClient* client() const;
	
	/**
	 * Gets the URL currently set to load
	 * 
	 * Gets the URL which is either set to load (if
	 * {@link WebPage::run() WebPage::run()} has not been called)
	 * or is loading (if it has been called).
	 * 
	 * @return					The URL which is either set to load or is currently loading.
	 */
	const char* url() const { return m_url; }
	
	/**
	 * Gets the Palm::WebView associated with this WebPage
	 * 
	 * It appears that this is some kind of wrapper for a WebKit
	 * object which is responsible for drawing to an output
	 * buffer.
	 * 
	 * @todo Document this once Palm::WebView is publicly documented.  It appears to be responsible for actually drawing the web page to an output buffer.
	 * 
	 * @return					The Palm::WebView instance associated with this WebPage.
	 */
	Palm::WebView* webkitView() const { return m_webView; }
	
	/**
	 * Gets the Palm::WebPage associated with this WebPage
	 * 
	 * It appears that this is some kind of wrapper for a WebKit
	 * object, the function of which is currently unknown.
	 * 
	 * @todo Document this once Palm::WebPage is publicly documented.
	 * 
	 * @return					The Palm::WebPage instance associated with this WebPage.
	 */
	Palm::WebPage* webkitPage() const { return m_webPage; }
	
	/**
	 * Sets the launch parameters for the main WebFrame for this WebPage
	 * 
	 * @todo Document this once {@link WebFrame WebFrame} is documented.
	 * 
	 * @param	args				Arguments, in JSON string format.
	 */
	void setArgs(const char* args);
	
	/**
	 * Relaunches a Mojo app loaded in the current WebPage instance
	 * 
	 * @todo Figure out, based on what uses this, what purpose it serves once more of the webbase/ classes are documented.
	 * @todo Document this more fully once Palm::WebPage is documented publicly.
	 * 
	 * @param	args				Arguments to pass into the frame, in JSON string format.  See WebPage::setArgs().
	 * @param	launchingAppId			App ID of the process requesting the relaunch.  See WebPage::setLaunchingAppId().
	 * @param	launchingProcId			Process ID of the process requesting the relaunch.  See WebPage::setLaunchingProcessId().
	 */
	bool relaunch(const char* args, const char* launchingAppId, const char* launchingProcId);
	
	/**
	 * Calls inspect() on the Palm::WebPage instance associated with this WebPage object
	 * 
	 * @todo Document this more fully once Palm::WebPage is documented publicly.
	 */
	void inspect();
	
	/**
	 * Gets the GLib main event loop associated with this WebPage instance
 	 * 
	 * Calls webkit_palm_get_mainloop().
	 * 
	 * @todo Document this once webkit_palm_get_mainloop() is documented.
	 * 
	 * @see webkit_palm_get_mainloop()
	 * 
	 * @return					Main GLib event loop associated with this WebPage instance
	 */
	GMainLoop* mainLoop() const;
	
	/**
	 * Gets page loading progress percentage
	 * 
	 * As the page is loaded, this percentage is updated with the
	 * current completion percentage of loading of the requested
	 * page.  Starts at 0 and progresses to 100 as the page is
	 * loaded.
	 * 
	 * @return					Percentage in the range [0...100] indicating how much of the page being loaded has already loaded.
	 */
	uint32_t progress() const { return m_progress; }
	
	/**
	 * Checks whether or not text editing mode for filling in an <input> is currently active
	 * 
	 * Mechanics of this are not fully understood.
	 * 
	 * When editing is disabled, so it automatic capitalization.
	 * 
	 * @return					Whether or not we're currently in text editing mode.
	 */
	bool isEditing() const { return m_manualFocusEnabled ? m_isManualEditorFocused 
                                                         : (m_isEditorFocused || m_isExplicitEditorFocused); }
	
	/**
	 * Unknown at this time
	 * 
	 * Mechanics of this are not fully understood.  As such, the
	 * purpose for it is also currently unknown.
	 * 
	 * @todo Figure out what this is for.
	 * 
	 * @return					Unknown for certain.
	 */
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
	
	/**
	 * Unknown at this time
	 * 
	 * Has something to do with automatic capitalization.
	 * 
	 * @todo Figure out the purpose for this.
	 * 
	 * @return					Unknown at this time.
	 */
	bool lastAutoCap() const { return m_lastAutoCap; }
	
	/**
	 * Copies currently select text to the clipboard
	 * 
	 * Issues a copy() call to the Palm::WebView associated with
	 * this WebPage instance.
	 * 
	 * @todo Document this more fully once Palm::WebView is publicly documented.
	 */
	void copy();
	
	/**
	 * Cuts currently select text to the clipboard
	 * 
	 * Issues a cut() call to the Palm::WebView associated with
	 * this WebPage instance.
	 * 
	 * @todo Document this more fully once Palm::WebView is publicly documented.
	 */
	void cut();
	
	/**
	 * Pastes clipboard into field currently being edited
	 * 
	 * Issues a paste() call to the Palm::WebView associated with
	 * this WebPage instance.
	 * 
	 * @todo Document this more fully once Palm::WebView is publicly documented.
	 */
	void paste();
	
	/**
	 * Selects all text within the current field or view
	 * 
	 * Issues a selectAll() call to the Palm::WebView associated with
	 * this WebPage instance.
	 * 
	 * @todo Document this more fully once Palm::WebView is publicly documented.
	 */
	void selectAll();
	
	/**
	 * Gets the width of the parent page when this one was opened
	 * 
	 * @todo Document this further once users of WebPage::createPage() are found, since they are what set the width.  That it's documented as being from the parent is an assumption at this point.
	 * 
	 * @see WebPage::setParent()
	 * @see WebPage::addChild()
	 * 
	 * @return					Width, in pixels, of the WebPage instance which opened this one.
	 */
	int requestedWidth() const { return m_requestedWidth; }
	
	/**
	 * Gets the height of the parent page when this one was opened
	 * 
	 * @todo Document this further once users of WebPage::createPage() are found, since they are what set the height.  That it's documented as being from the parent is an assumption at this point.
	 * 
	 * @see WebPage::setParent()
	 * @see WebPage::addChild()
	 * 
	 * @return					Height, in pixels, of the WebPage instance which opened this one.
	 */
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
