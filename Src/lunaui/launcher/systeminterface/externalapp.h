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




#ifndef EXTERNALAPP_H_
#define EXTERNALAPP_H_

#include <QObject>
#include <QUuid>

namespace DimensionsSystemInterface
{

namespace ExternalAppType
{
	enum Enum
	{
		INVALID,
		WebOSApp
	};
}

class ExternalApp : public QObject
{
	Q_OBJECT

public:

	friend class AppEffector;
	friend class AppMonitor;

	ExternalApp(ExternalAppType::Enum type = ExternalAppType::INVALID);
	virtual ~ExternalApp();

	virtual bool isValid() const = 0;
	virtual QUuid uid() const;
	virtual ExternalAppType::Enum type() const;

	virtual bool isUpdating() const;
	virtual bool isInRemoval() const;
	virtual bool isReady() const;		//meaning, ok to launch, etc
	virtual bool isFailed() const;

	virtual void setReady();

protected:

	ExternalAppType::Enum m_type;
	QUuid	m_uid;
	bool	m_stateBeingRemoved;			//set to true if the app is currently undergoing removal
	bool	m_stateBeingUpdated;			//set to true if the app is currently being (re)installed/updated
	bool 	m_stateFailed;					//set to true if the app for whatever reason couldn't be scanned correctly
											// 	(usually an installation fail)
};

} //end namespace

#endif /* EXTERNALAPP_H_ */
