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




#ifndef FRICTIONTRANSFORM_H_
#define FRICTIONTRANSFORM_H_

#include "linearmotiontransform.h"

class FrictionTransform : public LinearMotionTransform
{
	Q_OBJECT
public:
	FrictionTransform();
	virtual ~FrictionTransform();

	virtual void    restart(const qreal newStartTime=0.0);

	qreal			setInitialForce(const qreal force);			//can specify by either force or accel+mass
	qreal			setInitialAcceleration(const qreal accel);
	qreal			setObjectMass(const qreal mass);
	qreal			setInitialMomentTime(const qreal time);
	qreal			setCoeffKineticFriction(const qreal coeffKinFriction);
	qreal			coeffKineticFriction() const;
	qreal			initialForce() const;
	qreal			initialAcceleration() const;
	qreal			objectMass() const;
	qreal			initialMomentTime() const;

	virtual qreal 	displacement(const qreal time);
	virtual qreal 	velocity(const qreal time);
	virtual qreal 	acceleration(const qreal time);

	virtual void 	vec(const qreal time,qreal& d,qreal& v, qreal& a);
	virtual QVector<qreal> vec(const qreal time);

	virtual qreal	haltTime() const;

	virtual void	setReverse();
	virtual void	setForward();

public Q_SLOTS:

	virtual void slotTimetic(const qreal absTime=0);	//absolute time coordinate (increasing)

protected:

	virtual void parametersChanged();

	//PARAMETERS
	qreal m_initialAcceleration;
	qreal m_objectMass;
	qreal m_forceMomentTime;
	qreal m_reverse;		//-1.0 to let the displacement go in reverse

	qreal m_coeffFrictionKinetic;

	//INVARIANTS
	qreal m_coeffFrictionStatic;
	qreal m_gravityAccel;

	qreal m_computedInitialVelocity;
	qreal m_computedAccelStaticFriction;
	qreal m_computedAccelKineticFriction;
	qreal m_computedGrindToHaltTime;
	qreal m_computedFinalDisplacement;
};

#endif /* FRICTIONTRANSFORM_H_ */
