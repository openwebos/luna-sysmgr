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



#include "InputManager.h"

#include "Common.h"
#include "CustomEvents.h"
#include "HostBase.h"
#include "JSONUtils.h"
#include "Preferences.h"
#include "SystemService.h"

#include <QKeyEvent>
#include <cjson/json.h>
#include <glib.h>
#include <map>
#include <string>

#define LUNA_KEYS		"com.palm.keys"
#define CATEGORY_AUDIO		"/audio"
#define CATEGORY_MEDIA		"/media"
#define CATEGORY_SWITCHES	"/switches"
#define CATEGORY_HEADSET	"/headset"
#define METHOD_STATUS		"status"

#define STR_VOLUME_UP		"volume_up"
#define STR_VOLUME_DOWN		"volume_down"
#define STR_MEDIA_PLAY		"play"
#define STR_MEDIA_PAUSE		"pause"
#define STR_MEDIA_TOGGLE_PAUSE_PLAY   "togglePausePlay"
#define STR_MEDIA_STOP		"stop"
#define STR_MEDIA_NEXT		"next"
#define STR_MEDIA_PREV		"prev"
#define STR_RINGER		"ringer"
#define STR_SLIDER		"slider"
#define STR_POWER		"power"
#define STR_HEADSET_BTN		"headset_button"
#define STR_REPEAT_ALL		"repeat-all"
#define STR_REPEAT_TRACK	"repeat-track"
#define STR_REPEAT_NONE		"repeat-none"
#define STR_SHUFFLE_ON		"shuffle-on"
#define STR_SHUFFLE_OFF		"shuffle-off"

// inserting/removing the headset
#define STR_HEADSET		"headset"
#define STR_HEADSET_MIC		"headset-mic"

#define PRESS_AND_HOLD_TIME_MS	2000
#define DOUBLE_PRESS_TIME_MS	1000

// Add a little bit of a fudge factor for the activity timer
// in case the system is busy
#define ACTIVITY_TIMER_DURATION_HEADSET	"1500"	
#define ACTIVITY_START_URI		"palm://com.palm.power/com/palm/power/activityStart"
#define ACTIVITY_ID_HEADSET		"com.palm.keys-headset-btn-delay"
#define ACTIVITY_JSON_MSG_HEADSET	"{\"id\":\""ACTIVITY_ID_HEADSET"\",\"duration_ms\":"ACTIVITY_TIMER_DURATION_HEADSET"}"

#define HEADSET_BTN_HOLD		"hold"
#define HEADSET_BTN_SINGLE_PRESS	"single_click"
#define HEADSET_BTN_DOUBLE_PRESS	"double_click"

/*! \page com_palm_keys Service API com.palm.keys/
 *
 * Public methods:
 *  - \ref com_palm_keys_audio_status
 *  - \ref com_palm_keys_headset_status
 *  - \ref com_palm_keys_media_status
 *  - \ref com_palm_keys_switches_status
 */

static LSMethod s_audioMethods[] = {
	{ METHOD_STATUS, InputManager::audioKeyServiceCallback },
	{ 0, 0 },
};

static LSMethod s_mediaMethods[] = {
	{ METHOD_STATUS, InputManager::mediaKeyServiceCallback },
	{ 0, 0 },
};

// subscribe to METHOD_STATUS to know when a switch changes, can also send
// queries
static LSMethod s_switchesMethods[] = {
	{ METHOD_STATUS, InputManager::switchesStatusCallback },
	{ 0, 0 },
};

// subscribe to know about headset events (currently only headset_button)
// exists with "up" "down" "hold" "single_press" and "double_press"
static LSMethod s_headsetMethods[] = {
	{ METHOD_STATUS, InputManager::headsetStatusCallback },
	{ 0, 0 },
};

InputManager::InputManager()
	: m_palmService(NULL)
	, m_service(NULL)
	, m_publicService(NULL)
	, m_ringerState(QEvent::None)
	, m_sliderState(QEvent::None)
	, m_headsetState(QEvent::None)
	, m_headsetType(HeadsetInvalid)
	, m_headsetBtnState(Start)
	, m_headsetBtnTimer(HostBase::instance()->masterTimer(), this, &InputManager::headsetBtnTimerCallback)
{
	m_categoryMap[CATEGORY_AUDIO] = new CategoryMapEntry(CATEGORY_AUDIO, true);
	m_categoryMap[CATEGORY_MEDIA] = new CategoryMapEntry(CATEGORY_MEDIA, true);
	m_categoryMap[CATEGORY_SWITCHES] = new CategoryMapEntry(CATEGORY_SWITCHES, true);
	m_categoryMap[CATEGORY_HEADSET] = new CategoryMapEntry(CATEGORY_HEADSET, true);
	
	// If there are no switches, then go ahead and start the service,
	// since there are no switches to trigger it in handleEvent
	if (HostBase::instance()->getNumberOfSwitches() == 0) {
		startService();
	}
}

InputManager::~InputManager()
{
	stopService();
}

void InputManager::startService()
{
	LSError lserror;
	LSErrorInit(&lserror);
	bool result;
	
	GMainLoop* mainLoop = HostBase::instance()->mainLoop();

	g_debug ("%s starting", __PRETTY_FUNCTION__);

	result = LSRegisterPalmService(LUNA_KEYS, &m_palmService, &lserror);
	if (!result) goto Error;

	// Save off public and private bus handles
	m_publicService = LSPalmServiceGetPublicConnection(m_palmService);
	if (NULL == m_publicService) {
		g_message("unable to get public handle");
	}

	
	m_service = LSPalmServiceGetPrivateConnection(m_palmService);
	if (NULL == m_service) {
		g_message("unable to get private handle");
	}

	// We're providing the notion of "key categories" that you can
	// subscribe to in order to get notifications when a key is pressed
	result = LSPalmServiceRegisterCategory(m_palmService, CATEGORY_AUDIO, s_audioMethods, NULL, NULL, this, &lserror);
	if (!result) goto Error;

	// For now the media keys are the only ones that have been requested
	// to be put on the public bus -- the calling syntax is a little
	// strange, but when using this call everything on the public bus
	// is also put on the private bus
	result = LSPalmServiceRegisterCategory(m_palmService, CATEGORY_MEDIA, s_mediaMethods, NULL, NULL, this, &lserror);
	if (!result) goto Error;
	
	result = LSPalmServiceRegisterCategory(m_palmService, CATEGORY_SWITCHES, s_switchesMethods, NULL, NULL, this, &lserror);
	if (!result) goto Error;

	result = LSPalmServiceRegisterCategory(m_palmService, CATEGORY_HEADSET, s_headsetMethods, NULL, NULL, this, &lserror);
	if (!result) goto Error;

	result = LSGmainAttachPalmService(m_palmService, mainLoop, &lserror);
	if (!result) goto Error;
	
	g_debug ("%s service started", LUNA_KEYS);

	return;

Error:
	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	g_debug(":%s: Unable to start service", __PRETTY_FUNCTION__);
}

void InputManager::stopService()
{
	LSError lserror;
	LSErrorInit(&lserror);
	bool result;

	result = LSUnregister(m_publicService, &lserror);
	if (!result) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	result = LSUnregister(m_service, &lserror);
	if (!result) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}
}

bool InputManager::headsetBtnTimerCallback()
{
	switch (m_headsetBtnState) {

	case PotentialDoublePress: {
		// timer fired, so we don't have a double press
		m_headsetBtnState = Start;
		break;
	}
	case SinglePressOrHold:
	case DoublePressOrHold: {
		// timer fired, so we're in the hold state
		postKeyToSubscribers(NULL, CATEGORY_HEADSET, STR_HEADSET_BTN, HEADSET_BTN_HOLD);
		m_headsetBtnState = Hold;
		break;
	}
	case Start:
	case Hold:
	default: {
		g_critical("%s: Invalid headset state: %d",
			__PRETTY_FUNCTION__, (int)m_headsetBtnState);
		break;
	}

	}

	return false;	// don't restart the timer
}

void InputManager::headsetStateMachine(QEvent* e)
{
	switch (m_headsetBtnState) {

	case Start: {
		if (e->type() == QEvent::KeyPress) {
			m_headsetBtnState = SinglePressOrHold;
			m_headsetBtnTimer.start(PRESS_AND_HOLD_TIME_MS, true); // singleshot
		}
		// if we get an up in this state, we'll ignore it
		// that might happen if the user is holding the button down on
		// start up
		break;
	}
	case SinglePressOrHold: {
		if (e->type() == QEvent::KeyRelease) {
			m_headsetBtnTimer.stop();
			
			// single press
			postKeyToSubscribers(NULL, CATEGORY_HEADSET, STR_HEADSET_BTN, HEADSET_BTN_SINGLE_PRESS);

			m_headsetBtnState = PotentialDoublePress;
			m_headsetBtnTimer.start(DOUBLE_PRESS_TIME_MS, true); // singleshot

			// FIXME:
			// We start an activity timer to make sure that
			// we don't go to sleep before the double-click
			// timer fires.
			LSCallOneReply(m_publicService, ACTIVITY_START_URI, ACTIVITY_JSON_MSG_HEADSET,
					 activityStartCallback, NULL, NULL, NULL);


		} else {
			g_critical("%s: SinglePressOrHold state received event type: %d",
				__PRETTY_FUNCTION__, (int)e->type());
		}
		break;
	}
	case Hold: {
		if (e->type() == QEvent::KeyRelease) {
			// make sure timer is stopped
			m_headsetBtnTimer.stop();
			m_headsetBtnState = Start;
		} else {
			g_critical("%s: Hold state received event type: %d",
				__PRETTY_FUNCTION__, (int)e->type());
		}
		break;
	}
	case PotentialDoublePress: {
		if (e->type() == QEvent::KeyPress) {
			m_headsetBtnTimer.stop();
			m_headsetBtnState = DoublePressOrHold;
			m_headsetBtnTimer.start(PRESS_AND_HOLD_TIME_MS, true);	// singleshot
		} else {
			g_critical("%s: Hold state received event type: %d",
				__PRETTY_FUNCTION__, (int)e->type());
		}
		break;
	}
	case DoublePressOrHold: {
		if (e->type() == QEvent::KeyRelease) {
			m_headsetBtnTimer.stop();
			
			// double press
			postKeyToSubscribers(NULL, CATEGORY_HEADSET, STR_HEADSET_BTN, HEADSET_BTN_DOUBLE_PRESS);
			m_headsetBtnState = Start;
		}
		break;
	}
	default: {
		g_critical("%s: Invalid headset state: %d",
			__PRETTY_FUNCTION__, (int)m_headsetBtnState);
		break;
	}

	}
}

bool InputManager::processSubscription(LSHandle* handle, LSMessage* msg, void* userData)
{
    SUBSCRIBE_SCHEMA_RETURN(handle, msg);

	// process subscriptions for a group of keys
	bool retVal = false;
	LSError lserror;
	LSErrorInit(&lserror);
	bool subscribed = false;
	json_object* json = json_object_new_object();

	if (LSMessageIsSubscription(msg)) {
		retVal = LSSubscriptionProcess(handle, msg, &subscribed, &lserror);
		if (!retVal) {
			LSErrorPrint (&lserror, stderr);
			LSErrorFree (&lserror);
			goto Error;
		}
	} else {
		json_object_object_add(json, "errorCode", json_object_new_int(-1));
		json_object_object_add(json, "errorText", json_object_new_string("We were expecting a subscribe type message, but we did not recieve one."));
	}

Error:

	json_object_object_add(json, "returnValue", json_object_new_boolean(retVal));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));
	

	if (!LSMessageReply(handle, msg, json_object_to_json_string(json), &lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree (&lserror);
	}
	json_object_put(json);

	return true;	// message has been processed, don't call the callback anymore
}

bool InputManager::processKeyState(LSHandle* handle, LSMessage* msg, void* userData)
{
    // {"get": string}
    VALIDATE_SCHEMA_AND_RETURN(handle,
                               msg,
                               SCHEMA_1(REQUIRED(get, string)));

	bool success = false;
	const char* keyString = NULL;
	QEvent::Type state = QEvent::None;
	LSError err;
	json_object* root = 0;

	LSErrorInit(&err);

	// get the text name of the key
	const char* str = LSMessageGetPayload(msg);
	if (!str) {
		g_debug("%s: Unable to get JSON payload from message", __PRETTY_FUNCTION__);
		return false;
	}

	root = json_tokener_parse(str);
	if (root && !is_error(root)) {

		// Get the key name from the msg -- the format will be {"get":"NAME"},
		// where NAME is something like ringer, slider, etc
		keyString = json_object_get_string(json_object_object_get(root, "get"));
		if (keyString) {

			// lookup the state of the key
			Qt::Key key = stringToKey(keyString);
			state = getKeyState(key);

			success = true;
		}
	}
	
	json_object* response = 0;
	if (success) {
		response = createKeyJson(keyString, state);
	}
	else {
		response = json_object_new_object();
	}
	json_object_object_add(response, "returnValue", json_object_new_boolean(success));

	if (!LSMessageReply(handle, msg, json_object_to_json_string(response), &err)) {
		LSErrorPrint(&err, stderr);
		LSErrorFree(&err);
	}

	if (root && !is_error(root))
		json_object_put(root);

	json_object_put(response);

	return true;
}

/*!
\page com_palm_keys
\n
\section com_palm_keys_audio_status audio/status

\e Public.

com.palm.keys/audio/status

Subscribe to audio key status changes.

\subsection com_palm_keys_audio_status_syntax Syntax:
\code
{
    "subscribe": boolean
}
\endcode

\param subscribe Set to true to subscribe to status changes.

\subsection com_palm_keys_audio_status_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string
    "returnValue": boolean,
    "subscribed": boolean
}
\endcode

\param errorCode Code for the error if call was not succesful.
\param errorText Describes the error if call was not succesful.
\param returnValue Indicates if the call was succesful.
\param subscribed True if subscription to status changes is made.

\subsection com_palm_keys_audio_status_examples Examples:
\code
luna-send -n 2 -f luna://com.palm.keys/audio/status '{ "subscribe": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "subscribed": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": -1,
    "errorText": "We were expecting a subscribe type message, but we did not recieve one.",
    "returnValue": false,
    "subscribed": false
}
\endcode
*/
bool InputManager::audioKeyServiceCallback(LSHandle* handle, LSMessage* msg, void* userData)
{
	// we only care about subscriptions
	return processSubscription(handle, msg, userData);
}

/*!
\page com_palm_keys
\n
\section com_palm_keys_media_status media/status

\e Public.

com.palm.keys/media/status

Subscribe to media key status changes.

\subsection com_palm_keys_media_status_syntax Syntax:
\code
{
    "subscribe": boolean
}
\endcode

\param subscribe Set to true to subscribe to status changes.

\subsection com_palm_keys_media_status_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string
    "returnValue": boolean,
    "subscribed": boolean
}
\endcode

\param errorCode Code for the error if call was not succesful.
\param errorText Describes the error if call was not succesful.
\param returnValue Indicates if the call was succesful.
\param subscribed True if subscription to status changes is made.

\subsection com_palm_keys_media_status_examples Examples:
\code
luna-send -n 2 -f luna://com.palm.keys/media/status '{ "subscribe": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "subscribed": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": -1,
    "errorText": "We were expecting a subscribe type message, but we did not recieve one.",
    "returnValue": false,
    "subscribed": false
}
\endcode
*/
bool InputManager::mediaKeyServiceCallback(LSHandle* handle, LSMessage* msg, void* userData)
{
	// we only care about subscriptions
	return processSubscription(handle, msg, userData);
}

/*!
\page com_palm_keys
\n
\section com_palm_keys_switches_status switches/status

\e Public.

com.palm.keys/switches/status

Subscribe to switch state changes or request the state of a particular switch.

\subsection com_palm_keys_switches_status_syntax_subscribe Syntax for subscribing:
\code
{
    "subscribe": boolean
}
\endcode

\param subscribe Set to true to subscribe to status changes.

\subsection com_palm_keys_switches_status_syntax_request Syntax for requesting a switch state:
\code
{
    "get": string
}
\endcode

\param get Name of the swtich, for example ringer, slider, etc.

\subsection com_palm_keys_switches_status_returns_subscribe Returns for subscribe messages:
\code
{
    "returnValue": boolean,
    "subscribe": boolean
}
\endcode

\param returnValue Indicates if the call was succesful.
\param subscribe True if subscribed to switch status changes.

\subsection com_palm_keys_switches_status_returns_request Returns for request messages:
\code
{
    "key": string,
    "state": string,
    "returnValue": boolean
}
\endcode

\param key Name of the switch.
\param state State of the switch.
\param returnValue Indicates if the call was succesful.

\subsection com_palm_keys_switches_status_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.keys/switches/status '{ "get": "ringer" } '
\endcode

Example response for a succesful request call:
\code
{
    "key": "ringer",
    "state": "up",
    "returnValue": true
}
\endcode

Example response for a succesful subscription call:
\code
{
    "returnValue": true,
    "subscribed": true
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": false
}
\endcode
*/
bool InputManager::switchesStatusCallback(LSHandle* handle, LSMessage* msg, void* userData)
{
	InputManager* i = (InputManager*)userData;

	// users can subscribe for notification of changes in state and also
	// request the current state
	if (LSMessageIsSubscription(msg)) {
		return processSubscription(handle, msg, userData);
	} else {
		return i->processKeyState(handle, msg, userData);
	}
}

/*!
\page com_palm_keys
\n
\section com_palm_keys_headset_status headset/status

\e Public.

com.palm.keys/headset/status

Subscribe to headset key events. Currently only "headset_button" exists with "up" "down" "hold" "single_press" and "double_press" states.

\subsection com_palm_keys_headset_status_syntax Syntax:
\code
{
    "subscribe": boolean
}
\endcode

\param subscribe Set to true to subscribe to status changes.

\subsection com_palm_keys_headset_status_returns Returns:
\code
{
    "errorCode": int,
    "errorText": string
    "returnValue": boolean,
    "subscribed": boolean
}
\endcode

\param errorCode Code for the error if call was not succesful.
\param errorText Describes the error if call was not succesful.
\param returnValue Indicates if the call was succesful.
\param subscribed True if subscription to status changes is made.

\subsection com_palm_keys_headset_status_examples Examples:
\code
luna-send -n 2 -f luna://com.palm.keys/headset/status '{ "subscribe": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "subscribed": true
}
\endcode

Example response for a failed call:
\code
{
    "errorCode": -1,
    "errorText": "We were expecting a subscribe type message, but we did not recieve one.",
    "returnValue": false,
    "subscribed": false
}
\endcode
*/
bool InputManager::headsetStatusCallback(LSHandle* handle, LSMessage* msg, void* userData)
{
	// we only care about subscriptions
	return processSubscription(handle, msg, userData);
}

bool InputManager::activityStartCallback(LSHandle* handle, LSMessage* msg, void* userData)
{
	struct json_object* retValObj = NULL;
	bool retVal = false;
	const char* msgStr = LSMessageGetPayload(msg);
	
	struct json_object* respObj = json_tokener_parse(msgStr);
	if (is_error(respObj)) {
		g_message("%s: Unable to parse JSON response: %s\n", __PRETTY_FUNCTION__, msgStr);
		goto Exit;
	}
	
	retValObj = json_object_object_get(respObj, "returnValue");

	if (NULL == retValObj) {
		g_message("%s: Unable to get returnValue from: %s", __PRETTY_FUNCTION__, msgStr);
		goto Exit;
	}

	retVal = json_object_get_boolean(retValObj);
	if (!retVal) {
		g_message("%s: error response: %s", __PRETTY_FUNCTION__, msgStr);
	}

Exit:
	if (!is_error(respObj)) json_object_put(respObj);
	return true;
}

#if 0
bool InputManager::switchesRingerCallback(LSHandle* handle, LSMessage* msg, void* userData)
{
	// get and return ringer state from HIDd
	// com.palm.hidd/HidKeypad/ringer_state mode: get
	
	Event::Type type = Event::Key_Down; // Event::Key_Up

	return replyKeyState(handle, msg, "ringer", type);	
}

bool InputManager::switchesSliderCallback(LSHandle* handle, LSMessage* msg, void* userData)
{
	// get and return slider state from HIDd
	// com.palm.hidd/HidKeypad/slider_state mode: get
	
	return replyKeyState(handle, msg, "slider", type);	
}
#endif

QEvent::Type InputManager::getKeyState(Qt::Key key)
{
	// TODO: should probably replace with hash/map
	switch (key)
	{
	case Qt::Key_Ringer:
#if defined(TARGET_EMULATOR)
		return QEvent::KeyRelease; // report audio as un-muted
#else
		return m_ringerState;
#endif
	case Qt::Key_Slider:
#if defined(TARGET_EMULATOR)
		return QEvent::KeyPress; // report slider in "closed" position
#else
		return m_sliderState;
#endif
	case Qt::Key_Headset:
		if (m_headsetState == QEvent::KeyPress && m_headsetType == Headset)
			return QEvent::KeyPress;
		else
			return QEvent::KeyRelease;
		break;
	case Qt::Key_HeadsetMic:
		if (m_headsetState == QEvent::KeyPress && m_headsetType == HeadsetMic)
			return QEvent::KeyPress;
		else
			return QEvent::KeyRelease;
		break;
	default:
		return QEvent::None;
	}
}

bool InputManager::setKeyState(Qt::Key key, QEvent::Type state)
{
	switch (key)
	{
	case Qt::Key_Ringer:
		m_ringerState = state;
		break;
	case Qt::Key_Slider:
		m_sliderState = state;
		break;
	case Qt::Key_Headset:
	case Qt::Key_HeadsetMic:
		m_headsetState = state;
		
		if (key == Qt::Key_Headset) {
			m_headsetType = Headset;
		} else if (key == Qt::Key_HeadsetMic) {
			m_headsetType = HeadsetMic;
		} else {
			m_headsetType = HeadsetInvalid;
		}
		break;
	default:
		return false;
	}

	return true;
}

bool InputManager::keyToString(Qt::Key key, const char** string)
{
	// TODO: we may want to use a hash table/map if we need more keys
	switch (key)
	{
	case Qt::Key_Ringer:
		*string = STR_RINGER;
		break;
	case Qt::Key_Slider:
		*string = STR_SLIDER;
		break;
	case Qt::Key_Power:
		*string = STR_POWER;
		break;
	case Qt::Key_VolumeUp:
		*string = STR_VOLUME_UP;
		break;
	case Qt::Key_VolumeDown:
		*string = STR_VOLUME_DOWN;
		break;
	case Qt::Key_MediaPlay:
		*string = STR_MEDIA_PLAY;
		break;
	case Qt::Key_MediaPause:
		*string = STR_MEDIA_PAUSE;
		break;
    case Qt::Key_MediaTogglePlayPause:
        *string = STR_MEDIA_TOGGLE_PAUSE_PLAY;
        break;
	case Qt::Key_MediaStop:
		*string = STR_MEDIA_STOP;
		break;
	case Qt::Key_MediaNext:
		*string = STR_MEDIA_NEXT;
		break;
	case Qt::Key_MediaPrevious:
		*string = STR_MEDIA_PREV;
		break;
	case Qt::Key_HeadsetButton:
		*string = STR_HEADSET_BTN;
		break;
	case Qt::Key_Headset:
		*string = STR_HEADSET;
		break;
	case Qt::Key_HeadsetMic:
		*string = STR_HEADSET_MIC;
		break;
	case Qt::Key_MediaRepeatAll:
		*string = STR_REPEAT_ALL;
		break;
	case Qt::Key_MediaRepeatTrack:
		*string = STR_REPEAT_TRACK;
		break;
	case Qt::Key_MediaRepeatNone:
		*string = STR_REPEAT_NONE;
		break;
	case Qt::Key_MediaShuffleOn:
		*string = STR_SHUFFLE_ON;
		break;
	case Qt::Key_MediaShuffleOff:
		*string = STR_SHUFFLE_OFF;
		break;
	default:
		*string = NULL;
		return false;
	}

	return true;
}

Qt::Key InputManager::stringToKey(const char* string)
{
	// TODO: we may want to use a hash table/map if we need more keys
	if (0 == strcmp(STR_RINGER, string)) {
		return Qt::Key_Ringer;
	} else if (0 == strcmp(STR_SLIDER, string)) {
		return Qt::Key_Slider;		
	} else if (0 == strcmp(STR_POWER, string)) {
		return Qt::Key_Power;		
	} else if (0 == strcmp(STR_VOLUME_UP, string)) {
		return Qt::Key_VolumeUp;
	} else if (0 == strcmp(STR_VOLUME_DOWN, string)) {
		return Qt::Key_VolumeDown;
	} else if (0 == strcmp(STR_MEDIA_PLAY, string)) {
		return Qt::Key_MediaPlay;
	} else if (0 == strcmp(STR_MEDIA_PAUSE, string)) {
		return Qt::Key_MediaPause;
    } else if (0 == strcmp(STR_MEDIA_TOGGLE_PAUSE_PLAY, string)) {
        return Qt::Key_MediaTogglePlayPause;
	} else if (0 == strcmp(STR_MEDIA_STOP, string)) {
		return Qt::Key_MediaStop;
	} else if (0 == strcmp(STR_MEDIA_NEXT, string)) {
		return Qt::Key_MediaNext;
	} else if (0 == strcmp(STR_MEDIA_PREV, string)) {
		return Qt::Key_MediaPrevious;
	} else if (0 == strcmp(STR_HEADSET_BTN, string)) {
		return Qt::Key_HeadsetButton;
	} else if (0 == strcmp(STR_HEADSET, string)) {
		return Qt::Key_Headset;
	} else if (0 == strcmp(STR_HEADSET_MIC, string)) {
		return Qt::Key_HeadsetMic;
	} else if (0 == strcmp(STR_REPEAT_ALL, string)) {
		return Qt::Key_MediaRepeatAll;
	} else if (0 == strcmp(STR_REPEAT_TRACK, string)) {
		return Qt::Key_MediaRepeatTrack;
	} else if (0 == strcmp(STR_REPEAT_NONE, string)) {
		return Qt::Key_MediaRepeatNone;
	} else if (0 == strcmp(STR_SHUFFLE_ON, string)) {
		return Qt::Key_MediaShuffleOn;
	} else if (0 == strcmp(STR_SHUFFLE_OFF, string)) {
		return Qt::Key_MediaShuffleOff;
	} else {
		return Qt::Key_unknown;
	}
}

bool InputManager::isAudioKey(Qt::Key key)
{
	switch (key)
	{
	case Qt::Key_VolumeUp:
	case Qt::Key_VolumeDown:
		return true;
	default:
		return false;
	}
}

bool InputManager::isMediaKey(Qt::Key key)
{
	switch (key)
	{
	case Qt::Key_MediaPlay:
	case Qt::Key_MediaPause:
    case Qt::Key_MediaTogglePlayPause:
	case Qt::Key_MediaStop:
	case Qt::Key_MediaNext:
	case Qt::Key_MediaPrevious:
	case Qt::Key_MediaRepeatAll:
	case Qt::Key_MediaRepeatTrack:
	case Qt::Key_MediaRepeatNone:
	case Qt::Key_MediaShuffleOn:
	case Qt::Key_MediaShuffleOff:
		return true;
	default:
		return false;
	}
}

bool InputManager::isSwitch(Qt::Key key)
{
	switch (key)
	{
	case Qt::Key_Ringer:
	case Qt::Key_Slider:
	case Qt::Key_Power:
		return true;
	default:
		return false;
	}
}

bool InputManager::isBluetoothKey(Qt::Key key)
{
    switch (key)
    {
    case Qt::Key_VolumeMute:
        return true;
    default:
        return false;
    }
}

bool InputManager::isHeadsetKey(Qt::Key key)
{
	switch (key)
	{
	case Qt::Key_HeadsetButton:
	case Qt::Key_Headset:
	case Qt::Key_HeadsetMic:
		return true;
	default:
		return false;
	}
}

bool InputManager::isPublicCategory(const char* category)
{
	CategoryMapEntry* mapEntry = m_categoryMap[category];

	if (mapEntry) {
		return mapEntry->isPublic();
	} else {
		g_message("%s: called with invalid category: %s", __PRETTY_FUNCTION__, category);
		return false;
	}
}

void InputManager::handleBluetoothKey(const QKeyEvent* keyEvent)
{
    // toggle the mute state
    if (keyEvent->key() == Qt::Key_VolumeMute && keyEvent->type() == QEvent::KeyRelease) {
        Preferences::instance()->setMuteSoundPref(!Preferences::instance()->isMuteOn());
    }
}

// on success returns a ptr to an allocated string that must be freed
// failure returns NULL 
json_object* InputManager::createKeyJson(const char* key, const char* value)
{
	json_object* json = json_object_new_object();
	if (!json)
		return NULL;

	json_object_object_add(json, "key", json_object_new_string(key));
	json_object_object_add(json, "state", json_object_new_string(value));

	return json;
}

// on success returns a ptr to an allocated string that must be freed
// failure returns NULL 
json_object* InputManager::createKeyJson(const char* key, QEvent::Type type)
{
	const char* value = NULL;

	if (type == QEvent::KeyPress) {
		value = "down";
	} else if (type == QEvent::KeyRelease) {
		value = "up";
	} else {
		value = "unknown";
	}

	return createKeyJson(key, value);
}

// returns true on successful subscription post, false on failure
void InputManager::postKeyToSubscribers(QKeyEvent* event, const char* category, const char* keyString, const char* keyValue)
{
	LSError lserror;
	LSErrorInit(&lserror);
	
	json_object* response = NULL;

	if (!m_publicService || !m_service)
		return;

	// normally this will be called with an event ptr, but if we pass it a 
	// "keyValue", then we'll ignore the event ptr and use that directly
	if (keyValue != NULL) {
		response = createKeyJson(keyString, keyValue);
	} else {
		response = createKeyJson(keyString, event->type());
	}

	const char* payloadStr = json_object_to_json_string(response);

	// Post to all subscribers on private bus
	if (!LSSubscriptionPost(m_service, category, METHOD_STATUS, payloadStr, &lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	// If it's a public category, post it to public subscribers
	if (isPublicCategory(category)) {
		if (!LSSubscriptionPost(m_publicService, category, METHOD_STATUS, payloadStr, &lserror)) {
			LSErrorPrint(&lserror, stderr);
			LSErrorFree(&lserror);
		}
	}

	if (response && !is_error(response))
		json_object_put(response);
}

bool InputManager::handleEvent(QEvent* event)
{
	if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease)
		return false;

	QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
	Qt::Key key = static_cast<Qt::Key>(keyEvent->key());
	const char* keyCategory = NULL;

	// NOTE: We can potentially get key presses on the device before
	// com.palm.keys comes up, and these won't be broadcast to subscribers
	// However, we assume that we wouldn't want them to receive a collection
	// of queued up key presses

	// Each subset of keys must not overlap in the current handling	
	if (isAudioKey(key)) {
		keyCategory = CATEGORY_AUDIO;
	} else if (isMediaKey(key)) {
		keyCategory = CATEGORY_MEDIA;
	} else if (isSwitch(key)) {
		keyCategory = CATEGORY_SWITCHES;
	} else if (isHeadsetKey(key)) {
		if (!(key == Qt::Key_Headset || key == Qt::Key_HeadsetMic)) {
			// state machine time -- headset button
			headsetStateMachine(keyEvent);	
		}
		
		keyCategory = CATEGORY_HEADSET;
    } else if (isBluetoothKey(key)) {
        handleBluetoothKey(keyEvent);
        return true;
	} else {
		// Not a key that we care about, so we indicate we didn't handle it
		return false;
	}
	
	// first event that comes in sets the sticky state, no matter
	// whether it is a real action that a user did or a generated
	// "InitialState"
	static int switchInitCount = 0;
	if (setKeyState(key, event->type())) {
		if (NULL == m_publicService && (++switchInitCount == HostBase::instance()->getNumberOfSwitches() )) {
			startService();
		}
	}

	// We don't post the "InitialState" key because it is internally
	// generated in order to get initial state of a key
	if (!(keyEvent->nativeModifiers() & SysMgrNativeKeyboardModifier_InitialState)) {
		const char* keyString = NULL;
		keyToString(key, &keyString);

		(void)postKeyToSubscribers(keyEvent, keyCategory, keyString, NULL);
	}

	return true;
}

