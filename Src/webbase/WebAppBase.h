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

#ifndef WebAppBase_h
#define WebAppBase_h

#include "SysMgrWebBridge.h"

#include <QObject>

#include <lunaservice.h>
#include <palmimedefines.h>

class ApplicationDescription;

class WebAppBase : public QObject {

    Q_OBJECT

    public:
        WebAppBase();
        virtual ~WebAppBase();
        virtual void attach(SysMgrWebBridge*);
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
        virtual void autoCapEnabled(bool enabled) { }

        void createActivity();
        void destroyActivity();
        void focusActivity();
        void blurActivity();
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
