
#include "PalmSystem.h"

#include "AlertWebApp.h"
#include "BannerMessageEventFactory.h"
#include "CardWebApp.h"
#include "DeviceInfo.h"
#include "KeyboardMapping.h"
#include "Localization.h"
#include "NewContentIndicatorEventFactory.h"
#include "Preferences.h"
#include "Settings.h"
#include "SysMgrWebBridge.h"
#include "WebAppBase.h"
#include "WebAppManager.h"

#include <QtWebKit>
#include <QDebug>

#include <pbnjson.hpp>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

PalmSystem::PalmSystem(SysMgrWebBridge* bridge)
    : m_bridge(bridge)
    , m_hasAlphaHole(false)
    , m_windowOrientation("free")
{
}

QString PalmSystem::getIdentifier()
{
    return m_bridge->getIdentifier();
}

QString PalmSystem::nameForOrientation(Event::Orientation o)
{
    switch (o) {
    case (Event::Orientation_Down): return "down";
    case (Event::Orientation_Left): return "left";
    case (Event::Orientation_Right): return "right";
    default: break;
    }

    return "up";
}

void PalmSystem::setLaunchParams(const QString& params)
{
    QString p = params;
    if (!params.isEmpty()) {
        pbnjson::JDomParser parser;
        if (parser.parse(p.toAscii().constData(), pbnjson::JSchemaFragment("{}"))) {
            pbnjson::JValue root = parser.getDom();
            if (root.isObject() && root.begin() == root.end())
                p = QString();
        }
    }
    m_launchParams = p;
}

// methods exposed to javascript
QString PalmSystem::addBannerMessage(const QString& msg, const QString& params, 
                                  const QString& icon, const QString& soundClass, 
                                  const QString& soundFile, int duration, bool doNotSuppress)
{
     qDebug() << "addBannerMessage("
             << "msg:" << msg
             << ", params: " << params
             << ", icon: " << icon
             << ", soundClass: " << soundClass
             << ", soundFile: " << soundFile
             << ", duration: " << duration
             << ", doNotSuppress: " << doNotSuppress
             << ")";

    if (!m_bridge)
        return QString("");

    BannerMessageEvent* e = BannerMessageEventFactory::createAddMessageEvent(m_bridge->appId().toStdString(),
                                                                             msg.toStdString(), params.toStdString(),
                                                                             icon.toStdString(), soundClass.toStdString(), soundFile.toStdString(),
                                                                             duration, doNotSuppress);
    if (!e)
        return QString();

    // Send the message ID back to the JS side
    WebAppManager::instance()->sendAsyncMessage(new ViewHost_BannerMessageEvent(BannerMessageEventWrapper(e)));

    qDebug() << "Banner message created with id: " << QString::fromStdString(e->msgId);
    return QString::fromStdString(e->msgId);
}

void PalmSystem::removeBannerMessage(const QString& msgId)
{
    qDebug() << "removeBannerMessage(" << msgId << ")";

    if (!m_bridge)
        return;

    BannerMessageEvent* e = BannerMessageEventFactory::createRemoveMessageEvent(m_bridge->appId().toStdString(), msgId.toStdString());
    if (e)
        WebAppManager::instance()->sendAsyncMessage(new ViewHost_BannerMessageEvent(BannerMessageEventWrapper(e)));
}

void PalmSystem::clearBannerMessages()
{
    qDebug() << "clearBannerMessages()";

    if (!m_bridge)
        return;

    BannerMessageEvent* e = BannerMessageEventFactory::createClearMessagesEvent(m_bridge->appId().toStdString());
    if (e)
        WebAppManager::instance()->sendAsyncMessage(new ViewHost_BannerMessageEvent(BannerMessageEventWrapper(e)));
}

void PalmSystem::playSoundNotification(const QString& soundClass, const QString& soundFile, 
                                        int duration, bool wakeUpScreen)
{
    qDebug() << "playSoundNotification("
             << "soundClass: " << soundClass
             << ", soundFile: " << soundFile
             << ", duration: " << duration
             << ", wakeUpScreen: " << wakeUpScreen
             << ")";

    if (!m_bridge)
        return;

    BannerMessageEvent* e = BannerMessageEventFactory::createPlaySoundEvent(m_bridge->appId().toStdString(),
                                                                            soundClass.toStdString(), soundFile.toStdString(),
                                                                            duration, wakeUpScreen);
    if (e)
        WebAppManager::instance()->sendAsyncMessage(new ViewHost_BannerMessageEvent(BannerMessageEventWrapper(e)));
}

void PalmSystem::simulateMouseClick(int pageX, int pageY, bool pressed)
{

    if (WebAppManager::instance()->inSimulatedMouseEvent()) {
        qWarning() << "simulated mouse click called in middle of a simulated mouse event. Bailing...";
        return;
    }
/*
    WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return;

    if (pressed) {
        sptr<Event> e = new Event;
        e->x = pageX;
        e->y = pageY;
        e->type = Event::PenDown;
        e->button = Event::Left;
        e->modifiers = 0;
        e->setMainFinger(true);
        e->simulated = true;
        // See: https://jira.palm.com/browse/NOV-113203
        //e->clickCount = 1;
        int winKey = pageClient->getKey();
        WebAppManager::instance()->inputEvent(winKey,e);
    }
    else {
        sptr<Event> e = new Event;
        e->x = pageX;
        e->y = pageY;
        e->type = Event::PenUp;
        e->button = Event::Left;
        e->modifiers = 0;
        e->setMainFinger(true);
        e->simulated = true;
        int winKey = pageClient->getKey();
        WebAppManager::instance()->inputEvent(winKey,e);
    }
*/
    qDebug() << "NOTIMPLEMENTED: simulateMouseClick("
             << "x: " << pageX
             << ", y: " << pageY
             << ", pressed: " << pressed
             << ")";
}

void PalmSystem::paste()
{
    qDebug() << "paste()";

    // Request SysMgr to emmit a Paste command to the active window
    WebAppManager::instance()->sendAsyncMessage(new ViewHost_PasteToActiveWindow());
}

void PalmSystem::copiedToClipboard()
{
    qDebug() << "copiedToClipboard()";

    if (m_bridge)
        WebAppManager::instance()->copiedToClipboard(m_bridge->appId());
}

void PalmSystem::pastedFromClipboard()
{
    qDebug() << "pastedFromClipboard()";

    if (m_bridge)
        WebAppManager::instance()->pastedFromClipboard(m_bridge->appId());
}

void PalmSystem::setWindowOrientation(const QString& orientation)
{
    qDebug() << "setWindowOrientation("
             << "orientation: " << orientation
             << ")";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app || !app->isCardApp())
        return;

    m_specifiedWindowOrientation = orientation;

    CardWebApp* cardApp = static_cast<CardWebApp*>(app);

    if (orientation.compare("free", Qt::CaseInsensitive) == 0) {

        cardApp->setFixedOrientation(Event::Orientation_Invalid);
//        cardApp->setAllowOrientationChange(true);
        cardApp->setOrientation(WebAppManager::instance()->orientation());
    }
    else {

        Event::Orientation orient;

        if (orientation.compare("up", Qt::CaseInsensitive) == 0)
            orient = Event::Orientation_Up;
        else if (orientation.compare("down", Qt::CaseInsensitive) == 0)
            orient = Event::Orientation_Down;
        else if (orientation.compare("left", Qt::CaseInsensitive) == 0)
            orient = Event::Orientation_Left;
        else if (orientation.compare("right", Qt::CaseInsensitive) == 0)
            orient = Event::Orientation_Right;
        else if (orientation.compare("landscape", Qt::CaseInsensitive) == 0)
            orient = Event::Orientation_Landscape;
        else if (orientation.compare("portrait", Qt::CaseInsensitive) == 0)
            orient = Event::Orientation_Portrait;
        else {
            return;
        }

        cardApp->setFixedOrientation(orient);
    }
}

#if 0
QString PalmSystem::runTextIndexer(const QString& src, const QVariantMap& fields)
{
/*
    unsigned int flags = Palm::TextIndexer_Default;

    if (fields.contains("phoneNumber") && fields["phoneNumber"].type() == QVariant::Bool)

        if (fields["phoneNumber"].toBool())
            flags |= Palm::TextIndexer_PhoneNumber;
        else
            flags &= ~Palm::TextIndexer_PhoneNumber;
    }

    if (fields.contains("emailAddress") && fields["emailAddress"].type() == QVariant::Bool) {

        if (fields["emailAddress"].toBool())
            flags |= Palm::TextIndexer_EmailAddress;
        else
            flags &= ~Palm::TextIndexer_EmailAddress;
    }

    if (fields.contains("webLink") && fields["webLink"].type() == QVariant::Bool) {

        if (fields["webLink"].toBool())
            flags |= Palm::TextIndexer_WebLink;
        else
            flags &= ~Palm::TextIndexer_WebLink;
    }

    if (fields.contains("schemalessWebLink") && fields["schemalessWebLink"].type() == QVariant::Bool) {

        if (fields["schemalessWebLink"].toBool())
            flags |= Palm::TextIndexer_SchemalessWebLink;
        else
            flags &= ~Palm::TextIndexer_SchemalessWebLink;
    }

    if (fields.contains("emoticon") && fields["emoticon"].type() == QVariant::Bool) {

        if (fields["emoticon"].toBool())
            flags |= Palm::TextIndexer_Emoticon;
        else
            flags &= ~Palm::TextIndexer_Emoticon;
    }

    return QString(Palm::WebGlobal::runTextIndexerOnHtml( std::string(srcStr), flags ));
*/
    qDebug() << "NOTIMPLEMENTED: runTextIndexer("
             << "src: " << src
             << ", fields: " << fields
             << ")";
    return QString();
}
#endif

QString PalmSystem::encrypt(const QString& key, const QString& str)
{
    qDebug() << "encrypt("
             << "key: " << key
             << ", str: " << str
             << ")";

    QString result;

//    char* encryptedString = 0;
//    if (encryptString(str.toUtf8().constData(), key.toUtf8().constData(), &encryptedString))
//        result = encryptedString;
//    delete [] encryptedString;

    return result;
}

QString PalmSystem::decrypt(const QString& key, const QString& str)
{
    qDebug() << "decrypt("
             << "key: " << key
             << ", str: " << str
             << ")";

    QString result;

//    char* cleartextString = 0;
//    if (decryptString((const char*)str.toUtf8().constData(), key.toUtf8().constData(), &cleartextString))
//        result = cleartextString;
//    delete [] cleartextString;

    return result;
}

void PalmSystem::shutdown()
{
    qDebug() << "shutdown()";

    QString appId = m_bridge ? m_bridge->appId() : QString("");

    if ((Settings::LunaSettings()->uiType == Settings::UI_MINIMAL) ||
        (appId == QString("com.palm.systemui"))) {

        markFirstUseDone();
        exit(0);
    }
}

void PalmSystem::markFirstUseDone()
{
    qDebug() << "starting markFirstUseDone()";
    // For first-use mode, touch a marker file on the filesystem
    if (Settings::LunaSettings()->uiType == Settings::UI_MINIMAL) {
        g_mkdir_with_parents(Settings::LunaSettings()->lunaPrefsPath.c_str(), 0755);
        FILE* f = fopen((Settings::LunaSettings()->lunaPrefsPath + "/ran-first-use").c_str(), "w");
        fclose(f);
        /// See: NOV-117197
        qDebug() << "emitting first-use-finished";
        ::system("/sbin/initctl emit first-use-finished");
    }
    qDebug() << "leaving markFirstUseDone()";
}

void PalmSystem::enableFullScreenMode(bool enable)
{
    qDebug() << "enableFullScreenMode("
             << "enable: " << enable
             << ")";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app->isCardApp())
        return;

    CardWebApp* cardApp = static_cast<CardWebApp*>(app);
    cardApp->enableFullScreenMode(enable);
}

void PalmSystem::activate()
{
    qDebug() << "activate()";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app)
        return;

    app->focus();
}

void PalmSystem::deactivate()
{
    qDebug() << "deactivate()";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app)
        return;

    app->unfocus();
}

void PalmSystem::stagePreparing()
{
    qDebug() << "stagePreparing()";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app)
        return;

    app->stagePreparing();
}

void PalmSystem::stageReady()
{
    qDebug() << "stageReady()";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app)
        return;

    app->stageReady();
}

void PalmSystem::setAlertSound(const QString& soundClass, const QString& soundFile)
{
    qDebug() << "setAlertSound("
             << "soundClass: " << soundClass
             << ", soundFile: " << soundFile
             << ")";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app->isAlertApp()) {
        return;
    }

    AlertWebApp* alertApp = static_cast<AlertWebApp*>(app);
    if (!alertApp->getKey()) {
        return;
    }

    alertApp->setSoundParams(soundFile, soundClass);
}

void PalmSystem::receivePageUpDownInLandscape(bool enable)
{
    qDebug() << "receivePageUpDownInLandscape("
             << "enable: " << enable
             << ")";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app->isCardApp())
        return;

    CardWebApp* cardApp = static_cast<CardWebApp*>(app);
    cardApp->receivePageUpDownInLandscape(enable);
}

void PalmSystem::show()
{
    qDebug() << "show()";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app)
        return;

    app->thawFromCache();
}

void PalmSystem::hide()
{
    qDebug() << "hide()";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app)
        return;

    // Park this card -- we'll "close" it and the SysMgrWebBridge will automatically
    // get parked as long as this appID is marked as keep-alive.
    app->close();
}

void PalmSystem::enableDockMode(bool enable)
{
    qDebug() << "enableDockMode("
             << "enable: " << enable
             << ")";

    if (!m_bridge)
        return;

    // only allow this call from the systemui app
    if (m_bridge->appId() != "com.palm.systemui")
        return;

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_EnableDockMode(enable));
}

QString PalmSystem::getLocalizedString(const QString& str)
{
    QString textLocalized = str;

    if (!m_bridge)
        return textLocalized;

    // only allow this call from the memory app
    if (m_bridge->appId() != "com.palm.memory")
        return textLocalized;

    textLocalized = QString::fromStdString(LOCALIZED(str.toStdString()));

    return textLocalized;
}

QString PalmSystem::addNewContentIndicator()
{
    QString id;

    if (!m_bridge || !m_bridge->getClient())
        return id;

    WebAppBase* app = m_bridge->getClient();
    if (!(app->isAlertApp() || app->isDashboardApp()))
        return id;

    QString appId = m_bridge->appId();
    if (appId.isEmpty())
        return id;

    NewContentIndicatorEvent* e = NewContentIndicatorEventFactory::createAddEvent(appId.toStdString());
    if (!e)
        return id;

    id = QString::fromStdString(e->requestId);
    m_bridge->addNewContentRequestId(id);

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_NewContentEvent(NewContentIndicatorEventWrapper(e)));

    return id;
}

void PalmSystem::removeNewContentIndicator(const QString& requestId)
{
    qDebug() << "removeNewContentIndicator("
             << "requestId: " << requestId
             << ")";

    if (!m_bridge || !m_bridge->page())
        return;

    QString appId = m_bridge->appId();
    if (appId.isEmpty())
        return;

    NewContentIndicatorEvent* e = NewContentIndicatorEventFactory::createRemoveEvent(appId.toStdString(), requestId.toStdString());
    m_bridge->removeNewContentRequestId(requestId);

    if (!e)
        return;

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_NewContentEvent(NewContentIndicatorEventWrapper(e)));
}

void PalmSystem::runAnimationLoop(const QVariantMap& domObj, const QString& onStep,
                                const QString& onComplete, const QString& curve, qreal duration,
                                qreal start, qreal end)
{
    qWarning() << "UNUSED: runAnimationLoop("
             << "domObj: " << domObj
             << ", onStep: " << onStep
             << ", onComplete: " << onComplete
             << ", curve: " << curve
             << ", duration: " << duration
             << ", start: " << start
             << ", end: " << end
             << ")";
}

void PalmSystem::setActiveBannerWindowWidth()
{
    qDebug() << "setActiveBannerWindowWidth()";
}

void PalmSystem::cancelVibrations()
{
    qDebug() << "cancelVibrations()";

    WebAppManager::instance()->sendAsyncMessage(new ViewHost_CancelVibrations());
}

void PalmSystem::setWindowProperties(const QVariantMap& properties)
{
    qDebug() << "setWindowProperties(" 
             << "props: " << properties
             << ")";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app)
        return;

    WindowProperties props;

    // check for screen timeout
    if (properties.contains("blockScreenTimeout") && properties["blockScreenTimeout"].type() == QVariant::Bool) {
        props.setBlockScreenTimeout (properties["blockScreenTimeout"].toBool());
    }

    // check for subtle light bar: OLD API
    if (properties.contains("setSubtleLightbar") && properties["setSubtleLightbar"].type() == QVariant::Bool) {
        props.setSubtleLightbar (properties["setSubtleLightbar"].toBool());
    }

    // check for subtle light bar: NEW API
    if (properties.contains("subtleLightbar") && properties["subtleLightbar"].type() == QVariant::Bool) {
        props.setSubtleLightbar (properties["subtleLightbar"].toBool());
    }

    // check for fast accelerometer
    if (properties.contains("fastAccelerometer") && properties["fastAccelerometer"].type() == QVariant::Bool) {
/*
        WebPage* page = framePage(m_frame);
        if (page)
        {
            page->fastAccelerometerOn(properties["fastAccelerometer"].toBool());
        }
*/
    }

    if (properties.contains("activeTouchpanel") && properties["activeTouchpanel"].type() == QVariant::Bool) {
        props.setActiveTouchpanel (properties["activeTouchpanel"].toBool());
    }

    if (properties.contains("alsDisabled") && properties["alsDisabled"].type() == QVariant::Bool) {
        props.setAlsDisabled (properties["alsDisabled"].toBool());
    }

    if (properties.contains("enableCompass") && properties["enableCompass"].type() == QVariant::Bool) {
        props.setCompassEvents (properties["enableCompass"].toBool());
    }

    if (properties.contains("enableGyro") && properties["enableGyro"].type() == QVariant::Bool) {
        props.setAllowGyroEvents (properties["enableGyro"].toBool());
    }

    // check for Suppress Banner Messages
    if (properties.contains("suppressBannerMessages") && properties["suppressBannerMessages"].type() == QVariant::Bool) {
        props.setSuppressBannerMessages (properties["suppressBannerMessages"].toBool());
    }

    if (properties.contains("suppressGestures") && properties["suppressGestures"].type() == QVariant::Bool) {
        props.setSuppressGestures (properties["suppressGestures"].toBool());
    }

    if (properties.contains("webosDragMode") && properties["webosDragMode"].type() == QVariant::String) {
        props.setDashboardManualDragMode (properties["webosDragMode"].toString() == "manual");
    }

    if (properties.contains("statusBarColor") && properties["statusBarColor"].type() == QVariant::Int) {
        props.setStatusBarColor (properties["statusBarColor"].toInt());
    }

    if (properties.contains("rotationLockMaxmized") && properties["rotationLockMaximized"].type() == QVariant::Bool) {
        props.setRotationLockMaximized (properties["rotationLockMaximized"].toBool());
    }

    WebAppManager::instance()->setAppWindowProperties(app->getKey(), props);
}

bool PalmSystem::addActiveCallBanner(const QString& icon, const QString& message, uint32_t timeStart)
{
    qDebug() << "addActiveCallBanner("
             << "icon: " << icon
             << ", message: " << message
             << ", timeStart: " << timeStart;

    if (!m_bridge)
        return false;

#ifndef TARGET_DESKTOP
    if (m_bridge->appId() != "com.palm.app.phone")
        return false;
#endif

    ActiveCallBannerEvent* e = ActiveCallBannerEventFactory::createAddEvent(message.toStdString(), icon.toStdString(), timeStart);
    if (e)
        WebAppManager::instance()->sendAsyncMessage(new ViewHost_ActiveCallBannerEvent(ActiveCallBannerEventWrapper(e)));

    return e != 0;
}

void PalmSystem::removeActiveCallBanner()
{
    qDebug() << "removeActiveCallBanner()";

    if (!m_bridge)
        return;

#ifndef TARGET_DESKTOP
    if (m_bridge->appId() != "com.palm.app.phone")
        return;
#endif

    ActiveCallBannerEvent* e = ActiveCallBannerEventFactory::createRemoveEvent();
    if (e)
        WebAppManager::instance()->sendAsyncMessage(new ViewHost_ActiveCallBannerEvent(ActiveCallBannerEventWrapper(e)));
}

bool PalmSystem::updateActiveCallBanner(const QString& icon, const QString& message, uint32_t timeStart)
{
    qDebug() << "updateActiveCallBanner("
             << "icon: " << icon
             << ", message: " << message
             << ", timeStart: " << timeStart
             << ")";

    if (!m_bridge)
        return false;

#ifndef TARGET_DESKTOP
    if (m_bridge->appId() != "com.palm.app.phone")
        return false;
#endif

    ActiveCallBannerEvent* e = ActiveCallBannerEventFactory::createUpdateEvent(message.toStdString(), icon.toStdString(), timeStart);
    if (e)
        WebAppManager::instance()->sendAsyncMessage(new ViewHost_ActiveCallBannerEvent(ActiveCallBannerEventWrapper(e)));

    return e != 0;
}

void PalmSystem::applyLaunchFeedback(int offsetX, int offsetY)
{
    qDebug() << "applyLaunchFeedback("
             << "offsetX: " << offsetX
             << ", offsetY: " << offsetY
             << ")";

    if (!m_bridge || !m_bridge->getClient())
        return;

    if (m_bridge->appId() != "com.palm.launcher")
        return;

    WebAppBase* app = m_bridge->getClient();
    if (!app->isWindowed())
        return;

    static_cast<WindowedWebApp*>(app)->applyLaunchFeedback(offsetX, offsetY);
}

void PalmSystem::launcherReady()
{
    qDebug() << "launcherReady()";

    if (!m_bridge || m_bridge->appId() != "com.palm.launcher")
        return;

    WebAppManager::instance()->markUniversalSearchReady();
}

QString PalmSystem::getDeviceKeys(int key)
{
    qDebug() << "getDeviceKeys("
             << "key: " << key
             << ")";

    KeyMapType details = getDetailsForKey(key);
    pbnjson::JValue payload = pbnjson::Object();
    const int tempLen = 4;
    char temp[tempLen + 1];

    if (details.normal != Event::Key_Null)
        sprintf(temp, "%c", details.normal);
    else
        strcpy(temp, "");

    payload.put("normal", std::string(temp));

    if (details.shift != Event::Key_Null)
        sprintf(temp, "%c", details.shift);
    else
        strcpy(temp, "");

    payload.put("shift", std::string(temp));

    if (details.opt != Event::Key_Null)
        sprintf(temp, "%c", details.opt);
    else
        strcpy(temp, "");

    payload.put("opt", std::string(temp));

    pbnjson::JGenerator generator;
    std::string result;
    generator.toString(payload, pbnjson::JSchemaFragment("{}"), result);
    return QString::fromStdString(result);
}

void PalmSystem::repaint()
{
    qDebug() << "repaint()";

    if (!m_bridge || !m_bridge->getClient())
        return;

    if (m_bridge->appId() != "com.palm.launcher")
        return;

    WebAppBase* app = m_bridge->getClient();
    if (!app->isWindowed())
        return;

    WindowedWebApp* winApp = static_cast<WindowedWebApp*>(app);
    winApp->invalidate();
    winApp->paint();
}

void PalmSystem::hideSpellingWidget()
{
    qDebug() << "UNUSED: hideSpellingWidget()";
}

void PalmSystem::printFrame(const QString& frameName, int lpsJobId, int widthPx, int heightPx, 
                            int printDpi, bool landscape, bool reverseOrder)
{
/*
    WebPage* page = framePage(m_frame);
    if (!page || !page->webkitView())
        return;

    page->webkitView()->print(frameName, lpsJobId, widthPx, heightPx, printDpi, landscape, reverseOrder);
*/
    qDebug() << "NOTIMPLEMENTED: printFrame("
             << "frameName: " << frameName
             << ", lpsJobId: " << lpsJobId
             << ", widthPx: " << widthPx
             << ", heightPx: " << heightPx
             << ", printDpi: " << printDpi
             << ", landscape: " << landscape
             << ", reverseOrder: " << reverseOrder
             << ")";
}

void PalmSystem::editorFocused(bool focused, int fieldType, int fieldActions)
{
    qDebug() << "editorFocused("
             << "focused: " << focused
             << ", fieldType: " << fieldType
             << ", fieldActions: " << fieldActions
             << ")";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app || !app->isCardApp())
        return;

    PalmIME::EditorState editorState(static_cast<PalmIME::FieldType>(fieldType), static_cast<PalmIME::FieldAction>(fieldActions));

    app->setExplicitEditorFocus(focused, editorState);
}

void PalmSystem::allowResizeOnPositiveSpaceChange(bool allowResize)
{
    qDebug() << "allowResizeOnPositiveSpaceChange("
             << "allowResize: " << allowResize
             << ")";
    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app || !app->isCardApp())
        return;

    static_cast<CardWebApp*>(app)->allowResizeOnPositiveSpaceChange(allowResize);
}

void PalmSystem::keepAlive(bool keep)
{
    qDebug() << "keepAlive("
             << "keep: " << keep
             << ")";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app || !app->isCardApp())
        return;

    const std::set<std::string>& appsToKeepAlive = Settings::LunaSettings()->appsToKeepAlive;
    if (appsToKeepAlive.find(m_bridge->appId().toStdString()) == appsToKeepAlive.end())
        return;

    app->setKeepAlive(keep);
}

void PalmSystem::useSimulatedMouseClicks(bool uses)
{
/*
    WebPageClient* pageClient = framePageClient(m_frame);
    if (!pageClient)
        return;

    WebAppBase* app = static_cast<WebAppBase*>(pageClient);
    if (!app->isCardApp())
        return;

    WebPage* page = framePage(m_frame);
    if (!page || !page->webkitView())
        return;

    page->webkitView()->usesSimulatedMouseClicks(uses);
*/
    qDebug() << "NOTIMPLEMENTED: useSimulatedMouseClicks("
             << "uses: " << uses
             << ")";
}

void PalmSystem::handleTapAndHoldEvent(int pageX, int pageY)
{
/*
    WebPage* page = framePage(m_frame);
    if (!page || !page->webkitView())
        return;

    // Disconnect the API since clipboard should work in any app not just enyo apps.
    // page->webkitView()->mouseHoldEvent(pageX, pageY);
*/
    qDebug() << "NOTIMPLEMENTED: handleTapAndHoldEvent("
             << "pageX: " << pageX
             << ", pageY: " << pageY
             << ")";
}

void PalmSystem::setManualKeyboardEnabled(bool enabled)
{
    qDebug() << "setManualeKeyboardEnabled("
             << "enabled: " << enabled
             << ")";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app || !app->isCardApp())
        return;

    app->setManualEditorFocusEnabled(enabled);
}

void PalmSystem::keyboardShow(int fieldType)
{
    qDebug() << "keyboardShow("
             << "fieldType: " << fieldType
             << ")";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app || !app->isCardApp())
        return;

    PalmIME::EditorState editorState(static_cast<PalmIME::FieldType>(fieldType));
    app->setManualEditorFocus(true, editorState);
}

void PalmSystem::keyboardHide()
{
    qDebug() << "keyboardHide()";

    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app || !app->isCardApp())
        return;

    PalmIME::EditorState editorState;
    app->setManualEditorFocus(false, editorState);
}

QVariant PalmSystem::getResource(QVariant a, QVariant b)
{
    QFile f(a.toString());
    if(!f.open(QIODevice::ReadOnly))
        return QVariant();

    QByteArray data = f.readAll();

    return QVariant(data.constData());
}

// read properties
QString PalmSystem::launchParams() const
{
    return m_launchParams;
}

bool PalmSystem::hasAlphaHole() const
{
    return m_hasAlphaHole;
}

QString PalmSystem::locale() const
{
    return QString::fromStdString(Preferences::instance()->locale());
}

QString PalmSystem::localeRegion() const
{
    return QString::fromStdString(Preferences::instance()->localeRegion());
}

QString PalmSystem::timeFormat() const
{
    return QString::fromStdString(Preferences::instance()->timeFormat());
}

QString PalmSystem::timeZone() const
{
    return QString::fromStdString(WebAppManager::instance()->getTimeZone());
}

bool PalmSystem::isMinimal() const
{
    return Settings::LunaSettings()->uiType == Settings::UI_MINIMAL;
}

QString PalmSystem::identifier() const
{
    if (!m_bridge)
        return QString("");

    return m_bridge->getIdentifier();
}

QString PalmSystem::version() const
{
    return qWebKitVersion();
}

QString PalmSystem::screenOrientation() const
{
    // See: DFISH-28744: Enyo uses screenOrientation for windowOrientation so we need
    // to pretend to do the same
    return nameForOrientation(WebAppManager::instance()->orientation());
}

QString PalmSystem::windowOrientation() const
{
    return m_windowOrientation;
}

QString PalmSystem::specifiedWindowOrientation() const
{
    return m_specifiedWindowOrientation;
}

QString PalmSystem::videoOrientation() const
{
    Event::Orientation orient = Event::Orientation_Up;

    switch (Settings::LunaSettings()->homeButtonOrientationAngle) {
    case 90:
        orient = Event::Orientation_Right;
        break;
    case 180:
        orient = Event::Orientation_Down;
        break;
    case 270:// For Topaz
    case -90:
        orient = Event::Orientation_Left;
        break;
    case 0:
    default:
        break;
    }

    return nameForOrientation(orient);
}

QString PalmSystem::deviceInfo() const
{
    QString deviceInfo = QString::fromStdString(DeviceInfo::instance()->jsonString());
    return deviceInfo.isEmpty() ? "{}" : deviceInfo;
}

bool PalmSystem::isActivated() const
{
    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app || !app->isWindowed())
        return false;

    WindowedWebApp* windowedApp = static_cast<WindowedWebApp*>(app);
    return windowedApp->isFocused();
}

int PalmSystem::activityId() const
{
    if (!m_bridge)
        return -1;

    return m_bridge->activityId();
}

QString PalmSystem::phoneRegion() const
{
    return QString::fromStdString(Preferences::instance()->phoneRegion());
}

// write properties
void PalmSystem::setHasAlphaHole(bool newAlpha)
{
    m_hasAlphaHole = newAlpha;
}

void PalmSystem::setPropWindowOrientation(QString windowOrientation)
{
    WebAppBase* app = (m_bridge ? m_bridge->getClient() : 0);
    if (!app || !app->isCardApp()) {
        return;
    }

    m_specifiedWindowOrientation = windowOrientation;

    CardWebApp* cardApp = static_cast<CardWebApp*>(app);

    if (windowOrientation.compare("free", Qt::CaseInsensitive) == 0) {

        cardApp->setFixedOrientation(Event::Orientation_Invalid);
//        cardApp->setAllowOrientationChange(true);
        cardApp->setOrientation(WebAppManager::instance()->orientation());

//        cardApp->setAllowOrientationChange(true);
    }
    else {

        Event::Orientation orientation;

        if (windowOrientation.compare("ee", Qt::CaseInsensitive) == 0)
            orientation = Event::Orientation_Up;
        else if (windowOrientation.compare("down", Qt::CaseInsensitive) == 0)
            orientation = Event::Orientation_Down;
        else if (windowOrientation.compare("left", Qt::CaseInsensitive) == 0)
            orientation = Event::Orientation_Left;
        else if (windowOrientation.compare("right", Qt::CaseInsensitive) == 0)
            orientation = Event::Orientation_Right;
        else if (windowOrientation.compare("landscape", Qt::CaseInsensitive) == 0)
            orientation = Event::Orientation_Landscape;
        else if (windowOrientation.compare("portrait", Qt::CaseInsensitive) == 0)
            orientation = Event::Orientation_Portrait;
        else {
            return;
        }

        cardApp->setFixedOrientation(orientation);
    }
}

