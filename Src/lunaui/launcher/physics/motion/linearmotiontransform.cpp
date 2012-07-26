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




#include "linearmotiontransform.h"

//public:

//virtual
qreal LinearMotionTransform::setStartTime(const qreal time)
{	//returns old start time
	qreal _v = m_startTime;
	m_startTime = time;
	return _v;
}

//virtual
qreal LinearMotionTransform::startTime()
{
	return m_startTime;
}

//virtual
void  LinearMotionTransform::activate()
{
	m_active = true;
}
//virtual
void  LinearMotionTransform::deactivate()
{
	m_active = false;
}

//virtual
bool  LinearMotionTransform::isActive() const
{
	return m_active;
}

//virtual
void  LinearMotionTransform::restart(const qreal newStartTime)
{
	m_startTime = newStartTime;
	m_validLast = false;
	m_lastComputedTime = 0.0;
	m_lastComputedAcceleration = 0.0;
	m_lastComputedVelocity = 0.0;
	m_lastComputedDisplacement = 0.0;

	m_autocountAccumulator = m_startTime;
}

//virtual
qreal LinearMotionTransform::setAutocount(const qreal incTime)
{
	qreal _v = m_autocountIncrement;

	if (qFuzzyCompare(incTime+1.0,1.0))
	{
		//it's zero
		disableAutocount();
	}
	else
	{
		m_useAutocount = true;
		m_autocountIncrement = qAbs(incTime);	//counting backwards not allowed
	}
	return _v;
}

//virtual
void  LinearMotionTransform::disableAutocount()
{
	m_useAutocount = false;
	m_autocountIncrement = 0.0;
	m_autocountAccumulator = m_startTime;
}

//virtual
qreal LinearMotionTransform::setInitialDisplacement(const qreal di)
{
	qreal _v = m_initialDisplacement;
	m_initialDisplacement = di;
	parametersChanged();
	return _v;
}

//virtual
qreal LinearMotionTransform::setInitialVelocity(const qreal vi)
{
	qreal _v = m_initialVelocity;
	m_initialVelocity = vi;
	parametersChanged();
	return _v;
}
//virtual
qreal LinearMotionTransform::setInitialAcceleration(const qreal ai)
{
	qreal _v = m_initialAcceleration;
	m_initialAcceleration	 = ai;
	parametersChanged();
	return _v;
}

//virtual
qreal LinearMotionTransform::initialDisplacement() const
{
	return m_initialDisplacement;
}

//virtual
qreal LinearMotionTransform::initialVelocity() const
{
	return m_initialVelocity;
}

//virtual
qreal LinearMotionTransform::initialAcceleration() const
{
	return m_initialAcceleration;
}

//virtual
qreal LinearMotionTransform::displacement(const qreal time)
{
	return 0.0;
}

//virtual
qreal LinearMotionTransform::velocity(const qreal time)
{
	return 0.0;
}

//virtual
qreal LinearMotionTransform::acceleration(const qreal time)
{
	return 0.0;
}

//virtual
void LinearMotionTransform::vec(const qreal time,qreal& d,qreal& v, qreal& a)
{

}

//virtual
QVector<qreal> LinearMotionTransform::vec(const qreal time)
{
	return QVector<qreal>();
}

//virtual
void LinearMotionTransform::setTrigger(quint32 component,
										LinearMotionTransformTriggers::Type type,
										const qreal val,
										QObject * receiver,
										const char * slot,
										LinearMotionTransformTriggers::Freq freq)
{
	if (type == LinearMotionTransformTriggers::UNSET)
		return clearTrigger(component);

	if (component & LinearMotionTransformTriggers::Displacement)
	{
		m_dTrigger = type;
		m_dTriggerFreq = freq;
		m_dTriggerValue = val;
		connect(this,SIGNAL(signalDisplacementTrigger(qreal)),
				receiver,slot);
		m_anyTrigger = true;

	}
	else if (component & LinearMotionTransformTriggers::Velocity)
	{
		m_vTrigger = type;
		m_vTriggerFreq = freq;
		m_vTriggerValue = val;
		connect(this,SIGNAL(signalVelocityTrigger(qreal)),
				receiver,slot);
		m_anyTrigger = true;

	}
	else if (component & LinearMotionTransformTriggers::Acceleration)
	{
		m_aTrigger = type;
		m_aTriggerFreq = freq;
		m_aTriggerValue = val;
		connect(this,SIGNAL(signalAccelerationTrigger(qreal)),
				receiver,slot);
		m_anyTrigger = true;
	}
}

//virtual
void LinearMotionTransform::clearTrigger(quint32 component)
{
	if (component & LinearMotionTransformTriggers::Displacement)
	{
		m_dTrigger = LinearMotionTransformTriggers::UNSET;
	}
	else if (component & LinearMotionTransformTriggers::Velocity)
	{
		m_vTrigger = LinearMotionTransformTriggers::UNSET;
	}
	else if (component & LinearMotionTransformTriggers::Acceleration)
	{
		m_aTrigger = LinearMotionTransformTriggers::UNSET;
	}
	m_anyTrigger = (m_dTrigger & m_vTrigger & m_aTrigger);
}

//public Q_SLOTS:

//virtual
void LinearMotionTransform::slotTimetic(const qreal absTime)
{
	if (m_useAutocount)
	{
		m_autocountAccumulator += m_autocountIncrement;
	}
	else
	{
		m_autocountAccumulator = absTime;
	}
}

//virtual
void LinearMotionTransform::positiveOnly(bool v)
{
	m_posOnly = v;
	if (v)
		m_negOnly = false;
}

//virtual
void LinearMotionTransform::negativeOnly(bool v)
{
	m_negOnly = v;
	if (v)
		m_posOnly = false;
}

//protected:

//
LinearMotionTransform::LinearMotionTransform(const qreal startTime)
:
  m_active(false)
, m_anyTrigger(false)
, m_posOnly(false)
, m_negOnly(false)
, m_startTime(0.0)
, m_useAutocount(false)
, m_autocountIncrement(0.0)
, m_autocountAccumulator(0.0)
, m_initialDisplacement(0.0)
, m_initialVelocity(0.0)
, m_initialAcceleration(0.0)
, m_validLast(false)
, m_lastComputedTime(0.0)
, m_lastComputedAcceleration(0.0)
, m_lastComputedVelocity(0.0)
, m_lastComputedDisplacement(0.0)
, m_aTrigger(LinearMotionTransformTriggers::UNSET)
, m_aTriggerSR(false)
, m_vTrigger(LinearMotionTransformTriggers::UNSET)
, m_vTriggerSR(false)
, m_dTrigger(LinearMotionTransformTriggers::UNSET)
, m_dTriggerSR(false)
{
}

//virtual

LinearMotionTransform::~LinearMotionTransform()
{
}

//virtual
void LinearMotionTransform::parametersChanged()
{
}

//#include <QDebug>
//virtual
void LinearMotionTransform::update(const qreal t,const qreal d,const qreal v,const qreal a)
{
	if (!m_active)
		return;

	bool triggerD = (m_dTrigger == LinearMotionTransformTriggers::UNCONDITIONAL);
	bool triggerV = (m_vTrigger == LinearMotionTransformTriggers::UNCONDITIONAL);
	bool triggerA = (m_aTrigger == LinearMotionTransformTriggers::UNCONDITIONAL);

	if (m_anyTrigger)
	{
		if (!triggerD)
			triggerD = checkTrigger(t,
									d,m_lastComputedDisplacement,
									m_dTrigger,m_dTriggerFreq,
									m_dTriggerValue,m_dTriggerSR);
		if (!triggerV)
			triggerV = checkTrigger(t,
									v,m_lastComputedVelocity,
									m_vTrigger,m_vTriggerFreq,
									m_vTriggerValue,m_vTriggerSR);
		if (!triggerA)
			triggerA = checkTrigger(t,
									a,m_lastComputedAcceleration,
									m_aTrigger,m_aTriggerFreq,
									m_aTriggerValue,m_aTriggerSR);

	}

	updateValuesOnly(t,d,v,a);

	// -- FIELDS UPDATED...NOW THE TRIGGERS CAN ACTIVATE. THIS MAKES SURE ANY RECEIVER QUERYING THIS OBJECT/SUBOBJ ON RESPONSE TO A
	//		TRIGGER SIGNAL WILL SEE VALUES IN A CONSISTENT STATE
	Q_EMIT signalRecomputed(t+m_startTime,d,v,a);
//	//qDebug() << "t,d,v,a : " << t
//			<< " , " << d
//			<< " , " << v
//			<< " , " << a;

	if (triggerA)
		Q_EMIT signalAccelerationTrigger(t+m_startTime,a,m_lastComputedTime,m_lastComputedAcceleration,m_aTrigger);
	if (triggerV)
		Q_EMIT signalVelocityTrigger(t+m_startTime,v,m_lastComputedTime,m_lastComputedVelocity,m_vTrigger);
	if (triggerD)
		Q_EMIT signalDisplacementTrigger(t+m_startTime,d,m_lastComputedTime,m_lastComputedDisplacement,m_dTrigger);
}

//private:

void LinearMotionTransform::updateValuesOnly(const qreal t,const qreal d,const qreal v,const qreal a)
{
	m_validLast = true;
	m_lastComputedTime = t;
	m_lastComputedAcceleration = a;
	m_lastComputedVelocity = v;
	m_lastComputedDisplacement = d;
}

//TODO:		check function doesn't care if the time went forward or backward. This is theoretically wrong...

bool LinearMotionTransform::checkTrigger(const qreal t,
						const qreal val,const qreal lastVal,
						const LinearMotionTransformTriggers::Type trig,
						const LinearMotionTransformTriggers::Freq freq,
						const qreal trigVal,
						bool& 	sr)
{
	if (!trig)		// == UNSET
		return false;
	if (m_validLast)
	{
		if ( ((val > trigVal) && (lastVal > trigVal)) ||
				((val < trigVal) && (lastVal < trigVal)))
		{
			//both were on the same side of the trigval, then there is no way it was either from_less or from_great
			return false;
		}
		if ( (trig == LinearMotionTransformTriggers::FROM_LESSER) && (val >= trigVal) && (lastVal < trigVal) )
			return true;
		if ( (trig == LinearMotionTransformTriggers::FROM_GREATER) && (val <= trigVal) && (lastVal > trigVal) )
			return true;
	}
	if (trig == LinearMotionTransformTriggers::EQUAL)
	{
		if (!qFuzzyCompare(val+1.0,trigVal+1.0))
		{
			sr = false;
			return false;
		}
		if ( (freq == LinearMotionTransformTriggers::SINGLE) && (sr))
			return false;
		sr = true;
		return true;
	}

	//shouldn't have missed any cases...
	return false;
}
