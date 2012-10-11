/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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




#include <sys/select.h>
#include "CoreNaviManager.h"
#include "DeviceInfo.h"
#include "Preferences.h"
#include "Settings.h"

#if defined(MACHINE_BROADWAY)
#define MINIMUM_BRIGHTNESS 5
#define MAXIMUM_BRIGHTNESS 100
#else
#define MINIMUM_BRIGHTNESS 0
#define MAXIMUM_BRIGHTNESS 100
#endif

CoreNaviManager* CoreNaviManager::m_instance = NULL;

/*
 * This file makes sure that all move/gesture events originating from the CoreNavi area displays the correct visual (LED) feedback.
 */

CoreNaviManager* CoreNaviManager::instance ()
{
    if (CoreNaviManager::m_instance == NULL)
        CoreNaviManager::m_instance = new CoreNaviManager;

    return CoreNaviManager::m_instance;
}

CoreNaviManager::CoreNaviManager() 
    : m_centerGlow(false),
      m_allGlow (false),
      m_brightness (MINIMUM_BRIGHTNESS),
      m_isSubtleLightbar (false),
      m_subtleModeBrightness (MINIMUM_BRIGHTNESS),
      m_metaKeyDown (false),
      m_lightbarRestoreTimer (HostBase::instance()->masterTimer(), this, &CoreNaviManager::restoreLightbar),
      m_lightbarHoldTimer (HostBase::instance()->masterTimer(), this, &CoreNaviManager::seesawLightbar),
      m_lightbarSwipeLeftTimer (HostBase::instance()->masterTimer(), this, &CoreNaviManager::swipeLeftLightbar),
      m_lightbarSwipeRightTimer (HostBase::instance()->masterTimer(), this, &CoreNaviManager::swipeRightLightbar)
{ 
    m_displayManager = DisplayManager::instance();
    m_leds = CoreNaviLeds::instance();
    m_ambientLightSensor = AmbientLightSensor::instance();
    m_systemUiController = SystemUiController::instance();

    if (!DeviceInfo::instance()->coreNaviButton() && Settings::LunaSettings()->lightbarEnabled)
		m_hasLightBar = true;
    else
        m_hasLightBar  = false;

    m_animationSpeed = Settings::LunaSettings()->gestureAnimationSpeed / 5;

    m_throbberBrightness = Settings::LunaSettings()->ledPulseMaxBrightness;
    m_throbberDimBrightness = Settings::LunaSettings()->ledPulseDarkBrightness;

    connect(m_displayManager, SIGNAL(signalDisplayStateChange(int)), this, SLOT(slotDisplayStateChange(int)));
    connect(m_displayManager, SIGNAL(signalLockStateChange(int, int)), this, SLOT(slotLockStateChange(int, int)));

    connect(m_systemUiController, SIGNAL(signalCardWindowMaximized()), this, SLOT(slotCardWindowMaximized()));
    connect(m_systemUiController, SIGNAL(signalCardWindowMinimized()), this, SLOT(slotCardWindowMinimized()));
    connect(m_systemUiController, SIGNAL(signalFocusMaximizedCardWindow(bool)), this, SLOT(slotFocusMaximizedCardWindow(bool)));

    if (m_leds)
        m_leds->stopAll (m_leds->All());
}

CoreNaviManager::~CoreNaviManager() { 
    clearAllStandbyRequests();
}

void CoreNaviManager::setMetaGlow(bool enable)
{
    if(!m_leds)
        return;

    if(enable) {
        int brightness = m_displayManager->getCoreNaviBrightness();
        m_leds->ledRampTo(m_leds->Center(),brightness,200);
        m_centerGlow = true;
    } else if(m_centerGlow) {
        m_leds->ledRampTo(m_leds->Center(),0,600);
        m_centerGlow = false;
    }
}


void CoreNaviManager::updateBrightness (int brightness) {
    if (!m_leds)
        return;

    if (brightness == 0 && useLightbar())  {
        g_debug ("%s: updating brightness to 0, turning lightbar off", __PRETTY_FUNCTION__);
        lightbarOff();
        return;
    }

    if (brightness > 0)
        m_brightness = brightness;

    if (m_brightness < MINIMUM_BRIGHTNESS)
        m_brightness = MINIMUM_BRIGHTNESS;

    if (m_brightness > MAXIMUM_BRIGHTNESS)
        m_brightness = MAXIMUM_BRIGHTNESS;

    if (useLightbar() && m_allGlow) {
        g_debug ("%s: calling lightbarOn", __PRETTY_FUNCTION__);
        lightbarOn();
    }
}

void CoreNaviManager::setSubtleLightbar (bool enable)
{
    m_isSubtleLightbar = enable;

    g_debug ("%s: set subtle lightbar mode to %d", __PRETTY_FUNCTION__, enable);
    if (useLightbar() && m_allGlow) {
	g_debug ("%s: calling lightbarOn", __PRETTY_FUNCTION__);
	lightbarOn();
    }
}


bool CoreNaviManager::useLightbar() 
{
    if (m_leds && m_hasLightBar && Settings::LunaSettings()->uiType != Settings::UI_MINIMAL)
        return true;
    return false;
}

void CoreNaviManager::lightbarOn (int brightness)
{
    if (brightness == 0) {
        lightbarOff();
        return;
    }

    if (brightness > 0)
        m_brightness = brightness;

    if (m_brightness < MINIMUM_BRIGHTNESS)
        m_brightness = MINIMUM_BRIGHTNESS;

    if (m_brightness > MAXIMUM_BRIGHTNESS)
        m_brightness = MAXIMUM_BRIGHTNESS;

    if (useLightbar()) {
        g_debug ("setting the lightbar to brightness %d", m_brightness);
	if (!m_isSubtleLightbar) {
	    if (m_allGlow)
		m_leds->ledSet (m_leds->All(), m_brightness);
	    else
		m_leds->ledRampTo (m_leds->All(), m_brightness, 200);
	    m_allGlow = true;
	}
	else {
	    if (m_allGlow)
		m_leds->ledSet (m_leds->All(), m_subtleModeBrightness);
	    else
		m_leds->ledRampTo (m_leds->All(), m_subtleModeBrightness, 200);
	    m_allGlow = true;
	}
    }
}


void CoreNaviManager::lightbarOff()
{
    if (useLightbar() && m_allGlow) {
        g_debug ("updating brightness of the core navi to 0");
        m_leds->ledRampTo (m_leds->All(), 0, 200);
        m_allGlow = false;
    }
}


//If we get another gesture before we finish rendering the previous one, try to just switch over and do the new one.
void CoreNaviManager::renderGesture(int key) {
	if (!m_leds)
		return;

	int brightness = m_brightness;

	switch (key) {
	case Qt::Key_CoreNavi_Launcher:
	case Qt::Key_CoreNavi_QuickLaunch:
		g_debug("%s: rendering launcher / quick launch", __PRETTY_FUNCTION__);
		m_leds->ledWaterdrop(brightness, 300, 500, 400, 500, false);
		m_centerGlow = false;
		break;
	case Qt::Key_CoreNavi_Back:
		g_debug("%s: rendering back", __PRETTY_FUNCTION__);
		m_leds->ledFade(m_leds->Left(), brightness, 200, 300, 200, 600);
		m_centerGlow = false;
		break;
	case Qt::Key_CoreNavi_Menu:
		g_debug("%s: rendering menu", __PRETTY_FUNCTION__);
		m_leds->ledFade(m_leds->Right(), brightness, 200, 300, 200, 600);
		m_centerGlow = false;
		break;
	case Qt::Key_CoreNavi_SwipeDown:
		//reverse waterdrop.
		g_debug("%s: rendering down", __PRETTY_FUNCTION__);
		m_leds->ledWaterdrop(brightness, 300, 500, 400, 500, true);
		m_centerGlow = false;
		break;
	case Qt::Key_CoreNavi_Previous:
		//Light all three in order
		g_debug("%s: rendering previous", __PRETTY_FUNCTION__);
		m_leds->ledFullFade(brightness, 100, 500, 100, 400, 300, false);
		m_centerGlow = false;
		break;
	case Qt::Key_CoreNavi_Next:
		//Reverse
		g_debug("%s: rendering next", __PRETTY_FUNCTION__);
		m_leds->ledFullFade(brightness, 100, 500, 100, 400, 300, true);
		m_centerGlow = false;
		break;
	default:
		return;
	}
}

//If we get another gesture before we finish rendering the previous one, try to just switch over and do the new one.
void CoreNaviManager::renderGestureOnLightbar(int key) {
	int brightness = m_brightness;

	m_lightbarHoldTimer.stop();
	m_lightbarRestoreTimer.stop();

	switch (key) {
	case Qt::Key_CoreNavi_Launcher:
	case Qt::Key_CoreNavi_QuickLaunch:
		g_debug("Rendering Launcher/QuickLaunch - ledWaterdrop");
		lightbarOff();
		m_leds->ledWaterdrop(brightness, 200, 400, 300, 400, false);
		m_lightbarRestoreTimer.start(200 + 400 + 300 + 400, true);
		break;
	case Qt::Key_CoreNavi_SwipeDown:
		//reverse waterdrop.
		g_debug("%s: rendering down", __PRETTY_FUNCTION__);
		lightbarOff();
		m_leds->ledWaterdrop(brightness, 200, 400, 300, 400, true);
		m_lightbarRestoreTimer.start(200 + 400 + 300 + 400, true);
		break;
	case Qt::Key_CoreNavi_Previous:
	case Qt::Key_CoreNavi_Back:
		g_debug("%s: rendering previous/back", __PRETTY_FUNCTION__);
		m_lightbarSwipeLeftTimer.start(100, true);
		break;
	case Qt::Key_CoreNavi_Menu:
	case Qt::Key_CoreNavi_Next:
		g_debug("%s: rendering next/menu", __PRETTY_FUNCTION__);
		m_lightbarSwipeRightTimer.start(100, true);
		break;
	default:
		return;
	}
}

bool CoreNaviManager::restoreLightbar() {
    if (!useLightbar())
        return true;

    if (m_systemUiController->isCardWindowMaximized() && !m_systemUiController->isScreenLocked()) {
        g_debug ("Card window is still maximized, turning lightbar on");
        lightbarOn();
    }
    else {
        g_debug ("card window is minimized, turning lightbar off");
        lightbarOff();
    }
    return true;
}

bool CoreNaviManager::seesawLightbar() {

    if (!useLightbar())
        return true;

    g_debug ("Hold timer triggered - starting seesaw");
    if (m_systemUiController->isCardWindowMaximized() && m_metaKeyDown) {
	m_leds->ledSet (m_leds->All(), m_brightness);
	m_leds->ledSeesaw (m_brightness, 500, true);
	m_allGlow = false;
    }

    return true;
}

bool CoreNaviManager::swipeLeftLightbar() {


    g_debug ("swipe left triggered");
    if (useLightbar() && m_systemUiController->isCardWindowMaximized() && !m_systemUiController->isScreenLocked()) {
	g_debug ("Rendering Back/Previous when card is maximized - ledLightbarFullSwipe");
	// resetting the led before rendering the gesture
	m_leds->ledSet (m_leds->All(), m_brightness);
	m_leds->ledLightbarFullSwipe (m_brightness, m_animationSpeed, true);
	m_allGlow = true;
    }
    else {
	g_debug ("Rendering Back/Previous when card is minimized");
	// resetting the led before rendering the gesture
	if (m_allGlow) {
	    g_debug ("Calling ledFadeOff");
	    m_leds->ledSet (m_leds->All(), m_brightness);
	    m_leds->ledFadeOff (m_brightness, m_animationSpeed, true);
	}
	else  {
	    g_debug ("Calling ledSwipe");
	    m_leds->ledSet (m_leds->All(), 0);
	    m_leds->ledSwipe (m_brightness, m_animationSpeed, true);
	}

	m_allGlow = false;
    }
    return true;
}

bool CoreNaviManager::swipeRightLightbar() {

    g_debug ("Swipe right triggered");
    if (useLightbar() && m_systemUiController->isCardWindowMaximized() && !m_systemUiController->isScreenLocked()) {
	g_debug ("Rendering Menu/Next when card is maximized - ledLightbarFullSwipe");
	// resetting the led before rendering the gesture
	m_leds->ledSet (m_leds->All(), m_brightness);
	m_leds->ledLightbarFullSwipe (m_brightness, m_animationSpeed, false);
	m_allGlow = true;
    }
    else {
	g_debug ("Rendering Menu/Next when card is minimized - ledSwipe");
	m_leds->ledSet (m_leds->All(), 0);
	m_leds->ledSwipe (m_brightness, m_animationSpeed, false);
	m_allGlow = false;
    }
    return true;
}

bool CoreNaviManager::handleEvent(QEvent *event) {
	if (!m_leds) {
		return false;
	}

	QKeyEvent *keyEvent = NULL;
	if (QEvent::KeyPress == event->type() || QEvent::KeyRelease == event->type()) {
		keyEvent = static_cast<QKeyEvent*> (event);
	}

	if (keyEvent && keyEvent->isGestureKey()) {
		switch (keyEvent->key()) {
		case Qt::Key_CoreNavi_QuickLaunch:
		case Qt::Key_CoreNavi_Launcher:
		case Qt::Key_CoreNavi_Previous:
		case Qt::Key_CoreNavi_Next:
		case Qt::Key_CoreNavi_Back:
		case Qt::Key_CoreNavi_Menu:
		case Qt::Key_CoreNavi_SwipeDown:
		case Qt::Key_CoreNavi_Home:
			//One shot events
			if (keyEvent->type() == QEvent::KeyPress) {
				if (m_hasLightBar) {
					renderGestureOnLightbar(keyEvent->key());
				} else {
					renderGesture(keyEvent->key());
				}
			}
			break;
		case Qt::Key_CoreNavi_Meta:
			if (!m_hasLightBar) {
				//continous glow
				if (keyEvent->type() == QEvent::KeyPress) {
					setMetaGlow(true);
				} else {
					setMetaGlow(false);
				}
			} else {
				if (useLightbar()) {
					if (keyEvent->type() == QEvent::KeyPress && m_systemUiController->isCardWindowMaximized()
							&& !m_systemUiController->isScreenLocked()) {
						// this is just finger down in the meta area, triggering hold timer
						g_debug("Key down in core navi area, triggering lightbarHoldTimer");
						m_metaKeyDown = true;
						m_lightbarHoldTimer.start(300, true);
					} else if (keyEvent->type() == QEvent::KeyRelease) {
						m_lightbarHoldTimer.stop();
						if (!m_systemUiController->isScreenLocked() && m_systemUiController->isCardWindowMaximized()
								&& !m_allGlow && !m_lightbarSwipeLeftTimer.running()
								&& !m_lightbarSwipeRightTimer.running() && !m_lightbarRestoreTimer.running()) {
							m_metaKeyDown = false;
							g_debug("key up, setting leds back to %d", m_brightness);
							m_leds->ledSet(m_leds->All(), m_brightness);
							m_allGlow = true;
						}
					}
				}
			}
			break;
		default:
			break;
		}
	}
	return false;
}


bool CoreNaviManager::handleEvent(sptr<Event> e)
{
	return false;
}

CoreNaviManager::LedRequest::LedRequest (const std::string& appId, const std::string& requestId)
{ 
    this->appId = appId; 
    this->requestId = requestId;
}

void CoreNaviManager::addStandbyRequest (const std::string& appId, const std::string& requestId)
{
    if (!m_leds)
        return;

    LedRequest newRequest (appId, requestId);

    m_ledRequestList.push_back (newRequest);

    g_debug ("CoreNaviManager: Added throbber request from appId %s requestId %s list length %d\n",
            appId.c_str(), requestId.c_str(), m_ledRequestList.size());

    stopStandbyLeds();
    startStandbyLeds();

}

void CoreNaviManager::removeStandbyRequest (const std::string& appId, const std::string& requestId)
{
    if (!m_leds)
        return;

    /* iterate through list and find the request */
    for (LedRequestList::iterator it = m_ledRequestList.begin(); it != m_ledRequestList.end(); ++it) {
        LedRequest& req = *it;
        if (req.appId == appId && req.requestId == requestId) {
            m_ledRequestList.erase (it);
            break;
        }
    }

    g_debug ("CoreNaviManager: Removed throbber request for appId %s requestId %s list length %d\n",
            appId.c_str(), requestId.c_str(), m_ledRequestList.size());

    if (m_ledRequestList.empty())  {
        /* removed all requests, stop leds */
	stopStandbyLeds();
    }

}

void CoreNaviManager::clearStandbyRequests (const std::string& appId) 
{
    if (!m_leds)
        return;

    for (LedRequestList::iterator it = m_ledRequestList.begin(); it != m_ledRequestList.end(); ++it) {
        LedRequest& req = *it;
        if (req.appId == appId) {
            removeStandbyRequest (req.appId, req.requestId);
        }
        if (m_ledRequestList.empty())
            break;
    }
}

int CoreNaviManager::numStandbyRequests() const {
    return m_ledRequestList.size();
}

void CoreNaviManager::clearAllStandbyRequests() {
    if (!m_leds)
        return;

    g_debug ("*CoreNaviManager: clearing all requests\n");
    stopStandbyLeds();
    m_ledRequestList.clear();
}


void CoreNaviManager::startStandbyLeds()
{
    if (!m_leds)
	return;

    if (m_allGlow || m_centerGlow) {
	m_leds->ledSet (m_leds->All(), 0);
	m_allGlow = false;
    }

    if (m_ledRequestList.empty()) {
	g_debug ("CoreNaviManager: empty list, returning immediately\n");
	return;
    }

    if (m_isStandbyLedsActive)   {
	g_debug ("CoreNaviManager: throbber already on\n");
	return;
    }

    if (m_isStandbyLedsActive)
	stopStandbyLeds();

    if (Preferences::instance()->ledThrobberEnabled()) {
	int brightness = 50;
	if (m_ambientLightSensor && m_ambientLightSensor->getCurrentRegion() == ALS_REGION_OUTDOOR) {
	    brightness = 100;
	    g_debug ("CoreNaviManager:: setting throbber at outdoor brightness %d", brightness);
	}
	else {
	    // New throbber brightness as defined by HI
	    time_t rawtime;
	    struct tm* timeinfo;
	    time (&rawtime);
	    timeinfo = localtime (&rawtime);
	    if (timeinfo->tm_hour > 6  || timeinfo->tm_hour < 22) {
		brightness = 50;
		g_debug ("CoreNaviManager:: setting at indoor brightness %d (current time = %d:%d)",
			brightness, timeinfo->tm_hour, timeinfo->tm_min);
	    }
	    else {
		if (m_hasLightBar) {
		    brightness = 25;
		}
		else {
		    brightness = 15;
		}
		g_debug ("CoreNaviManager:: setting at night-time brightness %d (current time = %d:%d)",
			brightness, timeinfo->tm_hour, timeinfo->tm_min);
	    }
	}

	LedRequest request = m_ledRequestList.front();
	if (!m_displayManager->isDisplayOn()) {
	    g_debug ("CoreNaviManager: starting throbber for %d requests, first in queue appId %s requestId %s",
		    m_ledRequestList.size(), request.appId.c_str(), request.requestId.c_str());
	    if (m_hasLightBar) {
		m_leds->ledDoublePulse (m_leds->All(), brightness, 150, 100, 100, 3000, 1);
	    }
	    else {
		m_leds->ledDoublePulse (m_leds->Center(), brightness, 150, 100, 100, 3000, 1);
	    }
	    m_isStandbyLedsActive = true;
	}
    }
    else  {
	g_debug ("CoreNaviManager: led throb is not enabled");
    }
}

void CoreNaviManager::stopStandbyLeds() {
    if (!m_leds)
        return;

    if (m_isStandbyLedsActive) {
        m_leds->stopAll();
        m_leds->ledSet (m_leds->All(), 0);
	m_allGlow = false;
        /* add ledFade for other leds when support is added */
        m_isStandbyLedsActive = false;
    }
}


void CoreNaviManager::slotDisplayStateChange (int displaySignal) {
    switch (displaySignal) {
	case DISPLAY_SIGNAL_OFF:
	    lightbarOff();
	    startStandbyLeds();
	    break;
	case DISPLAY_SIGNAL_DIM: 
	    lightbarOff();
	    stopStandbyLeds();
	    startStandbyLeds();
	    break;
	case DISPLAY_SIGNAL_ON:
	    stopStandbyLeds();
	    if (useLightbar() && !m_systemUiController->isScreenLocked() && m_systemUiController->isCardWindowMaximized()) {
		g_debug ("%s: display is unlocked and card is maximized, calling lightbarOn", __PRETTY_FUNCTION__);
		lightbarOn();
	    }
	    break;
	case DISPLAY_SIGNAL_OFF_ON_CALL:
	    lightbarOff();
	    break;
	case DISPLAY_SIGNAL_DOCK:
	    lightbarOff();
	    startStandbyLeds();
	    break;
	default:
	    g_debug ("received invalid display change signal\n");
	    break;
    }
}

void CoreNaviManager::slotLockStateChange (int displaySignal, int displayEvent) {
    switch (displaySignal) {
        case DISPLAY_LOCK_SCREEN: 
        case DISPLAY_DOCK_SCREEN: 
	    if (useLightbar()) {
		// turn off leds if the screen is locked
                lightbarOff();
	    }
	    // when going from off to nightstand, the lock comes in after the display dim signal comes in
	    // so the throbber needs to be reset.
	    stopStandbyLeds();
	    startStandbyLeds();
	    break;
        case DISPLAY_UNLOCK_SCREEN:
	    clearAllStandbyRequests();
	    if (useLightbar() && m_systemUiController->isCardWindowMaximized()
			    && !m_systemUiController->isScreenLocked() && m_displayManager->isDisplayOn()) {
                g_debug ("%s: screen unlocked when card is maximized, calling lightbarOn", __PRETTY_FUNCTION__);
                lightbarOn();
	    }
            break;
        default:
            g_debug ("received invalid lock change signal\n");
            break;
    }
}

void CoreNaviManager::slotCardWindowMaximized () {
    if (!m_leds)
        return;

    if (useLightbar() && !m_systemUiController->isScreenLocked() && m_displayManager->isDisplayOn()) {
        g_debug ("card window maximized calling lightbarOn()");
        lightbarOn();
    }
}

void CoreNaviManager::slotCardWindowMinimized () {
    if (!m_leds)
        return;
    if (useLightbar() && !m_lightbarRestoreTimer.running()
	    && !m_lightbarSwipeLeftTimer.running() && !m_lightbarSwipeRightTimer.running())
    {
        g_debug ("ramping leds down");
	m_leds->ledSet(m_leds->All(), m_brightness);
        m_leds->ledRampTo (m_leds->All(), 0, 500);
	m_allGlow = false;
    }
}

void CoreNaviManager::slotFocusMaximizedCardWindow (bool enable) {
    if (!m_leds)
        return;

    if (useLightbar() && !m_systemUiController->isScreenLocked()) {
	if (enable && m_displayManager->isDisplayOn()) {
	    g_debug ("card window got focus, calling lightbarOn()");
	    lightbarOn();
	}
	else {
	    g_debug ("card window lost focus, calling lightbarOff()");
	    lightbarOff();
	}
    }
}

