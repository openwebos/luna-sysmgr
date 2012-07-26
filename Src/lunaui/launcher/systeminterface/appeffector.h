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




#ifndef APPEFFECTOR_H_
#define APPEFFECTOR_H_

#include <QObject>
#include <QPointer>
#include <QUuid>

class ApplicationDescription;

namespace DimensionsSystemInterface
{

class ExternalApp;

class AppEffector : public QObject
{
	Q_OBJECT

public:

	static AppEffector * appEffector();

	//makes a request to the outside world to launch the app specified
	void launch(ExternalApp * pApp,const QUuid& optionalIconUid = QUuid());
	//makes a request to the outside world to bring up app info
	void info(ExternalApp * pApp);
	//makes a request to the outside world to start app removal (appinstaller, etc)
	void remove(ExternalApp * pApp);
	//the webos-appId variant...the iconUid is the icon that was activated to trigger the remove. This is needed because webos apps may have multiple launchpoints; each of these has an icon
	//	but only the main icon can trigger an app delete
	void remove(const QString& webosAppId,const QUuid& iconUid);

	//remove launchpoint for the webos-app
	void removeLaunchpoint(const QString& webosAppId,const QString& launchpointId);

private:

	AppEffector();
	~AppEffector();

	static QPointer<AppEffector> s_qp_instance;
};

} //end namespace

#endif /* APPEFFECTOR_H_ */
