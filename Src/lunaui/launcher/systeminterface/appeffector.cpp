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





#include "appeffector.h"
#include "externalapp.h"
#include "webosapp.h"
#include "appmonitor.h"
#include "stringtranslator.h"
#include "ApplicationDescription.h"
#include "ApplicationManager.h"
#include "LaunchPoint.h"
#include "dimensionslauncher.h"

#include "WebAppMgrProxy.h"
#include <cjson/json.h>

#include <QDebug>

namespace DimensionsSystemInterface
{

QPointer<AppEffector> AppEffector::s_qp_instance = 0;

////public:

//static
AppEffector * AppEffector::appEffector()
{
	if (!s_qp_instance)
	{
		s_qp_instance = new AppEffector();
	}
	return s_qp_instance;
}

void AppEffector::launch(ExternalApp * pApp,const QUuid& optionalIconUid)
{
	if (!pApp)
	{
		return;
	}

	//TODO: TEMP: only handle WebOS apps for the moment
	WebOSApp * pWoApp = qobject_cast<WebOSApp *>(pApp);
	if (!pWoApp)
	{
		return;
	}

	ApplicationDescription * pAppDesc = ApplicationManager::instance()->getAppById(StringTranslator::outputString(pWoApp->appId()));
	if (!pAppDesc)
	{
		//check to see if it is a pending application
		pAppDesc = ApplicationManager::instance()->getPendingAppById(StringTranslator::outputString(pWoApp->appId()));
		if (!pAppDesc)
		{
			g_message("%s: error: It's hopeless; app id [%s] that was being launched by the launcher has no mapping in application manager (neither as an installed app nor a pending app. Aborting launch attempt.",
					__FUNCTION__,qPrintable(pWoApp->appId()));
			return;
		}
	}

	//check and see if this is some auxiliary launch point of the app

	//virtual QString launchpointIdOfIcon(const QUuid& iconUid,WOAppIconType::Enum * p_r_type = 0) const;
	WOAppIconType::Enum type;
	QString launchPointId = pWoApp->launchpointIdOfIcon(optionalIconUid,&type);

	std::string launcherAppId = std::string("com.palm.launcher");
	std::string launcherProcessId = std::string("launcher-0");
	std::string procId = std::string("");
	std::string errMsg = std::string("");
	std::string request = pAppDesc->getDefaultLaunchPoint()->params();

	if (type == WOAppIconType::Auxiliary)
	{
		//retrieve the launch point
		const LaunchPoint * pLaunchPoint = pAppDesc->findLaunchPoint(StringTranslator::outputString(launchPointId));
		if (pLaunchPoint)
		{
			request = pLaunchPoint->params();
		}
	}
	//see if the app is actually a built-in
	if (pAppDesc->type() == ApplicationDescription::Type_SysmgrBuiltin)
	{
		///launch direct
		pAppDesc->startSysmgrBuiltIn();
//		//fake a procId
		procId = "SYSMGR_BUILTIN";

	}
	else
	if (procId.empty())
	{
		//normal launch
		procId = WebAppMgrProxy::instance()->appLaunch(
				StringTranslator::outputString(pWoApp->appId()),
				request,
				launcherAppId, launcherProcessId, errMsg);
	}

	if (procId.empty()) {
		qDebug() << "Failed to launch " << pWoApp->appId() << ": " << StringTranslator::inputString(errMsg);
	} else {
		qDebug() << "Request to launch " << pWoApp->appId() << " " << StringTranslator::inputString(request);
	}


}

//TODO: CHANGEME!!! This way of getting info is deprecated, as the JS component of com.palm.launcher that did the work is no longer existing
//				in Dfish and onward.
void AppEffector::info(ExternalApp * pApp)
{
	//TODO: IMPLEMENT (unfinished)
	if (!pApp)
	{
		return;
	}

	//TODO: TEMP: only handle WebOS apps for the moment
	WebOSApp * pWoApp = qobject_cast<WebOSApp *>(pApp);
	if (!pWoApp)
	{
		return;
	}

	ApplicationDescription * pAppDesc = ApplicationManager::instance()->getAppById(StringTranslator::outputString(pWoApp->appId()));
	if (!pAppDesc)
	{
		//qDebug() << "ApplicationDescription for " << pWoApp->appId() << " not found in the AppMan";
		return;
	}

	//see if the app is actually a built-in
	if (pAppDesc->type() == ApplicationDescription::Type_SysmgrBuiltin)
	{
		//TODO: no info for these...handle it better by giving the user something, but for now since the javascript part that gives the
		// info probably will go all weird on a sysmgr builtin, don't try it right now
		return;
	}

	//get the primary launchpoint, because that's what the app info command wants
	const LaunchPoint * pLp =  pAppDesc->getDefaultLaunchPoint();
	if (!pLp)
	{
		//there isn't one? hmm ok.
		return;
	}

	std::string launcherAppId = std::string("com.palm.launcher");
	std::string launcherProcessId = std::string("launcher-0");
	std::string procId = std::string("");
	std::string errMsg = std::string("");
	//TODO: don't form the json manually
	std::string request = std::string("{\"action\":\"showAppInfo\", \"appInfo\":\"") + pLp->launchPointId() + std::string("\"}");

	if (procId.empty())
	{
		//normal launch
		procId = WebAppMgrProxy::instance()->appLaunch(
				launcherAppId,
				request,
				launcherAppId, launcherProcessId, errMsg);
	}

	if (procId.empty()) {
		//qDebug() << "Failed to launch " << pWoApp->appId() << ": " << StringTranslator::inputString(errMsg);
	} else {
		//qDebug() << "Request to launch " << pWoApp->appId() << " " << StringTranslator::inputString(request);
	}

}

void AppEffector::remove(ExternalApp * pApp)
{
	//TODO: IMPLEMENT (unfinished)
	if (!pApp)
	{
		return;
	}

	WebOSApp * pWebOSapp = qobject_cast<WebOSApp *>(pApp);
	pApp->m_stateBeingRemoved = true;		//this is set in case at some point the removal process needs to go async
	qDebug() << __FUNCTION__ << ": Removing app " << pApp->m_uid
			<< (pWebOSapp ? QString(" , ") + pWebOSapp->appId()
												: QString(" (not a webOS app type)"));

	if (!pWebOSapp)
	{
		qDebug() << __FUNCTION__ << ": not supporting removal of non-WebOSApp type at this point";
		return;
	}

	//I'm going to hijack ApplicationManager's LS handle to make a call to remove an app
	json_object* payload = json_object_new_object();
	LSError lserror;
	LSErrorInit(&lserror);

	///TODO: do I want to track remove status???
	json_object_object_add(payload, "id", json_object_new_string(pWebOSapp->appId().toAscii().constData()));
	if (!LSCall(ApplicationManager::instance()->m_serviceHandlePrivate,
			"palm://com.palm.appInstallService/remove",json_object_to_json_string(payload),
			NULL, NULL, NULL, &lserror)) {
		LSErrorFree(&lserror);
	}
	json_object_put(payload);

	//!...this HAS TO BE THE LAST CALL IN THIS FUNCTION!!! and also, make sure nothing much happens up the chain either
#if defined(TARGET_DESKTOP)
	LauncherObject::primaryInstance()->slotAppPreRemove(*pWebOSapp,DimensionsSystemInterface::AppMonitorSignalType::AppManSourced);
#endif

	return;
}

void AppEffector::remove(const QString& webosAppId,const QUuid& iconUid)
{
	WebOSApp * pWebOSapp = AppMonitor::appMonitor()->webosAppByAppId(webosAppId);
	if (!pWebOSapp)
	{
		qDebug() << __FUNCTION__ << ": didn't find a webos app with appId ["<<webosAppId<<"]";
		return;
	}

	ApplicationDescription * pAppDesc = ApplicationManager::instance()->getAppById(StringTranslator::outputString(pWebOSapp->appId()));
	if (!pAppDesc)
	{
		return;
	}

	//if the app is a sysmgr builtin, then ignore
	if (pAppDesc->type() == ApplicationDescription::Type_SysmgrBuiltin)
	{
		return;
	}

	//check to see if the icon that triggered this is an auxiliary launch point or the main one

	WOAppIconType::Enum type;
	QString launchPointId = pWebOSapp->launchpointIdOfIcon(iconUid,&type);

	if (type == WOAppIconType::Auxiliary)
	{
		//yes, auxiliary...delegate to removeLaunchpoint
		g_message("%s: The remove was on one of the app's auxiliary icons (launchpoints)...delegating to another function to do that removal",
				__FUNCTION__);
		return removeLaunchpoint(webosAppId,launchPointId);
	}

	g_message("%s: The remove is on the app's main icon; the whole app will be removed from the system",__FUNCTION__);
	//else, it's the main icon, so the app removal procedure needs to take place

	pWebOSapp->m_stateBeingRemoved = true;		//this is set in case at some point the removal process needs to go async

	//I'm going to hijack ApplicationManager's LS handle to make a call to remove an app
	json_object* payload = json_object_new_object();
	LSError lserror;
	LSErrorInit(&lserror);

	///TODO: do I want to track remove status???
	json_object_object_add(payload, "id", json_object_new_string(pWebOSapp->appId().toAscii().constData()));
	if (!LSCall(ApplicationManager::instance()->m_serviceHandlePrivate,
			"palm://com.palm.appInstallService/remove",json_object_to_json_string(payload),
			NULL, NULL, NULL, &lserror)) {
		LSErrorFree(&lserror);
	}
	json_object_put(payload);
	//!...this HAS TO BE THE LAST CALL IN THIS FUNCTION!!! and also, make sure nothing much happens up the chain either
#if defined(TARGET_DESKTOP)
	LauncherObject::primaryInstance()->slotAppPreRemove(*pWebOSapp,DimensionsSystemInterface::AppMonitorSignalType::AppManSourced);
#endif

	return;
}

void AppEffector::removeLaunchpoint(const QString& webosAppId,const QString& launchpointId)
{
	//call application manager to do it.
	// That call will eventually do a postLaunchPointChange with "removed", which will come back to LauncherObject to get rid of the icon
	std::string extendedStatusString;
	if (! (ApplicationManager::instance()->removeLaunchPoint(StringTranslator::outputString(launchpointId),extendedStatusString)))
	{
		qDebug() << __FUNCTION__ << ": failed: extended error from ApplicationManager = " << StringTranslator::inputString(extendedStatusString);
	}
}

////private:

AppEffector::AppEffector()
{
}

AppEffector::~AppEffector()
{
}

} //end namespace
