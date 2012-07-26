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

#include "AlertWebApp.h"

#include "Settings.h"
#include "SysMgrWebBridge.h"
#include "WebAppManager.h"
#include <PIpcBuffer.h>
#include <PIpcChannel.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

static int kMinHeight = 0;
static const int kAlertActivityDuration = 10000;

AlertWebApp::AlertWebApp(const QString& appId, int width, int height, Window::Type type, PIpcChannel *channel)
	: WindowedWebApp(0, 0, type,channel), m_isPowerdActivityRunning (false)
{
	setAppId(appId);

	kMinHeight = Settings::LunaSettings()->positiveSpaceBottomPadding;

	// Set width to a valid value
	if(width <= 0)
		m_width = WebAppManager::instance()->currentUiWidth();
	else
		m_width = width;

	m_height = constraintHeight(height);
	
	m_windowWidth =  m_width;
	m_windowHeight = m_height;
	init();
}

AlertWebApp::~AlertWebApp()
{
    // we need to re-parent the children since there might be pages that are tied to this apps web bridge
    // for example a new popup alert or a dashboad app is launched within the original alert web app
    // and without detaching them they will be killed when the parent is
    QList<QObject*>::const_iterator i;
    for (i = page()->children().constBegin(); i != page()->children().constEnd(); ++i)
        (*i)->setParent(0);

	stopPowerdActivity();
}

void AlertWebApp::attach(SysMgrWebBridge* page)
{
	const StringVariantMap& stageArgs = page->stageArguments();

	StringVariantMap::const_iterator itSound = stageArgs.find("sound");
	StringVariantMap::const_iterator itSoundClass = stageArgs.find("soundclass");

	WindowedWebApp::attach(page);

	m_channel->sendAsyncMessage(new ViewHost_Alert_SetSoundParams(routingId(),
																  (itSound != stageArgs.end()) ?
																  (*itSound).toString().toStdString() : std::string(), 
																  (itSoundClass != stageArgs.end()) ?
																  (*itSoundClass).toString().toStdString() : std::string()));

	startPowerdActivity();
}

void AlertWebApp::stageReady() {

	WindowedWebApp::stageReady();

	stopPowerdActivity();
}

void AlertWebApp::startPowerdActivity()
{
	if (!m_isPowerdActivityRunning) {
		LSHandle* lshandle = WebAppManager::instance()->getStatsServiceHandle();
		LSError lsError;
		LSErrorInit(&lsError);

		gchar* activityId = g_strdup_printf("com.palm.lunastats-alert-%p", this);
		g_message ("%s: starting powerd activity for %s", __PRETTY_FUNCTION__, activityId);
		gchar* params = g_strdup_printf("{\"id\": \"%s\", \"duration_ms\": %d}", activityId, kAlertActivityDuration);

		bool ret = LSCall(lshandle, "palm://com.palm.power/com/palm/power/activityStart",
				params, NULL, NULL, NULL, &lsError);

		g_free (activityId);
		g_free (params);

		if (!ret) {
			g_warning("%s: Failed to call powerd activityStart: %s", __PRETTY_FUNCTION__, lsError.message);
			LSErrorFree(&lsError);
			return;
		}
		m_isPowerdActivityRunning = true;
	}
}

void AlertWebApp::stopPowerdActivity() {

	if (m_isPowerdActivityRunning) {
		LSHandle* lshandle = WebAppManager::instance()->getStatsServiceHandle();
		LSError lsError;
		LSErrorInit(&lsError);

		gchar* activityId = g_strdup_printf("com.palm.lunastats-alert-%p", this);
		g_message ("%s: stopping powerd activity for %s", __PRETTY_FUNCTION__, activityId);
		gchar* params = g_strdup_printf("{\"id\": \"%s\"}", activityId);

		bool ret = LSCall(lshandle, "palm://com.palm.power/com/palm/power/activityEnd",
				params, NULL, NULL, NULL, &lsError);

		g_free (activityId);
		g_free (params);

		if (!ret) {
			g_warning("%s: Failed to call powerd activityEnd: %s", __PRETTY_FUNCTION__, lsError.message);
			LSErrorFree(&lsError);
			return;
		}
		m_isPowerdActivityRunning = false;
	}
}


void AlertWebApp::loadFinished()
{
	updateContentRect();
	WindowedWebApp::loadFinished();
}

void AlertWebApp::paint()
{
	updateContentRect();
    WindowedWebApp::paint();
}

void AlertWebApp::updateContentRect()
{
    QWebFrame* frame = page()->page()->mainFrame();
    QWebElement el = frame->findFirstElement("[x-palm-popup-content]");
    QRect r;
    if (!el.isNull()) {
        r = el.geometry();
    	r.setLeft(MAX(0, r.left()));
	    r.setRight(MIN(r.right(), (int) m_windowWidth));
    	r.setTop(MAX(0, r.top()));
	    r.setBottom(MIN(r.bottom(), (int) m_windowHeight));
    }

	m_channel->sendAsyncMessage(new ViewHost_Alert_SetContentRect(routingId(),
																  r.left(), r.right(), 
																  r.top(), r.bottom()));
}

void AlertWebApp::focus()
{
	// Persistent windows use stage Activate (which internally
	// maps to this call) to show their window. so we need to
	// make sure window is painted before showing it
	invalidate();
    paint();

	WindowedWebApp::focus();
}

int AlertWebApp::constraintHeight(int h)
{
	Settings* settings = Settings::LunaSettings();
    int maxNegativeSpaceHeight = (int) (settings->maximumNegativeSpaceHeightRatio * WebAppManager::instance()->currentUiHeight());

	if (appId() == "com.palm.systemui") {
        maxNegativeSpaceHeight = WebAppManager::instance()->currentUiHeight() - settings->positiveSpaceTopPadding;
	}

	if (h <= 0)
		h = maxNegativeSpaceHeight;
	else {
		h = MIN(maxNegativeSpaceHeight, h);
		h = MAX(kMinHeight, h);
	}

	return h;
}

void AlertWebApp::setSoundParams(const QString& fileName, const QString& soundClass)
{
	if(!getKey() || !m_channel)
		return;
	
	m_channel->sendAsyncMessage(new ViewHost_Alert_SetSoundParams(routingId(), fileName.toStdString(), soundClass.toStdString()));
}


// Handler for orientation changed event
void AlertWebApp::setOrientation(Event::Orientation orient)
{
	switch (orient) {
	case (Event::Orientation_Up):    
	case (Event::Orientation_Down):  
	case (Event::Orientation_Left):  
	case (Event::Orientation_Right):
		break;
	default:
		return;
	}

	if(Settings::LunaSettings()->displayUiRotates)
			return;

	// Execute the orientation change
    //callMojoScreenOrientationChange();
}

void AlertWebApp::callMojoScreenOrientationChange()
{
/*
	Event::Orientation orient = WebAppManager::instance()->orientation();
	
	// FIXME: We should call the callback directly without using a script
	static const char* scriptUp    = "Mojo.screenOrientationChanged(\"up\")";
	static const char* scriptDown  = "Mojo.screenOrientationChanged(\"down\")";
	static const char* scriptLeft  = "Mojo.screenOrientationChanged(\"left\")";
	static const char* scriptRight = "Mojo.screenOrientationChanged(\"right\")";

	const char* script = 0;
	
	switch (orient) {
	case (Event::Orientation_Up):    script = scriptUp; break;
	case (Event::Orientation_Down):  script = scriptDown; break;
	case (Event::Orientation_Left):  script = scriptLeft; break;
	case (Event::Orientation_Right): script = scriptRight; break;
	default: return;
	}

	if (script && m_page && m_page->webkitPage()) {

		// Recursion protector
		static bool s_alreadyHere = false;
		if (!s_alreadyHere) {
			s_alreadyHere = true;
			m_page->webkitPage()->evaluateScript(script);
			s_alreadyHere = false;
		}
	}
*/
}

