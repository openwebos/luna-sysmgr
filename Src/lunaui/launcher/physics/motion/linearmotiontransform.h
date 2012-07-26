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




#ifndef LINEARMOTIONTRANSFORM_H_
#define LINEARMOTIONTRANSFORM_H_

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>

#include "../../dimensionsglobal.h"

namespace LinearMotionTransformTriggers
{
	enum Component
	{
		Displacement,
		Velocity,
		Acceleration,
	};
	enum  Type
	{
		UNSET = 0,			//do not change
		EQUAL,				//really only useful when a value will be converged on
		FROM_LESSER,			//trigger that fires when the value is crossed from less than v (left to right on numberline)
		FROM_GREATER,			//trigger that fires when the value is crossed from greater than v (right to left on numberline)
		UNCONDITIONAL		//on every computation; useful with slotTimetic()
	};
	enum Freq			//only useful with Type == EQUAL for now
	{
		SINGLE,			//only goes off once per EQUAL match.
							//value must un-equal trigger and then come back to equal before next signal

		REPEATED		// it will go off on every subsequent repeated evaluation that EQUALs
	};
}

class LinearMotionTransform : public QObject
{
	Q_OBJECT

public:

	virtual qreal setStartTime(const qreal time);	//returns old start time
	virtual qreal startTime();
	virtual void  activate();
	virtual void  deactivate();
	virtual bool  isActive() const;
	virtual void  restart(const qreal newStartTime);

	virtual qreal setAutocount(const qreal incTime);		//automatically counts time on timetics. (see slotTimetic())
															// returns old autocount increment
	virtual void  disableAutocount();

	virtual qreal setInitialDisplacement(const qreal di);
	virtual qreal setInitialVelocity(const qreal vi);
	virtual qreal setInitialAcceleration(const qreal ai);

	virtual qreal initialDisplacement() const;
	virtual qreal initialVelocity() const;
	virtual qreal initialAcceleration() const;

	virtual qreal displacement(const qreal time);
	virtual qreal velocity(const qreal time);
	virtual qreal acceleration(const qreal time);

	virtual void vec(const qreal time,qreal& d,qreal& v, qreal& a);
	virtual QVector<qreal> vec(const qreal time);

	virtual void setTrigger(quint32 component,
							LinearMotionTransformTriggers::Type type,
							const qreal val,
							QObject * receiver,
							const char * slot,
							LinearMotionTransformTriggers::Freq freq = LinearMotionTransformTriggers::SINGLE);
	virtual void clearTrigger(quint32 component);

	//not hard enforced. interface spec that tells a correctly-implemented tr to never return + or - values for (d,v,a)
	virtual void positiveOnly(bool v=true);
	virtual void negativeOnly(bool v=true);

public Q_SLOTS:

	virtual void slotTimetic(const qreal absTime=0);	//absolute time coordinate (increasing)

Q_SIGNALS:

	void signalDisplacementTrigger(qreal t,qreal d, qreal last_t,qreal last_d, LinearMotionTransformTriggers::Type reason);
	void signalVelocityTrigger(qreal t,qreal v,qreal last_t,qreal last_v, LinearMotionTransformTriggers::Type reason);
	void signalAccelerationTrigger(qreal t,qreal a,qreal last_t,qreal last_a, LinearMotionTransformTriggers::Type reason);

	void signalRecomputed(qreal t,qreal d,qreal v,qreal a);

protected:
	LinearMotionTransform(const qreal startTime=0.0);
	virtual ~LinearMotionTransform();

	//called when initial conditions/parameters are changed
	// (careful to not slice when subclassing and overriding)
	virtual void parametersChanged();
	virtual void update(const qreal t,const qreal d,const qreal v,const qreal a);

	bool								m_active;
	bool								m_anyTrigger;
	bool								m_posOnly;
	bool								m_negOnly;
	qreal								m_startTime;	//abs time
	bool								m_useAutocount;
	qreal								m_autocountIncrement;
	qreal								m_autocountAccumulator;	//abs time
	qreal								m_initialDisplacement;	//these in general ADD to the specific transform params
	qreal								m_initialVelocity;
	qreal								m_initialAcceleration;

	bool								m_validLast;
	qreal 								m_lastComputedTime;	//relative time (to startTime)
	qreal								m_lastComputedAcceleration;
	qreal								m_lastComputedVelocity;
	qreal								m_lastComputedDisplacement;

	LinearMotionTransformTriggers::Type m_aTrigger;
	LinearMotionTransformTriggers::Freq m_aTriggerFreq;
	qreal 								m_aTriggerValue;
	bool								m_aTriggerSR;

	LinearMotionTransformTriggers::Type m_vTrigger;
	LinearMotionTransformTriggers::Freq m_vTriggerFreq;
	qreal 								m_vTriggerValue;
	bool								m_vTriggerSR;

	LinearMotionTransformTriggers::Type m_dTrigger;
	LinearMotionTransformTriggers::Freq m_dTriggerFreq;
	qreal 								m_dTriggerValue;
	bool								m_dTriggerSR;

private:
	void 	updateValuesOnly(const qreal t,const qreal d,const qreal v,const qreal a);
	bool	checkDisplacementTrigger(const qreal t,const qreal d,const qreal v,const qreal a);
	bool	checkVelocityTrigger(const qreal t,const qreal d,const qreal v,const qreal a);
	bool	checkAccelerationTrigger(const qreal t,const qreal d,const qreal v,const qreal a);
	bool 	checkTrigger(const qreal t,
			const qreal val,const qreal lastVal,
			const LinearMotionTransformTriggers::Type trig,
			const LinearMotionTransformTriggers::Freq freq,
			const qreal trigVal,
			bool& sr);
};

#endif /* LINEARMOTIONTRANSFORM_H_ */
