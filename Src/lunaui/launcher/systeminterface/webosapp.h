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




#ifndef WEBOSAPP_H_
#define WEBOSAPP_H_

#include "externalapp.h"
#include <QString>
#include <QMap>
#include <QPointer>

class IconBase;

namespace DimensionsSystemInterface
{

class AppMonitor;

namespace WOAppVisibility
{
	enum Enum
	{
		INVALID,
		Visible,
		Invisible
	};
}
namespace WOAppOrigin
{
	enum Enum
	{
		INVALID = 0,
		Unknown,			// catch-all
		ROM,				// index entry, use specific ones below
		ROMImage,			// provided on the image coming out of the factory; core apps needed for critical functions (e.g. Phone app)
		NonROM,				// index entry, use specific ones below
		NonROMPlatform,		// provided by the factory, but not on the ROM image; feature apps not critical for operation (e.g. doc viewer)
		NonROMUpdating,		// company apps that are intended to be able to update outside the OTA ROM update cycle; otherwise identical to NonROMPlatform
		NonROMPartnerVendor,	// provided by a launch partner; feature apps not critical for operation (e.g. facebook)
		User,				//index entry, use specific ones below
		UserInstalled		// the general apps; installed by the user from an app catalog/other source
	};
}

namespace WOAppIconType
{
	enum Enum
	{
		INVALID,
		Main,
		Auxiliary
	};
}

class WebOSApp : public ExternalApp
{
	Q_OBJECT
public:

	friend class AppMonitor;

	WebOSApp(const QString& appId,const QString& iconFilePath);
	virtual ~WebOSApp();

	virtual bool isValid() const;

	virtual IconBase * mainAppIcon() const;
	virtual QList<IconBase *> auxAppIcons() const;
	virtual QString launchpointIdOfIcon(const QUuid& iconUid,WOAppIconType::Enum * p_r_type = 0) const;

	virtual QString launchPointTile(const QString& launchPointId) const;
	virtual QString appId() const;
	virtual QString title() const;
	virtual QString version() const;
	virtual QString category() const;

	virtual bool	nonRemovableSystemApp() const;
	virtual bool	platformApp() const;
	virtual bool	userInstalledApp() const;
	virtual bool	removableOrHideable() const;

protected:

	QString m_appId;
	QString m_mainLaunchPointId;
	QString m_mainIconFilePath;
	QString m_appLocationPath;	//on the filesys
	WOAppVisibility::Enum m_visibility;
	WOAppOrigin::Enum m_origin;

	typedef QMap<QString,QPointer<IconBase> > LaunchPointsMap;
	typedef LaunchPointsMap::iterator LaunchPointsMapIter;
	typedef LaunchPointsMap::const_iterator LaunchPointsMapConstIter;

	LaunchPointsMap m_launchPointsMapById;		//includes the main launch point

};

} //end namespace

#endif /* WEBOSAPP_H_ */
