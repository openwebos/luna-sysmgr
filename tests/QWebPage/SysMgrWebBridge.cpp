#include "SysMgrWebBridge.h"

#include <QDebug>
#include <pbnjson.hpp>

SysMgrWebBridge::SysMgrWebBridge(bool viewable) : m_page(0),
                                                  m_viewable(viewable),
                                                  m_isShuttingDown(false),
                                                  m_stageReadyPending(false),
                                                  m_activityId(-1)
{
    commonSetup();
}

SysMgrWebBridge::SysMgrWebBridge(bool viewable, QUrl url) : m_page(0),
                                                            m_progress(0),
                                                            m_viewable(viewable),
                                                            m_isShuttingDown(false),
                                                            m_stageReadyPending(false),
                                                            m_activityId(-1)
{
    commonSetup();
    m_page->mainFrame()->load(url);
    setupStageArgs(url);
}

void SysMgrWebBridge::commonSetup()
{
    // TODO: PalmIME field types
    m_page = new SysMgrWebPage;
    if (m_viewable) {
        (new QWebView())->setPage(m_page);
        m_page->view()->hide();
    }
    QWebFrame* frame = m_page->mainFrame();

    connect(frame, SIGNAL(contentsSizeChanged(const QSize&)), this, SIGNAL(signalResizedContents(const QSize&)));
    connect(m_page, SIGNAL(repaintRequested(const QRect&)), this, SIGNAL(signalInvalidateRect(const QRect&)));
    connect(m_page, SIGNAL(linkClicked(const QUrl&)), this, SIGNAL(signalLinkClicked(const QUrl&)));
    connect(m_page, SIGNAL(repaintRequested(const QRect&)), this, SLOT(slotRepaint(const QRect&)));
    connect(frame, SIGNAL(titleChanged(const QString&)), this, SIGNAL(signalTitleChanged(const QString&)));
    connect(frame, SIGNAL(urlChanged(const QUrl&)), this, SIGNAL(signalUrlChanged(const QUrl&)));
    connect(frame, SIGNAL(urlChanged(const QUrl&)), this, SLOT(slotSetupPage(const QUrl&)));

    connect(m_page, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
    connect(m_page, SIGNAL(loadProgress(int)), this, SLOT(slotLoadProgress(int)));
    connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished(bool)));
    connect(m_page, SIGNAL(viewportChangeRequested()), this, SLOT(slotViewportChangeRequested()));
    // TODO: tell sysmgr that we want touch
}

SysMgrWebBridge::~SysMgrWebBridge() 
{
    m_isShuttingDown = true;
    delete m_page;
    m_page = 0;
}

bool SysMgrWebBridge::relaunch(const char* args, const char* launchingAppId, const char* launchingProcId)
{
    return true;
}

void SysMgrWebBridge::setArgs(const char* args)
{
    m_args = QString(args);
    setupStageArgs(args);
}


void SysMgrWebBridge::cut()
{
    if (m_page)
        m_page->triggerAction(QWebPage::Cut);
}

void SysMgrWebBridge::copy()
{
    // TODO: old webpage notified webappmgr that it copied something
    if (m_page)
        m_page->triggerAction(QWebPage::Copy);
}

void SysMgrWebBridge::paste()
{
    // TODO: old webpage notified webappmgr that it pasted something
    if (m_page)
        m_page->triggerAction(QWebPage::Paste);
}

void SysMgrWebBridge::selectAll()
{
    if (m_page)
        m_page->triggerAction(QWebPage::SelectAll);
}

void SysMgrWebBridge::setupStageArgs(const QUrl url) 
{
    qDebug() << "parsing url for args" << url;
    m_stageArgs.clear();
    if (url.hasQuery()) {
        QList<QPair<QString,QString> > items = url.queryItems();
        for (QList<QPair<QString, QString> >::const_iterator it = items.constBegin(); it != items.end(); it++) {
            QPair<QString, QString> item = *it;
            qDebug() << item;
            m_stageArgs[item.first] = item.second;
        }
    }
}

void SysMgrWebBridge::setupStageArgs(const char* json) 
{
    // args is a json object
    m_stageArgs.clear();
    if (json) {
        pbnjson::JDomParser parser;
        parser.parse(json, pbnjson::JSchemaFragment("{}"));
        pbnjson::JValue root = parser.getDom();
        for (pbnjson::JValue::ObjectIterator pair = root.begin(); pair != root.end(); pair++) {
            std::string key;

            std::string valueString;
            int valueInt;
            bool valueBool;

            (*pair).first.asString(key);
            QString qKey = QString::fromStdString(key);

            if ((*pair).second.asString(valueString) == CONV_OK)
                m_stageArgs[qKey] = QString::fromStdString(valueString);
            else if ((*pair).second.asNumber(valueInt) == CONV_OK)
                m_stageArgs[qKey] = valueInt;
            else if ((*pair).second.asBool(valueBool) == CONV_OK)
                m_stageArgs[qKey] = valueBool;
        }
    }
}

void SysMgrWebBridge::setName(const char* name)
{
    m_name = QString(name);
}

void SysMgrWebBridge::addNewContentRequestId(const QString id)
{
    m_newContentRequestIds.insert(id);
}

void SysMgrWebBridge::removeNewContentRequestId(const QString id)
{
    m_newContentRequestIds.erase(m_newContentRequestIds.find(id));
}

int SysMgrWebBridge::activityId(void)
{
    if (parent()) {
        SysMgrWebBridge* bridgeParent = qobject_cast<SysMgrWebBridge*>(parent());
        if (bridgeParent)
            return bridgeParent->activityId();
    }

    return m_activityId;
}

// slots
void SysMgrWebBridge::slotLoadProgress(int progress) 
{
    m_progress = progress;
    qDebug() << __PRETTY_FUNCTION__ << progress;
}

void SysMgrWebBridge::slotLoadStarted() 
{
    qDebug() << __PRETTY_FUNCTION__;
}

void SysMgrWebBridge::slotLoadFinished(bool ok) 
{
    qDebug() << __PRETTY_FUNCTION__;
}

void SysMgrWebBridge::slotRepaint(const QRect& dirtyRect)
{
    qDebug() << "page invalidated @ " << dirtyRect;
}

void SysMgrWebBridge::slotViewportChangeRequested()
{
    QWebPage::ViewportAttributes attributes = m_page->viewportAttributesForSize(m_page->viewportSize());
    Q_EMIT signalViewportChanged(attributes);
}

void SysMgrWebBridge::slotSetupPage(const QUrl& url)
{
    setupStageArgs(url);

    QString windowType = url.queryItemValue("window");

    // set app id
    // set name
    // set width
    // set height

    // set launching app id
    // set launching process id

    // set parent (this should flatten out the heirarchy)

    // create and attach to WebAppBase
}

// Page

SysMgrWebPage::SysMgrWebPage(QObject* parent)
{
    settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
}

QWebPage* SysMgrWebPage::createWindow(WebWindowType type)
{
    SysMgrWebBridge* newWindow = new SysMgrWebBridge(true);
    newWindow->setParent(this);
    // TODO: need to add this to a list of pages in web app manager
    return newWindow->page();
}

bool SysMgrWebPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type) 
{
    // mimeHandoffUrl/mimeNotHandled
    return true;
}

void SysMgrWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceId)
{
    qDebug() << "At line" << lineNumber << ":" << message << "from" << sourceId;
}
