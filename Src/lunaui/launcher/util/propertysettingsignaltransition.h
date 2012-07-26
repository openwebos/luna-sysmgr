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




#ifndef PROPERTYSETTINGSIGNALTRANSITION_H_
#define PROPERTYSETTINGSIGNALTRANSITION_H_

#include <QObject>
#include <QSignalTransition>
#include <QVariant>
#include <QString>
#include <QPointer>

class QEvent;
class PropertySettingSignalTransition : public QSignalTransition
{
	Q_OBJECT
public:
	PropertySettingSignalTransition(QObject * triggerObject,const char * triggerSignal,
									QObject * targetObject,const QString& propertyName);
	virtual ~PropertySettingSignalTransition();

	//old ones returned
	virtual QObject * setTargetObject(QObject * targetObj);
	virtual QString setPropertyName(const QString& propertyName);

protected:
	virtual void onTransition(QEvent * e);

	QPointer<QObject> m_qp_targetObject;
	QString m_propertyName;
};


#endif /* PROPERTYSETTINGSIGNALTRANSITION_H_ */
