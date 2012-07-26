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
#if defined (TARGET_DEVICE)
#include <lunaprefs.h>
#endif
#include <lunaservice.h>

#define AMBIENT_LIGHT_SENSOR_ID "com.palm.ambientLightSensor"

#define INTERVAL_SLOW  (1000)

#define INTERVAL_FAST (100)

#define ALS_CALIBRATION_TOKEN   "com.palm.properties.ALSCal"

AmbientLightSensor* AmbientLightSensor::m_instance = NULL;

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


#if defined (MACHINE_CASTLE)

    m_alsEnabled = true; 

    m_alsBorder[0] = 10  * ALS_SAMPLE_SIZE; 
    m_alsBorder[1] = 200 * ALS_SAMPLE_SIZE;

    m_alsMargin[0] = 2 * ALS_SAMPLE_SIZE;
    m_alsMargin[1] = (m_alsBorder[1] / 10) > (ALS_SAMPLE_SIZE * 2) ?  m_alsBorder[1] / 10 : ALS_SAMPLE_SIZE * 2;

#else
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

#endif

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
#if defined (TARGET_DEVICE)
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
#if !defined (MACHINE_CASTLE)
    m_alsRegion = ALS_REGION_INDOOR;
#endif

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
#if defined (MACHINE_CASTLE)
    return updateAlsCastle (intensity);
#else
    if (Settings::LunaSettings()->enableAls)
        return updateAls (intensity);
    else 
        return false;
#endif
}

// this function is called only on castle due to the sensor limitations
// the als region is calculated from a running average of the als values
// this causes the als region change to occur slowly
bool AmbientLightSensor::updateAlsCastle (int intensity)
{
#if defined (TARGET_DEVICE)
    LSError lserror;
    LSErrorInit(&lserror);
    bool result = true;

    int current = m_alsRegion;
    int oldPtr = m_alsPointer;

    if (m_alsSampleCount == 0)
    {
        // the first sample comes is, assume others will be close
        int i = 0;
        for ( ; i < m_alsSamplesNeeded ; i++)
            m_alsValue[i] = intensity;
        m_alsSum = intensity * m_alsSamplesNeeded;
    }
    else
    {
        m_alsSum -= m_alsValue[m_alsPointer];
        m_alsValue[m_alsPointer] = intensity;
        m_alsSum += intensity;
    }

    if (m_alsSampleCount < m_alsSamplesNeeded)
    {
        m_alsSampleCount++;
        if (m_alsSampleCount == m_alsSamplesNeeded)
        {
            g_debug ("resetting ALS to sample at 1sec");

            // switch to the slow mode
            InputControl* ic = HostBase::instance()->getInputControlALS();
            if (NULL != ic)
            {
                if (!ic->setRate(NYX_REPORT_RATE_LOW))
                    return false;
            }
        }
    }

    m_alsPointer = (m_alsPointer + 1) % m_alsSamplesNeeded;

    if (m_alsDisabled > 0)
    {
        m_alsRegion = ALS_REGION_UNDEFINED;
        g_debug ("%s: reported light level of %i (%i) [region set to default by subscription]", __PRETTY_FUNCTION__, intensity, m_alsSum / m_alsSamplesNeeded);

        goto end;
    }

    if (!m_alsEnabled)
    {
        m_alsRegion = ALS_REGION_UNDEFINED;
        g_debug ("%s: reported light level of %i (%i) [device not calibrated]", __PRETTY_FUNCTION__, intensity, m_alsSum / m_alsSamplesNeeded);

        goto end;
    }

    if (m_alsSum < m_alsBorder[0])
    {
        if (current == ALS_REGION_INDOOR && m_alsSum >= m_alsBorder[0] - m_alsMargin[0])
            m_alsRegion = ALS_REGION_INDOOR;
        else
            m_alsRegion = ALS_REGION_DARK;
    }
    else if (m_alsSum < m_alsBorder[1])
    {
        if (current == ALS_REGION_DARK && m_alsSum < m_alsBorder[0] + m_alsMargin[0])
            m_alsRegion = ALS_REGION_DARK;
        else if (current == ALS_REGION_OUTDOOR && m_alsSum >= m_alsBorder[1] - m_alsMargin[1])
            m_alsRegion = ALS_REGION_OUTDOOR;
        else
            m_alsRegion = ALS_REGION_INDOOR;
    }
    else
    {
        if (current == ALS_REGION_INDOOR && m_alsSum < m_alsBorder[1] + m_alsMargin[1])
            m_alsRegion = ALS_REGION_INDOOR;
        else
            m_alsRegion = ALS_REGION_OUTDOOR;
    }

end:

    if (m_alsSubscriptions > 0)
    {
        gchar *status = g_strdup_printf ("{\"returnValue\":true,\"current\":%i,\"average\":%i,\"region\":%i}",
                m_alsValue[oldPtr], m_alsSum / m_alsSamplesNeeded, m_alsRegion);

        if (NULL != status)
            result = LSSubscriptionReply(m_service, "/control/status", status, &lserror);
        if(!result)
        {
            LSErrorPrint (&lserror, stderr);
            LSErrorFree (&lserror);
        }
        g_free(status);
    }

    // if there was no change return false, no need to update anything
    return  (m_alsRegion != current);
#else
    return false;
#endif
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
#if defined (TARGET_DEVICE)
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

bool AmbientLightSensor::controlStatus(LSHandle *sh, LSMessage *message, void *ctx)
{
#if defined (TARGET_DEVICE)
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

