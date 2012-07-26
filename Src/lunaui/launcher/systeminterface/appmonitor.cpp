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




#include "appmonitor.h"
#include "iconheap.h"
#include "icon.h"
#include "stringtranslator.h"
#include "operationalsettings.h"
#include "dimensionslauncher.h"
#include "pixmaploader.h"
#include "ApplicationDescription.h"
#include "ApplicationManager.h"
#include "LaunchPoint.h"

#include "safefileops.h"

#include <QDebug>

namespace DimensionsSystemInterface
{

QPointer<AppMonitor> AppMonitor::s_qp_mainInstance = 0;


//static
AppMonitor * AppMonitor::appMonitor()
{
	if (!s_qp_mainInstance)
	{
		//app monitor needs the IconHeap initialized to work correctly
		IconHeap::iconHeap();

		new AppMonitor();
		connect(ApplicationManager::instance(),SIGNAL(signalInitialScanStart()),
				s_qp_mainInstance,SLOT(slotInitialScanStart()));
		connect(ApplicationManager::instance(),SIGNAL(signalInitialScanEnd()),
				s_qp_mainInstance,SLOT(slotInitialScanEnd()));
		connect(ApplicationManager::instance(),SIGNAL(signalScanFoundApp(const ApplicationDescription *)),
				s_qp_mainInstance,SLOT(slotScanFoundApp(const ApplicationDescription *)));
		connect(ApplicationManager::instance(),SIGNAL(signalScanFoundAuxiliaryLaunchPoint(const ApplicationDescription *,const LaunchPoint *)),
				s_qp_mainInstance,SLOT(slotScanFoundAuxiliaryLaunchPoint(const ApplicationDescription *,const LaunchPoint *)));

		connect(ApplicationManager::instance(),SIGNAL(signalLaunchPointAdded(const LaunchPoint*,QBitArray)),
				s_qp_mainInstance,SLOT(slotLaunchPointAdded(const LaunchPoint*,QBitArray))
				);
		connect(ApplicationManager::instance(),SIGNAL(signalLaunchPointUpdated(const LaunchPoint*,QBitArray)),
				s_qp_mainInstance,SLOT(slotLaunchPointUpdated(const LaunchPoint*,QBitArray))
				);
		connect(ApplicationManager::instance(),SIGNAL(signalLaunchPointRemoved(const LaunchPoint*,QBitArray)),
				s_qp_mainInstance,SLOT(slotLaunchPointRemoved(const LaunchPoint*,QBitArray))
				);
	}
	return s_qp_mainInstance;
}

AppMonitor::AppMonitor()
: m_withinInitialScan(false)
, m_fullScanCounter(0)
{
	if (!s_qp_mainInstance)
	{
		s_qp_mainInstance = this;
	}

	//load the blacklist
	m_appBlacklist.loadDefault();
	
	//load the keyword->page designator map
	loadDesignatorKeywordMapping(OperationalSettings::settings()->appKeywordsToPageDesignatorMapFilepath);
}

//virtual
AppMonitor::~AppMonitor()
{
}

bool AppMonitor::completedScan() const
{
	return (m_fullScanCounter > 0);
}

QList<QUuid> AppMonitor::allAppUids() const
{
	return m_appMapByUid.keys();
}

QList<QPointer<ExternalApp> > AppMonitor::allApps() const
{
	return m_appMapByUid.values();
}

ExternalApp * AppMonitor::appByUid(const QUuid& uid) const
{
	return find(uid);
}

WebOSApp * AppMonitor::webosAppByAppId(const QString& appId) const
{
	return pointerToWebOSApp(appId);
}

bool AppMonitor::isAppRemovable(const QUuid& appUid)
{
	//only support webosapp type
	WebOSApp * pWoApp = qobject_cast<WebOSApp *>(find(appUid));
	if (!pWoApp)
	{
		return true;		//just default to removable
	}

	return webosAppRemovableOrHideable(pWoApp->appId());
}

//static
QString AppMonitor::webosAppLaunchPointTitle(const QString& appId,const QString& launchPointId)
{
	ApplicationDescription * pAppdescriptor = ApplicationManager::instance()->getAppById(StringTranslator::outputString(appId));
	if (pAppdescriptor)
	{
		const LaunchPoint * pLaunchpoint = pAppdescriptor->findLaunchPoint(StringTranslator::outputString(launchPointId));
		if (pLaunchpoint)
		{
			return StringTranslator::inputString(pLaunchpoint->title());
		}
	}
	return QString();
}

//static
QString AppMonitor::webosAppTitle(const QString& appId)
{

	ApplicationDescription * pAppdescriptor = ApplicationManager::instance()->getAppById(StringTranslator::outputString(appId));
	if (pAppdescriptor)
	{
		return StringTranslator::inputString(pAppdescriptor->title());
	}
	return QString();

}

//static
QString AppMonitor::webosAppVersion(const QString& appId)
{
	ApplicationDescription * pAppdescriptor = ApplicationManager::instance()->getAppById(StringTranslator::outputString(appId));
	if (pAppdescriptor)
	{
		return StringTranslator::inputString(pAppdescriptor->version());
	}
	return QString();
}

//static
QString AppMonitor::webosAppCategory(const QString& appId)
{
	ApplicationDescription * pAppdescriptor = ApplicationManager::instance()->getAppById(StringTranslator::outputString(appId));
	if (pAppdescriptor)
	{
		return StringTranslator::inputString(pAppdescriptor->category());
	}
	return QString();
}

//static
bool	AppMonitor::webosAppNonRemovableSystemApp(const QString& appId)
{
	ApplicationDescription * pAppdescriptor = ApplicationManager::instance()->getAppById(StringTranslator::outputString(appId));
	if (pAppdescriptor)
	{
		return (!(pAppdescriptor->isRemovable()));
	}
	return true;
}

//static
bool AppMonitor::webosAppPlatformApp(const QString& appId)
{
	return ApplicationManager::instance()->isFactoryPlatformApp(StringTranslator::outputString(appId));
}

//TODO: tweak this to your neverending liking!
//static
bool AppMonitor::webosAppUserInstalledApp(const QString& appId)
{
	return !webosAppPlatformApp(appId);
}

//static
bool AppMonitor::webosAppRemovableOrHideable(const QString& appId)
{
	ApplicationDescription * pAppdescriptor =
			ApplicationManager::instance()->getAppById(StringTranslator::outputString(appId));
	if (pAppdescriptor)
	{
		return (pAppdescriptor->isUserHideable() || pAppdescriptor->isRemovable());
	}
	return true;
}

//////public Q_SLOTS:

//virtual
void AppMonitor::slotInitialScanStart()
{
	m_withinInitialScan = true;
}

//virtual
void AppMonitor::slotInitialScanEnd()
{
	++m_fullScanCounter;
	m_withinInitialScan = false;

	Q_EMIT signalFullScanCompleted(true);

}

void AppMonitor::slotScanFoundApp(const ApplicationDescription * pAppdescriptor)
{
	if (!pAppdescriptor)
	{
		return;
	}

	QString appId = StringTranslator::inputString(pAppdescriptor->id());

	//does the app already exist here (already been signaled for this appid)?
	if (pointerToWebOSApp(appId))
	{
		qDebug() << __FUNCTION__ << ": app [" << appId << "] has already been recognized and processed...ignoring this additional signal";
		return;
	}

	//if the app has been blacklisted, skip
	if (m_appBlacklist.deny(appId))
	{
		qDebug() << __FUNCTION__ << ": app [" << appId << "] has been blacklisted and is being ignored";
		return;
	}

	//check to see if the ApplicationDescriptor wants this to be a hidden app
	if ((!pAppdescriptor->isVisible()) && OperationalSettings::settings()->useApplicationManagerHiddenFlag)
	{
		qDebug() << __FUNCTION__ << ": app [" << appId << "] has been marked isVisible = false by AppMan and is being ignored";
		return;
	}

	//if the app if pending, then split off here
	WebOSApp * pNewWebOSapp = 0;
	if (pAppdescriptor->status() != ApplicationDescription::Status_Ready)
	{
		if ( (pNewWebOSapp = newPendingWebOSApp(pAppdescriptor)) == 0)
		{
			qDebug() << __FUNCTION__ << ": (pending) app [" << appId << "] failed to create";
			return;
		}
	}
	else
	{
		if ((pNewWebOSapp = newWebOSApp(pAppdescriptor)) == 0)
		{
			qDebug() << __FUNCTION__ << ": app [" << appId << "] failed to create";
			return;
		}
	}
	//TODO: signal further up the chain or set state so it's done later
	Q_EMIT signalNewApp(*pNewWebOSapp);
}

//virtual
void AppMonitor::slotScanFoundAuxiliaryLaunchPoint(const ApplicationDescription * p_appDesc,const LaunchPoint * p_launchPoint)
{
	//This happens as result of ApplicationManager::scanForLaunchPoints, and should ALWAYS ALWAYS ALWAYS
	// happen after ALL of the apps have been scanned. In other words, appmonitor has ALL of the main
	// app structures and icons already processed.
	// This is vital assumption as auxiliar launch points must be added into the WebOSApp object for the app
	// in order to be correctly recognized by the launcher3 codebase

	if (!p_appDesc || !p_launchPoint)
	{
		return;
	}
	//retrieve the app object
	QString appId = StringTranslator::inputString(p_appDesc->id());
	WebOSApp * pWebOSapp = webosAppByAppId(appId);
	if (!pWebOSapp)
	{
		g_message("%s: could not locate WebOSApp entry for passed-in app desc with id = %s",__FUNCTION__,qPrintable(appId));
		return;
	}

	IconBase * pAuxIcon = createAppIcon(StringTranslator::inputString((p_launchPoint)->iconPath()),
			StringTranslator::inputString((p_launchPoint)->title()));
	if (!pAuxIcon)
	{
		QStringList dbgstr;
		dbgstr << __FUNCTION__ << ": error: couldn't create non-primary launch point icon for appid " << appId
				<< " , icon file: " << StringTranslator::inputString((p_launchPoint)->iconPath())
		<< " , title: " << StringTranslator::inputString((p_launchPoint)->title());
		g_message("%s: %s",__FUNCTION__,qPrintable(dbgstr.join("")));
		return;
	}
	QString launchpointId = StringTranslator::inputString((p_launchPoint)->launchPointId());
	//add icon to the heap
	//TODO: error check on add
	IconHeap::iconHeap()->addIcon(pAuxIcon,appId,launchpointId);
	pWebOSapp->m_launchPointsMapById.insert(launchpointId,pAuxIcon);

	//pass on the information
	Q_EMIT signalNewAdditionalLaunchPointForApp(pWebOSapp->uid(),pAuxIcon->uid());
}

//virtual
WebOSApp * AppMonitor::newWebOSApp(const ApplicationDescription * pAppdescriptor)
{

	/*
	 * DO NOT CALL ME FROM UNCHECKED FUNCTIONS
	 */

	QString appId = StringTranslator::inputString(pAppdescriptor->id());

	//get the launch point list, and immediately the default LP
	const LaunchPointList * pLaunchPointList = &(pAppdescriptor->launchPoints());
	if (pLaunchPointList->empty())
	{
		//bail...
		//TODO: add the possibility of having launchpoint-less app recognized by the launcher
		return 0;
	}
	const LaunchPoint * pMainLaunchPoint = pAppdescriptor->getDefaultLaunchPoint();
	if (!pMainLaunchPoint)
	{
		//again, bail...
		//TODO: same as above
		return 0;
	}

	//create a WebOSApp entry for it, including the icon, etc
	WebOSApp * pWebOSapp = new WebOSApp(appId,StringTranslator::inputString(pMainLaunchPoint->iconPath()));
	//set some things on it
	pWebOSapp->m_mainLaunchPointId = StringTranslator::inputString(pMainLaunchPoint->launchPointId());
	pWebOSapp->m_appLocationPath = StringTranslator::inputString(pAppdescriptor->folderPath());
	//TODO: TEMP: unhardcode
	pWebOSapp->m_origin = WOAppOrigin::Unknown;
	pWebOSapp->m_visibility = WOAppVisibility::Visible;

	QString iconLabel = StringTranslator::inputString(pAppdescriptor->title());
	if (iconLabel.isEmpty())
	{
		iconLabel = QString("(?)");
	}

	//try and load its main icon
	IconBase * pMainIcon = createAppIcon(pWebOSapp->m_mainIconFilePath,iconLabel);
	if (!pMainIcon)
	{
		//bailamos once again!
		delete pWebOSapp;
		return 0;
	}

	//(add icon to the heap)
	IconHeap::iconHeap()->addIcon(pMainIcon,pWebOSapp->m_appId,pWebOSapp->m_mainLaunchPointId);

	pWebOSapp->m_launchPointsMapById.insert(pWebOSapp->m_appId,pMainIcon);
	pWebOSapp->m_launchPointsMapById.insert(pWebOSapp->m_mainLaunchPointId,pMainIcon);

	//add to the tracking maps in this class
	m_appMapByUid.insert(pWebOSapp->uid(),pWebOSapp);
	m_aliasWebosAppMapByAppId.insert(pWebOSapp->m_appId,pWebOSapp);

	return pWebOSapp;
}

//virtual
IconBase * AppMonitor::createAppIcon(const QString& mainIconFile,const QString& iconLabel)
{
	//try and load its main icon
	qDebug() << __FUNCTION__ << ": entry: mainIconFile = " << mainIconFile << " , iconLabel = " << iconLabel;
	IconBase * pMainIcon = IconHeap::makeIconConstrainedStandardFrameAndDecorators(mainIconFile,QSize(64,64));
	if (!pMainIcon)
	{
		//bail'amos!
		return 0;
	}

	pMainIcon->slotChangeIconVisibility(true);

	//load the decorators
	PixmapObject * pRemovePmo = IconHeap::iconHeap()->commonImageRemoveDecoratorNormal();
	PixmapObject * pRemovePressedPmo = IconHeap::iconHeap()->commonImageRemoveDecoratorPressed();
	PixmapObject * pDeletePmo = IconHeap::iconHeap()->commonImageDeleteDecoratorNormal();
	PixmapObject * pDeletePressedPmo = IconHeap::iconHeap()->commonImageDeleteDecoratorPressed();

	PixmapObject * pOld = 0;
	pMainIcon->slotUpdateRemoveDecoratorPic(RemoveDeleteDecoratorState::Normal,pRemovePmo,pOld);
	pMainIcon->slotUpdateRemoveDecoratorPic(RemoveDeleteDecoratorState::Activated,pRemovePressedPmo,pOld);
	pMainIcon->slotUpdateDeleteDecoratorPic(RemoveDeleteDecoratorState::Normal,pDeletePmo,pOld);
	pMainIcon->slotUpdateDeleteDecoratorPic(RemoveDeleteDecoratorState::Activated,pDeletePressedPmo,pOld);

	pMainIcon->setProperty(IconBase::IconLabelPropertyName,iconLabel);
	return pMainIcon;
}

//virtual
void AppMonitor::loadDesignatorKeywordMapping(const QString& mapFile)
{
	if (mapFile.isEmpty())
	{
		return;
	}
	qDebug() << __FUNCTION__ << ": reading from " << mapFile;
	SafeFileOperator safesave(SafeFileOperator::Read,mapFile,QSettings::IniFormat);
	QSettings& settings = safesave.safeSettings();
	if (settings.status() != QSettings::NoError)
	{
		//problem with the file op
		return;
	}

	m_auxDesignatorList.clear();
	int numDesignators = settings.beginReadArray("designators");
	QSet<QString> designatorSet;
	for (int i = 0; i < numDesignators;++i)
	{
		settings.setArrayIndex(i);
		QString designator = settings.value("designator",QString("")).toString();
		if (designator.isEmpty()
			|| (designator == Page::PageDesignatorFavorites()) )
		{
			continue;
		}
		if (designatorSet.contains(designator))
		{
			continue; //duplicate
		}
		m_auxDesignatorList << designator;
		//if there is a name associated, load the mapping
		QString name = settings.value("name",QString("")).toString();
		if (!name.isEmpty())
		{
			m_designatorToNameMap[designator] = name;
		}
	}

	settings.endArray();

	int numKeywords = settings.beginReadArray("keywords");
	if (numKeywords == 0)
	{
		return;
	}

	for (int i = 0;i < numKeywords;++i)
	{
		settings.setArrayIndex(i);
		QString keyword = settings.value("keyword",QString("")).toString().toLower();
		QString designator = settings.value("designator",QString("")).toString();
		if ((keyword.isEmpty()) || (designator.isEmpty()))
		{
			continue;
		}
		m_pageDesignatorByKeyword[keyword] = designator;
	}

	return;
}

QString AppMonitor::pageDesignatorForWebOSApp(const QString& appId)
{
	//get the app descriptor
	ApplicationDescription * pAppDesc = ApplicationManager::instance()->getAppById(StringTranslator::outputString(appId));
	if (!pAppDesc)
	{
		return QString();
	}

	//check category first
	QMap<QString,QString>::const_iterator f =
			m_pageDesignatorByKeyword.constFind(StringTranslator::inputString(pAppDesc->category()).toLower());
	if (f != m_pageDesignatorByKeyword.constEnd())
	{
		return f.value();
	}

	//no? ok, then go through all the keywords
	std::list<std::string> keywList = pAppDesc->keywords();
	for (std::list<std::string>::iterator it = keywList.begin();
			it != keywList.end();++it)
	{
		if (!(it->empty()))
		{
			if ( (f = m_pageDesignatorByKeyword.constFind(StringTranslator::inputString(*it).toLower()))
					!=  m_pageDesignatorByKeyword.constEnd())
			{
				return f.value();
			}
		}
	}
	return QString();
}

//TODO: placeholder
QString AppMonitor::pageNameFromDesignator(const QString& designator)
{
	//look it up from the name map
	QMap<QString,QString>::iterator f = m_designatorToNameMap.find(designator);
	QString name;
	if (f != m_designatorToNameMap.end())
	{
		if (!(*f).isEmpty())
		{
			name = *f;
		}
		else
		{
			name = designator;
		}
	}
	else
	{
		name = designator;
	}
	return name.toUpper();
}

QList<QString> AppMonitor::auxPageDesignators() const
{
	return m_auxDesignatorList;
}

//static
QString AppMonitor::appIdFromLaunchpointId(const QString& launchpointId)
{
	if (ApplicationManager::instance())
	{
		const LaunchPoint * pLaunchpoint = ApplicationManager::instance()->getLaunchPointById(StringTranslator::outputString(launchpointId));
		if (pLaunchpoint)
		{
			if (pLaunchpoint->appDesc())
			{
				return StringTranslator::inputString(pLaunchpoint->appDesc()->id());
			}
		}
		return QString();
	}
	else
	{
		//use a brittle, hard-assumption method of stripping anything after the first '_' encountered, searching in reverse on the launchpoint id string
		QString appId  = launchpointId;
		appId.truncate(qMax((int)0,(int)(launchpointId.indexOf(QChar('_'),launchpointId.size()-1))));
		return appId;
	}
	return QString();
}

//virtual
WebOSApp * AppMonitor::newPendingWebOSApp(const ApplicationDescription * pAppdescriptor)
{

	QString appId = StringTranslator::inputString(pAppdescriptor->id());
	//get the launch point list, and immediately the default LP
	const LaunchPointList * pLaunchPointList = &(pAppdescriptor->launchPoints());
	if (pLaunchPointList->empty())
	{
		//bail...
		//TODO: add the possibility of having launchpoint-less app recognized by the launcher
		return 0;
	}
	const LaunchPoint * pMainLaunchPoint = pAppdescriptor->getDefaultLaunchPoint();
	if (!pMainLaunchPoint)
	{
		//again, bail...
		//TODO: same as above
		return 0;
	}

	//create a WebOSApp entry for it, including the icon, etc
	qDebug() << "icon file paths: launchpoint: " << StringTranslator::inputString(pMainLaunchPoint->iconPath());
	WebOSApp * pWebOSapp = new WebOSApp(appId,StringTranslator::inputString(pMainLaunchPoint->iconPath()));
	//set some things on it
	pWebOSapp->m_mainLaunchPointId = StringTranslator::inputString(pMainLaunchPoint->launchPointId());
	pWebOSapp->m_appLocationPath = StringTranslator::inputString(pAppdescriptor->folderPath());
	//TODO: TEMP: unhardcode
	pWebOSapp->m_origin = WOAppOrigin::Unknown;
	pWebOSapp->m_visibility = WOAppVisibility::Visible;

	QString iconLabel = StringTranslator::inputString(pAppdescriptor->title());
	if (iconLabel.isEmpty())
	{
		iconLabel = StringTranslator::inputString(pMainLaunchPoint->title());
		if (iconLabel.isEmpty())
		{
			iconLabel = QString("(?)");
		}
	}

	//try and load its main icon
	IconBase * pMainIcon = createAppIcon(pWebOSapp->m_mainIconFilePath,iconLabel);
	if (!pMainIcon)
	{
		//bailamos once again!
		delete pWebOSapp;
		return 0;
	}

	ApplicationDescription::Status appStatus = pAppdescriptor->status();
	PixmapObject * pOld = 0;
	PixmapObject * pStatusPmo = 0;
	if (appStatus == ApplicationDescription::Status_Failed)
	{
		pWebOSapp->m_stateFailed = true;
		pStatusPmo = IconHeap::iconHeap()->commonImageWarningDecorator();
		pMainIcon->slotUpdateInstallStatusDecoratorPic(pStatusPmo,pOld);
	}
	else if (appStatus == ApplicationDescription::Status_Installing)
	{
		pWebOSapp->m_stateBeingUpdated = true;
		pStatusPmo = IconHeap::iconHeap()->commonImageProgressFilmstrip();
		pMainIcon->slotUpdateInstallStatusDecoratorPic(pStatusPmo,pOld);
		pMainIcon->slotUpdateInstallStatusDecoratorResetProgress(0,100);
	}
	else if (appStatus == ApplicationDescription::Status_Updating)
	{
		pWebOSapp->m_stateBeingUpdated = true;
		pStatusPmo = IconHeap::iconHeap()->commonImageProgressFilmstrip();
		pMainIcon->slotUpdateInstallStatusDecoratorPic(pStatusPmo,pOld);
		pMainIcon->slotUpdateInstallStatusDecoratorResetProgress(0,100);
	}
	else if (appStatus == ApplicationDescription::Status_Ready)
	{
		pWebOSapp->m_stateBeingUpdated = false;
		pWebOSapp->m_stateFailed = false;
		pWebOSapp->m_stateBeingRemoved=false;
	}
	//(add icon to the heap)
	IconHeap::iconHeap()->addIcon(pMainIcon,pWebOSapp->m_appId,pWebOSapp->m_mainLaunchPointId);

	pWebOSapp->m_launchPointsMapById.insert(pWebOSapp->m_appId,pMainIcon);
	pWebOSapp->m_launchPointsMapById.insert(pWebOSapp->m_mainLaunchPointId,pMainIcon);

	//add to the tracking maps in this class
	m_appMapByUid.insert(pWebOSapp->uid(),pWebOSapp);
	m_aliasWebosAppMapByAppId.insert(pWebOSapp->m_appId,pWebOSapp);

	return pWebOSapp;
}

void AppMonitor::slotAppBeingRemoved(const ApplicationDescription * pAppdescriptor)
{
	//TODO: IMPLEMENT

	//use the application's id to be used to get the main (master) icon's uid
	if (!pAppdescriptor)
	{
		//qDebug() << "ApplicationDescription for " << pWoApp->appId() << " not found in the AppMan";
		return;
	}

	WebOSApp * pWoApp = webosAppByAppId(StringTranslator::inputString(pAppdescriptor->id()));
	slotAppBeingRemoved(pWoApp);
}

void AppMonitor::slotAppBeingRemoved(WebOSApp * p_webOSapp)
{
	g_warning("%s: entry",__FUNCTION__);

	if (!p_webOSapp)
	{
		g_warning("%s: early-exit, null app ptr passed",__FUNCTION__);
		return;
	}
	QUuid removedAppUid = p_webOSapp->uid();
	Q_SIGNAL signalRemovedApp(*p_webOSapp,AppMonitorSignalType::AppManSourced);
	//remove from the lists
	remove(p_webOSapp);
	//finally signal that it's finished removing, in case any post steps need to be taken at the
	// LauncherObject or similar level
	Q_SIGNAL signalRemovedAppComplete(removedAppUid,AppMonitorSignalType::AppManSourced);
}

void AppMonitor::slotAppUpdated(const ApplicationDescription * pAppdescriptor)
{
	//TODO: IMPLEMENT
}

void AppMonitor::slotLaunchPointAdded(const LaunchPoint* p_launchpoint,QBitArray reasons)
{
	if (!p_launchpoint)
	{
		return;
	}

	//get the appid and launchpoint id
	QString appId = StringTranslator::inputString(p_launchpoint->id());
	QString launchpointId = StringTranslator::inputString(p_launchpoint->launchPointId());

	//grab the application descriptor
	const ApplicationDescription * pAppdescriptor = p_launchpoint->appDesc();
	if (!pAppdescriptor)
	{
		//TODO: is this correct? could there be a launch point that as no associated app desc?
		qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- no app descriptor associated with launchpointId";
		return;
	}
	//check to see if this icon already exists
	if (IconHeap::iconHeap()->getIcon(appId,launchpointId))
	{
		//TODO: if this came from a postLaunchPointUpdate added "reason", then it's actually the funky-rotten way the app installer
		//	is advertising the final update to an installed app.

		qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- SPECIAL CASE: this Add is really the last Install Update...calling update handler...";
		//convert this to an Update, with a reason of Status. It (slotLaunchPointUpdated) SHOULD pick up ApplicationStatus::Ready and do the right thing
		QBitArray updateReason = QBitArray(LaunchPointUpdatedReason::SIZEOF);
		updateReason.setBit(LaunchPointUpdatedReason::Status);
		return slotLaunchPointUpdated(p_launchpoint,updateReason);
	}
	//try and get the app uid for the app
	WebOSApp * pWebOSapp = webosAppByAppId(appId);
	if (!pWebOSapp)
	{
		qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- No app entry yet";

		//there is no app entry for this app yet. create one...though, I really should:
		// TODO: make sure this launchpoint is a default launch point. There shouldn't be an add otherwise

		if (reasons.testBit(LaunchPointAddedReason::InstallerStatusUpdate))
		{
			//--> this is an app install that has just initially added the icon
			//	I must create the pending app (newPendingWebOSApp() ), and then bail out of here, since the rest of this code
			//	creates ADDITIONAL icons for apps that already exist
			qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- LaunchPointAddedReason::InstallerStatusUpdate";
			if ( (pWebOSapp = newPendingWebOSApp(pAppdescriptor)) == NULL)
			{
				qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- failed to create a new app object";
			}
			else
			{
				//TODO: signal up the chain to let the launcher know to add a new icon
				qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- created new pending webOS app object...signaling upward...";
				Q_EMIT signalNewApp(*pWebOSapp);
			}
		}
		else if (reasons.testBit(LaunchPointAddedReason::PostLaunchPointUpdateAdded))
		{
			qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- LaunchPointAddedReason::PostLaunchPointUpdateAdded";
			if ( (pWebOSapp = newWebOSApp(pAppdescriptor)) == NULL)
			{
				qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- failed to create a new app object";
			}
			else
			{
				//TODO: signal up the chain to let the launcher know to add a new icon
				qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- created new webOS app object...signaling upward...";
				Q_EMIT signalNewApp(*pWebOSapp);
			}
		}
		
		return;
	}

	qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- App entry already exists";

	//TODO: likewise, make sure here it ISN'T the default launch point (or I would end up overwriting the one already
	//		in place as the default and losing it)

	//the app exists already, and this is NOT the main launch point for it; it's an additional launch point

	//create an icon for it
	IconBase * pIcon = createAppIcon(StringTranslator::inputString(p_launchpoint->iconPath()),
										StringTranslator::inputString(p_launchpoint->title()));
	if (!pIcon)
	{
		//couldn't create the icon...get out
		qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- could not create icon";
		return;
	}

	//(add icon to the heap)
	IconHeap::iconHeap()->addIcon(pIcon,pWebOSapp->m_appId,launchpointId);

	//and the app's launch point map
	pWebOSapp->m_launchPointsMapById.insert(launchpointId,pIcon);

	//(since this isn't a new app/main launch point add, the app<->icon tracking maps already have the needed entries)
	qDebug() << __FUNCTION__ << ": appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- SUCCESS";
	Q_EMIT signalNewAdditionalLaunchPointForApp(pWebOSapp->uid(),pIcon->uid());
	return;
}

//virtual
void AppMonitor::slotLaunchPointUpdated(const LaunchPoint* p_launchpoint,QBitArray reasons)
{
	if (!p_launchpoint)
	{
		return;
	}

	//get the appid and launchpoint id
	QString appId = StringTranslator::inputString(p_launchpoint->id());
	QString launchpointId = StringTranslator::inputString(p_launchpoint->launchPointId());

	qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- " << ApplicationManager::dbgOutputLaunchpointUpdateReasons(reasons);

	//grab the application descriptor
	const ApplicationDescription * pAppdescriptor = p_launchpoint->appDesc();
	if (!pAppdescriptor)
	{
		//TODO: is this correct? could there be a launch point that as no associated app desc?
		qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- No app descriptor!?";
		return;
	}

	//check to see if this icon already exists
	IconBase * pIcon = IconHeap::iconHeap()->getIcon(appId,launchpointId);
	if (!pIcon)
	{
		//this icon doesn't exist, so it can't be "updated". It should have been added first
		qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] called but this launchpoint was never added. Ignoring";
		return;
	}

	//get the master copy, so that all the copies update
	pIcon = pIcon->master();

	//figure out what to update...it can be multiple things at the same time, that's fine.

	PixmapObject * pOldPmo = 0;
	PixmapObject * pNewPmo = 0;
	bool installUpdatedIcon = false;
	////			---- STATUS UPDATE ----
	if (reasons.testBit((int)(LaunchPointUpdatedReason::Status)))
	{
		switch (pAppdescriptor->status())
		{
		case ApplicationDescription::Status_Failed:
			qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- Status: Failed";
			// Load the warning icon decorator
			pNewPmo = IconHeap::iconHeap()->commonImageWarningDecorator();
			pIcon->slotUpdateInstallStatusDecoratorResetProgress(0,100);
			pIcon->slotUpdateInstallStatusDecoratorPic(pNewPmo,pOldPmo);
			//TODO: PMO-MANAGE: if there was a old install status decorator pmo...
			break;
		case ApplicationDescription::Status_Ready:
		{
			qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] , visibility:[" << pAppdescriptor->isVisible() << "]-- Status: Ready";

			//remove the installing status decorator (if there wasn't one, no big deal)
			pIcon->slotUpdateInstallStatusDecoratorPic(0,pOldPmo);

			// If the app was marked to be invisible, remove the icon/label from the launcher.
			if(!pAppdescriptor->isVisible()) {
				// Get the app corresponding to this launchpoint.
				WebOSApp* app =  this->pointerToWebOSApp(appId);

				if(app) {
					// make the icon invisible
					pIcon->slotChangeIconVisibility(false);
					// make the icon label invisible
					pIcon->setIconLabelVisibility(false);
					// remove from the map
					remove(find(app->uid()));
				}

				return;
			}

			//TODO: PMO-MANAGE: if there was a old install status decorator pmo...
			QString newIconFilename = StringTranslator::inputString(p_launchpoint->iconPath());
			pNewPmo = PixmapObjectLoader::instance()->quickLoad(newIconFilename);
			if (pNewPmo)
			{
				qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- attempting to set new icon (loaded from: " << newIconFilename << ")";
				pIcon->slotUpdateIconPic(pNewPmo,true,pOldPmo);
				//TODO: PMO-MANAGE: dispose of the old icon pixmap
			}
			else
			{
				qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- failed to load new icon (from: " << newIconFilename << ")";
			}
			installUpdatedIcon = true;

			if (!pIcon->connectRequestsToLauncher())
			{
				qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- failed to connect to LauncherObject's request slot";
			}

			//locate the app that this icon belongs to, and clear the status bits so that the rest of the system knows it is now usable
			WebOSApp * pWoApp = this->pointerToWebOSApp(appId);
			if (pWoApp)
			{
				pWoApp->setReady();
			}
		}
			break;
		case ApplicationDescription::Status_Installing:
			qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- Status: Installing";
		case ApplicationDescription::Status_Updating:
			qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- Status: (Updating)";
			if (!pIcon->usingInstallDecorator())
			{
				//Initial install/update.
				//not using the install decorator yet, so load and use it
				//regarding pOldPmo - shouldn't be one in this case so there won't be a need to dispose
				qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- initializing the install status decorator";
				pNewPmo = IconHeap::iconHeap()->commonImageProgressFilmstrip();
				pIcon->slotUpdateInstallStatusDecoratorResetProgress(0,100);
				pIcon->slotUpdateInstallStatusDecoratorPic(pNewPmo,pOldPmo);
			}
			//else nothing...the Progress update will handle things already being installed/updated
			break;
		}
	}

	////			---- PROGRESS UPDATE ----
	if (reasons.testBit((int)(LaunchPointUpdatedReason::Progress)))
	{
		qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- Reason: Progress";
		//if the icon isn't actually installing, this will do nothing
		pIcon->slotUpdateInstallStatusDecoratorPicNewProgress(pAppdescriptor->progress());
	}

	////			---- MAIN ICON UPDATE ----
	if (reasons.testBit((int)(LaunchPointUpdatedReason::Icon)))
	{
		qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- Reason: Icon";
		if (!installUpdatedIcon)
		{
			//The install status did not update the icon so I am ok to do it here
			//load the new pixmap
			QString newIconFilename = StringTranslator::inputString(p_launchpoint->iconPath());
			PixmapObject * pNewIconPixmap = PixmapObjectLoader::instance()->quickLoad(newIconFilename);
			if (pNewIconPixmap)
			{
				pIcon->slotUpdateIconPic(pNewPmo,true,pOldPmo);
				//TODO: PMO-MANAGE: dispose of the old icon pixmap
			}
		}
		else
		{
			qDebug() << __FUNCTION__ << ": Update on appId:[" << appId << "] , launchpointId:[" << launchpointId << "] -- the 'Status' handler already updated the icon, so this is safe to ignore here";
		}
	}

	//redraw it
	pIcon->update();
}

//virtual
void AppMonitor::slotLaunchPointRemoved(const LaunchPoint* p_launchpoint,QBitArray reasons)
{
	if (!p_launchpoint)
	{
		return;
	}

	//get the appid and launchpoint id
	QString appId = StringTranslator::inputString(p_launchpoint->id());
	QString launchpointId = StringTranslator::inputString(p_launchpoint->launchPointId());

	g_warning("%s: Remove on appId:[%s] , launchpointId:[%s]",
			__FUNCTION__,qPrintable(appId),qPrintable(launchpointId));

	WebOSApp * pWebOSapp = pointerToWebOSApp(appId);
	if (!pWebOSapp)
	{
		g_warning("%s: appId:[%s] , launchpointId:[%s] -- No such app known to launcher",
				__FUNCTION__,qPrintable(appId),qPrintable(launchpointId));
		return;
	}

	//remove the launchpoint, and all of its copies. If it happens to be the default launchpoint of the app
	//	then remove the app entry as well
	if (pWebOSapp->m_mainLaunchPointId == launchpointId)
	{
		//the main launch point is being removed. This is the Whole Smash!
		// call this->slotAppBeingRemoved() , as this will take care of killing of all the icons
		slotAppBeingRemoved(pWebOSapp);
	}
	else
	{
		//tell the launcher to kill of auxiliary icons.
		Q_EMIT signalAppAuxiliaryIconRemove(pWebOSapp->uid(),launchpointId);
	}
}

//////protected:

//virtual
bool AppMonitor::rescanWebOSApp(WebOSApp& webOSApp)
{
	//TODO: IMPLEMENT
	return false;
}

//virtual
WebOSApp * AppMonitor::pointerToWebOSApp(const QString& appId) const
{
	WebOSAppMapConstIter it = m_aliasWebosAppMapByAppId.constFind(appId);
	if (it != m_aliasWebosAppMapByAppId.constEnd())
	{
		return it.value();
	}
	return 0;
}

//virtual
ExternalApp * AppMonitor::find(const QUuid& uid) const
{
	AppMapConstIter it = m_appMapByUid.constFind(uid);
	if (it != m_appMapByUid.constEnd())
	{
		return it.value();
	}
	return 0;
}

//virtual
void AppMonitor::remove(ExternalApp * p_eapp)
{

	//TODO: any other cleanup of the structures inside the maps???
	if (!p_eapp)
	{
		return;
	}
	m_appMapByUid.remove(p_eapp->uid());
	WebOSApp * pWoApp = qobject_cast<WebOSApp *>(p_eapp);
	if (pWoApp)
	{
		m_aliasWebosAppMapByAppId.remove(pWoApp->appId());
	}
}

} //end namespace
