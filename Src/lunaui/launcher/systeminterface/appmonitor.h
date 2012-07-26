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




#ifndef APPMONITOR_H_
#define APPMONITOR_H_

#include "externalapp.h"
#include "webosapp.h"
#include "blacklist.h"

#include <QObject>
#include <QPointer>
#include <QList>
#include <QUuid>
#include <QBitArray>

class ApplicationDescription;
class LaunchPoint;

namespace DimensionsSystemInterface
{

namespace AppMonitorSignalType
{
	enum Enum
	{
		INVALID,				//or not applicable
		InitialScan,
		AppInstallerSourced,
		AppManSourced
	};
}

class AppMonitor : public QObject
{
	Q_OBJECT

public:

	static AppMonitor * appMonitor();
	AppMonitor();
	virtual ~AppMonitor();

	bool completedScan() const;
	QList<QUuid> allAppUids() const;
	QList<QPointer<ExternalApp> > allApps() const;
	ExternalApp * appByUid(const QUuid& appUid) const;
	WebOSApp * webosAppByAppId(const QString& appId) const;

	bool			isAppRemovable(const QUuid& appUid);

	static QString webosAppLaunchPointTitle(const QString& appId,const QString& launchPointId);
	static QString webosAppTitle(const QString& appId);
	static QString webosAppVersion(const QString& appId);
	static QString webosAppCategory(const QString& appId);
	static bool	   webosAppNonRemovableSystemApp(const QString& appId);
	static bool	   webosAppPlatformApp(const QString& appId);
	static bool    webosAppUserInstalledApp(const QString& appId);
	static bool	   webosAppRemovableOrHideable(const QString& appId);

	QString pageDesignatorForWebOSApp(const QString& appId);
	QString pageNameFromDesignator(const QString& designator);
	QList<QString> auxPageDesignators() const;

	static QString appIdFromLaunchpointId(const QString& launchpointId);

public Q_SLOTS:

	// These are the primary interface to AppMan,AppInstaller... which are modified to emit signals compatible with these slots.
	// const'd the parameter in order to isolate the sender (AppMan for the most part); e.g. if AppMan wants to index the appdescriptor in any
	//	way by keying to some appdescr. variable, then it would have to know that I won't mess with any of the variables in here (or anywhere the signal that
	// sends the appdescr. ptr is sent)

	virtual void slotInitialScanStart();
	virtual void slotInitialScanEnd();

	virtual void slotScanFoundApp(const ApplicationDescription * pAppdescriptor);
	virtual void slotScanFoundAuxiliaryLaunchPoint(const ApplicationDescription * p_appDesc,
													const LaunchPoint * p_launchPoint);

	virtual void slotAppBeingRemoved(const ApplicationDescription * pAppdescriptor);
	virtual void slotAppBeingRemoved(WebOSApp * p_webOSapp);

	virtual void slotAppUpdated(const ApplicationDescription * pAppdescriptor);

	//these are for ApplicationManager; since they're more or less stricly for AppMan, they don't need to be slots and I could just have AppMan call here directly
	//	but this will be more convenient, ESPECIALLY if something changes with AppMan or the rest of the sys outside
	virtual void slotLaunchPointAdded(const LaunchPoint*,QBitArray reasons);
	virtual void slotLaunchPointUpdated(const LaunchPoint*,QBitArray reasons);
	virtual void slotLaunchPointRemoved(const LaunchPoint*,QBitArray reasons);

Q_SIGNALS:

	//USE THESE SIGNALS ONLY INTERNALLY TO DIMENSIONS! THESE ARE RE-SIGNALS OF EXTERNAL EVENTS BUT ARE NOT INTENDED TO BE
	// USED 'BACKWARDS'/UPSTREAM (I.E. BACK INTO THE SYSTEM). USING THESE TO SYNCHRONIZE OR TRIGGER EXTERNAL EVENTS IS STRONGLY
	// DISCOURAGED! YOU'VE BEEN WARNED.

	//signalFullScanCompleted called after each full scan completes (AppMan)
	void signalFullScanCompleted(bool initialScan=false);

	//signalNewApp is called *after* the ExternalApp object has been created and in the proper list(s)
	void signalNewApp(const DimensionsSystemInterface::ExternalApp& eapp,DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID);
	// note: signalNewAdditionalLaunchPointForApp will not get signaled for the main/default launch point. That is assumed in signalNewApp
	void signalNewAdditionalLaunchPointForApp(const QUuid& appUid,const QUuid& newLaunchPointIconUid,DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID);

	//signalRemovedApp is called *before* the ExternalApp object is removed and discarded from the proper list(s)
	void signalRemovedApp(const DimensionsSystemInterface::ExternalApp& eapp,DimensionsSystemInterface::AppMonitorSignalType::Enum origin = AppMonitorSignalType::INVALID);
	// and this one is *after*...
	void signalRemovedAppComplete(const QUuid& removedAppUid,DimensionsSystemInterface::AppMonitorSignalType::Enum origin= DimensionsSystemInterface::AppMonitorSignalType::INVALID);

	void signalAppAuxiliaryIconRemove(const QUuid& appUid,const QString& launchpointId,
			DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID);

	void signalUpdatedApp(const DimensionsSystemInterface::ExternalApp& eapp,DimensionsSystemInterface::AppMonitorSignalType::Enum origin = DimensionsSystemInterface::AppMonitorSignalType::INVALID);

protected:

	// return false if app no longer exists (rescan failed)
	virtual bool rescanWebOSApp(WebOSApp& webOSApp);
	virtual WebOSApp * pointerToWebOSApp(const QString& appId) const;	//TODO: NAMING
	virtual ExternalApp * find(const QUuid& uid) const;

	virtual WebOSApp * newPendingWebOSApp(const ApplicationDescription * pAppdescriptor);
	virtual WebOSApp * newWebOSApp(const ApplicationDescription * pAppdescriptor);

	virtual IconBase * createAppIcon(const QString& mainIconFile,const QString& label);

	virtual void remove(ExternalApp * p_eapp);

	virtual void loadDesignatorKeywordMapping(const QString& mapFile);

protected:

	static QPointer<AppMonitor> s_qp_mainInstance;

	bool	m_withinInitialScan;
	quint32 m_fullScanCounter;

	typedef QMap<QUuid,QPointer<ExternalApp> > AppMap;
	typedef AppMap::iterator AppMapIter;
	typedef AppMap::const_iterator AppMapConstIter;
	AppMap m_appMapByUid;

	typedef QMap<QString,QPointer<WebOSApp> > WebOSAppMap;
	typedef WebOSAppMap::iterator WebOSAppMapIter;
	typedef WebOSAppMap::const_iterator WebOSAppMapConstIter;
	WebOSAppMap m_aliasWebosAppMapByAppId;

	Blacklist m_appBlacklist;
	QMap<QString,QString>	m_pageDesignatorByKeyword;
	QList<QString> m_auxDesignatorList;		//everything besides "favorites" (or actually, the name in Page::PageDesignator_Favorites)
	QMap<QString,QString> m_designatorToNameMap;
};

}

#endif /* APPMONITOR_H_ */
