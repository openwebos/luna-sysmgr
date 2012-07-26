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




#include "Common.h"

#include "WebAppBase.h"

#include "ApplicationDescription.h"
#include "WebAppManager.h"

#include <QDebug>

#include <cjson/json.h>


WebAppBase::WebAppBase() : m_page(0),
                           m_inCache(false),
                           m_keepAlive(false),
                           m_appDesc(0),
                           m_activityManagerToken(LSMESSAGE_TOKEN_INVALID)
{
}

WebAppBase::~WebAppBase()
{
    if (m_inCache)
        WebAppCache::remove(this);

    WebAppManager* wam = WebAppManager::instance();
    wam->appDeleted(this);

    cleanResources();

    wam->removeHeadlessAppFromWatchList(this);
}

void WebAppBase::cleanResources()
{
    if (m_page) {
        WebAppManager::instance()->reportAppClosed(m_page->appId(), m_page->processId());
    }

    // does nothing if m_page has already been deleted and set to 0 by ~WindowedWebApp
    destroyActivity();

    // NOTE: the WebPage's destructor accesses appDescImage so the order
    // of operations here matters
    if (m_page) {
        delete m_page;
        m_page = 0;
    }

    if (m_appDesc) {
        delete m_appDesc;
        m_appDesc = 0;
    }
}

void WebAppBase::attach(SysMgrWebBridge* bridge)
{
    // connect to the signals of the WebBridge
    // parse up the ApplicationDescription

    if (m_page)
        detach();

    m_page = bridge;
    m_page->setClient(this);

    connect(m_page, SIGNAL(signalUrlChanged(const QUrl&)), this, SLOT(uriChanged(const QUrl&)));

    m_appId = m_page->appId();
    m_processId = m_page->processId();

    createActivity();
}

SysMgrWebBridge* WebAppBase::detach(void)
{
    WebAppManager::instance()->reportAppClosed(m_page->appId(), m_page->processId());

    destroyActivity();

    SysMgrWebBridge* p = m_page;

    // TODO: disconnect the pages signals
    //
    disconnect(m_page, 0, this, 0);

    m_page->setClient(0);
    m_page = 0;
    return p;
}

void WebAppBase::relaunch(const char* args, const char* launchingAppId, const char* launchingProcId)
{
    bool ret = false;
    if (m_page)
        ret = m_page->relaunch(args, launchingAppId, launchingProcId);
    if (!ret && isWindowed())
        focus();
}

void WebAppBase::stagePreparing()
{
    // just has some perf testing
}

void WebAppBase::stageReady()
{
    // just has perf testing
}

void WebAppBase::setAppDescription(ApplicationDescription* appDesc)
{
    if (m_appDesc) {
        delete m_appDesc;
        m_appDesc = 0;
    }
    m_appDesc = appDesc;
}

void WebAppBase::setManualEditorFocusEnabled(bool enable)
{
    if (m_page) {
        m_page->manualFocusEnabled(enable);
    }
}

void WebAppBase::setManualEditorFocus(bool focused, const PalmIME::EditorState& editorState)
{
    if (m_page) {
        m_page->manualEditorFocused(focused, editorState);
    }
}

void WebAppBase::setExplicitEditorFocus(bool focused, const PalmIME::EditorState& editorState)
{
    if (m_page) {
        m_page->explicitEditorFocused(focused, editorState);
        // turn off auto-cap behavior when input focus has been explicitly requested.
        //m_page->autoCapEnabled(false);
    }
}

void WebAppBase::close()
{
    WebAppManager::instance()->closeAppInternal(this);
}

void WebAppBase::screenSize(int& width, int& height)
{
    // TODO: get sizes from WAM
    width = 0;
    height = 0;
}

void WebAppBase::resizedContents(int contentsWidth, int contentsHeight)
{
}

void WebAppBase::zoomedContents(double scaleFactor, int contentsWidth, int contentsHeight, int newScrollOffsetX, int newScrollOffsetY)
{
}

void WebAppBase::scrolledContents(int newContentsX, int newContentsY)
{
}

void WebAppBase::uriChanged(const char* uri)
{
    if (uri)
        m_url = QString::fromStdString(uri);
    else
        m_url = QString();
}
void WebAppBase::uriChanged(const QUrl& uri)
{
    uriChanged(uri.toString().toAscii().constData());
}

void WebAppBase::titleChanged(const char* title)
{
}

void WebAppBase::statusMessage(const char* msg)
{
}

void WebAppBase::dispatchFailedLoad(const char* domain, int errorCode, const char* failingURL, const char* localizedDescription)
{
}

void WebAppBase::createActivity()
{
    if (!m_page)
        return;

    if (m_page->parent())
        return;

    LSError lsError;
    LSErrorInit(&lsError);

    json_object* payload = json_object_new_object();
    json_object* activityObj = json_object_new_object();
    json_object_object_add(activityObj, (char*) "name",
                           json_object_new_string(m_appId.toAscii().constData()));
    json_object_object_add(activityObj, (char*) "description",
                           json_object_new_string(m_processId.toAscii().constData()));
    json_object* activityTypeObj = json_object_new_object();
    json_object_object_add(activityTypeObj, (char*) "foreground",
                           json_object_new_boolean(true));
    json_object_object_add(activityObj, (char*) "type", activityTypeObj);

    json_object_object_add(payload, "activity", activityObj);
    json_object_object_add(payload, "subscribe", json_object_new_boolean(true));
    json_object_object_add(payload, "start", json_object_new_boolean(true));
    json_object_object_add(payload, "replace", json_object_new_boolean(true));

    if (!LSCallFromApplication(WebAppManager::instance()->m_servicePrivate,
                "palm://com.palm.activitymanager/create",
                json_object_to_json_string(payload),
                m_page->getIdentifier(),
                WebAppManager::activityManagerCallback,
                this, &m_activityManagerToken, &lsError)) {
        qCritical() << __PRETTY_FUNCTION__ << ":" << __LINE__ << "Failed in calling activity manager create:" << lsError.message;
        LSErrorFree(&lsError);
    }

    json_object_put(payload);
}

void WebAppBase::destroyActivity()
{
    if (!m_page)
        return;

    if (m_page->parent())
        return;

    if (m_activityManagerToken == LSMESSAGE_TOKEN_INVALID)
        return;

    LSError lsError;
    LSErrorInit(&lsError);

    if (!LSCallCancel(WebAppManager::instance()->m_servicePrivate,
                      m_activityManagerToken, &lsError)) {
        qCritical() << __PRETTY_FUNCTION__ << ":" << __LINE__ << "Failed in canceling activity:" << lsError.message;
        LSErrorFree(&lsError);
    }

    m_activityManagerToken = LSMESSAGE_TOKEN_INVALID;
}

void WebAppBase::focusActivity()
{
    if (!m_page || m_page->activityId() < 0)
        return;

    LSError lsError;
    LSErrorInit(&lsError);

    json_object* payload = json_object_new_object();
    json_object_object_add(payload, "activityId", json_object_new_int(m_page->activityId()));

    if (!LSCallFromApplication(WebAppManager::instance()->m_servicePrivate,
                "palm://com.palm.activitymanager/focus",
                json_object_to_json_string(payload),
                m_page->getIdentifier(),
                0, 0, 0, &lsError)) {
        qCritical() << __PRETTY_FUNCTION__ << ":" << __LINE__ << "Failed in calling activity manager focus:" << lsError.message;
        LSErrorFree(&lsError);
    }

    json_object_put(payload);
}

void WebAppBase::blurActivity()
{
    if (!m_page || m_page->activityId() < 0)
        return;

    LSError lsError;
    LSErrorInit(&lsError);

    json_object* payload = json_object_new_object();
    json_object_object_add(payload, "activityId", json_object_new_int(m_page->activityId()));

    if (!LSCallFromApplication(WebAppManager::instance()->m_servicePrivate,
                "palm://com.palm.activitymanager/unfocus",
                json_object_to_json_string(payload),
                m_page->getIdentifier(),
                0, 0, 0, &lsError)) {
        qCritical() << __PRETTY_FUNCTION__ << ":" << __LINE__ << "Failed in calling activity manager focus:" << lsError.message;
        LSErrorFree(&lsError);
    }

    json_object_put(payload);
}

void WebAppBase::resizeWebPage(uint32_t width, uint32_t height)
{
    if (page() && page()->page())
        page()->page()->setViewportSize(QSize(width, height));
}

