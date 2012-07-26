/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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

#include "NyxSensorConnector.h"

#include "NyxSensorCommonTypes.h"
#include "Settings.h"

#include <QApplication>
#include <QWidget>
#include <QMap>

#include <cjson/json.h>
#include <glib.h>

#define CHECK_ERROR(err, msg)                                                                               \
        do {                                                                                                \
             if (NYX_ERROR_NONE != err)                                                                  \
             {                                                                                              \
               g_critical("[%s : %d] : %s : Error Code -> [%d]", __PRETTY_FUNCTION__, __LINE__, msg, err);  \
               return;                                                                                      \
             }                                                                                              \
           } while(0)

/**
  * Macro is designed to safely call the NYX APIs due to the asynchronous nature of these NYX connectors
  * m_Hanlde can be closed during reading the sensor data from NYX. (i.e. readSensorData())
  */
#define SAFE_NYX_CALL(expr)                     \
        do {                                    \
              if ((m_Handle) && (!m_Finished))  \
              {                                 \
                expr;                           \
              }                                 \
           } while(0)

// Forward Declration
static void InitSensorMap();

// Idle callback for deletion
static gboolean deleteCallback(gpointer apObject)
{
    if (apObject)
    {
        NYXConnectorBase* pObj = (NYXConnectorBase*)apObject;
        delete pObj;
        apObject = 0;
    }
    return false;
}

NYXConnectorBase::NYXConnectorBase (Sensor aSensorType, nyx_device_type_t aDevType, nyx_device_id_t aDevID, NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : m_SensorType(aSensorType),
      m_NYXSensorNotifier(0),
      m_Handle(0),
      m_NYXDeviceType(aDevType),
      m_NYXDeviceId(aDevID),
      m_Observer(aObserver),
      m_SensorFD(0),
      m_CanPostEvent(bCanPostEvent),
      m_OrientationAngle(INVALID_ANGLE),
      m_Finished(false)
{
    InitSensorMap();
}

NYXConnectorBase::~NYXConnectorBase()
{
    if (m_Handle)
    {
        nyx_error_t nError = nyx_device_close(m_Handle);
        if (NYX_ERROR_NONE != nError)
        {
            g_critical("Unable to release Sensor handle : [%d]", nError);
        }

        m_Handle = 0;
    }

    m_Observer = 0;
    delete m_NYXSensorNotifier;
}

nyx_error_t NYXConnectorBase::openSensor ()
{
    nyx_error_t nError = nyx_device_open(m_NYXDeviceType, m_NYXDeviceId, &m_Handle);
    if ((NYX_ERROR_NONE != nError) || (0 == m_Handle))
    {
        g_critical("Unable to open Sensor : [%s : %d] : [NYX Sensor Type = %d]", __PRETTY_FUNCTION__, __LINE__, type());
    }

    return nError;
}

bool NYXConnectorBase::on()
{
    if (m_Handle)
    {
        nyx_error_t nError;
        nError = nyx_device_set_operating_mode(m_Handle, NYX_OPERATING_MODE_ON);
        return (nError == NYX_ERROR_NONE || nError == NYX_ERROR_NOT_IMPLEMENTED);
    }

    return false;
}

bool NYXConnectorBase::off()
{
    if (m_Handle)
    {
        nyx_error_t error;
        error = nyx_device_set_operating_mode(m_Handle, NYX_OPERATING_MODE_OFF);
        return (error == NYX_ERROR_NONE || error == NYX_ERROR_NOT_IMPLEMENTED);
    }

    return false;
}

bool NYXConnectorBase::setRate(SensorReportRate aRate)
{
    if (m_Handle)
    {
        nyx_error_t nError;
        nError = nyx_device_set_report_rate(m_Handle, (nyx_report_rate_t)aRate);
        return (nError == NYX_ERROR_NONE || nError == NYX_ERROR_NOT_IMPLEMENTED);
    }

    return false;
}

std::string NYXConnectorBase::toJSONString()
{
    std::string strJson = "";

    json_object *jsonObj = toJSONObject();

    if (jsonObj)
    {
        strJson = json_object_to_json_string(jsonObj);

        json_object_put(jsonObj);
    }

    return strJson;
}

void NYXConnectorBase::connectSensorSignalToSlot()
{
    if ((m_Handle) && (0 == m_NYXSensorNotifier))
    {
        // get the event source from the NYX
        int nError = nyx_device_get_event_source(m_Handle, &m_SensorFD);
        CHECK_ERROR(nError, "Unable to obtain Sensor Handle event_source");

        // Connect the Sensor FD from NYX to a SLOT
        m_NYXSensorNotifier = new QSocketNotifier(m_SensorFD, QSocketNotifier::Read, this);
        connect(m_NYXSensorNotifier, SIGNAL(activated(int)), this, SLOT(readSensorData(int)));
    }
}

std::vector<NYXConnectorBase::Sensor> NYXConnectorBase::getSupportedSensors()
{
    std::vector<NYXConnectorBase::Sensor> sensorList;
    QMap<int, int> sensorMap;

    nyx_device_iterator_handle_t    dev_iterator    = 0;
    nyx_device_id_t                 dev_id;
    nyx_error_t                     error;

    // Main loop, iterate over all possible device types
    for (int deviceType = NYX_DEVICE_SENSOR_FIRST; deviceType <= NYX_DEVICE_SENSOR_LAST; ++deviceType)
    {
        error = nyx_device_get_iterator((nyx_device_type_t)deviceType, NYX_FILTER_DEFAULT, &dev_iterator);

        if (NYX_ERROR_NONE != error)
            g_critical("nyx_device_get_iterator() reported an error of %d\n", error);

        /*
         * Use the iterator we just got and get a count for each of the device types
         * that we have on this device.
         */
        do
        {
            dev_id = 0;

            if (0 != dev_iterator)
            {
                error = nyx_device_iterator_get_next_id(dev_iterator, &dev_id);

                if (NYX_ERROR_NONE != error)
                    g_critical("nyx_device_get_iterator_get_next_id() reported an error of %d\n", error);
            }


            if (0 != dev_id)
            {
                if (!sensorMap.contains(deviceType))
                {
                    sensorMap.insert(deviceType, 1);

                    NYXConnectorBase::Sensor mappedSensor = MapFromNYX((nyx_device_type_t)deviceType);
                    if (NYXConnectorBase::SensorIllegal != mappedSensor)
                    {
                        sensorList.push_back(mappedSensor);
                    }
                }
            }
        } while (0 != dev_id);

        /* Release the iterator if we are one */
        if (0 != dev_iterator)
        {
            error = nyx_device_release_iterator(dev_iterator);
            if (NYX_ERROR_NONE != error)
                g_critical("nyx_device_release_iterator() reported an error of %d\n", error);
        }

        dev_iterator = 0;
    }

    return sensorList;
}

NYXConnectorBase::Sensor NYXConnectorBase::MapFromNYX(nyx_device_type_t aNYXDevType)
{
    NYXConnectorBase::Sensor mappedType = NYXConnectorBase::SensorIllegal;

    switch(aNYXDevType)
    {
        case NYX_DEVICE_SENSOR_ACCELERATION:
        {
            mappedType = NYXConnectorBase::SensorAcceleration;
            break;
        }

        case NYX_DEVICE_SENSOR_ALS:
        {
            mappedType = NYXConnectorBase::SensorALS;
            break;
        }

        case NYX_DEVICE_SENSOR_ANGULAR_VELOCITY:
        {
            mappedType = NYXConnectorBase::SensorAngularVelocity;
            break;
        }

        case NYX_DEVICE_SENSOR_BEARING:
        {
            mappedType = NYXConnectorBase::SensorBearing;
            break;
        }

        case NYX_DEVICE_SENSOR_GRAVITY:
        {
            mappedType = NYXConnectorBase::SensorGravity;
            break;
        }

        case NYX_DEVICE_SENSOR_LINEAR_ACCELERATION:
        {
            mappedType = NYXConnectorBase::SensorLinearAcceleration;
            break;
        }

        case NYX_DEVICE_SENSOR_MAGNETIC_FIELD:
        {
            mappedType = NYXConnectorBase::SensorMagneticField;
            break;
        }

        case NYX_DEVICE_SENSOR_ORIENTATION:
        {
            mappedType = NYXConnectorBase::SensorOrientation;
            break;
        }

        case NYX_DEVICE_SENSOR_PROXIMITY:
        {
            mappedType = NYXConnectorBase::SensorScreenProximity;
            break;
        }

        case NYX_DEVICE_SENSOR_ROTATION:
        {
            mappedType = NYXConnectorBase::SensorRotation;
            break;
        }

        case NYX_DEVICE_SENSOR_SHAKE:
        {
            mappedType = NYXConnectorBase::SensorShake;
            break;
        }

        default:
        {
            break;
        }
    }

    return mappedType;
}

typedef QMap<NYXConnectorBase::Sensor, QString> SensorMap;
static SensorMap sSensorMap;

static void InitSensorMap()
{
    static bool sInitialized = false;
    if (!sInitialized)
    {
        // Add all the available sensors
        sSensorMap.insert(NYXConnectorBase::SensorAcceleration,         QString(SensorNames::strAccelerometer()));
        sSensorMap.insert(NYXConnectorBase::SensorOrientation,          QString(SensorNames::strOrientation()));
        sSensorMap.insert(NYXConnectorBase::SensorShake,                QString(SensorNames::strShake()));
        sSensorMap.insert(NYXConnectorBase::SensorALS,                  QString(SensorNames::strALS()));
        sSensorMap.insert(NYXConnectorBase::SensorAngularVelocity,      QString(SensorNames::strAngularVelocity()));
        sSensorMap.insert(NYXConnectorBase::SensorBearing,              QString(SensorNames::strBearing()));
        sSensorMap.insert(NYXConnectorBase::SensorGravity,              QString(SensorNames::strGravity()));
        sSensorMap.insert(NYXConnectorBase::SensorLinearAcceleration,   QString(SensorNames::strLinearAcceleration()));
        sSensorMap.insert(NYXConnectorBase::SensorMagneticField,        QString(SensorNames::strMagneticField()));
        sSensorMap.insert(NYXConnectorBase::SensorScreenProximity,      QString(SensorNames::strScreenProximity()));
        sSensorMap.insert(NYXConnectorBase::SensorRotation,             QString(SensorNames::strRotation()));
        sSensorMap.insert(NYXConnectorBase::SensorLogicalOrientation,   QString(SensorNames::strLogicalDeviceOrientation()));
        sSensorMap.insert(NYXConnectorBase::SensorLogicalMotion,        QString(SensorNames::strLogicalDeviceMotion()));

        sInitialized = true;
    }
}

std::string NYXConnectorBase::getSupportedSensors(bool bJson)
{
    std::string                             strJson     = "";
    std::vector<NYXConnectorBase::Sensor>   sensorList  = getSupportedSensors();

    json_object *jsonSensorList = json_object_new_array();

    for (unsigned int nCounter = 0; nCounter < sensorList.size(); ++nCounter)
    {
        if (sSensorMap.contains(sensorList[nCounter]))
        {
            json_object_array_add(jsonSensorList, json_object_new_string(sSensorMap.value(sensorList[nCounter]).toStdString().c_str()));
        }
        else
        {
            g_critical("[%s : %d] : This shouldn't have happened.!!!", __PRETTY_FUNCTION__, __LINE__);
        }
    }

    strJson = json_object_to_json_string(jsonSensorList);

    json_object_put (jsonSensorList);

    return strJson;
}

NYXConnectorBase* NYXConnectorBase::getSensor (Sensor aSensorType, NYXConnectorObserver *aObserver, bool bCanPostEvent)
{
    NYXConnectorBase *pSensorConnector = 0;

    switch (aSensorType)
    {
        case SensorAcceleration:
        {
            pSensorConnector = new NYXAccelerationSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorALS:
        {
            pSensorConnector = new NYXAlsSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorAngularVelocity:
        {
            pSensorConnector = new NYXAngularVelocitySensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorBearing:
        {
            pSensorConnector = new NYXBearingSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorGravity:
        {
            pSensorConnector =  new NYXGravitySensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorLinearAcceleration:
        {
            pSensorConnector =  new NYXLinearAccelearationSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorMagneticField:
        {
            pSensorConnector =  new NYXMagneticFieldSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorOrientation:
        {
            pSensorConnector =  new NYXOrientationSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorScreenProximity:
        {
            pSensorConnector = new NYXScreenProximitySensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorRotation:
        {
            pSensorConnector =  new NYXRotationSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorShake:
        {
            pSensorConnector =  new NYXShakeSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorLogicalAccelerometer:
        {
            pSensorConnector = new NYXLogicalAccelerometerSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorLogicalOrientation:
        {
            pSensorConnector = new NYXLogicalOrientationSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorLogicalDeviceOrientation:
        {
            pSensorConnector = new NYXLogicalDeviceOrientationSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        case SensorLogicalMotion:
        {
            pSensorConnector = new NYXLogicalDeviceMotionSensorConnector(aObserver, bCanPostEvent);
            break;
        }

        default:
        {
            g_critical("Asked to open Invalid Sensor Type. : [%s : %d]", __PRETTY_FUNCTION__, __LINE__);
            break;
        }
    }

    if (pSensorConnector)
    {
        if (NYX_ERROR_NONE == pSensorConnector->openSensor())
        {
            pSensorConnector->connectSensorSignalToSlot();
        }
        else
        {
            delete pSensorConnector;
            pSensorConnector = 0;
            g_critical("Unable to open the requested sensor type : [%s : %d] : [SensorType = %d]", __PRETTY_FUNCTION__, __LINE__, aSensorType);
        }
    }

    return pSensorConnector;
}

void NYXConnectorBase::callObserver(bool aShouldEmit)
{
    if (!m_Finished)
    {
        if (aShouldEmit)
        {
            Q_EMIT sensorDataAvailable();
        }

        if ((canPostEvent()) && (m_Observer))
        {
            m_Observer->NYXDataAvailable(type());
        }
    }
}

void NYXConnectorBase::scheduleDeletion()
{
    if (!m_Finished)
    {
        // Mark the sensor as finished (i.e. no longer usable)
        m_Finished = true;

        // Turn off the sensor
        off();

        // DFISH-30217: Use timeout source at normal priority instead of an idle source
        // at idle priority, otherwise the idle callback gets queued behind the spewing
        // sensor and the sensor never gets destroyed and the spewing sensor causes
        // idle sources to never fire
        //g_idle_add(deleteCallback, (gpointer)this);
        g_timeout_add(0, deleteCallback, (gpointer) this);
    }
}

/**
 * Acceleration Sensor Connector
 */
NYXAccelerationSensorConnector::NYXAccelerationSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorAcceleration, NYX_DEVICE_SENSOR_ACCELERATION, "Default", aObserver, bCanPostEvent)
{
    memset(&m_AccelerationData, 0x00, sizeof(nyx_sensor_acceleration_event_item_t));
}

void NYXAccelerationSensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain NYXAccelerationSensorConnector event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nError = nyx_sensor_acceleration_event_get_item(eventHandle, &m_AccelerationData);
        CHECK_ERROR(nError, "Unable to obtain Acceleration event items");

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release Acceleration event");

        postProcessSensorData();
        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

void NYXAccelerationSensorConnector::postProcessSensorData()
{
    AccelerationEvent *event = static_cast<AccelerationEvent*>(getQSensorData());

    if (90 == m_OrientationAngle)
    {
        m_AccelerationData.x = event->y();
        m_AccelerationData.y = event->x();
    }
    else if (180 == m_OrientationAngle)
    {
        m_AccelerationData.x = -(event->x());
        m_AccelerationData.y = -(event->y());
    }
    else if (270 == m_OrientationAngle)
    {
        m_AccelerationData.x = -(event->y());
        m_AccelerationData.y = -(event->x());
    }

    delete event;
}

json_object* NYXAccelerationSensorConnector::toJSONObject()
{
    json_object* jsonAccelerationValue = json_object_new_object();

    // Add x,y & z values
    json_object_object_add(jsonAccelerationValue, NYXJsonStringConst::strX(), json_object_new_double(X()));
    json_object_object_add(jsonAccelerationValue, NYXJsonStringConst::strY(), json_object_new_double(Y()));
    json_object_object_add(jsonAccelerationValue, NYXJsonStringConst::strZ(), json_object_new_double(Z()));

    // Wrap above values to another json object
    json_object* jsonAccelerationObject = json_object_new_object();
    json_object_object_add(jsonAccelerationObject, SensorNames::strAccelerometer(), jsonAccelerationValue);

    return jsonAccelerationObject;
}

QEvent* NYXAccelerationSensorConnector::getQSensorData()
{
    AccelerationEvent *e = new AccelerationEvent (X(),
                                                  Y(),
                                                  Z());

    return e;
}

/**
 * ALS Sensor Connector
 */
NYXAlsSensorConnector::NYXAlsSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorALS, NYX_DEVICE_SENSOR_ALS, "Default", aObserver, bCanPostEvent),
      m_LightIntensity (0)
{
}

void NYXAlsSensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain ALS event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nError = nyx_sensor_als_event_get_intensity(eventHandle, &m_LightIntensity);
        CHECK_ERROR(nError, "Unable to obtain ALS intensity");

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release ALS event");

        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

json_object* NYXAlsSensorConnector::toJSONObject()
{
    json_object* jsonLightIntensityValue = json_object_new_object();

    // Add lightIntensity value
    json_object_object_add(jsonLightIntensityValue, NYXJsonStringConst::strLightIntensity(), json_object_new_int(getLightIntensity()));

    // Wrap above values to another json object
    json_object* jsonALSObject = json_object_new_object();
    json_object_object_add(jsonALSObject, SensorNames::strALS(), jsonLightIntensityValue);

    return jsonALSObject;
}

QEvent* NYXAlsSensorConnector::getQSensorData()
{
    return (new AlsEvent(getLightIntensity()));
}

/**
 * Angular Velocity Sensor Connector
 */
NYXAngularVelocitySensorConnector::NYXAngularVelocitySensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorAngularVelocity, NYX_DEVICE_SENSOR_ANGULAR_VELOCITY, "Default", aObserver, bCanPostEvent)
{
    memset(&m_AngularVelocity, 0x00, sizeof(nyx_sensor_angular_velocity_event_item_t));
}

void NYXAngularVelocitySensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain Angular Velocity Sensor event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nError = nyx_sensor_angular_velocity_event_get_item(eventHandle, &m_AngularVelocity);
        CHECK_ERROR(nError, "Unable to obtain Angular Velocity data");

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release Angular Velocity event");

        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

json_object* NYXAngularVelocitySensorConnector::toJSONObject()
{
    json_object* jsonAngularVelocityValue = json_object_new_object();

    // Add x,y & z values
    json_object_object_add(jsonAngularVelocityValue, NYXJsonStringConst::strX(), json_object_new_double(X()));
    json_object_object_add(jsonAngularVelocityValue, NYXJsonStringConst::strY(), json_object_new_double(Y()));
    json_object_object_add(jsonAngularVelocityValue, NYXJsonStringConst::strZ(), json_object_new_double(Z()));

    // Wrap above values to another json object
    json_object* jsonAngularVelocityObject = json_object_new_object();
    json_object_object_add(jsonAngularVelocityObject, SensorNames::strAngularVelocity(), jsonAngularVelocityValue);

    return jsonAngularVelocityObject;
}

QEvent* NYXAngularVelocitySensorConnector::getQSensorData()
{
    AngularVelocityEvent *e = new AngularVelocityEvent(X(),
                                                       Y(),
                                                       Z());

    return e;
}

/**
 * Bearing Sensor Connector
 */
NYXBearingSensorConnector::NYXBearingSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorBearing, NYX_DEVICE_SENSOR_BEARING, "Default", aObserver, bCanPostEvent)
{
    memset(&m_Bearing, 0x00, sizeof(nyx_sensor_bearing_event_item_t));
}

void NYXBearingSensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain Bearing Sensor event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nError = nyx_sensor_bearing_event_get_item(eventHandle, &m_Bearing);
        CHECK_ERROR(nError, "Unable to obtain bearing data");

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release Bearing event");

        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

json_object* NYXBearingSensorConnector::toJSONObject()
{
    json_object* jsonBearingValues = json_object_new_object();

    json_object_object_add(jsonBearingValues, NYXJsonStringConst::strMagnetic(),    json_object_new_double(bearingMagnitude()));
    json_object_object_add(jsonBearingValues, NYXJsonStringConst::strTrueBearing(), json_object_new_double(trueBearing()));
    json_object_object_add(jsonBearingValues, NYXJsonStringConst::strConfidence(),  json_object_new_double(confidence()));

    // Wrap above values to another json object
    json_object* jsonBearingObject = json_object_new_object();
    json_object_object_add(jsonBearingObject, SensorNames::strBearing(), jsonBearingValues);

    return jsonBearingObject;
}

QEvent* NYXBearingSensorConnector::getQSensorData()
{
    CompassEvent* e = new CompassEvent (bearingMagnitude(),
                                        trueBearing(),
                                        (int)confidence());

    return e;
}

bool NYXBearingSensorConnector::setLocation(double aLatitude, double aLongitude)
{
    if (m_Handle)
    {
        nyx_error_t                     nError = NYX_ERROR_NONE;
        nyx_sensor_bearing_location_t   location;

        location.latitude   = aLatitude;
        location.longitude  = aLongitude;
        location.altitude   = 0.0;

        SAFE_NYX_CALL(nError = nyx_sensor_bearing_set_location(m_Handle, &location));

        // g_debug("Set bearing location to %f, %f, nyx-error = %d", aLatitude, aLongitude, nError);
        return (nError == NYX_ERROR_NONE || nError == NYX_ERROR_NOT_IMPLEMENTED);
    }

    return false;
}

/**
 * Gravity Sensor Connector
 */
NYXGravitySensorConnector::NYXGravitySensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorGravity, NYX_DEVICE_SENSOR_GRAVITY, "Default", aObserver, bCanPostEvent)
{
    memset(&m_Gravity, 0x00, sizeof(nyx_sensor_gravity_event_item_t));
}

void NYXGravitySensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain Gravity Sensor event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nError = nyx_sensor_gravity_event_get_item(eventHandle, &m_Gravity);
        CHECK_ERROR(nError, "Unable to obtain gravity data");

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release Gravity event");

        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

json_object* NYXGravitySensorConnector::toJSONObject()
{
    json_object* jsonGravityValues = json_object_new_object();

    // Add x,y & z values
    json_object_object_add(jsonGravityValues, NYXJsonStringConst::strX(), json_object_new_double(X()));
    json_object_object_add(jsonGravityValues, NYXJsonStringConst::strY(), json_object_new_double(Y()));
    json_object_object_add(jsonGravityValues, NYXJsonStringConst::strZ(), json_object_new_double(Z()));

    // Wrap above values to another json object
    json_object* jsonGravityObject = json_object_new_object();
    json_object_object_add(jsonGravityObject, SensorNames::strGravity(), jsonGravityValues);

    return jsonGravityObject;
}

QEvent* NYXGravitySensorConnector::getQSensorData()
{
    GravityEvent *e = new GravityEvent (X(),
                                        Y(),
                                        Z());

    return e;
}

/**
 * Linear Acceleration Sensor Connector
 */
NYXLinearAccelearationSensorConnector::NYXLinearAccelearationSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorLinearAcceleration, NYX_DEVICE_SENSOR_LINEAR_ACCELERATION, "Default", aObserver, bCanPostEvent)
{
    memset(&m_LinearAcceleration, 0x00, sizeof(nyx_sensor_linear_acceleration_event_item_t));
}

void NYXLinearAccelearationSensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain Linear Acceleration Sensor event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nError = nyx_sensor_linear_acceleration_event_get_item(eventHandle, &m_LinearAcceleration);
        CHECK_ERROR(nError, "Unable to obtain linear acceleration data");

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release linear acceleration event");

        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

json_object* NYXLinearAccelearationSensorConnector::toJSONObject()
{
    json_object* jsonLinearAccelerationValues = json_object_new_object();

    json_object_object_add(jsonLinearAccelerationValues, NYXJsonStringConst::strX(),        json_object_new_double(X()));
    json_object_object_add(jsonLinearAccelerationValues, NYXJsonStringConst::strY(),        json_object_new_double(Y()));
    json_object_object_add(jsonLinearAccelerationValues, NYXJsonStringConst::strZ(),        json_object_new_double(Z()));
    json_object_object_add(jsonLinearAccelerationValues, NYXJsonStringConst::strWorldX(),   json_object_new_double(WorldX()));
    json_object_object_add(jsonLinearAccelerationValues, NYXJsonStringConst::strWorldY(),   json_object_new_double(WorldY()));
    json_object_object_add(jsonLinearAccelerationValues, NYXJsonStringConst::strWorldZ(),   json_object_new_double(WorldZ()));

    // Wrap above values to another json object
    json_object* jsonLinearAccelerationObject = json_object_new_object();
    json_object_object_add(jsonLinearAccelerationObject, SensorNames::strLinearAcceleration(), jsonLinearAccelerationValues);

    return jsonLinearAccelerationObject;
}

QEvent* NYXLinearAccelearationSensorConnector::getQSensorData()
{
    LinearAccelerationEvent *e = new LinearAccelerationEvent (X(),
                                                              Y(),
                                                              Z(),
                                                              WorldX(),
                                                              WorldY(),
                                                              WorldZ());

    return e;
}

/**
 * Magnetic Field Sensor Connector
 */
NYXMagneticFieldSensorConnector::NYXMagneticFieldSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorMagneticField, NYX_DEVICE_SENSOR_MAGNETIC_FIELD, "Default", aObserver, bCanPostEvent)
{
    memset(&m_MagneticField, 0x00, sizeof(nyx_sensor_magnetic_field_event_item_t));
}

void NYXMagneticFieldSensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain Magnetic Field Sensor event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nError = nyx_sensor_magnetic_field_event_get_item(eventHandle, &m_MagneticField);
        CHECK_ERROR(nError, "Unable to obtain Magnetic field data");

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release magnetic field event");

        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

json_object* NYXMagneticFieldSensorConnector::toJSONObject()
{
    json_object* jsonMagneticFieldValues = json_object_new_object();

    json_object_object_add(jsonMagneticFieldValues, NYXJsonStringConst::strX(),     json_object_new_double(X()));
    json_object_object_add(jsonMagneticFieldValues, NYXJsonStringConst::strY(),     json_object_new_double(Y()));
    json_object_object_add(jsonMagneticFieldValues, NYXJsonStringConst::strZ(),     json_object_new_double(Z()));
    json_object_object_add(jsonMagneticFieldValues, NYXJsonStringConst::strRawX(),  json_object_new_double(rawX()));
    json_object_object_add(jsonMagneticFieldValues, NYXJsonStringConst::strRawY(),  json_object_new_double(rawY()));
    json_object_object_add(jsonMagneticFieldValues, NYXJsonStringConst::strRawZ(),  json_object_new_double(rawZ()));

    // Wrap above values to another json object
    json_object* jsonMagneticFieldObject = json_object_new_object();
    json_object_object_add(jsonMagneticFieldObject, SensorNames::strMagneticField(), jsonMagneticFieldValues);

    return jsonMagneticFieldObject;
}

QEvent* NYXMagneticFieldSensorConnector::getQSensorData()
{
    MagneticFieldEvent *e = new MagneticFieldEvent (X(),
                                                    Y(),
                                                    Z(),
                                                    rawX(),
                                                    rawY(),
                                                    rawZ());

    return e;
}

/**
 * Orientation Sensor Connector
 */
NYXOrientationSensorConnector::NYXOrientationSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorOrientation, NYX_DEVICE_SENSOR_ORIENTATION, "Default", aObserver, bCanPostEvent),
      m_Orientation(OrientationEvent::Orientation_Invalid)
{
}

void NYXOrientationSensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain Orientation Sensor event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nyx_sensor_orientation_event_item_t orientationData;
        nError = nyx_sensor_orientation_event_get_item(eventHandle, &orientationData);
        CHECK_ERROR((nError != NYX_ERROR_NONE), "Unable to obtain Orientation event items");

        m_Orientation = mapAccelerometerOrientation(orientationData.value);

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release Orientation event");

        postProcessSensorData();
        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

void NYXOrientationSensorConnector::postProcessSensorData()
{
    OrientationEvent *event = static_cast<OrientationEvent *>(getQSensorData());

    if (INVALID_ANGLE != m_OrientationAngle && 0 != m_OrientationAngle)
    {
        switch(event->orientation())
        {
            case OrientationEvent::Orientation_Up:
            {
                m_Orientation = OrientationEvent::Orientation_Left;
                break;
            }

            case OrientationEvent::Orientation_Down:
            {
                m_Orientation = OrientationEvent::Orientation_Right;
                break;
            }

            case OrientationEvent::Orientation_Left:
            {
                m_Orientation = OrientationEvent::Orientation_Down;
                break;
            }

            case OrientationEvent::Orientation_Right:
            {
                m_Orientation = OrientationEvent::Orientation_Up;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    delete event;
}

const char* NYXOrientationSensorConnector::toPositionString()
{
    switch(getOrientation())
    {
        case OrientationEvent::Orientation_FaceUp:
        {
            return NYXJsonStringConst::strOrientationFaceUp();
        }

        case OrientationEvent::Orientation_FaceDown:
        {
            return NYXJsonStringConst::strOrientationFaceDown();
        }

        case OrientationEvent::Orientation_Up:
        {
            return NYXJsonStringConst::strOrientationFaceForward();
        }

        case OrientationEvent::Orientation_Down:
        {
            return NYXJsonStringConst::strOrientationFaceBack();
        }

        case OrientationEvent::Orientation_Left:
        {
            return NYXJsonStringConst::strOrientationLeft();
        }

        case OrientationEvent::Orientation_Right:
        {
            return NYXJsonStringConst::strOrientationRight();
        }

        default:
        {
            return NYXJsonStringConst::strEmpty();
        }
    }
}

json_object* NYXOrientationSensorConnector::toJSONObject()
{
    json_object* jsonOrientationValue = json_object_new_object();

    json_object_object_add(jsonOrientationValue, NYXJsonStringConst::strPosition(), json_object_new_string(toPositionString()));

    // Wrap above values to another json object
    json_object* jsonOrientationObject = json_object_new_object();
    json_object_object_add(jsonOrientationObject, SensorNames::strOrientation(), jsonOrientationValue);

    return jsonOrientationObject;
}

QEvent* NYXOrientationSensorConnector::getQSensorData()
{
    OrientationEvent *e = new OrientationEvent ((OrientationEvent::Orientation)m_Orientation);

    return e;
}

/**
 * convert the accelerometer orientation from the lower level into
 * an Orientation type
 */
OrientationEvent::Orientation NYXOrientationSensorConnector::mapAccelerometerOrientation(int aValue)
{
#if defined(MACHINE_TOPAZ)
    // Temporary hack to flip around orientations till Topaz framebuffer is aligned right
    switch (aValue) {
    case NYX_SENSOR_ORIENTATION_FACE_DOWN:
        return OrientationEvent::Orientation_FaceDown;
    case NYX_SENSOR_ORIENTATION_FACE_UP:
        return OrientationEvent::Orientation_FaceUp;
    case NYX_SENSOR_ORIENTATION_RIGHT:
        return OrientationEvent::Orientation_Left;
    case NYX_SENSOR_ORIENTATION_LEFT:
        return OrientationEvent::Orientation_Right;
    case NYX_SENSOR_ORIENTATION_FACE_FORWARD:
        return OrientationEvent::Orientation_Down;
    case NYX_SENSOR_ORIENTATION_FACE_BACK:
        return OrientationEvent::Orientation_Up;
    default:
        return OrientationEvent::Orientation_Invalid;
    }
#else
    switch (aValue)
    {
        case NYX_SENSOR_ORIENTATION_FACE_DOWN:
        {
            return OrientationEvent::Orientation_FaceDown;
        }

        case NYX_SENSOR_ORIENTATION_FACE_UP:
        {
            return OrientationEvent::Orientation_FaceUp;
        }

        case NYX_SENSOR_ORIENTATION_RIGHT:
        {
            return OrientationEvent::Orientation_Right;
        }

        case NYX_SENSOR_ORIENTATION_LEFT:
        {
            return OrientationEvent::Orientation_Left;
        }

        case NYX_SENSOR_ORIENTATION_FACE_BACK:
        {
            return OrientationEvent::Orientation_Down;
        }

        case NYX_SENSOR_ORIENTATION_FACE_FORWARD:
        {
            return OrientationEvent::Orientation_Up;
        }

        default:
        {
            return OrientationEvent::Orientation_Invalid;
        }
    }
#endif
}

/**
 * Screen Proximity Sensor Connector
 */
NYXScreenProximitySensorConnector::NYXScreenProximitySensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorScreenProximity, NYX_DEVICE_SENSOR_PROXIMITY, "Screen", aObserver, bCanPostEvent),
      m_Present(0)
{
}

void NYXScreenProximitySensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain Proximity event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nError = nyx_sensor_proximity_event_get_presence(eventHandle, &m_Present);
        CHECK_ERROR(nError, "Unable to get proximity event");

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release proximity event");

        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

QEvent* NYXScreenProximitySensorConnector::getQSensorData()
{
    return (new ProximityEvent(m_Present));
}

/**
 * Rotation Sensor Connector
 */
NYXRotationSensorConnector::NYXRotationSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorRotation, NYX_DEVICE_SENSOR_ROTATION, "Default", aObserver, bCanPostEvent)
{
    memset(&m_RotationData, 0x00, sizeof(nyx_sensor_rotation_event_item_t));
}

void NYXRotationSensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain Rotation event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nError = nyx_sensor_rotation_event_get_item(eventHandle, &m_RotationData);
        CHECK_ERROR(nError, "Unable to get Rotation event");

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release Rotation event");

        postProcessSensorData();
        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

void NYXRotationSensorConnector::postProcessSensorData()
{
    RotationEvent *event = static_cast<RotationEvent *>(getQSensorData());

    if (0 == m_OrientationAngle)
    {
        m_RotationData.euler_angle.pitch = -(event->pitch());
        m_RotationData.euler_angle.roll  =   event->roll();
    }
    else if (90 == m_OrientationAngle)
    {
        m_RotationData.euler_angle.pitch = -(event->roll());
        m_RotationData.euler_angle.roll  = -(event->pitch());
    }
    else if (180 == m_OrientationAngle)
    {
        m_RotationData.euler_angle.pitch =   event->pitch();
        m_RotationData.euler_angle.roll  = -(event->roll());
    }
    else if (270 == m_OrientationAngle)
    {
        m_RotationData.euler_angle.pitch =   event->roll();
        m_RotationData.euler_angle.roll  = -(event->pitch());
    }

    delete event;
}

json_object* NYXRotationSensorConnector::toJSONObject()
{
    // Create rotationMatrix Array
    json_object* rotationMatrix = json_object_new_array();

    int nArraySize = sizeof(m_RotationData.matrix)/sizeof(m_RotationData.matrix[0]);
    for (int nCounter = 0; nCounter < nArraySize; ++nCounter)
    {
        json_object_array_add(rotationMatrix, json_object_new_double(m_RotationData.matrix[nCounter]));
    }

    //Create quaternion Vector
    json_object* quaternionVector = json_object_new_object();

    json_object_object_add(quaternionVector, NYXJsonStringConst::strW(), json_object_new_double(quaternionW()));
    json_object_object_add(quaternionVector, NYXJsonStringConst::strX(), json_object_new_double(quaternionX()));
    json_object_object_add(quaternionVector, NYXJsonStringConst::strY(), json_object_new_double(quaternionY()));
    json_object_object_add(quaternionVector, NYXJsonStringConst::strZ(), json_object_new_double(quaternionZ()));

    // Create eulerAngle Vector
    json_object* eulerAngleVector = json_object_new_object();

    json_object_object_add(eulerAngleVector, NYXJsonStringConst::strRoll(),  json_object_new_double(roll()));
    json_object_object_add(eulerAngleVector, NYXJsonStringConst::strPitch(), json_object_new_double(pitch()));
    json_object_object_add(eulerAngleVector, NYXJsonStringConst::strYaw(),   json_object_new_double(yaw()));

    // Wrap above values to another json inner object
    json_object* jsonRotationInnerObject = json_object_new_object();
    json_object_object_add(jsonRotationInnerObject, NYXJsonStringConst::strRotationMatrix(),     rotationMatrix);
    json_object_object_add(jsonRotationInnerObject, NYXJsonStringConst::strQuaternionVector(),   quaternionVector);
    json_object_object_add(jsonRotationInnerObject, NYXJsonStringConst::strEulerAngle(),         eulerAngleVector);

    // Wrap the json inner object to final object
    json_object* jsonRotationObject = json_object_new_object();
    json_object_object_add(jsonRotationObject, SensorNames::strRotation(), jsonRotationInnerObject);

    return jsonRotationObject;
}

QEvent* NYXRotationSensorConnector::getQSensorData()
{
    RotationEvent *e = new RotationEvent(m_RotationData.euler_angle.pitch,
                                         m_RotationData.euler_angle.roll);

    return e;
}

/**
 * Shake Sensor Connector
 */
NYXShakeSensorConnector::NYXShakeSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(SensorShake, NYX_DEVICE_SENSOR_SHAKE, "Default", aObserver, bCanPostEvent),
      m_ShakeState(ShakeEvent::Shake_Invalid),
      m_ShakeMagnitude(0.0)
{
}

void NYXShakeSensorConnector::readSensorData(int )
{
    nyx_event_handle_t  eventHandle = 0;
    nyx_error_t         nError      = NYX_ERROR_NONE;

    if (0 == m_Handle) return;

    SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    CHECK_ERROR(nError, "Unable to obtain Shake Sensor event handle");

    while ((!m_Finished) && (NYX_ERROR_NONE == nError) && (eventHandle) && (m_Handle))
    {
        nyx_sensor_shake_event_item_t   shakeData;
        nError = nyx_sensor_shake_event_get_item(eventHandle, &shakeData);
        CHECK_ERROR((nError != NYX_ERROR_NONE), "Unable to obtain Shake Handle event items");

        SAFE_NYX_CALL(nError = nyx_device_release_event(m_Handle, eventHandle));
        CHECK_ERROR(nError, "Unable to release Shake event");

        m_ShakeMagnitude = shakeData.magnitude;
        m_ShakeState     = mapShakeSensorEvent(shakeData.state);

        callObserver();

        eventHandle = 0;
        SAFE_NYX_CALL(nError = nyx_device_get_event(m_Handle, &eventHandle));
    }
}

const char* NYXShakeSensorConnector::toShakeStateString()
{
    switch(shakeState())
    {
        case ShakeEvent::Shake_Start:
        {
            return NYXJsonStringConst::strShakeStart();
        }

        case ShakeEvent::Shake_Shaking:
        {
            return NYXJsonStringConst::strShaking();
        }

        case ShakeEvent::Shake_End:
        {
            return NYXJsonStringConst::strShakeEnd();
        }

        default:
        {
            return "";
        }
    }
}

json_object* NYXShakeSensorConnector::toJSONObject()
{
    json_object* jsonShakeSensorValue = json_object_new_object();

    json_object_object_add(jsonShakeSensorValue, NYXJsonStringConst::strShakeState(),     json_object_new_string(toShakeStateString()));
    json_object_object_add(jsonShakeSensorValue, NYXJsonStringConst::strShakeMagnitude(), json_object_new_double(shakeMagnitude()));

    // Wrap above values to another json object
    json_object* jsonShakeObject = json_object_new_object();
    json_object_object_add(jsonShakeObject, SensorNames::strShake(), jsonShakeSensorValue);

    return jsonShakeObject;
}

QEvent* NYXShakeSensorConnector::getQSensorData()
{
    return (new ShakeEvent(m_ShakeState, m_ShakeMagnitude));
}

ShakeEvent::Shake NYXShakeSensorConnector::mapShakeSensorEvent(int aValue)
{
    switch (aValue)
    {
        case NYX_SENSOR_SHAKE_START:
            return ShakeEvent::Shake_Start;

        case NYX_SENSOR_SHAKE_SHAKING:
            return ShakeEvent::Shake_Shaking;

        case NYX_SENSOR_SHAKE_STOP:
            return ShakeEvent::Shake_End;

        case NYX_SENSOR_SHAKE_NONE:
        default:
            return ShakeEvent::Shake_Invalid;
    }
}

/**
 * NYX Logical sensor connector base class
 */
NYXLogicalSensorConnectorBase::NYXLogicalSensorConnectorBase(Sensor aSensorType, NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXConnectorBase(aSensorType, NYX_DEVICE_ILLEGAL_DEVICE, "Default", aObserver, bCanPostEvent)
{
}

NYXLogicalSensorConnectorBase::~NYXLogicalSensorConnectorBase()
{
}

bool NYXLogicalSensorConnectorBase::on()
{
    NYXConnectorBase  *pSensor = 0;
    std::vector<NYXConnectorBase *>::iterator it;

    for (it = m_SensorList.begin() ; it < m_SensorList.end(); ++it )
    {
        pSensor = *it;

        if ((pSensor) && (!(pSensor->on())))
        {
            g_critical("Unable to start a Sensor : [%s : %d] : [Sensor Type = %d]]", __PRETTY_FUNCTION__, __LINE__, pSensor->type());
        }
    }

    return true;
}

bool NYXLogicalSensorConnectorBase::off()
{
    NYXConnectorBase  *pSensor = 0;
    std::vector<NYXConnectorBase *>::iterator it;

    for (it = m_SensorList.begin() ; it < m_SensorList.end(); ++it )
    {
        pSensor = *it;

        if ((pSensor) && (!(pSensor->off())))
        {
            g_critical("Unable to stop a Sensor : [%s : %d] : [Sensor Type = %d]]", __PRETTY_FUNCTION__, __LINE__, pSensor->type());
        }
    }

    return true;
}

bool NYXLogicalSensorConnectorBase::setRate(SensorReportRate aRate)
{
    NYXConnectorBase  *pSensor = 0;
    std::vector<NYXConnectorBase *>::iterator it;

    for (it = m_SensorList.begin() ; it < m_SensorList.end(); ++it )
    {
        pSensor = *it;

        if ((pSensor) && (!(pSensor->setRate(aRate))))
        {
            g_critical("Unable to set rate for the Sensor : [%s : %d] : [Sensor Type = %d]]", __PRETTY_FUNCTION__, __LINE__, pSensor->type());
        }
    }

    return true;
}

void NYXLogicalSensorConnectorBase::setOrientationAngle(int aAngle)
{
    NYXConnectorBase  *pSensor = 0;
    std::vector<NYXConnectorBase *>::iterator it;

    m_OrientationAngle = aAngle;

    for (it = m_SensorList.begin() ; it < m_SensorList.end(); ++it )
    {
        pSensor = *it;

        if (pSensor)
        {
            pSensor->setOrientationAngle(aAngle);
        }
    }
}

void NYXLogicalSensorConnectorBase::scheduleDeletion()
{
    NYXConnectorBase  *pSensor = 0;
    std::vector<NYXConnectorBase *>::iterator it;

    NYXConnectorBase::scheduleDeletion();

    for (it = m_SensorList.begin() ; it < m_SensorList.end(); ++it )
    {
        pSensor = *it;

        if (pSensor)
        {
            pSensor->scheduleDeletion();
        }
    }
}

void NYXLogicalSensorConnectorBase::logicalSensorDataAvailable()
{
    if ((m_Observer) && (!m_Finished))
    {
        m_Observer->NYXDataAvailable(type());
    }
}

json_object* NYXLogicalSensorConnectorBase::toJSONObject()
{
    NYXConnectorBase   *pSensor         = 0;
    std::string         strJson         = "";

    std::vector<NYXConnectorBase *>::iterator it;

    json_object* jsonLogicalSensorInnerObjects = json_object_new_array();
    for (it = m_SensorList.begin() ; it < m_SensorList.end(); ++it )
    {
        pSensor = *it;
        if (pSensor)
        {
            json_object_array_add(jsonLogicalSensorInnerObjects, pSensor->toJSONObject());
        }
    }

    // Create the final object
    json_object* jsonLogicalSensorObject = json_object_new_object();
    json_object_object_add(jsonLogicalSensorObject, sSensorMap.value(type()).toStdString().c_str(), jsonLogicalSensorInnerObjects);

    return jsonLogicalSensorObject;
}

/**
 * Logical Accelerometer Sensor Connector
 *  - Accelerometer
 *  - Orientation
 *  - Shake
 *  - Rotation
 */
NYXLogicalAccelerometerSensorConnector::NYXLogicalAccelerometerSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXLogicalSensorConnectorBase(SensorLogicalAccelerometer, aObserver, bCanPostEvent)
{
    NYXConnectorBase *pSensor = NYXConnectorBase::getSensor(SensorAcceleration, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }

    pSensor = NYXConnectorBase::getSensor(SensorOrientation, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }

    pSensor = NYXConnectorBase::getSensor(SensorShake, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }

    pSensor = NYXConnectorBase::getSensor(SensorRotation, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }
}

QEvent* NYXLogicalAccelerometerSensorConnector::readAllSensorData()
{
    OrientationEvent::Orientation   aOrientation    = OrientationEvent::Orientation_Invalid;
    ShakeEvent::Shake               aShakeState     = ShakeEvent::Shake_Invalid;
    NYXConnectorBase               *pSensor         = 0;
    float                           aRotationPitch  = 0.0;
    float                           aRotationRoll   = 0.0;
    float                           aAccelX         = 0.0;
    float                           aAccelY         = 0.0;
    float                           aAccelZ         = 0.0;
    float                           aShakeMagnitude = 0.0;

    std::vector<NYXConnectorBase *>::iterator it;

    for (it = m_SensorList.begin() ; it < m_SensorList.end(); ++it )
    {
        pSensor = *it;
        if (pSensor)
        {
            switch (pSensor->type())
            {
                case SensorAcceleration:
                {
                    NYXAccelerationSensorConnector *pAccelSensor = static_cast<NYXAccelerationSensorConnector *>(pSensor);
                    aAccelX = pAccelSensor->X();
                    aAccelY = pAccelSensor->Y();
                    aAccelZ = pAccelSensor->Z();
                    break;
                }

                case SensorOrientation:
                {
                    NYXOrientationSensorConnector *pOrientationSensor = static_cast<NYXOrientationSensorConnector *>(pSensor);
                    aOrientation = (OrientationEvent::Orientation) pOrientationSensor->getOrientation();
                    break;
                }

                case SensorShake:
                {
                    NYXShakeSensorConnector *pShakeSensor = static_cast<NYXShakeSensorConnector *>(pSensor);
                    aShakeState     = pShakeSensor->shakeState();
                    aShakeMagnitude = pShakeSensor->shakeMagnitude();
                    break;
                }

                case SensorRotation:
                {
                    NYXRotationSensorConnector *pRotationSensor = static_cast<NYXRotationSensorConnector *>(pSensor);
                    aRotationPitch = pRotationSensor->pitch();
                    aRotationRoll  = pRotationSensor->roll();
                    break;
                }

                default:
                {
                    g_critical("Mustn't have reached here. [%s]:[%d] !!!!",__PRETTY_FUNCTION__, __LINE__);
                    break;
                }
            }
        }
    }

    AccelerometerEvent* e = new AccelerometerEvent(aOrientation,
                                                   aRotationPitch,
                                                   aRotationRoll,
                                                   aAccelX,
                                                   aAccelY,
                                                   aAccelZ,
                                                   aShakeState,
                                                   aShakeMagnitude);

    // g_debug("Accelerometer event A: floatX: %1.4f, floatY: %1.4f, floatZ: %1.4f, orientation: %d, \npitch: %3.4f, roll: %3.4f, shakeState: %d, shakeMagnitude: %1.4f\n", e->x(), e->y(), e->z(), e->orientation(), e->pitch(), e->roll(), e->shakeState(), e->shakeMagnitude());

    return e;
}

QEvent* NYXLogicalAccelerometerSensorConnector::getQSensorData()
{
    return readAllSensorData();
}

/**
 * NYX Logical Orientation sensor connector class
 *  - Orientation
 *  - Rotation
 */
NYXLogicalOrientationSensorConnector::NYXLogicalOrientationSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXLogicalSensorConnectorBase(SensorLogicalOrientation, aObserver, bCanPostEvent)
{
    NYXConnectorBase *pSensor = NYXConnectorBase::getSensor(SensorOrientation, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }

    pSensor = NYXConnectorBase::getSensor(SensorRotation, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }
}

QEvent* NYXLogicalOrientationSensorConnector::readAllSensorData()
{
    OrientationEvent::Orientation   aOrientation    = OrientationEvent::Orientation_Invalid;
    NYXConnectorBase               *pSensor         = 0;
    float                           aRotationPitch  = 0.0;
    float                           aRotationRoll   = 0.0;

    std::vector<NYXConnectorBase *>::iterator it;

    for (it = m_SensorList.begin() ; it < m_SensorList.end(); ++it )
    {
        pSensor = *it;
        if (pSensor)
        {
            switch (pSensor->type())
            {
                case SensorOrientation:
                {
                    NYXOrientationSensorConnector *pOrientationSensor = static_cast<NYXOrientationSensorConnector *>(pSensor);
                    aOrientation = (OrientationEvent::Orientation)pOrientationSensor->getOrientation();
                    break;
                }

                case SensorRotation:
                {
                    NYXRotationSensorConnector *pRotationSensor = static_cast<NYXRotationSensorConnector *>(pSensor);
                    aRotationPitch = pRotationSensor->pitch();
                    aRotationRoll  = pRotationSensor->roll();
                    break;
                }

                default:
                {
                    g_critical("Mustn't have reached here. [%s]:[%d] !!!!",__PRETTY_FUNCTION__, __LINE__);
                    break;
                }
            }
        }
    }

    OrientationEvent* e = new OrientationEvent(aOrientation,
                                               aRotationPitch,
                                               aRotationRoll);

    // g_debug("Logical OrientationEvent event : orientation: %d, pitch: %3.4f, roll: %3.4f", e->orientation(), e->pitch(), e->roll());

    return e;
}

QEvent* NYXLogicalOrientationSensorConnector::getQSensorData()
{
    return readAllSensorData();
}

/**
 * NYX Logical Orientation sensor connector class
 *  - Rotation
 *  - Bearing
 *  - Acceleration
 *  - Linear Acceleration
 */
NYXLogicalDeviceMotionSensorConnector::NYXLogicalDeviceMotionSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXLogicalSensorConnectorBase(SensorLogicalMotion, aObserver, bCanPostEvent)
{
    NYXConnectorBase *pSensor = NYXConnectorBase::getSensor(SensorRotation, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }

    pSensor = NYXConnectorBase::getSensor(SensorBearing, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }

    pSensor = NYXConnectorBase::getSensor(SensorAcceleration, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }

    pSensor = NYXConnectorBase::getSensor(SensorLinearAcceleration, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }
}

/**
 * NYX Logical Device Orientation sensor connector class
 *  - Bearing
 *  - Rotation
 */
NYXLogicalDeviceOrientationSensorConnector::NYXLogicalDeviceOrientationSensorConnector(NYXConnectorObserver *aObserver, bool bCanPostEvent)
    : NYXLogicalSensorConnectorBase(SensorLogicalOrientation, aObserver, bCanPostEvent)
{
    NYXConnectorBase *pSensor = NYXConnectorBase::getSensor(SensorRotation, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }

    pSensor = NYXConnectorBase::getSensor(SensorBearing, 0, false);
    if (pSensor)
    {
        m_SensorList.push_back(pSensor);
        connect(pSensor, SIGNAL(sensorDataAvailable()), this, SLOT(logicalSensorDataAvailable()));
        pSensor = 0;
    }
}

