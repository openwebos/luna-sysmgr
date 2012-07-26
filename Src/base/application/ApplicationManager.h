/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef __ApplicationManager_h__
#define __ApplicationManager_h__

#include "Common.h"

#include <vector>
#include <string>
#include <list>
#include <map>
#include <set>

#include "lunaservice.h"
#include "Mutex.h"
#include "MimeSystem.h"

#include <QObject>
#include <QBitArray>

class ApplicationDescription;
class PackageDescription;
class ServiceDescription;
class LaunchPoint;
class CommandHandler;
class ResourceHandler;
class RedirectHandler;

//LAUNCHER3-ADDED:
namespace LaunchPointUpdatedReason
{
	enum Enum
	{
		INVALID = 0,
		Status,
		Progress,
		Icon,
		SIZEOF
	};
}

namespace LaunchPointAddedReason
{
	enum Enum
	{
		INVALID = 0,
		InstallerStatusUpdate,
		PostLaunchPointUpdateAdded,		// temp flag - see AppMonitor::slotLaunchPointAdded
		SIZEOF
	};
}

namespace LaunchPointRemovedReason
{
	enum Enum
	{
		INVALID = 0,
		InstallerStatusUpdate,
		SIZEOF
	};
}

// sort items in the search set by title
struct cmptitle {
	bool operator()(const LaunchPoint*,const LaunchPoint*) const;
};
typedef std::set<const LaunchPoint*,cmptitle> SearchSet;

namespace DimensionsSystemInterface
{
class AppEffector;
}

class ApplicationManager : public QObject
{
	Q_OBJECT

public:

	//LAUNCHER3-ADDED:  TEMP: need a better way to send LS messages but for now, just letting AppEffector in
	//					launcher have access to the service handle in here (with the proper precautions)
	friend class DimensionsSystemInterface::AppEffector;

	typedef std::vector<const LaunchPoint *> LaunchPointCollection;

	~ApplicationManager();

	static ApplicationManager* instance();

	bool init();
	void scan();
	void postInstallScan(const std::string& appId);
	void postInstallScan(json_object * pPackageInfoJson, const std::string& packageFolder);

	void launchBootTimeApps();
	bool isLaunchAtBootApp(const std::string& appId);

	ApplicationDescription* getAppById(const std::string& id);
	ApplicationDescription* getAppByIdHardwareCompatibleAppsOnly(const std::string& id);
	ApplicationDescription* getAppByMime(const std::string& mime);
	ApplicationDescription* getPendingAppById(const std::string& id);

	PackageDescription* getPackageInfoByAppId(const std::string& anyAppIdInPackage);
	PackageDescription* getPackageInfoByServiceId(const std::string& anyServiceIdInPackage);
	PackageDescription* getPackageInfoByPackageId(const std::string& packageId);

	ServiceDescription* getServiceInfoByServiceId(const std::string& serviceId);

	bool getAppsByPackageId(const std::string& packageId, std::vector<ApplicationDescription *>& r_apps);
	bool getServicesByPackageId(const std::string& packageId, std::vector<ServiceDescription *>& r_services);

	void searchLaunchPoints(SearchSet& matchedByTitle, SearchSet& matchedByKeyword, const std::string& searchTerm) const;

	bool 					removePendingApp(const std::string& id);
	bool                    removePackage(const std::string& id,int cause);
	const LaunchPoint*      getLaunchPointById(const std::string& launchPointId);
	const LaunchPoint*      getLaunchPointByIdHardwareCompatibleAppsOnly(const std::string& launchPointId);

	std::string addLaunchPoint(const std::string& id,
							   const std::string& title,
							   const std::string& menuName,
							   const std::string& icon,
							   const std::string& params,
							   const bool removable=true);
	bool        removeLaunchPoint(const std::string& launchPointId,std::string& extendedReturnCause);
	// NOTE: this will need to be extended in the future to support other field updates
	bool		updateLaunchPointIcon(const std::string& launchPointId, const std::string& newIconPath);

	bool enableDockModeLaunchPoint (const char* appId);
	bool disableDockModeLaunchPoint (const char* appId);

	LaunchPointCollection dockLaunchPoints(bool *needsUpdate = NULL);
	LaunchPointCollection allLaunchPoints();
	LaunchPointCollection allPendingLaunchPoints();
	LaunchPointCollection allDockModeLaunchPoints();

	const std::set<const LaunchPoint*>& enabledDockModeLaunchPoints() { return m_dockModeLaunchPoints; }

	std::vector<ApplicationDescription*> allApps();
	std::map<std::string, PackageDescription*> allPackages();


	std::string				mimeTableAsJsonString();

	void relayStatus(const std::string& jsonPayload,const unsigned long ticketId);

	static ApplicationDescription* checkAppAgainstWhiteList( ApplicationDescription* appDesc );
	bool isTrustedInstallerApp (const std::string& uri) const;
	bool isTrustedPalmApp(const ApplicationDescription* appDesc) const;
	bool isTrustedPalmApp(const std::string& appId);

	//LAUNCHER3-ADDED:  this is intended to be a one-stop shopping point to tell the launcher if this is an app which
	//		ships with the platform (either HPalm's own app or a "2nd party" contractor/partner app which should be treated the same way)
	//		For now it will just be a passthrough to isTrustedPalm().
	bool isFactoryPlatformApp(const std::string& appId);

	struct DownloadRequest {
		unsigned long m_ticket;
		std::string m_overrideHandlerAppId;
		std::string m_mime;
		bool m_isSubscribed;

		DownloadRequest (unsigned long ticket, const std::string& ovrHandlerAppId, const std::string& strMime, bool isSubscribed)
		: m_ticket(ticket), m_overrideHandlerAppId (ovrHandlerAppId), m_mime (strMime), m_isSubscribed (isSubscribed)
		{  }

		DownloadRequest (unsigned long ticket, bool isSubscribed)
		: m_ticket (ticket), m_isSubscribed (isSubscribed)
		{  }
	};

	static unsigned long s_ticketGenerator;

	static unsigned long generateNewTicket();

	static bool cbDownloadManagerUpdate (LSHandle* lsHandle, LSMessage* msg, void* user_data);
	static bool isRemoteFile (const char* uri);
	static bool isAppPackage(const char* uri);

	static bool getAppEntryPointFromAppinfoFile(const std::string& baseDirOfApp,std::string& r_entryPointPath);

	static bool cbAppInstallServiceConnection (LSHandle* lshandle, LSMessage* msg, void* user_data);
	static bool cbApplicationStatusCallback (LSHandle* lsHandle, LSMessage* msg, void* user_data);
	void handleApplicationStatusUpdates(LSMessage* msg);

	enum ExecuteLockOperation {
		ExecuteUnlock,
		ExecuteLock,
		ExecuteLockAndTerminate
	};
	static void executeLockApp(const std::string& appId,ExecuteLockOperation op);

	void dbgEmitSignalLaunchPointUpdated(const LaunchPoint * lp,const QBitArray& statusBits);
	static QString dbgOutputLaunchpointUpdateReasons(const QBitArray& reasons);

Q_SIGNALS:

	//LAUNCHER3-ADD: (modified)
	void signalLaunchPointRemoved(const LaunchPoint*,QBitArray reasons = QBitArray());
	void signalLaunchPointUpdated(const LaunchPoint*,QBitArray reasons = QBitArray());
	void signalLaunchPointAdded(const LaunchPoint*,QBitArray reasons = QBitArray());

	//LAUNCHER3-ADD:
	void signalInitialScanStart();
	void signalInitialScanEnd();
	void signalScanFoundApp(const ApplicationDescription *);
	void signalScanFoundAuxiliaryLaunchPoint(const ApplicationDescription *,const LaunchPoint *);

	//--end

	void signalDockModeLaunchPointEnabled (const LaunchPoint*);
	void signalDockModeLaunchPointDisabled (const LaunchPoint*);

public Q_SLOTS:

	void slotBuiltInAppEntryPoint_DockMode(const std::string& argsAsStringEncodedJson);
	void slotBuiltInAppEntryPoint_VoiceDial(const std::string& argsAsStringEncodedJson);
	void slotVoiceDialAllowSettingChanged(bool v);

	void slotBuiltInAppEntryPoint_Launchermode0(const std::string& argsAsStringEncodedJson);

private:

	void scanForApplications();
	void scanForPackages();
	void createPackageDescriptionForOldApps();
	void scanForServices();
	void scanForSystemApplications();
	void scanForPendingApplications();
	void scanForLaunchPoints(std::string launchPointFolder);
	void scanApplicationsFolders(const std::string& appFolders);
	void scanApplicationsFolders(const std::string& appFoldersPath,std::map<std::string,ApplicationDescription *>& foundApps);
	ApplicationDescription* scanOneApplicationFolder(const std::string& appFolderPath);
	PackageDescription* scanOnePackageFolder(const std::string& packageFolderPath);
	ServiceDescription* scanOneServiceFolder(const std::string& serviceFolderPath);

	ApplicationDescription* installApp(const std::string& appId);
	ApplicationDescription* installSysApp(const std::string& appId);
	bool                    removeApp(const std::string& id,int cause);
	bool					removeSysApp(const std::string& id);

	//discoverAppChanges: temporal non-safety; apps may change state after the lists are generated. Call under proper locks
	void discoverAppChanges(std::vector<ApplicationDescription *>& added,std::vector<ApplicationDescription *>& removed,std::vector<ApplicationDescription *>& changed);

	ApplicationDescription* getAppById( const std::string& appId,const std::map<std::string,ApplicationDescription *>& appMap);

	void scanFolderResursively( const std::string& path );
	void clear();
	void dumpStats();
	std::string findUniqueFileName(const std::string& folder);
	bool isNumber(const std::string& str) const;
	static bool isValidMimeType( const std::string& mime );
	static bool isGenericMimeType( const std::string& mime );
	static std::string getContentType(const std::string& mime);
	static bool urlSchemeIsFile( const std::string& url );
	static std::string deriveMimeTypeFromFileMagic( const std::string& localFilePath );

	static void createOrUpdatePackageManifest(PackageDescription* packageDesc);

	static void serviceInstallerInstallApp(const std::string& id, const std::string& type, const std::string& root);
	static void serviceInstallerUninstallApp(const std::string& id, const std::string& type, const std::string& root);

	void runAppInstallScripts();
	void loadHiddenApps();
	void hideApp(const std::string& appId);
	bool isAppHidden(const std::string& appId) const;
	bool isSysappAllowed(const std::string& sysappId,const std::string& sourcePath);

	void executeLockAppLoaded(const std::string& appId,ExecuteLockOperation op);
	static std::set<std::string> s_appExeclockSet;
	static Mutex s_mutexExecLockFunctions;

	std::set<std::string> m_hiddenApps;
	std::vector<ApplicationDescription*> m_registeredApps;
	std::vector<ApplicationDescription*> m_systemApps;
	std::vector<ApplicationDescription*> m_pendingApps;

	std::set<const LaunchPoint*> m_dockModeLaunchPoints;

	std::map<std::string, PackageDescription*> m_registeredPackages;
	std::map<std::string, ServiceDescription*> m_registeredServices;

	Mutex m_mutex;

	bool	startService();
	void	stopService();
	void    postLaunchPointChange(const LaunchPoint* lp, const std::string& change);
	LSPalmService*	m_service;
	LSHandle* m_serviceHandlePublic;
	LSHandle* m_serviceHandlePrivate;
	static long s_ticketId;
	bool m_initialScan;

	ApplicationManager( );
	ApplicationManager& operator=( const ApplicationManager& );
	ApplicationManager( const ApplicationManager& );
};

// Calback for the ListRunningApps response generated by the AppManager service
void appManagerCallback_listRunningApps( const std::string& runnigAppsJsonArray );

#endif // __ApplicationManager_h__
