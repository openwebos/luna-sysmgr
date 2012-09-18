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




#include "Common.h"

#include <glib.h>

#include "ApplicationManager.h"
//MDK-LAUNCHER #include "DockPositionManager.h"
#include "ApplicationDescription.h"
#include "ApplicationStatus.h"
#include "PackageDescription.h"
#include "ServiceDescription.h"
#include "DeviceInfo.h"
#include "LaunchPoint.h"
#include "MutexLocker.h"
#include "Preferences.h"
#include "WebAppMgrProxy.h"
#include "Settings.h"
#include "SystemService.h"
#include "HostBase.h"
#include "Utils.h"
#include "WindowServer.h"
#include "ApplicationInstaller.h"
#include "EventReporter.h"
#include "SystemUiController.h"
#include "dimensionsmain.h"
#include "WindowServerLuna.h"

#if !(defined(TARGET_DESKTOP) || defined(TARGET_EMULATOR))
// TODO:  Reactivate ServiceInstaller
#include <serviceinstall.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <algorithm>
#include <vector>
#include <string>

#include <QUrl>
#include <openssl/blowfish.h>

#include <QBitArray>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
// This can be defined (or better yet removed) when magic number support has
// been added to Nova.
#ifdef USE_LIBMAGIC
#include <magic.h>
#endif
#include <sys/time.h>

#include <cjson/json.h>

#include "MimeSystem.h"

#include "Logging.h"

#include "OverlayWindowManager.h"

//LAUNCHER3-ADD:
#include "appmonitor.h"
#include <QProcess>
//--end

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

static const char* sAppMgrChnl = "ApplicationManager";

static const char* sServiceInstallerTypeService = "services";
static const char* sServiceInstallerTypeApplication = "applications";

static ApplicationManager* s_instance = 0;

std::set<std::string> ApplicationManager::s_appExeclockSet;
Mutex ApplicationManager::s_mutexExecLockFunctions;

static std::string rot13( const char* s );
static bool hardwareFeaturesRequirementSatisfied(uint32_t hardwareFeaturesNeeded);

unsigned long ApplicationManager::s_ticketGenerator = 1;

ApplicationManager* ApplicationManager::instance()
{
	MutexLocker m(&s_mutexExecLockFunctions);		//WARNING: currently the s_instance variable and the exec-lock stuff are the only things that need sync-ing, but
														// if more things need sync in the future, a more encompassing s_ mutex should be created
	if( s_instance )
		return s_instance;
	s_instance = new ApplicationManager();

	//LAUNCHER3-ADD:
	// do NOT reposition this line anywhere!
	DimensionsSystemInterface::AppMonitor::appMonitor();	//this is enough to kick it off so that it catches signals from the
															// beginning of execution (scans, etc) of application manager
	//--end
	return s_instance;
}


ApplicationManager::ApplicationManager()
{
	m_service = 0;
	m_serviceHandlePublic = 0;
	m_serviceHandlePrivate = 0;
	m_initialScan = true;

	////hmmm, maybe better to load these in init()? need to consider race based on request-before-init...
	if (doesExistOnFilesystem(Settings::LunaSettings()->lunaCmdHandlerSavedPath.c_str()))
		MimeSystem::instance(Settings::LunaSettings()->lunaCmdHandlerSavedPath);
	else
		MimeSystem::instance(Settings::LunaSettings()->lunaCmdHandlerPath);

	startService();
}

ApplicationManager::~ApplicationManager()
{
	clear();
	stopService();
	s_instance = 0;
}


void ApplicationManager::clear()
{
	MutexLocker locker(&m_mutex);

	for (unsigned int i = 0; i < m_registeredApps.size(); ++i) {
		delete m_registeredApps[i];
	}

	m_registeredApps.clear();
	m_initialScan = true;

	for (unsigned int i=0; i < m_systemApps.size(); ++i) {
		delete m_systemApps[i];
	}

	m_systemApps.clear();
}

static const char* s_hiddenAppsPath = "/var/luna/data/.hidden-apps.json";

bool ApplicationManager::init(  )
{
	if (Settings::LunaSettings()->uiType == Settings::UI_MINIMAL) {

		static const char* s_cryptoFsPath = "/media/internal/.palm";

		// Wipe apps from /media/internal/.palm as NDI doesn't do it
		g_warning("Wiping contents of /media/internal/.palm");
		std::string cmd = "rm -rf ";
		cmd += s_cryptoFsPath;
		cmd += "/*";
		g_warning("Executing command: '%s'", cmd.c_str());
		int ret = ::system(cmd.c_str());
		g_warning("Result: %d", ret);
	}

	runAppInstallScripts();

	loadHiddenApps();

	// scan for applications.
	m_initialScan = true;
	Q_EMIT signalInitialScanStart();
	scan();
	Q_EMIT signalInitialScanEnd();

	connect(Preferences::instance(),SIGNAL(signalVoiceDialSettingChanged(bool)),this,SLOT(slotVoiceDialAllowSettingChanged(bool)));
	return true;
}

void ApplicationManager::runAppInstallScripts()
{
	std::string cmd;
	int ret;

	cmd = "mountcfs";
	ret = ::system(cmd.c_str());

	if (ret == 0) {
		g_warning("Running app install script");
		cmd = "/usr/bin/app-install -install-only";
		ret = ::system(cmd.c_str());

		if (ret == 0 && Settings::LunaSettings()->uiType != Settings::UI_MINIMAL) {
			/* Use g_spawn_async instead of system().
			 * g_spawn_async will correctly close the inherited
			 * file descriptors from the parent */
			GError *gerr = NULL;
			const char *argv[4] = {0};
			argv[0] = "/usr/bin/nohup";
			argv[1] = "/usr/bin/app-install";
			argv[2] = "-notify-only";
			argv[3] = NULL;
			gboolean spawnRet = g_spawn_async(NULL,
							(gchar**)argv,
							NULL,
							(GSpawnFlags)0,
							NULL,
							NULL,
							NULL,
							&gerr);

			if (!spawnRet) {
				g_warning("%s: Failed to spawn app-install: (%d) %s", __func__,
						gerr->code,
						gerr->message);
				g_error_free(gerr);
			}
		}
		else if (ret != 0) {
			g_warning("Failed in app install script install only. Not running notify step");
		}
		else {
			g_warning("In first use mode. Not running notify step");
		}
	}
	else {
		g_warning("cryptofs could not be mounted. Not running app-install script");
	}

}

void ApplicationManager::loadHiddenApps()
{
	json_object* root = json_object_from_file(const_cast<char*>(s_hiddenAppsPath));
	if (!root || is_error(root))
		return;

	if (json_object_is_type(root, json_type_array)) {
		int numApps = json_object_array_length(root);
		for (int i=0; i<numApps; i++) {
			json_object* app = json_object_array_get_idx(root, i);
			if (app && json_object_is_type(app, json_type_string)) {
				const char* appId = json_object_get_string(app);
				if (appId) {
					m_hiddenApps.insert(std::string(appId));
				}
			}
		}
	}

	json_object_put(root);
}

bool ApplicationManager::isAppHidden(const std::string& appId) const
{
	return m_hiddenApps.find(appId) != m_hiddenApps.end();
}

void ApplicationManager::hideApp(const std::string& appId)
{
	if (m_hiddenApps.find(appId) != m_hiddenApps.end())
		return;		//already hidden

	m_hiddenApps.insert(appId);

	json_object* apps = json_object_new_array();
	if (apps == NULL)
		return;

	std::set<std::string>::const_iterator it, itEnd;
	for (it = m_hiddenApps.begin(), itEnd = m_hiddenApps.end(); it != itEnd; ++it) {
		json_object_array_add(apps, json_object_new_string((*it).c_str()));
	}
	json_object_to_file(const_cast<char*>(s_hiddenAppsPath), apps);
	json_object_put(apps);

	MimeSystem::instance()->removeAllForAppId(appId);
}

bool ApplicationManager::isSysappAllowed(const std::string& sysappId,const std::string& sourcePath)
{

#if !defined(TARGET_DESKTOP)

	//check to see if this is a system folder: rooted at /usr		TODO: make this better
	bool isSystemFolder;
	if (sourcePath.find("/usr") == 0)
		isSystemFolder = true;
	else
		isSystemFolder = false;

	if (!isSystemFolder)
	{
		//SECURITY: NOT ALLOWED! sysmgr builtins can only be in system folders
		g_warning("%s: [SECURITY]: not allowing id [%s] in [%s] because sys builtins are only allowed in system folders",
				__FUNCTION__,sysappId.c_str(),sourcePath.c_str());
		return false;
	}
#endif

	// -- PLACE OTHER RESTRICTION LOGIC HERE --

	if (sysappId == "com.palm.sysapp.voicedial")
	{
		//only if the enableVoiceCommand is set in the preferences
		return (Preferences::instance()->getVoiceDialEnabled());
	}
	return true;
}

#include <QDebug>
#include <QMetaObject>
#include <QMetaMethod>

void ApplicationManager::scan()
{
	MutexLocker locker(&m_mutex);

	// FIXME: Need to launch boot time apps

	if (m_initialScan) {
		m_initialScan=false;				//TODO: reset this if scans fail
		scanForSystemApplications();
		scanForApplications();
		scanForPackages();
		createPackageDescriptionForOldApps();
		scanForServices();
		scanForPendingApplications();

		scanForLaunchPoints(Settings::LunaSettings()->lunaPresetLaunchPointsPath);
		scanForLaunchPoints(Settings::LunaSettings()->lunaLaunchPointsPath);
		return;
	}

	//..at this point, it's a "rescan"....
	std::vector<ApplicationDescription *> removed;			//these pointers will point to things in m_registeredApps
	std::vector<ApplicationDescription *> added;			//these pointers will point to NEW ApplicationDescription objects
	std::vector<ApplicationDescription *> changed;			//these pointers will point to things in m_registeredApps

	ApplicationDescription * pAppDesc, *pRegAppDesc;
	ApplicationManager::instance()->discoverAppChanges(added,removed,changed);

	std::vector<ApplicationDescription *>::iterator it = added.begin();

	g_message("Apps added:");
	while (it !=  added.end()) {
		pAppDesc = *it;
		g_message("(A)\t%s",pAppDesc->id().c_str());
		EventReporter::instance()->report( "install", pAppDesc->id().c_str() );
		it++;
	}

	it = changed.begin();

	g_message("Apps changed/updated:");
	while (it !=  changed.end()) {
		pAppDesc = *it;
		g_message("(U)\t%s",pAppDesc->id().c_str());
		it++;
	}

	it = removed.begin();

	g_message("Apps (forcibly!) removed:");
	while (it !=  removed.end()) {
		pAppDesc = *it;
		g_message("(R)\t%s",pAppDesc->id().c_str());
		it++;
	}

	//for every application "changed", stop the application if it is running. The user will have to reload it...
	it = changed.begin();
	while (it !=  changed.end()) {
		pAppDesc = *it;						//pAppDesc points to a NEW ApplicationDescriptor
		pRegAppDesc = getAppById(pAppDesc->id());
		if (pRegAppDesc == NULL) {
			//YIKES! serious issues! something became out of sync...try to ignore
			it++;
			delete pAppDesc;
			continue;
		}
		pRegAppDesc->executionLock();
		WebAppMgrProxy::instance()->sendAsyncMessage(new View_Mgr_KillApp(pAppDesc->id()));

		//now update the app descriptor for this app
		pRegAppDesc->update(*pAppDesc);
		//and post a launchpoint update
		postLaunchPointChange(pRegAppDesc->getDefaultLaunchPoint(), "updated");
		// remove the update appdesc from our pending list
		removePendingApp(pRegAppDesc->id());
		pRegAppDesc->executionLock(false);

		it++;
		//get rid of the app descriptor
		delete pAppDesc;
	}

	//for every application "removed", stop the application if it is running. Flag it so that it can't be executed anymore, and for removal to avoid reiterating over 2 lists
	//	This whole subroutine is actually an error recovery case. Any applications listed in the "removed" list here were forcibly deleted from storage without using app removal procedures.
	it = removed.begin();
	while (it !=  removed.end()) {
		pAppDesc = *it;					//pAppDesc points to something in m_registeredApps
		pAppDesc->executionLock();
		pAppDesc->flagForRemoval();
		WebAppMgrProxy::instance()->sendAsyncMessage(new View_Mgr_KillApp(pAppDesc->id()));
		it++;
	}

	//...then remove it from the registered applications, and the launchpoints
	it = m_registeredApps.begin();
	LaunchPointList launchPoints;
	while (it !=  m_registeredApps.end()) {
		pAppDesc = *it;
		if (pAppDesc->isRemoveFlagged()) {
			it = m_registeredApps.erase(it);
			pAppDesc->launchPoints(launchPoints);
			for (LaunchPointList::iterator lpit = launchPoints.begin();lpit != launchPoints.end();lpit++) {
				const LaunchPoint *pLpoint = *lpit;
				if (pLpoint->isDefault()) {
					postLaunchPointChange(pLpoint, "removed");
					pAppDesc->removeLaunchPoint(pLpoint);
				}
				else {
					std::string erc;
					if (this->removeLaunchPoint(pLpoint->id(),erc) == false)
						g_message("ApplicationManager::scan(): can't remove launchpoint: %s\n",erc.c_str());
				}
			}

			//notify of app removal
			ApplicationInstaller::instance()->notifyAppRemoved(pAppDesc->id(),pAppDesc->version());
			delete pAppDesc;
		}
		else
			it++;
	}

	//for each application added, add the application description and advertise the default launchpoint

	it = added.begin();
	while (it !=  added.end()) {
		pAppDesc = *it;								//pAppDesc points to a NEW ApplicationDescriptor
		m_registeredApps.push_back(pAppDesc);
		const LaunchPoint *pLpoint = pAppDesc->getDefaultLaunchPoint();
		if (pLpoint) {
			postLaunchPointChange(pLpoint,"added");
		}
		// remove the pending install appdesc from our pending list
		removePendingApp(pAppDesc->id());
		//notify of app install
		ApplicationInstaller::instance()->notifyAppInstalled(pAppDesc->id(),pAppDesc->version());
		it++;
	}

	//force caches to clear
	WebAppMgrProxy::instance()->clearWebkitCache();

	//force a repaint

	WindowServer::instance()->update(0, 0,SystemUiController::instance()->currentUiWidth(),SystemUiController::instance()->currentUiHeight());
}

void ApplicationManager::postInstallScan(const std::string& appId) {
	if (appId.empty()) {
		g_message("appId in %s was empty. This should never happen!", __PRETTY_FUNCTION__);
		return;
	}
	ApplicationDescription* appDesc = installApp(appId);
	PackageDescription* packageDesc = PackageDescription::fromApplicationDescription(appDesc);
	if (packageDesc) {
		m_registeredPackages[packageDesc->id()] = packageDesc;
	}

	createOrUpdatePackageManifest(packageDesc);

	serviceInstallerInstallApp(appId, sServiceInstallerTypeApplication, Settings::LunaSettings()->appInstallBase);

	//force caches to clear
	WebAppMgrProxy::instance()->clearWebkitCache();

	//force a repaint
	WindowServer::instance()->update(0, 0, SystemUiController::instance()->currentUiWidth(),SystemUiController::instance()->currentUiHeight());
}

void ApplicationManager::postInstallScan(json_object * pPackageInfoJson, const std::string& packageFolder)
{
	if (!pPackageInfoJson)
		return;

	PackageDescription* packageDesc = PackageDescription::fromJson(pPackageInfoJson, packageFolder);
	if (!packageDesc)
		return;

	m_registeredPackages[packageDesc->id()] = packageDesc;

	std::vector<std::string>::const_iterator appIdIt, appIdItEnd;
	for (appIdIt = packageDesc->appIds().begin(), appIdItEnd = packageDesc->appIds().end(); appIdIt != appIdItEnd; ++appIdIt) {
		ApplicationDescription* appDesc = installApp(*appIdIt);
		if (appDesc) {
			if (packageDesc->accountIds().size() > 0) {
				appDesc->setHasAccounts(true);
			}
			serviceInstallerInstallApp(*appIdIt, sServiceInstallerTypeApplication, Settings::LunaSettings()->appInstallBase);
		}
	}

	std::vector<std::string>::const_iterator serviceIdIt, serviceIdItEnd;
	for (serviceIdIt = packageDesc->serviceIds().begin(), serviceIdItEnd = packageDesc->serviceIds().end(); serviceIdIt != serviceIdItEnd; ++serviceIdIt) {
		std::string servicePathFull = Settings::LunaSettings()->serviceInstallBase + std::string("/")
				+ Settings::LunaSettings()->serviceInstallRelative + std::string("/") + *serviceIdIt;
		ServiceDescription* serviceDesc = scanOneServiceFolder(servicePathFull);
		if (serviceDesc) {
			m_registeredServices[serviceDesc->id()] = serviceDesc;
			serviceInstallerInstallApp(*serviceIdIt, sServiceInstallerTypeService, Settings::LunaSettings()->appInstallBase);
		}
	}

	createOrUpdatePackageManifest(packageDesc);

	//force caches to clear
	WebAppMgrProxy::instance()->clearWebkitCache();

	//force a repaint
	WindowServer::instance()->update(0, 0, SystemUiController::instance()->currentUiWidth(),SystemUiController::instance()->currentUiHeight());
}

ApplicationDescription* ApplicationManager::installSysApp(const std::string& appId)
{
	if (appId.empty()) {
		g_message("%s: appId in %s was empty. This should never happen!", __FUNCTION__,appId.c_str());
		return NULL;
	}
	std::string appPathFull = Settings::LunaSettings()->appInstallBase + std::string("/")
	+ Settings::LunaSettings()->appInstallRelative + std::string("/") + appId;
	ApplicationDescription* pAppDesc = getAppById(appId);
	if (pAppDesc)
	{
		//TODO: support updating
		//for now updates are not supported, so just make sure the visible flag is on
		pAppDesc->setVisible(true);
		const LaunchPoint *pLpoint = pAppDesc->getDefaultLaunchPoint();
		if (pLpoint)
		{
			postLaunchPointChange(pLpoint, "updated");
		}
		return pAppDesc;
	}
	else
	{
		pAppDesc = scanOneApplicationFolder(appPathFull);
		if (!pAppDesc)
		{
			//try the root folder paths
			appPathFull = std::string("/") + Settings::LunaSettings()->appInstallRelative + std::string("/") + appId;
			pAppDesc = scanOneApplicationFolder(appPathFull);
		}

		if (!pAppDesc) {
			g_warning("%s: Failed to scan sysapp [%s] from any of its expected locations",__FUNCTION__, appId.c_str());
			return NULL;
		}
	}
	//sysapps are not removable
	pAppDesc->setRemovable(false);
	pAppDesc->setVisible(true);

	// newly installed app
	g_message("(A)\t%s", pAppDesc->id().c_str());
	m_registeredApps.push_back(pAppDesc);
	const LaunchPoint *pLpoint = pAppDesc->getDefaultLaunchPoint();
	if (pLpoint)
	{
		postLaunchPointChange(pLpoint, "added");
	}
	//notify of app install
	ApplicationInstaller::instance()->notifyAppInstalled(pAppDesc->id(), pAppDesc->version());
	return pAppDesc;
}

ApplicationDescription* ApplicationManager::installApp(const std::string& appId)
{
	if (appId.empty()) {
		g_message("appId in %s was empty. This should never happen!", __PRETTY_FUNCTION__);
		return NULL;
	}
	std::string appPathFull = Settings::LunaSettings()->appInstallBase + std::string("/")
			+ Settings::LunaSettings()->appInstallRelative + std::string("/") + appId;
	ApplicationDescription* existingAppDesc = getAppById(appId);
	ApplicationDescription* newAppDesc = scanOneApplicationFolder(appPathFull);
	if (!newAppDesc) {
		g_warning("Failed to scan newly installed/updated app: %s, which was supposed to be in [%s]", appId.c_str(),appPathFull.c_str());
		return NULL;
	}

	if (existingAppDesc) {
		// updated app
		g_message("(U)\t%s", newAppDesc->id().c_str());
		std::vector<std::string> pidList;
		existingAppDesc->executionLock();
		WebAppMgrProxy::instance()->sendAsyncMessage(new View_Mgr_KillApp(newAppDesc->id()));

		//now update the app descriptor for this app
		g_message("%s: updating app descriptor: initial value: %s",__FUNCTION__,existingAppDesc->toString().c_str());
		if (existingAppDesc->dockModeStatus() && !newAppDesc->dockModeStatus()) {
			g_message ("%s: Removing the old launch point from dock mode", __FUNCTION__);
			disableDockModeLaunchPoint(existingAppDesc->id().c_str());
		}
		existingAppDesc->update(*newAppDesc);
		g_message("%s: updated app descriptor: new value: %s",__FUNCTION__,existingAppDesc->toString().c_str());

		//and post a launchpoint update
		postLaunchPointChange(existingAppDesc->getDefaultLaunchPoint(), "updated");
		// remove the update appdesc from our pending list
		removePendingApp(existingAppDesc->id());
		existingAppDesc->executionLock(false);

		//notify of app install
		ApplicationInstaller::instance()->notifyAppInstalled(existingAppDesc->id(), existingAppDesc->version());

		//get rid of the new app descriptor; don't need it since there is an existing descriptor
		delete newAppDesc;

		return existingAppDesc;
	} else {
		// newly installed app
		g_message("(A)\t%s", newAppDesc->id().c_str());
		EventReporter::instance()->report("install", newAppDesc->id().c_str());
		m_registeredApps.push_back(newAppDesc);
		const LaunchPoint *pLpoint = newAppDesc->getDefaultLaunchPoint();
		if (pLpoint) {
			postLaunchPointChange(pLpoint, "added");
		}
		// remove the pending install appdesc from our pending list
		removePendingApp(newAppDesc->id());
		//notify of app install
		ApplicationInstaller::instance()->notifyAppInstalled(newAppDesc->id(), newAppDesc->version());
		return newAppDesc;
	}
}

void ApplicationManager::createOrUpdatePackageManifest(PackageDescription* packageDesc)
{
#if !defined(TARGET_DESKTOP)
	if (!packageDesc) {
		g_warning("packageDesc was null in %s", __PRETTY_FUNCTION__);
		return;
	}

	bool isSystemApp = (packageDesc->folderPath().find("/usr") == 0);

	// check the size of the package
	if (Settings::LunaSettings()->scanCalculatesAppSizes && !isSystemApp) {
		g_debug("%s: [SIZE-SCAN] [MANIFESTS]: Calculating package size for [%s]", __PRETTY_FUNCTION__, packageDesc->id().c_str());
		uint64_t fsbsize;
		std::string packageFolderPath = packageDesc->folderPath();
		(void)ApplicationInstaller::getFsFreeSpaceInBlocks(packageFolderPath, &fsbsize); //I don't expect this to EVER fail if the folderPath is correct
																								 //(which means the rest of the scan before ths is not corrupted)
		// check to see if the manifest file exists
		std::string manifestFilePath = Settings::LunaSettings()->packageManifestsPath + std::string("/") + packageDesc->id() + std::string(".pmmanifest");
		json_object * manifestJobj = json_object_from_file((char *)manifestFilePath.c_str());
		if ((manifestJobj == 0) || (is_error(manifestJobj))) {
			// didn't find the manifest...generate it!
			g_debug("%s: [MANIFESTS]: manifest for %s not found, generating",__PRETTY_FUNCTION__, packageDesc->id().c_str());
			packageDesc->setPackageSize(ApplicationInstaller::getSizeOfPackageOnFsGenerateManifest("", packageDesc, NULL));
		} else {
			// Found the manifest, but check to see if the version of the package was updated...if it was, then regenerate manifest.
			// otherwise, just grab the total size from the manifest...providing the current installed fs blocksize's corresponding size is in the manifest. If not, regenerate!
			std::string manifestVersion;
			std::string installerVersion;
			extractFromJson(manifestJobj,"version",manifestVersion);
			extractFromJson(manifestJobj,"installer",installerVersion);

			if ((manifestVersion != packageDesc->version()) || (installerVersion != ApplicationInstaller::s_installer_version)) {
				// regen manifest
				g_debug("%s: [MANIFESTS]: manifest for %s is for version [%s],[%s] and package and installer are at version [%s],[%s],  re-generating",
							__PRETTY_FUNCTION__,
							packageDesc->id().c_str(),
							manifestVersion.c_str(),
							installerVersion.c_str(),
							packageDesc->version().c_str(),
							ApplicationInstaller::s_installer_version.c_str());
				packageDesc->setPackageSize(ApplicationInstaller::getSizeOfPackageOnFsGenerateManifest("", packageDesc, NULL));
			} else {
				json_object * totalSizesJobj = JsonGetObject(manifestJobj,"totals");
				if (totalSizesJobj == NULL) {
					// totals not found...manifest needs to be regen-d
					g_debug("%s: [MANIFESTS]: manifest for %s missing size totals table, re-generating", __PRETTY_FUNCTION__, packageDesc->id().c_str());
					packageDesc->setPackageSize(ApplicationInstaller::getSizeOfPackageOnFsGenerateManifest("", packageDesc, NULL));
				} else {
					std::string bsizeStr = toSTLString<uint64_t>(fsbsize);
					std::string packageSizeStr;
					if (extractFromJson(totalSizesJobj, bsizeStr.c_str(), packageSizeStr) == false) {
						// totals not found...manifest needs to be regen-d
						g_debug("%s: [MANIFESTS]: manifest for %s missing size total for blocksize = %s, re-generating", __PRETTY_FUNCTION__, packageDesc->id().c_str(), bsizeStr.c_str());
						packageDesc->setPackageSize(ApplicationInstaller::getSizeOfPackageOnFsGenerateManifest("", packageDesc, NULL));
					} else {
						// set the size that's found
						uint64_t packageSize = strtouq(packageSizeStr.c_str(), NULL, 10);
						g_debug("%s: [MANIFESTS]: manifest for %s blocksize = %s found total size = %llu...setting on package desc",__PRETTY_FUNCTION__, packageDesc->id().c_str(), bsizeStr.c_str(), packageSize);
						packageDesc->setPackageSize(packageSize);
					}
				}
			} //end else = manifest is for the correct version
			json_object_put(manifestJobj);
		}	//end else = found manifest
		packageDesc->setBlockSize((uint32_t)fsbsize);	//should be an ok cast, because if bsize is ever > 2^32 -1 , we have issues!!!!

		g_debug("%s: [SIZES]: size of [%s] is %llu",__PRETTY_FUNCTION__, packageDesc->id().c_str(), packageDesc->packageSize());
	}
#endif
}

void ApplicationManager::serviceInstallerInstallApp(const std::string& id, const std::string& type, const std::string& root)
{
#if !(defined(TARGET_DESKTOP) || defined(TARGET_EMULATOR))
// TODO:  Reactivate ServiceInstaller
	::installApp(id, type, root);
#endif
}

void ApplicationManager::serviceInstallerUninstallApp(const std::string& id, const std::string& type, const std::string& root)
{
#if !(defined(TARGET_DESKTOP) || defined(TARGET_EMULATOR))
// TODO:  Reactivate ServiceInstaller
	::uninstallApp(id, type, root);
#endif
}

void ApplicationManager::dumpStats( )
{

}

//MDK-LAUNCHER
//std::vector<const LaunchPoint*> ApplicationManager::dockLaunchPoints(bool *needsUpdate)
//{
//	// needsUpdate won't touch the existing state unless it needs to mark it as stale
//	// thus you can safely propogate existing state through here without losing it
//	// or needing to back it up
//	MutexLocker locker(&m_mutex);
//
//	std::vector<const LaunchPoint*> dockLaunchPoints;
//
//	std::vector<std::string> dockLaunchPointIds;
//	DockPositionManager* dpm = DockPositionManager::instance();
//	bool lpLoaded = dpm->getLaunchPointIds(dockLaunchPointIds);
//
//	if (lpLoaded) {
//		for (std::vector<std::string>::iterator it = dockLaunchPointIds.begin();
//		it != dockLaunchPointIds.end(); ++it) {
//
//			const LaunchPoint* lp = getLaunchPointById( *it );
//
//			if (lp) {
//				dockLaunchPoints.push_back(lp);
//			} else {
//				if (needsUpdate) *needsUpdate = true;
//			}
//		}
//	}
//
//	return dockLaunchPoints;
//}

std::vector<const LaunchPoint*> ApplicationManager::dockLaunchPoints(bool *needsUpdate)
{
	if (needsUpdate) *needsUpdate = false;
	return std::vector<const LaunchPoint*>();
}

std::vector<const LaunchPoint*> ApplicationManager::allLaunchPoints()
{
	MutexLocker locker(&m_mutex);

	std::vector<const LaunchPoint*> launchPoints;
	std::vector<ApplicationDescription*>::const_iterator it, itEnd;
	for (it = m_registeredApps.begin(), itEnd = m_registeredApps.end();	it != itEnd; ++it) {

		ApplicationDescription* appDesc = *it;

		if (appDesc == NULL)
			continue;

		if (appDesc->isVisible() == false)
			continue;

		if (!hardwareFeaturesRequirementSatisfied(appDesc->hardwareFeaturesNeeded()))
			continue;

		for (LaunchPointList::const_iterator iter = appDesc->launchPoints().begin();
		iter != appDesc->launchPoints().end(); ++iter) {

			launchPoints.push_back(*iter);
		}
	}

	return launchPoints;
}

std::vector<const LaunchPoint*> ApplicationManager::allPendingLaunchPoints()
{
	MutexLocker locker(&m_mutex);

	std::vector<const LaunchPoint*> launchPoints;
	std::vector<ApplicationDescription*>::const_iterator it, itEnd;
	for (it = m_pendingApps.begin(), itEnd = m_pendingApps.end(); it != itEnd; ++it) {

		ApplicationDescription* appDesc = *it;

		if (appDesc == NULL)
			continue;

		if (appDesc->isVisible() == false)
			continue;

		if (!hardwareFeaturesRequirementSatisfied(appDesc->hardwareFeaturesNeeded()))
			continue;

		for (LaunchPointList::const_iterator iter = appDesc->launchPoints().begin();
		iter != appDesc->launchPoints().end(); ++iter) {

			launchPoints.push_back(*iter);
		}
	}

	return launchPoints;
}

std::vector<ApplicationDescription*> ApplicationManager::allApps()
{
	MutexLocker locker(&m_mutex);

	std::vector<ApplicationDescription*> allApps = m_registeredApps;

	return allApps;
}

std::map<std::string, PackageDescription*> ApplicationManager::allPackages()
{
	return m_registeredPackages;
}

ApplicationDescription* ApplicationManager::getAppById( const std::string& appId )
{
	MutexLocker locker(&m_mutex);

	for( std::vector<ApplicationDescription*>::iterator it=m_registeredApps.begin();
	it != m_registeredApps.end(); ++it )
	{
		ApplicationDescription* app = *it;
		if( app->id() == appId )
			return app;
	}

	for( std::vector<ApplicationDescription*>::iterator it=m_systemApps.begin();
	it != m_systemApps.end(); ++it )
	{
		ApplicationDescription* app = *it;
		if (app->id() == appId )
			return app;
	}
	return 0;
}

ApplicationDescription* ApplicationManager::getAppByIdHardwareCompatibleAppsOnly( const std::string& appId )
{
	MutexLocker locker(&m_mutex);

	for( std::vector<ApplicationDescription*>::iterator it=m_registeredApps.begin();
	it != m_registeredApps.end(); ++it )
	{
		ApplicationDescription* app = *it;
		if ((app->id() == appId) && (hardwareFeaturesRequirementSatisfied(app->hardwareFeaturesNeeded())))
			return app;
	}

	for( std::vector<ApplicationDescription*>::iterator it=m_systemApps.begin();
	it != m_systemApps.end(); ++it )
	{
		ApplicationDescription* app = *it;
		if ((app->id() == appId) && (hardwareFeaturesRequirementSatisfied(app->hardwareFeaturesNeeded())))
			return app;
	}
	return 0;
}


std::vector<const LaunchPoint*> ApplicationManager::allDockModeLaunchPoints()
{
	MutexLocker locker(&m_mutex);

	std::vector<const LaunchPoint*> launchPoints;
	std::vector<ApplicationDescription*>::const_iterator it, itEnd;
	for (it = m_registeredApps.begin(), itEnd = m_registeredApps.end();	it != itEnd; ++it) {

		ApplicationDescription* appDesc = *it;

		if (appDesc == NULL)
			continue;

		if (!hardwareFeaturesRequirementSatisfied(appDesc->hardwareFeaturesNeeded()))
			continue;

		if (!appDesc->dockModeStatus())
			continue;

		launchPoints.push_back(appDesc->getDefaultLaunchPoint());
	}

	return launchPoints;
}

bool ApplicationManager::enableDockModeLaunchPoint (const char* appId)
{
	if (appId) {
		ApplicationDescription* appDesc = getAppById(appId);
		if (appDesc) {
			m_dockModeLaunchPoints.insert(appDesc->getDefaultLaunchPoint());
			Q_EMIT signalDockModeLaunchPointEnabled (appDesc->getDefaultLaunchPoint());
			return true;
		}
		else {
			g_warning ("%s: appId %s not found", __PRETTY_FUNCTION__, appId);
		}
	}
	return false;
}

bool ApplicationManager::disableDockModeLaunchPoint (const char* appId)
{
	if (appId) {
		ApplicationDescription* appDesc = getAppById(appId);
		if (appDesc) {
			Q_EMIT signalDockModeLaunchPointDisabled (appDesc->getDefaultLaunchPoint());
			m_dockModeLaunchPoints.erase(appDesc->getDefaultLaunchPoint());
		}
	}
	return false;
}


ApplicationDescription* ApplicationManager::getPendingAppById(const std::string& appId)
{
	MutexLocker locker(&m_mutex);

	std::vector<ApplicationDescription*>::iterator it, itEnd;
	for (it=m_pendingApps.begin(), itEnd = m_pendingApps.end(); it != itEnd; ++it)
	{
		ApplicationDescription* app = *it;
		if( app->id() == appId )
			return app;
	}
	return 0;
}

bool ApplicationManager::getAppsByPackageId(const std::string& packageId, std::vector<ApplicationDescription *>& r_apps)
{
	PackageDescription* packageDesc = NULL;
	std::map<std::string, PackageDescription*>::iterator find_it = m_registeredPackages.find(packageId);
	if (find_it != m_registeredPackages.end())
		packageDesc = find_it->second;
	else
		return false;

	std::vector<std::string>::const_iterator appIdIt, appIdItEnd;
	for (appIdIt = packageDesc->appIds().begin(), appIdItEnd = packageDesc->appIds().end(); appIdIt != appIdItEnd; ++appIdIt) {
		ApplicationDescription* appDesc = getAppById(*appIdIt);
		if (appDesc)
			r_apps.push_back(appDesc);
		else
			g_critical("Package %s specified app %s, but it is missing",
					   packageId.c_str(), appIdIt->c_str());
	}
	return true;
}

bool ApplicationManager::getServicesByPackageId(const std::string& packageId, std::vector<ServiceDescription *>& r_services)
{
	PackageDescription* packageDesc = getPackageInfoByPackageId(packageId);
	if (!packageDesc)
		return false;

	std::vector<std::string>::const_iterator serviceIdIt, serviceIdItEnd;
	for (serviceIdIt = packageDesc->serviceIds().begin(), serviceIdItEnd = packageDesc->serviceIds().end(); serviceIdIt != serviceIdItEnd; ++serviceIdIt) {
		r_services.push_back(getServiceInfoByServiceId(*serviceIdIt));
	}
	return true;
}

PackageDescription* ApplicationManager::getPackageInfoByAppId(const std::string& anyAppIdInPackage)
{
    for (std::map<std::string, PackageDescription*>::const_iterator it = m_registeredPackages.begin(); it != m_registeredPackages.end(); ++it) {
		PackageDescription* packageDesc = (*it).second;
		std::vector<std::string> appIds = packageDesc->appIds();
		std::vector<std::string>::const_iterator appIdIt, appIdItEnd;
		for (appIdIt = appIds.begin(), appIdItEnd = appIds.end(); appIdIt != appIdItEnd; ++appIdIt) {
			if (anyAppIdInPackage == *appIdIt) {
				return packageDesc;
			}
		}
    }
	return NULL;
}

PackageDescription* ApplicationManager::getPackageInfoByServiceId(const std::string& anyServiceIdInPackage)
{
    for (std::map<std::string, PackageDescription*>::const_iterator it = m_registeredPackages.begin(); it != m_registeredPackages.end(); ++it) {
		PackageDescription* packageDesc = (*it).second;
		std::vector<std::string> serviceIds = packageDesc->serviceIds();
		std::vector<std::string>::const_iterator serviceIdIt, serviceIdItEnd;
		for (serviceIdIt = serviceIds.begin(), serviceIdItEnd = serviceIds.end(); serviceIdIt != serviceIdItEnd; ++serviceIdIt) {
			if (anyServiceIdInPackage == *serviceIdIt) {
				return packageDesc;
			}
		}
    }
	return NULL;
}

PackageDescription* ApplicationManager::getPackageInfoByPackageId(const std::string& packageId)
{
	std::map<std::string, PackageDescription*>::iterator find_it = m_registeredPackages.find(packageId);
	if (find_it != m_registeredPackages.end())
		return find_it->second;
	return NULL;
}

ServiceDescription* ApplicationManager::getServiceInfoByServiceId(const std::string& serviceId)
{
	std::map<std::string, ServiceDescription*>::iterator find_it = m_registeredServices.find(serviceId);
	if (find_it != m_registeredServices.end())
		return find_it->second;
	return NULL;
}

///BE SURE TO EXTERNALLY LOCK APPLIST IF NEEDED!!!
ApplicationDescription* ApplicationManager::getAppById( const std::string& appId,const std::map<std::string,ApplicationDescription *>& appMap)
{
	std::map<std::string,ApplicationDescription *>::const_iterator it = appMap.find(appId);
	if (it != appMap.end()) {
		return it->second;
	}

	return 0;
}

const LaunchPoint* ApplicationManager::getLaunchPointByIdHardwareCompatibleAppsOnly(const std::string& launchPointId)
{
	if (launchPointId.empty())
		return 0;

	MutexLocker locker(&m_mutex);

	std::vector<ApplicationDescription*>::const_iterator it, itEnd;
	for (it = m_registeredApps.begin(), itEnd = m_registeredApps.end(); it != itEnd; ++it)
	{
		ApplicationDescription* appDesc = *it;
		if (appDesc == NULL)
			continue;	//BAD
		if (!(hardwareFeaturesRequirementSatisfied(appDesc->hardwareFeaturesNeeded())))
			continue;

		for (LaunchPointList::const_iterator iter = appDesc->launchPoints().begin();
				iter != appDesc->launchPoints().end(); ++iter) {

			const LaunchPoint* lp = *iter;
			if (launchPointId == lp->launchPointId()) {
				// always include pending versions
				if (lp->isDefault()) {
					ApplicationDescription* pending = getPendingAppById(lp->appDesc()->id());
					if (pending) {
						lp = pending->getDefaultLaunchPoint();
					}
				}
				return lp;
			}
		}
	}

	for (it = m_pendingApps.begin(), itEnd = m_pendingApps.end(); it != itEnd; ++it)
	{
		ApplicationDescription* appDesc = *it;
		if (appDesc == NULL)
			continue;	//BAD
		if (!(hardwareFeaturesRequirementSatisfied(appDesc->hardwareFeaturesNeeded())))
			continue;
		for (LaunchPointList::const_iterator iter = appDesc->launchPoints().begin();
				iter != appDesc->launchPoints().end(); ++iter) {

			const LaunchPoint* lp = *iter;
			if (launchPointId == lp->launchPointId())
				return lp;
		}
	}

	return 0;
}

const LaunchPoint* ApplicationManager::getLaunchPointById(const std::string& launchPointId)
{
	if (launchPointId.empty())
		return 0;

	MutexLocker locker(&m_mutex);

	std::vector<ApplicationDescription*>::const_iterator it, itEnd;
	for (it = m_registeredApps.begin(), itEnd = m_registeredApps.end(); it != itEnd; ++it)
	{
		ApplicationDescription* appDesc = *it;

		for (LaunchPointList::const_iterator iter = appDesc->launchPoints().begin();
		iter != appDesc->launchPoints().end(); ++iter) {

			const LaunchPoint* lp = *iter;
			if (launchPointId == lp->launchPointId()) {
				// always include pending versions
				if (lp->isDefault()) {
					ApplicationDescription* pending = getPendingAppById(lp->appDesc()->id());
					if (pending) {
						lp = pending->getDefaultLaunchPoint();
					}
				}
				return lp;
			}
		}
	}

	for (it = m_pendingApps.begin(), itEnd = m_pendingApps.end(); it != itEnd; ++it)
	{
		ApplicationDescription* appDesc = *it;

		for (LaunchPointList::const_iterator iter = appDesc->launchPoints().begin();
		iter != appDesc->launchPoints().end(); ++iter) {

			const LaunchPoint* lp = *iter;
			if (launchPointId == lp->launchPointId())
				return lp;
		}
	}

	return 0;
}

void ApplicationManager::scanForApplications()
{
	MutexLocker locker(&m_mutex);

	//clear();

	std::string appFolder = Settings::LunaSettings()->lunaAppsPath;
	std::vector<std::string>::iterator appFolderIter = Settings::LunaSettings()->lunaAppsPaths.begin();
	while (appFolderIter !=  Settings::LunaSettings()->lunaAppsPaths.end()) {
		appFolder = *appFolderIter;
		if (appFolder.size()) {

			if (appFolder[appFolder.size() - 1] != '/')
				appFolder += "/";

			luna_log(sAppMgrChnl, "scanning apps from %s", appFolder.c_str());
			scanApplicationsFolders(appFolder);
		}
		appFolderIter++;
	}
	//dumpStats();
}

void ApplicationManager::scanForPackages()
{
	std::string packagesFolder = Settings::LunaSettings()->packageInstallBase + std::string("/")
			+ Settings::LunaSettings()->packageInstallRelative;

	if (packagesFolder[packagesFolder.size() - 1] != '/')
		packagesFolder += "/";

	struct dirent** list = NULL;
	int count = ::scandir(packagesFolder.c_str(), &list, 0, 0);
	if (count < 0)
		return;

	for (int i = 0; i < count; i++) {
		if (list[i]) {
			if (list[i]->d_name[0] != '.') {
				std::string onePackageFolderPath = packagesFolder + list[i]->d_name;

				struct stat stBuf;
				if (::stat(onePackageFolderPath.c_str(), &stBuf) == 0 && stBuf.st_mode & S_IFDIR) {
					PackageDescription* packageDesc = scanOnePackageFolder(onePackageFolderPath);
					if (packageDesc) {
						m_registeredPackages[packageDesc->id()] = packageDesc;
						if (packageDesc->accountIds().size() > 0) {
							std::vector<ApplicationDescription*> apps;
							getAppsByPackageId(packageDesc->id(), apps);
							for (std::vector<ApplicationDescription*>::iterator it = apps.begin(); it != apps.end(); ++it) {
								ApplicationDescription* appDesc = *it;
								appDesc->setHasAccounts(true);
							}
						}
						createOrUpdatePackageManifest(packageDesc);
					}
				}
			}
			free(list[i]);
		}
	}

	g_message("%s(%s): the new-style packages are now: ", __PRETTY_FUNCTION__, packagesFolder.c_str());
    for (std::map<std::string, PackageDescription*>::const_iterator it = m_registeredPackages.begin(); it != m_registeredPackages.end(); ++it) {
		PackageDescription* packageDesc = (*it).second;
		g_message("\t%s", packageDesc->id().c_str());
	}

	if (list)
		free(list);
}

void ApplicationManager::createPackageDescriptionForOldApps()
{
	for (std::vector<ApplicationDescription*>::iterator it = m_registeredApps.begin(); it != m_registeredApps.end(); ++it) {
		ApplicationDescription* appDesc = *it;
		if (!getPackageInfoByAppId(appDesc->id())) {
			// This app did not have a packageinfo under /packages. We therefore need to create the PackageDescription for it
			PackageDescription* packageDesc = PackageDescription::fromApplicationDescription(appDesc);
			if (packageDesc) {
				m_registeredPackages[packageDesc->id()] = packageDesc;
				createOrUpdatePackageManifest(packageDesc);
			}
		}
	}

	g_message("%s: the packages are now: ", __PRETTY_FUNCTION__);
    for (std::map<std::string, PackageDescription*>::const_iterator it = m_registeredPackages.begin(); it != m_registeredPackages.end(); ++it) {
		PackageDescription* packageDesc = (*it).second;
		g_message("\t%s", packageDesc->id().c_str());
	}
}

void ApplicationManager::scanForServices()
{
	std::string servicesFolder = Settings::LunaSettings()->serviceInstallBase + std::string("/")
			+ Settings::LunaSettings()->serviceInstallRelative;

	if (servicesFolder[servicesFolder.size() - 1] != '/')
		servicesFolder += "/";

	struct dirent** list = NULL;
	int count = ::scandir(servicesFolder.c_str(), &list, 0, 0);
	if (count < 0)
		return;

	for (int i = 0; i < count; i++) {
		if (list[i]) {
			if (list[i]->d_name[0] != '.') {
				std::string oneServiceFolderPath = servicesFolder + list[i]->d_name;

				struct stat stBuf;
				if (::stat(oneServiceFolderPath.c_str(), &stBuf) == 0 && stBuf.st_mode & S_IFDIR) {
					ServiceDescription* serviceDesc = scanOneServiceFolder(oneServiceFolderPath);
					if (serviceDesc) {
						m_registeredServices[serviceDesc->id()] = serviceDesc;
					}
				}
			}
			free(list[i]);
		}
	}

	g_message("%s(%s): the services are now: ", __PRETTY_FUNCTION__, servicesFolder.c_str());

    for (std::map<std::string, ServiceDescription*>::const_iterator it = m_registeredServices.begin(); it != m_registeredServices.end(); ++it) {
		ServiceDescription* serviceDesc = (*it).second;
		g_message("\t%s", serviceDesc->id().c_str());
	}

	if (list)
		free(list);
}

void ApplicationManager::scanForSystemApplications()
{
	MutexLocker locker(&m_mutex);

	std::vector<std::string> systemPaths;
	std::string folder = Settings::LunaSettings()->lunaAppLauncherPath;
	if (folder[folder.size() - 1] != '/')
		folder += '/';
	systemPaths.push_back(folder);

	folder = Settings::LunaSettings()->lunaSystemPath;
	if (folder[folder.size() - 1] != '/')
		folder += '/';
	systemPaths.push_back(folder);

	const char* lowMemoryFolder = "/usr/palm/sysmgr/low-memory";
	systemPaths.push_back(lowMemoryFolder);

	std::string platformVersion = DeviceInfo::instance()->platformVersion();

	for (size_t i=0; i < systemPaths.size(); i++) {
		ApplicationDescription* appDesc = scanOneApplicationFolder(systemPaths[i]);
		if (appDesc) {

			if (!getAppById(appDesc->id())) {			//if not seen yet, process, else discard
				//force non-removable
				appDesc->setRemovable(false);
				appDesc->setVersion(platformVersion);
				m_systemApps.push_back(appDesc);
			}
			else {
				delete appDesc;
			}
		}
	}
}

void ApplicationManager::scanForPendingApplications()
{
	MutexLocker locker(&m_mutex);

	// scan for applications which are pending installation or updating
	std::string folder = Settings::LunaSettings()->pendingAppsPath;
	if (folder[folder.size()-1] != '/')
		folder += '/';
	g_message("%s: attempting to open directory %s", __PRETTY_FUNCTION__, folder.c_str());
	GDir* dir = g_dir_open(folder.c_str(), 0, 0);
	if (dir) {
		const gchar* subdir = 0;
		while((subdir = g_dir_read_name(dir)) != NULL) {
			std::string appFolderPath = folder + subdir;
			if (g_file_test(appFolderPath.c_str(), G_FILE_TEST_IS_DIR)) {
				g_message("found a directory: %s", appFolderPath.c_str());
				ApplicationDescription* appDesc = scanOneApplicationFolder(appFolderPath);
				if (appDesc) {
					// we found a valid app!
					bool appExists = getAppById(appDesc->id()) != 0;
					if (appExists)
						appDesc->setStatus(ApplicationDescription::Status_Updating);
					else
						appDesc->setStatus(ApplicationDescription::Status_Installing);
					appDesc->setRemovable(true); // always deletable
					m_pendingApps.push_back(appDesc);
					//LAUNCHER3-ADD:
					Q_EMIT signalScanFoundApp(appDesc);
				}
			}
			else {
				g_message("encountered non-directory item: %s", appFolderPath.c_str());
			}
		}
		g_dir_close(dir);
	}
	else {
		g_warning("%s: directory does not exist!", __PRETTY_FUNCTION__);
	}
}

void ApplicationManager::scanForLaunchPoints(std::string launchPointFolder)
{
	MutexLocker locker(&m_mutex);

	//std::string launchPointFolder = Settings::LunaSettings()->lunaLaunchPointsPath;

	if (!launchPointFolder.size())
		return;

	/*
	 * 	This should be the responsibility of the individual making the preset launch point and placing it into the
	 * read-only system partition with the correct removable:false key-value in the launchpoint's json content/descriptor.
	 */

	bool forceNonRemovable=false;
	if (launchPointFolder == Settings::LunaSettings()->lunaPresetLaunchPointsPath)
		forceNonRemovable=true;

	if (launchPointFolder[launchPointFolder.size() - 1] != '/')
		launchPointFolder += '/';

	struct dirent** list;
	int count = scandir(launchPointFolder.c_str(), &list, 0, 0);
	if (count < 0)
		return;

	for (int i = 0; i < count; i++) {

		if (list[i]->d_name[0] == '.') {
			free(list[i]);
			continue;
		}

		std::string filePath = launchPointFolder + list[i]->d_name;

		LaunchPoint* launchPoint = LaunchPoint::fromFile(0, filePath);
		if (!launchPoint) {
			free(list[i]);
			continue;
		}
		else {
			if (forceNonRemovable)
				launchPoint->setRemovable(false);
		}

		ApplicationDescription* appDesc = getAppById(launchPoint->id());
		if (!appDesc) {
			free(list[i]);
			delete launchPoint;
			continue;
		}

		launchPoint->setAppDesc(appDesc);
		appDesc->addLaunchPoint(launchPoint);
		//LAUNCHER3-ADD
		Q_EMIT signalScanFoundAuxiliaryLaunchPoint(appDesc,launchPoint);
		free(list[i]);
	}

	free(list);
}

void ApplicationManager::scanApplicationsFolders(const std::string& appFoldersPath)
{
	std::string folderPath(appFoldersPath);
	//folderPath += "/";									// HV	-  this was causing e.g.  '/var/luna/applications//phone' instead of '/var/luna/applications/phone'
	//			I changed the local mod here rather than appFoldersPath in the caller, as I don't know what else in the sys relies on
	//			it having a trailing slash  (/var/luna/applications/)

	struct dirent** list=NULL;
	int count = ::scandir(appFoldersPath.c_str(), &list, 0, 0);
	if (count < 0)
		return;

	//check to see if this is a system folder: rooted at /usr               TODO: make this better
	bool isSystemFolder;
	std::string systemPath("/usr");

	if (appFoldersPath.find(systemPath.c_str()) == 0)
		isSystemFolder = true;
	else
		isSystemFolder = false;

	std::string platformVersion = DeviceInfo::instance()->platformVersion();


	for (int i = 0; i < count; i++) {

		if (list[i]) {
			if (list[i]->d_name[0] != '.') {

				std::string oneAppFolderPath = folderPath + list[i]->d_name;

				struct stat stBuf;
				if (::stat(oneAppFolderPath.c_str(), &stBuf) == 0 && stBuf.st_mode & S_IFDIR) {
					ApplicationDescription* appDesc = scanOneApplicationFolder(oneAppFolderPath);
					if (appDesc) {

#if !defined(TARGET_DESKTOP)
						if (!isSystemFolder && appDesc->type() == ApplicationDescription::Type_SysmgrBuiltin)
						{
							if (!isSysappAllowed(appDesc->id(),appFoldersPath))
							{
								delete appDesc;
								free(list[i]);
								continue;
							}
						}
#endif
						// ignore duplicate applications and applications which have been user-hidden
						if (!isAppHidden(appDesc->id()) && !getAppById(appDesc->id())) {
							if (isSystemFolder) {

								//appDesc->setUserHideable(appDesc->isRemovable());

								// It appears as if we've multiplexed hideable and removable for system applications. So a system app that wants to
								//		be hideable has removable=true in its appinfo json.
								//		Use this to first set the hideable flag. Then flip it back to removable = false
								//		Thus, we use the extra variable isSystemFolder to disambiguate the two cases (removable or just hideable)
								appDesc->setUserHideable(appDesc->isRemovable());
								appDesc->setRemovable(false);

								if (isTrustedPalmApp(appDesc)) {
									appDesc->setVersion(platformVersion);
								}
							}

							qDebug() << " ============= App: " << QString(appDesc->id().c_str()) << " is Removable = " << (appDesc->isRemovable() ? "TRUE" : "FALSE")
									<< " , UserHideable = " << (appDesc->isUserHideable() ? "TRUE" : "FALSE");

							m_registeredApps.push_back(appDesc);
							//LAUNCHER3-ADD:
							Q_EMIT signalScanFoundApp(appDesc);
							//--end
						}
						else {
							delete appDesc;
						}
					}
				}
			}

			free(list[i]);
		}
	}

	g_message("ApplicationManager::scanApplicationsFolders(%s): the apps are now: ",appFoldersPath.c_str());
	for( std::vector<ApplicationDescription*>::iterator it=m_registeredApps.begin();
	it != m_registeredApps.end(); ++it )
	{
		ApplicationDescription* app = *it;
		g_message("\t%s",app->id().c_str());
	}

	if (list)
		free(list);
}

void ApplicationManager::scanApplicationsFolders(const std::string& appFoldersPath,std::map<std::string,ApplicationDescription *>& foundApps)
{
	std::string folderPath(appFoldersPath);

//	g_message("\tappFoldersPath = %s",appFoldersPath.c_str());
	struct dirent** list=NULL;
	int count = ::scandir(appFoldersPath.c_str(), &list, 0, 0);
	if (count < 0)
		return;

	for (int i = 0; i < count; i++) {

		if (list[i]) {
			if (list[i]->d_name[0] != '.') {

				std::string oneAppFolderPath = folderPath + list[i]->d_name;

				struct stat stBuf;
				if (::stat(oneAppFolderPath.c_str(), &stBuf) == 0 && stBuf.st_mode & S_IFDIR) {
					ApplicationDescription* appDesc = scanOneApplicationFolder(oneAppFolderPath);
					if (appDesc) {
						if (!getAppById(appDesc->id(),foundApps)) {
//							g_message("ApplicationManager::scanApplicationsFolders(%s): adding %s",appFoldersPath.c_str(),appDesc->id().c_str());
							foundApps[appDesc->id()] = appDesc;
						}
						else {
							delete appDesc;
						}
					}
				}
			}

			free(list[i]);
		}
	}

	g_message("ApplicationManager::scanApplicationsFolders(%s): the apps are now: ",appFoldersPath.c_str());
	for( std::map<std::string,ApplicationDescription*>::iterator it=foundApps.begin();
	it != foundApps.end(); ++it )
	{
		ApplicationDescription* app = it->second;
		g_message("\t%s",app->id().c_str());
	}

	if (list)
		free(list);
}

ApplicationDescription* ApplicationManager::scanOneApplicationFolder(const std::string& appFolderPath)
{
	// Do we have a locale setting
	std::string locale = Preferences::instance()->locale();

	// Look for the language/region specific appinfo.json

	std::string language, region;
	std::size_t underscorePos = locale.find("_");
	if (underscorePos != std::string::npos) {
		language = locale.substr(0, underscorePos);
		region = locale.substr(underscorePos+1);
	}
	
	std::string appJsonPath;
	ApplicationDescription* appDesc = 0;

	if (!language.empty() && !region.empty()) {
		appJsonPath = appFolderPath + "/resources/" + language + "/" + region +"/appinfo.json";
		appDesc = ApplicationDescription::fromFile(appJsonPath, appFolderPath);
	}
	
	if (!appDesc) {
		// try the language-only one
		appJsonPath = appFolderPath + "/resources/" + language + "/appinfo.json";
		appDesc = ApplicationDescription::fromFile(appJsonPath, appFolderPath);
	}

	if (!appDesc) {
		//try the old version
		appJsonPath = appFolderPath + "/resources/" + locale + "/appinfo.json";
		appDesc = ApplicationDescription::fromFile(appJsonPath, appFolderPath);
	}

	if (!appDesc) {

		// FIXME: AppId needs to be based on folder name (and not specified in appinfo.json)
		// try the default one
		appJsonPath = appFolderPath + "/appinfo.json";
		appDesc = ApplicationDescription::fromFile(appJsonPath, appFolderPath);
	}

	if (!appDesc) {
		// Failed to find valid appinfo. bail out
		return 0;
	}

	// Check the white-list to see if this app is "allowed" to be installed.
	appDesc = ApplicationManager::checkAppAgainstWhiteList(appDesc);

	if (appDesc) {
		if (appDesc->type() == ApplicationDescription::Type_SysmgrBuiltin)
		{
			if (!isSysappAllowed(appDesc->id(),appFolderPath))
			{
				delete appDesc;
				return NULL;
			}
		}
		if (!appDesc->isVisible())
		{
			g_message("%s: app [%s] is set invisible via visible:false in its appinfo",__FUNCTION__,appDesc->id().c_str());
			//TODO: this if-clause is just for debug for now, so it prints a message. Let it fall through to the next
			// if-clause to handle the case where this app is ALSO hidden via the hidden list or duplicated
		}
		// ignore duplicate applications
		if (!isAppHidden(appDesc->id()) || !getAppById(appDesc->id())) {

			//check to see if this is a system folder: rooted at /usr		TODO: make this better
			bool isSystemFolder;
			std::string systemPath("/usr");

			if (appFolderPath.find(systemPath.c_str()) == 0)
				isSystemFolder = true;
			else
				isSystemFolder = false;

			std::string platformVersion = DeviceInfo::instance()->platformVersion();

			if (isSystemFolder) {
				appDesc->setUserHideable(appDesc->isRemovable());
				if (isTrustedPalmApp(appDesc)) {
					appDesc->setVersion(platformVersion);
				}
			}
		} else {
			delete appDesc;
			return NULL;
		}
	}

	return appDesc;
}

PackageDescription* ApplicationManager::scanOnePackageFolder(const std::string& packageFolderPath)
{
	PackageDescription* packageDesc = NULL;

	// Do we have a locale setting
	std::string locale = Preferences::instance()->locale();

	// Look for the language/region specific appinfo.json

	std::string language, region;
	std::size_t underscorePos = locale.find("_");
	if (underscorePos != std::string::npos) {
		language = locale.substr(0, underscorePos);
		region = locale.substr(underscorePos+1);
	}

	std::string packageJsonPath;

	if (!language.empty() && !region.empty()) {
		packageJsonPath = packageFolderPath + "/resources/" + language + "/" + region +"/packageinfo.json";
		packageDesc = PackageDescription::fromFile(packageJsonPath, packageFolderPath);
	}

	if (!packageDesc) {
		// try the language-only one
		packageJsonPath = packageFolderPath + "/resources/" + language + "/packageinfo.json";
		packageDesc = PackageDescription::fromFile(packageJsonPath, packageFolderPath);
	}

	if (!packageDesc) {
		//try the old version
		packageJsonPath = packageFolderPath + "/resources/" + locale + "/packageinfo.json";
		packageDesc = PackageDescription::fromFile(packageJsonPath, packageFolderPath);
	}

	if (!packageDesc) {
		// FIXME: AppId needs to be based on folder name (and not specified in appinfo.json)
		// try the default one
		packageJsonPath = packageFolderPath + "/packageinfo.json";
		packageDesc = PackageDescription::fromFile(packageJsonPath, packageFolderPath);
	}
	return packageDesc;
}

ServiceDescription* ApplicationManager::scanOneServiceFolder(const std::string& serviceFolderPath)
{
	ServiceDescription* serviceDesc = NULL;

	//TODO: localize it!
	std::string serviceJsonPath = serviceFolderPath + "/services.json";
	serviceDesc = ServiceDescription::fromFile(serviceJsonPath);

	return serviceDesc;
}

//TODO: Need a better mechanism to discover app changes.
// 		FULL new ApplicationDescription objects (icon and all!) for apps it finds on disk. This is also developer error-prone because:
//		added:  this list gets the pointer to a NEW ApplicationDescriptor, which MUST BE DEALLOCATED in the caller (or above)
//		changed: this list gets the pointer to a NEW ApplicationDescriptor, which MUST BE DEALLOCATED in the caller (or above)
//		removed: this list gets the pointer to an EXISTING ApplicationDescriptor, which will be deallocated somewhere in the caller (or above) when the
//					app is completely removed
void ApplicationManager::discoverAppChanges(std::vector<ApplicationDescription *>& added,std::vector<ApplicationDescription *>& removed,std::vector<ApplicationDescription *>& changed) {
	//DANGER: temporal non-safety; apps may change state after the lists are generated. Call under proper locks

	//gather up the current view of apps from "what's on the disk" perspective, into a new vector
	std::map<std::string,ApplicationDescription *> onDiskApps;

	std::string appFolder;
	std::vector<std::string>::iterator appFolderIter = Settings::LunaSettings()->lunaAppsPaths.begin();
	while (appFolderIter !=  Settings::LunaSettings()->lunaAppsPaths.end()) {
		appFolder = *appFolderIter;
		if (appFolder.size()) {

			if (appFolder[appFolder.size() - 1] != '/')
				appFolder += "/";

			luna_log(sAppMgrChnl, "scanning apps from %s", appFolder.c_str());
			scanApplicationsFolders(appFolder,onDiskApps);
		}
		appFolderIter++;
	}

	//map to help differentiate unchanged vs newly added apps
	std::map<std::string,ApplicationDescription *> unchangedApps;
	//now compare against already registered apps
	std::vector<ApplicationDescription *>::iterator it = m_registeredApps.begin();
	while (it != m_registeredApps.end()) {
		ApplicationDescription *pAppDesc = *it;
		if (!pAppDesc) continue;

		//is it in the app list and not on disk?
		if (getAppById(pAppDesc->id(),onDiskApps) == NULL) {
			//Yes...this means it was removed. Goes into remove list
			removed.push_back(pAppDesc);
		} else {
			//No (it is on-disk and in the registered apps list)...this means it's added, changed info, or totally unchanged .
			//add to the temp map to be able to differentiate later
			unchangedApps[pAppDesc->id()] = pAppDesc;
		}
		it++;
	}

	//run through the onDiskApps map and lookup against unchangedApps map. Anything that fails lookup is a newly added app
	std::map<std::string,ApplicationDescription *>::iterator map_iter = onDiskApps.begin();

	while (map_iter != onDiskApps.end()) {
		ApplicationDescription *pAppDesc = map_iter->second;
		std::map<std::string,ApplicationDescription *>::const_iterator find_it = unchangedApps.find(pAppDesc->id());
		if (find_it == unchangedApps.end())
			added.push_back(pAppDesc);
		else if (pAppDesc->strictCompare(*(find_it->second)) == false) {
			//failed strict comparison, which means it somehow changed on disk (March.09.2009 - means only that its appinfo.json changed)
			changed.push_back(pAppDesc);
		}
		else
			delete pAppDesc;		//not needed...this represents unchanged app
		map_iter++;
	}
}

bool ApplicationManager::removePendingApp(const std::string& id)
{
	MutexLocker locker(&m_mutex);

	std::vector<ApplicationDescription*>::iterator it, itEnd;
	for (it = m_pendingApps.begin(), itEnd = m_pendingApps.end(); it != itEnd; ++it) {

		ApplicationDescription* appDesc = *it;
		if (appDesc->id() == id) {
			g_warning("%s: successfully removed '%s' from the set of pending applications", __PRETTY_FUNCTION__, id.c_str());
			delete appDesc;
			m_pendingApps.erase(it);
			return true;
		}
	}
	g_warning("%s: '%s' was not found in the set of pending applications", __PRETTY_FUNCTION__, id.c_str());
	return false;
}

bool ApplicationManager::removePackage( const std::string& id,int cause)
{
	PackageDescription* packageDesc = getPackageInfoByPackageId(id);
	if (!packageDesc)
		return false;

	std::vector<std::string>::const_iterator appIdIt, appIdItEnd;
	for (appIdIt = packageDesc->appIds().begin(), appIdItEnd = packageDesc->appIds().end(); appIdIt != appIdItEnd; ++appIdIt) {
		if (removeApp(*appIdIt, cause)) {
			serviceInstallerUninstallApp(*appIdIt, sServiceInstallerTypeApplication, Settings::LunaSettings()->appInstallBase);
		}
	}

	std::vector<std::string>::const_iterator serviceIdIt, serviceIdItEnd;
	for (serviceIdIt = packageDesc->serviceIds().begin(), serviceIdItEnd = packageDesc->serviceIds().end(); serviceIdIt
			!= serviceIdItEnd; ++serviceIdIt) {
		ServiceDescription* serviceDesc = getServiceInfoByServiceId(*serviceIdIt);
		m_registeredServices.erase(*serviceIdIt);
		delete serviceDesc;
		serviceInstallerUninstallApp(*serviceIdIt, sServiceInstallerTypeService, Settings::LunaSettings()->appInstallBase);
	}

	m_registeredPackages.erase(id);
	delete packageDesc;

	return true;
}

bool ApplicationManager::removeSysApp(const std::string& id)
{
	//remove from the registered applications, and the launchpoints
	std::vector<ApplicationDescription *>::iterator it = m_registeredApps.begin();
	LaunchPointList launchPoints;
	ApplicationDescription * pAppDesc=NULL;
	while (it !=  m_registeredApps.end()) {

		pAppDesc = *it;
		g_warning("%s",pAppDesc->id().c_str());
		if (pAppDesc->id() == id) {
			pAppDesc->launchPoints(launchPoints);
			for (LaunchPointList::iterator lpit = launchPoints.begin();lpit != launchPoints.end();lpit++) {
				const LaunchPoint *pLpoint = *lpit;
				if (pLpoint->isDefault()) {
					postLaunchPointChange(pLpoint, "removed");
					pAppDesc->removeLaunchPoint(pLpoint);
				}
				else {
					std::string erc;
					if (this->removeLaunchPoint(pLpoint->id(),erc) == false)
						g_message("%s: can't remove launchpoint: %s\n",__FUNCTION__,erc.c_str());
				}
			}

			// possibly removing a pending update
			removePendingApp(id);

			//notify of app removal to the ApplicationInstaller
			//(this is necessary because apps can be removed with the app installer *OR* via the launcher ( coming through this function)
			// ApplicationInstaller maintains subscriptions for app installs and removals
			ApplicationInstaller::instance()->notifyAppRemoved(pAppDesc->id(),pAppDesc->version(),0);

			// flag this application as hidden to prevent rescanning from picking it back up
			if (pAppDesc->isUserHideable()) {
				hideApp(pAppDesc->id());
			}

			delete pAppDesc;
			return true;
		}		//break out here if we're guaranteed that there are no duplicate app ids in the list
		else
			it++;
	}

	it = m_pendingApps.begin();
	while (it != m_pendingApps.end()) {

		pAppDesc = *it;
		if (pAppDesc->id() == id) {
			m_pendingApps.erase(it);
			postLaunchPointChange(pAppDesc->getDefaultLaunchPoint(), "removed");
			ApplicationInstaller::instance()->notifyAppRemoved(pAppDesc->id(),pAppDesc->version(),0);
			delete pAppDesc;
			return true;
		}
		it++;
	}

	return false;
}

bool ApplicationManager::removeApp( const std::string& appId,int cause)
{
	//remove from the registered applications, and the launchpoints
	std::vector<ApplicationDescription *>::iterator it = m_registeredApps.begin();
	LaunchPointList launchPoints;
	ApplicationDescription * pAppDesc=NULL;
	while (it !=  m_registeredApps.end()) {
		pAppDesc = *it;
		if (pAppDesc->id() == appId) {
			//lock it against execution (shouldn't be an issue since no thread can really interrupt here, but I'll do it anyways)
			pAppDesc->executionLock();
			pAppDesc->flagForRemoval();	//not needed but it helps in debugging later, in case any of this fn fails
			it = m_registeredApps.erase(it);

			WebAppMgrProxy::instance()->sendAsyncMessage(new View_Mgr_KillApp(pAppDesc->id()));

			pAppDesc->launchPoints(launchPoints);
			for (LaunchPointList::iterator lpit = launchPoints.begin();lpit != launchPoints.end();lpit++) {
				const LaunchPoint *pLpoint = *lpit;
				if (pLpoint->isDefault()) {
					postLaunchPointChange(pLpoint, "removed");
					pAppDesc->removeLaunchPoint(pLpoint);
				}
				else {
					std::string erc;
					if (this->removeLaunchPoint(pLpoint->id(),erc) == false)
						g_message("ApplicationManager::removeApp(): can't remove launchpoint: %s\n",erc.c_str());
				}
			}

			// possibly removing a pending update
			removePendingApp(appId);

			//notify of app removal to the ApplicationInstaller
			//(this is necessary because apps can be removed with the app installer *OR* via the launcher ( coming through this function)
			// ApplicationInstaller maintains subscriptions for app installs and removals
			ApplicationInstaller::instance()->notifyAppRemoved(pAppDesc->id(),pAppDesc->version(),cause);

			// flag this application as hidden to prevent rescanning from picking it back up
			if (pAppDesc->isUserHideable()) {
				hideApp(pAppDesc->id());
			}

			delete pAppDesc;
			return true;
		}		//break out here if we're guaranteed that there are no duplicate app ids in the list
		else
			it++;
	}

	it = m_pendingApps.begin();
	while (it != m_pendingApps.end()) {

		pAppDesc = *it;
		if (pAppDesc->id() == appId) {
			m_pendingApps.erase(it);
			postLaunchPointChange(pAppDesc->getDefaultLaunchPoint(), "removed");
			ApplicationInstaller::instance()->notifyAppRemoved(pAppDesc->id(),pAppDesc->version(),cause);
			delete pAppDesc;
			return true;
		}
		it++;
	}

	return false;
}

void ApplicationManager::launchBootTimeApps()
{
	MutexLocker locker(&m_mutex);

	const std::set<std::string>& appsToLaunchAtBoot = Settings::LunaSettings()->appsToLaunchAtBoot;

	for (std::vector<ApplicationDescription*>::iterator it = m_registeredApps.begin();
	it != m_registeredApps.end(); ++it) {

		ApplicationDescription* app = *it;
		if (appsToLaunchAtBoot.find(app->id()) != appsToLaunchAtBoot.end()) {
			luna_log(sAppMgrChnl, "Launching headless app: %s (%s)",
					app->id().c_str(), app->entryPoint().c_str());
			WebAppMgrProxy::instance()->launchBootTimeApp(app->id().c_str());
		}
	}
}

bool ApplicationManager::isLaunchAtBootApp(const std::string& appId)
{
	MutexLocker locker(&m_mutex);
	const std::set<std::string>& appsToLaunchAtBoot = Settings::LunaSettings()->appsToLaunchAtBoot;
	return (appsToLaunchAtBoot.find(appId) != appsToLaunchAtBoot.end());

}

std::string ApplicationManager::addLaunchPoint(const std::string& id,
		const std::string& title,
		const std::string& menuName,
		const std::string& icon,
		const std::string& params,
		const bool removable)
{
	MutexLocker locker(&m_mutex);

	g_message("ApplicationManager::addLaunchPoint");
	ApplicationDescription* appDesc = getAppById(id);
	if (!appDesc ||
		appDesc->isRemoveFlagged())
		return "";

	std::string launchPointFolder = Settings::LunaSettings()->lunaLaunchPointsPath;
	if (!launchPointFolder.size()) {
		luna_warn(sAppMgrChnl, "Launch Point Folder path not set");
		return "";
	}

	if (launchPointFolder[launchPointFolder.size()-1] != '/')
		launchPointFolder += "/";

	// We need a unique filename
	std::string lpId = findUniqueFileName(launchPointFolder);

	launchPointFolder += lpId;

	LaunchPoint* lp = new LaunchPoint(appDesc, id, lpId, title, menuName, icon, params,removable);

	json_object* json = lp->toJSON();

	bool success = json_object_to_file((char*)launchPointFolder.c_str(), json) != -1;
	if (json && !is_error(json))
		json_object_put(json);

	if (!success) {
		luna_warn(sAppMgrChnl, "Failed to write file: %s", launchPointFolder.c_str());
		delete lp;
		return "";
	}

	appDesc->addLaunchPoint(lp);

	postLaunchPointChange(lp, "added");

	return lpId;
}

bool ApplicationManager::removeLaunchPoint(const std::string& launchPointId,std::string& extendedReturnCause)
{
	MutexLocker locker(&m_mutex);

	// Make sure we are not trying to remove a "default" launchPoint
	if (!isNumber(launchPointId))
		return false;

	const LaunchPoint* lp = getLaunchPointById(launchPointId);
	if (!lp) {
		extendedReturnCause = std::string("launch point ["+launchPointId+"] not found");
		return false;
	}

	//if the launch point it marked not removable, then exit out
	if (lp->isRemovable() == false) {
		extendedReturnCause = std::string("launch point ["+launchPointId+"] not marked non-removable");
		return false;
	}

	std::string launchPointFolder = Settings::LunaSettings()->lunaLaunchPointsPath;
	if (!launchPointFolder.size()) {
		luna_warn(sAppMgrChnl, "Launch Point Folder path not set");
		extendedReturnCause = std::string("launch point folder not set");
		return false;
	}

	if (launchPointFolder[launchPointFolder.size() - 1] != '/')
		launchPointFolder += "/";

	launchPointFolder += launchPointId;

	// First delete the LP file
	if (!deleteFile(launchPointFolder.c_str())) {
		extendedReturnCause = std::string("launch point deletion failed");
		return false;
	}

	postLaunchPointChange(lp, "removed");

	ApplicationDescription* appDesc = lp->appDesc();
	appDesc->removeLaunchPoint(lp);

	return true;
}

bool ApplicationManager::updateLaunchPointIcon(const std::string& launchPointId, const std::string& newIconPath)
{
	MutexLocker lock(&m_mutex);

	bool success = false;
	LaunchPoint* lp = const_cast<LaunchPoint*>(getLaunchPointById(launchPointId));
	if (!lp)
		return success;

	success = lp->updateIconPath(newIconPath);
	if (success) {
		postLaunchPointChange(lp, "updated");
	}
	return success;
}

std::string ApplicationManager::findUniqueFileName(const std::string& folder)
{
	static bool randSeeded = false;
	if (!randSeeded) {

		struct timeval tv;
		gettimeofday(&tv, 0);

		srand(tv.tv_sec * 1000000 + tv.tv_usec);
		randSeeded = true;
	}

	const int len = 16;
	char numStr[len];
	int num;
	std::string filePath;
	struct stat stBuf;

	while (true) {

		// Random number between 1 and 1000000
		num = 1 + (int) (1000000.0 * (rand() / (RAND_MAX + 1.0)));

		snprintf(numStr, len, "%08d", num);

		g_message("Checking file: %s", numStr);

		filePath = folder + numStr;

		if (stat(filePath.c_str(), &stBuf) == 0)
			continue; // File exists

		return std::string(numStr);
	}
}

bool ApplicationManager::isNumber(const std::string& str) const
{
	if (str.empty())
		return false;

	for (unsigned int i = 0; i < str.size(); i++) {
		if (str[i] < '0' || str[i] > '9')
			return false;
	}

	return true;
}

//const RedirectHandler* ApplicationManager::cmdHandler(const std::string& url) {
//
//	if (url.empty())
//		return NULL;
//
//	MutexLocker locker(&m_mutex);
//
//	for( std::list<RedirectHandler*>::const_iterator it=m_cmdHandlerTable.begin(); it != m_cmdHandlerTable.end(); ++it )
//	{
//		const RedirectHandler * p_handler = *it;		//just makes it easier...
//
//		if (NULL != p_handler && p_handler->matches(url)) {
//			return p_handler;
//		}
//	}
//
//	return NULL;
//}

bool ApplicationManager::isValidMimeType( const std::string& mime )
{
	return !mime.empty() && strcasecmp(mime.c_str(), "unknown") != 0;
}

/**
 * Lots of servers (specifically web servers) will sometimes lie about the
 * mime type of a resource and return a "generic" one for all or some resource
 * types.
 */
bool ApplicationManager::isGenericMimeType( const std::string& mime )
{
	return !mime.empty() && (strcasecmp(mime.c_str(), "text/plain") == 0 || strcasecmp(mime.c_str(), "application/octet-stream"));
}

/**
 * Is the scheme of a url "file"?
 */
bool ApplicationManager::urlSchemeIsFile( const std::string& url )
{
	return !url.empty() && strncasecmp(url.c_str(), "file://", 7) == 0;
}

/**
 * Return the content type portion of the MIME type.
 *
 * @param mime The full MIME type.
 *
 * @note Assumes the first part of the MIME type is the content type. i.e.:
 *       application/x-executable, for GNU/Linux 2.6.8, dynamically linked (uses shared libs), not stripped
 *       text/x-c; charset=us-ascii
 *       text/html
 *
 * @return The content type portion of the MIME type, or an empty string if none could be
 *         found.
 *
 * @see http://en.wikipedia.org/wiki/MIME
 */
std::string ApplicationManager::getContentType(const std::string& mime)
{
	std::string content(mime);
	const char* const delims = ";,";

	if (!mime.empty()) {
		for (const char* delim = delims; *delim != '\0'; delim++) {
			int pos = mime.find(*delim);
			if (pos > 0) {
				content = mime.substr(0, pos);
				break;
			}
		}
	}

	return content;
}

/**
 * Determine the full MIME type for the specified file.
 *
 * @param filepath The file path to determine the MIME type for.
 *
 * @return The determined MIME type or an empty string if none could be determined.
 *
 * @note You may want to call getContentType on the return value of this function as it
 *       may return a longer MIME type (i.e full) than you expect.
 *
 * @see http://en.wikipedia.org/wiki/MIME
 */
std::string ApplicationManager::deriveMimeTypeFromFileMagic( const std::string& localFilePath )
{
	std::string	mime;
#ifdef USE_LIBMAGIC
	if (!localFilePath.empty()) {
		magic_t cookie = ::magic_open(MAGIC_MIME);
		if (cookie) {
			magic_load(cookie, NULL);
			const char* pszMime = magic_file(cookie, localFilePath.c_str());
			if (NULL != pszMime) {
				mime = pszMime;
			}
			magic_close(cookie);
		}
	}
#endif
	return mime;
}

bool cmptitle::operator()(const LaunchPoint* lp1, const LaunchPoint* lp2) const
{
	return lp1->compareByKeys(lp2) < 0;
}

void ApplicationManager::searchLaunchPoints(SearchSet& matchedByTitle, SearchSet& matchedByKeyword,
		const std::string& searchTerm) const
		{
	if (searchTerm.empty())
		return;

	matchedByTitle.clear();
	matchedByKeyword.clear();

	std::vector<ApplicationDescription*>::const_iterator appDescIt = m_registeredApps.begin();
	std::vector<ApplicationDescription*>::const_iterator appDescEndIt = m_registeredApps.end();

	gchar* lcSearchTerm = g_utf8_strdown(searchTerm.c_str(), -1);

	for (; appDescIt != appDescEndIt; ++appDescIt) {

		ApplicationDescription* appDesc = *appDescIt;
		if (!appDesc)
			continue;

		if (!appDesc->isVisible() || appDesc->isRemoveFlagged())
			continue;

		if (!hardwareFeaturesRequirementSatisfied(appDesc->hardwareFeaturesNeeded()))
			continue;

		LaunchPointList::const_iterator lpIt = appDesc->launchPoints().begin();
		LaunchPointList::const_iterator lpEndIt = appDesc->launchPoints().end();

		for (; lpIt != lpEndIt; ++lpIt) {

			bool foundMatch = false;
			const LaunchPoint* lp = *lpIt;
			if (!lp)
				continue;

			if (lp->matchesTitle(lcSearchTerm)) {
				g_message("found match by title: %s", lp->title().c_str());
				matchedByTitle.insert(lp);
			}
			else if (lp->isDefault() && !foundMatch) {
				// whole/partial keyword starts with search term?
				if (searchTerm.size() >= 3 && Settings::LunaSettings()->usePartialKeywordAppSearch) {
					foundMatch = appDesc->doesMatchKeywordPartial(lcSearchTerm);
				}
				else {
					foundMatch  = appDesc->doesMatchKeywordExact(lcSearchTerm);
				}
				// menu name starts with search term?
				if (!foundMatch) {
					gchar* lcMenuName = g_utf8_strdown(appDesc->menuName().c_str(), -1);
					foundMatch = g_str_has_prefix(lcMenuName, lcSearchTerm);
					g_free(lcMenuName);
				}

				if (foundMatch) {
					g_message("found match by keyword/appmenu: %s", lp->title().c_str());
					matchedByKeyword.insert(lp);
				}
			}
		}
	}

	if (lcSearchTerm)
		g_free(lcSearchTerm);
}

std::string	ApplicationManager::mimeTableAsJsonString()
{
	std::vector<std::pair<std::string,std::vector<std::string> > > r_resourceTableString;
	std::vector<std::pair<std::string,std::vector<std::string> > > r_redirectTableString;
	MimeSystem::instance()->dbg_getResourceTableStrings(r_resourceTableString);
	MimeSystem::instance()->dbg_getRedirectTableStrings(r_redirectTableString);

	for (std::vector<std::pair<std::string,std::vector<std::string> > >::iterator outer_it = r_resourceTableString.begin();
	outer_it != r_resourceTableString.end();++outer_it)
	{
		for (std::vector<std::string>::iterator inner_it = outer_it->second.begin();inner_it != outer_it->second.end();++inner_it)
		{
			if (inner_it == outer_it->second.begin()) {
				printf("mimeType: [%s] , primary: %s\n",outer_it->first.c_str(),(*inner_it).c_str());
			}
			else
				printf("\t\t%s\n",(*inner_it).c_str());
		}
	}
	printf("\n\n");
	for (std::vector<std::pair<std::string,std::vector<std::string> > >::iterator outer_it = r_redirectTableString.begin();
	outer_it != r_redirectTableString.end();++outer_it)
	{
		for (std::vector<std::string>::iterator inner_it = outer_it->second.begin();inner_it != outer_it->second.end();++inner_it)
		{
			if (inner_it == outer_it->second.begin()) {
				printf("url: [%s] , primary: %s\n",outer_it->first.c_str(),(*inner_it).c_str());
			}
			else
				printf("\t\t%s\n",(*inner_it).c_str());
		}
	}

	return MimeSystem::instance()->allTablesAsJsonString();
}

// support of whitelist removed
ApplicationDescription* ApplicationManager::checkAppAgainstWhiteList( ApplicationDescription* appDesc )
{
	return appDesc;
}


bool ApplicationManager::isTrustedInstallerApp (const std::string& app) const {
	if (app == "com.palm.app.findapps" || app == "com.palm.app.firstuse"  || app == "com.palm.app.updates")
		return true;

	return false;
}

bool ApplicationManager::isTrustedPalmApp(const ApplicationDescription* appDesc) const {
	return (appDesc->id().find("com.palm.") == 0 && ((appDesc->vendorName().find("Palm") == 0) || (appDesc->vendorName().find("HP") == 0))) ? true : false;
}

bool ApplicationManager::isTrustedPalmApp(const std::string& appId)
{
	ApplicationDescription * appDesc = getAppById(appId);
	if (!appDesc)
	{
		return false;
	}
	return isTrustedPalmApp(appDesc);
}

bool ApplicationManager::isFactoryPlatformApp(const std::string& appId)
{
	return isTrustedPalmApp(appId);
}

unsigned long ApplicationManager::generateNewTicket()
{
	return s_ticketGenerator++;
}

bool ApplicationManager::cbAppInstallServiceConnection(LSHandle* lshandle, LSMessage* msg, void* user_data)
{
	json_object* payload = 0;
	const char* str = LSMessageGetPayload(msg);
	if (!str)
		return true;

	payload = json_tokener_parse(str);
	if (!payload || is_error(payload))
		return true;

	if (json_object_get_boolean(json_object_object_get(payload, "connected"))) {
		LSError lserror;
		LSErrorInit(&lserror);

		if (!LSCall(ApplicationManager::instance()->m_serviceHandlePrivate,
					"palm://com.palm.appInstallService/status", "{\"subscribe\":true}",
					ApplicationManager::cbApplicationStatusCallback, NULL, NULL, &lserror)) {
			LSErrorFree(&lserror);
		}
	}

	json_object_put(payload);
	return true;
}

bool ApplicationManager::cbApplicationStatusCallback(LSHandle* lshandle, LSMessage* msg, void* user_data)
{
	ApplicationManager::instance()->handleApplicationStatusUpdates(msg);
	return true;
}

void ApplicationManager::handleApplicationStatusUpdates(LSMessage* msg)
{
	MutexLocker locker(&m_mutex);

	static bool initialStatus = false;
	const char* str = 0;
	json_object* payload = NULL;
	json_object* key = NULL;

	str = LSMessageGetPayload(msg);
	if (!str) {
		g_warning("%s [INSTALLER]: blank payload", __FUNCTION__);
		goto Done;
	}

	payload = json_tokener_parse(str);
	if (!payload || is_error(payload)) {
		g_warning("%s: [INSTALLER] invalid payload", __FUNCTION__);
		goto Done;
	}

//	qDebug() << "Trace: " << QString(str);
	key = json_object_object_get(payload, "status");
	if (key && json_object_is_type(key,json_type_object)) {
		key = json_object_object_get(key, "apps");
		if (key && json_object_is_type(key, json_type_array) && !initialStatus) {
			// this is the first status response
			int length = json_object_array_length(key);
			for (int i=0; i<length; i++) {
				json_object* item = json_object_array_get_idx(key, i);
				if (item) {
					ApplicationStatus appStatus(item);
					// do we care about it? (canceled items are never returned by the first response)
					if (appStatus.state != ApplicationStatus::State_Unknown) {
						ApplicationDescription* appDesc = getPendingAppById(appStatus.id);
						bool appExists = getAppById(appStatus.id) != 0;
						if (appDesc) {
							// update status
							appDesc->update(appStatus, appExists);
							QBitArray statusBits = QBitArray(LaunchPointUpdatedReason::SIZEOF);
							statusBits.setBit(LaunchPointUpdatedReason::Status);
							statusBits.setBit(LaunchPointUpdatedReason::Icon);
							Q_EMIT signalLaunchPointUpdated(appDesc->getDefaultLaunchPoint(),statusBits);
						}
						else {
							// brand new item
							appDesc = ApplicationDescription::fromApplicationStatus(appStatus, appExists);
							if (appDesc)
							{
								m_pendingApps.push_back(appDesc);
							}
						}
					}
				}
			}
			g_message("%s: processed initial status update (pending list now has %d elements)", __FUNCTION__, m_pendingApps.size());
			initialStatus = true;
		}
	}
	else {
		// this is a singular update
		ApplicationStatus appStatus(payload);
		ApplicationDescription* existingAppDesc = getAppById(appStatus.id);
		bool appExists = existingAppDesc != 0;

		if (appStatus.state == ApplicationStatus::State_Canceled) {
			g_debug("%s [INSTALLER]: ApplicationStatus::State_Canceled", __FUNCTION__);
			// revert an applications pending status
			ApplicationDescription* appDesc = getPendingAppById(appStatus.id);
			if (appDesc) {

				if (existingAppDesc) {
					// revert back to the existing
					QBitArray statusBits = QBitArray(LaunchPointUpdatedReason::SIZEOF);
					statusBits.setBit(LaunchPointUpdatedReason::Status);
					Q_EMIT signalLaunchPointUpdated(existingAppDesc->getDefaultLaunchPoint(),statusBits);
				}
				else {
					// tell those interested that the item is about to get removed
					QBitArray statusBits = QBitArray(LaunchPointRemovedReason::SIZEOF);
					statusBits.setBit(LaunchPointRemovedReason::InstallerStatusUpdate);
					Q_EMIT signalLaunchPointRemoved(appDesc->getDefaultLaunchPoint());
				}
				// clean up our pending item
				removePendingApp(appStatus.id);
			}
		}
		else if (appStatus.state != ApplicationStatus::State_Unknown) {

			g_debug("%s [INSTALLER]: ApplicationStatus State = %d", __FUNCTION__,(int)(appStatus.state));
			ApplicationDescription* appDesc = getPendingAppById(appStatus.id);
			if (appDesc) {
				// update status
				g_debug("%s [INSTALLER]: ApplicationStatus State = %d - updating status of an installing app", __FUNCTION__,(int)(appStatus.state));
				appDesc->update(appStatus, appExists);
				QBitArray statusBits = QBitArray(LaunchPointUpdatedReason::SIZEOF);
				statusBits.setBit(LaunchPointUpdatedReason::Status);
				statusBits.setBit(LaunchPointUpdatedReason::Progress);
				Q_EMIT signalLaunchPointUpdated(appDesc->getDefaultLaunchPoint(),statusBits);
			}
			else {
				appDesc = ApplicationDescription::fromApplicationStatus(appStatus, appExists);
				if (appDesc) {
					g_debug("%s [INSTALLER]: ApplicationStatus State = %d - initial status of an installing app", __FUNCTION__,(int)(appStatus.state));
					m_pendingApps.push_back(appDesc);
					QBitArray statusBits = QBitArray(LaunchPointAddedReason::SIZEOF);
					statusBits.setBit(LaunchPointAddedReason::InstallerStatusUpdate);
					Q_EMIT signalLaunchPointAdded(appDesc->getDefaultLaunchPoint(),statusBits);
				}
			}
		}
	}

Done:

	if (payload && !is_error(payload))
		json_object_put(payload);
}

bool ApplicationManager::cbDownloadManagerUpdate (LSHandle* lshandle, LSMessage* msg, void* user_data)
{
	std::string errMsg;
	LSError lserror;
	LSErrorInit (&lserror);
	const char* payloadStr = NULL;
	struct json_object* payload = NULL;
	struct json_object* ticketField = NULL;
	struct json_object* completedField = NULL;
	struct json_object* targetField = NULL;
	ResourceHandler resourceHandler;
	std::string ticket, target, processId, ticketStr;
	std::string guessedMime;
	std::string resourceExtension;

	ApplicationManager::DownloadRequest* req;

	LSMessageToken token = LSMessageGetResponseToken (msg);

	if (!user_data) {
		/* invalid update, ignore */
		errMsg = "bad user_data, invalid update, no msg to reply to, ignoring this update";
		return true;
	}

	req = (ApplicationManager::DownloadRequest*)user_data;

	g_debug ("%s:%d download request: ticket = %lu ovrHandlerAppId = %s strMime = %s\n", __FILE__, __LINE__,
			req->m_ticket, req->m_overrideHandlerAppId.c_str(), req->m_mime.c_str());

	if (!req || !req->m_ticket) {
		g_warning ("garbage user data, ignoring this msg");
		return true;
	}

	payloadStr = LSMessageGetPayload (msg);
	if (!payloadStr) {
		g_warning (" %s: invalid message, no payload", __PRETTY_FUNCTION__);
		return true;
	}
	g_debug ("%s:%d payloadStr = %s\n", __FILE__, __LINE__, payloadStr);

	payload = json_tokener_parse (payloadStr);
	if (!payload || is_error (payload)) {
		g_warning ("%s: invalid message, could not convert this to json %s", __PRETTY_FUNCTION__, payloadStr);
		return true;
	}

	ticketField = json_object_object_get (payload, (char*)"ticket");
	if (!ticketField) {
		g_warning ("%s: invalid update, no ticket in payload %s", __PRETTY_FUNCTION__, payloadStr);
		goto done;
	}

	ticket = json_object_get_string (ticketField);
	if (ticket.empty()) {
		g_warning ("%s: invalid update, invalid ticket in payload %s", __PRETTY_FUNCTION__, payloadStr);
		goto done;
	}

	// replacing download ticket with app manager/installer ticket
	// is there a better way to do this?
	json_object_object_del (payload, const_cast<char*>("ticket"));
	ticketStr = toSTLString<unsigned long> (req->m_ticket);
	json_object_object_add (payload, "ticket", json_object_new_string (ticketStr.c_str()));
	g_debug ("Updated ticket in progress msg - %s", json_object_to_json_string (payload));

	ApplicationManager::instance()->relayStatus (std::string (json_object_to_json_string (payload)), req->m_ticket);


	g_debug ("%s:%d checking for completed \n", __FILE__, __LINE__);
	completedField = json_object_object_get (payload, (char *) "completed");
	if (!completedField) {
		g_debug ("only a progress updated, no completion state yet, return.");
		/* this is probably a progress update, ok to return here */
		goto done;
	}
	else if (json_object_get_boolean (completedField) == false) {
		g_warning ("download failed");
		/* download failed but failure msg has already gone to the app as part of the forwarding
		 * if the app didn't subscribe, it will never know that the download failed
		 */
		goto cleanup;
	}
	else  {
		g_debug ("download manager has completed successfully, lets launch");

		targetField = json_object_object_get (payload, (char *) "target");
		if (!targetField) {
			errMsg = "No target field on the msg though completed is true, download manager bug";
			goto in_error;
		}

		target = json_object_get_string (targetField);
		if (isAppPackage (target.c_str())) {
			if (!ApplicationInstaller::instance()->install (target,0, req->m_ticket)) {
				errMsg = "installation of ipkg failed";
				goto in_error;
			}
		}
		else if (!req->m_overrideHandlerAppId.empty()) {
			g_warning ("%s:%d launching with overrideHandlerAppId %s target %s\n", __FILE__, __LINE__, req->m_overrideHandlerAppId.c_str(), target.c_str());
			std::string processId = WebAppMgrProxy::instance()->appLaunch(req->m_overrideHandlerAppId, "{ \"target\" : \"" + target + "\" }", "", "", errMsg);
			if (processId.empty()) {
				errMsg = "launching app " + req->m_overrideHandlerAppId + " on target " + target + " failed with msg " + errMsg;
				goto in_error;
			}
		}
		else {
			resourceHandler = MimeSystem::instance()->getActiveHandlerForResource(req->m_mime);
			if (resourceHandler.valid() == false) {
				//try getting it by extension
				MimeSystem::getExtensionFromUrl(target,resourceExtension);
				//map to mime type
				MimeSystem::instance()->getMimeTypeByExtension(resourceExtension,guessedMime);
				//try again
				resourceHandler = MimeSystem::instance()->getActiveHandlerForResource(guessedMime);
				if (resourceHandler.valid() == false) {
					errMsg = "no resource handler for target: " + target;
					goto in_error;
				}
			}
			g_debug ("%s:%d launching with resource handler appid %s target %s\n", __FILE__, __LINE__, resourceHandler.appId().c_str(), target.c_str());
			processId = WebAppMgrProxy::instance()->appLaunch(resourceHandler.appId(), " { \"target\" : \"" + target + "\" }","", "", errMsg);
			if (processId.empty()) {
				errMsg = "launching app " + resourceHandler.appId() + " on target " + target + " failed with msg " + errMsg;
				goto in_error;
			}
		}
	}

	in_error:
	if (!errMsg.empty()) {
		g_debug ("detected error msg %s", errMsg.c_str());

		json_object* errorObject = json_object_new_object ();
		json_object_object_add (errorObject, "returnValue", json_object_new_boolean (false));
		json_object_object_add (errorObject, "errMsg", json_object_new_string (errMsg.c_str()));

		ApplicationManager::instance()->relayStatus(std::string(json_object_to_json_string (errorObject)), req->m_ticket);

		json_object_put (errorObject);
	}


	cleanup:

	if (!LSCallCancel(lshandle, token, &lserror)) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree (&lserror);
		return true;
	}

	g_debug ("no more updates from download manager, deleting the download req");
	delete req;

	done:
	json_object_put (payload);
	return true;
}

bool ApplicationManager::isRemoteFile (const char* uri)
{
	if (strncasecmp (uri, "http", 4) == 0)
		return true;
	return false;
}

bool ApplicationManager::isAppPackage(const char* uri)
{
	std::string resName = getResourceNameFromUrl(QUrl(uri));
	std::string::size_type pos = resName.rfind('.');
	if( std::string::npos == pos )
	{
		// TODO : there is no file extension -- open up the file if it's http and see if
		// we get a content-type field back.

		return 0;
	}

	std::string ext = resName.substr( pos+1 );
	//printf("ext = '%s'\n", ext.c_str() );
	std::string ipkext = "ipk";

	if( !strcasecmp(ipkext.c_str(), ext.c_str()))
		return true;

	return false;
}

//static
bool ApplicationManager::getAppEntryPointFromAppinfoFile(const std::string& baseDirOfApp,std::string& r_entryPointPath)
{
	/*
	 * Replicates some of the functionality of scan(). This is necessary though, because the entry point is the basis of the
	 * HTML5 database name. The DB name is required at times before scan() has run (therefore, before ApplicationDescriptions have been generated)
	 *
	 */

	// Do we have a locale setting
	std::string locale = Preferences::instance()->locale();

	// Look for the language specific appinfo.json
	std::string filePath = baseDirOfApp + "/resources/" + locale + "/appinfo.json";

	char * str = readFile(filePath.c_str());
	if (!str || !g_utf8_validate(str, -1, NULL))
	{
		filePath = baseDirOfApp + "/appinfo.json";
		str = readFile(filePath.c_str());
		if (!str || !g_utf8_validate(str, -1, NULL))
		{
			return false;			//still can't find it
		}

	}

	std::string dirPath;
	gchar* dirPathCStr;
	dirPathCStr = g_path_get_dirname(filePath.c_str());
	dirPath = dirPathCStr;
	dirPath += "/";
	g_free(dirPathCStr);

	struct json_object* root=0;
	struct json_object* label=0;

	root = json_tokener_parse( str );
	delete[] str;

	if( !root || is_error( root ) )
	{
		g_warning("%s: couldn't parse [%s] contents into json",__FUNCTION__,filePath.c_str());
		return false;
	}

	// MAIN: mandatory
	label = json_object_object_get(root, "main");
	if( label && !is_error(label) )
	{
		r_entryPointPath = json_object_get_string(label);
	}
	else
	{
		g_warning("%s: no entry point found in [%s]",__FUNCTION__,filePath.c_str());
		json_object_put(root);
		return false;
	}

	if (!strstr(r_entryPointPath.c_str(), "://"))
		r_entryPointPath = std::string("file://") + dirPath + r_entryPointPath;

	json_object_put(root);

	return true;

}

//static
void ApplicationManager::executeLockApp(const std::string& appId,ExecuteLockOperation op)
{
	/*
	 *   A function to flip the "execution lock" bit on an app. This version of the function will work independently of whether of not appmanager has been
	 * 	 init-ed or not, and whether or not a scan() has taken place. Locks will persist across rescans, but not across reboots
	 *
	 *   Call the fn with op = ExecuteLock to execution-lock the app (prevent launches), ExecuteLockAndTerminate to lock the app and then kill all current processes of it
	 * 		and ExecuteUnlock to unlock it
	 *
	 */

	MutexLocker * m = 0;
	//if the appmanager was init-ed, then go actually do a live mod of the app's descriptor
	if (s_instance != NULL)
	{
		s_instance->executeLockAppLoaded(appId,op);
	}
	else {
		m = new MutexLocker(&s_mutexExecLockFunctions);
		switch (op)
		{
		case ApplicationManager::ExecuteLockAndTerminate:
		case ApplicationManager::ExecuteLock:
			s_appExeclockSet.insert(appId);
			break;
		case ApplicationManager::ExecuteUnlock:
			s_appExeclockSet.erase(appId);
			break;
		default: break;
		}
		delete m;
	}
}

void ApplicationManager::executeLockAppLoaded(const std::string& appId,ExecuteLockOperation op)
{
	/*
	 *   A function to flip the "execution lock" bit on an app. This version of the function operates on an initialized appmanager, though it will act like the static version
	 * 		(executeLockApp) if the apps have not yet been scanned
	 *
	 *   Call the fn with op = ExecuteLock to execution-lock the app (prevent launches), ExecuteLockAndTerminate to lock the app and then kill all current processes of it
	 * 		and ExecuteUnlock to unlock it
	 *
	 */

	if (op == ApplicationManager::ExecuteLockAndTerminate)
	{
		MutexLocker * appmutex=new MutexLocker(&m_mutex);
		ApplicationDescription * pAppDesc = getAppById(appId);
		if (pAppDesc)
		{
			//exec lock
			pAppDesc->executionLock();
			//terminate all instances
			WebAppMgrProxy::instance()->sendAsyncMessage(new View_Mgr_KillApp(pAppDesc->id()));
		}
		delete appmutex;
		MutexLocker * m = new MutexLocker(&s_mutexExecLockFunctions);
		s_appExeclockSet.insert(appId);
		delete m;

	}
	else if (op == ApplicationManager::ExecuteLock)
	{
		MutexLocker * appmutex=new MutexLocker(&m_mutex);
		ApplicationDescription * pAppDesc = getAppById(appId);
		if (pAppDesc)
		{
			//exec lock
			pAppDesc->executionLock();
		}
		delete appmutex;
		MutexLocker * m = new MutexLocker(&s_mutexExecLockFunctions);
		s_appExeclockSet.insert(appId);
		delete m;

	}
	else if (op == ApplicationManager::ExecuteUnlock)
	{
		MutexLocker * m = new MutexLocker(&s_mutexExecLockFunctions);
		s_appExeclockSet.erase(appId);
		delete m;
		MutexLocker * appmutex=new MutexLocker(&m_mutex);
		ApplicationDescription * pAppDesc = getAppById(appId);
		if (pAppDesc)
		{
			//exec unlock
			pAppDesc->executionLock(false);
		}
		delete appmutex;
		//if this was a boot time app, re-launch it...
		if (isLaunchAtBootApp(appId))
			WebAppMgrProxy::instance()->launchBootTimeApp(appId.c_str());
	}

}

static bool hardwareFeaturesRequirementSatisfied(uint32_t hardwareFeaturesNeeded)
{
#if !defined(TARGET_DEVICE)
	return true;
#endif

	DeviceInfo* di = DeviceInfo::instance();

	if (G_LIKELY(hardwareFeaturesNeeded == ApplicationDescription::HardwareFeaturesNeeded_None))
		return true;

	if (hardwareFeaturesNeeded & ApplicationDescription::HardwareFeaturesNeeded_Wifi) {
		if (di->wifiAvailable() == false)
			return false;
	}

	if (hardwareFeaturesNeeded & ApplicationDescription::HardwareFeaturesNeeded_Bluetooth) {
		if (di->bluetoothAvailable() == false)
			return false;
	}

	if (hardwareFeaturesNeeded & ApplicationDescription::HardwareFeaturesNeeded_Compass) {
		if (di->compassAvailable() == false)
			return false;
	}

	if (hardwareFeaturesNeeded & ApplicationDescription::HardwareFeaturesNeeded_Accelerometer) {
		if (di->accelerometerAvailable() == false)
			return false;
	}

	return true;
}

void ApplicationManager::slotBuiltInAppEntryPoint_DockMode(const std::string& argsAsStringEncodedJson)
{
	//FIXME: a bit round-about going through system ui controller - kind of like the inner workings "simulating" a user/ui event. Perhaps this can
	//			be replaced with more direct logic

	SystemUiController::instance()->enterOrExitDockModeUi(true);
}

void ApplicationManager::slotBuiltInAppEntryPoint_VoiceDial(const std::string& argsAsStringEncodedJson)
{
	//per guidelines from developers making the voice dial, this needs to make a service call:
	//luna://com.palm.pmvoicecommand/startVoiceCommand '{"source":"appicon"}'

	LSError lserror;
	LSErrorInit(&lserror);

	if (!LSCall(ApplicationManager::instance()->m_serviceHandlePrivate,
			"palm://com.palm.pmvoicecommand/startVoiceCommand", "{\"source\":\"appicon\"}",
			NULL, NULL, NULL, &lserror))
	{
		g_warning("FAILED to contact com.palm.pmvoicecommand/startVoiceCommand");
		LSErrorFree(&lserror);
	}

}

void ApplicationManager::slotBuiltInAppEntryPoint_Launchermode0(const std::string& argsAsStringEncodedJson)
{
	   QString program = "/bin/sh";
	   QStringList arguments;
	   arguments << "/usr/palm/applications/com.palm.sysapp.launchermode0/.system.sh";
	   ///bin/sh /usr/palm/applications/com.palm.sysapp.launchermode0/.system.sh
	   qDebug() << "About to run " << program << " with args: " << arguments;
	   QProcess *myProcess = new QProcess(this);
	   myProcess->start(program, arguments);
}

void ApplicationManager::slotVoiceDialAllowSettingChanged(bool v)
{
	//if the initial scan didn't run yet ( == true; yes it's somewhat backwards), then just adjust the hidden apps list
	if (m_initialScan)
	{
		if (v)
			m_hiddenApps.erase("com.palm.sysapp.voicedial");
		else
			m_hiddenApps.insert("com.palm.sysapp.voicedial");
		return;
	}

	ApplicationDescription * pAppDesc = getAppById("com.palm.sysapp.voicedial");
	if (!pAppDesc)
	{
		//voice dial not "installed"
		if (v)
		{
			//make sure it didn't somehow wrongly get placed into "hidden apps" (as a remnant of a previous op)
			m_hiddenApps.erase("com.palm.sysapp.voicedial");
			//install it...
			if ((pAppDesc = installSysApp("com.palm.sysapp.voicedial")) == NULL)
			{
				g_warning("%s: failed to (re)install the voice dial sysapp",__FUNCTION__);
				return;
			}
		}
		return;
	}
	if (!v)
	{
		removeSysApp("com.palm.sysapp.voicedial");
	}
	else
	{
		//the app is visible and the setting says "visible". It's a no-op case.
	}
}

void ApplicationManager::dbgEmitSignalLaunchPointUpdated(const LaunchPoint * lp,const QBitArray& statusBits)
{
	Q_EMIT signalLaunchPointUpdated(lp,statusBits);
}

//static
QString ApplicationManager::dbgOutputLaunchpointUpdateReasons(const QBitArray& reasons)
{
	if (reasons.size() != LaunchPointUpdatedReason::SIZEOF)
	{
		return QString("not a valid reason bitfield (size is %1 != %2)").arg(reasons.size()).arg((int)LaunchPointUpdatedReason::SIZEOF);
	}

	QString s = "{ ";
	if (reasons.testBit(LaunchPointUpdatedReason::INVALID))
		s.append("INVALID ");
	if (reasons.testBit(LaunchPointUpdatedReason::Status))
		s.append("STATUS ");
	if (reasons.testBit(LaunchPointUpdatedReason::Progress))
		s.append("PROGRESS ");
	if (reasons.testBit(LaunchPointUpdatedReason::Icon))
		s.append("ICON ");
	s.append("}");
	return QString("Reason Bits: ")+s;
}
