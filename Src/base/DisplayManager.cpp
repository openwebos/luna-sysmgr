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

#include "DisplayManager.h"

#include "AmbientLightSensor.h"
#include "Common.h"
#include "CoreNaviManager.h"
#include "CustomEvents.h"
#include "DeviceInfo.h"
#include "DisplayStates.h"
#include "HostBase.h"
#include "IMEController.h"
#include "JSONUtils.h"
#include "Preferences.h"
#include "Settings.h"
#include "SystemService.h"
#include "SystemUiController.h"
#include "Time.h"
#include "WindowServer.h"

#include <nyx/nyx_client.h>
#include "NyxSensorConnector.h"

#include <QApplication>

#if defined(TARGET_DEVICE) && defined(HAVE_OPENGL)
#include <QGLContext>
#endif

#include <cjson/json.h>

#include <glib.h>
#include <lunaservice.h>

#if defined(HAS_LUNA_PREF)
#include <lunaprefs.h>
#endif

#define URI_POWERD_ACTIVITY_START  "palm://com.palm.power/com/palm/power/activityStart"
#define URI_POWERD_ACTIVITY_END    "palm://com.palm.power/com/palm/power/activityEnd"
#define JSON_POWERD_ACTIVITY_START "{\"id\":\"com.palm.display-lock.timer\",\"duration_ms\":%i}"
#define JSON_POWERD_ACTIVITY_END   "{\"id\":\"com.palm.display-lock.timer\"}"

#define DISPLAY_APPID "com.palm.display"
#define URI_PREFS_GET "palm://com.palm.preferences/appProperties/Get"
#define URI_PREFS_SET "palm://com.palm.preferences/appProperties/Set"

#define URI_SIGNAL_ADDMATCH "palm://com.palm.lunabus/signal/addmatch"
#define URI_POWERD_BATTERY_SIGNAL_REQUEST "palm://com.palm.display/com/palm/power/batteryStatusQuery"
#define URI_CHARGER_SIGNAL_REQUEST "palm://com.palm.display/com/palm/power/chargerStatusQuery"
#define URI_TELEPHONY_CHARGER_SIGNAL_REQUEST "palm://com.palm.telephony/chargeSourceQuery"
#define URI_USBDOCK_SIGNAL_REQUEST "palm://com.palm.display/com/palm/power/USBDockStatus"
#define URI_LBS_GETCURRENTLOC			"palm://com.palm.location/getCurrentPosition"

#define URI_SLIDER_STATUS_REQUEST "palm://com.palm.keys/switches/status"

#define JSON_CHARGER_SIGNAL_ADDMATCH "{\"category\":\"/com/palm/power\",\"method\":\"chargerStatus\"}"
#define JSON_USBDOCK_SIGNAL_ADDMATCH "{\"category\":\"/com/palm/power\",\"method\":\"USBDockStatus\"}"
#define JSON_BATTERY_SIGNAL_ADDMATCH "{\"category\":\"/com/palm/power\",\"method\":\"batteryStatus\"}"
#define JSON_SIGNAL_REQUEST "{}"
#define JSON_LBS_CURRENTLOCATIONINF		"{\"accuracy\":%i,\"responseTime\":%i}"

#define JSON_SLIDER_STATUS_REQUEST "{\"get\":\"slider\"}"
#define URI_AUDIOD_STATUS "palm://com.palm.audio/phone/status"
#define JSON_AUDIOD_SUBSCRIBE "{\"subscribe\":true}"

#define URI_DISPLAY_POWER_KEY_SIGNAL "palm://com.palm.display/com/palm/display/powerKeyPressed"
#define JSON_DISPLAY_POWER_KEY_SIGNAL "{\"showDialog\":true}"

#define DEFAULT_TIMEOUT          120
#define DEFAULT_BRIGHTNESS       40

#define MINIMUM_ON_BRIGHTNESS     1  // do not go below 1 as we can messup the touchpanel.
#define MINIMUM_DIMMED_BRIGHTNESS 1  //

#define DISPLAY_UNDEFINED  -1
#define DISPLAY_OFF         0
#define DISPLAY_DIMMED      1
#define DISPLAY_ON          2

#define DNAST_SUBSCRIPTION_KEY "DNAST"
#define POWER_KEY_BLOCK_SUBSCRIPTION_KEY "PKBSK"
#define PROXIMITY_SUBSCRIPTION_KEY "PESK"

#define SLIDER_TIMEOUT 1500
#define SLIDER_MINTIME 200
#define ALERT_TIMEOUT  6000
#define SLIDER_LOCK_TIMEOUT  2000
#define TOUCHPANEL_DELAY 200
#define DISPLAY_LOCK_TIMEOUT 2000

#define DISPLAY_EVENT_REQUEST     0
#define DISPLAY_EVENT_ON          1
#define DISPLAY_EVENT_DIMMED      2
#define DISPLAY_EVENT_OFF         3
#define DISPLAY_EVENT_TIMEOUTS    4
#define DISPLAY_EVENT_PUSH_DNAST  5
#define DISPLAY_EVENT_POP_DNAST   6
#define DISPLAY_EVENT_ACTIVE      7
#define DISPLAY_EVENT_INACTIVE    8
#define DISPLAY_EVENT_DOCKMODE    9

#define CHARGER_NONE              0
#define CHARGER_USB               1
#define CHARGER_INDUCTIVE         (1 << 1)

#define DISPLAY_EVENT_NONE                           100
#define DISPLAY_EVENT_SLIDER_LOCKED                  101
#define DISPLAY_EVENT_SLIDER_UNLOCKED                102
#define DISPLAY_EVENT_POWER_BUTTON_UP                103
#define DISPLAY_EVENT_POWER_BUTTON_DOWN              104
#define DISPLAY_EVENT_INDUCTIVE_CHARGER_DISCONNECTED 105
#define DISPLAY_EVENT_INDUCTIVE_CHARGER_CONNECTED    106
#define DISPLAY_EVENT_USB_CHARGER_DISCONNECTED       107
#define DISPLAY_EVENT_USB_CHARGER_CONNECTED          108
#define DISPLAY_EVENT_ALS_REGION_CHANGED             109
#define DISPLAY_EVENT_ENTER_EMERGENCY_MODE           110
#define DISPLAY_EVENT_EXIT_EMERGENCY_MODE            111
#define DISPLAY_EVENT_PROXIMITY_ON                   112
#define DISPLAY_EVENT_PROXIMITY_OFF                  113
#define DISPLAY_EVENT_ON_CALL                        114
#define DISPLAY_EVENT_OFF_CALL                       115
#define DISPLAY_EVENT_HOME_BUTTON_UP                 116

#if defined(TARGET_DEVICE)
extern "C" void setEglSwapInterval(int);
#endif

DisplayManager* DisplayManager::m_instance = NULL;

double DisplayManager::s_currentLatitude = 0.0;
double DisplayManager::s_currentLongitude = 0.0;

typedef struct DisplayCallbackCtx
{
    DisplayManager* ctx;
    bool isPublic;
} DisplayCallbackCtx_t;

static DisplayCallbackCtx_t publicCtx;
static DisplayCallbackCtx_t privateCtx;

/*! \page com_palm_display Service API com.palm.display/
 *
 * Public methods:
 * - \ref com_palm_display_status
 */
// when using LSPalmServiceRegisterCategory, the public methods are mirrored
// in the private methods
static LSMethod publicDisplayMethods[] = {
    {"status", DisplayManager::controlStatus},
    {},
};

/*! \page com_palm_display_control Service API com.palm.display/control/
 *
 * Private methods:
 * - \ref com_palm_display_control_set_state
 * - \ref com_palm_display_control_set_property
 * - \ref com_palm_display_control_get_property
 * - \ref com_palm_display_control_status
 */
static LSMethod privateDisplayMethods[] = {
    {"setState", DisplayManager::controlSetState},
    {"setProperty", DisplayManager::controlSetProperty},
    {"getProperty", DisplayManager::controlGetProperty},
    {"status", DisplayManager::controlStatus},
//    {"setCallStatus", DisplayManager::controlCallStatus},
    {},
};

LSSignal displaySignals[] = {
    { "powerKeyPressed" },
    {},
};

DisplayManager::DisplayManager()
    : m_palmService(0)
    , m_service(0)
    , m_publicService(0)
    , m_als(0)
    , m_alsDisabled(false)
    , m_powerdOnline(false)
    , m_chargerConnected(CHARGER_NONE)
    , m_batteryL(100)
    , m_onWhenConnected(false)
    , m_drop_key(false)
    , m_drop_pen(false)
    , m_allow_move(false)
    , m_dnast(0)
    , m_blockedPowerKey(0)
    , m_hasSlider(DeviceInfo::instance()->keyboardSlider())
    , m_sliderOpen(false)
    , m_slidingNow(NOT_SLIDING)
    , m_slidingStart(0)
    , m_dropPowerKey(false)
    , m_penDown(false)
    , m_blockId(0)
    , m_metaId(0)
    , m_lastEvent(0)
    , m_lastKey(0)
    , m_dimTimeout(DEFAULT_TIMEOUT * 2000 / 3)
    , m_offTimeout(DEFAULT_TIMEOUT * 1000 / 3)
    , m_totalTimeout (DEFAULT_TIMEOUT * 1000)
    , m_lockedOffTimeout (Settings::LunaSettings()->lockScreenTimeout)
    , m_activityTimeout(DEFAULT_TIMEOUT *1000)
    , m_powerKeyTimeout(3000)
    , m_onCall(false)
    , m_demo(false)
    , m_bootFinished(false)
    , m_alertState(DISPLAY_UNDEFINED)
    , m_proximityCount(0)
    , m_proximityEnabled(false)
    , m_proximityActivated(false)
    , m_calbackOnToken(0)
    , m_calbackOffToken(0)
    , m_displayOn(false)
    , m_touchpanelIsOn(false)
    , m_backlightIsOn (false)
    , m_activeTouchpanel (false)
    , m_activity(new Timer<DisplayManager>(HostBase::instance()->masterTimer(), this, &DisplayManager::activity))
    , m_power(new Timer<DisplayManager>(HostBase::instance()->masterTimer(), this, &DisplayManager::power))
    , m_slider(new Timer<DisplayManager>(HostBase::instance()->masterTimer(), this, &DisplayManager::slider))
    , m_alertTimer(new Timer<DisplayManager>(HostBase::instance()->masterTimer(), this, &DisplayManager::alertTimerCallback))
    , m_maxBrightness(DEFAULT_BRIGHTNESS)
    , m_currentState (NULL)
    , m_displayStates (NULL)
    , m_lockState(DisplayLockInvalid)
    , m_homeKeyDown(false)
{
    GMainLoop* mainLoop = HostBase::instance()->mainLoop();

    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    initStates();

    connect(SystemUiController::instance(), SIGNAL(signalEmergencyMode(bool)), this, SLOT(slotEmergencyMode(bool)));
    connect(SystemUiController::instance(), SIGNAL(signalBootFinished()), this, SLOT(slotBootFinished()));
    connect(Preferences::instance(), SIGNAL(signalAlsEnabled(bool)), this, SLOT(slotAlsEnabled(bool)));
    connect(IMEController::instance(), SIGNAL(signalShowIME()), this, SLOT(slotShowIME()));
    connect(IMEController::instance(), SIGNAL(signalHideIME()), this, SLOT(slotHideIME()));
    connect(HostBase::instance(), SIGNAL(signalBluetoothKeyboardActive(bool)), this, SLOT(slotBluetoothKeyboardActive(bool)));
    connect(Preferences::instance(), SIGNAL(signalAirplaneModeChanged(bool)), this, SLOT(slotAirplaneModeChanged(bool)));

    m_lastEvent = Time::curTimeMs();
    m_lastKey= m_lastEvent;

    result = LSRegisterPalmService(DISPLAY_APPID, &m_palmService, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    /* save off the public and private handles */
    m_publicService = LSPalmServiceGetPublicConnection(m_palmService);
    if (NULL == m_publicService)
    {
        g_message("unable to get public handle");
    }

    m_service = LSPalmServiceGetPrivateConnection(m_palmService);
    if (NULL == m_service)
    {
        g_message("unable to get private handle");
    }

    result = LSRegisterCategory (m_publicService, "/", publicDisplayMethods, NULL, NULL, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    publicCtx.ctx = this;
    publicCtx.isPublic = true;

    result = LSCategorySetData (m_publicService, "/", &publicCtx, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    result = LSRegisterCategory (m_service, "/control", privateDisplayMethods, NULL, NULL, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    privateCtx.ctx = this;
    privateCtx.isPublic = false;

    result = LSCategorySetData (m_service, "/control", &privateCtx, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    result = LSRegisterCategory (m_service, "/com/palm/display", NULL, displaySignals, NULL, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    result = LSGmainAttachPalmService (m_palmService, mainLoop, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    result = LSSubscriptionSetCancelFunction(m_service, DisplayManager::cancelSubscription, this, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    /*
    result = LSCall(m_service, URI_SIGNAL_ADDMATCH, JSON_CHARGER_SIGNAL_ADDMATCH, DisplayManager::chargerCallback, this, NULL, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }
    */

    result = LSCall(m_service, URI_SIGNAL_ADDMATCH, JSON_USBDOCK_SIGNAL_ADDMATCH, DisplayManager::usbDockCallback, this, NULL, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    result = LSCall(m_service, URI_SIGNAL_ADDMATCH, JSON_BATTERY_SIGNAL_ADDMATCH, DisplayManager::batteryCallback, this, NULL, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    result = LSRegisterServerStatus(m_service, "com.palm.keys", DisplayManager::keysServiceNotification, this, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    result = LSRegisterServerStatus(m_service, "com.palm.audio", DisplayManager::audiodServiceNotification, this, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    bool qemu = HostBase::hostIsQemu();
    if (!qemu)
    {
        // check if the service is up
        result = LSRegisterServerStatus(m_service, "com.palm.power",
                DisplayManager::powerdServiceNotification, this, &lserror);
        if (!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
        }
        result = LSRegisterServerStatus(m_service, "com.palm.telephony",
                DisplayManager::telephonyServiceNotification, this, &lserror);
        if (!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
        }
    }

    m_instance = this;

    // initialize als
    m_als = new AmbientLightSensor ();

#if defined(HAS_LUNA_PREF)
    char *build = NULL;
    LPErr err = LPSystemCopyStringValue ("com.palm.properties.buildName", &build);
    if (LP_ERR_NONE == err && NULL != build)
    {
        if (0 == strcmp ("Nova-Demo", build))
        {
            g_warning ("%s: running in demo mode", __PRETTY_FUNCTION__);
            m_demo = true;
        }
        g_free (build);
    }
#endif

    // get the default brightness
#if defined(HAS_LUNA_PREF)
    LPAppHandle prefHandle = NULL;
    LPErr e = LPAppGetHandle (DISPLAY_APPID, &prefHandle);
    if (LP_ERR_NONE == e && prefHandle)
    {
        // restore the preferences :-)
		json_object* object = 0;
        LPAppCopyValueCJ (prefHandle, "maximumBrightness", &object);
        if (object)
        {
			m_maxBrightness = json_object_get_int(json_object_object_get(object, "maximumBrightness"));
            m_maxBrightness = qBound(MINIMUM_ON_BRIGHTNESS, m_maxBrightness, 100);
			json_object_put(object);
			object = 0;
        }

        // maybe we should get more preferences here
        LPAppCopyValueCJ (prefHandle, "timeout", &object);
        if (object)
        {
            int timeout = json_object_get_int(json_object_object_get(object, "timeout")) * 1000;
            setTimeout (timeout);
			json_object_put(object);
			object = 0;
        }

        LPAppCopyValueCJ (prefHandle, "onWhenConnected", &object);
        if (object)
        {
            m_onWhenConnected = json_object_get_boolean(json_object_object_get(object, "onWhenConnected"));
			json_object_put(object);
			object = 0;
        }

        // discard the prefs handle
        LPAppFreeHandle (prefHandle, false);
    }
#else
    // modify defaults when luna-prefs not supported
    m_onWhenConnected = true;
    setTimeout(DEFAULT_TIMEOUT * 1000);
    m_maxBrightness = DEFAULT_BRIGHTNESS;

#endif

    m_currentState->enter(DisplayStateOff, DisplayEventApiOn, NULL); // initializing state
    m_activity->start (m_activityTimeout);

}

DisplayManager* DisplayManager::instance (void)
{
    return DisplayManager::m_instance;
}

void DisplayManager::initStates()
{
    m_displayStates = new DisplayStateBase* [DisplayStateMax];
    m_displayStates [DisplayStateOff] = new DisplayOff;
    m_displayStates [DisplayStateOffOnCall] = (DisplayStateBase*) new DisplayOffOnCall;
    m_displayStates [DisplayStateOn] = (DisplayStateBase*) new DisplayOn;
    m_displayStates [DisplayStateOnLocked] = (DisplayStateBase*) new DisplayOnLocked;
    m_displayStates [DisplayStateDim] = (DisplayStateBase*) new DisplayDim;
    m_displayStates [DisplayStateOnPuck] = (DisplayStateBase*) new DisplayOnPuck;
    m_displayStates [DisplayStateDockMode] = (DisplayStateBase*) new DisplayDockMode;
    m_displayStates [DisplayStateOffSuspended] = (DisplayStateBase*) new DisplayOffSuspended;

    m_currentState = m_displayStates[DisplayStateOn];
}

void DisplayManager::clearStates()
{
    delete m_displayStates[DisplayStateOff];
    delete m_displayStates[DisplayStateOffOnCall];
    delete m_displayStates[DisplayStateOn];
    delete m_displayStates[DisplayStateOnLocked];
    delete m_displayStates[DisplayStateDim];
    delete m_displayStates[DisplayStateOnPuck];
    delete m_displayStates[DisplayStateDockMode];
    delete m_displayStates[DisplayStateOffSuspended];
    memset (m_displayStates, 0, sizeof (m_displayStates));
    m_currentState = NULL;
}

GMainLoop* DisplayManager::mainLoop()
{
	return HostBase::instance()->mainLoop();
}

bool DisplayManager::isUSBCharging() const
{
    return (m_chargerConnected & CHARGER_USB) ? true : false;
}

bool DisplayManager::isOnCall() const
{
    return m_onCall || m_proximityEnabled;
    // if prox enabled, treat this as on a call.
    // this is to prevent the incoherence of call status (from audiod)
    // & prox sensor status (from phone app)
}

bool DisplayManager::isDemo() const
{
    return m_demo;
}

bool DisplayManager::isSliderOpen() const
{
    return m_sliderOpen;
}

bool DisplayManager::isDNAST() const
{
    return m_dnast > 0;
}

bool DisplayManager::isOnPuck() const
{
    return (m_chargerConnected & CHARGER_INDUCTIVE) ? true : false;
}

bool DisplayManager::isDisplayOn() const
{
    return m_displayOn;
}

bool DisplayManager::isTouchpanelOn() const
{
    return m_touchpanelIsOn;
}

bool DisplayManager::isBacklightOn() const
{
    return m_backlightIsOn;
}

int DisplayManager::dimTimeout() const
{
    return m_dimTimeout;
}

int DisplayManager::offTimeout() const
{
    return m_offTimeout;
}

int DisplayManager::lockedOffTimeout() const
{
    return m_lockedOffTimeout;
}

int DisplayManager::lastEvent() const
{
    return m_lastEvent;
}

bool DisplayManager::hasSlider() const
{
    return m_hasSlider;
}

std::string DisplayManager::puckId() const
{
	return m_puckId;
}

bool DisplayManager::isProximityActivated() const
{
    return m_proximityActivated;
}

DisplayState DisplayManager::currentState() const
{
    return m_currentState->state();
}

bool DisplayManager::isLocked() const
{
    if (m_lockState == DisplayLockLocked || m_lockState == DisplayLockDockMode) {
        g_debug ("%s: display is locked", __PRETTY_FUNCTION__);
        return true;
    }

    g_debug ("%s: display is unlocked", __PRETTY_FUNCTION__);
    return false;
}

bool DisplayManager::pushDNAST(const char *id)
{
    g_warning ("%s: recieved push for DNAST from '%s'", __FUNCTION__, id);

    m_dnast++;

	if (!m_powerdOnline)
		return true;

    g_debug ("%s: push (%i)", __FUNCTION__, m_dnast);

    if (m_dnast == 1)
    {
        if (currentState() == DisplayStateOff
                || currentState() == DisplayStateDim)
        {
            g_message ("%s: Calling on()", __PRETTY_FUNCTION__);
            on();
        }

	m_currentState->stopInactivityTimer();
        notifySubscribers(DISPLAY_EVENT_PUSH_DNAST);
    }

    return true;
}

bool DisplayManager::popDNAST(const char *id)
{
    g_warning ("%s: recieved pop for DNAST from '%s'", __FUNCTION__, id);

    if (m_dnast <= 0) {
        g_critical ("%s: unmatched pop: %d", __FUNCTION__, m_dnast);
		return false;
	}

    m_dnast--;

    if (!m_powerdOnline)
        return true;

    g_debug ("%s: pop (%i)", __FUNCTION__, m_dnast);

    if (m_dnast <= 0)
    {
        g_debug ("%s: popped last DNAST, starting inactivity timer", __FUNCTION__);
        m_currentState->startInactivityTimer();
        notifySubscribers(DISPLAY_EVENT_POP_DNAST);
    }

    return true;
}


bool DisplayManager::powerdServiceNotification(LSHandle *sh, const char *serviceName, bool connected, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result = true;

    DisplayManager *dm = (DisplayManager *)ctx;
    dm->m_powerdOnline = connected;

    if (connected)
    {
        result = LSSignalSend(sh, URI_CHARGER_SIGNAL_REQUEST, JSON_SIGNAL_REQUEST, &lserror);
        if (!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
        }

        result = LSSignalSend(sh, URI_USBDOCK_SIGNAL_REQUEST, JSON_SIGNAL_REQUEST, &lserror);
        if (!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
        }

        result = LSSignalSend(sh, URI_POWERD_BATTERY_SIGNAL_REQUEST, JSON_SIGNAL_REQUEST, &lserror);
        if (!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
        }

        dm->m_lastEvent = Time::curTimeMs();
        g_message ("%s: calling on()", __PRETTY_FUNCTION__);
        dm->on ();
    }
#if !defined(TARGET_DEVICE)
    // dont (ever) go to sleep in the emulator
    dm->pushDNAST ("LunaSysMgr-on-Desktop");
#endif

    return true;
}

bool DisplayManager::telephonyServiceNotification(LSHandle *sh, const char *serviceName, bool connected, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result = true;

    if (connected)
    {
        result = LSCall(sh, URI_TELEPHONY_CHARGER_SIGNAL_REQUEST, JSON_SIGNAL_REQUEST, NULL, NULL, NULL, &lserror);
        if (!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
        }
    }

    return true;
}

bool DisplayManager::keysServiceNotification(LSHandle *sh, const char *serviceName, bool connected, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result = true;

    if (connected)
    {
        result = LSCall(sh, URI_SLIDER_STATUS_REQUEST, JSON_SLIDER_STATUS_REQUEST, DisplayManager::sliderCallback, ctx, NULL, &lserror);
        if (!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
        }
    }

    return true;
}

bool DisplayManager::audiodServiceNotification(LSHandle *sh, const char *serviceName, bool connected, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result = true;

    if (connected)
    {
        result = LSCall(sh, URI_AUDIOD_STATUS, JSON_AUDIOD_SUBSCRIBE, DisplayManager::audiodCallback, ctx, NULL, &lserror);
        if (!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
        }
    }

    return true;
}

bool DisplayManager::proximityOn ()
{
    if (!m_proximityEnabled)
    {
        /* fine-tuning support for NYX */
        InputControl* ic = HostBase::instance()->getInputControlProximity();
        if (NULL != ic)
        {
            if (!ic->on())
                return false;
        }
        m_proximityEnabled = true;
    }

    return true;
}

bool DisplayManager::proximityOff ()
{
    if (m_proximityActivated) {
        updateState (DISPLAY_EVENT_PROXIMITY_OFF);
    }

    if (m_proximityEnabled)
    {
        /* fine-tuning support for NYX */
        InputControl* ic = HostBase::instance()->getInputControlProximity();
        if (NULL != ic)
        {
            if (!ic->off())
                return false;
        }
        m_proximityEnabled = false;
        m_proximityActivated = false;
    }

    return true;
}


bool DisplayManager::audiodCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);

    // {"action": string, "scenario": string, "active": boolean}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_3(REQUIRED(action, string), REQUIRED(scenario, string), REQUIRED(active, boolean)));

    DisplayManager* dm = (DisplayManager *)ctx;

    json_object *active = NULL, *action = NULL, *scenario = NULL;
    bool onCall = false;
    const char* str = LSMessageGetPayload(message);
    if (!str)
        return true;
    json_object* root = json_tokener_parse(str);
    if (!root || is_error(root))
        return true;

    action = json_object_object_get(root, "action");
    scenario = json_object_object_get (root, "scenario");

    if (action && scenario) {
        if (strncmp (json_object_get_string (scenario), "phone_headset", strlen ("phone_headset")) == 0
                && (strncmp (json_object_get_string (action), "enabled", strlen ("enabled")) == 0
                    || strncmp (json_object_get_string (action), "disabled", strlen ("disabled")) == 0))
        {
            g_debug ("user action with the headset, turning display on");
            dm->m_currentState->handleEvent (DisplayEventUserActivity);
        }
    }

    active = json_object_object_get(root, "active");
    if (active)
    {
        onCall = json_object_get_boolean(active);
        if (dm->m_onCall != onCall)
        {
            dm->m_onCall = onCall;
            if (onCall) {
                dm->updateState (DISPLAY_EVENT_ON_CALL);
            }
            else {
                dm->updateState (DISPLAY_EVENT_OFF_CALL);
                // reset the m_lastEvent when the call ends
                // to prevent the lock of the display when the display
                // turns on.
                dm->m_lastEvent = Time::curTimeMs();
                // update the display ALS state
                dm->updateState (DISPLAY_EVENT_ALS_REGION_CHANGED);
            }
        }
    }

    json_object_put(root);

    return true;
}

bool DisplayManager::sliderCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);
    LSMessageToken token;

    // {"key": string, "state": string}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_2(REQUIRED(key, string), REQUIRED(state, string)));

    DisplayManager* dm = (DisplayManager *)ctx;

    // modify the state according to the message
    // and store it in the newState
	json_object* root = 0;
	json_object* label = 0;
	const char* str = LSMessageGetPayload(message);
	if (!str)
		goto error;
	root = json_tokener_parse(str);
	if (!root || is_error(root))
		goto error;

	label = json_object_object_get(root, "key");
	if (!label)
		goto error;

    if (0 != strcmp (json_object_get_string(label), "slider"))
        goto error;

    label = json_object_object_get(root, "state");
    if (!label)
        goto error;

    if (0 == strcmp (json_object_get_string(label), "up"))
    {
        dm->m_sliderOpen = true;
    }
    else if (0 == strcmp (json_object_get_string(label), "down"))
    {
        dm->m_sliderOpen = false;
    }

error:

    token = LSMessageGetResponseToken (message);
   	if (!LSCallCancel (sh, token, &lserror))
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    if (root && !is_error(root))
        json_object_put(root);

    return true;
}

bool DisplayManager::batteryCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result = false;

    // {"percent": integer}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_1(REQUIRED(percent, integer)));

    DisplayManager *dm = (DisplayManager *)ctx;
    json_object* root = 0;
    const char* str = LSMessageGetPayload(message);
    if (str) {
	root = json_tokener_parse(str);
	result = (root && !is_error(root));
    }

    if (result)
    {
	json_object* label = json_object_object_get(root, "percent");
	if (label) {

	    dm->m_batteryL = json_object_get_int(label);

	    // if the brightness is changed because of the
	    // battery level changing, update it.
	    dm->updateBrightness();
	}

	json_object_put(root);
    }

    return true;
}

bool DisplayManager::usbDockCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);

    // {"Charging": boolean, "DockConnected": boolean, "DockPower": false, "DockSerialNo": string, "USBConnected": boolean, "USBName": string}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_6(REQUIRED(Charging, boolean), REQUIRED(DockConnected, boolean), REQUIRED(DockPower, boolean), REQUIRED(DockSerialNo, string), REQUIRED(USBConnected, boolean), REQUIRED(USBName, string)));

    DisplayManager *dm = (DisplayManager *)ctx;
    int32_t newState = dm->m_chargerConnected;

    bool usb = true;
    int event = DISPLAY_EVENT_NONE;

    json_object* label = 0;
    json_object* root = 0;
    const char* str = LSMessageGetPayload(message);
    bool charging = false;

    if (!str)
            goto error;

    root = json_tokener_parse(str);
    if (!root || is_error(root))
            goto error;

    label = json_object_object_get(root, "Charging");
    if (label && !is_error (label)) {
	    charging = json_object_get_boolean (label);
    }

    label = json_object_object_get(root, "DockConnected");
    if (label && !is_error (label)) {
	    bool dockConnected = json_object_get_boolean (label);
	    if (dockConnected && charging) {
		    newState |= CHARGER_INDUCTIVE;

		    label = json_object_object_get(root, "DockSerialNo");
		    if (label && !is_error (label)) {
			    dm->m_puckId = json_object_get_string (label);
			    if (dm->m_puckId == "NULL")
				    dm->m_puckId.clear();
		    }

		    if (dm->m_chargerConnected != newState)
		    {
			    event = DISPLAY_EVENT_INDUCTIVE_CHARGER_CONNECTED;
			    dm->updateState (event);
		    }
	    }
	    else if (dm->m_chargerConnected & CHARGER_INDUCTIVE) {
		    newState &= ~(CHARGER_INDUCTIVE);
		    dm->m_puckId.clear();

		    if (dm->m_chargerConnected != newState)
		    {
			    event = DISPLAY_EVENT_INDUCTIVE_CHARGER_DISCONNECTED;
			    dm->updateState (event);
		    }
	    }
    }

    label = json_object_object_get(root, "USBConnected");
    if (label && !is_error (label)) {
	    bool usbConnected = json_object_get_boolean (label);
	    if (usbConnected) {
		    newState |= CHARGER_USB;
		    if (dm->m_chargerConnected != newState
				    && !(dm->m_chargerConnected & CHARGER_INDUCTIVE))
		    {
			    event = DISPLAY_EVENT_USB_CHARGER_CONNECTED;
			    dm->updateState (event);
		    }
	    }
	    else {
		    newState &= ~(CHARGER_USB);
		    if (dm->m_chargerConnected != newState
				    && !(dm->m_chargerConnected & CHARGER_INDUCTIVE))
		    {
			    event = DISPLAY_EVENT_USB_CHARGER_DISCONNECTED;
			    dm->updateState (event);
		    }
	    }
    }

    dm->m_chargerConnected = newState;
    dm->updateBrightness ();

error:

	if (root && !is_error(root))
		json_object_put(root);

    return true;
}
bool DisplayManager::chargerCallback(LSHandle *sh, LSMessage *message, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);

    // {"type": string, "connected": boolean}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_2(REQUIRED(type, string), REQUIRED(connected, boolean)));

    DisplayManager *dm = (DisplayManager *)ctx;
    int32_t newState = dm->m_chargerConnected;

    bool usb = true;
    int event = DISPLAY_EVENT_NONE;

    json_object* label = 0;
    json_object* root = 0;
    const char* str = LSMessageGetPayload(message);
    if (!str)
            goto error;

    root = json_tokener_parse(str);
    if (!root || is_error(root))
            goto error;

    label = json_object_object_get(root, "type");
    if (!label)
            goto error;

    if (0 == strcmp (json_object_get_string(label), "usb"))
    {
        usb = true;
    }
    else if (0 == strcmp (json_object_get_string(label), "inductive"))
    {
        usb = false;
    }
    else
    {
        goto error;
    }

    label = json_object_object_get(root, "connected");
    if (!label)
        goto error;

    if (json_object_get_boolean(label))
    {
        if (usb)
        {
            newState |= CHARGER_USB;

            if (dm->m_chargerConnected != newState)
            {
                event = DISPLAY_EVENT_USB_CHARGER_CONNECTED;
            }
        }
        else
        {
            newState |= CHARGER_INDUCTIVE;

            if (dm->m_chargerConnected != newState)
            {
                event = DISPLAY_EVENT_INDUCTIVE_CHARGER_CONNECTED;
            }
        }
    }
    else
    {
        if (usb)
        {
            newState &= ~(CHARGER_USB);

            if (dm->m_chargerConnected != newState)
            {
                event = DISPLAY_EVENT_USB_CHARGER_DISCONNECTED;
            }
        }
        else
        {
            newState &= ~(CHARGER_INDUCTIVE);

            if (dm->m_chargerConnected != newState)
            {
                event = DISPLAY_EVENT_INDUCTIVE_CHARGER_DISCONNECTED;
            }
        }
    }

    dm->m_chargerConnected = newState;

    if (DISPLAY_EVENT_NONE != event)
        dm->updateState (event);

    dm->updateBrightness ();

error:

	if (root && !is_error(root))
		json_object_put(root);

    return true;
}

/*!
\page com_palm_display_control
\n
\section com_palm_display_control_set_state setState

\e Private.

com.palm.display/control/setState

Set the state of the display.

\subsection com_palm_display_control_set_state_syntax Syntax:
\code
{
    "state": string
}
\endcode

\param state The state to which the display is set. Can be:
\li on
\li dimmed
\li off
\li unlock
\li dock
\li undock

\subsection com_palm_display_control_set_state_returns Returns:
\code
{
    "returnValue": boolean,
    "errorText": string
}
\endcode

\param

\subsection com_palm_display_control_set_state_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.display/control/setState '{ "state": "dock" }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": false,
    "errorText": "call failed"
}
\endcode
*/
bool DisplayManager::controlSetState(LSHandle *sh, LSMessage *message, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);

    // {"state" : string}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_1(REQUIRED(state, string)));

    bool result = false;
    DisplayManager *dm = ((DisplayCallbackCtx_t *)ctx)->ctx;
	json_object* root = 0;
	const char* value = 0;
	const char* str = LSMessageGetPayload(message);
	if (!str)
		goto send;
	root = json_tokener_parse(str);
	if (!root || is_error(root))
		goto send;

	value = json_object_get_string(json_object_object_get(root, "state"));
	if (!value || is_error (value))
		goto send;


    result = true;
    if (0 == strcmp (value, "on"))
    {
        g_message ("%s: calling on() from %s", __PRETTY_FUNCTION__, LSMessageGetApplicationID (message));
        if (dm->currentState() != DisplayStateDockMode)
            dm->on ();
        else
            g_message ("%s: ignoring call to go on() in nightstand mode", __PRETTY_FUNCTION__);
    }
    else if (0 == strcmp (value, "dimmed"))
    {
        g_message ("%s: calling dim()", __PRETTY_FUNCTION__);
        dm->dim ();
    }
    else if (0 == strcmp (value, "off"))
    {
        g_message ("%s: calling off()", __PRETTY_FUNCTION__);
        dm->off ();
    }
    else if (0 == strcmp (value, "unlock"))
    {
        g_message ("%s: calling unlock()", __PRETTY_FUNCTION__);
        dm->on ();
        dm->unlock ();
    }
    else if (0 == strcmp (value, "dock"))
    {
        g_message ("%s: calling dock()", __PRETTY_FUNCTION__);
        dm->dock ();
    }
    else if (0 == strcmp (value, "undock"))
    {
        g_message ("%s: calling undock()", __PRETTY_FUNCTION__);
        dm->undock ();
    }
    else
    {
        result = false;
    }

send:

    if (result)
        result = LSMessageReply(sh, message, "{\"returnValue\":true}", &lserror);
    else
        result = LSMessageReply(sh, message, "{\"returnValue\":false,\"errorText\":\"call failed\"}", &lserror);

    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

	if (root && !is_error(root))
		json_object_put(root);

    return true;
}

#if 0
bool DisplayManager::controlCallStatus(LSHandle *sh, LSMessage *message, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);

    bool result = false;
    DisplayManager *dm = ((DisplayCallbackCtx_t *)ctx)->ctx;
	json_object* root = 0;
	const char* value = 0;
	const char* str = LSMessageGetPayload(message);
	if (!str)
		goto send;
	root = json_tokener_parse(str);
	if (!root || is_error(root))
		goto send;

	value = json_object_get_string(json_object_object_get(root, "state"));

    result = true;
    if (0 == strcmp (value, "on"))
    {
        // set the flags for onCall
        if (!dm->m_onCall) {
            dm->m_onCall = true;
            dm->m_lastEvent = Time::curTimeMs();
            dm->updateState (DISPLAY_EVENT_ON_CALL);
        }
        // check if the audio is on the front speaker and start proximity sensor
    }
    else if (0 == strcmp (value, "off"))
    {
        if (dm->m_onCall) {
            dm->m_onCall = false;
            dm->m_lastEvent = Time::curTimeMs();
            dm->updateState (DISPLAY_EVENT_OFF_CALL);
        }
        // if prox sensor was enabled, turn it off and turn display on
    }
    else
    {
        result = false;
    }

send:

    if (result)
        result = LSMessageReply(sh, message, "{\"returnValue\":true}", &lserror);
    else
        result = LSMessageReply(sh, message, "{\"returnValue\":false,\"errorText\":\"call failed\"}", &lserror);

    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

	if (root && !is_error(root))
		json_object_put(root);

    return true;
}
#endif

bool DisplayManager::cancelSubscription(LSHandle *sh, LSMessage *message, void *ctx)
{
    // {"requestBlock": boolean, "client": string, "powerKeyBlock": boolean, "proximityEnabled": boolean}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_4(REQUIRED(requestBlock, boolean), REQUIRED(client, string), REQUIRED(powerKeyBlock, boolean), REQUIRED(proximityEnabled, boolean)));

    g_debug("%s: category %s, method %s", __PRETTY_FUNCTION__, LSMessageGetCategory(message), LSMessageGetMethod(message));
    DisplayManager *dm = (DisplayManager *)ctx;

    if (0 == strcmp (LSMessageGetMethod(message), "setProperty") &&
        0 == strcmp (LSMessageGetCategory(message), "/control"))
    {

        // deterimne it if for DNAST or PKBSK
        const char* str = LSMessageGetPayload(message);
		if (!str)
			return true;
		json_object* root = json_tokener_parse(str);
		if (!root || is_error(root))
			return true;

		bool value = json_object_get_boolean(json_object_object_get(root, "requestBlock"));
        if (value)
        {
            const char *client = json_object_get_string(json_object_object_get(root, "client"));
            if (client)
            {
                gchar *clientID = g_strdup_printf ("%s-%s", client, LSMessageGetSender(message));
                dm->popDNAST (clientID);
                g_free (clientID);
            }
        }

		value = json_object_get_boolean(json_object_object_get(root, "powerKeyBlock"));
        if (value)
        {
            const char *client = json_object_get_string(json_object_object_get(root, "client"));
            if (client)
            {
                dm->m_blockedPowerKey--;
                if (dm->m_blockedPowerKey < 0)
                {
                    // correct the value. should never be 0.
                    dm->m_blockedPowerKey = 0;
                    g_warning ("%s: m_blockedPowerKey was < 0, corrected now", __FUNCTION__);
                }
            }
        }

		value = json_object_get_boolean(json_object_object_get(root, "proximityEnabled"));
        if (value)
        {
            const char *client = json_object_get_string(json_object_object_get(root, "client"));
            if (client)
            {
                dm->m_proximityCount--;
                if (dm->m_proximityCount < 0)
                {
                    // correct the value. should never be 0.
                    dm->m_proximityCount = 0;
                    g_warning ("%s: m_proximityCount was < 0, corrected now", __FUNCTION__);
                }
                if (0 == dm->m_proximityCount && dm->currentState() != DisplayStateOffOnCall)
                {
                    dm->proximityOff ();
                }
            }
        }

		json_object_put(root);
    }

    return true;
}

bool DisplayManager::notifySubscribers(int type, sptr<Event> event)
{
    // notify all the subscribers of changes to the system
    // like state changes or DNAST mode

    LSError lserror;
    LSErrorInit(&lserror);
    bool result;
    bool isPublic = false;

    gchar *notification = NULL;

    switch (type)
    {
        case DISPLAY_EVENT_ON:
            isPublic = true;
            notification = g_strdup_printf ("{\"returnValue\":true,\"event\":\"displayOn\"}");
            break;
        case DISPLAY_EVENT_DOCKMODE:
            isPublic = true;
            notification = g_strdup_printf ("{\"returnValue\":true,\"event\":\"displayOn\",\"dockMode\":true}");
            break;
        case DISPLAY_EVENT_DIMMED:
            isPublic = true;
            notification = g_strdup_printf ("{\"returnValue\":true,\"event\":\"displayDimmed\"}");
            break;
        case DISPLAY_EVENT_OFF:
            isPublic = true;
            notification = g_strdup_printf ("{\"returnValue\":true,\"event\":\"displayOff\"}");
            break;
        case DISPLAY_EVENT_TIMEOUTS:
            notification = g_strdup_printf ("{\"returnValue\":true,\"event\":\"changedTimeout\",\"timeout\":%i}",
                    (m_totalTimeout) / 1000);
            break;
        case DISPLAY_EVENT_PUSH_DNAST:
            notification = g_strdup_printf ("{\"returnValue\":true,\"event\":\"blockedDisplay\"}");
            break;
        case DISPLAY_EVENT_POP_DNAST:
            notification = g_strdup_printf ("{\"returnValue\":true,\"event\":\"unblockedDisplay\"}");
            break;
        case DISPLAY_EVENT_ACTIVE:
            notification = g_strdup_printf ("{\"returnValue\":true,\"event\":\"displayActive\"}");
            break;
        case DISPLAY_EVENT_INACTIVE:
            notification = g_strdup_printf ("{\"returnValue\":true,\"event\":\"displayInactive\"}");
            break;
        case DISPLAY_EVENT_REQUEST:
        default:
            g_debug("%s: Ignored: %d", __PRETTY_FUNCTION__, type);
            break;
    }

    if (notification)
    {
        g_debug("%s: %s", __PRETTY_FUNCTION__, notification);
        // Post all events to private bus
        result = LSSubscriptionPost (m_service, "/control", "status", notification, &lserror);
        if (!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
        }

        // Only a subset of the events go onto the public bus
        if (isPublic)
        {
            result = LSSubscriptionPost (m_publicService, "/", "status", notification, &lserror);
            if (!result)
            {
                LSErrorPrint (&lserror, stderr);
                LSErrorFree (&lserror);
            }
        }

        g_free (notification);
    }

    return true;
}

bool DisplayManager::updateTimeout(int timeoutInMs)
{
	bool success = setTimeout(timeoutInMs);
	if (!success)
		return false;

	// save to luna-prefs
	LSError lserror;
	LSErrorInit(&lserror);
	gchar* payload = g_strdup_printf("{\"appId\":\"com.palm.display\",\"key\":\"timeout\",\"value\":{\"timeout\":%i}}",
									 (m_totalTimeout) / 1000);
	success = LSCallOneReply (m_service, URI_PREFS_SET, payload, NULL, NULL, NULL, &lserror);
	if (!success) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}
	g_free(payload);
	return success;
}

bool DisplayManager::setTimeout (int timeoutInMs)
{
    if (timeoutInMs <= 0) {
        timeoutInMs = DEFAULT_TIMEOUT * 1000;
    }

    m_dimTimeout = (timeoutInMs * 2) / 3;
    m_offTimeout = timeoutInMs / 3;
    m_totalTimeout = timeoutInMs;

    notifySubscribers (DISPLAY_EVENT_TIMEOUTS);

    m_currentState->startInactivityTimer();
    return true;
}

/*!
\page com_palm_display_control
\n
\section com_palm_display_control_get_property getProperty

\e Private.

com.palm.display/control/getProperty

Get display properties.

\subsection com_palm_display_control_get_property_syntax Syntax:
\code
{
    "properties": [string array]
}
\endcode

\param properties List of properties to get, can include one or more of the following:
\li requestBlock
\li powerKeyBlock
\li timeout
\li maximumBrightness
\li onWhenConnected
\li proximityEnabled

\subsection com_palm_display_control_get_property_returns Returns:
\code
{
    "returnValue": boolean,
    "<property name>": <value>,
    ...
    "<property name>": <value>,
    "errorCode": int,
    "errorText": string
}
\endcode

\param returnValue Indicates if the call was succesful.
\param errorCode Code for the error if call was not succesful.
\param errorText Describes the error if call was not succesful.

\subsection com_palm_display_control_get_property_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.display/control/getProperty '{ "properties": ["maximumBrightness", "timeout", "requestBlock", "onWhenConnected" ] } '
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "maximumBrightness": 75,
    "timeout": 120,
    "requestBlock": false,
    "onWhenConnected": false
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": false,
    "errorCode": 1,
    "errorText": "failed to get property"
}
\endcode
*/
bool DisplayManager::controlGetProperty(LSHandle *sh, LSMessage *message, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);

    // {"properties": array}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_1(REQUIRED(properties, array)));

    bool result = true;
	const char* str = LSMessageGetPayload(message);
	json_object* root = 0;
	json_object* array = 0;
	json_object* reply = 0;

    DisplayManager *dm = ((DisplayCallbackCtx_t *)ctx)->ctx;

	reply = json_object_new_object();

	if (!str) {
        result = false;
        goto done;
    }
	root = json_tokener_parse(str);
	if (!root || is_error(root)) {
		result = false;
		goto done;
	}

	json_object_object_add(reply, "returnValue", json_object_new_boolean(true));

    result = false;

	array = json_object_object_get(root, "properties");
    if (!array || !json_object_is_type(array, json_type_array))
        goto done;

	for (int i=0; i<json_object_array_length(array); i++) {

		const char* property = json_object_get_string(json_object_array_get_idx(array, i));
		if (!property)
			continue;

        g_debug ("%s: (%d) %s", __FUNCTION__, i, property);

		if (0 == strcmp (property, "requestBlock"))
		{
			result = true;
			json_object_object_add(reply, "requestBlock", json_object_new_boolean(dm->m_dnast > 0));
		}
		else if (0 == strcmp (property, "powerKeyBlock"))
		{
			result = true;
			json_object_object_add(reply, "powerKeyBlock", json_object_new_boolean(dm->m_blockedPowerKey > 0));
		}
		else if (0 == strcmp (property, "timeout"))
		{
			result = true;
			json_object_object_add(reply, "timeout", json_object_new_int( (dm->m_totalTimeout) / 1000));
		}
		else if (0 == strcmp (property, "maximumBrightness"))
		{
			result = true;
			json_object_object_add(reply, "maximumBrightness", json_object_new_int(dm->m_maxBrightness));
		}
		else if (0 == strcmp (property, "onWhenConnected"))
		{
			result = true;
			json_object_object_add(reply, "onWhenConnected", json_object_new_boolean(dm->m_onWhenConnected));
		}
		else if (0 == strcmp (property, "proximityEnabled"))
		{
			result = true;
			json_object_object_add(reply, "proximityEnabled", json_object_new_boolean(dm->m_proximityCount > 0));
		}
    }

done:

    if (result)
        result = LSMessageReply(sh, message, json_object_to_json_string(reply), &lserror);
    else
        result = LSMessageReply(sh, message, "{\"returnValue\":false,\"errorCode\":1,\"errorText\":\"failed to get property\"}", &lserror);

    if (reply)
	json_object_put(reply);

    if(!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    return true;
}

/*!
\page com_palm_display_control
\n
\section com_palm_display_control_set_property setProperty

\e Private.

com.palm.display/control/setProperty

Set display properties.

\subsection com_palm_display_control_set_property_syntax Syntax:
\code
{
    "requestBlock": boolean,
    "client": string,
    "powerKeyBlock": boolean,
    "timeout": integer,
    "onWhenConnected": boolean,
    "maximumBrightness": integer,
    "proximityEnabled": boolean
}
\endcode

\param requestBlock Block changes of display state. Requires \e client parameter to be set.
\param client Client ID.
\param powerKeyBlock Block the power key. Requires \e client parameter to be set.
\param timeout Timeout in seconds for the display to turn off.
\param onWhenConnected Should the display remain on when a USB cable is connected to the device.
\param maximumBrightness Display maximum brightness.
\param proximityEnabled Toggle proximity sensor. Requires \e client parameter to be set.

\subsection com_palm_display_control_set_property_returns Returns:
\code
{
    "returnValue": boolean,
    "errorCode": int,
    "errorText": string
}
\endcode

\param returnValue Indicates if the call was succesful.
\param errorCode Code for the error if call was not succesful.
\param errorText Describes the error if call was not succesful.

\subsection com_palm_display_control_set_property_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.display/control/setProperty '{ "timeout": 140 }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": false,
    "errorCode": 22,
    "errorText": "'powerKeyBlock' needs 'client' string"
}
\endcode
*/
bool DisplayManager::controlSetProperty(LSHandle *sh, LSMessage *message, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);

    // {"requestBlock": boolean, "client": string, "powerKeyBlock": boolean, "timeout": integer, "onWhenConnected": boolean, "maximumBrightness": integer, "proximityEnabled": boolean, }
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_7(REQUIRED(requestBlock, boolean), REQUIRED(client, string), REQUIRED(powerKeyBlock, boolean), REQUIRED(timeout, integer), REQUIRED(onWhenConnected, boolean), REQUIRED(maximumBrightness, integer), REQUIRED(proximityEnabled, boolean)));

    bool result = false;
	const char* str = LSMessageGetPayload(message);
	json_object* root = 0;
	json_object* label = 0;
    DisplayManager *dm = ((DisplayCallbackCtx_t *)ctx)->ctx;
    const char *errorText = NULL;
    int errorCode = 0;

    int timeout = -1;
    bool onWhenConnected = true;
    int32_t mbr = dm->m_maxBrightness;

	if (!str)
        goto done;
	root = json_tokener_parse(str);
	if (!root || is_error(root))
		goto done;

	label = json_object_object_get(root, "requestBlock");
    if (json_object_get_boolean(label))
    {
        const char *client = json_object_get_string(json_object_object_get(root, "client"));
        if (!client)
        {
                errorCode = 22;
                errorText = "'requestBlock' needs 'client' string";
                result = false;
                goto done;
        }

        result = LSSubscriptionAdd (sh, DNAST_SUBSCRIPTION_KEY, message, &lserror);
        if(!result)
        {
            errorCode = 25;
            errorText = "failed to subscribe";
            LSErrorFree (&lserror);
        }
        else
        {
            gchar *clientID = g_strdup_printf ("%s-%s", client, LSMessageGetSender(message));
            dm->pushDNAST (clientID);
            g_free (clientID);
        }
    }
    else
        result = true;

	label = json_object_object_get(root, "powerKeyBlock");
    if (json_object_get_boolean(label))
    {
        const char *client = json_object_get_string(json_object_object_get(root, "client"));
        if (!client)
        {
                errorCode = 22;
                errorText = "'powerKeyBlock' needs 'client' string";
                result = false;
                goto done;
        }

        result = LSSubscriptionAdd (sh, POWER_KEY_BLOCK_SUBSCRIPTION_KEY, message, &lserror);
        if(!result)
        {
            errorCode = 25;
            errorText = "failed to subscribe";
            LSErrorFree (&lserror);
        }
        else
        {
            dm->m_blockedPowerKey++;
        }
    }
    else
        result = true;

	label = json_object_object_get(root, "timeout");
	timeout = json_object_get_int(label);
    if (label && timeout != -1)
    {
		result = dm->updateTimeout(timeout * 1000);
		if (!result) {
            errorCode = 43;
            errorText = "failed to set timeout values";
            result = false;
            goto done;
        }
    }
    else
        result = true;

	label = json_object_object_get(root, "onWhenConnected");
    if (label)
    {
		onWhenConnected = json_object_get_boolean(label);
        // store the setting in the preferences.
        gchar *blah = g_strdup_printf ("{\"appId\":\"com.palm.display\",\"key\":\"onWhenConnected\",\"value\":{\"onWhenConnected\":%s}}",
                onWhenConnected ? "true" : "false");
        result = LSCallOneReply  (sh, URI_PREFS_SET, blah, NULL, NULL, NULL, &lserror);
        g_free (blah);

        if (!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
            errorCode = 33;
            errorText = "could not store 'onWhenConnected' in preferences";
            goto done;
        }

        // the value is changing from what it was before
        if (dm->m_onWhenConnected != onWhenConnected)
        {
            dm->m_onWhenConnected = onWhenConnected;
            // if the usb is connected and since value is changing
            // make sure that we block transition changes
            // and note with an id why we do block the state changes.
            if (CHARGER_USB & dm->m_chargerConnected)
            {
                gchar *report = g_strdup_printf ("%s-dm-usb-charger-connected", __FUNCTION__);
                if (dm->m_onWhenConnected)
                    dm->pushDNAST (report);
                else
                    dm->popDNAST (report);
                g_free (report);
            }
        }
    }
    else
        result = true;

	label = json_object_object_get(root, "maximumBrightness");
	mbr = json_object_get_int(label);
    if (label)
    {
        result = dm->setMaximumBrightness (mbr);
        if(!result) {
            errorCode = 54;
            errorText = "failed to store maximumBrightness in preferences";
        }

        Q_EMIT dm->signalDisplayMaxBrightnessChanged(mbr);
    }
    else
        result = true;

	label = json_object_object_get(root, "proximityEnabled");
    if (json_object_get_boolean(label))
    {
        const char *client = json_object_get_string(json_object_object_get(root, "client"));
        if (!client)
        {
                errorCode = 22;
                errorText = "'proximityEnabled' needs 'client' string";
                result = false;
                goto done;
        }

        result = LSSubscriptionAdd (sh, PROXIMITY_SUBSCRIPTION_KEY, message, &lserror);
        if(!result)
        {
            errorCode = 25;
            errorText = "failed to subscribe";
            LSErrorFree (&lserror);
        }
        else
        {
            if (0 == dm->m_proximityCount)
                dm->proximityOn ();

            dm->m_proximityCount++;
        }
    }
    else
        result = true;

done:

    if (result)
        result = LSMessageReply(sh, message, "{\"returnValue\":true}", &lserror);
    else
    {
        gchar *r = g_strdup_printf ("{\"returnValue\":false,\"errorCode\":%i,\"errorText\":\"%s\"}", errorCode, errorText);
        result = LSMessageReply(sh, message, r, &lserror);
        g_free (r);
    }
    if(!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

	if (root && !is_error(root))
		json_object_put(root);

    return true;
}


/*
 * Returns the max allowed brightness of the CoreNavi lights. (0-100)
 */
uint32_t DisplayManager::getCoreNaviBrightness()
{
    if (DeviceInfo::instance()->coreNaviButton()) {
	// Only castle ALS is bust
	int region = m_als->getCurrentRegion ();

	switch (region)
	{
	    case ALS_REGION_OUTDOOR:
		return 100;
		break;
	    case ALS_REGION_UNDEFINED:
	    case ALS_REGION_DARK:
	    default:
		return 25;
		break;
	}
    }
    else {
	if (currentState() == DisplayStateOn
		|| currentState() == DisplayStateOnLocked
		|| currentState() == DisplayStateOnPuck)
	    return Settings::LunaSettings()->coreNaviScaler * getDisplayBrightness() / 100;
	else if (currentState() == DisplayStateDim
		|| currentState() == DisplayStateDockMode)
	    return MINIMUM_DIMMED_BRIGHTNESS;
	else
	    return 0;
    }

}
/* the following function defines the levels of
 * backlight britness using current system state
 * including factors like battery level and
 * ambient light sensor (soming soon to  a
 * pre near you
 */
int32_t DisplayManager::getDisplayBrightness()
{
	// Set display to max brightness when in minimal mode.
    if (Settings::LunaSettings()->uiType == Settings::UI_MINIMAL) {
	    g_message ("UI is in minimal mode, setting to max brightness");
	    return 100;
    }

    int b = m_maxBrightness;

    if (m_chargerConnected == CHARGER_NONE)
    {
	// since there is no charger adjust the level
	// of brightness if battery is low
	if (m_batteryL < 30)
	    b -= 5;
	if (m_batteryL < 20)
	    b -= 5;
	if (m_batteryL < 10)
	    b -= 10;
    }

    if (Preferences::instance()->isAlsEnabled()) {
		int region = m_als->getCurrentRegion ();

		switch (region)
		{
		case ALS_REGION_OUTDOOR:
			b = Settings::LunaSettings()->backlightOutdoorScale * b / 100;
			break;
		case ALS_REGION_DIM:
			b = Settings::LunaSettings()->backlightDimScale * b / 100;
			break;
		case ALS_REGION_DARK:
			b = Settings::LunaSettings()->backlightDarkScale * b / 100;
			break;
		default:
			// for all other cases, we use b directly
			break;
		}
    }

    if (b < MINIMUM_ON_BRIGHTNESS)
	b = MINIMUM_ON_BRIGHTNESS;

    if (b > 100)
	b = 100;

    if (b < 0)
	b = 0;

    return b;
}


int32_t DisplayManager::getKeypadBrightness()
{
    int b = m_maxBrightness;

    if (m_hasSlider && !m_sliderOpen)
        return 0;

    if (currentState() == DisplayStateOff
	    || currentState() == DisplayStateOffOnCall
	    || currentState() == DisplayStateDim
	    || currentState() == DisplayStateDockMode)
        return 0;

    if (m_chargerConnected == CHARGER_NONE)
    {
       // since there is no charger adjust the level
       // of brightness if battery is low
        if (m_batteryL < 30)
            b -= 5;
        if (m_batteryL < 20)
            b -= 5;
        if (m_batteryL < 10)
            b -= 10;
    }

    int region = m_als->getCurrentRegion ();

    switch (region)
    {
        case ALS_REGION_OUTDOOR:
            b = 0;
            break;
        default:
            break;
    }

    if (b > 100)
	b = 100;

    if (b < 0)
	b = 0;

    return b;
}

bool DisplayManager::updateBrightness ()
{
    m_currentState->handleEvent (DisplayEventUpdateBrightness);
    return true;

}

bool DisplayManager::setMaximumBrightness (int maxBrightness, bool save)
{
	int result = true;

    maxBrightness = qBound(MINIMUM_ON_BRIGHTNESS, maxBrightness, 100);

    if (maxBrightness != m_maxBrightness)
    {
        if(currentState() == DisplayStateOn
			|| currentState() == DisplayStateOnPuck
			|| currentState() == DisplayStateDockMode) {
		// set the new max brightness
		m_maxBrightness = maxBrightness;
		// update the brightness directly
		backlightOn (getDisplayBrightness(), getKeypadBrightness(), false);
		// update navi brightness
		CoreNaviManager::instance()->updateBrightness (getCoreNaviBrightness());

		Q_EMIT signalDisplayMaxBrightnessChanged(maxBrightness);
	} 
	else {
            updateBrightness ();
        }
    }

	if(save) {
		LSError lserror;
		LSErrorInit(&lserror);
	   // send a call to set a new value
		gchar *blah = g_strdup_printf ("{\"appId\":\"com.palm.display\",\"key\":\"maximumBrightness\",\"value\":{\"maximumBrightness\":%i}}", m_maxBrightness);
		result = LSCallOneReply  (m_service, URI_PREFS_SET, blah, NULL, NULL, NULL, &lserror);
		g_free (blah);
		if (!result)
		{
			LSErrorPrint (&lserror, stderr);
			LSErrorFree (&lserror);
		}
	}
    return result;
}

/*!
\page com_palm_display
\n
\section com_palm_display_status status

\e Public.

com.palm.display/status

Get the status of the display.

\subsection com_palm_display_status_syntax Syntax:
\code
{
    "subscribe": boolean
}
\endcode

\param subscribe Subscribe for updates.

\subsection com_palm_display_status_returns Returns:
\code
{
    "returnValue": boolean,
    "event": string,
    "state": string,
    "subscribed": boolean
}
\endcode

\param returnValue Indicates if the call was succesful.
\param event Event that triggered this call.
\param state State of the display. "Undefined", "dimmed", "off" or "on".
\param subscribed True if subscribed for updates.

Example response for a succesful call:
\code
{
    "returnValue": true,
    "event": "request",
    "state": "off",
    "subscribed": false
}
\endcode
*/

/*!
\page com_palm_display_control
\n
\section com_palm_display_control_status status

\e Private.

com.palm.display/control/status

Get the status of the display.

\subsection com_palm_display_control_status_syntax Syntax:
\code
{
    "subscribe": boolean
}
\endcode

\param subscribe Subscribe for updates.

\subsection com_palm_display_control_status_returns Returns:
\code
{
    "returnValue": boolean,
    "event": string,
    "state": string,
    "timeout": int,
    "blockDisplay": string,
    "active": boolean,
    "subscribed": boolean
}
\endcode

\param returnValue Indicates if the call was succesful.
\param event Event that triggered this call.
\param state State of the display. "Undefined", "dimmed", "off" or "on".
\param timeout Time in seconds after which the display turns off.
\param blockDisplay True if display \e state is blocked from changes.
\param active True if the timer that shuts down the display after \e timeout seconds of inactivity is running.
\param subscribed True if subscribed for updates.

\subsection com_palm_display_control_status_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.display/control/status '{}'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "event": "request",
    "state": "off",
    "timeout": 120,
    "blockDisplay": "false",
    "active": false,
    "subscribed": false
}
\endcode
*/
bool DisplayManager::controlStatus(LSHandle *sh, LSMessage *message, void *ctx)
{
    SUBSCRIBE_SCHEMA_RETURN(sh, message);

    LSError lserror;
    LSErrorInit(&lserror);
    bool result = true;

    DisplayManager *dm = ((DisplayCallbackCtx_t *)ctx)->ctx;
    bool isPublic = ((DisplayCallbackCtx_t *)ctx)->isPublic;

    bool subscribed = false;
    const char *state = "undefined";

    result = LSSubscriptionProcess (sh, message, &subscribed, &lserror);
    if(!result)
    {
        LSErrorFree (&lserror);
        result = true;
        subscribed = false;
    }

    if (DisplayStateDim == dm->currentState())
        state = "dimmed";
    else if (DisplayStateOff == dm->currentState())
        state = "off";
    else if (DisplayStateOn == dm->currentState()
            || DisplayStateOnLocked == dm->currentState()
            || DisplayStateOnPuck == dm->currentState()
            || DisplayStateDockMode == dm->currentState()
            || DisplayStateOffOnCall == dm->currentState())
        state = "on";
    else
        result = false;

    gchar *status = NULL;

    // We expose less information on the public bus than the private bus
    if (isPublic)
    {
        status = g_strdup_printf ("{\"returnValue\":true,\"event\":\"request\",\"state\":\"%s\",\"subscribed\":%s}", state, subscribed ? "true" : "false");
    }
    else
    {
        status = g_strdup_printf ("{\"returnValue\":true,\"event\":\"request\",\"state\":\"%s\",\"timeout\":%i,\"blockDisplay\":\"%s\",\"active\":%s,\"subscribed\":%s}", state, (dm->m_totalTimeout) / 1000, dm->m_dnast > 0 ? "true" : "false" , dm->m_activity->running() ? "true": "false", subscribed ? "true" : "false");
    }

    if (NULL != status)
        result = LSMessageReply(sh, message, status, &lserror);

    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    if (NULL != status)
        g_free(status);

    return true;
}

DisplayManager::~DisplayManager()
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    result = LSUnregister(m_publicService, &lserror);
    if (!result)
    {
        g_message ("%s: failed at %s with message %s", __FUNCTION__, lserror.func, lserror.message);
        LSErrorFree(&lserror);
    }

    result = LSUnregister(m_service, &lserror);
    if (!result)
    {
        g_message ("%s: failed at %s with message %s", __FUNCTION__, lserror.func, lserror.message);
        LSErrorFree(&lserror);
    }
}

void DisplayManager::slotEmergencyMode (bool enable)
{
    if (enable)
        updateState (DISPLAY_EVENT_ENTER_EMERGENCY_MODE);
    else
        updateState (DISPLAY_EVENT_EXIT_EMERGENCY_MODE);
}

void DisplayManager::slotAlsEnabled (bool enable)
{
	g_message ("%s: ALS being %s", __PRETTY_FUNCTION__, enable ? "enabled" : "disabled");
	if (enable) {
		if (currentState() == DisplayStateOn
				|| currentState() == DisplayStateOnLocked
				|| currentState() == DisplayStateOnPuck
				|| currentState() == DisplayStateDockMode)
			m_als->start();
	}
	else {
		m_als->stop();
		updateBrightness();
	}
}

void DisplayManager::slotBootFinished()
{
	m_bootFinished = true;
	if (currentState() == DisplayStateOn
			|| currentState() == DisplayStateOnLocked
			|| currentState() == DisplayStateOnPuck
			|| currentState() == DisplayStateDockMode
			|| (currentState() == DisplayStateDim && !Settings::LunaSettings()->turnOffAccelWhenDimmed))
        orientationSensorOn();

    // Update compass with correct lat/long
    requestCurrentLocation();
}

void DisplayManager::slotShowIME()
{
	setTouchpanelMode(true);
}

void DisplayManager::slotHideIME()
{
	if (!m_activeTouchpanel)
		setTouchpanelMode(false);
}

void DisplayManager::slotBluetoothKeyboardActive(bool active)
{
    // when the bluetooth keyboard connects to the device,
    // also attempt to unlock the device
    if (active)
        unlock();
}

bool DisplayManager::updateState (int eventType)
{
    switch (eventType)
    {
        case DISPLAY_EVENT_SLIDER_UNLOCKED:
            {
                m_sliderOpen = true;
                m_currentState->handleEvent (DisplayEventSliderOpen);
            }
            break;
        case DISPLAY_EVENT_SLIDER_LOCKED:
            {
                m_sliderOpen = false;
                m_currentState->handleEvent (DisplayEventSliderClose);
                updateBrightness();
            }
            break;
        case DISPLAY_EVENT_POWER_BUTTON_UP:
            {
                if (m_power->running())
                    m_power->stop ();

                // Disabling this for now in Dartfish, as we for now are using Emergency Mode for the Full Screen FLASH player
//                if (SystemUiController::instance()->isInEmergencyMode ()) {
//                    g_warning ("%s: in emergency mode, ignoring power button up", __PRETTY_FUNCTION__);
//                    break;
//                }

                if (m_blockedPowerKey > 0)
                {
                    // special mode. Do not do anything. Let the subscirbers know bout powerbutton but drop it.
                    g_warning ("%s: block power key is set, forwarding this to subscribers but ignoring it internally.", __PRETTY_FUNCTION__);
                    LSError lserror;
                    LSErrorInit(&lserror);
                    bool result = LSSubscriptionReply (m_service, POWER_KEY_BLOCK_SUBSCRIPTION_KEY, "{\"powerKey\":\"released\"}", &lserror);
                    if (!result)
                    {
                        g_message ("%s: failed at %s with message %s", __FUNCTION__, lserror.func, lserror.message);
                        LSErrorFree(&lserror);
                    }
                }
                else
                {
                    // disable power key if the user is on a call on the puck
                    // this is so that the user never sees a lock screen when on a call
                    if (!(m_onCall && currentState() == DisplayStateOnPuck))
                    {
                        g_message ("%s: power key press", __PRETTY_FUNCTION__);
                        m_currentState->handleEvent (DisplayEventPowerKeyPress);
                    }
                }
            }
            break;
        case DISPLAY_EVENT_POWER_BUTTON_DOWN:
            {
                m_dropPowerKey = false;
                // start the timer
		if ((currentState() == DisplayStateOn 
					|| currentState() == DisplayStateOnPuck
					|| currentState() == DisplayStateDockMode
					|| currentState() == DisplayStateDim)
				&& !m_power->running()) {
                    m_power->start (m_powerKeyTimeout);
		}
            }
            break;
        case DISPLAY_EVENT_INDUCTIVE_CHARGER_DISCONNECTED:
            {
                g_message ("%s: off the puck", __PRETTY_FUNCTION__);
                m_currentState->handleEvent (DisplayEventOffPuck);
		Q_EMIT signalPuckConnected (false);
            }
            break;
        case DISPLAY_EVENT_INDUCTIVE_CHARGER_CONNECTED:
            {
                g_message ("%s: on the puck", __PRETTY_FUNCTION__);
		Q_EMIT signalPuckConnected (true);
                m_currentState->handleEvent (DisplayEventOnPuck);
            }
            break;
        case DISPLAY_EVENT_USB_CHARGER_DISCONNECTED:
            {
                if (m_onWhenConnected)
                {
                    gchar *report = g_strdup_printf ("%s-dm-usb-charger-connected", __FUNCTION__);
                    popDNAST (report);
                    g_free (report);
                }
                m_currentState->handleEvent (DisplayEventUsbOut);
            }
            break;
        case DISPLAY_EVENT_USB_CHARGER_CONNECTED:
            {
                if (m_onWhenConnected)
                {
                    gchar *report = g_strdup_printf ("%s-dm-usb-charger-connected", __FUNCTION__);
                    pushDNAST (report);
                    g_free (report);
                }
                m_currentState->handleEvent (DisplayEventUsbIn);
            }
            break;
        case DISPLAY_EVENT_ALS_REGION_CHANGED:
            {
                m_currentState->handleEvent (DisplayEventAlsChange);
            }
            break;
        case DISPLAY_EVENT_ENTER_EMERGENCY_MODE:
            {
		if (SystemUiController::instance()->isInEmergencyMode()) {
		    g_message ("%s: entering emergency mode, pushing DNAST\n", __PRETTY_FUNCTION__);
		    gchar *report = g_strdup_printf ("%s-dm-in-emergency-mode", __FUNCTION__);
		    pushDNAST (report);
		    g_free (report);
		}
            }
            break;
        case DISPLAY_EVENT_EXIT_EMERGENCY_MODE:
            {
		if (!SystemUiController::instance()->isInEmergencyMode()) {
		    g_message ("%s: exiting emergency mode, popping DNAST\n", __PRETTY_FUNCTION__);
		    gchar *report = g_strdup_printf ("%s-dm-in-emergency-mode", __FUNCTION__);
		    popDNAST (report);
		    g_free (report);
		}
            }
            break;
        case DISPLAY_EVENT_PROXIMITY_ON:
            {
                m_currentState->handleEvent (DisplayEventProximityOn);
            }
            break;
        case DISPLAY_EVENT_PROXIMITY_OFF:
            {
                m_currentState->handleEvent (DisplayEventProximityOff);
            }
            break;
        case DISPLAY_EVENT_ON_CALL:
            {
                m_currentState->handleEvent (DisplayEventOnCall);
            }
            break;
        case DISPLAY_EVENT_OFF_CALL:
            {
                m_currentState->handleEvent (DisplayEventOffCall);
            }
            break;
        case DISPLAY_EVENT_HOME_BUTTON_UP:
            {
                if(HostBase::instance()->homeButtonWakesUpScreen())
                {
                    m_lastEvent = Time::curTimeMs();
                }
                m_currentState->handleEvent (DisplayEventHomeKeyPress);
            }
            break;
        default:
            g_warning("%s: unhandled eventType (%i)", __PRETTY_FUNCTION__, eventType);
    }

    return true;
}

bool DisplayManager::activity()
{
    uint32_t now = Time::curTimeMs();
    g_debug("%s: now=%i last=%i diff=%i", __PRETTY_FUNCTION__, now, m_lastEvent, now - m_lastEvent);

    if (now <  m_activityTimeout + m_lastEvent)
    {
        g_debug("%s: restart the timer in %i", __PRETTY_FUNCTION__, m_activityTimeout + m_lastEvent - now);
        m_activity->start (m_activityTimeout + m_lastEvent - now);
    }
    else
    {
        notifySubscribers (DISPLAY_EVENT_INACTIVE);
    }

    return false;
}

bool DisplayManager::slider()
{
    if (m_slidingNow)
    {
        g_warning("%s: optical sensor timeout", __FUNCTION__);
        m_slidingNow = NOT_SLIDING;
    }

    return false;
}


void DisplayManager::backlightOn (int displayBrightness, int keyBrightness, bool als)
{

    // Ignore als for now (led-controller module needs t
    LedControl* lcKeypadAndDisplay = HostBase::instance()->getLedControlKeypadAndDisplay();
    if (NULL == lcKeypadAndDisplay) g_message("%s: LedControlKeypad returns NULL", __PRETTY_FUNCTION__);

    if (!m_touchpanelIsOn)
    {
        if (lcKeypadAndDisplay) lcKeypadAndDisplay->setBrightness(keyBrightness, displayBrightness,
                &DisplayManager::backlightOnCallback, this);
    }
    else
    {
        if (lcKeypadAndDisplay) lcKeypadAndDisplay->setBrightness(keyBrightness, displayBrightness, NULL, NULL);
    }

}

void DisplayManager::backlightOnCallback(void *ctx)
{
	DisplayManager *dm = (DisplayManager *)ctx;
	dm->m_backlightIsOn = true;
	g_message("%s: setting m_backlightIsOn to true", __PRETTY_FUNCTION__);

	if (dm->m_displayOn)
		dm->touchPanelOn();
	else
		g_warning("%s: display is supposed to be off, not turning on touchpanel", __PRETTY_FUNCTION__);

        if (dm->m_displayOn)
                dm->touchPanelOn();
        else
                g_warning("%s: display is supposed to be off, not turning on touchpanel", __PRETTY_FUNCTION__);

        // turn vsync on as soon as the display is on to avoid tearing
        changeVsyncControl(true);
}

void DisplayManager::setActiveTouchpanel (bool enable)
{
	m_activeTouchpanel = enable;
	setTouchpanelMode (enable);
}


void DisplayManager::setTouchpanelMode (bool active)
{
#if defined(HAS_NYX)
    LSError lserror;
    LSErrorInit (&lserror);
    nyx_error_t err;

    InputControl* ic = HostBase::instance()->getInputControlTouchpanel();
    if (NULL != ic)
    {
	    if (active) {
		    g_message ("%s: Setting nyx touchpanel mode to %d", __PRETTY_FUNCTION__, NYX_TOUCHPANEL_MODE_VIRTUAL_KEYBOARD);
		    err = nyx_touchpanel_set_mode (ic->getHandle(), NYX_TOUCHPANEL_MODE_VIRTUAL_KEYBOARD);
	    }
	    else  {
		    g_message ("%s: Setting nyx touchpanel mode to %d", __PRETTY_FUNCTION__, NYX_TOUCHPANEL_MODE_DEFAULT);
		    err = nyx_touchpanel_set_mode (ic->getHandle(), NYX_TOUCHPANEL_MODE_DEFAULT);
	    }
       if (err) {
           g_warning("%s: failed to set Touchpanel mode !", __PRETTY_FUNCTION__);
       }
    }
#endif
}


void DisplayManager::setAlsDisabled (bool disable)
{
	g_message ("%s: %s Als", __PRETTY_FUNCTION__, disable ? "Disabling" : "Enabling");
	m_alsDisabled = disable;
	slotAlsEnabled (!disable);
}


bool DisplayManager::touchPanelOn ()
{
#if defined(HAS_NYX)
    InputControl* ic = HostBase::instance()->getInputControlTouchpanel();
    if (NULL != ic)
    {
        m_touchpanelIsOn = true;
        return ic->on();
    }
#endif
    return true;
}

bool DisplayManager::touchPanelOff ()
{
#if defined(HAS_NYX)
    InputControl* ic = HostBase::instance()->getInputControlTouchpanel();
    if (NULL != ic)
    {
        m_touchpanelIsOn = false;
        ic->off();
        touchPanelOffCallback (NULL, NULL, this);
    }
#endif
    return true;
}

bool DisplayManager::touchPanelOffCallback (LSHandle *sh, LSMessage *message, void *ctx)
{
    EMPTY_SCHEMA_RETURN(sh, message);

    DisplayManager *dm = (DisplayManager *)ctx;

    if (!dm->m_displayOn)
	dm->backlightOff();
    else
	g_warning ("%s: display is supposed to be on, not turning off backlight", __PRETTY_FUNCTION__);

    return true;
}

void DisplayManager::backlightOff()
{
        // turn vsync off before turning the display off to avoid having the driver hang at swapBuffers
    changeVsyncControl(false);

    LedControl* lcKeypadAndDisplay = HostBase::instance()->getLedControlKeypadAndDisplay();
    if (NULL == lcKeypadAndDisplay) g_message("%s: LedControlKeypadAndDisplay returns NULL", __PRETTY_FUNCTION__);

    if (lcKeypadAndDisplay) lcKeypadAndDisplay->setBrightness(0, -1, &DisplayManager::backlightOffCallback, this);

}

void DisplayManager::backlightOffCallback (void *ctx)
{
    g_message("%s setting m_backlightIsOn to false", __PRETTY_FUNCTION__);
    DisplayManager *dm = (DisplayManager *)ctx;
    dm->m_backlightIsOn = false;
}

bool DisplayManager::orientationSensorOn ()
{
	if (!m_bootFinished)
		return true;

    HostBase::instance()->OrientationSensorOn(true);

	return true;
}

bool DisplayManager::orientationSensorOff ()
{
    HostBase::instance()->OrientationSensorOn(false);
    return true;
}

void DisplayManager::getBearingInfo(double& latitude, double& longitude)
{
	latitude = DisplayManager::s_currentLatitude;
	longitude = DisplayManager::s_currentLongitude;
}

void DisplayManager::setBearingInfo(double latitude, double longitude)
{
	DisplayManager::s_currentLatitude = latitude;
	DisplayManager::s_currentLongitude = longitude;
}

bool DisplayManager::updateCompassBearingInfo(LSHandle* sh, LSMessage* message, void* ctx)
{
	LSError lserror;
	LSErrorInit(&lserror);
	bool result = false;

	json_object* label = 0;
	json_object* root = 0;
	int errCode = -1;
	double latitude = 0.0, longitude = 0.0, currentLatitude = 0.0, currentLongitude = 0.0;

    // {"errorCode": integer, "latitude": double, "longitude": double}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_3(REQUIRED(errorCode, integer), REQUIRED(latitude, double), REQUIRED(longitude, double)));

	// if we dont have a valid message just return
	if(!message) {
		return false;
	}

	const char* str = LSMessageGetPayload(message);

	if (!str)
		goto error;

	root = json_tokener_parse(str);
	if (!root || is_error(root)) {
		goto error;
	}

	// Try to parse the message - These values are set based on the WIKI

	// get the errorCode
	label = json_object_object_get(root, "errorCode");
	if (label && !is_error (label)) {
		errCode = json_object_get_int (label);
		if(0 != errCode) {
			goto error;
		}
	}

	// get latitude
	label = json_object_object_get(root, "latitude");
	if (label && !is_error (label)) {
		latitude = json_object_get_double (label);
	}
	else {
		goto error;
	}

	// get longitude
	label = json_object_object_get(root, "longitude");
	if (label && !is_error (label)) {
		longitude = json_object_get_double (label);
	}
	else {
		goto error;
	}

	// Get the current bearings
	DisplayManager::instance()->getBearingInfo(currentLatitude, currentLongitude);

	// Ensure that we have valid data - then call NYX and update them with the new values
	if(currentLatitude != latitude || currentLongitude != longitude)
    {
        result = DisplayManager::instance()->updateNyxWithLocation(latitude, longitude);
        // If we updated NYX correctly, update our state as well
        if(result)
            DisplayManager::instance()->setBearingInfo(latitude, longitude);
    }

error:
	return result;
}

void DisplayManager::slotAirplaneModeChanged(bool change)
{
    // If we are coming out of airplane mode then update the compass by requesting a new location
    if(false == change)
    {
        requestCurrentLocation();
    }
}

void DisplayManager::requestCurrentLocation()
{
	LSError lserror;
	LSErrorInit(&lserror);
	bool result;

	static const int kAccuracy = 2;
	static const int kResponseTime = 3;

	gchar *compassInf = g_strdup_printf (JSON_LBS_CURRENTLOCATIONINF, kAccuracy, kResponseTime);

	// Now request the current location from LBS
	result = LSCallOneReply (m_service, URI_LBS_GETCURRENTLOC, compassInf, &DisplayManager::updateCompassBearingInfo, this, NULL, &lserror);
	if(!result) {
		LSErrorFree(&lserror);
	}
}

bool DisplayManager::updateNyxWithLocation(double latitude, double longitude)
{
	bool result = false;

    NYXBearingSensorConnector* pBearingSensor = static_cast<NYXBearingSensorConnector *> (NYXConnectorBase::getSensor(NYXConnectorBase::SensorBearing));
    if (pBearingSensor)
    {
        result = pBearingSensor->setLocation(latitude, longitude);
        delete pBearingSensor;
    }

	return result;
}

bool DisplayManager::alertTimerCallback ()
{
    alert (DISPLAY_ALERT_GENERIC_DEACTIVATED);
    return false;
}

bool DisplayManager::alert (int state)
{
    g_debug ("%s: got the following message: %i", __FUNCTION__, state);
    switch (state)
    {
        case DISPLAY_BANNER_ACTIVATED:
        case DISPLAY_ALERT_GENERIC_ACTIVATED:
            if (currentState() != DisplayStateOn
                    && currentState() != DisplayStateOnLocked
                    && currentState() != DisplayStateOnPuck
		    && currentState() != DisplayStateDockMode)
            {
                m_alertTimer->start (ALERT_TIMEOUT);
                m_alertState = currentState();
                g_message ("%s: calling on due to alert %d", __PRETTY_FUNCTION__, state);
                return on ();
            }
            break;
        case DISPLAY_ALERT_PHONECALL_ACTIVATED:
            if (currentState() != DisplayStateOn
                    && currentState() != DisplayStateOnLocked
                    && currentState() != DisplayStateOnPuck)
            {
                m_alertTimer->start (ALERT_TIMEOUT);
                m_alertState = currentState();
                g_message ("%s: calling on due to activated phonecall %d", __PRETTY_FUNCTION__, state);
                m_currentState->handleEvent (DisplayEventIncomingCall);
            }
            break;
        case DISPLAY_ALERT_PHONECALL_DEACTIVATED:
            if (!m_onCall) {
                if (m_alertState == DisplayStateOff) {
                    g_message ("%s: calling off due to deactivated phonecall %d", __PRETTY_FUNCTION__, state);
                    off ();
                }
                else if (m_alertState == DisplayStateDim) {
                    g_message ("%s: calling dim due to deactivated phonecall %d", __PRETTY_FUNCTION__, state);
                    dim ();
                }
                m_currentState->handleEvent (DisplayEventIncomingCallDone);
            }
            break;
        case DISPLAY_ALERT_GENERIC_DEACTIVATED:
        case DISPLAY_BANNER_DEACTIVATED:
            if (!m_onCall) {
                if (m_alertState == DisplayStateOff) {
                    g_message ("%s: calling off due to alert %d", __PRETTY_FUNCTION__, state);
                    off ();
                }
                else if (m_alertState == DisplayStateDim) {
                    g_message ("%s: calling dim due to alert %d", __PRETTY_FUNCTION__, state);
                    dim();
                }
            }
            break;
        case DISPLAY_ALERT_CANCEL:
            m_alertState = DISPLAY_UNDEFINED;
            if (m_alertTimer->running ())
                m_alertTimer->stop ();
            break;
        default:
            break;
    }

    return true;
}

void DisplayManager::lock()
{
    m_currentState->handleEvent(DisplayEventLockScreen);
}

void DisplayManager::unlock()
{
    m_currentState->handleEvent(DisplayEventUnlockScreen);
}


bool DisplayManager::on (sptr<Event> event)
{
    LSError lserror;
    LSErrorInit(&lserror);

    m_lastEvent = Time::curTimeMs();
    if (currentState() == DisplayStateOn
	    || currentState() == DisplayStateOnLocked
            || currentState() == DisplayStateOnPuck)
	return true;

    m_currentState->handleEvent (DisplayEventApiOn, event);
    return true;

}

bool DisplayManager::off (sptr<Event> event)
{

    // Disabling this for now in Dartfish, as we for now are using Emergency Mode for the Full Screen FLASH player
//    // in emergency mode do not allow the off state
//    if (SystemUiController::instance()->isInEmergencyMode ())
//        return true;

    m_lastEvent = Time::curTimeMs();

    if (isDNAST())
        return true;

    if (currentState() == DisplayStateOff
	    || currentState() == DisplayStateOffOnCall)
	return true;

    m_currentState->handleEvent (DisplayEventApiOff, event);

    return true;
}

bool DisplayManager::dim (sptr<Event> event)
{
    // in emergency mode do not allow the dim state
    if (SystemUiController::instance()->isInEmergencyMode ())
        return true;

    m_lastEvent = Time::curTimeMs() - m_dimTimeout;
    if (currentState() == DisplayStateDim)
        return true;

    m_currentState->handleEvent (DisplayEventApiDim, event);

    return true;
}

bool DisplayManager::dock()
{
    LSError lserror;
    LSErrorInit(&lserror);

    m_lastEvent = Time::curTimeMs();
    if (currentState() == DisplayStateDockMode)
	return true;

    m_currentState->handleEvent (DisplayEventApiDock, NULL);
    return true;

}


bool DisplayManager::undock()
{
    LSError lserror;
    LSErrorInit(&lserror);
    g_debug ("Undock api called");

    m_lastEvent = Time::curTimeMs();
    if (currentState() != DisplayStateDockMode) {
    	g_debug ("Display not in dock mode, so returning true");
    	return true;
    }

    g_debug ("Display in dock mode, sending ApiUndock event") ;
    m_currentState->handleEvent (DisplayEventApiUndock, NULL);
    return true;

}


bool DisplayManager::power()
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    result = LSSignalSend(m_service, URI_DISPLAY_POWER_KEY_SIGNAL, JSON_DISPLAY_POWER_KEY_SIGNAL, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }
    m_dropPowerKey = true;

    return false;
}

bool DisplayManager::handleEvent(QEvent *event)
{

    static int sFirstDroppedPenMoveX = -1;
    static int sFirstDroppedPenMoveY = -1;

	QKeyEvent *keyEvent = NULL;
	QMouseEvent *mouseEvent = NULL;
	if (QEvent::KeyPress == event->type() || QEvent::KeyRelease == event->type()) {
		keyEvent = static_cast<QKeyEvent*>(event);
	} else if (QEvent::MouseButtonPress == event->type() || QEvent::MouseButtonRelease == event->type()
			|| QEvent::MouseMove == event->type()) {
		mouseEvent = static_cast<QMouseEvent*>(event);
	}

    if (m_slidingNow == SLIDING_WAIT && m_slidingStart + SLIDER_MINTIME < Time::curTimeMs()) {
		m_slidingNow = NOT_SLIDING;
	}

	if (!m_service) {
		return false;
	}

    // check for the brick case
	if (SystemService::instance()->brickMode() || !SystemUiController::instance()->bootFinished()
			|| WindowServer::instance()->progressRunning()) {
		return false;
	}

    // if the screen was turned on by the alert or banner, make sure that
	// the state is reset when there is a key press or touch.
	if ((m_alertState != DISPLAY_UNDEFINED) && (event->type() == QEvent::KeyPress || event->type() == QEvent::MouseButtonPress)) {
		alert(DISPLAY_ALERT_CANCEL);
	}

    // pass the ringer and banner messages, no effect on display state at all
    // also the "media" keys from headset and avrcp
    if (keyEvent && !(keyEvent->modifiers() & Qt::ExternalKeyboardModifier) &&
			(Qt::Key_Ringer == keyEvent->key() ||
			Qt::Key_VolumeDown == keyEvent->key() ||
			Qt::Key_VolumeUp == keyEvent->key() ||
			Qt::Key_VolumeMute == keyEvent->key() ||
			Qt::Key_MediaPlay == keyEvent->key() ||
			Qt::Key_MediaPause == keyEvent->key() ||
			Qt::Key_MediaTogglePlayPause == keyEvent->key() ||
			Qt::Key_MediaStop == keyEvent->key() ||
			Qt::Key_MediaNext == keyEvent->key() ||
			Qt::Key_MediaPrevious == keyEvent->key() ||
			Qt::Key_MediaRepeatAll == keyEvent->key() ||
			Qt::Key_MediaRepeatTrack == keyEvent->key() ||
			Qt::Key_MediaRepeatNone == keyEvent->key() ||
			Qt::Key_MediaShuffleOn == keyEvent->key() ||
			Qt::Key_MediaShuffleOff == keyEvent->key() || 
			Qt::Key_HeadsetButton == keyEvent->key() ||
			Qt::Key_Headset == keyEvent->key() ||
			Qt::Key_HeadsetMic == keyEvent->key())) {
        return false;
    }

    // Always allow a mouse cancel event to go through
    if (mouseEvent && mouseEvent->type() == QEvent::MouseButtonRelease && mouseEvent->canceled()) {
        g_debug("%s: Allowing canceled event at %d, %d through", __PRETTY_FUNCTION__, mouseEvent->x(), mouseEvent->y());
        return false;
    }
    if (event->type() == QEvent::TouchEnd)
    {
        g_debug("%s: Allowing touch-end event through", __PRETTY_FUNCTION__);
        return false;
    }

    if (alsEventType == event->type()) {
    	AlsEvent *alsEvent = static_cast<AlsEvent*>(event);
		if (m_als->update(alsEvent->getLightIntensity())) {
			updateState(DISPLAY_EVENT_ALS_REGION_CHANGED);
		}
		return true;
	}

	if (keyEvent && Qt::Key_Optical == keyEvent->key()) {
		if (keyEvent->type() == QEvent::KeyRelease) {
            g_debug("%s: optical up", __PRETTY_FUNCTION__);
			m_slidingNow = SLIDING_NOW;
			m_slidingStart = Time::curTimeMs();
			
			// fire a timer to cancel m_slidingNow
			m_slider->start(SLIDER_TIMEOUT);

			if (m_metaId) {
				m_blockId = m_metaId;
			}
		} else if (keyEvent->type() == QEvent::KeyPress) {
			// if slider timer is running stop it!
			if (m_slider->running())
				m_slider->stop();

			if (m_slidingNow) {
				g_debug("%s: optical down", __FUNCTION__);
				if (m_slidingStart + SLIDER_MINTIME < Time::curTimeMs()) {
					m_slidingNow = NOT_SLIDING;
				} else {
					m_slidingNow = SLIDING_WAIT;
				}
			}
		}
		return true;
	}

    // check for the slider key
    if (keyEvent && Qt::Key_Slider == keyEvent->key()) {
		if (keyEvent->type() == QEvent::KeyRelease) {
			updateState(DISPLAY_EVENT_SLIDER_UNLOCKED);
		} else if (keyEvent->type() == QEvent::KeyPress) {
			updateState(DISPLAY_EVENT_SLIDER_LOCKED);
		}
		return false;
	}

    if (event->type() == proximityEventType) {
    	ProximityEvent *proximityEvent = static_cast<ProximityEvent*>(event);
        g_debug ("%s: Proximity Event %i", __PRETTY_FUNCTION__, proximityEvent->isObjectDetected());
        if (m_proximityCount > 0 && proximityEvent->isObjectDetected() && !(m_chargerConnected & CHARGER_INDUCTIVE)) {
            g_debug ("%s: Proximity Activated, display going off", __PRETTY_FUNCTION__);
            m_proximityActivated = true;

            updateState (DISPLAY_EVENT_PROXIMITY_ON);

        }
        if (!proximityEvent->isObjectDetected()) {
            g_debug ("%s: Proximity Deactivated, display going on", __PRETTY_FUNCTION__);
            m_proximityActivated = false;

            updateState (DISPLAY_EVENT_PROXIMITY_OFF);

            if (m_proximityCount <= 0) {
                proximityOff ();
            }
        }
    }

    if (m_proximityActivated && m_onCall) {
        // let all the key events through
        if (keyEvent) {
            return false;
        }
    }

    // this key was used to wake up the device, drop
    // all the events until
    if(m_drop_key && keyEvent) {
        if (keyEvent->type() == QEvent::KeyRelease) {
            m_drop_key = false;
        }
        return true;
    }

    if (event->type() == QEvent::MouseButtonPress || (keyEvent && keyEvent->type() == QEvent::KeyRelease && keyEvent->key() == Qt::Key_CoreNavi_QuickLaunch)) {
        m_penDown = true;
//        if(m_blockId && event->id > m_blockId) {
//            m_blockId = 0;
//        }
    } else if (event->type() == QEvent::MouseButtonRelease) {
        m_penDown = false;
    }

    // if the display is off drop the following events
    if (currentState() == DisplayStateOff || currentState() == DisplayStateOffOnCall) {
	    // drop the gestures
	    if (keyEvent && (keyEvent->isGestureKey() && Qt::Key_CoreNavi_Home != keyEvent->key()) ) {
		    return true;
	    }
    }

    if (keyEvent && keyEvent->key() == Qt::Key_CoreNavi_Home) {
	    // if power key is down and power alert timer is running, the home down key should stop the timer 
	    m_homeKeyDown = (keyEvent->type() == QEvent::KeyPress);
	    if (m_homeKeyDown && m_power->running()) {
		    g_message ("%s: Detected home key down when power key is being held, disable power key timer", __PRETTY_FUNCTION__);
		    m_power->stop();
		    m_homeKeyDown = false;
	    }

	    // drop core navi home button if display is off, slider is closed and setting is to not wake the screen
	    if (m_hasSlider && !m_sliderOpen
			    && currentState() == DisplayStateOff
			    && currentState() == DisplayStateOffOnCall
			    && currentState() == DisplayStateOffSuspended
			    && !HostBase::instance()->homeButtonWakesUpScreen()) 
	    {
		    return true;
	    }

	    if (keyEvent->type() == QEvent::KeyRelease) {
		    updateState(DISPLAY_EVENT_HOME_BUTTON_UP);
	    }
	    return false;
    }

    if (keyEvent && keyEvent->key() == Qt::Key_Power) {

		if (keyEvent->modifiers() & Qt::GroupSwitchModifier) {
			if (keyEvent->type() == QEvent::KeyPress) {
				m_dropPowerKey = true;
			}
			else {
				if (m_power->running())
					m_power->stop();
			}
		}
		else if (keyEvent->type() == QEvent::KeyPress && m_homeKeyDown) {
			g_message ("%s: Detected power down when home down, stopping power key timer", __PRETTY_FUNCTION__);
			if (m_power->running())
				m_power->stop();
			m_homeKeyDown = false;
		}
		else {
			if (keyEvent->type() == QEvent::KeyPress) {
				updateState(DISPLAY_EVENT_POWER_BUTTON_DOWN);
			} else if (!m_dropPowerKey) {
				updateState(DISPLAY_EVENT_POWER_BUTTON_UP);
			}
		}

		// tell the window manager to drop the events
		return false;
	}

    // if key down happens when the screen is off or dimmed let's wake up the display, but let's also
	// eat all the events until the next key up.
	if (keyEvent && keyEvent->type() == QEvent::KeyPress
			&& !(keyEvent->key() == Qt::Key_unknown || keyEvent->key() == 0)
			&& currentState() != DisplayStateOn
			&& currentState() != DisplayStateOnPuck
			&& currentState() != DisplayStateDockMode) {
		if (keyEvent->modifiers() & Qt::ExternalKeyboardModifier)
		{
			g_message("%s: sending user activity external input event on key down", __PRETTY_FUNCTION__);
			m_currentState->handleEvent(DisplayEventUserActivityExternalInput);
		}
		else if (m_hasSlider || currentState() != DisplayStateOff || keyEvent->key() == Qt::Key_Power)
		{
			g_message("%s: sending user activity event on key down", __PRETTY_FUNCTION__);
			m_currentState->handleEvent(DisplayEventUserActivity);
		}

		m_drop_key = true;
		return true;
	}

    if (DisplayStateOff == currentState() || DisplayStateOffOnCall == currentState()) {
		if (event->type() == QEvent::MouseButtonPress) {
			m_drop_pen = true;
		}
		return true;
	}

    if (keyEvent || mouseEvent
    		/*
            event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease ||
            event->type() == QEvent::MouseMove || event->type() == Event::PenFlick ||
            event->type() == Event::PenPressAndHold || event->type() == Event::PenCancel ||
            event->type() == Event::GestureStart || event->type() == Event::GestureChange ||
            event->type() == Event::GestureEnd */) {
        m_lastEvent = Time::curTimeMs();
        if (currentState() == DisplayStateDim) {
			g_message("%s: sending user activity event", __PRETTY_FUNCTION__);
			m_currentState->handleEvent(DisplayEventUserActivity);

			if (mouseEvent && mouseEvent->type() == QEvent::MouseButtonPress) {
				m_drop_pen = true;
				m_allow_move = true;
				sFirstDroppedPenMoveX = mouseEvent->x();
				sFirstDroppedPenMoveY = mouseEvent->y();
				return true;
			}
		}

		if ((currentState() == DisplayStateOn || currentState() == DisplayStateOnPuck) && !m_activity->running()) {
			m_activity->start(m_activityTimeout);
			notifySubscribers(DISPLAY_EVENT_ACTIVE);
		}
    }

    if (keyEvent && (keyEvent->key() == Qt::Key_MonBrightnessUp || keyEvent->key() == Qt::Key_MonBrightnessDown)) {

        static bool sHandledAutoRepeat = false;
        if ((keyEvent->type() == QEvent::KeyRelease && !sHandledAutoRepeat) || keyEvent->isAutoRepeat()) {
            int adjustment = (keyEvent->key() == Qt::Key_MonBrightnessUp ? 10 : -10);
            // tighter bounds for brightness adjustment through keys to allow for the user
            // to distinguish "on" vs. "dimm"
            int newBrightness = qBound(10, m_maxBrightness + adjustment, 100);
            setMaximumBrightness(newBrightness);
        }
        sHandledAutoRepeat = keyEvent->isAutoRepeat();
        return true;        
    }

    return false;
}


bool DisplayManager::isOn() const
{
    return (currentState() == DisplayStateOn
            || currentState() == DisplayStateOnLocked
            || currentState() == DisplayStateOnPuck
	    || currentState() == DisplayStateDockMode);
}


void DisplayManager::changeDisplayState (DisplayState newState, DisplayState oldState, DisplayEvent displayEvent, sptr<Event> event)
{
    m_lastEvent = Time::curTimeMs();
    m_currentState = m_displayStates[newState];

    switch (newState) {
        case DisplayStateOff:
        case DisplayStateOffOnCall:
        case DisplayStateOn:
        case DisplayStateOnLocked:
        case DisplayStateDim:
        case DisplayStateOnPuck:
        case DisplayStateDockMode:
        case DisplayStateOffSuspended:
            m_currentState->enter (oldState, displayEvent, event);
            break;
        default:
            g_warning("%s: unhandled state %d", __PRETTY_FUNCTION__, newState);
            break;
    }
}

void DisplayManager::emitDisplayStateChange (int displaySignal)
{
    switch (displaySignal) {
	case DISPLAY_SIGNAL_OFF:
	    Q_EMIT signalDisplayStateChange (DISPLAY_SIGNAL_OFF);
	    break;
	case DISPLAY_SIGNAL_ON:
	    Q_EMIT signalDisplayStateChange (DISPLAY_SIGNAL_ON);
	    break;
	case DISPLAY_SIGNAL_DIM:
	    Q_EMIT signalDisplayStateChange (DISPLAY_SIGNAL_DIM);
	    break;
	case DISPLAY_SIGNAL_DOCK:
	    Q_EMIT signalDisplayStateChange (DISPLAY_SIGNAL_DOCK);
	    break;
	case DISPLAY_SIGNAL_OFF_ON_CALL:
	    Q_EMIT signalDisplayStateChange (DISPLAY_SIGNAL_OFF_ON_CALL);
	    break;
	default:
        g_warning("%s: invalid display signal %d", __PRETTY_FUNCTION__, displaySignal);
        break;
    }
}

void DisplayManager::updateLockState (DisplayLockState lockState, DisplayState displayState, DisplayEvent displayEvent)
{
    if (lockState != m_lockState) {
	m_lockState = lockState;
        switch (lockState) {
            case DisplayLockLocked:
                {
                    g_debug ("%s: firing DISPLAY_LOCK_SCREEN", __PRETTY_FUNCTION__);
                    Q_EMIT signalLockStateChange (DISPLAY_LOCK_SCREEN, displayEvent);
                }
                break;
            case DisplayLockUnlocked:
                {
                    g_debug ("%s: firing DISPLAY_UNLOCK_SCREEN", __PRETTY_FUNCTION__);
                    Q_EMIT signalLockStateChange (DISPLAY_UNLOCK_SCREEN, displayEvent);
                }
                break;
            case DisplayLockDockMode:
		{
			g_debug ("%s: firing DISPLAY_DOCK_SCREEN", __PRETTY_FUNCTION__);
			Q_EMIT signalLockStateChange (DISPLAY_DOCK_SCREEN, displayEvent);
                }
        		break;
            default:
                g_warning("%s: Unknown lock state %d", __PRETTY_FUNCTION__, lockState);
                break;
        }
    }
}

void DisplayManager::displayOn(bool als)
{
    m_displayOn = true;

	if (!als) {
		// If display is "turned on" due to ALS/brightness
		// change, we shouldn't notify the subscribers
		if (currentState() == DisplayStateDockMode)
			notifySubscribers (DISPLAY_EVENT_DOCKMODE);
		else
			notifySubscribers (DISPLAY_EVENT_ON);
	}

    // backlight has to be turned on first, On completion,
    // the callback will turn on the touchpanel
    backlightOn (getDisplayBrightness(), getKeypadBrightness(), als);
    // update navi brightness
    CoreNaviManager::instance()->updateBrightness (getCoreNaviBrightness());

    if (Preferences::instance()->isAlsEnabled() && !m_alsDisabled && Settings::LunaSettings()->uiType != Settings::UI_MINIMAL)
	    m_als->start();
}

void DisplayManager::displayDim()
{
    m_displayOn = true;
    int b = getDisplayBrightness();

    b /= 10;
    if (b < MINIMUM_DIMMED_BRIGHTNESS)
	b = MINIMUM_DIMMED_BRIGHTNESS;

    // backlight has to be turned on first, On completion,
    // the callback will turn on the touchpanel
    backlightOn (b, 0, false);

    // update navi brightness
    CoreNaviManager::instance()->updateBrightness (0);

    notifySubscribers (DISPLAY_EVENT_DIMMED);

    m_als->stop();
}


void DisplayManager::displayOff()
{

    m_displayOn = false;
    // touch panel has to be turned off first. On completion, the
    // callback will turn off the backlight
    notifySubscribers (DISPLAY_EVENT_OFF);

    if (m_penDown) {
	    m_drop_pen = true;
    }

    touchPanelOff();
    CoreNaviManager::instance()->updateBrightness (0);

    m_als->stop();

}

bool DisplayManager::allowSuspend()
{
#if defined(MACHINE_BROADWAY)
	return (currentState() == DisplayStateOff || currentState() == DisplayStateOffOnCall) && !m_backlightIsOn;
#else
	return currentState() == DisplayStateOff && !m_backlightIsOn;
	// allow suspend when state is off and backlight is really off.
#endif
}


void DisplayManager::setSuspended (bool suspended) {

	if (suspended)
	    m_currentState->handleEvent (DisplayEventPowerdSuspend);
	else 
	    m_currentState->handleEvent (DisplayEventPowerdResume);
//	g_warning ("%s: Device is %s", __func__, m_deviceIsSuspended ? "suspended" : "not suspended");
}

bool DisplayManager::s_forceVsyncDisable = false;
bool DisplayManager::s_vsyncEnabled = true;

void DisplayManager::forceVsyncOff(bool forceNoVsync)
{
	if (s_forceVsyncDisable == forceNoVsync)
		return;

	if (forceNoVsync) {
		// turn off vsync before disabling vsync control
		// make sure to back up the state first so that this call
		// doesn't corrupt it
		bool wasEnabled = s_vsyncEnabled;
		changeVsyncControl(false);
		s_vsyncEnabled = wasEnabled;
		s_forceVsyncDisable = forceNoVsync;
	} else {
		s_forceVsyncDisable = forceNoVsync;
		changeVsyncControl(s_vsyncEnabled);
	}
}

bool DisplayManager::isVsyncOff()
{
	return s_forceVsyncDisable || !s_vsyncEnabled;
}

void DisplayManager::changeVsyncControl(bool enable)
{
	if (Settings::LunaSettings()->forceSoftwareRendering)
		return;

	s_vsyncEnabled = enable;

	if (s_forceVsyncDisable)
		return;

#if defined(TARGET_DEVICE) && defined(HAVE_OPENGL)
	QGLContext* gc = (QGLContext*) QGLContext::currentContext();
	if (gc) {
		setEglSwapInterval(enable ? 1 : 0);
        g_message("Turned vsync %s", enable ? "on" : "off");
	}
#endif
}
