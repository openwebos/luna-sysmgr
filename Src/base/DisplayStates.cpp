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




#include "Common.h"

#include "AppDirectRenderingArbitrator.h"
#include "HostBase.h"
#include "CoreNaviManager.h"
#include "DisplayStates.h"
#include "DisplayManager.h"
#include "WindowServer.h"
#include "SystemService.h"
#include "Preferences.h"
#include "SystemUiController.h"
#include "Time.h"

#define DOCK_MODE_EXIT_TIMER 1000

// ---------------------- DisplayStateBase -------------------------------

DisplayStateBase::DisplayStateBase()
    : dm (NULL)
{
}
 
bool DisplayStateBase::isDisplayUnlocked()
{
    if (isUSBCharging() || isSliderOpen()
         || Settings::LunaSettings()->uiType == Settings::UI_MINIMAL
         || Settings::LunaSettings()->disableLocking)
        return true;

    return false;
}

bool DisplayStateBase::isUSBCharging() 
{ 
    if (!dm)
        dm = DisplayManager::instance();
    return dm->isUSBCharging(); 
}

bool DisplayStateBase::isOnCall() 
{
    if (!dm)
        dm = DisplayManager::instance();
    return dm->isOnCall();
}

bool DisplayStateBase::isSliderOpen() 
{
    if (!dm)
        dm = DisplayManager::instance();

    return (dm->hasSlider() && dm->isSliderOpen());
}

bool DisplayStateBase::isDNAST() 
{
    if (!dm)
        dm = DisplayManager::instance();

    return dm->isDNAST();
}

bool DisplayStateBase::isBacklightOn() 
{
    if (!dm)
        dm = DisplayManager::instance();

    return (dm->isBacklightOn());
}

bool DisplayStateBase::isTouchpanelOn() 
{
    if (!dm)
        dm = DisplayManager::instance();

    return (dm->isTouchpanelOn());
}

bool DisplayStateBase::isDisplayOn() 
{
    if (!dm)
        dm = DisplayManager::instance();

    return (dm->isDisplayOn());
}

int DisplayStateBase::dimTimeout() 
{
    if (!dm)
        dm = DisplayManager::instance();

    return dm->dimTimeout();
}

int DisplayStateBase::offTimeout() 
{
    if (!dm)
        dm = DisplayManager::instance();

    return dm->offTimeout();
}

int DisplayStateBase::lockedOffTimeout() 
{
    if (!dm)
        dm = DisplayManager::instance();

    return dm->lockedOffTimeout();
}

int DisplayStateBase::lastEvent() 
{
    if (!dm)
        dm = DisplayManager::instance();

    return dm->lastEvent();
}

bool DisplayStateBase::isOnPuck() 
{
    if (!dm)
        dm = DisplayManager::instance();

    return dm->isOnPuck();
}

bool DisplayStateBase::isProximityActivated()
{
    if (!dm)
        dm = DisplayManager::instance();

    return dm->isProximityActivated();
}

bool DisplayStateBase::isDemo()
{
    if (!dm)
        dm = DisplayManager::instance();

    return dm->isDemo();
}

bool DisplayStateBase::orientationSensorOn()
{
    if (!dm)
        dm = DisplayManager::instance();
    return dm->orientationSensorOn();
}

bool DisplayStateBase::orientationSensorOff()
{
    if (!dm)
        dm = DisplayManager::instance();
    return dm->orientationSensorOff();
}

void DisplayStateBase::startInactivityTimer()
{
    updateLastEvent();
}

void DisplayStateBase::stopInactivityTimer()
{
    updateLastEvent();
}

void DisplayStateBase::updateLastEvent()
{
    if (!dm)
        dm = DisplayManager::instance();

    dm->m_lastEvent = Time::curTimeMs();
}

void DisplayStateBase::displayOn(bool als)
{
    if (!dm)
        dm = DisplayManager::instance();
    dm->displayOn(als);
}

void DisplayStateBase::displayDim()
{
    if (!dm)
        dm = DisplayManager::instance();
    dm->displayDim();
}

void DisplayStateBase::displayOff()
{
    if (!dm)
        dm = DisplayManager::instance();
    dm->displayOff();
}

bool DisplayStateBase::proximityOff()
{
    if (!dm)
        dm = DisplayManager::instance();
    return dm->proximityOff();
}

bool DisplayStateBase::proximityOn()
{
    if (!dm)
        dm = DisplayManager::instance();
    return dm->proximityOn();
}

int DisplayStateBase::getCurrentAlsRegion() {
    if (!dm)
        dm = DisplayManager::instance();

    if (dm->m_als && Preferences::instance()->isAlsEnabled())
        return dm->m_als->getCurrentRegion();

    return ALS_REGION_INDOOR;
}

void DisplayStateBase::enablePainting() {
    WindowServer::instance()->setPaintingDisabled (false);
	AppDirectRenderingArbitrator::resume();
}

void DisplayStateBase::disablePainting() {
    WindowServer::instance()->setPaintingDisabled (true);
	AppDirectRenderingArbitrator::suspend();
}

void DisplayStateBase::emitDisplayStateChange(int displaySignal)
{
    if (!dm)
        dm = DisplayManager::instance();

    dm->emitDisplayStateChange(displaySignal);
}
void DisplayStateBase::changeDisplayState (DisplayState displayState, DisplayEvent displayEvent, sptr<Event> event)
{
    if (!dm)
        dm = DisplayManager::instance();

	DisplayState currentState = state();

    if (currentState == displayState)
	return;

    if (displayState >= DisplayStateMax)
	return;

    leave (displayState, displayEvent, event);

    dm->changeDisplayState (displayState, currentState, displayEvent, event);
    return;
}

void DisplayStateBase::updateLockState (DisplayLockState lockState, DisplayEvent displayEvent) 
{
    if (!dm)
	dm = DisplayManager::instance();

    dm->updateLockState (lockState, state(), displayEvent);
}

bool DisplayStateBase::updateBrightness (int alsRegion)
{
    return false;
}


// ---------------------- DisplayOff -------------------------------------

void DisplayOff::enter (DisplayState state, DisplayEvent displayEvent, sptr<Event> event)
{
    g_message ("%s: entering state", __PRETTY_FUNCTION__);
    if (DisplayStateOffSuspended != state)
    {
        orientationSensorOff();
        displayOff();
        disablePainting();
        updateLockState (DisplayLockLocked, displayEvent);
        g_debug ("Emitting DISPLAY_SIGNAL_OFF");
        emitDisplayStateChange (DISPLAY_SIGNAL_OFF);
    }
}

void DisplayOff::handleEvent (DisplayEvent displayEvent, sptr<Event> event) 
{
    switch (displayEvent) {
	case DisplayEventPowerKeyPress:
	    if (isBacklightOn()) {
		g_warning ("backlight not off yet, debouncing power key");
		break;
	    }
	    if (isOnPuck()) {
		if (isDisplayUnlocked() || isOnCall()) {
		    g_message ("%s: power key press while display %s (usb %s slider %s disable locking %s) and phonecall %s, moving to OnPuck", __PRETTY_FUNCTION__,
			    (isDisplayUnlocked()) ? "unlocked" : "locked", 
			    (isUSBCharging()) ? "connected" : "disconnected",
			    (isSliderOpen()) ? "open" : "closed",
			    (Settings::LunaSettings()->disableLocking) ? "set" : "unset",
			    (isOnCall()) ? "on" : "off");
		    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		}
		else {
		    g_debug ("%s: power key press when on puck, move to dock mode", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateDockMode, displayEvent, event);
		}
	    }
	    else if (isDisplayUnlocked() || isOnCall()) {
		g_message ("%s: power key press while display %s (usb %s slider %s disable locking %s) and phonecall %s, moving to on", __PRETTY_FUNCTION__,
			(isDisplayUnlocked()) ? "unlocked" : "locked", 
			(isUSBCharging()) ? "connected" : "disconnected",
			(isSliderOpen()) ? "open" : "closed",
			(Settings::LunaSettings()->disableLocking) ? "set" : "unset",
			(isOnCall()) ? "on" : "off");
		changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    else  {
		g_debug ("%s: power key press, moving to OnLocked", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnLocked, displayEvent, event);
	    }
	    break;

	case DisplayEventPowerKeyHold:
	    if (isOnPuck()) {
		g_debug ("%s: power key hold on puck, moving to on", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    }
	    else if (isDisplayUnlocked()) {
		g_debug ("%s: power key hold, moving to on", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    else  {
		g_debug ("%s: power key hold, moving to OnLocked", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnLocked, displayEvent, event);
	    }
	    break;

	case DisplayEventOnPuck:
	    if (isOnCall() || isDisplayUnlocked()) {
		g_debug ("%s: on puck on a call, moving to onpuck", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    }
	    else {
		g_debug ("%s: on puck, moving to dock mode", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateDockMode, displayEvent, event);
	    }
	    break;

	case DisplayEventOffPuck:
	    g_debug ("%s: off puck received in Off state, staying off", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventUsbIn:
	    if (isOnPuck()) {
		g_debug ("%s: usb in when on puck, move to on puck", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    }
	    else {
		g_debug ("%s: usb in, moving to on", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    break;

	case DisplayEventUsbOut:
	    break;

	case DisplayEventIncomingCall:
	    if (isOnPuck()) {
		g_debug ("%s: incoming call when on puck, move to OnPuck", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    }
	    else {
		g_debug ("%s: incoming call, moving to on / onlocked", __PRETTY_FUNCTION__);
		if (isDisplayUnlocked())
		    changeDisplayState (DisplayStateOn, displayEvent, event);
		else 
		    changeDisplayState (DisplayStateOnLocked, displayEvent, event);
	    }

	    break;

	case DisplayEventIncomingCallDone:
	    g_warning ("%s: incoming call done, should never happen", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventOnCall:
	    if (isOnPuck()) {
		g_debug ("%s: on call on puck, move to OnPuck", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    }
	    else if (isDisplayUnlocked()) {
		g_debug ("%s: on call, moving to On", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    else {
		g_debug ("%s: on call, moving to OnLocked", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnLocked, displayEvent, event);
	    }
	    break;

	case DisplayEventOffCall:
	    g_warning ("%s: off call - invalid event, should be OffOnCall", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventSliderOpen:
	    if (isOnPuck()) {
		g_debug ("%s: slider open when on puck, move to OnPuck", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    }
	    else {
		g_debug ("%s: slider open, moving to on", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    break;

	case DisplayEventSliderClose:
	    break;

	case DisplayEventAlsChange:
	    break;

	case DisplayEventProximityOn:
	    g_warning ("%s: proximity on event received in off state - invalid event, should be OffOnCall", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventProximityOff:
	    g_warning ("%s: proximity off event received in off state - invalid event, should be OffOnCall", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventApiOn:
	    if (isOnPuck()) {
		    if (isOnCall()) {
			    g_debug ("%s: public api on called when on puck on call, move to on puck", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		    }
		    else {
			    g_debug ("%s: public api on called when on puck, move to dock mode", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateDockMode, displayEvent, event);
		    }
	    }
	    else if (isDisplayUnlocked()) {
		g_debug ("%s: public api on called, move to on", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    else {
		g_debug ("%s: public api on called, move to onLocked", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnLocked, displayEvent, event);
	    }
	    break;

	case DisplayEventApiDim:
	    break;

	case DisplayEventApiOff:
	    break;

	case DisplayEventUserActivity:
	    if (isOnPuck()) {
		g_debug ("%s: user activity when on puck, move to on puck", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    }
	    else if (isDisplayUnlocked()) {
		g_debug ("%s: received user activity when display is off and unlocked, moving to on", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    else {
		g_debug ("%s: received user activity when display is off and locked, moving to onlocked", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnLocked, displayEvent, event);
	    }
	    break;

	case DisplayEventUserActivityExternalInput:
	    if (isOnPuck()) {
			g_debug ("%s: external input user activity when on puck, move to on puck", __PRETTY_FUNCTION__);
			changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    }
	    else  {
			g_debug ("%s: external input user activity, moving to on", __PRETTY_FUNCTION__);
			changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    break;		

	case DisplayEventHomeKeyPress:
	    if (isOnPuck()) {
		if (isDisplayUnlocked() || isOnCall()) {
		    g_message ("%s: home key press while display %s (usb %s slider %s disable locking %s) and phonecall %s, moving to OnPuck", __PRETTY_FUNCTION__,
			    (isDisplayUnlocked()) ? "unlocked" : "locked", 
			    (isUSBCharging()) ? "connected" : "disconnected",
			    (isSliderOpen()) ? "open" : "closed",
			    (Settings::LunaSettings()->disableLocking) ? "set" : "unset",
			    (isOnCall()) ? "on" : "off");
		    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		}
		else {
		    g_debug ("%s: home key press when on puck, move to dock mode", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateDockMode, displayEvent, event);
		}
	    }
	    else if (isDisplayUnlocked() || isOnCall()) {
		g_message ("%s: home key press while display %s (usb %s slider %s disable locking %s) and phonecall %s, moving to on", __PRETTY_FUNCTION__,
			(isDisplayUnlocked()) ? "unlocked" : "locked", 
			(isUSBCharging()) ? "connected" : "disconnected",
			(isSliderOpen()) ? "open" : "closed",
			(Settings::LunaSettings()->disableLocking) ? "set" : "unset",
			(isOnCall()) ? "on" : "off");
		changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    else  {
		g_debug ("%s: home key press, moving to OnLocked", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOnLocked, displayEvent, event);
	    }
	    break;

	case DisplayEventUpdateBrightness:
	    break;
	case DisplayEventLockScreen:
	    break;
	case DisplayEventUnlockScreen:
	    break;
	case DisplayEventPowerdSuspend:
	    g_debug ("%s: received suspend event, going to off suspended", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOffSuspended, displayEvent, event);
	    break;
	case DisplayEventPowerdResume:
	    break;
	case DisplayEventApiDock:
	    if (!isOnCall()) {
		    g_debug ("%s: received dock event, going to dock mode", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateDockMode, displayEvent, event);
	    }
	    break;
	case DisplayEventApiUndock:
	    break;
	default:
	    break;
    }

}

void DisplayOff::leave (DisplayState newState, DisplayEvent displayEvent, sptr<Event> event)
{
    g_debug ("%s:", __PRETTY_FUNCTION__);
}


// ---------------------- DisplayOffOnCall ---------------------------------

DisplayOffOnCall::DisplayOffOnCall()
{
}

void DisplayOffOnCall::enter (DisplayState state, DisplayEvent displayEvent, sptr<Event> event)
{
    g_message ("%s: entering state", __PRETTY_FUNCTION__);
    orientationSensorOff();
    displayOff();
    disablePainting();
    emitDisplayStateChange (DISPLAY_SIGNAL_OFF_ON_CALL);
}

void DisplayOffOnCall::handleEvent (DisplayEvent displayEvent, sptr<Event> event) 
{
    switch (displayEvent) {
	case DisplayEventPowerKeyPress:
	    if (isBacklightOn()) {
		g_warning ("backlight not off yet, debouncing power key");
		break;
	    }
            if (!isProximityActivated()) {
		    if (isOnPuck()) {
			    g_debug ("%s: power key press on puck, proximity not activated, going to OnPuck", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		    }
		    else {
			    g_debug ("%s: power key press, proximity not activated, going to on", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOn, displayEvent, event);
		    }
            }
            else {
                g_message ("%s: power key press when off on call, proximity is activated, ignoring", __PRETTY_FUNCTION__);
            }
	    break;

	case DisplayEventPowerKeyHold:
            if (!isProximityActivated()) {
		    if (isOnPuck()) {
			    g_debug ("%s: power key on puck hold going to OnPuck", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		    }
		    else {
			    g_debug ("%s: power key hold going to On", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOn, displayEvent, event);
		    }
            }
	    break;

	case DisplayEventOnPuck:
            g_debug ("%s: on a call, going to On", __PRETTY_FUNCTION__);
            changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    break;

	case DisplayEventOffPuck:
	    g_warning ("%s: off puck received - invalid event", __PRETTY_FUNCTION__);
	    break;

        case DisplayEventUsbIn:
	    break;

        case DisplayEventUsbOut:
	    break;

        case DisplayEventIncomingCall:
	    if (!isProximityActivated()) {
		    if (isOnPuck()) {
			    g_message ("%s: incoming call when off on call on puck, proximity not activated, going OnPuck", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		    }
		    else {
			    g_message ("%s: incoming call when off on call, proximity not activated, going On", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOn, displayEvent, event);
		    }
            }
            else {
                g_message ("%s: incoming call when off on call, proximity is activated, ignoring", __PRETTY_FUNCTION__);
            }
            break;

        case DisplayEventIncomingCallDone:
            g_message ("%s: incoming call done when off on call, ignoring", __PRETTY_FUNCTION__);
            break;

	case DisplayEventOnCall:
	    g_warning ("%s: on call event - invalid event", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventOffCall:
            if (!isProximityActivated()) {
		    if (isOnPuck()) {
			    g_message ("%s: off call on puck, proximity not activated, going OnPuck", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		    } 
		    else {
			    g_debug ("%s: off call, going to on", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOn, displayEvent, event);
		    }
            }
	    else {
		    g_message ("Call disconnected, not going to on till proximity sensor has unactivated as per HI requirement");
	    }
	    break;

        case DisplayEventSliderOpen:
            if (!isProximityActivated()) {
		    if (isOnPuck()) {
			    g_message ("%s: slider open on puck, proximity not activated, going OnPuck", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		    } 
		    else {
			    g_debug ("%s: slider open, going to on", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOn, displayEvent, event);
		    }
            }
	    break;

        case DisplayEventSliderClose:
	    break;

	case DisplayEventAlsChange:
            g_debug ("%s: ignoring Als change", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventProximityOn:
	    break;

	case DisplayEventProximityOff:
	    if (isOnPuck()) {
		    g_message ("%s: proximity off event on puck, moving to OnPuck", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    }
	    else {
		    g_message ("%s: proximity off event, moving to on", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    break;

	case DisplayEventApiOn:
	    if (!isProximityActivated()) {
		    if (isOnPuck()) {
			    g_message ("%s: public api on called when off on call on puck, proximity not activated, going OnPuck", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		    }
		    else {
			    g_message ("%s: public api on called when off on call, proximity not activated, going on", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOn, displayEvent, event);
		    }
            }
            else {
                g_message ("%s: public api on called when off on call, proximity is activated, ignoring", __PRETTY_FUNCTION__);
            }
	    break;

	case DisplayEventApiDim:
	    break;
	case DisplayEventApiOff:
	    break;
	case DisplayEventUserActivity:
	    if (!isProximityActivated()) {
		    if (isOnPuck()) {
			    g_message ("%s: user activity when off on call on puck, proximity not activated, going OnPuck", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		    }
		    else {
			    g_message ("%s: user activity when off on call, proximity not activated, going on", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOn, displayEvent, event);
		    }
	    }
	    break;

	case DisplayEventUserActivityExternalInput:
	    if (isOnPuck()) {
			g_debug ("%s: external input user activity when on puck, move to on puck", __PRETTY_FUNCTION__);
			changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    }
	    else  {
			g_debug ("%s: external input user activity, moving to on", __PRETTY_FUNCTION__);
			changeDisplayState (DisplayStateOn, displayEvent, event);
	    }
	    break;		
		
        case DisplayEventUpdateBrightness:
            break;
        case DisplayEventLockScreen:
	    g_message ("%s: locking screen", __PRETTY_FUNCTION__);
	    updateLockState (DisplayLockLocked, displayEvent);
            break;
        case DisplayEventUnlockScreen:
	    g_message ("%s: unlocking screen", __PRETTY_FUNCTION__);
	    updateLockState (DisplayLockUnlocked, displayEvent);
            break;
	case DisplayEventPowerdSuspend:
#if defined (MACHINE_BROADWAY) || defined(MACHINE_MANTARAY)
	    g_debug ("%s: received suspend event, going to off suspended", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOffSuspended, displayEvent, event);
#else
	    g_warning ("%s: got powerd suspend, invalid event in current state", __PRETTY_FUNCTION__);
#endif
	    break;
	case DisplayEventPowerdResume:
	    break;
	case DisplayEventHomeKeyPress:
            if (!isProximityActivated()) {
		    if (isOnPuck()) {
			    g_debug ("%s: home key press on puck, proximity not activated, going to OnPuck", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
		    }
		    else {
			    g_debug ("%s: home key press, proximity not activated, going to on", __PRETTY_FUNCTION__);
			    changeDisplayState (DisplayStateOn, displayEvent, event);
		    }
            }
            else {
                g_message ("%s: power key press when off on call, proximity is activated, ignoring", __PRETTY_FUNCTION__);
            }
	    break;

	default:
	    break;
    }

}

void DisplayOffOnCall::leave (DisplayState newState, DisplayEvent displayEvent, sptr<Event> event)
{
    g_debug ("%s:", __PRETTY_FUNCTION__);
}

// ---------------------- DisplayOn ----------------------------------------

DisplayOn::DisplayOn()
    : m_timerUser (new Timer<DisplayOn>(HostBase::instance()->masterTimer(), this, &DisplayOn::timeoutUser))
    , m_timerInternal (new Timer<DisplayOn>(HostBase::instance()->masterTimer(), this, &DisplayOn::timeoutInternal))
{
}

void DisplayOn::enter (DisplayState state, DisplayEvent displayEvent, sptr<Event> event)
{
    g_message ("%s: entering state", __PRETTY_FUNCTION__);
    enablePainting();
    updateLockState (DisplayLockUnlocked, displayEvent);
    g_debug ("Emitting DISPLAY_SIGNAL_ON");
    emitDisplayStateChange (DISPLAY_SIGNAL_ON);
    orientationSensorOn();
    displayOn(false);

    if (displayEvent != DisplayEventApiOn)
	startUserInactivityTimer();
    else 
	startInternalInactivityTimer();
}

void DisplayOn::startInactivityTimer()
{
    startUserInactivityTimer();
}

void DisplayOn::startUserInactivityTimer()
{
    if (m_timerInternal->running())
        m_timerInternal->stop();

    if (m_timerUser->running())
        m_timerUser->stop();

    if (isDNAST()) {
        g_warning ("not starting timer due to DNAST enabled");
        return;
    }

#if (defined(TARGET_DESKTOP) || defined (TARGET_EMULATOR))
    g_debug("%s: not starting timer on emulator or desktop", __PRETTY_FUNCTION__);
    return;

#else
    g_debug ("%s: %d ms", __PRETTY_FUNCTION__, dimTimeout());
    if (isDemo()) {
        g_warning ("in demo mode, inactivity timer started for %d", dimTimeout() + offTimeout());
        m_timerUser->start (dimTimeout() + offTimeout());
    }
    else {
        m_timerUser->start (dimTimeout());
    }

#endif
}

void DisplayOn::startInternalInactivityTimer()
{
    if (isDNAST()) {
        g_warning ("not starting timer due to DNAST enabled");
        return;
    }

    if (m_timerUser->running()) {
        g_warning ("user inactivity timer is running, ignoring this");
        return;
    }

    g_debug ("%s: %d ms", __PRETTY_FUNCTION__, dimTimeout());
    m_timerInternal->start (lockedOffTimeout());
}

void DisplayOn::stopInactivityTimer()
{
    g_debug ("%s: ", __PRETTY_FUNCTION__);
    if (m_timerUser->running())
        m_timerUser->stop();
    if (m_timerInternal->running())
        m_timerInternal->stop();
}

bool DisplayOn::timeoutUser()
{
    uint32_t now = Time::curTimeMs();
    g_debug ("%s: now=%i last=%i diff=%i", __PRETTY_FUNCTION__, now, lastEvent(), now - lastEvent());

    // check for brick mode
    if (SystemService::instance()->brickMode() || 
	    !SystemUiController::instance()->bootFinished() || 
	    WindowServer::instance()->progressRunning())
	return false;


    if (dimTimeout() == 0 && offTimeout() == 0)
	return false;

    if (isDNAST())
	return false;

    if (now <  (unsigned int) dimTimeout() + lastEvent())
    {
	g_debug ("%s: restart the dim timer for %u ms", __PRETTY_FUNCTION__, dimTimeout() + lastEvent() - now);
	m_timerUser->start (dimTimeout() + lastEvent() - now);
    }
    else
    {
	g_message ("%s: change to dim in on", __PRETTY_FUNCTION__);
	if (isDemo()) {
	    if (isOnCall())
		changeDisplayState (DisplayStateOffOnCall, DisplayEventTimeout, NULL);
	    else
		changeDisplayState (DisplayStateOff, DisplayEventTimeout, NULL);
	}
	else 
	    changeDisplayState (DisplayStateDim, DisplayEventTimeout, NULL);
    }
    return false;
}

bool DisplayOn::timeoutInternal()
{
    uint32_t now = Time::curTimeMs();
    g_debug ("%s: now=%i last=%i diff=%i", __PRETTY_FUNCTION__, now, lastEvent(), now - lastEvent());

    // check for brick mode
    if (SystemService::instance()->brickMode() || 
	    !SystemUiController::instance()->bootFinished() || 
	    WindowServer::instance()->progressRunning())
	return false;

    if (isDNAST())
	return false;

    if (now <  (unsigned int) lockedOffTimeout() + lastEvent())
    {
	g_debug ("%s: restart the dim timer for %u ms", __PRETTY_FUNCTION__, dimTimeout() + lastEvent() - now);
	m_timerInternal->start (lockedOffTimeout() + lastEvent() - now);
    }
    else
    {
#if (defined(TARGET_DESKTOP) || defined (TARGET_EMULATOR))
    g_warning("%s: not turning off display on emulator or desktop", __PRETTY_FUNCTION__);
    changeDisplayState (DisplayStateOnLocked, DisplayEventTimeout, NULL);
#else
	g_message ("%s: change to dim in on", __PRETTY_FUNCTION__);
	if (isOnCall())
	    changeDisplayState (DisplayStateOffOnCall, DisplayEventTimeout, NULL);
	else 
	    changeDisplayState (DisplayStateOff, DisplayEventTimeout, NULL);
#endif
    }
    return false;
}

void DisplayOn::handleEvent (DisplayEvent displayEvent, sptr<Event> event) 
{
    if (displayEvent != DisplayEventAlsChange && displayEvent != DisplayEventUpdateBrightness)
	startUserInactivityTimer();

    switch (displayEvent) {
	case DisplayEventPowerKeyPress:
	    if (!isBacklightOn()) {
		g_warning ("backlight not on yet, debouncing power key");
		break;
	    }
	    g_debug ("%s: power key down", __PRETTY_FUNCTION__);
            if (isOnCall())
                changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
            else if (isOnPuck()) 
                changeDisplayState (DisplayStateDockMode, displayEvent, event);
            else
                changeDisplayState (DisplayStateOff, displayEvent, event);

	    break;

	case DisplayEventPowerKeyHold:
	    break;

	case DisplayEventOnPuck:
	    g_debug ("%s: on puck, moving to onpuck", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    break;

	case DisplayEventOffPuck:
	    g_warning ("%s: off puck received - invalid event", __PRETTY_FUNCTION__);
	    break;

        case DisplayEventUsbIn:
        case DisplayEventUsbOut:
        case DisplayEventIncomingCall:
        case DisplayEventIncomingCallDone:
	case DisplayEventOnCall:
	case DisplayEventOffCall:
	    break;

        case DisplayEventSliderOpen:
	    g_debug ("%s: slider open, updating last event and turning on keypad", __PRETTY_FUNCTION__);
            displayOn(false);  // turning on keypad
	    break;

        case DisplayEventSliderClose:
	    break;

	case DisplayEventAlsChange:
            g_debug ("%s: received Als update, update brightness", __PRETTY_FUNCTION__);
            displayOn(true);
            break;

	case DisplayEventProximityOn:
	    g_debug ("%s: proximity on event, moving to OffOnCall", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
	    break;

	case DisplayEventApiDim:
	    g_debug ("%s: public api dim called, going to dim", __PRETTY_FUNCTION__);
            changeDisplayState (DisplayStateDim, displayEvent, event);
	    break;

	case DisplayEventApiOff:
            if (isOnCall()) {
                g_debug ("%s: public api off called, going to OffOnCall", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
            }
            else if (isOnPuck()) {
                g_debug ("%s: public api off called, going to dockmode", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateDockMode, displayEvent, event);
            }
            else {
                g_debug ("%s: public api off called, going to off", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateOff, displayEvent, event);
            }
	    break;

	case DisplayEventUserActivity:
	case DisplayEventUserActivityExternalInput:
            break;
        case DisplayEventUpdateBrightness:
            g_debug ("%s: received update brightness event", __PRETTY_FUNCTION__);
            displayOn(false);
            break;
        case DisplayEventLockScreen:
	    if (isOnPuck() && !isOnCall()) {
		    g_debug ("%s: received lock event, going to OnLocked", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateDockMode, displayEvent, event);
	    }
	    else {
		    g_debug ("%s: received lock event, going to Dockmode", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateOnLocked, displayEvent, event);
	    }
            break;
        case DisplayEventUnlockScreen:
            break;
	case DisplayEventPowerdSuspend:
	    g_warning ("%s: got powerd suspend, invalid event in current state", __PRETTY_FUNCTION__);
	    break;
	case DisplayEventPowerdResume:
	    break;
	case DisplayEventApiDock:
	    if (!isOnCall()) {
		    g_debug ("%s: received dock event, going to dock mode", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateDockMode, displayEvent, event);
	    }
	    break;
	case DisplayEventApiUndock:
	    break;
	case DisplayEventHomeKeyPress:
	    break;
	default:
	    break;
    }

}

void DisplayOn::leave (DisplayState newState, DisplayEvent displayEvent, sptr<Event> event)
{
    g_debug ("%s:", __PRETTY_FUNCTION__);
    stopInactivityTimer();
}

bool DisplayOn::updateBrightness (int alsRegion)
{
    displayOn(true);
    return true;
}

// ---------------------- DisplayOnLocked ----------------------------------------
DisplayOnLocked::DisplayOnLocked()
    : m_timer (new Timer<DisplayOnLocked>(HostBase::instance()->masterTimer(), this, &DisplayOnLocked::timeout))
{
}

void DisplayOnLocked::enter (DisplayState state, DisplayEvent displayEvent, sptr<Event> event)
{
    g_message ("%s: entering state", __PRETTY_FUNCTION__);
    g_debug ("Emitting DISPLAY_SIGNAL_ON");
    enablePainting();
    updateLockState (DisplayLockLocked, displayEvent);
    emitDisplayStateChange (DISPLAY_SIGNAL_ON);
    orientationSensorOn();
    displayOn(false);
    startInactivityTimer();
}

void DisplayOnLocked::startInactivityTimer()
{
    g_debug ("%s: ", __PRETTY_FUNCTION__);
    if (m_timer->running())
        m_timer->stop();

    if (isDNAST()) {
	g_warning ("not starting timer due to DNAST enabled");
	return;
    }
    g_debug ("%s: %d ms", __PRETTY_FUNCTION__, lockedOffTimeout());

    m_timer->start (lockedOffTimeout());
}

void DisplayOnLocked::stopInactivityTimer()
{
    g_debug ("%s: ", __PRETTY_FUNCTION__);
    if (m_timer->running())
        m_timer->stop();
}

bool DisplayOnLocked::timeout()
{
    uint32_t now = Time::curTimeMs();
    g_debug ("%s: now=%i last=%i diff=%i", __FUNCTION__, now, lastEvent(), now - lastEvent());

    // check for brick mode
    if (SystemService::instance()->brickMode() || 
	    !SystemUiController::instance()->bootFinished() || 
	    WindowServer::instance()->progressRunning())
	return false;


    if (dimTimeout() == 0 && offTimeout() == 0)
	return false;

    if (isDNAST())
	return false;

    if (now <  (unsigned int) lockedOffTimeout() + lastEvent())
    {
	g_debug ("%s: restart the dim timer for %u ms", __PRETTY_FUNCTION__, lockedOffTimeout() + lastEvent() - now);
	m_timer->start (lockedOffTimeout() + lastEvent() - now);
    }
    else
    {
#if (defined(TARGET_DESKTOP) || defined (TARGET_EMULATOR))
    g_warning ("%s: not turning off display on emulator or desktop", __PRETTY_FUNCTION__);
#else
	g_message ("%s: going to off from onlocked state", __FUNCTION__);
	if (isOnCall())
	    changeDisplayState (DisplayStateOffOnCall, DisplayEventTimeout, NULL);
	else
	    changeDisplayState (DisplayStateOff, DisplayEventTimeout, NULL);
#endif
    }
    return false;
}

void DisplayOnLocked::handleEvent (DisplayEvent displayEvent, sptr<Event> event) 
{
    switch (displayEvent) {
	case DisplayEventPowerKeyPress:
	    if (!isBacklightOn()) {
		g_warning ("backlight not on yet, debouncing power key");
		break;
	    }

            g_debug ("%s: power key up, moving to off", __PRETTY_FUNCTION__);
            // if power key timer has not fired, turn display off
            if (isOnCall())
                changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
            else 
                changeDisplayState (DisplayStateOff, displayEvent, event);
            break;

	case DisplayEventPowerKeyHold:
	    break;

	case DisplayEventOnPuck:
	    g_debug ("%s: on puck, moving to OnPuck", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    break;

	case DisplayEventOffPuck:
	    g_warning ("%s: off puck received - invalid event", __PRETTY_FUNCTION__);
	    break;

        case DisplayEventUsbIn:
	    g_debug ("%s: usb in, restarting activity timer", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOn, displayEvent, event);
	    break;

        case DisplayEventUsbOut:
	    break;

        case DisplayEventIncomingCall:
            break;

        case DisplayEventIncomingCallDone:
            break;

	case DisplayEventOnCall:
	    break;

	case DisplayEventOffCall:
	    break;

        case DisplayEventSliderOpen:
	    g_debug ("%s: slider open, going to on", __PRETTY_FUNCTION__);
            changeDisplayState (DisplayStateOn, displayEvent, event);
	    break;

        case DisplayEventSliderClose:
	    break;

	case DisplayEventAlsChange:
            g_debug ("%s: received ALS change, update brightness", __PRETTY_FUNCTION__);
            displayOn(true);
	    break;

	case DisplayEventProximityOn:
	    g_debug ("%s: proximity on event, changing to OffOnCall", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
	    break;

	case DisplayEventProximityOff:
	    g_warning ("%s: proximity off event - invalid event", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventApiOn:
	    break;

	case DisplayEventApiOff:
	    g_debug ("%s: public api off called, going to Off / OffOnCall", __PRETTY_FUNCTION__);
	    if (isOnCall())
		changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
	    else
                changeDisplayState (DisplayStateOff, displayEvent, event);
	    break;

	case DisplayEventUserActivity:
	    break;
	case DisplayEventUserActivityExternalInput:
		g_debug ("%s: external input user activity, moving to on", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOn, displayEvent, event);
		break;

        case DisplayEventUpdateBrightness:
            displayOn(true);
            break;

        case DisplayEventLockScreen:
            break;
	case DisplayEventApiDock:
	    if (!isOnCall()) {
		    g_debug ("%s: received dock event, going to dock mode", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateDockMode, displayEvent, event);
	    }
	    break;
	case DisplayEventApiUndock:
	    break;

        case DisplayEventUnlockScreen:
            g_debug ("%s: received unlock event, going to on", __PRETTY_FUNCTION__);
            changeDisplayState (DisplayStateOn, displayEvent, event);
            break;

	case DisplayEventPowerdSuspend:
	    g_warning ("%s: got powerd suspend, invalid event in current state", __PRETTY_FUNCTION__);
	    break;
	case DisplayEventPowerdResume:
	    break;
	case DisplayEventHomeKeyPress:
	    break;
	default:
	    break;
    }

}

void DisplayOnLocked::leave (DisplayState newState, DisplayEvent displayEvent, sptr<Event> event)
{
    g_debug ("%s:", __PRETTY_FUNCTION__);
    stopInactivityTimer();

    if (displayEvent == DisplayEventPowerKeyPress || displayEvent == DisplayEventUnlockScreen)
    {
        g_warning ("%s: display event is %d (power key press / unlock screen), clearing throbber requests", __PRETTY_FUNCTION__, displayEvent);
        CoreNaviManager::instance()->clearAllStandbyRequests();
    }

}


bool DisplayOnLocked::updateBrightness (int alsRegion)
{
    displayOn(true);
    return true;
}

// ---------------------- DisplayDim ----------------------------------------

DisplayDim::DisplayDim()
    : m_timer (new Timer<DisplayDim>(HostBase::instance()->masterTimer(), this, &DisplayDim::timeout))
{
}

void DisplayDim::startInactivityTimer()
{
    g_debug ("%s: ", __PRETTY_FUNCTION__);
    if (m_timer->running())
        m_timer->stop();

    g_debug ("%s: %d ms", __PRETTY_FUNCTION__, offTimeout());
    m_timer->start (offTimeout());
}

void DisplayDim::stopInactivityTimer()
{
    g_debug ("%s: ", __PRETTY_FUNCTION__);
    if (m_timer->running())
        m_timer->stop();
}

bool DisplayDim::timeout()
{
    uint32_t now = Time::curTimeMs();
    g_debug ("%s: now=%i last=%i diff=%i", __FUNCTION__, now, lastEvent(), now - lastEvent());

    // check for brick mode
    if (SystemService::instance()->brickMode() || 
	    !SystemUiController::instance()->bootFinished() || 
	    WindowServer::instance()->progressRunning())
	return false;


    if (dimTimeout() == 0 && offTimeout() == 0)
	return false;

    if (isDNAST())
	return false;

    if (now <  (unsigned int) offTimeout() + lastEvent())
    {
	g_debug ("%s: restart the dim timer for %u ms", __PRETTY_FUNCTION__, dimTimeout() + lastEvent() - now);
	m_timer->start (offTimeout() + lastEvent() - now);
    }
    else {
	g_message ("%s: calling off() in on or onpuck state", __PRETTY_FUNCTION__);
	if (isOnCall())
	    changeDisplayState (DisplayStateOffOnCall, DisplayEventTimeout, NULL);
	else if (isOnPuck())
	    changeDisplayState (DisplayStateDockMode, DisplayEventTimeout, NULL);
	else
	    changeDisplayState (DisplayStateOff, DisplayEventTimeout, NULL);
    }
    return false;
}

void DisplayDim::enter (DisplayState state, DisplayEvent displayEvent, sptr<Event> event)
{
    g_message ("%s: entering state", __PRETTY_FUNCTION__);
    g_debug ("Emitting DISPLAY_SIGNAL_DIM");
    emitDisplayStateChange (DISPLAY_SIGNAL_DIM);
    if (Settings::LunaSettings()->turnOffAccelWhenDimmed)
        orientationSensorOff();
    else
        orientationSensorOn();
    displayDim();
    startInactivityTimer();
}

void DisplayDim::handleEvent (DisplayEvent displayEvent, sptr<Event> event) 
{
    if (isOnCall() && isOnPuck())
        g_warning ("%s: in dim when on call and on puck, bug!", __PRETTY_FUNCTION__);

    switch (displayEvent) {
	case DisplayEventPowerKeyPress:
	    g_debug ("%s: power key press, moving to off/dockmode", __PRETTY_FUNCTION__);

            if (isOnCall()) {
                changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
            }
            else  {
                if (isOnPuck())
                    changeDisplayState (DisplayStateDockMode, displayEvent, event);
                else
                    changeDisplayState (DisplayStateOff, displayEvent, event);
            }
	    break;

	case DisplayEventPowerKeyHold:
	    g_debug ("%s: power key hold, moving to on/onpuck", __PRETTY_FUNCTION__);
            if (isOnPuck())
                changeDisplayState (DisplayStateOnPuck, displayEvent, event);
            else
                changeDisplayState (DisplayStateOn, displayEvent, event);
	    break;

	case DisplayEventOnPuck:
	    g_debug ("%s: on puck, moving to on puck", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    break;

	case DisplayEventOffPuck:
	    g_debug ("%s: lifted off puck, moving to on", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOn, displayEvent, event);
	    break;

        case DisplayEventUsbIn:
	    g_debug ("%s: usb in, going to on / onpuck", __PRETTY_FUNCTION__);
            if (isOnPuck())
                changeDisplayState (DisplayStateOnPuck, displayEvent, event);
            else 
                changeDisplayState (DisplayStateOn, displayEvent, event);

	    break;

        case DisplayEventUsbOut:
	    break;

        case DisplayEventIncomingCall:
            g_debug ("%s: incoming call when dim, moving to on/on locked", __PRETTY_FUNCTION__);
            if (isOnPuck())
                changeDisplayState (DisplayStateOnPuck, displayEvent, event);
            else 
                changeDisplayState (DisplayStateOn, displayEvent, event);
            break;

        case DisplayEventIncomingCallDone:
            break;

	case DisplayEventOnCall:
	    break;

	case DisplayEventOffCall:
	    break;

        case DisplayEventSliderOpen:
	    break;

        case DisplayEventSliderClose:
	    g_debug ("%s: slider close, going to DockMode/OffOnCall/Off", __PRETTY_FUNCTION__);
            if (isOnCall())
                changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
            else if (isOnPuck())
                changeDisplayState (DisplayStateDockMode, displayEvent, event);
            else
                changeDisplayState (DisplayStateOff, displayEvent, event);
	    break;

	case DisplayEventAlsChange:
	    break;

	case DisplayEventProximityOn:
            if (isOnCall()) {
                g_debug ("%s: proximity on event, moving to off on call", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
            }
	    break;

	case DisplayEventProximityOff:
	    g_warning ("%s: proximity off event - invalid event", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventApiOn:
            if (isOnPuck()) {
                g_debug ("%s: public api on called, going to onpuck", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateOnPuck, displayEvent, event);
            }
            else {
                g_debug ("%s: public api on called, going to on", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateOn, displayEvent, event);
            }
	    break;

	case DisplayEventApiDim:
	    break;

	case DisplayEventApiOff:
            if (isOnCall()) {
                g_debug ("%s: public api off called while on call, going to OffOnCall", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
            }
            else if (isOnPuck()) {
                g_debug ("%s: public api off called, going to dockmode", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateDockMode, displayEvent, event);
            }
            else {
                g_debug ("%s: public api off called, going to off", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateOff, displayEvent, event);
            }
	    break;

	case DisplayEventHomeKeyPress:
	case DisplayEventUserActivity:
	case DisplayEventUserActivityExternalInput:
            if (isOnPuck()) {
                g_debug ("%s: received key/pen/gesture event, going to onpuck", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateOnPuck, displayEvent, event);
            }
            else {
                g_debug ("%s: received key/pen/gesture event, going to on", __PRETTY_FUNCTION__);
                changeDisplayState (DisplayStateOn, displayEvent, event);
            }
            break;

        case DisplayEventLockScreen:
            g_debug ("%s: lock event received, going to on locked", __PRETTY_FUNCTION__);
            changeDisplayState (DisplayStateOnLocked, displayEvent, event);
            break;
        case DisplayEventUnlockScreen:
            break;
	case DisplayEventPowerdSuspend:
	    g_warning ("%s: got powerd suspend, invalid event in current state", __PRETTY_FUNCTION__);
	    break;
	case DisplayEventPowerdResume:
	    break;
	case DisplayEventApiDock:
	    if (!isOnCall()) {
		    g_debug ("%s: received dock event, going to dock mode", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateDockMode, displayEvent, event);
	    }
	    break;
	case DisplayEventApiUndock:
	    break;
	default:
	    break;
    }

}

void DisplayDim::leave (DisplayState newState, DisplayEvent displayEvent, sptr<Event> event)
{
    g_debug ("%s:", __PRETTY_FUNCTION__);
    stopInactivityTimer();
}

// ---------------------- DisplayOnPuck ----------------------------------------

DisplayOnPuck::DisplayOnPuck()
    : m_timer (new Timer<DisplayOnPuck>(HostBase::instance()->masterTimer(), this, &DisplayOnPuck::timeout))
{

}

void DisplayOnPuck::enter (DisplayState state, DisplayEvent displayEvent, sptr<Event> event)
{
    g_message ("%s: entering state", __PRETTY_FUNCTION__);
    displayOn(false);
    updateLockState (DisplayLockUnlocked, displayEvent); 
    g_debug ("Emitting DISPLAY_SIGNAL_ON");
    emitDisplayStateChange (DISPLAY_SIGNAL_ON);
    enablePainting();
    orientationSensorOn();

    if (!isOnCall())
	    startInactivityTimer();
}

void DisplayOnPuck::startInactivityTimer()
{
    g_debug ("%s: ", __PRETTY_FUNCTION__);
    if (m_timer->running())
        m_timer->stop();

    if (isOnCall() || isDNAST()) {
	g_warning ("not starting timer due to DNAST enabled or onCall");
	return;
    }

    g_debug ("%s: %d ms", __PRETTY_FUNCTION__, dimTimeout() + offTimeout());
    m_timer->start (dimTimeout() + offTimeout());
}

void DisplayOnPuck::stopInactivityTimer()
{
    g_debug ("%s: ", __PRETTY_FUNCTION__);
    if (m_timer->running())
        m_timer->stop();
}

bool DisplayOnPuck::timeout()
{
    uint32_t now = Time::curTimeMs();
    g_debug ("%s: now=%i last=%i diff=%i", __FUNCTION__, now, lastEvent(), now - lastEvent());

    // check for brick mode
    if (SystemService::instance()->brickMode() || 
	    !SystemUiController::instance()->bootFinished() || 
	    WindowServer::instance()->progressRunning())
	return false;


    if (dimTimeout() == 0 && offTimeout() == 0)
	return false;

    if (isDNAST())
	return false;

    if (isOnCall())
	    return false;
    if (now <  (unsigned int) (dimTimeout() + offTimeout()) + lastEvent())
    {
	g_debug ("%s: restart the dim timer for %u ms", __PRETTY_FUNCTION__, dimTimeout() + lastEvent() - now);
	m_timer->start ((dimTimeout() + offTimeout()) + lastEvent() - now);
    }
    else {
	g_message ("%s: going to dockmode", __PRETTY_FUNCTION__);
	changeDisplayState (DisplayStateDockMode, DisplayEventTimeout, NULL);
    }
    return false;
}


void DisplayOnPuck::handleEvent (DisplayEvent displayEvent, sptr<Event> event) 
{
    switch (displayEvent) {
        case DisplayEventPowerKeyPress:
		if (isOnCall()) {
			g_debug ("%s: power key press received, going to off", __PRETTY_FUNCTION__);
			changeDisplayState (DisplayStateOff, displayEvent, event);
		}
		else {
			g_debug ("%s: power key press received, going to dockmode", __PRETTY_FUNCTION__);
			changeDisplayState (DisplayStateDockMode, displayEvent, event);
		}
	    break;
        case DisplayEventPowerKeyHold:
            break;
	case DisplayEventOnPuck:
	    g_warning ("%s: on puck, invalid - bug?", __PRETTY_FUNCTION__);
	    break;
	case DisplayEventOffPuck:
	    g_debug ("%s: off puck received, going to on", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOn, displayEvent, event);
	    break;
        case DisplayEventUsbIn:
	    break;
        case DisplayEventUsbOut:
            break;
        case DisplayEventIncomingCall:
	    break;
        case DisplayEventIncomingCallDone:
	    break;
	case DisplayEventOnCall:
            stopInactivityTimer();
	    break;
	case DisplayEventOffCall:
            if (!isOnCall())
                startInactivityTimer();
	    break;
        case DisplayEventSliderOpen:
	    break;
        case DisplayEventSliderClose:
	    break;
	case DisplayEventAlsChange:
            g_debug ("%s: received ALS change, update brightness", __PRETTY_FUNCTION__);
            displayOn(true);
	    break;
	case DisplayEventProximityOn:
	    break;
	case DisplayEventProximityOff:
	    break;
	case DisplayEventApiOn:
	    break;
	case DisplayEventApiDim:
	    break;
	case DisplayEventApiOff:
	    if (isOnCall()) {
	        g_debug ("%s: public api off called, going to off on call", __PRETTY_FUNCTION__);
	        changeDisplayState (DisplayStateOffOnCall, displayEvent, event);
	    }
	    else {
	        g_debug ("%s: public api off called, going to dock mode", __PRETTY_FUNCTION__);
	        changeDisplayState (DisplayStateDockMode, displayEvent, event);
	    }
	    break;
	case DisplayEventApiDock:
	    if (!isOnCall()) {
		    g_debug ("%s: received dock event, going to dock mode", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateDockMode, displayEvent, event);
	    }
	    break;
	case DisplayEventApiUndock:
	    break;
	case DisplayEventUserActivity:
	case DisplayEventUserActivityExternalInput:
	    break;
        case DisplayEventUpdateBrightness:
            displayOn(true);
            break;
        case DisplayEventLockScreen:
	    if (!isOnCall()) {
		    g_debug ("%s: public api lock called, going to dock mode", __PRETTY_FUNCTION__);
		    changeDisplayState (DisplayStateDockMode, displayEvent, event);
	    }
            break;
        case DisplayEventUnlockScreen:
            break;
	case DisplayEventPowerdSuspend:
	    g_warning ("%s: got powerd suspend, invalid event in current state", __PRETTY_FUNCTION__);
	    break;
	case DisplayEventPowerdResume:
	    break;
	case DisplayEventHomeKeyPress:
	    break;
	default:
	    break;
    }

}

void DisplayOnPuck::leave (DisplayState newState, DisplayEvent displayEvent, sptr<Event> event)
{
    g_debug ("%s:", __PRETTY_FUNCTION__);
    stopInactivityTimer();
}

bool DisplayOnPuck::updateBrightness (int alsRegion)
{
    displayOn(true);
    return true;
}

// ---------------------- DisplayDockMode -------------------------------------

DisplayDockMode::DisplayDockMode()
    : m_timerExit (new Timer<DisplayDockMode>(HostBase::instance()->masterTimer(), this, &DisplayDockMode::timeoutExit))
{

}

bool DisplayDockMode::timeoutExit()
{
	if (!isOnPuck()) {
		g_warning ("%s: Exiting dock mode", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOn, DisplayEventOffPuck, NULL);
	}
	return false;
}

void DisplayDockMode::enter (DisplayState state, DisplayEvent displayEvent, sptr<Event> event)
{
    g_message ("%s: entering state", __PRETTY_FUNCTION__);
    orientationSensorOn();

    g_debug ("Emitting DISPLAY_SIGNAL_DOCK");
    emitDisplayStateChange (DISPLAY_SIGNAL_DOCK);

    // update lock window before turning on the display
    if (state == DisplayStateOff)
	    updateLockState (DisplayLockDockMode, displayEvent);

    if (isOnCall()) {
	    g_warning ("%s: should never be in DisplayDockMode when on a call", __PRETTY_FUNCTION__);
    }

    displayOn(false);

    // if display was on, update lock window that display is in nightstand mode after dimming the display
    if (state != DisplayStateOff)
        updateLockState (DisplayLockDockMode, displayEvent);

    enablePainting();

    startInactivityTimer();
}

void DisplayDockMode::handleEvent (DisplayEvent displayEvent, sptr<Event> event) 
{
    switch (displayEvent) {
	case DisplayEventPowerKeyPress:
		g_debug ("%s: power key press, moving to off", __PRETTY_FUNCTION__);
		changeDisplayState (DisplayStateOff, displayEvent, event);
	    break;

	case DisplayEventPowerKeyHold:
	    break;

	case DisplayEventOnPuck:
	    // g_message ("%s: on puck - canceling exit timer", __PRETTY_FUNCTION__);
	    // m_timerExit->stop(); // Remove landscape from dock mode
	    break;

	case DisplayEventOffPuck:
	    g_message ("%s: off puck received - exiting dock mode, going to on", __PRETTY_FUNCTION__);
	    changeDisplayState (DisplayStateOn, DisplayEventOffPuck, NULL);
	    break;

        case DisplayEventUsbIn:
	    break;
        case DisplayEventIncomingCall:
            break;
        case DisplayEventIncomingCallDone:
            break;

	case DisplayEventOnCall:
	    g_debug ("%s: on call, moving to onpuck", __PRETTY_FUNCTION__);
	    if (isOnPuck())
		changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    else 
		changeDisplayState (DisplayStateOn, displayEvent, event);
	    break;

	case DisplayEventOffCall:
	    g_warning ("%s: off call - invalid event bug?", __PRETTY_FUNCTION__);
	    break;

        case DisplayEventSliderOpen:
	    break;

        case DisplayEventSliderClose:
	    break;

	case DisplayEventProximityOn:
	    g_warning ("%s: proximity on event received - invalid event", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventProximityOff:
	    g_warning ("%s: proximity off event received - invalid event", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventApiOn:
	    g_debug ("%s: public api on called, move to onpuck", __PRETTY_FUNCTION__);
	    if (isOnPuck())
		    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    else 
		    changeDisplayState (DisplayStateOn, displayEvent, event);
	    break;
	case DisplayEventApiOff:
	    break;
	case DisplayEventApiDock:
	    break;
	case DisplayEventApiUndock:
	    g_debug ("%s: api undock called, going to onpuck / on", __PRETTY_FUNCTION__);
	    if (isOnPuck())
		changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    else 
		changeDisplayState (DisplayStateOn, displayEvent, event);
	    break;
	case DisplayEventUserActivity:
	case DisplayEventUserActivityExternalInput:
	    break;
        case DisplayEventUpdateBrightness:
	    displayOn(true);
            break;
        case DisplayEventLockScreen:
            break;
        case DisplayEventUnlockScreen:
            break;
	case DisplayEventPowerdSuspend:
	    g_warning ("%s: got powerd suspend, invalid event in current state", __PRETTY_FUNCTION__);
	    break;
	case DisplayEventPowerdResume:
	    break;
	case DisplayEventHomeKeyPress:
	    g_debug ("%s: home key press, move to onpuck", __PRETTY_FUNCTION__);
	    if (isOnPuck())
		    changeDisplayState (DisplayStateOnPuck, displayEvent, event);
	    else
		    changeDisplayState (DisplayStateOn, displayEvent, event);

	    break;
	default:
	    break;
    }

}

void DisplayDockMode::leave (DisplayState newState, DisplayEvent displayEvent, sptr<Event> event)
{
    g_debug ("%s:", __PRETTY_FUNCTION__);
}


// ---------------------- DisplayOffSuspended -------------------------------------

DisplayOffSuspended::DisplayOffSuspended()
	: m_restoreState (DisplayStateOff)
	, m_restoreDisplayEvent (DisplayEventPowerdResume)
	, m_restoreEvent (NULL)
{
}

void DisplayOffSuspended::enter (DisplayState state, DisplayEvent displayEvent, sptr<Event> event)
{
	g_message ("%s: entering state", __PRETTY_FUNCTION__);
    orientationSensorOff();
	displayOff();
	disablePainting();

	if (state != DisplayStateOff && state != DisplayStateOffOnCall)
		g_warning ("%s: entering from state %d, not a valid state to transition from!", __PRETTY_FUNCTION__, state);

	// We do not emit signals or update the locked state in this state, since this is an internal-only state
	// and the system should does not need to be aware of this state, or change behavior based on this.
	m_restoreState = state;
	m_restoreDisplayEvent = DisplayEventPowerdResume;
	m_restoreEvent = NULL;
}

void DisplayOffSuspended::handleEvent (DisplayEvent displayEvent, sptr<Event> event) 
{
    switch (displayEvent) {
	case DisplayEventPowerKeyPress:
	    if (isOnPuck()) {
		if (isDisplayUnlocked() || isOnCall()) {
		    g_message ("%s: power key press while display %s (usb %s slider %s disable locking %s) and phonecall %s, moving to OnPuck", __PRETTY_FUNCTION__,
			    (isDisplayUnlocked()) ? "unlocked" : "locked", 
			    (isUSBCharging()) ? "connected" : "disconnected",
			    (isSliderOpen()) ? "open" : "closed",
			    (Settings::LunaSettings()->disableLocking) ? "set" : "unset",
			    (isOnCall()) ? "on" : "off");
		    m_restoreState = DisplayStateOnPuck;
		    m_restoreDisplayEvent = displayEvent;
		    m_restoreEvent = event;
		}
		else {
		    g_debug ("%s: power key press when on puck, move to dock mode", __PRETTY_FUNCTION__);
		    m_restoreState = DisplayStateDockMode;
		    m_restoreDisplayEvent = displayEvent;
		    m_restoreEvent = event;
		}
	    }
	    else if (isDisplayUnlocked() || isOnCall()) {
		g_message ("%s: power key press while display %s (usb %s slider %s disable locking %s) and phonecall %s, moving to on", __PRETTY_FUNCTION__,
			(isDisplayUnlocked()) ? "unlocked" : "locked", 
			(isUSBCharging()) ? "connected" : "disconnected",
			(isSliderOpen()) ? "open" : "closed",
			(Settings::LunaSettings()->disableLocking) ? "set" : "unset",
			(isOnCall()) ? "on" : "off");
		m_restoreState = DisplayStateOn;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else  {
		g_debug ("%s: power key press, moving to OnLocked", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnLocked;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    break;

	case DisplayEventPowerKeyHold:
	    if (isOnPuck()) {
		g_debug ("%s: power key hold on puck, moving to on", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnPuck;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else if (isDisplayUnlocked()) {
		g_debug ("%s: power key hold, moving to on", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOn;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else  {
		g_debug ("%s: power key hold, moving to OnLocked", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnLocked;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    break;

	case DisplayEventOnPuck:
	    if (isOnCall() || isDisplayUnlocked()) {
		g_debug ("%s: on puck on a call, moving to onpuck", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnPuck;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else {
		g_debug ("%s: on puck, moving to dock mode", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateDockMode;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    break;

	case DisplayEventOffPuck:
	    g_debug ("%s: off puck received in Off state, staying off", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventUsbIn:
	    if (isOnPuck()) {
		g_debug ("%s: usb in when on puck, move to on puck", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnPuck;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else {
		g_debug ("%s: usb in, moving to on", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOn;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    break;

	case DisplayEventUsbOut:
	    break;

	case DisplayEventIncomingCall:
	    if (isOnPuck()) {
		g_debug ("%s: incoming call when on puck, move to OnPuck", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnPuck;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else {
		g_debug ("%s: incoming call, moving to on / onlocked", __PRETTY_FUNCTION__);
		if (isDisplayUnlocked()) {
		    m_restoreState = DisplayStateOn;
		    m_restoreDisplayEvent = displayEvent;
		    m_restoreEvent = event;
		}
		else  {
		    m_restoreState = DisplayStateOnLocked;
		    m_restoreDisplayEvent = displayEvent;
		    m_restoreEvent = event;
		}
	    }

	    break;

	case DisplayEventIncomingCallDone:
	    g_warning ("%s: incoming call done, should never happen", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventOnCall:
	    if (isOnPuck()) {
		g_debug ("%s: on call on puck, move to OnPuck", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnPuck;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else if (isDisplayUnlocked()) {
		g_debug ("%s: on call, moving to On", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOn;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else {
		g_debug ("%s: on call, moving to OnLocked", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnLocked;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    break;

	case DisplayEventOffCall:
	    g_warning ("%s: off call - invalid event, should be OffOnCall", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventSliderOpen:
	    if (isOnPuck()) {
		g_debug ("%s: slider open when on puck, move to OnPuck", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnPuck;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else {
		g_debug ("%s: slider open, moving to on", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOn;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    break;

	case DisplayEventSliderClose:
	    break;

	case DisplayEventAlsChange:
	    break;

	case DisplayEventProximityOn:
	    g_warning ("%s: proximity on event received in off state - invalid event, should be OffOnCall", __PRETTY_FUNCTION__);
	    break;

	case DisplayEventProximityOff:
#if defined (MACHINE_BROADWAY) || defined(MACHINE_MANTARAY)
	    if (isOnPuck()) {
		    g_message ("%s: proximity off event on puck, moving to OnPuck", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnPuck;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else {
		    g_message ("%s: proximity off event, moving to on", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOn;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
#else
	    g_warning ("%s: proximity off event received in off state - invalid event, should be OffOnCall", __PRETTY_FUNCTION__);
#endif
	    break;

	case DisplayEventApiOn:
	    if (isOnPuck()) {
		g_debug ("%s: public api on called when on puck, move to on puck", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnPuck;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else if (isDisplayUnlocked()) {
		g_debug ("%s: public api on called, move to on", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOn;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else {
		g_debug ("%s: public api on called, move to onLocked", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnLocked;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    break;

	case DisplayEventApiDim:
	    break;

	case DisplayEventApiOff:
	    break;

	case DisplayEventUserActivity:
	    if (isOnPuck()) {
		g_debug ("%s: user activity when on puck, move to on puck", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnPuck;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else if (isDisplayUnlocked()) {
		g_debug ("%s: received key down for key, moving to on", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOn;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    break;

	case DisplayEventUserActivityExternalInput:
	    if (isOnPuck()) {
			g_debug ("%s: external input user activity when on puck, move to on puck", __PRETTY_FUNCTION__);
			m_restoreState = DisplayStateOnPuck;
			m_restoreDisplayEvent = displayEvent;
			m_restoreEvent = event;
	    }
	    else  {
			g_debug ("%s: external input user activity, moving to on", __PRETTY_FUNCTION__);
			m_restoreState = DisplayStateOn;
			m_restoreDisplayEvent = displayEvent;
			m_restoreEvent = event;
	    }
	    break;		
		
	    // reacted to the event, but the key should be handled elsewhere
	case DisplayEventUpdateBrightness:
	    break;
	case DisplayEventLockScreen:
	    break;
	case DisplayEventUnlockScreen:
	    break;
	case DisplayEventPowerdSuspend:
	    break;
	case DisplayEventPowerdResume:
	    g_debug ("%s: On resume, restoring to state %d with event %d",
			    __PRETTY_FUNCTION__, (int)m_restoreState, (int)m_restoreDisplayEvent);
	    changeDisplayState (m_restoreState, m_restoreDisplayEvent, m_restoreEvent);
	    break;
	case DisplayEventHomeKeyPress:
	    if (isOnPuck()) {
		if (isDisplayUnlocked() || isOnCall()) {
		    g_message ("%s: home key press while display %s (usb %s slider %s disable locking %s) and phonecall %s, moving to OnPuck", __PRETTY_FUNCTION__,
			    (isDisplayUnlocked()) ? "unlocked" : "locked", 
			    (isUSBCharging()) ? "connected" : "disconnected",
			    (isSliderOpen()) ? "open" : "closed",
			    (Settings::LunaSettings()->disableLocking) ? "set" : "unset",
			    (isOnCall()) ? "on" : "off");
		    m_restoreState = DisplayStateOnPuck;
		    m_restoreDisplayEvent = displayEvent;
		    m_restoreEvent = event;
		}
		else {
		    g_debug ("%s: home key press when on puck, move to dock mode", __PRETTY_FUNCTION__);
		    m_restoreState = DisplayStateDockMode;
		    m_restoreDisplayEvent = displayEvent;
		    m_restoreEvent = event;
		}
	    }
	    else if (isDisplayUnlocked() || isOnCall()) {
		g_message ("%s: home key press while display %s (usb %s slider %s disable locking %s) and phonecall %s, moving to on", __PRETTY_FUNCTION__,
			(isDisplayUnlocked()) ? "unlocked" : "locked", 
			(isUSBCharging()) ? "connected" : "disconnected",
			(isSliderOpen()) ? "open" : "closed",
			(Settings::LunaSettings()->disableLocking) ? "set" : "unset",
			(isOnCall()) ? "on" : "off");
		m_restoreState = DisplayStateOn;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    else  {
		g_debug ("%s: home key press, moving to OnLocked", __PRETTY_FUNCTION__);
		m_restoreState = DisplayStateOnLocked;
		m_restoreDisplayEvent = displayEvent;
		m_restoreEvent = event;
	    }
	    break;

	default:
	    break;
    }

}

void DisplayOffSuspended::leave (DisplayState newState, DisplayEvent displayEvent, sptr<Event> event)
{
    g_debug ("%s:", __PRETTY_FUNCTION__);
}


