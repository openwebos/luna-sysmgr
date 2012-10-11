/**
 *  Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#include "Common.h"

#include "WebAppBase.h"
#include "WebAppCache.h"
#include "WebPage.h"
#include "WebAppFactory.h"
#include "WebAppManager.h"
#include "Window.h"
#include "Time.h"
#include "WindowedWebApp.h"
#include "Settings.h"
#include "JsUtil.h"
#include "JsSysObject.h"
#include "ApplicationDescription.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <glib.h>
#include <string>
#include <algorithm>

#include <cjson/json.h>

#include <palmwebpage.h>
#include <palmwebview.h>

// constants used by the 'forceInterface' attribute option
static const char *wifiIdentifierString = "wifi";
static const char *wanIdentifierString = "wan";


WebAppBase::WebAppBase()
    : m_page(0)
    , m_inCache(false)
    , m_keepAlive(false)
    , appDescImage(0)
    , m_activityManagerToken(LSMESSAGE_TOKEN_INVALID)
{
#if defined(HAS_NYX)
    m_OrientationAngle = INVALID_ANGLE;
#endif
}

WebAppBase::~WebAppBase()
{
        if (m_inCache)
                WebAppCache::remove(this);

        // appDeleted relies on the page being valid so we need to call it
        // before issuing cleanResources which deletes the page
        WebAppManager* wam = WebAppManager::instance();
        wam->appDeleted(this);

        cleanResources();
#if defined(HAS_NYX)
        destroyAllSensors();
#endif

        wam->removeHeadlessAppFromWatchList(this);
}

void WebAppBase::cleanResources()
{
	if (m_page) {
		WebAppManager::instance()->reportAppClosed(m_page->appId(),
												   m_page->processId());
	}
	
	// does nothing if m_page has already been deleted and set to 0 by ~WindowedWebApp
	destroyActivity();
	
	// NOTE: the WebPage's destructor accesses appDescImage so the order
	// 	of operations here matters
	if (m_page) {
		delete m_page;
		m_page = 0;
	}
	if (appDescImage) {
		delete appDescImage;
		appDescImage = 0;
	}
}

void WebAppBase::setExplicitEditorFocus(bool focused, const PalmIME::EditorState & editorState)
{
    if (m_page) {
		m_page->explicitEditorFocused(focused, editorState);
        // turn off auto-cap behavior when input focus has been explicitly requested.
        m_page->autoCapEnabled(false);
    }
}

void WebAppBase::setManualEditorFocusEnabled(bool enable)
{
    if (m_page) {
        m_page->manualFocusEnabled(enable);
    }
}

void WebAppBase::setManualEditorFocus(bool focused, const PalmIME::EditorState & editorState)
{
    if (m_page) {
        m_page->manualEditorFocused(focused, editorState);
    }
}

ApplicationDescription* WebAppBase::getAppDescription()
{
	return appDescImage;
}

void WebAppBase::setAppDescription(ApplicationDescription* appDesc)
{
	if(appDescImage) {
		delete appDescImage;
		appDescImage = 0;
	}
	appDescImage = appDesc;
}

void WebAppBase::attach(WebPage* page)
{
	WebAppManager::instance()->reportAppLaunched(page->appId(),
												 page->processId());
	
	if (m_page) {
		detach();
	}

	m_page = page;
	m_page->setClient(this);

	m_appId = page->appId();
	m_processId = page->processId();

	createActivity();

	ApplicationDescription* appDesc = getAppDescription();
	if (appDesc == NULL || appDesc->id().empty() || appDesc->attributes().empty()) {
	    return;
	}

	struct json_object* root = json_tokener_parse(appDesc->attributes().c_str());

	if (!root || is_error(root))
	{
	    fprintf(stderr, "Failed to parse '%s' into a JSON string.\n",
	            appDesc->attributes().c_str());
	    return;
	}

	// parse out optional http proxy host and port app attributes
    char* httpProxyHost = NULL;
    int   httpProxyPort = 0;

    struct json_object* label = json_object_object_get(root, "proxyhost");
	if (label && !is_error(label)) {
	    httpProxyHost = json_object_get_string(label);
	}

	label = json_object_object_get(root, "proxyport");
	if (label && !is_error(label)) {
	    httpProxyPort = json_object_get_int(label);
	}

	if (httpProxyHost != NULL && httpProxyPort != 0 && m_page->webkitPage()) {
	    m_page->webkitPage()->enableHttpProxy(httpProxyHost, httpProxyPort);
	}

	// parse out optional data connection interface type attributes
    char* interfaceName = NULL;

    label = json_object_object_get(root, "forceInterface");
	if (label && !is_error(label)) {
		interfaceName = json_object_get_string(label);
	}

	if (interfaceName != NULL && m_page->webkitPage()) {
		if (0 == strcmp(interfaceName, wanIdentifierString)) {
			m_page->webkitPage()->forceNetworkInterface(Settings::LunaSettings()->wanInterfaceName.c_str());  // use cellular
		} else if (0 == strcmp(interfaceName, wifiIdentifierString)) {
			m_page->webkitPage()->forceNetworkInterface(Settings::LunaSettings()->wifiInterfaceName.c_str()); // use wifi
		} else {
			m_page->webkitPage()->forceNetworkInterface(NULL); // use any
		}
	}

	if (root && !is_error(root)) {
        json_object_put(root);
    }
}

WebPage* WebAppBase::detach()
{
	WebAppManager::instance()->reportAppClosed(m_page->appId(),
											   m_page->processId());

	destroyActivity();
	
	WebPage* p = m_page; 
	
	m_page->setClient(0);
	m_page = 0;
	
	return p;
}

void WebAppBase::markInCache(bool val)
{
	m_inCache = val;    
}

void WebAppBase::setKeepAlive(bool val)
{
	m_keepAlive = val;    
}

void WebAppBase::relaunch(const char* args, const char* launchingAppId, const char* launchingProcId)
{
	bool ret = false;
	
	if (m_page)
		ret = m_page->relaunch(args, launchingAppId, launchingProcId);

	if (!ret && isWindowed()) {
		// relaunch failed. if windowed, bring window to foreground
		focus();
	}
}

void WebAppBase::close()
{
	WebAppManager::instance()->closeAppInternal(this);
}

void WebAppBase::resizedContents(int contentsWidth, int contentsHeight)
{    
}

void WebAppBase::zoomedContents(double scaleFactor, int contentsWidth, int contentsHeight,
								int newScrollOffsetX, int newScrollOffsetY)
{
}

void WebAppBase::invalContents(int x, int y, int width, int height)
{
}

void WebAppBase::scrolledContents(int newContentsX, int newContentsY)
{    
}

void WebAppBase::uriChanged(const char* url)
{
	if (url)
		m_url = url;
	else
		m_url = std::string();
}

void WebAppBase::titleChanged(const char* title)
{    
}

void WebAppBase::statusMessage(const char* msg)
{
}

void WebAppBase::dispatchFailedLoad(const char* domain, int errorCode,
									const char* failingURL,
									const char* localizedDescription)
{

}

void WebAppBase::stagePreparing()
{
	if (G_UNLIKELY(Settings::LunaSettings()->perfTesting)) {
		g_message("SYSMGR PERF: APP PREPARE appid: %s, processid: %s, type: %s, time: %d",
				  m_page ? m_page->appId().c_str() : "unknown",
				  m_page ? m_page->processId().c_str() : "unknown",
				  WebAppFactory::nameForWindowType(Window::Type_None).c_str(),
				  Time::curTimeMs());
	}
}

void WebAppBase::stageReady()
{
	if (G_UNLIKELY(Settings::LunaSettings()->perfTesting)) {
		g_message("SYSMGR PERF: APP READY appid: %s, processid: %s, type: %s, time: %d",
				  m_page ? m_page->appId().c_str() : "unknown",
				  m_page ? m_page->processId().c_str() : "unknown",
				  WebAppFactory::nameForWindowType(Window::Type_None).c_str(),
				  Time::curTimeMs());
	}
}

void WebAppBase::createActivity()
{
	if (!m_page)
		return;

	// Setup an activity for this app if it is the top level app
	if (m_page->parent())
		return;

	LSError lsError;
	LSErrorInit(&lsError);

	json_object* payload = json_object_new_object();
	json_object* activityObj = json_object_new_object();
	json_object_object_add(activityObj, (char*) "name",
						   json_object_new_string(m_appId.c_str()));
	json_object_object_add(activityObj, (char*) "description",
						   json_object_new_string(m_processId.c_str()));
	json_object* activityTypeObj = json_object_new_object();
	json_object_object_add(activityTypeObj, (char*) "foreground",
						   json_object_new_boolean(true));
	json_object_object_add(activityObj, (char*) "type",
						   activityTypeObj);	

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
		g_critical("%s: %d. Failed in calling activity manager create: %s",
				   __PRETTY_FUNCTION__, __LINE__, lsError.message);
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
		g_critical("%s: %d. Failed in canceling activity: %s",
				   __PRETTY_FUNCTION__, __LINE__, lsError.message);
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
							   NULL, NULL, NULL, &lsError)) {
		g_critical("%s: %d. Failed in calling activity manager focus: %s",
				   __PRETTY_FUNCTION__, __LINE__, lsError.message);
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
							   NULL, NULL, NULL, &lsError)) {
		g_critical("%s: %d. Failed in calling activity manager focus: %s",
				   __PRETTY_FUNCTION__, __LINE__, lsError.message);
		LSErrorFree(&lsError);
	}

	json_object_put(payload);
}

bool WebAppBase::enableSensor(Palm::SensorType aSensorType, bool aNeedEvents)
{
    bool isSuccess = false;

#if defined(HAS_NYX)
    NYXConnectorBase::Sensor nyxSensorType = mapSensorType(aSensorType);

    if (aNeedEvents)
    {
        isSuccess = enableSensor(nyxSensorType);
    }
    else
    {
        isSuccess = disableSensor(nyxSensorType);
    }
#endif
    return isSuccess;
}

void WebAppBase::fastAccelerometerOn(bool enable)
{
#if defined(HAS_NYX)
    if (enable && enableSensor(NYXConnectorBase::SensorAcceleration))
    {
        if (m_SensorList.contains(NYXConnectorBase::SensorAcceleration))
        {
            NYXConnectorBase *pSensor = m_SensorList.value(NYXConnectorBase::SensorAcceleration);
            pSensor->setRate(NYXConnectorBase::SensorReportRateHighest);
        }
    }
#endif
}

#if defined(HAS_NYX)

void WebAppBase::destroyAllSensors()
{
    QMap<NYXConnectorBase::Sensor, QPointer<NYXConnectorBase> >::iterator itr = m_SensorList.begin();
    for (; itr != m_SensorList.end(); ++itr)
    {
        NYXConnectorBase *pSensor = itr.value();
        delete pSensor;
        pSensor = 0;
    }

    m_SensorList.clear();
}

bool WebAppBase::enableSensor(NYXConnectorBase::Sensor aSensorType)
{
    bool isSuccess = false;

    NYXConnectorBase *pSensor = 0;

    if (m_SensorList.contains(aSensorType))
    {
        pSensor = m_SensorList.value(aSensorType);
    }
    else
    {
        // Create the sensor
        pSensor = NYXConnectorBase::getSensor(aSensorType, this);
        if (pSensor)
        {
            m_SensorList.insert(aSensorType, pSensor);
        }
    }

    if (pSensor)
    {
        pSensor->setOrientationAngle(m_OrientationAngle);
        isSuccess = pSensor->on();
    }

    return isSuccess;
}

bool WebAppBase::disableSensor(NYXConnectorBase::Sensor aSensorType)
{
    bool isSuccess = false;

    if (m_SensorList.contains(aSensorType))
    {
        NYXConnectorBase *pSensor = m_SensorList.value(aSensorType);
        pSensor->scheduleDeletion();

        isSuccess = (bool) m_SensorList.remove(aSensorType);
    }

    return isSuccess;
}

void WebAppBase::NYXDataAvailable (NYXConnectorBase::Sensor aSensorType)
{
    bool eventHandled = false;

    switch (aSensorType)
    {
        case NYXConnectorBase::SensorAcceleration:
        {
            eventHandled = sendAccelerationEvent();
            break;
        }

        case NYXConnectorBase::SensorShake:
        {
            eventHandled = sendShakeEvent();
            break;
        }

        case NYXConnectorBase::SensorLogicalOrientation:
        {
            eventHandled = sendLogicalOrientationEvent();
            break;
        }

        case NYXConnectorBase::SensorBearing:
        {
            eventHandled = sendCompassEvent();
            break;
        }

        default:
        {
            g_critical("[%s : %d] : Unknown sensor data available!!!",__PRETTY_FUNCTION__, __LINE__);
            break;
        }
    }
}

bool WebAppBase::sendCompassEvent()
{
    bool eventHandled = false;

    if (m_SensorList.contains(NYXConnectorBase::SensorBearing))
    {
        NYXConnectorBase *pSensor = m_SensorList.value(NYXConnectorBase::SensorBearing);
        if (pSensor)
        {
            NYXBearingSensorConnector *pBearingSensor = static_cast<NYXBearingSensorConnector *>(pSensor);
            eventHandled = m_page->webkitView()->compassEvent(pBearingSensor->bearingMagnitude(),
                                                              pBearingSensor->trueBearing(),
                                                              pBearingSensor->confidence(),
                                                              Time::curSysTimeMs());
        }
    }
    else
    {
        g_critical("[%s : %d] : Compass sensor not found!!!",__PRETTY_FUNCTION__, __LINE__);
    }

    return eventHandled;
}

bool WebAppBase::sendAccelerationEvent()
{
    bool eventHandled = false;

    if (m_SensorList.contains(NYXConnectorBase::SensorAcceleration))
    {
        NYXConnectorBase *pSensor = m_SensorList.value(NYXConnectorBase::SensorAcceleration);
        if (pSensor)
        {
            NYXAccelerationSensorConnector *pAccelerationSensor = static_cast<NYXAccelerationSensorConnector *>(pSensor);
            eventHandled = m_page->webkitView()->accelerationEvent(pAccelerationSensor->X(),
                                                                   pAccelerationSensor->Y(),
                                                                   pAccelerationSensor->Z(),
                                                                   Time::curSysTimeMs());
        }
    }
    else
    {
        g_critical("[%s : %d] : Acceleration sensor not found!!!",__PRETTY_FUNCTION__, __LINE__);
    }

    return eventHandled;
}

bool WebAppBase::sendShakeEvent()
{
    bool eventHandled = false;

    if (m_SensorList.contains(NYXConnectorBase::SensorShake))
    {
        NYXConnectorBase *pSensor = m_SensorList.value(NYXConnectorBase::SensorShake);
        if (pSensor)
        {
            NYXShakeSensorConnector *pShakeSensor = static_cast<NYXShakeSensorConnector *>(pSensor);
            WebKitShakeEvent shakeState = mapShakeEvent(pShakeSensor->shakeState());
            if (WebKitShake_Invalid != shakeState)
            {
                eventHandled = m_page->webkitView()->shakeEvent(shakeState,
                                                                pShakeSensor->shakeMagnitude(),
                                                                Time::curSysTimeMs());
            }
        }
    }
    else
    {
        g_critical("[%s : %d] : Shake sensor not found!!!",__PRETTY_FUNCTION__, __LINE__);
    }

    return eventHandled;
}

WebAppBase::WebKitShakeEvent WebAppBase::mapShakeEvent(ShakeEvent::Shake aShakeState)
{
    WebKitShakeEvent shakeState = WebKitShake_Invalid;

    switch (aShakeState)
    {
        case ShakeEvent::Shake_Start:
            shakeState = WebKitShake_Start;
            break;

        case ShakeEvent::Shake_Shaking:
            shakeState = WebKitShake_Shaking;
            break;

        case ShakeEvent::Shake_End:
            shakeState = WebKitShake_End;
            break;

        default:
            break;
    }
    return shakeState;
}

bool WebAppBase::sendLogicalOrientationEvent()
{
    bool eventHandled = false;

    if (m_SensorList.contains(NYXConnectorBase::SensorLogicalOrientation))
    {
        NYXConnectorBase *pSensor = m_SensorList.value(NYXConnectorBase::SensorLogicalOrientation);
        if (pSensor)
        {
            NYXLogicalOrientationSensorConnector *pLogicalOrientationSensor = static_cast<NYXLogicalOrientationSensorConnector *>(pSensor);
            OrientationEvent *e = static_cast<OrientationEvent *>(pLogicalOrientationSensor->getQSensorData());
            if (e)
            {
                /**
                  * Get the current Screen Orientation and Map it to the Card/Window Orientation
                  * Remember, we need to do some post processing for emulated cards before we give
                  * it back to the WebApp
                  * WebAppManager::instance()->orientation() gives the current Screen Orientation
                  * which is already post processed and ready to consume for a particular device.
                  * i.e. For Topaz it's already mapped to current orientation. (HostArmTopaz.cpp)
                  */

                Event::Orientation winOrient = (Event::Orientation) e->orientation();

                WebKitOrientationEvent orientationState = mapOrientationEvent((OrientationEvent::Orientation)winOrient);

                if (WebKitOrientation_Invalid != orientationState)
                {
                    eventHandled = m_page->webkitView()->orientationChangeEvent(orientationState,
                                                                                e->pitch(),
                                                                                e->roll(),
                                                                                Time::curSysTimeMs());
                }
                delete e;
            }
        }
    }
    else
    {
        g_critical("[%s : %d] : Logical Orientation sensor not found!!!",__PRETTY_FUNCTION__, __LINE__);
    }

    return eventHandled;
}

WebAppBase::WebKitOrientationEvent WebAppBase::mapOrientationEvent(OrientationEvent::Orientation aOrientation)
{
    WebKitOrientationEvent orientationState = WebKitOrientation_Invalid;

    switch (aOrientation)
    {
        case OrientationEvent::Orientation_FaceUp:
            orientationState = WebKitOrientation_FaceUp;
            break;

        case OrientationEvent::Orientation_FaceDown:
            orientationState = WebKitOrientation_FaceDown;
            break;

        case OrientationEvent::Orientation_Up:
            orientationState = WebKitOrientation_Up;
            break;

        case OrientationEvent::Orientation_Down:
            orientationState = WebKitOrientation_Down;
            break;

        case OrientationEvent::Orientation_Left:
            orientationState = WebKitOrientation_Left;
            break;

        case OrientationEvent::Orientation_Right:
            orientationState = WebKitOrientation_Right;
            break;

        default:
            break;
    }

    return orientationState;
}

NYXConnectorBase::Sensor WebAppBase::mapSensorType(Palm::SensorType aSensorType)
{
    NYXConnectorBase::Sensor mappedSensor = NYXConnectorBase::SensorIllegal;

    switch(aSensorType)
    {
        case Palm::SensorAcceleration:
        {
            mappedSensor = NYXConnectorBase::SensorAcceleration;
            break;
        }

        case Palm::SensorOrientation:
        {
            // Current JS APIs expect Orientation, roll & pitch events.
            // So, map this sensor to Logical Orientation
            mappedSensor = NYXConnectorBase::SensorLogicalOrientation;
            break;
        }

        case Palm::SensorShake:
        {
            mappedSensor = NYXConnectorBase::SensorShake;
            break;
        }

        case Palm::SensorBearing:
        {
            mappedSensor = NYXConnectorBase::SensorBearing;
            break;
        }

        default:
        {
            g_critical("[%s : %d] : Mustn't have reached here : Sensor Type : [%d]", __PRETTY_FUNCTION__, __LINE__, aSensorType);
            break;
        }
    }

    return mappedSensor;
}
#endif
