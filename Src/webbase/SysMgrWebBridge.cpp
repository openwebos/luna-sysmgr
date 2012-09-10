#include "SysMgrWebBridge.h"

#include "Logging.h"
#include "PalmSystem.h"
#include "Utils.h"
#include "WebAppBase.h"
#include "WebAppManager.h"

#include <QDebug>

#include <cjson/json.h>
#include <pbnjson.hpp>


SysMgrWebBridge::SysMgrWebBridge(bool viewable) : m_page(0),
                                                  m_client(0),
                                                  m_progress(0),
                                                  m_viewable(viewable),
                                                  m_isShuttingDown(false),
                                                  m_stageReadyPending(false),
                                                  m_activityId(-1),
                                                  m_launchedAtBoot(false)
{
    commonSetup();
}

SysMgrWebBridge::SysMgrWebBridge(bool viewable, QUrl url) : m_page(0),
                                                            m_client(0),
                                                            m_progress(0),
                                                            m_viewable(viewable),
                                                            m_isShuttingDown(false),
                                                            m_stageReadyPending(false),
                                                            m_activityId(-1),
                                                            m_url(url),
                                                            m_launchedAtBoot(false)
{
    commonSetup();
    setupStageArgs(url);
}

void SysMgrWebBridge::commonSetup()
{
    // TODO: PalmIME field types
    m_page = new SysMgrWebPage(this);
    QWebFrame* frame = m_page->mainFrame();

    if (m_viewable) {
    }
    frame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    frame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

    connect(frame, SIGNAL(contentsSizeChanged(const QSize&)), this, SIGNAL(signalResizedContents(const QSize&)));
    connect(m_page, SIGNAL(geometryChangeRequested(const QRect&)), this, SIGNAL(signalGeometryChanged(const QRect&)));
    connect(m_page, SIGNAL(repaintRequested(const QRect&)), this, SIGNAL(signalInvalidateRect(const QRect&)));
    connect(m_page, SIGNAL(linkClicked(const QUrl&)), this, SIGNAL(signalLinkClicked(const QUrl&)));
    connect(frame, SIGNAL(titleChanged(const QString&)), this, SIGNAL(signalTitleChanged(const QString&)));
//    connect(frame, SIGNAL(urlChanged(const QUrl&)), this, SIGNAL(signalUrlChanged(const QUrl&)));
    connect(frame, SIGNAL(urlChanged(const QUrl&)), this, SLOT(slotSetupPage(const QUrl&)));

    connect(m_page, SIGNAL(loadStarted()), this, SLOT(slotLoadStarted()));
    connect(m_page, SIGNAL(loadProgress(int)), this, SLOT(slotLoadProgress(int)));
    connect(m_page, SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished(bool)));
    connect(m_page, SIGNAL(viewportChangeRequested()), this, SLOT(slotViewportChangeRequested()));

    connect(frame, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(slotJavaScriptWindowObjectCleared()));

    // Listening on microFocusChanged() is an expensive operation that can be avoided.
    connect(m_page, SIGNAL(microFocusChanged()), this, SLOT(slotMicroFocusChanged()));

    // TODO: tell sysmgr that we want touch

    ::memset(&m_explicitEditorState, 0, sizeof(PalmIME::EditorState));
    m_explicitEditorState.type = PalmIME::FieldType_Text;

    ::memset(&m_manualEditorState, 0, sizeof(PalmIME::EditorState));
    m_manualEditorState.type = PalmIME::FieldType_Text;

    m_manualFocusEnabled = false;

    m_inRelaunch = false;

    // Set focus explicitly
    QFocusEvent fEvent(QEvent::FocusIn);
    page()->event(&fEvent);

}

SysMgrWebBridge::~SysMgrWebBridge() 
{
    m_isShuttingDown = true;
    delete m_page;
    m_page = 0;
}

const QRect SysMgrWebBridge::requestedGeometry() const
{
    return page() ? page()->requestedGeometry() : QRect();
}

void SysMgrWebBridge::explicitEditorFocused(bool focused, const PalmIME::EditorState& editorState)
{
        if (!focused) {
            QFocusEvent fEvent(QEvent::FocusOut);
            page()->event(&fEvent);
        }
}

void SysMgrWebBridge::manualEditorFocused(bool focused, const PalmIME::EditorState& editorState)
{
        if (!focused) {
            QFocusEvent fEvent(QEvent::FocusOut);
            page()->event(&fEvent);
        }
}

void SysMgrWebBridge::manualFocusEnabled(bool enabled)
{
    /* This disrupts shouldBlink=TRUE and stops the cursor from blinking */
    
       /*
        if (!enabled) {
	    fprintf(stderr, "%s SENDING FOCUS OUT\n", __PRETTY_FUNCTION__);
            QFocusEvent fEvent(QEvent::FocusOut);
            page()->event(&fEvent);
        }
        */
}

bool SysMgrWebBridge::relaunch(const char* args, const char* launchingAppId, const char* launchingProcId)
{
    if (isShuttingDown())
        return false;

    // If the headless app is still loading, buffer this until our progress is complete.
    if (progress() < 100)
    {
        m_bufferedRelaunchArgs = args;
        m_bufferedRelaunchLaunchingAppId = launchingAppId;
        m_bufferedRelaunchLaunchingProcId = launchingProcId;
        return true;
    }


    m_args = QString(args);
    setLaunchingAppId(QString(launchingAppId));
    setLaunchingProcessId(QString(launchingProcId));

    if (m_jsObj)
        m_jsObj->setLaunchParams(m_args);

    m_inRelaunch = true;
    QVariant ret = m_page->mainFrame()->evaluateJavaScript(QString("Mojo.relaunch()"));
    m_inRelaunch = false;
    return ret.isValid() && ret.toBool();
}

void SysMgrWebBridge::slotMicroFocusChanged()
{
    //Set focus explicitly
    QFocusEvent fEvent(QEvent::FocusIn);
    page()->event(&fEvent);
}

void SysMgrWebBridge::setArgs(const char* args)
{
    m_args = QString(args);
    setupStageArgs(args);
    if(m_stageArgs.value("launchedAtBoot").toBool())
        m_launchedAtBoot = true;
    if (m_jsObj)
        m_jsObj->setLaunchParams(m_args);
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
    m_stageArgs.clear();
    if (url.hasQuery()) {
        QList<QPair<QString, QString> > items = url.queryItems();
        for (QList<QPair<QString, QString> >::const_iterator it = items.constBegin(); it != items.end(); it++) {
            QPair<QString, QString> item = *it;
            m_stageArgs[item.first] = item.second;
        }
    }
}

void SysMgrWebBridge::setupStageArgs(const char* json) 
{
    // args is a json object
    m_stageArgs.clear();

    if (json) {
        json_object* root = json_tokener_parse(json);
        if (root && !is_error(root) && json_object_get_type(root) == json_type_object) {

            json_object_object_foreach(root, key, val) 
            {

                if (!key || !val)
                    continue;

                switch (json_object_get_type(val)) {
                case json_type_boolean:
                    m_stageArgs[QString::fromStdString(key)] = QVariant((bool) json_object_get_boolean(val));
                    break;
                case json_type_int:
                    m_stageArgs[QString::fromStdString(key)] = QVariant((int) json_object_get_int(val));
                    break;
                case json_type_double:
                    m_stageArgs[QString::fromStdString(key)] = QVariant((double) json_object_get_double(val));
                    break;
                case json_type_string: {
                    std::string s = json_object_get_string(val);
                    m_stageArgs[QString::fromStdString(key)] = QVariant(QString::fromUtf8(s.c_str()));
                    break;
                }
                default:
                    qWarning() << __PRETTY_FUNCTION__ << ": Unknown stage argument type: " << json_object_get_type(val);
                    break;
                }
            }

            json_object_put(root);
        }
    }
}

void SysMgrWebBridge::addPalmSystemObject(void)
{
    QWebFrame* frame = m_page->mainFrame();

    if (!frame)
        qDebug() << "couldn't find the main frame?";

    m_jsObj = new PalmSystem(this);
    frame->addToJavaScriptWindowObject("PalmSystem", m_jsObj);

    frame->evaluateJavaScript("function palmGetResource(a,b) { return PalmSystem.getResource(a,b); }");
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

void SysMgrWebBridge::setComposingText(const char* text)
{
}

void SysMgrWebBridge::commitComposingText()
{
}

void SysMgrWebBridge::commitText(const char* text)
{
    QKeyEvent fEvent(QEvent::KeyPress, 0, Qt::NoModifier, QString(text));
    page()->event(&fEvent);
}

void SysMgrWebBridge::performEditorAction(PalmIME::FieldAction action)
{
}

void SysMgrWebBridge::removeInputFocus()
{
    m_client->editorFocusChanged(false, PalmIME::EditorState());
}

void SysMgrWebBridge::createViewForWindowlessPage()
{
    // only headless apps should create shell cards
    if (!m_viewable)
        return;

    if (!m_args.isEmpty()) {
        if (m_args.contains("mojoCrossPush"))
            return;
        if (m_args.contains("$disableCardPreLaunch"))
            return;
    }

    SysMgrWebBridge* shell = new SysMgrWebBridge(true);
    shell->setAppId(appId());
    shell->setLaunchingAppId(launchingAppId());
    shell->setLaunchingProcessId(launchingProcessId());

    shell->setParent(this);

    WebAppManager* wam = WebAppManager::instance();
    Window::Type winType = Window::Type_Card;

    shell->m_client = wam->launchWithPageInternal(shell, winType, m_client ? m_client->getAppDescription() : 0);
    if (!shell->m_client)
        shell->closePageSoon();
    wam->shellPageAdded(shell);
}

void SysMgrWebBridge::closePageSoon()
{

    WebAppManager::instance()->closePageSoon(this);
}

const char* SysMgrWebBridge::getIdentifier()
{
    if (m_identifier.empty())
        m_identifier = windowIdentifierFromAppAndProcessId(appId().toAscii().constData(), processId().toAscii().constData());

    return m_identifier.c_str();
}

// slots
void SysMgrWebBridge::slotLoadProgress(int progress) 
{
    m_progress = progress;
}

void SysMgrWebBridge::slotLoadStarted() 
{
}

void SysMgrWebBridge::slotLoadFinished(bool ok) 
{
}

void SysMgrWebBridge::slotJavaScriptWindowObjectCleared()
{
    addPalmSystemObject();
}

void SysMgrWebBridge::slotViewportChangeRequested()
{
    QWebPage::ViewportAttributes attributes = m_page->viewportAttributesForSize(m_page->viewportSize());
    Q_EMIT signalViewportChanged(attributes);
}

void SysMgrWebBridge::slotSetupPage(const QUrl& url)
{
    QString attributes = m_page->attributes();
    if (!attributes.isEmpty())
        setArgs(attributes.toUtf8().constData());
    else
        setupStageArgs(url);
    QString windowType = m_stageArgs.value("window").toString();
    SysMgrWebBridge* parent = 0;
    ApplicationDescription* appDesc = 0;
    if((parent = qobject_cast<SysMgrWebBridge*>(this->parent()))) 
    {
        appDesc = parent->m_client ? parent->m_client->getAppDescription() : 0;
    }
    WebAppManager* wam = WebAppManager::instance();
    if(windowType == "dashboard") {
        wam->launchWithPageInternal(this, Window::Type_Dashboard, appDesc);
    } else if(windowType == "banneralert") {
        wam->launchWithPageInternal(this, Window::Type_BannerAlert, appDesc);
    } else if(windowType == "popupalert") {
        wam->launchWithPageInternal(this, Window::Type_PopupAlert, appDesc);
    } else if(windowType == "menu") {
        wam->launchWithPageInternal(this, Window::Type_Menu, appDesc);
    } else if(windowType == "emergency") {
        wam->launchWithPageInternal(this, Window::Type_Emergency, appDesc);
    } else if(windowType == "childcard") {
        // the identifier of the parent window should have been passed down to us through the
        // url query params

        std::string parentIdentifier;
        std::string launchingAppId;
        std::string launchingProcessId;

        StringVariantMap::const_iterator it = m_stageArgs.find("parentidentifier");
        if (it != m_stageArgs.end())
            parentIdentifier = it.value().toString().toStdString();

        if (parentIdentifier.empty() || !splitWindowIdentifierToAppAndProcessId(parentIdentifier, launchingAppId, launchingProcessId)) {

            // hmmm... no. we just close ourselves
            qCritical() << "No parent identifier passed for child window creation";
            closePageSoon();
            return;
        }

        setLaunchingAppId(QString::fromStdString(launchingAppId));
        setLaunchingProcessId(QString::fromStdString(launchingProcessId));

        WebAppBase* app = wam->findApp(QString::fromStdString(launchingProcessId));
        if (app)
            wam->launchWithPageInternal(this, Window::Type_ChildCard, appDesc);
    } else if(windowType == "pin") {
        wam->launchWithPageInternal(this, Window::Type_PIN, appDesc);
    } else if(windowType == "dockmode") {
        wam->launchWithPageInternal(this, Window::Type_DockModeWindow, appDesc);
    } else if(windowType == "modalwindow") {
        wam->launchWithPageInternal(this, Window::Type_ModalChildWindowCard, appDesc);
    } else { // card
        if(parent) {
            if(parent->m_launchedAtBoot) {
                parent->m_launchedAtBoot = false;
                this->deleteLater();
            }
            else
                wam->launchWithPageInternal(this, Window::Type_Card, appDesc);
        }
    }
    // set app id
    // set name
    // set width
    // set height

    // set launching app id
    // set launching process id

    // set parent (this should flatten out the heirarchy)

    // create and attach to WebAppBase
    //
    Q_EMIT signalUrlChanged(url);
}

// Page

SysMgrWebPage::SysMgrWebPage(QObject* parent) : QWebPage(parent)
{
    const char* defaultStoragePath = "/media/cryptofs/.sysmgr";
    const char* envStoragePath = ::getenv("PERSISTENT_STORAGE_PATH");
    const char* storagePath = envStoragePath ? envStoragePath : defaultStoragePath;
    settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    settings()->setAttribute(QWebSettings::JavascriptCanCloseWindows, true);
    settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, true);
    settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
    settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    settings()->setPluginSupplementalPath("/usr/lib/BrowserPlugins");
    settings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    settings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);
    settings()->setIconDatabasePath(storagePath);
    settings()->setOfflineWebApplicationCachePath(storagePath);
    settings()->setOfflineStoragePath(QString("%1/Databases").arg(storagePath));
    settings()->setLocalStoragePath(QString("%1/LocalStorage").arg(storagePath));

    connect(this, SIGNAL(geometryChangeRequested(const QRect&)), this, SLOT(setRequestedGeometry(const QRect&)));

    if (getenv("LUNA_TILEDBACKINGSTORE")) {
        // FIXME still broken - produces empty windows
        qDebug() << "enabling Tiled Backing Store";
        settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, true);
    }
}

void SysMgrWebPage::setRequestedGeometry(const QRect& rect)
{
    m_requestedGeometry = rect;
}

const QRect SysMgrWebPage::requestedGeometry() const
{
    return m_requestedGeometry;
}

QWebPage* SysMgrWebPage::createWindow(WebWindowType type)
{
    SysMgrWebBridge* newWindow = new SysMgrWebBridge(true);
    newWindow->setParent(parent());
    newWindow->setAppId(qobject_cast<SysMgrWebBridge*>(parent())->appId());
    newWindow->setLaunchingAppId(qobject_cast<SysMgrWebBridge*>(parent())->launchingAppId());
    newWindow->setProcessId(qobject_cast<SysMgrWebBridge*>(parent())->processId());

    WebAppManager::instance()->webPageAdded(newWindow);
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
    QString appId;
    if(qobject_cast<SysMgrWebBridge*>(parent()))
        appId = qobject_cast<SysMgrWebBridge*>(parent())->appId();
    luna_syslogV(syslogContextJavascript(), G_LOG_LEVEL_MESSAGE, "%s: %s, %s:%d", appId.toStdString().c_str(), message.toStdString().c_str(), sourceId.toStdString().c_str(), lineNumber);

#ifdef ENABLE_JS_DEBUG_VERBOSE
     qDebug() << "\nJSConsole: [App" << appId.toStdString().c_str() << "] " << message.toStdString().c_str() << "[source]" << sourceId.toStdString().c_str() << "[line]" << lineNumber;
#endif
}
