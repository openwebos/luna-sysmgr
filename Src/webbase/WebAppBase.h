/**
 * @file
 * 
 * Base class for different types of app containers to derive from.
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

#ifndef WebAppBase_h
#define WebAppBase_h

#include "SysMgrWebBridge.h"

#include <QObject>

#include <lunaservice.h>
#include <palmimedefines.h>

class ApplicationDescription;

/**
 * Base class for different types of app containers to derive from.
 */
class WebAppBase : public QObject {

    Q_OBJECT

    public:
	/**
	 * Initializes this app container
	 * 
	 * Just initializes things to blank values.  DOES
	 * NOT report to {@link WebAppManager WebAppManager}
	 * that we've launched.
	 */
        WebAppBase();
	
	/**
	 * Cleans up this app container
	 * 
	 * This cleans up any resources allocated within
	 * this app container.  It does not detach from or
	 * clean up the WebPage it's attached to - that is
	 * expected to be handled by the caller.
	 * 
	 * Implementation details:
	 * 
	 * - Deletes our app from the global app cache
	 *   ({@link WebAppCache WebAppCache}).
	 * - Lets {@link WebAppManager WebAppManager} know
	 *   that we're being deleted.
	 * - Cleans up resources, including deleting the
	 *   content WebPage from memory.
	 * - Disconnects from sensors.
	 * - Asks {@link WebAppManager WebAppManager} to
	 *   remove us from the headless watch list.
	 * 
	 * @note This deletes the attached WebPage from memory.  After deleting this WebAppBase, do not attempt to access any WebPage attached to this instance when it was destroyed.
	 */
        virtual ~WebAppBase();
	
	/**
	 * Attaches to a WebPage instance to allow us to manage it
	 * 
	 * This method sets up the content to display for
	 * this app.
	 * 
	 * In detail, this method:
	 * - Reports to {@link WebAppManager WebAppManager}
	 *   that this app is launched.
	 * - Creates a new activity.
	 * - Loads and applies app properties from the app
	 *   description.
	 * 
	 * It's assumed that this WebPage instance will
	 * either already have a URL loaded or it will be
	 * loaded by the caller after calling this.  In
	 * other words, this attaches a WebPage, but does
	 * not load anything into it.
	 * 
	 * @note If still attached when this WebAppBase instance is destroyed, this page will also be deleted from memory.  Do not access this page again after attaching it without getting a new pointer to it via {@link WebAppBase::page() WebAppBase::page()}.
	 * 
	 * @param	page			WebPage of content for this app.
	 */
        virtual void attach(SysMgrWebBridge*);
	
	/**
	 * Detaches this instance from the WebPage it was previously managing
	 * 
	 * This essentially removes the content from this
	 * app.
	 * 
	 * In detail, this method:
	 * - Reports to {@link WebAppManager WebAppManager}
	 *   that this app is closed.
	 * - Destroys the activity for the app.
	 * 
	 * @return				The previously-managed WebPage instance containing this app's content.
	 */
        virtual SysMgrWebBridge* detach();

        virtual void thawFromCache() { }
        virtual void freezeInCache() { }
        bool inCache() const { return m_inCache; }
        void markInCache(bool inCache) { m_inCache = inCache; }

        void setKeepAlive(bool keepAlive) { m_keepAlive = keepAlive; }
        bool keepAlive() { return m_keepAlive; }

        SysMgrWebBridge* page() const { return m_page; }

        virtual bool isWindowed() const { return false; }
        virtual bool isCardApp() const { return false; }
        virtual bool isChildApp() const { return false; }
        virtual bool isDashboardApp() const { return false; }
        virtual bool isAlertApp() const { return false; }

        void relaunch(const char* args, const char* launchingAppId, const char* launchingProcId);
        virtual void stagePreparing();
        virtual void stageReady();
	
	/**
	 * Gets this app's app ID
	 * 
	 * This is set up when this instance is attached
	 * to a page and gets the app ID of the page it's
	 * attached to.
	 * 
	 * @return				App ID of this app.
	 */
        QString appId() const { return m_appId; }
        QString processId() const { return m_processId; }
        QString url() const { return m_url; }

        ApplicationDescription* getAppDescription() { return m_appDesc; }
        void setAppDescription(ApplicationDescription*);

        void setManualEditorFocusEnabled(bool);
        virtual void setManualEditorFocus(bool focused, const PalmIME::EditorState&);
        virtual void setExplicitEditorFocus(bool focused, const PalmIME::EditorState & editorState);

        virtual void suspendAppRendering() { }
        virtual void resumeAppRendering() { }
        virtual void resizeWebPage(uint32_t width, uint32_t height);

    protected:
        virtual int  getKey() const { return 0; }
        virtual void focus() { }
        virtual void unfocus() { }
        virtual void close();
        virtual void windowSize(int& width, int& height) { width = 0; height = 0; }
        virtual void screenSize(int& width, int& height);
        virtual void resizedContents(int contentsWidth, int contentsHeight);
        virtual void zoomedContents(double scaleFactor, int contentsWidth, int contentsHeight,
                                    int newScrollOffsetX, int newScrollOffsetY);
        virtual void scrolledContents(int newContentsX, int newContentsY);
        virtual void uriChanged(const char* url);
        virtual void titleChanged(const char* title);
        virtual void statusMessage(const char* msg);
        virtual void dispatchFailedLoad(const char* domain, int errorCode,
                                        const char* failingURL, const char* localizedDescription);
        virtual void loadFinished() { }
        virtual void editorFocusChanged(bool focused, const PalmIME::EditorState& state) { }
	
	/**
	 * Enables/disables auto-capitalization in fields in this app
	 * 
	 * @note Currently doesn't do anything - see derived class to see if they actually did anything with this.
	 * 
	 * @param	enabled			true to enable auto-capitalization, false to disable it.
	 */
        virtual void autoCapEnabled(bool enabled) { }

	/**
	 * Asks the Activity Manager service to create an activity for us
	 * 
	 * @todo Document this further once the palm://com.palm.activitymanager/create IPC call is fully documented.
	 */
        void createActivity();
	
	/**
	 * Destroys our activity
	 * 
	 * @todo Fully document this once LSCallCancel() is documented.
	 */
        void destroyActivity();
        void focusActivity();
	
	/**
	 * Asks the Activity Manager service to unfocus our app
	 * 
	 * @todo Document this further once the palm://com.palm.activitymanager/unfocus IPC call is fully documented.
	 */
        void blurActivity();
	
	/**
	 * Cleans up this app
	 * 
	 * Implementation details:
	 * - Lets {@link WebAppManager WebAppManager}
	 *   know that the app is closed.
	 * - Destroys the activity that was set up in
	 *   {@link WebAppBase::attach() WebAppBase::attach()}.
	 * - Deletes the page.
	 * - Deletes the desc image.
	 * 
	 * @todo Document this more fully once we know what a "desc image" is.
	 */
        void cleanResources();

        void setAppId(const QString& appId) { m_appId = appId; }

    protected Q_SLOTS:
        virtual void uriChanged(const QUrl&);

    private:
        // app description

        SysMgrWebBridge* m_page;
        bool m_inCache;
        bool m_keepAlive;

        QString m_appId;
        QString m_processId;
        QString m_url;

        ApplicationDescription* m_appDesc;

        LSMessageToken m_activityManagerToken;

        friend class PalmSystem;
        friend class SysMgrWebBridge;
};

#endif
