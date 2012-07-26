
#ifndef SysMgrWebBridge_h
#define SysMgrWebBridge_h

#include "ProcessBase.h"

#include <QtWebKit>
#include <QPointer>

#include <palmimedefines.h>

typedef QMap<QString, QVariant> StringVariantMap;
class WebAppBase;
class PalmSystem;

class SysMgrWebPage : public QWebPage {
    Q_OBJECT

    public:
        SysMgrWebPage(QObject* parent = 0);
        virtual ~SysMgrWebPage() { };

        QWebPage* createWindow(WebWindowType);

        /*!
          \fn QRect requestedGeometry();
          \brief Returns the requested geometry of the window.

          For example the following javascript would cause the requested geometry to be set.
          \code
            <script type="text/javascript">
                myWindow=window.open('','','width=200, height=100')
            </script>
          \endcode
         */
        const QRect requestedGeometry() const;

    protected:
        virtual bool acceptNavigationRequest(QWebFrame*, const QNetworkRequest&, NavigationType);
        virtual void javaScriptConsoleMessage(const QString&, int lineNumber, const QString& sourceID);

    protected Q_SLOTS:
        void setRequestedGeometry(const QRect&);

    private:
        /*! 
            Holds the windows geometry. We need to 'cache' this as QtWebKit does not, it only emits 
            it and without caching it we miss it.
        */
        QRect m_requestedGeometry;
};


class SysMgrWebBridge : public QObject,
                        public ProcessBase {
    Q_OBJECT

    public:
        SysMgrWebBridge(bool viewable);
        SysMgrWebBridge(bool viewable, QUrl);
        virtual ~SysMgrWebBridge();

        SysMgrWebPage* page() const { return m_page; }
        int progress() const { return m_progress; }
        QUrl url() const { return m_page->mainFrame()->url(); }
        bool relaunch(const char* args, const char* launchingAppId, const char* launchingProcId);
        void setArgs(const char*);
        void cut();
        void copy();
        void paste();
        void selectAll();
        void setName(const char*);
        bool isShuttingDown() { return m_isShuttingDown; }
        QString name() { return m_name; }
        void setStageReadyPending(bool pending) { m_stageReadyPending = pending; }
        bool stageReadyPending() const { return m_stageReadyPending; }
        void addNewContentRequestId(const QString);
        void removeNewContentRequestId(const QString);
        int activityId();
        void setActivityId(int id) { m_activityId = id; }

        void setComposingText(const char*);
        void commitComposingText();
        void commitText(const char*);
        void performEditorAction(PalmIME::FieldAction);
        void removeInputFocus();

        const StringVariantMap& stageArguments() const { return m_stageArgs; }
        /*!
            \fn const QRect requestedGeometry() const;
            \brief Returns the underlying web page's requested geometry.
        */
        const QRect requestedGeometry() const;

        void inspect() { }

        void createViewForWindowlessPage();

        WebAppBase* getClient(void) { return m_client; }
        void setClient(WebAppBase* client) { m_client = client; }

        void load() 
        {
            if (m_page)
                m_page->mainFrame()->load(m_url);
        }
        const char* getIdentifier();

    private:
        virtual void explicitEditorFocused(bool, const PalmIME::EditorState&);
        virtual void manualEditorFocused(bool, const PalmIME::EditorState&);
        virtual void manualFocusEnabled(bool);

        void closePageSoon();

    Q_SIGNALS:
        void signalResizedContents(const QSize&);
        void signalGeometryChanged(const QRect&);
        void signalInvalidateRect(const QRect&);
        void signalLinkClicked(const QUrl&);
        void signalTitleChanged(const QString&);
        void signalUrlChanged(const QUrl&);
        void signalViewportChanged(const QWebPage::ViewportAttributes&);

    protected Q_SLOTS:
        void slotLoadProgress(int progress);
        void slotLoadStarted();
        void slotLoadFinished(bool ok);
        void slotViewportChangeRequested();
        void slotSetupPage(const QUrl&);
        void slotJavaScriptWindowObjectCleared();
        void slotMicroFocusChanged();

    private:
        SysMgrWebPage* m_page;
        WebAppBase* m_client;
        int m_progress;
        bool m_viewable;
        QString m_args;
        StringVariantMap m_stageArgs;
        QString m_name;
        bool m_isShuttingDown;
        bool m_stageReadyPending;
        QSet<QString> m_newContentRequestIds;
        int m_activityId;
        QUrl m_url;
        std::string m_identifier;
        bool m_launchedAtBoot;

        PalmIME::EditorState m_explicitEditorState;
        PalmIME::EditorState m_manualEditorState;
        bool m_manualFocusEnabled;

        bool m_inRelaunch;

        QPointer<PalmSystem> m_jsObj;

        QString m_bufferedRelaunchArgs;
        QString m_bufferedRelaunchLaunchingAppId;
        QString m_bufferedRelaunchLaunchingProcId;

        void commonSetup();
        void setupStageArgs(const QUrl);
        void setupStageArgs(const char* url);
        void addPalmSystemObject();

        friend class WebAppBase;
};

#endif
