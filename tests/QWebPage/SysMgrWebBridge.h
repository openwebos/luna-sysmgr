
#ifndef SysMgrWebBridge_h
#define SysMgrWebBridge_h

#include <QtWebKit>

typedef QMap<QString, QVariant> StringVariantMap;

class SysMgrWebPage : public QWebPage {
    Q_OBJECT

    public:
        SysMgrWebPage(QObject* parent = 0);
        virtual ~SysMgrWebPage() { };

        QWebPage* createWindow(WebWindowType);

    protected:
        virtual bool acceptNavigationRequest(QWebFrame*, const QNetworkRequest&, NavigationType);
        virtual void javaScriptConsoleMessage(const QString&, int lineNumber, const QString& sourceID);

    private:
};


class SysMgrWebBridge : public QObject {
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

    Q_SIGNALS:
        void signalResizedContents(const QSize&);
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
        void slotRepaint(const QRect&);

    private:
        SysMgrWebPage* m_page;
        int m_progress;
        bool m_viewable;
        QString m_args;
        StringVariantMap m_stageArgs;
        QString m_name;
        bool m_isShuttingDown;
        bool m_stageReadyPending;
        QSet<QString> m_newContentRequestIds;
        int m_activityId;

        void commonSetup();
        void setupStageArgs(const QUrl);
        void setupStageArgs(const char* url);
};

#endif
