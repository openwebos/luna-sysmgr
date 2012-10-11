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




#include "AmbientLightSensor.h"

#include "Common.h"
#include "HostBase.h"
#include "JSONUtils.h"
#include "Settings.h"
#include "SystemService.h"
#include "Time.h"
#include "WindowServer.h"

#include <cjson/json.h>
#include <glib.h>
#if defined(HAS_LUNA_PREF)
#include <lunaprefs.h>
#endif
#include <lunaservice.h>

#define AMBIENT_LIGHT_SENSOR_ID "com.palm.ambientLightSensor"

#define INTERVAL_SLOW  (1000)

#define INTERVAL_FAST (100)

#define ALS_CALIBRATION_TOKEN   "com.palm.properties.ALSCal"

AmbientLightSensor* AmbientLightSensor::m_instance = NULL;

/*! \page com_palm_ambient_light_sensor_control Service API com.palm.ambientLightSensor/control/
 *  Public methods:
 *  - \ref com_palm_ambient_light_sensor_control_status
 */
static LSMethod alsMethods[] = {
    {"status", AmbientLightSensor::controlStatus},
    {},
};

AmbientLightSensor::AmbientLightSensor ()
    : m_service(NULL)
    , m_alsEnabled(false)
    , m_alsIsOn(false)
    , m_alsPointer(0)
    , m_alsRegion(ALS_REGION_UNDEFINED)
    , m_alsSum(0)
    , m_alsLastOff(0)
    , m_alsDisplayOn(false)
    , m_alsSubscriptions(0)
    , m_alsDisabled(0)
    , m_alsHiddOnline(false)
    , m_alsFastRate(false)
    , m_alsSampleCount(0)
    , m_alsCountInRegion(0)
    , m_alsSamplesNeeded (ALS_INIT_SAMPLE_SIZE)
    , m_alsLastSampleTs (0)
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    GMainLoop* mainLoop = HostBase::instance()->mainLoop();

    result = LSRegister(AMBIENT_LIGHT_SENSOR_ID, &m_service, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    result = LSRegisterCategory (m_service, "/control", alsMethods, NULL, NULL, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    result = LSCategorySetData (m_service, "/control", this, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    result = LSSubscriptionSetCancelFunction(m_service, AmbientLightSensor::cancelSubscription, this, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    result = LSGmainAttach(m_service, mainLoop, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree(&lserror);
    }

    result = LSRegisterServerStatus(m_service, "com.palm.hidd", AmbientLightSensor::hiddServiceNotification, this, &lserror);
    if (!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    if (Settings::LunaSettings()->enableAls) {
	m_alsEnabled = true; 

	g_warning ("ALSCal token found, expecting lux values in light events");


	m_alsBorder[ALS_REGION_UNDEFINED] = -1;
	m_alsBorder[ALS_REGION_DARK] = 6;
	m_alsBorder[ALS_REGION_DIM] = 100;
	m_alsBorder[ALS_REGION_INDOOR] = 1000;
	m_alsBorder[ALS_REGION_OUTDOOR] = INT_MAX;

	// margins are higher at lower lux values
	m_alsMargin[ALS_REGION_UNDEFINED] = 0;
	m_alsMargin[ALS_REGION_DARK] = 4;
	m_alsMargin[ALS_REGION_DIM] = 10;
	m_alsMargin[ALS_REGION_INDOOR] = 100;
	m_alsMargin[ALS_REGION_OUTDOOR] = 0;

    }
    else {
        g_warning ("%s: ALS is not enabled", __PRETTY_FUNCTION__); 

    }

    m_instance = this;

    g_debug ("%s started", __PRETTY_FUNCTION__);
}

AmbientLightSensor* AmbientLightSensor::instance (void)
{
    return AmbientLightSensor::m_instance;
}

AmbientLightSensor::~AmbientLightSensor()
{    
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

	result = LSUnregister(m_service, &lserror);
    if (!result)
    {
        g_message ("%s: failed at %s with message %s", __PRETTY_FUNCTION__, lserror.func, lserror.message);
        LSErrorFree(&lserror);
    }
}

int AmbientLightSensor::getCurrentRegion ()
{
    return m_alsRegion;
}

bool AmbientLightSensor::start ()
{
    m_alsDisplayOn = true;
    return on ();
}

bool AmbientLightSensor::stop ()
{
    m_alsDisplayOn = false;
    return off ();
}

bool AmbientLightSensor::on ()
{
#if defined(TARGET_DEVICE)
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    // if display is off do not bother to enable the 
    // als sensor
    if (!m_alsDisplayOn)
        return true;

    // if it is already one do not bother to enable it
    if (m_alsIsOn)
        return true;

    // if we are not calibrated and there are no subscriptions
    // do not enable it
    if (!m_alsEnabled)
        return true;

    m_alsIsOn = true;

    int timeSinceLastReading = Time::curTimeMs() - m_alsLastOff;

    m_alsSampleCount = 0;
    m_alsCountInRegion = 0;
    m_alsSamplesNeeded = ALS_INIT_SAMPLE_SIZE;
    m_alsSampleList.clear();
    m_alsRegion = ALS_REGION_INDOOR;

    /* fine-tuning support for NYX */
    InputControl* ic = HostBase::instance()->getInputControlALS();
    if (NULL != ic)
    {
        g_debug ("%s: ALS on!", __PRETTY_FUNCTION__);
        if (!ic->setRate(NYX_REPORT_RATE_HIGH))
            return false;
        m_alsFastRate = true;
        return ic->on();
    }
#endif
    return true;
}

bool AmbientLightSensor::off ()
{
    LSError lserror;
    LSErrorInit(&lserror);
    bool result;

    if (!m_alsIsOn)
        return true;

    if (m_alsEnabled)
    {
        if (m_alsDisplayOn)
            return true;
    }
    else
    {
        if (m_alsDisplayOn && m_alsSubscriptions > 0)
            return true;
    }

    m_alsIsOn = false;

    m_alsLastOff = Time::curTimeMs();

    /* fine-tuning support for NYX */
    InputControl* ic = HostBase::instance()->getInputControlALS();
    if (NULL != ic)
    {
        g_debug ("%s: ALS off!", __PRETTY_FUNCTION__);
        return ic->off();
    }
    return true;
}

bool AmbientLightSensor::update (int intensity)
{
    if (Settings::LunaSettings()->enableAls)
        return updateAls (intensity);
    else 
        return false;
}

bool sortIncr (int32_t alsVal1, int32_t alsVal2) 
{
    if (alsVal1 > alsVal2)
	return false;
    return true;
}

// this is the als region estimation for the newer sensors
// the als region is estimated from  a fixed number of samples (m_alsSamplesNeeded).
// once the als region is determined, all incoming values that fall in the current region are discarded
// if an incoming value is outside the current region, we collect the samples again and re-estimate the region
// this allows the als region to move directly to the current light condition.

bool AmbientLightSensor::updateAls(int intensity)
{
#if defined(TARGET_DEVICE)
    LSError lserror;
    LSErrorInit(&lserror);
    bool result = true;

    int current = m_alsRegion;

    if (m_alsDisabled > 0) {
        m_alsRegion = ALS_REGION_UNDEFINED;
        g_debug(
                "%s: reported light level of %d [region set to default by subscription]",
                __PRETTY_FUNCTION__, intensity);

        goto end;
    }

    if (!m_alsEnabled) {
        m_alsRegion = ALS_REGION_UNDEFINED;
        g_debug("%s: reported light level of %d [device not calibrated]",
                __PRETTY_FUNCTION__, intensity);

        goto end;
    }

    if (intensity < 0) {
        g_warning("%s: invalid intensity %d", __PRETTY_FUNCTION__, intensity);
        return false;
    }

    if (m_alsRegion < ALS_REGION_UNDEFINED || m_alsRegion > ALS_REGION_OUTDOOR) {
        g_warning("%s: current region is invalid, resetting to indoor",
                __PRETTY_FUNCTION__);
        m_alsRegion = ALS_REGION_INDOOR;
    }

    if (m_alsSampleCount == 0)
    {
        m_alsSum = 0;
    }

    if (m_alsSampleCount == m_alsSamplesNeeded) {
        m_alsSum -= m_alsSampleList.front();
        m_alsSampleList.pop_front();
        m_alsSampleCount--;
    }

    // start collecting samples
    if (m_alsSampleCount < m_alsSamplesNeeded) {
        // g_debug("%s: received sample %d", __PRETTY_FUNCTION__, intensity);
        // maintaining a running sum of the last m_alsSamplesNeeded number of samples
        // also maintaining the last m_alsSamplesNeeded values in an array
        m_alsSampleList.push_back(intensity);
        m_alsSum += intensity;
        m_alsSampleCount++;
    }

    if (intensity < m_alsBorder[m_alsRegion - 1] - m_alsMargin[m_alsRegion - 1] || intensity > m_alsBorder[m_alsRegion] + m_alsMargin[m_alsRegion])
    {
        if (!m_alsFastRate)
        {
            m_alsCountInRegion = 0;
            g_debug ("resetting ALS to sample at fast rate");
            // switch to the slow mode
            InputControl* ic = HostBase::instance()->getInputControlALS();
            if (NULL != ic)
            {
                if (!ic->setRate(NYX_REPORT_RATE_HIGH))
                    return false;
            }
            m_alsFastRate = true;
        }
    } else {
        if (m_alsCountInRegion == ALS_INIT_SAMPLE_SIZE)
        {
            if (m_alsFastRate) {
                g_debug ("resetting ALS to sample at slow rate");
                // switch to the slow mode
                InputControl* ic = HostBase::instance()->getInputControlALS();
                if (NULL != ic)
                {
                    if (!ic->setRate(NYX_REPORT_RATE_LOW))
                        return false;
                }
                m_alsFastRate = false;
            }
        }
        if (m_alsCountInRegion <= ALS_INIT_SAMPLE_SIZE)
            m_alsCountInRegion++;
    }

    if (m_alsSampleCount == m_alsSamplesNeeded) {
        // sample count is now the required sample size, estimate ALS region
        while (m_alsRegion > ALS_REGION_DARK && (m_alsSum / ALS_SAMPLE_SIZE)
                < (m_alsBorder[m_alsRegion - 1] - m_alsMargin[m_alsRegion - 1])) {
            --m_alsRegion;
        }

        while (m_alsRegion < ALS_REGION_OUTDOOR && (m_alsSum / ALS_SAMPLE_SIZE)
                > (m_alsBorder[m_alsRegion] + m_alsMargin[m_alsRegion])) {
            ++m_alsRegion;
        }
    }

end:

    if (m_alsSubscriptions > 0) {
        gchar *status = g_strdup_printf(
                "{\"returnValue\":true,\"current\":%i,\"region\":%i}",
                intensity, m_alsRegion);

        if (NULL != status)
            result = LSSubscriptionReply(m_service, "/control/status", status,
                    &lserror);
        if (!result) {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }
        g_free(status);
    }

    // if there was no change return false, no need to update anything
    return (m_alsRegion != current);
#else
    return false;
#endif
}

/*!
\page com_palm_ambient_light_sensor_control
\n
\section com_palm_ambient_light_sensor_control_status status

\e Public.

com.palm.ambientLightSensor/control/status

Get status and optionally enable or disable the ambient light sensor.

\subsection com_palm_ambient_light_sensor_control_status_syntax Syntax:
\code
{
    "subscribe": boolean,
    "disableALS": boolean
}
\endcode

\param subscribe Set to true to receive status updates.
\param disableALS If \e subscribe is set to true, set this to true to disable the ambient light sensor.

\subsection com_palm_ambient_light_sensor_control_status_returns_call Returns for a call:
\code
{
    "returnValue": boolean,
    "current": int,
    "average": int,
    "disabled": boolean,
    "subscribed": boolean
}
\endcode

\param returnValue Indicates if the call was succesful.
\param current Current value of the ambient light sensor.
\param average Average value of the ambient light sensor.
\param disabled True if ambient light sensor is disabled.
\param subscribed True if subscribed to receive status updates.

\subsection com_palm_ambient_light_sensor_control_status_returns_status Returns for status updates:
\code
{
    "returnValue": boolean,
    "current": int,
    "region": int
}
\endcode

\param returnValue Indicates if the call was succesful.
\param current Current value of the ambient light sensor.
\param region A value between 0-4 describing the amount of ambient light:
\li 0: Undefined, when the sensor is disabled.
\li 1: Dark
\li 2: Dim
\li 3: Indoor
\li 4: Outdoor

\subsection com_palm_ambient_light_sensor_control_status_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.ambientLightSensor/control/status '{ "subscribe": true, "disableALS": false }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "current": 6,
    "average": 187,
    "disabled": true,
    "subscribed": true
}
\endcode

Example status updates:
\code
{
    "returnValue": true,
    "current": 184,
    "region": 3
}
{
    "returnValue": true,
    "current": 179,
    "region": 3
}
{
    "returnValue": true,
    "current": 171,
    "region": 3
}
{
    "returnValue": true,
    "current": 66,
    "region": 3
}
\endcode
*/
bool AmbientLightSensor::controlStatus(LSHandle *sh, LSMessage *message, void *ctx)
{
#if defined(TARGET_DEVICE)
    LSError lserror;
    LSErrorInit(&lserror);
    bool result = true;

    AmbientLightSensor *als = (AmbientLightSensor*)ctx;

    // {"subscribe":boolean, "disableALS" : boolean}
    VALIDATE_SCHEMA_AND_RETURN(sh,
                               message,
                               SCHEMA_2(REQUIRED(subscribe, boolean), REQUIRED(disableALS, boolean)));

    g_debug ("%s: received '%s;", __PRETTY_FUNCTION__, LSMessageGetPayload(message));

    bool subscribed = false;

    result = LSSubscriptionProcess (sh, message, &subscribed, &lserror);
    if(!result)
    {
        LSErrorFree (&lserror);
        result = true;
        subscribed = false;
    }

    if (subscribed)
    {
        als->m_alsSubscriptions++;
        als->on ();

		bool disable = false;
		const char* str = LSMessageGetPayload(message);
		if (str) {
			json_object* root = json_tokener_parse(str);
			if (root && !is_error(root)) {
				result = true;
	    	    disable = json_object_get_boolean(json_object_object_get(root, "disableALS"));
				json_object_put(root);
			}
		}

        if (disable)
            als->m_alsDisabled++;
    }

    int ptr = (als->m_alsPointer + als->m_alsSamplesNeeded - 1) % als->m_alsSamplesNeeded;
    gchar *status = g_strdup_printf ("{\"returnValue\":true,\"current\":%i,\"average\":%i,\"disabled\":%s,\"subscribed\":%s}",
            als->m_alsValue[ptr], als->m_alsSum / als->m_alsSamplesNeeded, als->m_alsDisabled > 0 ? "true" : "false", 
            subscribed ? "true" : "false");

    if (NULL != status)
        result = LSMessageReply(sh, message, status, &lserror);
    if(!result)
    {
        LSErrorPrint (&lserror, stderr);
        LSErrorFree (&lserror);
    }

    g_free(status);
#endif
    return true;
}

bool AmbientLightSensor::cancelSubscription(LSHandle *sh, LSMessage *message, void *ctx)
{
    bool result = false;

    g_debug ("%s: category %s, method %s", __FUNCTION__, LSMessageGetCategory(message), LSMessageGetMethod(message));
    AmbientLightSensor *als = (AmbientLightSensor *)ctx;

    if (0 == strcmp (LSMessageGetMethod(message), "status") &&
        0 == strcmp (LSMessageGetCategory(message), "/control"))
    {
        als->m_alsSubscriptions--;
        if (als->m_alsSubscriptions == 0)
        {
			bool disable = false;
			const char* str = LSMessageGetPayload(message);
			if (str) {
				json_object* root = json_tokener_parse(str);
				if (root && !is_error(root)) {
					result = true;
					disable = json_object_get_boolean(json_object_object_get(root, "disableALS"));
					json_object_put(root);
				}
			}
            if (result && disable)
                als->m_alsDisabled--;
            als->off ();
        }
    }
    return true;
}

bool AmbientLightSensor::hiddServiceNotification(LSHandle *sh, const char *serviceName, bool connected, void *ctx)
{
    LSError lserror;
    LSErrorInit(&lserror);

    AmbientLightSensor *als = (AmbientLightSensor *)ctx;

    als->m_alsHiddOnline = connected;

    g_debug ("%s: received conntection for '%s' and status is %s", __PRETTY_FUNCTION__, serviceName, connected ? "up" : "down");

    return true;
}

