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




#include "webosapp.h"
#include "icon.h"
#include "appmonitor.h"

namespace DimensionsSystemInterface
{

WebOSApp::WebOSApp(const QString& appId,const QString& iconFilePath)
: DimensionsSystemInterface::ExternalApp(ExternalAppType::WebOSApp)
, m_appId(appId)
, m_mainIconFilePath(iconFilePath)
, m_visibility(WOAppVisibility::INVALID)
, m_origin(WOAppOrigin::INVALID)
{
}

//virtual
WebOSApp::~WebOSApp()
{
}

//virtual
bool WebOSApp::isValid() const
{
	return !(m_appId.isEmpty() || m_appId.isNull());
}

//virtual
IconBase * WebOSApp::mainAppIcon() const
{
	LaunchPointsMapConstIter it = m_launchPointsMapById.constFind(m_appId);
	if (it != m_launchPointsMapById.constEnd())
	{
		return it.value();
	}
	return 0;
}

//virtual
QList<IconBase *> WebOSApp::auxAppIcons() const
{
	QList<IconBase *> rlist;
	for (LaunchPointsMapConstIter it = m_launchPointsMapById.constBegin();
			it != m_launchPointsMapById.constEnd();++it)
	{
		if ((it.key() == m_mainLaunchPointId) || (it.key() == m_appId))
		{
			//skip it; it's the main one
			continue;
		}
		if (!(it.value()))
		{
			//it's null...skip it
		}
		rlist << it.value();
	}
	return rlist;
}

//virtual
QString WebOSApp::launchpointIdOfIcon(const QUuid& iconUid,WOAppIconType::Enum * p_r_type) const
{
	IconBase * pIcon = mainAppIcon();
	if (pIcon->uid() == iconUid)
	{
		if (p_r_type)
		{
			*p_r_type = WOAppIconType::Main;
		}
		return m_appId;
	}

	for (LaunchPointsMapConstIter it = m_launchPointsMapById.constBegin();
			it != m_launchPointsMapById.constEnd();++it)
	{
		IconBase * pIcon = it.value();
		if (!pIcon)
		{
			continue;
		}
		if (pIcon->uid() == iconUid)
		{
			//this is the one...

			WOAppIconType::Enum t;
			if ((it.key() == m_mainLaunchPointId) || (it.key() == m_appId))
			{
				//main icon
				t = WOAppIconType::Main;
			}
			else
			{
				t = WOAppIconType::Auxiliary;
			}

			if (p_r_type)
			{
				*p_r_type = t;
			}
			return it.key();
		}
	}
	if (p_r_type)
	{
		*p_r_type = WOAppIconType::INVALID;
	}
	return QString();
}

//virtual
QString WebOSApp::appId() const
{
	return m_appId;
}

//(in case you're wondering why these loop through a static in appmonitor...it was to try and keep as much ApplicationManager dirt inside that class
// and out of this one)

//virtual
QString WebOSApp::launchPointTile(const QString& launchPointId) const
{
	return AppMonitor::webosAppLaunchPointTitle(m_appId,launchPointId);
}

//virtual
QString WebOSApp::title() const
{
	return AppMonitor::webosAppTitle(m_appId);
}

//virtual
QString WebOSApp::version() const
{
	return AppMonitor::webosAppVersion(m_appId);
}

//virtual
QString WebOSApp::category() const
{
	return AppMonitor::webosAppCategory(m_appId);
}

//virtual
bool WebOSApp::nonRemovableSystemApp() const
{
	return AppMonitor::webosAppNonRemovableSystemApp(m_appId);
}

//virtual
bool WebOSApp::platformApp() const
{
	return AppMonitor::webosAppPlatformApp(m_appId);
}

//virtual
bool WebOSApp::userInstalledApp() const
{
	return AppMonitor::webosAppUserInstalledApp(m_appId);
}

//virtual
bool WebOSApp::removableOrHideable() const
{
	return AppMonitor::webosAppRemovableOrHideable(m_appId);
}

} //end namespace
