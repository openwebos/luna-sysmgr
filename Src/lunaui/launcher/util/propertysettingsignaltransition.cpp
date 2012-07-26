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




#include "propertysettingsignaltransition.h"
#include <QEvent>
#include <QStateMachine>

////public:

PropertySettingSignalTransition::PropertySettingSignalTransition(QObject * triggerObject,const char * triggerSignal,
																QObject * targetObject,const QString& propertyName)
: QSignalTransition(triggerObject,triggerSignal)
, m_qp_targetObject(targetObject)
, m_propertyName(propertyName)
{
}

//virtual
PropertySettingSignalTransition::~PropertySettingSignalTransition()
{
}

//virtual
QObject * PropertySettingSignalTransition::setTargetObject(QObject * targetObj)
{
	QObject * v = m_qp_targetObject.data();
	m_qp_targetObject = targetObj;
	return v;

}
//virtual
QString PropertySettingSignalTransition::setPropertyName(const QString& propertyName)
{
	QString v = m_propertyName;
	m_propertyName = propertyName;
	return v;
}

///protected:

//virtual
void PropertySettingSignalTransition::onTransition(QEvent * e)
{
	if (m_qp_targetObject)
	{
		QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(e);
		if (!se->arguments().isEmpty())
		{
			m_qp_targetObject->setProperty(m_propertyName.toAscii().constData(),se->arguments().at(0));
		}
	}
	QSignalTransition::onTransition(e);
}
