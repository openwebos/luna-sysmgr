#ifndef WebAppBase_h
#define WebAppBase_h

#include "SysMgrWebBridge.h"

#include <QObject>

#include <palmimedefines.h>

class ApplicationDescription;

class WebAppBase : public QObject {

    Q_OBJECT

    public:
        WebAppBase();
        virtual ~WebAppBase();
        virtual void attach(SysMgrWebBridge*);
        virtual void detach();

        virtual void thawFromCache() { }
        virtual void freezeInCache() { }
        void markInCache(bool inCache) { m_inCache = inCache; }

        void setKeepAlive(bool keepAlive) { m_keepAlive = keepAlive; }
        bool keepAlive() { return m_keepAlive; }

        SysMgrWebBridge* page() { return m_page; }

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

        virtual void suspendAppRendering() { }
        virtual void resumeAppRendering() { }

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
        virtual void needTouchEvents(bool needTouchEvents) { }

        void createActivity();
        void destroyActivity();
        void focusActivity();
        void blurActivity();
        void cleanResources();

        void destroyAllSensors();
        virtual Event::Orientation orientationForThisCard(Event::Orientation orient) { return orient; }
        virtual Event::Orientation postProcessOrientationEvent(Event::Orientation aInputEvent) { return aInputEvent; }

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
#if defined(HAS_NYX)
        int m_OrientationAngle;
#endif
};

#endif
