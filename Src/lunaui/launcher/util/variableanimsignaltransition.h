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




#ifndef VARIABLEANIMSIGNALTRANSITION_H_
#define VARIABLEANIMSIGNALTRANSITION_H_

#include <QSignalTransition>

class QState;
class QEvent;

class VariableAnimationSignalTransition : public QSignalTransition
{
	Q_OBJECT
public:
	VariableAnimationSignalTransition( QState * sourceState = 0 );
	VariableAnimationSignalTransition( QObject * sender, const char * signal, QState * sourceState = 0 );

	static VariableAnimationSignalTransition *
		makeTransition(QObject * sender, const char * signal, QState * targerState);

	static VariableAnimationSignalTransition *
		makeTransition(QObject * sender, const char * signal, QState * sourceState, QState * targetState);

	static VariableAnimationSignalTransition *
		makeTransition(QObject * sender, const char * signal, const char * sendersNotifySlot,
						QState * sourceState, QState * targetState);

	void setAnimation(QAbstractAnimation * p_anim);
	void clearAnimations();

Q_SIGNALS:

	void signalSetupAnimation();

protected:
	virtual void onTransition( QEvent * event );

};

#endif /* VARIABLEANIMSIGNALTRANSITION_H_ */
