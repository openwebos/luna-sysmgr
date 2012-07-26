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



#ifndef __NYX_SENSOR_CONNECTOR_H__
#define __NYX_SENSOR_CONNECTOR_H__

#include <limits.h>
#include <QObject>
#include <QSocketNotifier>
#include <QEvent>
#include <vector>
#include <string>

#include <nyx/nyx_client.h>

#include "CustomEvents.h"

/**
 * Indicates a Invalid Angle. Since 0 cannot be used as an invalid angle
 */
#define INVALID_ANGLE (INT_MIN)

// Forward declaration
struct json_object;
class NYXConnectorObserver;

/**
 * Base Class for a connector to physical Sensors/devices using NYX APIs
 */
class NYXConnectorBase : public QObject
{
    Q_OBJECT
public:
    typedef enum
    {
        SensorIllegal = 0,
        SensorFirst,
        SensorAcceleration = SensorFirst,
        SensorOrientation,
        SensorShake,
        SensorALS,
        SensorAngularVelocity,
        SensorBearing,
        SensorGravity,
        SensorLinearAcceleration,
        SensorMagneticField,
        SensorScreenProximity,
        SensorRotation,
        SensorLogicalAccelerometer,             /**< A logical sensor made of 4 physical sensors (Acceleration, Orientation, Shake & Rotation)*/
        SensorLogicalOrientation,               /**< A logical sensor made of 2 physical sensors (Orientation, Rotation). Do not add any other physical sensor as it may slow down the whole device */
        SensorLogicalDeviceOrientation,         /**< A logical sensor made of 2 physical sensors (Rotation & Bearing) */
        SensorLogicalMotion,                    /**< A logical sensor made of 4 physical sensors (Rotation, Bearing, Acceleration & Linear Acceleration) */
        SensorLast = SensorLogicalMotion        /**< Device iterators should stop iterating beyond this point*/
    }Sensor;


    /**
     * @brief Defines all valid poll rates for sensors.
     *
     * Each sensor module that polls for events defines a corresponding rate (in samples per second)
     * for each of these values.
     */
    typedef enum
    {
        SensorReportRateUnknown = NYX_REPORT_RATE_UNKNOWN,  /**< Report rate is unknown */
        SensorReportRateDefault = NYX_REPORT_RATE_DEFAULT,  /**< Default reporting rate for sensor when user has not requested specific rate */
        SensorReportRateLow     = NYX_REPORT_RATE_LOW,      /**< Slow rate of reporting sensor events */
        SensorReportRateMedium  = NYX_REPORT_RATE_MEDIUM,   /**< Medium rate of reporting sensor events */
        SensorReportRateHigh    = NYX_REPORT_RATE_HIGH,     /**< Fast rate of reporting sensor events */
        SensorReportRateHighest = NYX_REPORT_RATE_HIGHEST,  /**< Fastest rate of reporting sensor events */
        SensorReportRateCount   = NYX_REPORT_RATE_COUNT     /**< Must always be last */
    }SensorReportRate;

    /**
     * default destructor
     */
    virtual ~NYXConnectorBase ();

    /**
     * Function returns the available sensors for the current device
     *
     * @return list of sensors in a vector
     */
    static std::vector<NYXConnectorBase::Sensor> getSupportedSensors();

    /**
     * Function returns the available sensors for the current device
     *
     * @param aJson - This is a Fake argument and won't be used. This is just
     *                to re-use the method name
     *
     * @return json Array List
     */
    static std::string getSupportedSensors(bool bJson);

    /**
     * Factory method which initializes(opens) the appropriate sensor
     * and returns a valid instance.
     *
     * @param[in] aSensorType   - Sensor Type
     * @param[in] bCanPostEvent - Control whether sensor can post event data to applications or not
     *                            By default it sends sensor data.
     * @param[in] aObserver     - Observer for the NYX Connectors.
     *
     * @return appropriate Sensor instance if successful, NULL otherwise
     */
    static NYXConnectorBase* getSensor (Sensor aSensorType,
                                        NYXConnectorObserver *aObserver = 0,
                                        bool bCanPostEvent = true);

    /**
     * Start the appropriate sensor
     *
     * @return true if successful, false otherwise
     */
    virtual bool on();

    /**
     * Stop the appropriate sensor
     *
     * @return true if successful, false otherwise
     */
    virtual bool off();

    /**
     * Sets the report rate for the sensor
     *
     * @param[in] aRate - report rate
     *
     * @return appropriate Sensor instance if successful, NULL otherwise
     */
    virtual bool setRate(SensorReportRate aRate);

    /**
     * Sets the Orientation angle, which will be taken into account
     * while processing sensor data for Accelerometer, Rotation, Bearing (Compass)
     * and Gyro
     *
     * @param aAngle - Angle
     */
    virtual void setOrientationAngle(int aAngle)
    {
        if (INVALID_ANGLE != aAngle)
        {
            m_OrientationAngle = aAngle;
        }
    }

    /**
     * Gets the sensor data in json format
     *
     * @return sensor data in json format if successful, "" (empty string) otherwise
     */
    virtual std::string toJSONString();

    /**
     * Gets the sensor data as a json Object
     *
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject() = 0;

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData() = 0;

    /**
     * Gets the low level NYX handle for the sensor
     *
     * @return valid handle, NULL otherwise
     */
    virtual inline nyx_device_handle_t getHandle()
    {
        return m_Handle;
    }

    /**
     * Gets the Sensor Type
     *
     * @return appropriate valid Sensor type, SENSOR_ILLEGAL if not initialized
     */
    inline Sensor type()
    {
        return m_SensorType;
    }

    /**
      * Function marks this object for deletion
      * As Qt deleteLater() is not working as expected, we will add
      * an idle runloop callback which will help us delete this object
      */
    virtual void scheduleDeletion();

protected:
    /**
     * Parameterize constructor
     *
     * @param[in] aSensorType   - Sensor Type
     * @param[in] aDevType      - NYX device Type
     * @param[in] aDevID        - NYX device ID
     * @param[in] aObserver     - Observer for the NYX device
     * @param[in] bCanPostEvent - Defines whether a particular sensor can post event data or not
     */
    NYXConnectorBase (Sensor aSensorType, nyx_device_type_t aDevType, nyx_device_id_t aDevID, NYXConnectorObserver *aObserver, bool bCanPostEvent);

    /**
     * Opens a given sensor.
     * Override this method if a sensor doesn't wants to be opened in a default way.
     */
    virtual nyx_error_t openSensor ();

    /**
     * Connects the Sensor FD to a SLOT
     * Override this method if a sensor has some other slot to register than the default one
     */
    virtual void connectSensorSignalToSlot();

    /**
     * Function emits sensorDataAvailable() signal based on the passed argument
     * Once the signal is emitted it calls the observer
     *
     * @param[in] aShouldEmit   - Whether to emit signal or not. Default to true.
     */
    void callObserver(bool aShouldEmit = true);

    /**
     * Find out whether a particular sensor can post event data or not
     */
    inline bool canPostEvent()
    {
        return m_CanPostEvent;
    }

private:
    /**
     * Maps the NYX device type
     */
    static NYXConnectorBase::Sensor MapFromNYX(nyx_device_type_t aNYXDevType);

Q_SIGNALS:
    /**
     * SIGNAL is emitted whenever sensor data is available to consume
     */
    virtual void sensorDataAvailable();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket) = 0;

protected:
    // Functions

    /**
     * Implement this method, if a sensor requires post processing of the sensor
     * data before sending it for App consumption
     */
    virtual void postProcessSensorData() {}

    // Data
    Sensor                      m_SensorType;
    QSocketNotifier*            m_NYXSensorNotifier;
    nyx_device_handle_t         m_Handle;
    nyx_device_type_t           m_NYXDeviceType;
    nyx_device_id_t             m_NYXDeviceId;
    NYXConnectorObserver       *m_Observer;
    int                         m_SensorFD;
    bool                        m_CanPostEvent;
    int                         m_OrientationAngle;
    bool                        m_Finished;
};

/**
 * Observer class for NYXConnectorBase objects.
 */
class NYXConnectorObserver
{
public:
    /**
     * Function gets called whenever there is some data
     * available from NYX.
     *
     * @param[in]   - aSensorType - Sensor which has got some data to report
     */
    virtual void NYXDataAvailable (NYXConnectorBase::Sensor aSensorType) = 0;
};

/**
 * NYX Acceleration sensor connector class
 */
class NYXAccelerationSensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXAccelerationSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Acceleration value for x-axis as a fraction of "g"
     */
    inline float X() const { return m_AccelerationData.x; }

    /**
     * acceleration value for y-axis as a fraction of "g"
     */
    inline float Y() const { return m_AccelerationData.y; }

    /**
     * acceleration value for z-axis as a fraction of "g"
     */
    inline float Z() const { return m_AccelerationData.z; }

    /**
     * Gets the sensor data as a json Object
     *
     * JSON Format: Json Object
     * {
     *      "acceleration" :
     *      {
     *          "x" : float,
     *          "y" : float,
     *          "z" : float
     *      }
     * }
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

protected:
    /**
     * Implement this method, if a sensor requires post processing of the sensor
     * data before sending it for App consumption
     */
    virtual void postProcessSensorData();

private:
    // Data
    nyx_sensor_acceleration_event_item_t    m_AccelerationData;
};

/**
 * NYX ALS sensor connector class
 */
class NYXAlsSensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXAlsSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * light intensity value
     */
    inline int getLightIntensity() { return m_LightIntensity; }

    /**
     * Gets the sensor data as a json Object
     *
     * JSON Format: Json Object
     * {
     *      "als" :
     *      {
     *          "lightIntesity" : int
     *      }
     * }
     *
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

private:
    int     m_LightIntensity;
};

/**
 * NYX Compass sensor connector class
 */
class NYXAngularVelocitySensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXAngularVelocitySensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Angular velocity around x-axis in radians per second
     */
    inline float X() const { return m_AngularVelocity.x; }

    /**
     * Angular velocity around y-axis in radians per second
     */
    inline float Y() const { return m_AngularVelocity.y; }

    /**
     * Angular velocity around z-axis in radians per second
     */
    inline float Z() const { return m_AngularVelocity.z; }

    /**
     * Gets the sensor data as a json Object
     *
     * JSON Format: Json Object
     * {
     *      "angluarVelocity" :
     *      {
     *          "x" : float,
     *          "y" : float,
     *          "z" : float
     *      }
     * }
     *
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

private:
    nyx_sensor_angular_velocity_event_item_t  m_AngularVelocity;
};

/**
 * NYX Bearing sensor connector class
 */
class NYXBearingSensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXBearingSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Bearing Magnitude in degrees
     */
    inline float bearingMagnitude() const { return m_Bearing.magnetic; }

    /**
     * True Bearing in degrees
     */
    inline float trueBearing() const { return m_Bearing.true_bearing; }

    /**
     * Confidence of bearing values in percentage (%)
     */
    inline float confidence() const { return m_Bearing.confidence; }

    /**
     * Gets the sensor data as a json Object
     *
     * JSON Format: Json Object
     * {
     *      "bearing" :
     *      {
     *          "magnetic"      : float,
     *          "trueBearing"   : float,
     *          "confidence"    : float
     *      }
     * }
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

    /**
     * Sets the GPS location data to Bearing sensor
     *
     * @param[in]   aLatitude   - current Latitude
     * @param[in]   aLongitude  - current Longitude
     *
     * @return true if successful, false otherwise
     */
    bool setLocation(double aLatitude, double aLongitude);

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

private:
    nyx_sensor_bearing_event_item_t m_Bearing;
};

/**
 * NYX Gravity sensor connector class
 */
class NYXGravitySensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXGravitySensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Gravitational acceleration x component of unit vector
     */
    inline float X() const { return m_Gravity.x; }

    /**
     * Gravitational acceleration y component of unit vector
     */
    inline float Y() const { return m_Gravity.y; }

    /**
     * Gravitational acceleration z component of unit vector
     */
    inline float Z() const { return m_Gravity.z; }

    /**
     * Gets the sensor data as a json Object
     *
     * JSON Format: Json Object
     * {
     *      "gravity" :
     *      {
     *          "x" : float,
     *          "y" : float,
     *          "z" : float
     *      }
     * }
     *
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

private:
    nyx_sensor_gravity_event_item_t m_Gravity;
};

/**
 * NYX LinearAccelearation sensor connector class
 */
class NYXLinearAccelearationSensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXLinearAccelearationSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Acceleration value for x-axis in meters per second<sup>2</sup>
     */
    inline float X() const { return m_LinearAcceleration.x; }

    /**
     * Acceleration value for y-axis in meters per second<sup>2</sup>
     */
    inline float Y() const { return m_LinearAcceleration.y; }

    /**
     * Acceleration value for z-axis in meters per second<sup>2</sup>
     */
    inline float Z() const { return m_LinearAcceleration.z; }

    /**
     * World coordinate acceleration value for x-axis in meters per second<sup>2</sup>
     */
    inline float WorldX() const { return m_LinearAcceleration.world_x; }

    /**
     * World coordinate acceleration value for y-axis in meters per second<sup>2</sup>
     */
    inline float WorldY() const { return m_LinearAcceleration.world_y; }

    /**
     * World coordinate acceleration value for z-axis in meters per second<sup>2</sup>
     */
    inline float WorldZ() const { return m_LinearAcceleration.world_z; }

    /**
     * Gets the sensor data as a json Object
     *
     * JSON Format: Json Object
     * {
     *      "linearAcceleration" :
     *      {
     *          "x"         : float,
     *          "y"         : float,
     *          "z"         : float,
     *          "worldX"    : float,
     *          "worldY"    : float,
     *          "worldZ"    : float
     *      }
     * }
     *
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

private:
    nyx_sensor_linear_acceleration_event_item_t m_LinearAcceleration;
};

/**
 * NYX MagneticField sensor connector class
 */
class NYXMagneticFieldSensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXMagneticFieldSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Strength of magnetic field for x-axis in micro Teslas
     */
    inline int X() const { return m_MagneticField.x; }

    /**
     * Strength of magnetic field for y-axis in micro Teslas
     */
    inline int Y() const { return m_MagneticField.y; }

    /**
     * Strength of magnetic field for z-axis in micro Teslas
     */
    inline int Z() const { return m_MagneticField.z; }

    /**
     * Raw strength of magnetic field for x-axis in micro Teslas
     */
    inline int rawX() const { return m_MagneticField.raw_x; }

    /**
     * Raw strength of magnetic field for y-axis in micro Teslas
     */
    inline int rawY() const { return m_MagneticField.raw_y; }

    /**
     * Raw strength of magnetic field for z-axis in micro Teslas
     */
    inline int rawZ() const { return m_MagneticField.raw_z; }

    /**
     * Gets the sensor data as a json Object
     *
     * {
     *      "magneticField" :
     *      {
     *          "x"         : float,
     *          "y"         : float,
     *          "z"         : float,
     *          "rawX"      : float,
     *          "rawY"      : float,
     *          "rawZ"      : float
     *      }
     * }
     *
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

private:
    nyx_sensor_magnetic_field_event_item_t  m_MagneticField;
};

/**
 * NYX Orientation sensor connector class
 */
class NYXOrientationSensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXOrientationSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Orientation of the device
     */
    inline OrientationEvent::Orientation getOrientation()
    {
        return m_Orientation;
    }

    /**
     * Function converts the Orientation value to a String
     */
    const char* toPositionString();

    /**
     * Gets the sensor data as a json Object
     *
     * JSON Format: Json Object
     * {
     *      "orientation" :
     *      {
     *          "position" : string
     *      }
     * }
     *
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

protected:
    /**
     * Implement this method, if a sensor requires post processing of the sensor
     * data before sending it for App consumption
     */
    virtual void postProcessSensorData();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

private:
    /**
     * convert the accelerometer orientation from the lower level into
     * an Orientation type
     *
     * @param[in]   aValue - NYX enum value
     *
     * @return Mapped Orientation Event
     */
    OrientationEvent::Orientation mapAccelerometerOrientation(int aValue);

    // Data
    OrientationEvent::Orientation         m_Orientation;
};

/**
 * NYX Screen Proximity sensor connector class
 */
class NYXScreenProximitySensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXScreenProximitySensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * check whether screen proximity sensor is present or not
     */
    inline bool Present() const { return (bool)m_Present; }

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

    /**
     * Gets the sensor data as a json Object
     *
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject() { return 0; }

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

private:
    int m_Present;
};

/**
 * NYX Rotation sensor connector class
 */
class NYXRotationSensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXRotationSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Euler angle of rotation (in degrees)
     * amount of pitch in degrees
     */
    inline float pitch() const { return m_RotationData.euler_angle.pitch; }

    /**
     * Euler angle of rotation (in degrees)
     * amount of roll in degrees
     */
    inline float roll() const { return m_RotationData.euler_angle.roll; }

    /**
     * Euler angle of rotation (in degrees)
     * amount of yaw in degrees
     */
    inline float yaw() const { return m_RotationData.euler_angle.yaw; }

    /**
     * Quaternion vector of rotation
     */
    inline float quaternionW() const { return m_RotationData.quaternion_vector.w; }
    inline float quaternionX() const { return m_RotationData.quaternion_vector.x; }
    inline float quaternionY() const { return m_RotationData.quaternion_vector.y; }
    inline float quaternionZ() const { return m_RotationData.quaternion_vector.z; }

    /**
     * Rotation Matrix [3 x 3]
     */
    inline const float* rotationMatrix() const { return m_RotationData.matrix; }

    /**
     * Gets the sensor data as a json Object
     *
     * JSON Format: Json Object
     * {
     *      "rotationMatrix" : [0,1,2,3,4,5,6,7,8,9],
     *      "quaternionVector" :
     *      {
     *          "w" : float,
     *          "x" : float,
     *          "y" : float,
     *          "z" : float
     *      },
     *      "eulerAngle" :
     *      {
     *          "roll"  : float,
     *          "pitch" : float,
     *          "yaw"   : float
     *      }
     * }
     *
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

protected:
    /**
     * Implement this method, if a sensor requires post processing of the sensor
     * data before sending it for App consumption
     */
    virtual void postProcessSensorData();

private:
    nyx_sensor_rotation_event_item_t m_RotationData;
};

/**
 * NYX Shake sensor connector class
 */
class NYXShakeSensorConnector : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXShakeSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Shaking state of device
     */
    inline ShakeEvent::Shake shakeState() const
    {
        return m_ShakeState;
    }

    /**
     * magnitude of shaking in meters per (second)^2
     */
    inline float shakeMagnitude() const { return m_ShakeMagnitude; }

    /**
     * Gets the sensor data as a json Object
     *
     * JSON Format : JSON Object
     * {
     *      "shake" :
     *      {
     *          "shakestate" : string,
     *          "shakeMagnitude" : float
     *      }
     * }
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int aSocket);

private:
    /**
     * convert the Shake event from the lower level into
     * an Shake type
     *
     * @param[in]   aValue - NYX enum value
     */
    ShakeEvent::Shake mapShakeSensorEvent(int aValue);

    /**
     * Function maps the shake state to a string
     */
    const char* toShakeStateString();

    // Data
    ShakeEvent::Shake   m_ShakeState;
    float               m_ShakeMagnitude;
};

/**
 * NYX Logical sensor connector base class
 * A logical sensor is the one which combines more than one physical sensors
 * and generates useful data
 */
class NYXLogicalSensorConnectorBase : public NYXConnectorBase
{
    Q_OBJECT
public:
    /**
     * default destructor
     */
    virtual ~NYXLogicalSensorConnectorBase();

    /**
     * Start the appropriate sensor
     *
     * @return true if sucessful, false otherwise
     */
    virtual bool on();

    /**
     * Stop the apropriate sensor
     *
     * @return true if successful, false otherwise
     */
    virtual bool off();

    /**
     * Sets the report rate for the sensor
     *
     * @param[in] aRate - report rate
     *
     * @return appropriate Sensor instance if successful, NULL otherwise
     */
    virtual bool setRate(SensorReportRate aRate);

    /**
     * Sets the Orientation angle, which will be taken into account
     * while processing sensor data for Accelerometer, Rotation, Bearing (Compass)
     * and Gyro
     *
     * @param aAngle - Angle
     */
    virtual void setOrientationAngle(int aAngle);

    /**
      * Function marks this object for deletion
      * As Qt deleteLater() is not working as expected, we will add
      * an idle runloop callback which will help us delete this object
      */
    virtual void scheduleDeletion();

    /**
     * Gets the sensor data as a json Object
     *
     * @return sensor data as a json object if successful, NULL otherwise
     * @note The json object ownership is transferred to the caller and will be responsible
     *       for freeing it using json_object_put(<object>);
     */
    virtual json_object* toJSONObject();

protected:
    /**
     * Parameterize constructor
     *
     * @param[in] aSensorType   - Sensor Type
     * @param[in] aObserver     - Observer for the NYX device
     * @param[in] bCanPostEvent - Defines whether a particular sensor can post event data or not
     */
    NYXLogicalSensorConnectorBase (Sensor aSensorType, NYXConnectorObserver *aObserver, bool bCanPostEvent);

    /**
     * Opens a given sensor.
     * Override this method if a sensor doesn't wants to be opened in a default way.
     */
    virtual nyx_error_t openSensor () { return NYX_ERROR_NONE;}

    /**
     * Connects the Sensor FD to a SLOT
     * Override this method if a sensor has some other slot to register than the default one
     */
    virtual void connectSensorSignalToSlot() { /* do nothing */ }

protected Q_SLOTS:
    /**
     * SLOT gets called whenever there is some sensor data available to read.
     *
     * @param[in] aSocket   - Socket identifier
     */
    virtual void readSensorData(int ) { /* do nothing. It's an logical sensor and not the physical one */ }

    /**
     * SLOT gets called whenever any of the one sensor has data available to read.
     */
    void logicalSensorDataAvailable();

protected:
    std::vector<NYXConnectorBase *>          m_SensorList;
};

/**
 * NYX Logical Accelerometer sensor connector class
 * - Acceleration
 * - Orientation
 * - Shake
 * - Rotation
 */
class NYXLogicalAccelerometerSensorConnector : public NYXLogicalSensorConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXLogicalAccelerometerSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

private:
    /**
     * Function reads all the sensors data and creates an event
     */
    QEvent* readAllSensorData();
};

/**
 * NYX Logical Orientation sensor connector class
 *  - Orientation
 *  - Rotation
 */
class NYXLogicalOrientationSensorConnector : public NYXLogicalSensorConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXLogicalOrientationSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData();

private:
    /**
     * Function reads all the sensors data and creates an event
     */
    QEvent* readAllSensorData();
};

/**
 * NYX Logical Device Orientation sensor connector class
 *  - Bearing
 *  - Rotation
 */
class NYXLogicalDeviceOrientationSensorConnector : public NYXLogicalSensorConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXLogicalDeviceOrientationSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData() { return 0; }
};

/**
 * NYX Logical Orientation sensor connector class
 *  - Rotation
 *  - Bearing
 *  - Acceleration
 *  - Linear Acceleration
 */
class NYXLogicalDeviceMotionSensorConnector : public NYXLogicalSensorConnectorBase
{
    Q_OBJECT
public:
    /**
     * Parameterized constructor
     *
     * @param[in]   aObserver       - NYX sensor observer
     * @param[in]   bCanPostEvent   - Control whether to post event or not
     */
    NYXLogicalDeviceMotionSensorConnector (NYXConnectorObserver *aObserver, bool bCanPostEvent = true);

    /**
     * Gets the sensor data in Qt (QEvent) format
     *
     * @return QEvent if successful, NULL otherwise
     * @note the ownership of heap data (QEvent *) is transferred to caller
     *       and is responsible for deleting that data
     */
    virtual QEvent* getQSensorData() { return 0; }
};

#endif /* __NYX_SENSOR_CONNECTOR_H__ */
