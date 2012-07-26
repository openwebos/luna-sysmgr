/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef CUSTOMEVENTS_H_
#define CUSTOMEVENTS_H_

#include "Common.h"

#include <QEvent>

enum SysMgrNativeKeyboardModifiers
{
	SysMgrNativeKeyboardModifier_None          = 0,
	SysMgrNativeKeyboardModifier_InitialState  = 1 << 0,
	SysMgrNativeKeyboardModifier_Last          = 1 << 31
};


static const ushort proximityEventType = QEvent::User + 1000;
static const ushort alsEventType = QEvent::User + 1001;
static const ushort accelerometerEventType = QEvent::User + 1004;
static const ushort accelerationEventType =  QEvent::User + 1005;
static const ushort angularVelocityEventType =  QEvent::User + 1006;
static const ushort bearingEventType =  QEvent::User + 1007;
static const ushort compassEventType = QEvent::User + 1005;
static const ushort gravityEventType =  QEvent::User + 1008;
static const ushort linearAccelerationEventType =  QEvent::User + 1009;
static const ushort magneticFieldEventType =  QEvent::User + 1010;
static const ushort orientationEventType =  QEvent::User + 1011;
static const ushort rotationEventType =  QEvent::User + 1012;
static const ushort shakeEventType =  QEvent::User + 1013;


class ProximityEvent: public QEvent {
public:
	ProximityEvent(bool detected) :
		QEvent((QEvent::Type) proximityEventType), objectDetected(detected) {
	}

	bool isObjectDetected() {
		return objectDetected;
	}

private:
	bool objectDetected;
};


class AlsEvent: public QEvent {
public:
	AlsEvent(int intensity) :
		QEvent((QEvent::Type) alsEventType), lightIntensity(intensity) {
	}

	int getLightIntensity() {
		return lightIntensity;
	}

private:
	int lightIntensity;
};

class ShakeEvent : public QEvent
{
public:
    enum Shake
    {
        Shake_Invalid = 0,
        Shake_Start,
        Shake_Shaking,
        Shake_End,
        Shake_Last = 1 << 31
    };

    ShakeEvent(Shake shakeState, float shakeMagnitude) :
        QEvent((QEvent::Type) shakeEventType),
        m_shakeState(shakeState),
        m_shakeMagnitude(shakeMagnitude)
    {
    }

private:
    Shake   m_shakeState;
    float   m_shakeMagnitude;
};

class OrientationEvent : public QEvent
{
public:
    enum Orientation
    {
        Orientation_Invalid = 0,
        Orientation_FaceUp,
        Orientation_FaceDown,
        Orientation_Up,
        Orientation_Down,
        Orientation_Left,
        Orientation_Right,
        Orientation_Landscape,
        Orientation_Portrait,
        Orientation_Last = 1 << 31
    };

    OrientationEvent(Orientation orientation, float pitch, float roll) :
        QEvent((QEvent::Type) orientationEventType),
        m_orientation(orientation),
        m_pitch(pitch),
        m_roll(roll)
        {
        }

    OrientationEvent(Orientation orientation) :
        QEvent((QEvent::Type) orientationEventType),
        m_orientation(orientation),
        m_pitch(0),
        m_roll(0)
        {
        }

        Orientation orientation() const { return m_orientation; }
        float pitch() const { return m_pitch; }
        float roll() const { return m_roll; }

private:
    Orientation m_orientation;
    float m_pitch;
    float m_roll;
};

/**
 * Event that combines values for acceleration, bearing, orientation, rotation and shake.
 * This event is deprecated and can be removed when "event readers" use the new, individual
 * event types.
 */
class AccelerometerEvent : public QEvent
{
public:
	AccelerometerEvent(float x, float y, float z) :
		QEvent((QEvent::Type) accelerometerEventType),
		m_x(x),
		m_y(y),
		m_z(z),
        m_orientation(OrientationEvent::Orientation_Invalid),
		m_pitch(0),
		m_roll(0),
        m_shakeState(ShakeEvent::Shake_Invalid),
		m_shakeMagnitude(0)
	{
	}
	
    AccelerometerEvent(OrientationEvent::Orientation orientation) :
		QEvent((QEvent::Type) accelerometerEventType),
		m_x(0),
		m_y(0),
		m_z(0),
        m_orientation(orientation),
		m_pitch(0),
		m_roll(0),
        m_shakeState(ShakeEvent::Shake_Invalid),
		m_shakeMagnitude(0)
	{
	}

    AccelerometerEvent(OrientationEvent::Orientation orientation, float pitch, float roll,
			float x, float y, float z,
            ShakeEvent::Shake shakeState, float shakeMagnitude) :
			QEvent((QEvent::Type) accelerometerEventType),
			m_x(x),
			m_y(y),
			m_z(z),
            m_orientation(orientation),
			m_pitch(pitch),
			m_roll(roll),
			m_shakeState(shakeState),
			m_shakeMagnitude(shakeMagnitude)
		{
		}

	float x() const { return m_x; }
	float y() const { return m_y; }
	float z() const { return m_z; }
    OrientationEvent::Orientation orientation() const { return m_orientation; }

	float pitch() const { return m_pitch; }
	float roll() const { return m_roll; }
    ShakeEvent::Shake shakeState() const { return m_shakeState; }
	float shakeMagnitude() const { return m_shakeMagnitude; }

private:
	float m_x;
	float m_y;
	float m_z;

    OrientationEvent::Orientation m_orientation;
	float m_pitch;
	float m_roll;

    ShakeEvent::Shake m_shakeState;
	float m_shakeMagnitude;
};


class AccelerationEvent : public QEvent
{
public:
	AccelerationEvent(float x, float y, float z) :
		QEvent((QEvent::Type) accelerationEventType),
		m_x(x),
		m_y(y),
		m_z(z)
	{
	}
	
	float x() const { return m_x; }
	float y() const { return m_y; }
	float z() const { return m_z; }

private:
	float m_x;
	float m_y;
	float m_z;
};


class AngularVelocityEvent : public QEvent
{
public:
    AngularVelocityEvent(float x, float y, float z) :
        QEvent((QEvent::Type) angularVelocityEventType),
		m_x(x),
		m_y(y),
		m_z(z)
	{
	}
	
	float x() const { return m_x; }
	float y() const { return m_y; }
	float z() const { return m_z; }

private:
	float m_x;
	float m_y;
	float m_z;
};


class BearingEvent : public QEvent
{
public:
    BearingEvent(float mag_bearing, float true_bearing, float confidence) :
        QEvent((QEvent::Type) bearingEventType),
        m_mag_bearing(mag_bearing),
        m_true_bearing(true_bearing),
        m_confidence(confidence)
	{
	}
	
    float mag_bearing()  const { return m_mag_bearing; }
    float true_bearing() const { return m_true_bearing; }
    float confidence()   const { return m_confidence; }

private:
    float m_mag_bearing;
    float m_true_bearing;
    float m_confidence;
};


class GravityEvent : public QEvent
{
public:
    GravityEvent(float x, float y, float z) :
        QEvent((QEvent::Type) gravityEventType),
        m_x(x),
        m_y(y),
        m_z(z)
    {
    }

	float x() const { return m_x; }
	float y() const { return m_y; }
	float z() const { return m_z; }

private:
    float m_x;
    float m_y;
    float m_z;
};


class LinearAccelerationEvent : public QEvent
{
public:
    LinearAccelerationEvent(float x, float y, float z, float world_x, float world_y, float world_z) :
        QEvent((QEvent::Type) gravityEventType),
        m_x(x),
        m_y(y),
        m_z(z),
        m_world_x(world_x),
        m_world_y(world_y),
        m_world_z(world_z)
    {
    }

	float x() const { return m_x; }
	float y() const { return m_y; }
	float z() const { return m_z; }
	float world_x() const { return m_world_x; }
	float world_y() const { return m_world_y; }
	float world_z() const { return m_world_z; }

private:
    float m_x;
    float m_y;
    float m_z;
    float m_world_x;
    float m_world_y;
    float m_world_z;
};


class MagneticFieldEvent : public QEvent
{
public:
    MagneticFieldEvent(int x, int y, int z, int raw_x, int raw_y, int raw_z) :
        QEvent((QEvent::Type) magneticFieldEventType),
        m_x(x),
        m_y(y),
        m_z(z),
        m_raw_x(raw_x),
        m_raw_y(raw_y),
        m_raw_z(raw_z)
        {
        }

    int x() const { return m_x; }
    int y() const { return m_y; }
    int z() const { return m_z; }
    int raw_x() const { return m_raw_x; }
    int raw_y() const { return m_raw_y; }
    int raw_z() const { return m_raw_z; }

private:
    int m_x;
    int m_y;
    int m_z;
    int m_raw_x;
    int m_raw_y;
    int m_raw_z;
};

class CompassEvent : public QEvent
{
public:
	CompassEvent(float magBearing, float trueBearing, int confidence) :
		QEvent((QEvent::Type) compassEventType),
		m_mag_bearing(magBearing),
		m_true_bearing(trueBearing),
		m_confidence(confidence)

	{
	}
	int confidence() const { return m_confidence; }
	float mag_bearing() const { return m_mag_bearing; }
	float true_bearing() const { return m_true_bearing; }

private:
	float m_mag_bearing;
	float m_true_bearing;
	int m_confidence;
};

class RotationEvent : public QEvent
{
public:
    RotationEvent(float pitch, float roll) :
        QEvent((QEvent::Type) rotationEventType),
        m_pitch(pitch),
        m_roll(roll)
    {
    }

    float pitch() const { return m_pitch; }
    float roll() const { return m_roll; }

private:
    float m_pitch;
    float m_roll;
};

#endif /* CUSTOMEVENTS_H_ */
