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




#ifndef CONDITIONALSIGNALTRANSITION_H_
#define CONDITIONALSIGNALTRANSITION_H_

#include <QObject>
#include <QSignalTransition>
#include <QVariant>

class QEvent;
class ConditionalSignalTransition : public QSignalTransition
{
	Q_OBJECT
public:
	ConditionalSignalTransition(QObject * triggerObject,const char * triggerSignal,const QVariant& conditionVar);
	virtual ~ConditionalSignalTransition();

	//returns the old one
	virtual QVariant setCondition(const QVariant& cv);

protected:
	virtual bool eventTest(QEvent *e);

	QVariant m_cv;
};


#endif /* CONDITIONALSIGNALTRANSITION_H_ */
