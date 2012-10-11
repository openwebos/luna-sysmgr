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




#include "WebKitSensorConnector.h"

WebKitSensorConnector::WebKitSensorConnector(Palm::SensorType aSensorType, Palm::fnSensorDataCallback aDataCB, Palm::fnSensorErrorCallback aErrCB, void *pUserData)
    : m_PalmSensorType(aSensorType)
    , m_DataCB(aDataCB)
    , m_ErrCB(aErrCB)
    , m_UserData(pUserData)
{
#if defined(HAS_NYX)
    m_NYXSensorType = WebKitToNYX(aSensorType);

    // Create the sensor
    m_Sensor = NYXConnectorBase::getSensor(m_NYXSensorType, this);

    // Immediately switch-off the sensor, as NYX starts sending the NYX data immediately
    off();
#endif
}


WebKitSensorConnector::~WebKitSensorConnector()
{
#if defined(HAS_NYX)
    if (m_Sensor)
    {
        m_Sensor->scheduleDeletion();
    }
    m_Sensor = 0;
#endif
}

WebKitSensorConnector* WebKitSensorConnector::createSensor(Palm::SensorType aType, Palm::fnSensorDataCallback aDataCB, Palm::fnSensorErrorCallback aErrCB, void *pUserData)
{
    WebKitSensorConnector *pWebKitSensor = 0;

#if defined(HAS_NYX)
    if ((aDataCB) && (aErrCB))
    {
        NYXConnectorBase::Sensor nyxSensorType = WebKitToNYX(aType);
        if (NYXConnectorBase::SensorIllegal != nyxSensorType)
        {
            pWebKitSensor = new WebKitSensorConnector(aType, aDataCB, aErrCB, pUserData);
        }
    }
#endif

    return pWebKitSensor;
}

#if defined(HAS_NYX)
NYXConnectorBase::Sensor WebKitSensorConnector::WebKitToNYX(Palm::SensorType aSensorType)
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
            mappedSensor = NYXConnectorBase::SensorOrientation;
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

        case Palm::SensorALS:
        {
            mappedSensor = NYXConnectorBase::SensorALS;
            break;
        }

        case Palm::SensorAngularVelocity:
        {
            mappedSensor = NYXConnectorBase::SensorAngularVelocity;
            break;
        }

        case Palm::SensorGravity:
        {
            mappedSensor = NYXConnectorBase::SensorGravity;
            break;
        }

        case Palm::SensorLinearAcceleration:
        {
            mappedSensor = NYXConnectorBase::SensorLinearAcceleration;
            break;
        }

        case Palm::SensorMagneticField:
        {
            mappedSensor = NYXConnectorBase::SensorMagneticField;
            break;
        }

        case Palm::SensorScreenProximity:
        {
            mappedSensor = NYXConnectorBase::SensorScreenProximity;
            break;
        }

        case Palm::SensorRotation:
        {
            mappedSensor = NYXConnectorBase::SensorRotation;
            break;
        }

        case Palm::SensorLogicalDeviceOrientation:
        {
            mappedSensor = NYXConnectorBase::SensorLogicalDeviceOrientation;
            break;
        }

        case Palm::SensorLogicalDeviceMotion:
        {
            mappedSensor = NYXConnectorBase::SensorLogicalMotion;
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

NYXConnectorBase::SensorReportRate WebKitSensorConnector::WebKitToNYX(Palm::SensorRate aRate)
{
    NYXConnectorBase::SensorReportRate mappedRate = NYXConnectorBase::SensorReportRateDefault;

    switch(aRate)
    {
        case Palm::RATE_DEFAULT:
        {
            mappedRate = NYXConnectorBase::SensorReportRateDefault;
            break;
        }

        case Palm::RATE_LOW:
        {
            mappedRate = NYXConnectorBase::SensorReportRateLow;
            break;
        }

        case Palm::RATE_MEDIUM:
        {
            mappedRate = NYXConnectorBase::SensorReportRateMedium;
            break;
        }

        case Palm::RATE_HIGH:
        {
            mappedRate = NYXConnectorBase::SensorReportRateHigh;
            break;
        }

        case Palm::RATE_HIGHEST:
        {
            mappedRate = NYXConnectorBase::SensorReportRateHighest;
            break;
        }

        default:
        {
            g_critical("[%s : %d] : Mustn't have reached here : Sensor Type : Sensor Type.", __PRETTY_FUNCTION__, __LINE__);
            break;
        }
    }

    return mappedRate;
}

void WebKitSensorConnector::NYXDataAvailable (NYXConnectorBase::Sensor aSensorType)
{
    if ((NYXConnectorBase::SensorIllegal != aSensorType) && (m_Sensor) && (m_DataCB))
    {
        std::string jsonData = m_Sensor->toJSONString();

        m_DataCB(m_PalmSensorType, jsonData, m_UserData);
    }
}

#endif

bool WebKitSensorConnector::on()
{
    bool bRetValue = false;

#if defined(HAS_NYX)
    if (m_Sensor)
    {
        bRetValue = m_Sensor->on();
        if (!bRetValue)
        {
            g_critical("[%s : %d] : Critical Error Occurred while trying to turn the sensor on : Sensor Type : [%d]", __PRETTY_FUNCTION__, __LINE__, m_NYXSensorType);
        }
    }
#endif

    return bRetValue;
}


bool WebKitSensorConnector::off()
{
    bool bRetValue = false;

#if defined(HAS_NYX)
    if (m_Sensor)
    {
        bRetValue = m_Sensor->off();
        if (!bRetValue)
        {
            g_critical("[%s : %d] : Critical Error Occurred while trying to turn the sensor off : Sensor Type : [%d]", __PRETTY_FUNCTION__, __LINE__, m_NYXSensorType);
        }
    }
#endif

    return bRetValue;

}

bool WebKitSensorConnector::setRate(Palm::SensorRate aRate)
{
    bool bRetValue = false;

#if defined(HAS_NYX)
    if (m_Sensor)
    {
        NYXConnectorBase::SensorReportRate sensorRate = WebKitToNYX(aRate);
        if ((NYXConnectorBase::SensorReportRateUnknown != sensorRate) &&
            (NYXConnectorBase::SensorReportRateCount   != sensorRate))
        {
            bRetValue = m_Sensor->setRate(sensorRate);
            if (!bRetValue)
            {
                g_critical("[%s : %d] : Critical Error Occurred while trying to set the sensor rate : Sensor Type : [%d]", __PRETTY_FUNCTION__, __LINE__, m_NYXSensorType);
            }
        }
    }
#endif

    return bRetValue;
}

std::string WebKitSensorConnector::getSupportedSensors()
{
    std::string strSensorList = "";

#if defined(HAS_NYX)
    strSensorList = NYXConnectorBase::getSupportedSensors(true);
    if (strSensorList.empty())
    {
        g_critical("[%s : %d] : Critical Error Occurred while trying to get the Sensor List: Sensor Type", __PRETTY_FUNCTION__, __LINE__);
    }
#endif

    return strSensorList;
}

void WebKitSensorConnector::CallErrorCB(std::string aMsg)
{
#if defined(HAS_NYX)
    if ((m_Sensor) && (m_ErrCB))
    {
        m_ErrCB(m_PalmSensorType, aMsg, m_UserData);
    }
#endif
}


