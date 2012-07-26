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




#include "variableanimsignaltransition.h"
#include <QEvent>
#include <QState>
#include <QAbstractAnimation>

VariableAnimationSignalTransition::VariableAnimationSignalTransition( QState * sourceState)
: QSignalTransition(sourceState)
{
}

VariableAnimationSignalTransition::VariableAnimationSignalTransition( QObject * sender, const char * signal, QState * sourceState)
: QSignalTransition(sender,signal,sourceState)
{
}

//static
VariableAnimationSignalTransition *
VariableAnimationSignalTransition::makeTransition(QObject * sender, const char * signal, QState * targetState)
{
	VariableAnimationSignalTransition * p = new VariableAnimationSignalTransition(sender,signal);
	p->setTargetState(targetState);
	return p;
}

//static
VariableAnimationSignalTransition *
VariableAnimationSignalTransition::makeTransition(QObject * sender, const char * signal, QState * sourceState, QState * targetState)
{
	VariableAnimationSignalTransition * p = new VariableAnimationSignalTransition(sender,signal,sourceState);
	p->setTargetState(targetState);
	return p;
}

//static
VariableAnimationSignalTransition *
VariableAnimationSignalTransition::makeTransition(QObject * sender, const char * signal, const char * sendersNotifySlot,
						QState * sourceState, QState * targetState)
{
	VariableAnimationSignalTransition * p = new VariableAnimationSignalTransition(sender,signal,sourceState);
	p->setTargetState(targetState);
	connect(p,SIGNAL(signalSetupAnimation()),sender,sendersNotifySlot);
	return p;
}

void VariableAnimationSignalTransition::setAnimation(QAbstractAnimation * p_anim)
{
	if (!p_anim)
		return;
	clearAnimations();
	addAnimation(p_anim);
}

void VariableAnimationSignalTransition::clearAnimations()
{
	QList<QAbstractAnimation *> list = animations();
	Q_FOREACH(QAbstractAnimation * pAnim , list)
	{
		removeAnimation(pAnim);
		delete pAnim;
	}
}

//virtual
void VariableAnimationSignalTransition::onTransition( QEvent * event )
{
	//allow a chance to intercept this at the earliest possible
	Q_EMIT signalSetupAnimation();
	//call the base class' version to do the usual processing
	QSignalTransition::onTransition(event);
}
