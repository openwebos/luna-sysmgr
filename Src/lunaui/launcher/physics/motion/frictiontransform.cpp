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




#include "frictiontransform.h"

#define DEFAULT_COEF_FSTATIC	0.6
#define DEFAULT_COEF_FKINETIC	0.5
#define DEFAULT_MASS			1.0
#define DEFAULT_MOMENTTIME		1.0
#define GRAVITY_ACCEL			9.8
#define DEFAULT_ACCEL_INITIAL	72.0			//about 4g worth of force

FrictionTransform::FrictionTransform()
: m_initialAcceleration(DEFAULT_ACCEL_INITIAL)
, m_objectMass(DEFAULT_MASS)
, m_forceMomentTime(DEFAULT_MOMENTTIME)
, m_reverse(1.0)
, m_coeffFrictionStatic(DEFAULT_COEF_FSTATIC)
, m_coeffFrictionKinetic(DEFAULT_COEF_FKINETIC)
, m_gravityAccel(GRAVITY_ACCEL)
{
	parametersChanged();
}

//virtual
FrictionTransform::~FrictionTransform()
{
}

//virtual
void FrictionTransform::restart(const qreal newStartTime)
{
	LinearMotionTransform::restart(newStartTime);
	parametersChanged();
}

qreal FrictionTransform::setInitialForce(const qreal force)
{
	qreal _v = m_initialAcceleration*m_objectMass;
	qreal f = qAbs(force);
	if (force < 0.0)
	{
		m_reverse = -1.0;
	}
	else
	{
		m_reverse = 1.0;
	}
	m_initialAcceleration = f / m_objectMass;
	parametersChanged();
	return _v;
}

#include <QDebug>

qreal FrictionTransform::setInitialAcceleration(const qreal accel)
{
	qreal _v = accel;
	m_initialAcceleration = qAbs(accel);
	//qDebug() << "INITIAL ACCEL = " << m_initialAcceleration;
	if (accel < 0.0)
	{
		m_reverse = -1.0;
	}
	else
	{
		m_reverse = 1.0;
	}
	parametersChanged();
	return _v;
}

qreal FrictionTransform::setObjectMass(const qreal mass)
{
	qreal _v = m_objectMass;
	if (!DimensionsGlobal::isZeroF(mass))
	{
		m_objectMass = mass;
		parametersChanged();
	}
	return _v;
}

qreal FrictionTransform::setInitialMomentTime(const qreal time)
{
	qreal _v = m_forceMomentTime;
	m_forceMomentTime = time;
	parametersChanged();
	return _v;
}

qreal FrictionTransform::setCoeffKineticFriction(const qreal coeffKinFriction)
{
	qreal _v = m_coeffFrictionKinetic;
	m_coeffFrictionKinetic = coeffKinFriction;
	parametersChanged();
	return _v;
}

qreal FrictionTransform::coeffKineticFriction() const
{
	return m_coeffFrictionKinetic;
}

qreal FrictionTransform::initialForce() const
{
	return m_initialAcceleration*m_objectMass;
}

qreal FrictionTransform::initialAcceleration() const
{
	return m_initialAcceleration;
}

qreal FrictionTransform::objectMass() const
{
	return m_objectMass;
}

qreal FrictionTransform::initialMomentTime() const
{
	return m_forceMomentTime;
}

//virtual
qreal FrictionTransform::displacement(const qreal time)
{
	qreal d,v,a;
	vec(time,d,v,a);
	return d;
}

//virtual
qreal FrictionTransform::velocity(const qreal time)
{
	qreal d,v,a;
	vec(time,d,v,a);
	return v;
}

//virtual
qreal FrictionTransform::acceleration(const qreal time)
{
	qreal d,v,a;
	vec(time,d,v,a);
	return a;
}

//virtual
void FrictionTransform::vec(const qreal time,qreal& d,qreal& v, qreal& a)
{
	qreal t = time-m_startTime;
	if (m_validLast && (m_lastComputedTime == t))
	{
		//WARN: THIS IS CASE WILL *NOT* CAUSE THE SIGNALS TO RE-TRIGGER!!!
		d = m_lastComputedDisplacement;
		v = m_lastComputedVelocity;
		a = m_lastComputedAcceleration;
		return;
	}

	if (t > m_computedGrindToHaltTime)
	{
		v = 0.0;
		a = 0.0;
		d = m_computedFinalDisplacement;
	}
	else {
		qreal t2 = t*t;
		v = m_computedInitialVelocity - t*m_computedAccelKineticFriction;
		if (v <= 0.0)
		{
			v = 0.0;
			a = 0.0;
			d = m_computedFinalDisplacement;
		}
		else
		{
			a = m_computedAccelKineticFriction;
			d = m_initialDisplacement + m_reverse*(m_computedInitialVelocity*t - t2*a/2);
		}
	}
	update(t,d,v,a);
}

//virtual
QVector<qreal> FrictionTransform::vec(const qreal time)
{
	qreal d,v,a;
	vec(time,d,v,a);
	return (QVector<qreal>() << d << v << a);
}

//virtual
qreal FrictionTransform::haltTime() const
{
	return m_computedGrindToHaltTime;
}

//virtual
void FrictionTransform::setReverse()
{
	m_reverse = -1.0;
	parametersChanged();
}

//virtual
void FrictionTransform::setForward()
{
	m_reverse = 1.0;
	parametersChanged();
}

//public Q_SLOTS:

//virtual
void FrictionTransform::slotTimetic(const qreal absTime)
{
	LinearMotionTransform::slotTimetic(absTime);
	//m_autocountAccumulator now holds the abs time that should be used (it's == absTime if autocounting isn't active)
	qreal d,v,a;
	vec(m_autocountAccumulator,d,v,a);
}

//protected:

#include <QDebug>

void FrictionTransform::parametersChanged()
{
	LinearMotionTransform::parametersChanged();
	m_computedAccelStaticFriction = m_coeffFrictionStatic*m_gravityAccel;
	m_computedAccelKineticFriction = m_coeffFrictionKinetic*m_gravityAccel;
	m_computedInitialVelocity = qMax((qreal)0.0,m_initialVelocity + (m_initialAcceleration - m_computedAccelStaticFriction)*m_forceMomentTime);
	//compute where the discontinuity occurs, when velocity drops to 0
	m_computedGrindToHaltTime = m_computedInitialVelocity / m_computedAccelKineticFriction;
	m_computedFinalDisplacement = m_initialDisplacement + m_reverse*(m_computedInitialVelocity*m_computedGrindToHaltTime
			- m_computedGrindToHaltTime*m_computedGrindToHaltTime*m_computedAccelKineticFriction/2);

	//qDebug() << __FUNCTION__ << ": halt @ " << m_computedGrindToHaltTime << "(abs: " << m_computedGrindToHaltTime+m_startTime
//			<< " ), initialDisplacement: " << m_initialDisplacement
//			<< " , final displacement: " << m_computedFinalDisplacement;
}

