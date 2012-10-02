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

#include "ApplicationInstaller.h"
#include "ApplicationInstallerErrors.h"
#include "ApplicationManager.h"
#include "EventReporter.h"
#include "WebAppMgrProxy.h"
#include "ApplicationDescription.h"

#include "BootupAnimation.h"
#include "HostBase.h"
#include "JSONUtils.h"
#include "Logging.h"
#include "Settings.h"
#include "SystemService.h"
#include "Time.h"
#include "Preferences.h"

#undef min
#undef max
#include "Utils.h"
#include "WebAppMgrProxy.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/mount.h>
#include <unistd.h>
#include "cjson/json.h"
#include "cjson/json_util.h"
#include <errno.h>

#include <ftw.h>
#include <dirent.h>
#include <regex.h>
#include <glib.h>
#include <fcntl.h>
#include <algorithm>

#include <QUrl>

#include <PIpcChannel.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "PackageDescription.h"

#define REMOVER_RETURNC__FAILEDIPKGREMOVE			1
#define REMOVER_RETURNC__SUCCESS					0

// estimated size of uncompressed app is this value * x (where x is size of a compressed file/package, etc)
#define INSTALLER_DEFV__MIN_FREE_MULT		2				

// amount to reserve for the app database, for each app, in bytes
#define	INSTALLER_DEFV__APPDB_RESERVE_SPACE_BYTES			(1*1024*1024)

// default amount total reserved for app installation, in bytes	
#define INSTALLER_DEFV__DEFAULT_TOTAL_APP_RESERVE_BYTES		((uint64_t)(100*1024*1024))

// default for allowing downloads of packages to the media partition
#define INSTALLER_DEFV__MIN_FREE_TO_DL_ON_MEDIA_BYTES		((uint64_t)(5*1024*1024))

static void util_LSSubReplyWithRelay_IgnoreError(LSHandle* lshandle,const std::string& subscriptionKey,const unsigned long ticket,const std::string& payload); //fwd decl...this is defined at the end of the src file
static void util_cleanup_tempDir(const std::string& tmpDirPath); //fwd decl...this is defined at the end of the src file
static void util_cleanup_installDir(const std::string& installDirPath); //fwd decl...this is defined at the end of the src file
static void util_ipkgInstallDone (GPid pid, gint status, gpointer data);
static void util_ipkgRemoveDone (GPid pid, gint status, gpointer data);
static gboolean util_ipkgInstallIoChannelCallback(GIOChannel* channel, GIOCondition condition, gpointer arg);
static gboolean util_cryptofsMounted() __attribute__((unused));
static gboolean util_mountCryptofs() __attribute__((unused));
static void util_unmountCryptofs() __attribute__((unused));
static gboolean util_cryptofsPathAvailable() __attribute__((unused));
static void util_validateCryptofs() __attribute__((unused));
static int runScriptCwd(const std::string& scriptFile,const std::string& cwd);

//TODO: these don't need to be vars...they can be #defines ... I need this for now to debug something

static const char*		s_userInstalledPackageDir				= 	"/var/usr/palm/applications";
static const char*		s_oldIpkgBaseDir						=	"/var/usr/lib/ipkg/";	
static const char*		s_oldIpkgStatusFile						=	"/var/usr/lib/ipkg/status";
static const char*		s_newIpkgBaseDir						=	"/media/cryptofs/apps/usr/lib/ipkg/";
static const char*		s_newIpkgStatusFile						=	"/media/cryptofs/apps/usr/lib/ipkg/status";
static const char*		s_postRemoveServiceScript				= 	"/usr/bin/pmServicePostRemove.sh";

static const char*		s_pkginstallerExec 						=	"ipkg";
static const char*		s_pkginstallerOpts_location				=	"-o";
//static const char*		s_pkginstallerOpts_install				=	"install";
static const char*		s_pkginstallerOpts_remove				=	"remove";

static const char *	const s_verifyExec							=	"openssl";
#if defined(TARGET_DEVICE)
static const char * const s_revocationCertFile = "/etc/ssl/certs/pubsubsigning-bundle.crt";
#else
static const char * const s_revocationCertFile = "/etc/ssl/certs/pubsubsigning-bundle.crt";
#endif

static const char * const s_extractKeyOpts[] = { "x509" , "-in" , "-pubkey" , ">"};

static ApplicationInstaller* s_instance = 0;
static const char*    s_logChannel = "ApplicationInstaller";


////                    CLASS STATICS   --------------------------------------------------------------------------------
Mutex		ApplicationInstaller::s_sizeFnMutex;
std::string	ApplicationInstaller::s_sizeFnBaseDir;
uint64_t		ApplicationInstaller::s_sizeFnAccumulator = 0;
uint64_t		ApplicationInstaller::s_auxSizeFnAccumulator = 0;
uint64_t		ApplicationInstaller::s_targetFsBlockSize = 0;
json_object * ApplicationInstaller::s_manifestJobj = 0;
std::string  ApplicationInstaller::s_installer_version 	= 	"1.0.0";
	
std::list<CommandParams*> ApplicationInstaller::s_commandParams;
json_object * ApplicationInstaller::dbg_statxfs_persistent;
	
statfsfn ApplicationInstaller::s_statfsFn = ::statfs;
statvfsfn ApplicationInstaller::s_statvfsFn = ::statvfs;
std::map<uint32_t,std::pair<uint32_t,uint32_t> > ApplicationInstaller::dbg_statfs_map;
std::map<uint32_t,std::pair<uint32_t,uint32_t> > ApplicationInstaller::dbg_statvfs_map;
	
//// --------------------------------------------------------------------------------------------------------------------

/*! \page com_palm_appinstaller Service API com.palm.appinstaller/
 *  Public methods:
 *  - \ref com_palm_appinstaller_get_user_installed_app_sizes
 *  - \ref com_palm_appinstaller_query_install_capacity
 *  - \ref com_palm_appinstaller_install
 *  - \ref com_palm_appinstaller_install_no_verify
 *  - \ref com_palm_appinstaller_notify_on_change
 *  - \ref com_palm_appinstaller_remove
 *  - \ref com_palm_appinstaller_revoke
 */
/* TODO: These should be documented, but were not available on current emulator
 *       images. Check with a newer image.
 *  - \ref com_palm_appinstaller_install_progress_query
 *  - \ref com_palm_appinstaller_is_installed
 */
static LSMethod s_methods[]  = {
	{ "installProgressQuery",     	ApplicationInstaller::cbInstallProgressQuery },
	{ "install",					ApplicationInstaller::cbInstall },
	{ "installNoVerify",			ApplicationInstaller::cbInstallNoVerify },
	{ "remove",						ApplicationInstaller::cbRemove },
	{ "revoke",						ApplicationInstaller::cbRevoke },
	{ "isInstalled",				ApplicationInstaller::cbIsInstalled },
	{ "notifyOnChange",				ApplicationInstaller::cbNotifyOnChange},
	{ "getUserInstalledAppSizes",	ApplicationInstaller::cbGetSizes},
	{ "queryInstallCapacity",		ApplicationInstaller::cbQueryInstallCapacity},
	{ "dbg_getpackageinfofromstatusfile",	ApplicationInstaller::cbDbgGetPkgInfoFromStatusFile},
	{ "dbg_fakefssize",				ApplicationInstaller::cbDbgFakeFsSize},
	{ "dbg_unfakefssizes",			ApplicationInstaller::cbDbgUnFakeFsSizes},
	{ "dbg_getfssize",				ApplicationInstaller::cbGetFsSize},
	{ "dbg_fillsize",				ApplicationInstaller::cbDbgFillSize},
	{ "dbg_getappsizeonfs",			ApplicationInstaller::cbDbgGetAppSizeOnFs},
    { 0, 0 },
};


ApplicationInstaller* ApplicationInstaller::instance()
{
	if( !s_instance )
	{
		s_instance = new ApplicationInstaller();
		s_instance->init();
	}
	
	return s_instance;
}

ApplicationInstaller::ApplicationInstaller()
	: m_inBrickMode(false)
	, m_service (NULL)
{
}

ApplicationInstaller::~ApplicationInstaller()
{
	this->stopService();
}

bool ApplicationInstaller::init()
{
	// DEBUG ... LOAD FAKEFS SIZES
	dbg_restoreFakeFsEntriesAtStartup();
	
	// Do not bring up com.palm.appInstaller in minimal mode.
	if (Settings::LunaSettings()->uiType == Settings::UI_MINIMAL)
	    return true;

	//initialize us as a luna service
	
	g_mkdir_with_parents((Settings::LunaSettings()->appInstallBase + std::string("/") + Settings::LunaSettings()->appInstallRelative).c_str(),0755);
	this->startService();
	
	//now register with pubsub for revocation messages...this is done first by registering a call for notification on when pubsub joins/leaves lunabus
	LSError lsError;
	LSErrorInit(&lsError);
	
	if (!LSCall(m_service,
			"palm://com.palm.bus/signal/registerServerStatus",
			"{\"serviceName\":\"com.palm.pubsubservice\", \"subscribe\":true}",
			ApplicationInstaller::cbPubSubStatus, NULL,NULL, &lsError)) {
		g_warning("ApplicationInstaller::init(): Failed to register with pubsub for status!");
		LSErrorFree(&lsError);
	}

	connect(SystemService::instance(), SIGNAL(signalMediaPartitionAvailable(bool)), this, SLOT(slotMediaPartitionAvailable(bool)));

	return true;
}


void ApplicationInstaller::startService()
{
	bool result;
    LSError lsError;
    LSErrorInit(&lsError);

	GMainLoop *mainLoop = HostBase::instance()->mainLoop();

	luna_log(s_logChannel, "ApplicationInstaller (service) starting...");
   
    result = LSRegister("com.palm.appinstaller", &m_service, &lsError);
	if (!result)
		goto Done;

    result = LSRegisterCategory(m_service, "/", s_methods, NULL, NULL, &lsError);
    if (!result)
		goto Done;

    result = LSGmainAttach(m_service, mainLoop, &lsError);
    if (!result)
		goto Done;
	
Done:

	if (!result) {
		luna_critical(s_logChannel, "Failed in ApplicationInstaller: %s", lsError.message);
		LSErrorFree(&lsError);
	}
	else {
		luna_log(s_logChannel, "ApplicationInstaller on service bus");
	}
}

void ApplicationInstaller::stopService()
{
    LSError lsError;
    LSErrorInit(&lsError);
    bool result;

	result = LSUnregister(m_service, &lsError);
    if (!result)
        LSErrorFree(&lsError);

	m_service = 0;
}

static void util_ipkgInstallDone (GPid pid, gint status, gpointer data) 
{
    InstallParams* installParams = (InstallParams*)data;
    bool success = true;
    std::string message;
	std::string ls_sub_key = toSTLString<long>(installParams->ticketId);
	std::string ls_payload;

    g_warning ("%s: Step 4: child pid %d done with status %d",__PRETTY_FUNCTION__,  pid, status);
    if (isNonErrorProcExit((int)status) == false) {

		message = std::string();
		switch (WEXITSTATUS(status)) {
		case AI_ERR_INTERNAL:
		case AI_ERR_INVALID_ARGS:
			message = "FAILED_INTERNAL_ERROR"; break;
		case AI_ERR_INSTALL_FAILEDUNPACK:
			message = "FAILED_CREATE_TMP"; break;
		case AI_ERR_INSTALL_NOTENOUGHTEMPSPACE:
			message = "FAILED_NOT_ENOUGH_TEMP_SPACE";  break;
		case AI_ERR_INSTALL_NOTENOUGHINSTALLSPACE:
			message = "FAILED_NOT_ENOUGH_INSTALL_SPACE";  break;
		case AI_ERR_INSTALL_TARGETNOTFOUND:
			message = "FAILED_PACKAGEFILE_NOT_FOUND"; break;
		case AI_ERR_INSTALL_BADPACKAGE:
			message = "FAILED_PACKAGEFILE_CORRUPT"; break;
		case AI_ERR_INSTALL_FAILEDVERIFY:
			message = "FAILED_VERIFY"; break;
		case AI_ERR_INSTALL_FAILEDIPKGINST:
			message = "FAILED_IPKG_INSTALL"; break;
		default:
			g_warning("util_ipkgInstallDone(): unknown return code %d (%d)", WEXITSTATUS(status), status);
			break;
		}

		if (!message.empty()) {
			ls_payload = std::string("{ \"ticket\":") +ls_sub_key
						 +std::string(" , \"status\":\"")+message+std::string("\"")
						 +std::string(" }");
			util_LSSubReplyWithRelay_IgnoreError(installParams->_lshandle,ls_sub_key,installParams->ticketId,ls_payload);
			g_message("relaying install failure: %s", ls_payload.c_str());
		}			

		message = "FAILED_IPKG_INSTALL";
		success = false;
    }
    else {
		message = "SUCCESS";
		success = true;
    }

	ls_payload = std::string("{ \"ticket\":") +ls_sub_key
				 +std::string(" , \"status\":\"")+message+std::string("\"")
				 +std::string(" }");

    g_warning ("%s: sending subscription response %s", __PRETTY_FUNCTION__,ls_payload.c_str());
    //update the "chained" subscriptions that came off the application manager/download manager
    util_LSSubReplyWithRelay_IgnoreError(installParams->_lshandle,ls_sub_key,installParams->ticketId,ls_payload);

    if (success) {
		g_warning ("%s: successful ipkg install of package '%s', rescanning apps",
				   __PRETTY_FUNCTION__, installParams->_packageId.c_str());

		json_object * packageIdJo = ApplicationInstaller::instance()->packageInfoFileToJson(installParams->_packageId);
		if (packageIdJo)
		{
			std::string folderPath = Settings::LunaSettings()->appInstallBase + std::string("/") + Settings::LunaSettings()->packageInstallRelative + std::string("/")+installParams->_packageId+std::string("/");
			ApplicationManager::instance()->postInstallScan(packageIdJo,folderPath);
			json_object_put(packageIdJo);
		}
		else
		{
			g_warning("%s: FAILED TO PARSE PACKAGE INFO FILE FOR INSTALLED PACKAGE [%s] - possibly an old style package. Trying alternate scan function...",__FUNCTION__,installParams->_packageId.c_str());
			ApplicationManager::instance()->postInstallScan(installParams->_packageId);
		}
    }

    if (Settings::LunaSettings()->debug_appInstallerCleaner & 1)
    {
    	g_warning ("%s: unlinking the file %s", __PRETTY_FUNCTION__, installParams->_target.c_str());
    	unlink (installParams->_target.c_str());
    }

    g_warning ("%s: closing process id %d", __PRETTY_FUNCTION__, pid);
    g_spawn_close_pid (pid);

	ApplicationInstaller::instance()->oneCommandProcessed();
}

static void util_ipkgRemoveDone (GPid pid, gint status, gpointer data)
{
    RemoveParams* removeParams = (RemoveParams*)data;
    bool success = true;
    std::string message;

    g_warning ("%s: Step 3: child pid %d done with status %d",__PRETTY_FUNCTION__,  pid, status);
    if (isNonErrorProcExit ((int)status) == false) {
		message = "FAILED_IPKG_REMOVE";
		success = false;
    } else {
		message = "SUCCESS";
		success = true;

		EventReporter::instance()->report( "uninstall", removeParams->_packageName.c_str() );
		//also remove the app dir, because ipkg doesn't do a complete job
		if (removeParams->_packageName.size()) {   //check the package name size (at least 1 char) for safety or else the whole app subtree can be deleted!!!
			std::string installPathFull = Settings::LunaSettings()->appInstallBase +
									std::string("/") + Settings::LunaSettings()->appInstallRelative +
									std::string("/") + removeParams->_packageName +
									std::string(".app");
			util_cleanup_installDir(installPathFull);
			installPathFull = Settings::LunaSettings()->appInstallBase +
					std::string("/") + Settings::LunaSettings()->appInstallRelative +
					std::string("/") + removeParams->_packageName +
					std::string(".service");
			util_cleanup_installDir(installPathFull);
			installPathFull = Settings::LunaSettings()->appInstallBase +
								std::string("/") + Settings::LunaSettings()->appInstallRelative +
								std::string("/") + removeParams->_packageName;
			util_cleanup_installDir(installPathFull);
		}
    	g_warning ("%s: successful ipkg remove", __PRETTY_FUNCTION__);
    }
    std::string ls_sub_key = toSTLString<long>(removeParams->ticketId);
    std::string ls_payload = std::string("{ \"ticket\":") +ls_sub_key
	+std::string(" , \"status\":\"")+message+std::string("\"")
	+std::string(" }");

    g_warning ("%s: sending subscription response %s", __PRETTY_FUNCTION__,ls_payload.c_str());
    //update the "chained" subscriptions that came off the application manager/download manager
    util_LSSubReplyWithRelay_IgnoreError((LSHandle*) removeParams->_lshandle, ls_sub_key, removeParams->ticketId, ls_payload);

    g_warning ("%s: closing process id %d", __PRETTY_FUNCTION__, pid);
    g_spawn_close_pid (pid);

	ApplicationInstaller::instance()->oneCommandProcessed();
}

static gboolean util_ipkgInstallIoChannelCallback(GIOChannel* channel, GIOCondition condition, gpointer arg)
{
	InstallParams* params = (InstallParams*) arg;
	if (!params)
		return false;

	GString* str = g_string_new("");
	GError* error = 0;
	
	GIOStatus status = g_io_channel_read_line_string(channel, str, NULL, &error);
	if (status == G_IO_STATUS_NORMAL) {
		g_message("%s:%d Got status message from child: %s\n",
				  __PRETTY_FUNCTION__, __LINE__, str->str);

		if (str->str && strncmp(str->str, "status:", 7) == 0) {

			// This is our status message
			gchar** strArray = g_strsplit(str->str, " ", 0);
			if (strArray) {

				int count = 0;
				while (strArray[count] != 0) {
					count++;
				}

				g_debug("string split into count: %d", count);

				// strArray[0] -> "status:"
				// strArray[1] -> stage: starting, unpacking, verifying, installing, done
				// strArray[2] -> appid (only in the installing stage)
				if (count > 1) {
					std::string status = g_strstrip(strArray[1]);
					std::string payload;
					std::string ls_sub_key = toSTLString<long>(params->ticketId);

					if (status == "starting") {
						payload = std::string("{ \"ticket\":")
								  + ls_sub_key
								  + std::string(" , \"status\":\"STARTING\" }");
					}
					else if (status == "unpacking") {
						payload = std::string("{ \"ticket\":")
								  + ls_sub_key
								  + std::string(" , \"status\":\"CREATE_TMP\" }");
					}
					else if (status == "verifying") {
						payload = std::string("{ \"ticket\":")
								  + ls_sub_key
								  + std::string(" , \"status\":\"VERIFYING\" }");
					}
					else if (status == "installing") {
						payload = std::string("{ \"ticket\":")
								  + ls_sub_key
								  + std::string(" , \"status\":\"IPKG_INSTALL\" }");
					}
					else if (status == "spacecalculation") {

						if (count > 2) {
							std::string spaceNeededStr;
							spaceNeededStr = g_strstrip(strArray[2]);
						
							payload = std::string("{ \"ticket\":")
									  + ls_sub_key
									  + std::string(" , \"status\":\"SPACE_CALCULATION\" "
													" , \"spaceNeeded\":");
							payload += spaceNeededStr + " }";
						}
					}
						
					if (!payload.empty()) {

						util_LSSubReplyWithRelay_IgnoreError(params->_lshandle, ls_sub_key,
															 params->ticketId, payload);
						g_message("relaying install status: %s", payload.c_str());
					}
				}
				
				if (count > 2) {
					params->_packageId = g_strstrip(strArray[2]);

					// AppId is known. nuke any running instances
					// FIXME
				}

				g_strfreev(strArray);
			}

		}
	}
	else {
		if (error) {
			g_critical("%s:%d: Failed to read from child\'s stdout pipe: %s",
					   __PRETTY_FUNCTION__, __LINE__, error->message);
			g_error_free(error);
		}
	}

	g_string_free(str, TRUE);
	
	return true;
}

static gboolean util_cryptofsMounted()
{
	gchar* contents = 0;
	gsize length = 0;

	gboolean res = g_file_get_contents("/proc/mounts", &contents, &length, NULL);
	if (!res || !contents || !length)
		return FALSE;

	gchar* entry = g_strrstr(contents, "cryptofs");
	res = (entry != NULL);

	g_free(contents);

	return res;	
}

static gboolean util_mountCryptofs()
{
	g_message("Mounting cryptofs");
	int ret = ::system("mountcfs");
	return isNonErrorProcExit(ret);
}

static void util_unmountCryptofs()
{
	g_message("Unmounting cryptofs");
	::umount("/media/cryptofs");
}

static gboolean util_cryptofsPathAvailable()
{
	struct stat stBuf;
	if (::stat("/media/internal/.palm", &stBuf) != 0)
		return false;

	if (!S_ISDIR(stBuf.st_mode))
		return false;

	return true;
}

static void util_validateCryptofs()
{
	bool cryptofsMounted = util_cryptofsMounted();
	bool cryptofsPathAvailable = util_cryptofsPathAvailable();

	if (!cryptofsMounted || !cryptofsPathAvailable) {
		g_critical("Cryptofs gone after MSM mode: mounted: %s, path available: %s",
				   cryptofsMounted ? "true" : "false",
				   cryptofsPathAvailable ? "true" : "false");

		util_unmountCryptofs();
		bool ret = util_mountCryptofs();
		if (!ret) {
			g_critical("Failed to mount cryptofs");
			return;
		}

		g_mkdir_with_parents((Settings::LunaSettings()->appInstallBase + std::string("/") +
							  Settings::LunaSettings()->appInstallRelative).c_str(),0755);
		Settings::LunaSettings()->createNeededFolders();
		ApplicationManager::instance()->scan();

		if (!cryptofsPathAvailable)
			SystemService::instance()->postAppRestoredNeeded();			
	}
	else {
		g_message("Cryptofs fine after MSM mode exit");
	} 
}

static void util_deleteJail(const char* appId)
{
	int rc;
	std::string cmdbuf;
	cmdbuf.append("/usr/bin/jailer -D -i ");
	cmdbuf.append(appId);
	rc = ::system(cmdbuf.c_str());
	if (rc < 0) {
		g_critical("Can't delete jail");
	}
	else {
		g_message("jail delete call ok");
	}
}

////---------------------------------------------------- LUNA BUS SERVICE FUNCTIONS ---------------------------------------------------------------------------------------

bool ApplicationInstaller::lunasvcInstallProgressQuery(LSHandle* lshandle, LSMessage *msg,void *user_data) {
	return false;
}

int ApplicationInstaller::lunasvcRemove(RemoveParams *removeParams)
{

	///AS OF JUNE 2010, PACKAGE ID IS NO LONGER == TO APPID. PACKAGES CONTAIN A SINGLE APP (WITH A DIFFERENT ID) AND IN THE FUTURE MAY CONTAIN MULTIPLE APPS

	std::string packageName = removeParams->_packageName;
	LSHandle * lshandle = (LSHandle * )removeParams->_lshandle;
	unsigned long ticket = removeParams->ticketId;
	int cause = removeParams->_cause;
	std::string baseDir = Settings::LunaSettings()->appInstallBase;
	bool suppressIpkgRemove=false;
	gchar * g_stdoutBuffer = NULL; g_stdoutBuffer = 0;		//suppress warnings
	gchar * g_stderrBuffer = NULL; g_stderrBuffer = 0;

	std::string ls_sub_key = toSTLString<long>(ticket);
	std::string ls_payload;
	std::string appBasePath = Settings::LunaSettings()->appInstallBase;
	std::string packageBasePath = Settings::LunaSettings()->packageInstallBase;

	ls_payload = std::string("{ \"ticket\":")
	+ls_sub_key
	+std::string(" , \"status\":\"IPKG_REMOVE\"")
	+std::string(" }");

	util_LSSubReplyWithRelay_IgnoreError(lshandle,ls_sub_key,ticket,ls_payload);

	if (packageName.size() == 0) {
		return REMOVER_RETURNC__FAILEDIPKGREMOVE;
	}

	//get the list of all apps and services for this package
	std::vector<std::string> packageAppsList;
	std::vector<std::string> packageServicesList;
	PackageDescription * pPkgDesc = ApplicationManager::instance()->getPackageInfoByPackageId(packageName);

	//FOR EASE OF DEBUG - no copy needed otherwise, so remove the locals and use directly. This will have to change if the assumptions on the lifetime of the package desc. pointer ever change
	packageAppsList = pPkgDesc->appIds();
	packageServicesList = pPkgDesc->serviceIds();

	//do app specific removal things on all the app ids
	for (int i=0;i<(int)packageAppsList.size();++i)
	{
		ApplicationDescription * appDesc = ApplicationManager::instance()->getAppById(packageAppsList.at(i));;
		if (appDesc == NULL)
			continue;			///weird, not in the app list. probably a pretty serious error but I'll let the system trip over it later

		if (appDesc->isUserHideable())
		{
			if ((packageAppsList.size() > 1) || (packageServicesList.size() > 1))
			{
				//if any of the app ids is "user hideable" and there are other apps or services in the same package , then it isn't really removable.
				//This used to be supported for things like doc viewer that were really in ro partitions and couldn't be removed physically
				// It will only still work if it's the only app in the package
				g_warning("%s: failing the remove [%s] because the package has more than 1 app and/or service and one of them is marked user hideable (appid = [%s] is user hideable)",
						__FUNCTION__,packageName.c_str(),appDesc->id().c_str());
				return REMOVER_RETURNC__FAILEDIPKGREMOVE;
			}
			else
				suppressIpkgRemove=true;
		}

		// REMOVE THE APP'S JAIL
		util_deleteJail(packageName.c_str());

		// REMOVE THE APP'S DATABASES
		WebAppMgrProxy::instance()->sendAsyncMessage(new View_Mgr_DeleteHTML5Database(appDesc->entryPoint()));
	}
	
	std::string packageFolderPath = packageBasePath+std::string("/")+Settings::LunaSettings()->packageInstallRelative+std::string("/")+packageName;
	bool oldPackageStyle = pPkgDesc->isOldStyle();		//have to do this now because once removePackage is called, all bets are off
	ApplicationManager::instance()->removePackage(packageName,cause);
	
	//run the pre-remove script
	//FIXME: the script dir is expressed in 2 different ways in 2 different places: here (and Settings.cpp) and in the ApplicationInstallerUtility project (in Main.cpp)

	std::string preRemoveScriptFile = packageBasePath+std::string("/.scripts/")+packageName+std::string("/pmPreRemove.script");
	std::string cwdPath;
	//if old style package (meaning there is no package dir, then run the preremove script CWD to the app dir). If new style, CWD to package dir
	if (oldPackageStyle)
		cwdPath = Settings::LunaSettings()->appInstallBase+std::string("/")+Settings::LunaSettings()->appInstallRelative+std::string("/")+packageName;		//packageName == appid in this case, since it's an old style package
	else
		cwdPath = packageFolderPath;
	
	g_warning("%s: Trying to run preRemove script - %s",__FUNCTION__,preRemoveScriptFile.c_str());
	if (doesExistOnFilesystem(preRemoveScriptFile.c_str()))
	{
		int sr = runScriptCwd(preRemoveScriptFile.c_str(),cwdPath);
		g_warning("%s: Script ran - resultcode = %d",__FUNCTION__,sr);
	}
	else {
		g_warning("%s: Couldn't find %s on filesys",__FUNCTION__,preRemoveScriptFile.c_str());
	}
	
	//remove the script dir (if it doesn't exist, no foul -- but as a coarse safety, if the base path is "", then don't do this
	if (packageBasePath != "")
		util_cleanup_tempDir(packageBasePath+std::string("/.scripts/")+packageName);
	
	//	REMOVE THE IPK (THIS WILL REMOVE IT FROM THE DISK)
	if (suppressIpkgRemove) {

		// the pre remove script already ran (if it existed), and the script dir is gone so there is no way to rerun the pre remove script later.
		// conceivably, ro partition apps wouldn't have a pre remove script so this would all be ok

		// attempting an ipkg remove would fail + we don't really want to remove these from the system
		g_idle_add_full(G_PRIORITY_HIGH_IDLE, cbShallowRemove, (gpointer)removeParams, NULL);
		return REMOVER_RETURNC__SUCCESS;
	}

	//e.g. ipkg -o <installbasepath> remove <ipk pathandfilename>
	
	GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD);
	gchar * argv[8] = {0};
	argv[0] = (gchar *)s_pkginstallerExec; 
	argv[1] = (gchar *)s_pkginstallerOpts_location;
	argv[2] = (gchar *)(packageBasePath.c_str());
	argv[3] = (gchar *)s_pkginstallerOpts_remove;
	argv[4] = (gchar *)packageName.c_str();
	argv[5] = NULL;
	GPid childPid;

	g_warning("Step 1: executing: %s %s %s %s %s",argv[0],argv[1],argv[2],argv[3],argv[4]);
	
	GError * gerr = NULL;
	gboolean resultStatus = g_spawn_async(NULL,
			argv,
			NULL,
			flags,
			NULL,
			NULL,
			&childPid,
			&gerr);

	g_warning("ApplicationInstaller::lunasvcRemove(): Step 1: ipkg resultStatus = %d ",(int)resultStatus);
	if (resultStatus == false && gerr) {
		g_warning("ApplicationInstaller::lunasvcRemove(): Step 1: error: resultStatus was 0, error was %s, returning REMOVER_RETURNC__FAILEDIPKGREMOVE", gerr->message);
		g_error_free(gerr);
		//failed to exec ipk package remove command
		return REMOVER_RETURNC__FAILEDIPKGREMOVE;
	}
	else {
		setpriority(PRIO_PROCESS, childPid, IPKG_PROCESS_PRIORITY);

		m_cmdState.processing = true;
		m_cmdState.pid = childPid;
		m_cmdState.sourceId = g_child_watch_add_full(G_PRIORITY_HIGH_IDLE, childPid,
													 util_ipkgRemoveDone, removeParams, NULL);
		
	    g_warning ("ApplicationInstaller::lunasvcRemove(): Step 2: added watch on child pid %d", childPid);
	}
	
	//call the service removal script. Should really only be done if the package had one or more services, but it won't hurt for now to call it regardless
	g_warning("%s: Trying to run preRemove script - %s",__FUNCTION__,preRemoveScriptFile.c_str());
	if (doesExistOnFilesystem(s_postRemoveServiceScript))
	{
		int sr = runScriptCwd(s_postRemoveServiceScript,"/");
		g_debug("%s: Script ran - resultcode = %d",__FUNCTION__,sr);
	}
	else {
		g_warning("%s: Couldn't find %s on filesys",__FUNCTION__,s_postRemoveServiceScript);
	}

	//remove the app manifest
	std::string appManifestPath = Settings::LunaSettings()->packageManifestsPath + std::string("/")+packageName+std::string(".pmmanifest");
	unlink(appManifestPath.c_str());			//even if packageName is somehow == "", this will be harmless as it will try to unlink a <whatever>/.pmmanifest file (non existent)

	return REMOVER_RETURNC__SUCCESS;
}

gboolean ApplicationInstaller::cbShallowRemove(gpointer param)
{
	RemoveParams* removeParams = (RemoveParams*)param;
	std::string ls_sub_key = toSTLString<long>(removeParams->ticketId);
	std::string ls_payload = "{\"ticket\":"+ls_sub_key+", \"status\":\"SUCCESS\"}";

	EventReporter::instance()->report("uninstall", removeParams->_packageName.c_str());
	util_LSSubReplyWithRelay_IgnoreError((LSHandle*)removeParams->_lshandle, ls_sub_key, removeParams->ticketId, ls_payload);

	ApplicationInstaller::instance()->oneCommandProcessed();

	return FALSE;
}

#define LINEBUFFERSIZE		4*1024

bool ApplicationInstaller::lunasvcIsInstalled(LSHandle* lshandle, LSMessage *msg,void *user_data) 
{
	
	//TODO: should report on whether the package is installed or not. Waiting on mapping from appId<-->pkg name (Sept.29.08)
	return false;
}

bool ApplicationInstaller::lunasvcNotifyOnChange(const std::string& packageName,LSHandle * lshandle,LSMessage *msg) 
{
	
	//TODO: report on the current status of the packageName
	return false;
}

bool ApplicationInstaller::lunasvcGetInstalledSizes(LSHandle * lshandle,LSMessage *msg)
{

	std::vector<std::pair<std::string,uint64_t> > appList;
	getAllUserInstalledAppSizes(appList,Settings::LunaSettings()->appInstallBase);
	
	std::string response("{ \"returnValue\":true , \"apps\":[ ");
	bool first=true;
	uint64_t total=0;
	for (std::vector<std::pair<std::string,uint64_t> >::iterator it = appList.begin();
		it != appList.end();
		++it)
	{
		if (!first)
			response.append(",");
		else
			first=false;
		
		response.append(std::string("{ \"appName\":\"")+it->first+std::string("\", \"size\":")+toSTLString<uint64_t>(it->second)+std::string("}"));
		total += it->second;
	}
	response.append(" ] , \"totalSize\":"+toSTLString<uint64_t>(total)+std::string(" } "));
	
	LSError lserror;
	LSErrorInit(&lserror);
	const char* r = response.c_str();
	if (!LSMessageReply( lshandle, msg, r, &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	return true;
}

/// WARNING! these are tied to the app catalog's notion of what the various codes mean. Don't change arbitrarily
#define INSTALLER_DEFV__RC_QUERYINSTALLCAP_DOWNLOADSPACEINSUFFICIENT	1
#define INSTALLER_DEFV__RC_QUERYINSTALLCAP_INSTALLSPACEINSUFFICIENT		2

int ApplicationInstaller::lunasvcQueryInstallCapacity(const std::string& packageId,uint64_t packageSizeInKB,uint64_t uncompressedPackageSizeInKB,uint64_t& r_spaceNeededInKB)
{
	int rc = 0;
	g_warning("%s: called with packageId = [%s] , packageSizeInKB = %llu , uncompressedPackageSizeInKB = %llu",__FUNCTION__,packageId.c_str(),packageSizeInKB,uncompressedPackageSizeInKB);
	
	if (uncompressedPackageSizeInKB == 0)
		uncompressedPackageSizeInKB = packageSizeInKB * INSTALLER_DEFV__MIN_FREE_MULT;   //guess at it
	
	g_mkdir_with_parents (Settings::LunaSettings()->downloadPathMedia.c_str(), 755);

	//check size on download fs
	uint64_t downloadFsBlockSize = 0;
	uint64_t downloadFsFreeSpaceInBlocks = getFsFreeSpaceInBlocks(Settings::LunaSettings()->downloadPathMedia,&downloadFsBlockSize);
	if (downloadFsBlockSize == 0)
	{
		g_warning("%s: Using download Fs blocksize = 1 (because getFsFreeSpaceInBlocks() returned it as 0)",__FUNCTION__);
		downloadFsBlockSize = 1;
	}
	//check size on install fs (usually the same, but may be different in the future)
	uint64_t installFsBlockSize = 0;
	uint64_t installFsFreeSpaceInBlocks = getFsFreeSpaceInBlocks(Settings::LunaSettings()->appInstallBase,&installFsBlockSize);
	if (installFsBlockSize == 0)
	{
		g_warning("%s: Using install Fs blocksize = 1 (because getFsFreeSpaceInBlocks() returned it as 0)",__FUNCTION__);
		installFsBlockSize = 1;
	}
	
	uint64_t packageSizeInBytes = (uint64_t)packageSizeInKB << 10;
	uint64_t packageSizeInFsBlocks = (packageSizeInBytes) / downloadFsBlockSize + 1;		//avoid modulus and just add 1; most probable and error is negligible and on the side of safety
	uint64_t uncompressedSizeInBytes = (uint64_t)uncompressedPackageSizeInKB << 10;
	uint64_t uncompressedSizeInFsBlocks = (uncompressedSizeInBytes) / (installFsBlockSize) + 1;	//   ""

	if (packageSizeInFsBlocks > downloadFsFreeSpaceInBlocks)
		rc |= INSTALLER_DEFV__RC_QUERYINSTALLCAP_DOWNLOADSPACEINSUFFICIENT;
	
	uint64_t totalSizeNeededForPackageInInstallFsBlocks = 0;
	
	//check to see if the package is already installed on the installFs   --- ONLY THE ALREADY MIGRATED APPS WILL HIT HERE
	
	uint64_t alreadyInstalledPackageSizeInBytes = 0;
	uint64_t alreadyInstalledPackageSizeInFsBlocks = 0;
		
	if (findUserInstalledAppName(packageId,Settings::LunaSettings()->packageInstallBase))
	{
		alreadyInstalledPackageSizeInBytes = getSizeOfPackageById(packageId);
		alreadyInstalledPackageSizeInFsBlocks = alreadyInstalledPackageSizeInBytes / installFsBlockSize +1;
		if (alreadyInstalledPackageSizeInFsBlocks > uncompressedSizeInFsBlocks)
		{
			alreadyInstalledPackageSizeInFsBlocks = uncompressedSizeInFsBlocks;
			g_warning("%s: [WEIRD-NESS]: Apparently, the already installed package is smaller in size (%llu blocks) vs the new package uncompressed (%llu blocks)",
						__FUNCTION__,alreadyInstalledPackageSizeInFsBlocks,uncompressedSizeInFsBlocks);
		}
	}
	else 
	{
		g_warning("%s: Package %s is not installed on the %s fs",__FUNCTION__,packageId.c_str(),Settings::LunaSettings()->packageInstallBase.c_str());
	}
	
	if (arePathsOnSameFilesystem(Settings::LunaSettings()->downloadPathMedia,Settings::LunaSettings()->packageInstallBase))
	{
		//if yes, then the amount needed for install is:  package size + uncompressed size 
		totalSizeNeededForPackageInInstallFsBlocks = packageSizeInFsBlocks + uncompressedSizeInFsBlocks - alreadyInstalledPackageSizeInFsBlocks;
		g_warning("%s: Total size required for the package is: package size (%llu blocks) + uncompressed size (%llu blocks) - already installed size (%llu blocks) ==> %llu blocks total",
				__FUNCTION__,packageSizeInFsBlocks,uncompressedSizeInFsBlocks,alreadyInstalledPackageSizeInFsBlocks,totalSizeNeededForPackageInInstallFsBlocks);
	}
	else
	{
		//if no, then the amount needed for install is: uncompressed size
		totalSizeNeededForPackageInInstallFsBlocks = uncompressedSizeInFsBlocks - alreadyInstalledPackageSizeInFsBlocks;
		g_warning("%s: Total size required for the package is: uncompressed size (%llu blocks) - already installed size (%llu blocks) ==> %llu blocks total",
				__FUNCTION__,uncompressedSizeInFsBlocks,alreadyInstalledPackageSizeInFsBlocks,totalSizeNeededForPackageInInstallFsBlocks);
	}

	if (totalSizeNeededForPackageInInstallFsBlocks > installFsFreeSpaceInBlocks)	{
		g_warning("%s: Not enough free space on install fs; it has only %llu blocks, and %llu blocks are needed"
				,__FUNCTION__,
				installFsFreeSpaceInBlocks,
				totalSizeNeededForPackageInInstallFsBlocks);
		rc |= INSTALLER_DEFV__RC_QUERYINSTALLCAP_INSTALLSPACEINSUFFICIENT;
	}
	
	uint64_t totalSizeForPackageInBytes = ((uint64_t)totalSizeNeededForPackageInInstallFsBlocks * (uint64_t)installFsBlockSize);
	
	//warning! chance of overflow coming up next...
	uint64_t installSpaceNeeded= (totalSizeForPackageInBytes);
	r_spaceNeededInKB = installSpaceNeeded >> 10;		//to KB
	if ((r_spaceNeededInKB == 0) && installSpaceNeeded)
		r_spaceNeededInKB = 1;		//round up if less than 1K
	
	g_warning("%s: \t\t ===> Total size needed in bytes for package install = %llu (~%llu KB)  [%llu for new package]",
			__FUNCTION__,
			installSpaceNeeded,
			r_spaceNeededInKB,
			totalSizeForPackageInBytes);
		
	return rc;
}

/*
 * utility function to extract a package name ( which is the same as an app id) from a control.tar.gz file, which was embedded in the app package (IPK file)
 * 
 * takes the full path to the control.tar.gz file, as well as a path to a directory to use as temp space
 * 
 * if successful, returns true, in which case return_PackageName has the name of the package
 * 
 */
//static
bool ApplicationInstaller::packageNameFromControl(const std::string& controlTarGzPathAndFile,const std::string& tempDir,std::string& return_PackageName)
{

	FILE * fp = 0;
	std::string controlPathAndFile;
	int success=false;
	char linebuffer[2048];			//hopefully no line is bigger than this in the control file
	char * line=0;
	std::string parseLine;
	std::string key;
	std::string value;
	std::string::size_type marker = 0;
	const std::string innerTmpDir = tempDir + std::string("/controlTmp");
	int exit_status = g_mkdir_with_parents(innerTmpDir.c_str(),755);
	if (exit_status == -1)
		return false;		//do not go to Cleanup, as there is nothing to cleanup
	
	gchar * argv[6];
	gchar * envp[2];
	GError * gerr = NULL;
	
	GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL);

	argv[0] = (gchar*)"tar"; 
	argv[1] = (gchar*)"-xz";
	argv[2] = (gchar*)"--no-same-owner";
	argv[3] = (gchar*)"-f";
	argv[4] = (gchar*)controlTarGzPathAndFile.c_str();
	argv[5] = NULL;

	std::string envCWD = std::string("PWD=")+innerTmpDir;
	envp[0] = (gchar *)envCWD.c_str();
	envp[1] = NULL;

	g_warning("ApplicationInstaller::packageNameFromControl(): executing: %s %s %s",argv[0],argv[1],argv[2]);

	gboolean resultStatus = g_spawn_sync(innerTmpDir.c_str(),
			argv,
			envp,
			flags,
			NULL,
			NULL,
			NULL,
			NULL,
			&exit_status,
			&gerr);

	if (gerr) {
		g_warning("ApplicationInstaller::packageNameFromControl(): error: %s",gerr->message);
		g_error_free(gerr);
		gerr=NULL;
		goto Cleanup;
	}

	if (resultStatus) {
		if (isNonErrorProcExit((int)exit_status) == false) {
			g_warning("ApplicationInstaller::packageNameFromControl(): error: result status is %d , error code = %d",resultStatus,exit_status);
			goto Cleanup;
		}
	}
	else {
		//failed to exec unpack command
		g_warning("ApplicationInstaller::packageNameFromControl(): error: spawn failed");
		goto Cleanup;
	}

	//open the "control" file
	controlPathAndFile = innerTmpDir+std::string("/control");
	fp = fopen(controlPathAndFile.c_str(),"r");
	if (!fp) {
		//try "CONTROL"
		g_warning("ApplicationInstaller::packageNameFromControl(): couldn't open [%s]...trying alternate...",controlPathAndFile.c_str());
		controlPathAndFile = innerTmpDir+std::string("/CONTROL");
		fp = fopen(controlPathAndFile.c_str(),"r");
		if (!fp) {
			g_warning("ApplicationInstaller::packageNameFromControl(): couldn't open [%s]",controlPathAndFile.c_str());
			goto Cleanup;
		}
	}

	while (!feof(fp)) {
		line = fgets(linebuffer,2048,fp);
		if (line == NULL)
			continue;
		parseLine = std::string(line);
		if ((marker = parseLine.find(':')) == std::string::npos)
			continue;
		key = parseLine.substr(0,marker);
		if ((marker = parseLine.find_first_not_of(" \t",marker+1)) == std::string::npos)
			continue;
		value = parseLine.substr(marker);
		
		//trim whitespace from both key and value
		key = trimWhitespace(key);
		value = trimWhitespace(value);
		
		g_warning("ApplicationInstaller::packageNameFromControl(): key = [%s] , value = [%s]",key.c_str(),value.c_str());
		
		if (key == std::string("Package")) {
			return_PackageName = value;
			success=true;
			g_warning("ApplicationInstaller::packageNameFromControl(): Found the magic Package key! value = [%s]",value.c_str());
			break;
		}
	} //end while !feof
Cleanup:

	if (fp)
		fclose(fp);
	//remove the temp directory
	util_cleanup_tempDir(innerTmpDir);
	return success;
}

/*
 * Utility function to extract the names of all packages (apps) that were user installed (i.e. in /var)
 * returns number of app names found
 * 
 */
//static
int ApplicationInstaller::getAllUserInstalledAppNames(std::vector<std::string>& appList,std::string basePkgDirName) 
{
	gchar * g_stdoutBuffer = NULL;
	gchar * g_stderrBuffer = NULL;
	gchar * argv[] = {(gchar *)"ipkg",(gchar *)"-o",(gchar *)basePkgDirName.c_str(),(gchar *)"list_installed",0};
	GError * gerr = NULL;
	gint exit_status=0;
	GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_SEARCH_PATH);

	/*
	 * gboolean            g_spawn_sync                 (const gchar *working_directory,
	                                                         gchar **argv,
	                                                         gchar **envp,
	                                                         GSpawnFlags flags,
	                                                         GSpawnChildSetupFunc child_setup,
	                                                         gpointer user_data,
	                                                         gchar **standard_output,
	                                                         gchar **standard_error,
	                                                         gint *exit_status,
	                                                         GError **error);
	 */

	gboolean resultStatus = g_spawn_sync(NULL,
			argv,
			NULL,
			flags,
			NULL,
			NULL,
			&g_stdoutBuffer,
			&g_stderrBuffer,
			&exit_status,
			&gerr);

	if ((!resultStatus) || (!g_stdoutBuffer)) {
		if (g_stdoutBuffer)
			g_free(g_stdoutBuffer);
		if (g_stderrBuffer)
			g_free(g_stderrBuffer);
		if (gerr) {
			g_warning("ApplicationInstaller::getAllUserInstalledAppNames(): error - %s",gerr->message);
			g_error_free(gerr);
		}
		return 0;
	}
	
	int n_found=0;
	int start_idx=0;
	int end_idx=0;
	char ch = 0;
	while(1) {
		//search for first alphanum char
		for (ch=g_stdoutBuffer[start_idx];!isalnum(ch) && (ch != '\0');ch=g_stdoutBuffer[start_idx]) { ++start_idx; }
		if (ch == '\0')
			break;
		//found it...init the end index
		end_idx = start_idx;
		//search for first space or tab
		for (ch=g_stdoutBuffer[++end_idx];(!isspace(ch)) && (!iscntrl(ch)) && (ch != '\0');ch=g_stdoutBuffer[end_idx]) { ++end_idx; }
		if (ch == '\0')
			break;
		//found it
		//make a string of it, if it seems like a legit string
		if (isalnum(*(char*)(g_stdoutBuffer+start_idx))) {
			std::string s = std::string((char*)(g_stdoutBuffer+start_idx),end_idx-start_idx);
			printf("s = %s\n",s.c_str());
			appList.push_back(s);
			++n_found;
		}
		//reinit start index
		start_idx = end_idx;
		//search for the newlines
		for (ch=g_stdoutBuffer[start_idx];((ch != '\n') && (ch != '\r') && (ch != '\0'));ch=g_stdoutBuffer[start_idx]) { ++start_idx; }
		if (ch == '\0')
			break;
	}
	
	g_free(g_stdoutBuffer);
	if (g_stderrBuffer)
		g_free(g_stderrBuffer);
	if (gerr) {
		g_warning("error: %s",gerr->message);
		g_error_free(gerr);
	}
	return n_found;
}

//static 
bool ApplicationInstaller::findUserInstalledAppName(const std::string& packageName,const std::string& basePkgDirName)
{
	gchar * g_stdoutBuffer = NULL;
	gchar * g_stderrBuffer = NULL;
	gchar * argv[] = {(gchar *)"ipkg",(gchar *)"-o",(gchar *)basePkgDirName.c_str(),(gchar *)"list_installed",0};
	GError * gerr = NULL;
	gint exit_status=0;
	GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_SEARCH_PATH);
	
	gboolean resultStatus = g_spawn_sync(NULL,
			argv,
			NULL,
			flags,
			NULL,
			NULL,
			&g_stdoutBuffer,
			&g_stderrBuffer,
			&exit_status,
			&gerr);

	if ((!resultStatus) || (!g_stdoutBuffer)) {
		if (g_stdoutBuffer)
			g_free(g_stdoutBuffer);
		if (g_stderrBuffer)
			g_free(g_stderrBuffer);
		if (gerr) {
			g_warning("%s: error - %s",__FUNCTION__,gerr->message);
			g_error_free(gerr);
		}
		return 0;
	}

	bool rc;
	if (strstr(g_stdoutBuffer,packageName.c_str()) == NULL)
		rc = false;
	else
		rc = true;
	
	g_free(g_stdoutBuffer);
	if (g_stderrBuffer)
		g_free(g_stderrBuffer);
	if (gerr) {
		g_warning("%s: error: %s",__FUNCTION__,gerr->message);
		g_error_free(gerr);
	}
	return rc;
}

/*
 * Utility function that returns the name and installed size of each user installed app
 * returns the number of apps found
 * 
 * 
 */

//static
int ApplicationInstaller::getAllUserInstalledAppSizes(std::vector<std::pair<std::string,uint64_t> >& appList,std::string basePkgDirName)
{
	std::vector<std::string> appNames;
	int r = ApplicationInstaller::getAllUserInstalledAppNames(appNames,basePkgDirName);
	if (r == 0)
		return 0;		//no apps found
	
	int n_found=0;
	for (std::vector<std::string>::iterator it = appNames.begin();it != appNames.end();++it) 
	{
		
		ApplicationDescription * appDesc = ApplicationManager::instance()->getAppById(*it);
		if (appDesc == NULL)
		{
			g_warning("%s: Cannot find descriptor for %s",__FUNCTION__,(*it).c_str());
			appList.push_back(std::pair<std::string,uint64_t>(*it,0));
			++n_found;
			continue;
		}
		
		g_warning("%s: Found size = %llu for %s",__FUNCTION__,appDesc->appSize(),(*it).c_str());
		appList.push_back(std::pair<std::string,uint64_t>(*it,appDesc->appSize()));
		++n_found;
	} //end app name iteration
	
	return n_found;
}

//static 
uint64_t ApplicationInstaller::getFsFreeSpaceInMB(const std::string& pathOnFs)
{
	struct statvfs fs_stats;
	memset(&fs_stats,0,sizeof(fs_stats));

	//if (::statvfs(pathOnFs.c_str(),&fs_stats) != 0) {
	if (s_statvfsFn(pathOnFs.c_str(),&fs_stats) != 0) {
		//failed to execute statvfs...treat this as if there was no free space
		return 0;
	}

	g_warning("%s: %s = %lu units at %lu bytes/unit",__FUNCTION__,pathOnFs.c_str(),fs_stats.f_bfree,fs_stats.f_frsize);

	return ((fs_stats.f_bfree * fs_stats.f_frsize) / 1048576);
		
}

//static 
uint64_t ApplicationInstaller::getFsFreeSpaceInBlocks(const std::string& pathOnFs,uint64_t * pBlockSize)
{
	struct statvfs fs_stats;
	memset(&fs_stats,0,sizeof(fs_stats));

	//if (::statvfs(pathOnFs.c_str(),&fs_stats) != 0) {
	if (s_statvfsFn(pathOnFs.c_str(),&fs_stats) != 0) {	
		//failed to execute statvfs...treat this as if there was no free space
		g_warning("Failed to execute statvfs on %s", pathOnFs.c_str());
		if (pBlockSize)
			*pBlockSize = 0;
		return 0;
	}

	if (pBlockSize)
		*pBlockSize = fs_stats.f_frsize;
	return (fs_stats.f_bfree);
}

//static 
uint64_t ApplicationInstaller::getSizeOfAppDir(const std::string& dirName)
{
	gchar cstr[FILENAME_MAX+1];
	if (dirName.length() > FILENAME_MAX)
		return 0;
	strcpy(cstr,dirName.c_str());

	gchar * g_stdoutBuffer = NULL;
	gchar * g_stderrBuffer = NULL;
	gchar * argv[] = {(gchar *)"du",(gchar *)"-s",cstr,0};
	GError * gerr = NULL;
	gint exit_status=0;
	GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_SEARCH_PATH);
	gboolean resultStatus = g_spawn_sync(NULL,
			argv,
			NULL,
			flags,
			NULL,
			NULL,
			&g_stdoutBuffer,
			&g_stderrBuffer,
			&exit_status,
			&gerr);

	g_free(g_stderrBuffer);		//don't need it...can probably supress this buffer from being created with a GFlag but I've had some trouble w/ that...

	if ((!resultStatus) || (!g_stdoutBuffer)) {
		if (gerr) {
			g_warning("ApplicationInstaller::getSizeOfAppDir(): error - %s",gerr->message);
			g_error_free(gerr);
		}
		
	}

	//split
	gchar ** tokens = g_strsplit_set(g_stdoutBuffer,(gchar *)" \t",2);
	g_free(g_stdoutBuffer);
	if (tokens == NULL) {
		return 0;
	}
	if (tokens[0] == NULL) {
		//nothing...
		g_strfreev(tokens);
		return 0;
	}

	long sz = atol(tokens[0]);
	g_strfreev(tokens);
	if (sz == 0) {
		//nothing
		return 0;
	}

	return (((uint64_t)sz) * 1024);
}

//static 
int ApplicationInstaller::_getSizeCbFn(const char *fpath, const struct stat *sb,int typeflag, struct FTW *ftwbuf)
{
//	printf("%-3s %2d %7lld   %-40s %d %s\n",
//			(typeflag == FTW_D) ?   "dir"   : (typeflag == FTW_DNR) ? "(access denied)" :
//	(typeflag == FTW_DP) ?  "dp"  : (typeflag == FTW_F) ?   "file" :
//	(typeflag == FTW_DP) ?  "dp"  : (typeflag == FTW_SL) ?  "symlink" :
//	(typeflag == FTW_SLN) ? "symlink-broken" : "???",
//			ftwbuf->level, (long long) sb->st_size,
//			fpath, ftwbuf->base, fpath + ftwbuf->base);
	//if (typeflag == FTW_F) 
	if (s_sizeFnBaseDir != fpath) //exclude the base dir entry
	{
		s_sizeFnAccumulator += (uint64_t) sb->st_blocks;
		s_auxSizeFnAccumulator += (uint64_t) sb->st_size;
	}
	return 0;           /* To tell nftw() to continue */

}

//static 
int ApplicationInstaller::_getSizeOfAppCbFn(const char *fpath, const struct stat *sb,int typeflag, struct FTW *ftwbuf)
{
	uint64_t resizedBlocks;
	if (s_sizeFnBaseDir != std::string(fpath)) //exclude the base dir entry
	{
		s_auxSizeFnAccumulator += sb->st_size;
		if (typeflag == FTW_DP)
		{
			resizedBlocks = 1;
			s_sizeFnAccumulator += 1;				//1 block for directories	
		}
		else 
		{
			resizedBlocks = ((((uint64_t) sb->st_size)) / s_targetFsBlockSize) + ((((uint64_t) sb->st_size)) % s_targetFsBlockSize ? 1 : 0);
			s_sizeFnAccumulator += resizedBlocks;
		}
		
		if ((s_manifestJobj) && (!is_error(s_manifestJobj)))
		{
			//pull out the two j-arrays
			std::string bsizeStr = toSTLString<uint64_t>(s_targetFsBlockSize);
			int addRc = 0;
			json_object * jArray_real = json_object_object_get(s_manifestJobj,"real");
			if ((jArray_real == 0) || (is_error(jArray_real)))
			{
				jArray_real = json_object_new_array();
				addRc |= 1;
			}
			json_object * jArray_bsize = json_object_object_get(s_manifestJobj,bsizeStr.c_str());
			if ((jArray_bsize == 0) || (is_error(jArray_bsize)))
			{
				jArray_bsize = json_object_new_array();
				addRc |= 2;
			}
			
			//add to arrays
			std::string sz = toSTLString<uint64_t>(((uint64_t) sb->st_size));
			json_object * innerJobj = json_object_new_object();
			json_object_object_add(innerJobj,"file",json_object_new_string(fpath));
			json_object_object_add(innerJobj,"size",json_object_new_string(sz.c_str()));
			json_object_array_add(jArray_real,innerJobj);
			sz = toSTLString<uint64_t>(resizedBlocks * s_targetFsBlockSize);
			innerJobj = json_object_new_object();
			json_object_object_add(innerJobj,"file",json_object_new_string(fpath));
			json_object_object_add(innerJobj,"size",json_object_new_string(sz.c_str()));
			json_object_array_add(jArray_bsize,innerJobj);
			
			//add the arrays to the top level obj, if necessary
			if (addRc & 1)
				json_object_object_add(s_manifestJobj,"real",jArray_real);
			if (addRc & 2)
				json_object_object_add(s_manifestJobj,bsizeStr.c_str(),jArray_bsize);
		}
			
	}
	return 0;           /* To tell nftw() to continue */
}

//static
uint64_t ApplicationInstaller::getSizeOfPackageById(const std::string& packageId)
{
	/*
	 * The size of the package encoded in the package descriptor is the size of the package as installed at this moment, calculated w.r.t to the blocksize of the file on disk
	 * While this may be larger than the actual size in some cases (see Unix command 'du' manpages for a short explanation on this), it should be a safe and good (over)estimate at worst.
	 *
	 */

	PackageDescription* packageDesc = ApplicationManager::instance()->getPackageInfoByPackageId(packageId);
	if (packageDesc) {
		if (packageDesc->packageSize() == 0)
			packageDesc->setPackageSize(getSizeOfPackageOnFsGenerateManifest("", packageDesc, NULL));	//TODO: maybe check for an existing manifest? (though likely it's not there)
		return (packageDesc->packageSize());
	}
	return 0;
}

//static
uint64_t ApplicationInstaller::getSizeOfAppOnFs(const std::string& destFsPath,const std::string& dirName,uint32_t * r_pBsize)
{
	MutexLocker lock(&s_sizeFnMutex);
	s_sizeFnBaseDir = dirName;
	s_sizeFnAccumulator = 0;
	s_auxSizeFnAccumulator = 0;
	if (destFsPath.empty())
		getFsFreeSpaceInBlocks(dirName,&s_targetFsBlockSize);
	else
		getFsFreeSpaceInBlocks(destFsPath,&s_targetFsBlockSize);
	
	if (s_targetFsBlockSize == 0)
		return 0;
	
	nftw(dirName.c_str(), ApplicationInstaller::_getSizeOfAppCbFn, 20, FTW_PHYS | FTW_DEPTH);
	if (r_pBsize)
		*r_pBsize = s_targetFsBlockSize;
	return s_sizeFnAccumulator * s_targetFsBlockSize;			//up to this point, the size was in fs blocks
		
}

uint64_t ApplicationInstaller::getSizeOfPackageOnFsGenerateManifest(const std::string& destFsPath, PackageDescription* packageDesc, uint32_t * r_pBsize)
{
	MutexLocker lock(&s_sizeFnMutex);
	if (!packageDesc) {
		g_warning("packageDesc is null in %s", __PRETTY_FUNCTION__);
		return 0;
	}
	if (packageDesc->id().empty()) {
		g_warning("packageDesc->id() is empty in %s", __PRETTY_FUNCTION__);
		return 0;
	}

	s_sizeFnAccumulator = 0;
	s_auxSizeFnAccumulator = 0;

	if (destFsPath.empty()) {
		getFsFreeSpaceInBlocks(packageDesc->folderPath(), &s_targetFsBlockSize);
	} else {
		getFsFreeSpaceInBlocks(destFsPath, &s_targetFsBlockSize);
	}

	if (s_targetFsBlockSize == 0) {
		g_warning("s_targetFsBlockSize is 0 in %s", __PRETTY_FUNCTION__);
		return 0;
	}

	//if the manifest jobj is uncleared, clear it
	if (s_manifestJobj && (!is_error(s_manifestJobj)))
	{
		json_object_put(s_manifestJobj);
	}
	s_manifestJobj = json_object_new_object();
	json_object_object_add(s_manifestJobj,"version",json_object_new_string(packageDesc->version().c_str()));
	json_object_object_add(s_manifestJobj,"installer",json_object_new_string(ApplicationInstaller::s_installer_version.c_str()));

	// add up the sizes of all the apps in this package
	std::vector<std::string>::const_iterator appIdIt, appIdItEnd;
	for (appIdIt = packageDesc->appIds().begin(), appIdItEnd = packageDesc->appIds().end(); appIdIt != appIdItEnd; ++appIdIt) {
		std::string folderPath = Settings::LunaSettings()->appInstallBase + std::string("/") + Settings::LunaSettings()->appInstallRelative + std::string("/") + *appIdIt + std::string("/");
		s_sizeFnBaseDir = folderPath;
		nftw(folderPath.c_str(), ApplicationInstaller::_getSizeOfAppCbFn, 20, FTW_PHYS | FTW_DEPTH);
		printf("DDD: %s looking at app folder %s\n", __PRETTY_FUNCTION__, (*appIdIt).c_str());
	}

	// now add up the sizes of all the services in this package
	std::vector<std::string>::const_iterator serviceIdIt, serviceIdItEnd;
	for (serviceIdIt = packageDesc->serviceIds().begin(), serviceIdItEnd = packageDesc->serviceIds().end(); serviceIdIt != serviceIdItEnd; ++serviceIdIt) {
		std::string folderPath = Settings::LunaSettings()->serviceInstallBase + std::string("/") + Settings::LunaSettings()->serviceInstallRelative + std::string("/") + *serviceIdIt + std::string("/");
		s_sizeFnBaseDir = folderPath;
		nftw(folderPath.c_str(), ApplicationInstaller::_getSizeOfAppCbFn, 20, FTW_PHYS | FTW_DEPTH);
	}

	// finally add the size of the package folder itself
	std::string packageFolder = packageDesc->folderPath();
	s_sizeFnBaseDir = packageFolder;
	nftw(packageFolder.c_str(), ApplicationInstaller::_getSizeOfAppCbFn, 20, FTW_PHYS | FTW_DEPTH);

	if (r_pBsize)
		*r_pBsize = s_targetFsBlockSize;

	//put the total sizes into the manifest obj
	std::string bsizeStr = toSTLString<uint64_t>(s_targetFsBlockSize);
	std::string sizeStr = toSTLString<uint64_t>(s_auxSizeFnAccumulator);
	json_object * totalSizeJobj = json_object_new_object();
	json_object_object_add(totalSizeJobj,"real",json_object_new_string(sizeStr.c_str()));
	sizeStr = toSTLString<uint64_t>(s_sizeFnAccumulator * s_targetFsBlockSize);
	json_object_object_add(totalSizeJobj,bsizeStr.c_str(),json_object_new_string(sizeStr.c_str()));
	json_object_object_add(s_manifestJobj,"totals",totalSizeJobj);

	//write to the manifest file
	std::string path = Settings::LunaSettings()->packageManifestsPath + std::string("/") + packageDesc->id() + std::string(".pmmanifest");
	json_object_to_file((char *) path.c_str(), s_manifestJobj);
	json_object_put(s_manifestJobj);
	s_manifestJobj = NULL;

	return s_sizeFnAccumulator * s_targetFsBlockSize;			//up to this point, the size was in fs blocks
}

//static
bool ApplicationInstaller::arePathsOnSameFilesystem(const std::string& path1,const std::string& path2)
{
	if ((path1.size() == 0) || (path2.size() == 0))
		return false;
	if (path1 == path2)
		return true;
	
	return ((path1.find("/media") == 0) && (path2.find("/media") == 0));
	
//	//int stat(const char *restrict path, struct stat *restrict buf);
//	struct stat s1,s2;
//	memset(&s1,0,sizeof(s1));
//	memset(&s2,0,sizeof(s2));
//	
//	if ((stat(path1.c_str(),&s1) != 0) || (stat(path2.c_str(),&s2) != 0))
//		return false;
//	
//	return (s1.st_dev == s2.st_dev);	
}

//static 
int ApplicationInstaller::doSignatureVerifyOnFile(const std::string& file,const std::string& signatureFile,const std::string& pubkeyFile)
{
	std::vector<std::string> params;
	params.push_back("dgst");params.push_back("-sha1");params.push_back("-verify");
	params.push_back(pubkeyFile);params.push_back("-signature");params.push_back(signatureFile);
	params.push_back(file);
	return (ApplicationInstaller::runOpenSSL(params,std::string("")));
}

//static 
int ApplicationInstaller::doSignatureVerifyOnFiles(std::vector<std::string>& files,const std::string& signatureFile,const std::string& pubkeyFile)
{
	std::vector<std::string> params;
	std::string globbedparam = std::string("/bin/cat ");
	for (std::vector<std::string>::iterator it = files.begin();it != files.end();++it) {
		globbedparam += (*it + std::string(" "));
	}
	globbedparam += std::string("| openssl dgst -sha1 -verify ") + pubkeyFile + std::string(" -signature ") + signatureFile;
	params.push_back("-c");params.push_back(globbedparam);
	return (ApplicationInstaller::runOpenSSL(params,std::string("sh")));
}

//static 
int ApplicationInstaller::extractPublicKeyFromCert(const std::string& certFile,const std::string& pubkeyFile)
{
	if ((certFile.size() == 0) || (pubkeyFile.size() == 0))
		return 0;
	
	//warning: no explicit checks on pubkeyFile path/name validity, so the calls to this function need to be restricted
	//(or else someone could specify e.g. /usr/bin/LunaSysMgr as the pubkeyFile to write to)
	//...but as a basic safety check, check for the existence of the file. if it is there, fail.
	//This will require the file to be deleted before this function is run each time
	
	if (doesExistOnFilesystem(pubkeyFile.c_str()))
		return 0;
	
	//extract the public key
	//openssl x509 -in <certname> -pubkey -out pubkey.pem
	std::string space = std::string(" ");
	std::string cmdline = std::string(s_verifyExec)
	+space+std::string(s_extractKeyOpts[0])
	+space+std::string(s_extractKeyOpts[1])
	+space+certFile
	+space+std::string(s_extractKeyOpts[2])
	+space+std::string(s_extractKeyOpts[3])
	+space+pubkeyFile;

	g_warning("ApplicationInstaller::extractPublicKeyFromCert(): executing (via system()): %s",cmdline.c_str());
	int exit_status = system(cmdline.c_str());
	if (isNonErrorProcExit((int)exit_status) == false) {
		g_warning("ApplicationInstaller::extractPublicKeyFromCert(): error: system() call returned %d",exit_status);
		return 0;
	}

	return 1;
}

//static
/*
 * Returns <= 0 for error, >0 for success
 * 
 * parameter command can be specified as a "", in which case the default openssl commandname is used
 */
int ApplicationInstaller::runOpenSSL(std::vector<std::string>& params,const std::string& command)
{
	
	gchar * g_stdoutBuffer = NULL;
	gchar * g_stderrBuffer = NULL;
	std::string re;
	int rc=0;
	
	gchar ** argv = new gchar *[params.size()+2];
	if (command.size()) {
		argv[0] = new gchar[command.size()+1];
		strcpy(argv[0],command.c_str());
	}
	else {
		argv[0] = new gchar[strlen("openssl")+1];
		strcpy(argv[0],"openssl");
	}
	int sz=1;
	for (std::vector<std::string>::iterator it = params.begin();
			it != params.end();++it,++sz) 
	{
		std::string& str = (*it);
		argv[sz] = new gchar[str.size()+1];
		strcpy(argv[sz],str.c_str());
	}
	argv[sz] = 0;
	
 	GError * gerr = NULL;
	gint exit_status=0;
	GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_SEARCH_PATH);

	/*
	 * gboolean            g_spawn_sync                 (const gchar *working_directory,
		                                                         gchar **argv,
		                                                         gchar **envp,
		                                                         GSpawnFlags flags,
		                                                         GSpawnChildSetupFunc child_setup,
		                                                         gpointer user_data,
		                                                         gchar **standard_output,
		                                                         gchar **standard_error,
		                                                         gint *exit_status,
		                                                         GError **error);
	 */

	gboolean resultStatus = g_spawn_sync(NULL,
			argv,
			NULL,
			flags,
			NULL,
			NULL,
			&g_stdoutBuffer,
			&g_stderrBuffer,
			&exit_status,
			&gerr);

	if ((!resultStatus) || (!g_stdoutBuffer)) {
		rc = -1;
		goto Done_run_ssl;
	}
	
	//for some reason, the regexec doesn't like "|" or expects it in some weird way
	if (compare_regex(".*failure.*",g_stdoutBuffer) || compare_regex(".*error.*",g_stdoutBuffer))
		rc = 0;
	else
		rc =1;

Done_run_ssl:

	if (g_stdoutBuffer)
		g_free(g_stdoutBuffer);
	if (g_stderrBuffer)
		g_free(g_stderrBuffer);
	if (gerr) {
		g_warning("ApplicationInstaller::runOpenSSL(): error - %s",gerr->message);
		g_error_free(gerr);
	}
	
	while (sz >= 0) {
		if (argv[sz])
			delete[] argv[sz];
		--sz;
	}
	delete[] argv;
	
	return rc;	
}

//static 
int ApplicationInstaller::runIpkgRemove(const std::string& ipkgRoot,const std::string& packageName)
{
	gchar * g_stdoutBuffer = NULL; g_stdoutBuffer = 0;
	gchar * g_stderrBuffer = NULL; g_stderrBuffer = 0;
	gchar * argv[8] = {0};
	GError * gerr = NULL;
	gint exit_status=0;
	GSpawnFlags flags;

	if ((packageName.empty()) || (ipkgRoot.empty())) {
		return 0;
	}
	
	//	REMOVE THE IPK (THIS WILL REMOVE IT FROM THE DISK)
	//e.g. ipkg -o <installbasepath> remove <packagename>

	flags = (GSpawnFlags)(G_SPAWN_SEARCH_PATH);

	argv[0] = (gchar *)s_pkginstallerExec; 
	argv[1] = (gchar *)s_pkginstallerOpts_location;
	argv[2] = (gchar *)(ipkgRoot.c_str()); 
	argv[3] = (gchar *)s_pkginstallerOpts_remove;
	argv[4] = (gchar *)packageName.c_str();
	argv[5] = NULL;

	g_warning("%s: executing: %s %s %s %s %s",__FUNCTION__,argv[0],argv[1],argv[2],argv[3],argv[4]);

	gboolean resultStatus = g_spawn_sync(NULL,
			argv,
			NULL,
			flags,
			NULL,
			NULL,
			&g_stdoutBuffer,
			&g_stderrBuffer,
			&exit_status,
			&gerr);

	//TODO: actually handle errors!
	if (g_stdoutBuffer) {
		g_free(g_stdoutBuffer);
	}
	if (g_stderrBuffer) {
		g_free(g_stderrBuffer);
	}

	if (gerr) {
		g_warning("error: %s",gerr->message);
		g_error_free(gerr);
		gerr=NULL;
	}
	g_warning("ipkg resultStatus = %d , exit status = %d",(int)resultStatus,(int)exit_status);
	if (resultStatus) {
		if (isNonErrorProcExit((int)exit_status) == false) {
			return 0;
		}
	}
	else {
		//failed to exec ipk package remove command
		return 0;
	}
	
	//remove the appdir
	util_cleanup_installDir(std::string(s_userInstalledPackageDir)+std::string("/")+packageName);

	return 1;
}

////---------------------------------------------------- LUNA BUS SERVICE CALLBACKS ---------------------------------------------------------------------------------------


bool ApplicationInstaller::cbInstallProgressQuery(LSHandle* lshandle, LSMessage *msg,void *user_data) {
	return (ApplicationInstaller::instance()->lunasvcInstallProgressQuery(lshandle,msg,user_data));
}

/*!
\page com_palm_appinstaller
\n
\section com_palm_appinstaller_install install

\e Public.

com.palm.appinstaller/install

Install an application.

\subsection com_palm_appinstaller_install_syntax Syntax:
\code
{
    "target": string,
    "id": string,
    "uncompressedSize": integer,
    "subscribe": boolean
}
\endcode

\param target Package file. \e Required.
\param id ID for the package.
\param uncompressedSize Uncompressed size of the package.
\param subscribe Set to true to receive status change events.

\subsection com_palm_appinstaller_install_returns_call Returns when installation request is made:
\code
{
    "returnValue": boolean,
    "subscribed": boolean,
    "ticket": string
}
\endcode

\param returnValue Indicates if the call was succesful.
\param subscribed Indicates if subscribed to receive status change events.
\param ticket Identifier that was assigned for the installation request.

\subsection com_palm_appinstaller_install_returns_status Returns when status changes:
\code
{
    "ticket": int,
    "status": string
}
\endcode

\param ticket Identifier that was assigned for the installation request.
\param status Describes the installation status.

\subsection com_palm_appinstaller_install_examples Examples:
\code
luna-send -n 10 -f luna://com.palm.appinstaller/install '{ "target":  "/tmp/apackage", "id": "com.whatnot.package", "uncompressedSize": 100, "subscribe": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "subscribed": true,
    "ticket": "6"
}
\endcode

Example status updates for a failed installation when subscribed:
\code
{
    "ticket": 6,
    "status": "STARTING"
}
{
    "ticket": 6,
    "status": "FAILED_PACKAGEFILE_NOT_FOUND"
}
{
    "ticket": 6,
    "status": "FAILED_IPKG_INSTALL"
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": false,
    "subscribed": false
}
\endcode
*/
bool ApplicationInstaller::cbInstall(LSHandle* lshandle, LSMessage *msg,void *user_data) {
	
	LSError lserror;

    // {"target": string, "id": string, "uncompressedSize": integer, "subscribe": boolean}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_4(REQUIRED(target, string), REQUIRED(id, string), REQUIRED(uncompressedSize, integer), OPTIONAL(subscribe, boolean)));

	const char* str = LSMessageGetPayload(msg);
	if( !str )
		return false;

	bool success = false;
	bool subscribed=false;
	bool retVal=false;
	
	struct json_object* root = json_tokener_parse(str);
	struct json_object* label = NULL;
	
	std::string key = "0";
	std::string targetPackageFile ="";
	std::string id = "";
	unsigned int uncompressedAppSize=0;
	
	unsigned long ticket_id=ApplicationManager::generateNewTicket();
	LSErrorInit(&lserror);
	
	const char * target_ccptr = NULL;
	
	if (is_error(root)) {
		root = NULL;
		success = false;
		goto Done;
	}

	label = json_object_object_get(root, "target");
	if ((!label) || (is_error(label))) {
		luna_warn(s_logChannel, "Failed to find param target in message");
		goto Done;
	}

	target_ccptr = (const char *)(json_object_get_string(label));

	if (!target_ccptr) {
		luna_warn(s_logChannel, "Failed to find param target (non-string in tag) in message");
		goto Done;
	}
	targetPackageFile = target_ccptr;

	label = json_object_object_get(root, "id");
	if (label && !is_error(label))
		id = json_object_get_string(label);
	
	if ((label = JsonGetObject(root,"uncompressedSize")) != NULL)
	{
		uncompressedAppSize = json_object_get_int(label);
	}
	
	success=true;
	key = toSTLString<long>(ticket_id);

	if (LSMessageIsSubscription(msg)) {

			retVal = LSSubscriptionAdd(lshandle,key.c_str(), msg, &lserror);
			if (!retVal) {
				subscribed=false;
				LSErrorPrint (&lserror, stderr);
				LSErrorFree(&lserror);
			}
			else
				subscribed=true;
	}
	
Done:

	json_object_put(root);

	json_object* reply = json_object_new_object();
	json_object_object_add(reply, "returnValue", json_object_new_boolean(success));
	json_object_object_add(reply, "subscribed", json_object_new_boolean(subscribed));
	if (success)
		json_object_object_add(reply, "ticket", json_object_new_string(key.c_str()));

	if (!LSMessageReply( lshandle, msg, json_object_to_json_string(reply), &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}

	if (success) {
		g_debug("%s: Queueing up install for %s with ticket: %ld",
				__PRETTY_FUNCTION__, targetPackageFile.c_str(), ticket_id);
		//set an install to start when the main loop gets the next chance to exec something
		InstallParams * installParams = new InstallParams(targetPackageFile, id, ticket_id,
														  lshandle,msg,uncompressedAppSize);
		//MEMALLOC: reclaim:cbInstall_detached
		ApplicationInstaller::instance()->processOrQueueCommand(installParams);
	}

	json_object_put(reply);
	return true;
}

/*!
\page com_palm_appinstaller
\n
\section com_palm_appinstaller_install_no_verify installNoVerify

\e Public.

com.palm.appinstaller/installNoVerify

Install a package without verification.

\subsection com_palm_appinstaller_install_no_verify_syntax Syntax:
\code
{
    "target": string,
    "uncompressedSize": integer,
    "systemMode": boolean,
    "subscribe": boolean
}
\endcode

\param target Package file. \e Required.
\param uncompressedSize Uncompressed size of the package.
\param systemMode Set to true to turn on system mode, which disables ipkg flags.
\param subscribe Set to true to receive status change events.

\subsection com_palm_appinstaller_install_no_verify_returns_call Returns for a call:
\code
{
    "returnValue": boolean,
    "ticket": int,
    "subscribed": boolean
}
\endcode

\param returnValue Indicates if the call was succesful.
\param ticket Identifier that was assigned for the installation request.
\param subscribed Indicates if subscribed to receive status change events.

\subsection com_palm_appinstaller_install_no_verify_returns_status Returns for status change events:
\code
{
    "ticket": int,
    "status": string
}
\endcode

\param ticket Identifier that was assigned for the installation request.
\param status Describes the installation status.

\subsection com_palm_appinstaller_install_no_verify_examples Examples:
\code
luna-send -n 10 -f luna://com.palm.appinstaller/installNoVerify '{ "target":  "/tmp/apackage", "uncompressedSize": 100, "systemMode": true, "subscribe": true }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "ticket": 1,
    "subscribed": true
}
\endcode

Example of status changes for a failed installation:
\code
{
    "ticket": 1,
    "status": "STARTING"
}
{
    "ticket": 1,
    "status": "FAILED_PACKAGEFILE_NOT_FOUND"
}
{
    "ticket": 1,
    "status": "FAILED_IPKG_INSTALL"
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": false,
    "errorCode": "Failed to find param target in message"
}
\endcode
*/

/**
 * Nearly identical to cbInstall, but will pass in a parameter to cbInstall_detached that will
 * disable package verification.
 * 
 * The reason why this is a completely separate function is that it seems the current lunabus security mechanism
 * cannot disallow only parameters to functions, but whole functions only. Therefore a separate function for
 * "no verify" exists so it can be locked down
 */
//TODO: move this to use the PRIVATE bus!
bool ApplicationInstaller::cbInstallNoVerify(LSHandle* lshandle, LSMessage *msg,void *user_data) {
	
	LSError lserror;
	std::string result;

    // {"target": string, "uncompressedSize": integer, "systemMode": boolean, "subscribe": boolean}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_4(REQUIRED(target, string), REQUIRED(uncompressedSize, integer), REQUIRED(systemMode, boolean), OPTIONAL(subscribe, boolean)));

	const char* str = LSMessageGetPayload(msg);
	if( !str )
		return false;

	bool success = false;
	bool subscribed=false;
	bool retVal=false;

	struct json_object* root = json_tokener_parse(str);
	struct json_object* label = NULL;
	
	std::string subscribe_answer = "";
	std::string key = "0";
	std::string targetPackageFile ="";
	std::string errorCode = "";
	unsigned int uncompressedAppSize = 0;
	bool systemMode=false;
	unsigned long ticket_id=ApplicationManager::generateNewTicket();
	LSErrorInit(&lserror);
		
	if (is_error(root)) {
		root = NULL;
		success = false;
		goto Done;
	}

	label = json_object_object_get(root, "target");
	if ((!label) || (is_error(label))) {
		luna_warn(s_logChannel, "Failed to find param target in message");
		errorCode = std::string("Failed to find param target in message");
		goto Done;
	}

	targetPackageFile = std::string((const char *)(json_object_get_string(label)));

	if (targetPackageFile.length() == 0) {
		luna_warn(s_logChannel, "Failed to find param target (non-string in tag) in message");
		errorCode = std::string("Failed to find param target (non-string in tag) in message");
		goto Done;
	}

	if ((label = JsonGetObject(root,"uncompressedSize")) != NULL)
	{
		uncompressedAppSize = json_object_get_int(label);
	}

	if ((label = JsonGetObject(root,"systemMode")) != NULL)
	{
		systemMode = json_object_get_boolean(label);
	}

	success=true;
	key = toSTLString<long>(ticket_id);

	if (LSMessageIsSubscription(msg)) {

			retVal = LSSubscriptionAdd(lshandle,key.c_str(), msg, &lserror);
			if (!retVal) {
				subscribed=false;
				LSErrorPrint (&lserror, stderr);
				LSErrorFree(&lserror);
			}
			else
				subscribed=true;
	}
	else
		subscribed=false;

	if (subscribed) {
		subscribe_answer = "\"subscribed\":true";
	} else {
		subscribe_answer = "\"subscribed\":false";
	}

Done:

	json_object_put(root);

	if (success) {
		std::string payload = std::string("\"ticket\":")
		+key;
		result  = std::string("{\"returnValue\":true , ")
		+payload
		+std::string(", ")+subscribe_answer + std::string("}");
	}
	else
		result  = std::string("{\"returnValue\":false ,\"errorCode\":\"")+errorCode+std::string("\"}");
	
	const char* r = result.c_str();
	if (!LSMessageReply( lshandle, msg, r, &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}

	if (success) {
		//set an install to start when the main loop gets the next chance to exec something (VERIFY = false)
		InstallParams * installParams = new InstallParams(targetPackageFile, "", ticket_id,
														  lshandle,msg,uncompressedAppSize,
														  false,systemMode);			//MEMALLOC: reclaim:cbInstall_detached
		ApplicationInstaller::instance()->processOrQueueCommand(installParams);
	}
	return true;
}

/*!
\page com_palm_appinstaller
\n
\section com_palm_appinstaller_remove remove

\e Public.

com.palm.appinstaller/remove

Remove a package.

\subsection com_palm_appinstaller_remove_syntax Syntax:
\code
{
    "packageName": string,
    "subscribe": boolean
}
\endcode

\param packageName Name of the package to remove.
\param subscribe Set to true to receive events on status changes.

\subsection com_palm_appinstaller_remove_returns_call Returns for a call:
\code
{
    "ticket": int,
    "returnValue": boolean,
    "version": version,
    "subscribed": boolean
}
\endcode

\param ticket Identifier for the removal.
\param returnValue Indicates if the call was succesful.
\param version Installed version of the package.
\param subscribed Indicates if subscribed to receive status change events.

\subsection com_palm_appinstaller_remove_returns_change Returns when ticket status changes:
\code
{
    "ticket": int,
    "status": string
}
\endcode

\param ticket Identifier for the removal.
\param status Describes the status of the remove action.

\subsection com_palm_appinstaller_remove_examples Examples:
\code
luna-send -n 10 -f luna://com.palm.appinstaller/remove '{ "packageName": "com.palm.app.email", "subscribe": true }'
\endcode

Example response for a succesful call:
\code
{
    "ticket": 6,
    "returnValue": true,
    "version": "3.0.13801",
    "subscribed": true
}
\endcode

Examples of removal status changes:
\code
{
    "ticket": 6,
    "status": "IPKG_REMOVE"
}
{
    "ticket": 6,
    "status": "SUCCESS"
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": false
}
\endcode
*/
bool ApplicationInstaller::cbRemove(LSHandle* lshandle, LSMessage *msg,void *user_data) {
	
	LSError lserror;
	std::string result;

    // {"packageName": string, "subscribe": boolean}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_2(REQUIRED(packageName, string), OPTIONAL(subscribe, boolean)));

	const char* str = LSMessageGetPayload(msg);
	if( !str )
		return false;

	bool success = false;
	bool subscribed=false;
	bool retVal=false;

	struct json_object* root = json_tokener_parse(str);
	struct json_object* package = NULL;

	std::string key = "0";
	std::string packageName ="";
	std::string version;

	unsigned long ticket_id=ApplicationManager::generateNewTicket();
	LSErrorInit(&lserror);

	PackageDescription * pPkgDesc = NULL;

	const char * packageName_ccptr = NULL;

	if (is_error(root)) {
		root = NULL;
		success = false;
		goto Done;
	}

	package = json_object_object_get(root, "packageName");
	if (!package) {
		luna_warn(s_logChannel, "Failed to find param packageName in message");
		goto Done;
	}

	packageName_ccptr = (const char *)(json_object_get_string(package));

	if (!packageName_ccptr) {
		luna_warn(s_logChannel, "Failed to find param packageName (non-string in tag) in message");
		goto Done;
	}
	packageName = packageName_ccptr;

	//BLOWFISH: figure out if the packageName is referring to an app id (old style) or a package name (new style)...
	pPkgDesc = ApplicationManager::instance()->getPackageInfoByPackageId(packageName);
	if (pPkgDesc == NULL)
	{
		g_warning("%s: name provided = [%s], getPackageInfoByPackageId FAIL",__FUNCTION__,packageName.c_str());
		//not found, so the "packageName" provided is likely an appid that belongs to a package, a new style package to be exact ...find out which
		pPkgDesc = ApplicationManager::instance()->getPackageInfoByAppId(packageName);
		if (pPkgDesc == NULL)
		{
			//nope, not found...this app id doesn't exist or no known package provided it
			g_warning("%s: name provided = [%s], getPackageInfoByPackageId FAIL, getPackageInfoByAppId FAIL - aborting remove",__FUNCTION__,packageName.c_str());
			goto Done;
		}
		//...regardless of whether it's old or new style, the package id() will be the correct one to move forward
		//remap the packageName to the one provided by the last call
		packageName = pPkgDesc->id();
		version = pPkgDesc->version();
	}
	else
	{
		g_warning("%s: name provided = [%s], getPackageInfoByPackageId PASS",__FUNCTION__,packageName.c_str());
		ApplicationDescription* appDesc = ApplicationManager::instance()->getAppById(packageName);
		if (appDesc) {
			version = appDesc->version();
		}
	}

	success=true;
	key = toSTLString<long>(ticket_id);

	if (LSMessageIsSubscription(msg)) {

			retVal = LSSubscriptionAdd(lshandle,key.c_str(), msg, &lserror);
			if (!retVal) {
				subscribed=false;
				LSErrorPrint (&lserror, stderr);
				LSErrorFree(&lserror);
			}
			else
				subscribed=true;

	}
	
	
Done:

	json_object_put(root);

	json_object* response = json_object_new_object();
	if (success) {
		json_object_object_add (response, "ticket", json_object_new_int (ticket_id));
		json_object_object_add (response, "returnValue", json_object_new_boolean (true));
		if (!version.empty())
			json_object_object_add (response, "version", json_object_new_string (version.c_str()));
		if (subscribed) {
			json_object_object_add (response, "subscribed", json_object_new_boolean (true));

		} else {
			json_object_object_add (response, "subscribed", json_object_new_boolean (false));
		}
	}
	else {
		json_object_object_add (response, "returnValue", json_object_new_boolean (false));
	}

	
	if (!LSMessageReply( lshandle, msg, json_object_to_json_string (response), &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	json_object_put (response);
	
	if (success) {
		g_debug("%s: Queueing up remove for %s with ticket: %ld",
				__PRETTY_FUNCTION__, packageName.c_str(), ticket_id);
		//set a remove to start when the main loop gets the next chance to exec something
		RemoveParams * removeParams = new RemoveParams(packageName,ticket_id,lshandle,msg,APPREMOVED_CAUSE_USERDELETED);
		ApplicationInstaller::instance()->processOrQueueCommand(removeParams);
	}
	
	return true;
}

bool ApplicationInstaller::cbIsInstalled(LSHandle* lshandle, LSMessage *msg,void *user_data) {
	return (ApplicationInstaller::instance()->lunasvcIsInstalled(lshandle,msg,user_data));
}

/*!
\page com_palm_appinstaller
\n
\section com_palm_appinstaller_notify_on_change notifyOnChange

\e Public.

com.palm.appinstaller/notifyOnChange

Subscribe to receive notifications when applications are installed or removed.

\subsection com_palm_appinstaller_notify_on_change_syntax Syntax:
\code
{
    "appId": string
}
\endcode

\param appId ID of the application to watch. If not specified, notifications are received for all applications.

\subsection com_palm_appinstaller_notify_on_change_returns_call Returns on call:
\code
{
    "returnValue": boolean,
    "subscribed": boolean,
    "appId":string
}
\endcode

\param returnValue Indicates if the call was succesful.
\param subscribed True when subscribed to receive change events.
\param appId ID of the watched application, or * in case all applications are watched.

\subsection com_palm_appinstaller_notify_on_change_returns_on_change Returns on status changes:
\code
{
    "appId": string,
    "version": string,
    "statusChange": string,
    "cause": string
}
\endcode

\param appId ID of the application.
\param version Version of the application.
\param statusChange Describes the status change.
\param cause What caused the change.

\subsection com_palm_appinstaller_notify_on_change_examples Examples:
\code
luna-send -n 10 -f luna://com.palm.appinstaller/notifyOnChange '{ "appId": "com.palm.app.email" }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "subscribed": true,
    "appId": "com.palm.app.email"
}
\endcode

Example of an application removal:
\code
{
    "appId": "com.palm.app.email",
    "version": "3.0.13801",
    "statusChange": "REMOVED",
    "cause": "USER"
}
\endcode
*/
bool ApplicationInstaller::cbNotifyOnChange(LSHandle* lshandle, LSMessage *msg,void *user_data) {
	
	LSError lserror;
	std::string result;

    // {"appId": string}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_1(REQUIRED(appId, string)));

	const char* str = LSMessageGetPayload(msg);
	if( !str )
		return false;

	bool subscribed=false;
	bool retVal=false;

	struct json_object* root = json_tokener_parse(str);
	struct json_object* j_appid = NULL;

	std::string subscribe_answer = "";
	std::string key = "0";
	std::string appId ="";

	LSErrorInit(&lserror);

	const char * appid_ccptr = NULL;

	if (is_error(root)) {
		root = NULL;
		luna_warn(s_logChannel, "Failed to find param appId in message...defaulting to all appIds");
		appid_ccptr = "*";
	}
	else {
		j_appid = json_object_object_get(root, "appId");
		if (!j_appid) {
			luna_warn(s_logChannel, "Failed to find param appId in message...defaulting to all appIds");
			appid_ccptr = "*";
		}
		else {
			appid_ccptr = (const char *)(json_object_get_string(j_appid));

			if (!appid_ccptr) {
				luna_warn(s_logChannel, "Failed to find param appId (non-string in tag) in message...defaulting to all appIds");
				appid_ccptr = "*";
			}
		}
	}

	appId= appid_ccptr;

	retVal = LSSubscriptionAdd(lshandle,appId.c_str(), msg, &lserror);
	if (!retVal) {
		subscribed=false;
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	else
		subscribed=true;
		
	if (subscribed) {
			subscribe_answer = "\"subscribed\":true";
	} else {
			subscribe_answer = "\"subscribed\":false";
	}

	if (root)
		json_object_put(root);

	result  = std::string("{\"returnValue\":true ,")+subscribe_answer+std::string(", ");
	
	std::string payload = std::string("\"appId\":\"")
							+appId+std::string("\"");
	result += payload;
	result += std::string("}");

	const char* r = result.c_str();
	if (!LSMessageReply( lshandle, msg, r, &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}

	//do any additional processing besides just adding the subscription and checking input param validity.
	ApplicationInstaller::instance()->lunasvcNotifyOnChange(appId,lshandle,msg);

	return true;
}

/*!
\page com_palm_appinstaller
\n
\section com_palm_appinstaller_get_user_installed_app_sizes getUserInstalledAppSizes

\e Public.

com.palm.appinstaller/getUserInstalledAppSizes

Get sizes for user installed applications.

\subsection com_palm_appinstaller_get_user_installed_app_sizes_syntax Syntax:
\code
{
}
\endcode

\subsection com_palm_appinstaller_get_user_installed_app_sizes_returns Returns:
\code
{
    "returnValue": boolean,
    "apps": [
        {
            "appName": string,
            "size": int
        }
    ],
    "totalSize": int
}
\endcode

\param returnValue Indicates if the call was succesful.
\param apps Object array, see fields of contained objects below.
\param appName Name of the application.
\param size Size of the application in kilobytes.
\param totalSize Combined size of all the applications in kilobytes.

\subsection com_palm_appinstaller_get_user_installed_app_sizes_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.appinstaller/getUserInstalledAppSizes '{ }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "apps": [
        {
            "appName": "com.palm.app.enyo-facebook",
            "size": 0
        },
        {
            "appName": "com.palm.app.enyo-findapps",
            "size": 0
        },
        {
            "appName": "com.palm.app.kindle",
            "size": 0
        },
        {
            "appName": "com.palm.app.youtube",
            "size": 0
        },
        {
            "appName": "com.quickoffice.ar",
            "size": 0
        },
        {
            "appName": "com.quickoffice.webos",
            "size": 0
        }
    ],
    "totalSize": 0
}
\endcode
*/
bool ApplicationInstaller::cbGetSizes(LSHandle* lshandle,LSMessage *msg,void *user_data)
{
	return ApplicationInstaller::instance()->lunasvcGetInstalledSizes(lshandle,msg);
}

/*!
\page com_palm_appinstaller
\n
\section com_palm_appinstaller_query_install_capacity queryInstallCapacity

\e Public.

com.palm.appinstaller/queryInstallCapacity

Query for the space that is available, and space that is required to install an application package.

\subsection com_palm_appinstaller_query_install_capacity_syntax Syntax:
\code
{
    "appId": string,
    "packageId": string,
    "size": string,
    "uncompressedSize": string
}
\endcode

\param appId Application ID. Either this, or \e packageId is required.
\param packageId Package ID. Either this, or \e appId is required.
\param size Size of the package in kilobytes. Required.
\param uncompressedSize Uncompressed size of the package in kilobytes.

\subsection com_palm_appinstaller_query_install_capacity_returns Returns:
\code
{
    "returnValue": boolean,
    "result": int,
    "spaceNeededInKB": string,
    "errorCode": string,
    "errorText": string
}
\endcode

\param returnValue Indicates if the call was succesful.
\param result Available space in kilobytes.
\param spaceNeededInKB Space required to install the package.
\param errorCode Error code in case the call failed.
\param errorText Describes the error in more detail.

\subsection com_palm_appinstaller_query_install_capacity_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.appinstaller/queryInstallCapacity '{ "packageId": "com.whatnot.app", "size": "300", "uncompressedSize": "1200" }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true,
    "result": 0,
    "spaceNeededInKB": "1536"
}

\endcode

Example response for a failed call:
\code
{
    "returnValue": false,
    "errorCode": "appinstaller_error",
    "errorText": "missing size parameter"
}
\endcode
*/
bool ApplicationInstaller::cbQueryInstallCapacity(LSHandle* lshandle,LSMessage *msg,void *user_data) {
	
	LSError lserror;
	std::string result;
	int queryCapacityResult=0;
	uint64_t spaceNeeded = 0;
	std::string packageId;
	std::string errorText;
	std::string strsize;
	std::string strucsize;
	
	uint64_t size;			//of package, in KB
	uint64_t ucSize;		//uncompressed size of the app in the package, in KB
	LSErrorInit(&lserror);

    // {"appId": string, "packageId": string, "size": string, "uncompressedSize": string}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_4(REQUIRED(appId, string), REQUIRED(packageId, string), REQUIRED(size, string), REQUIRED(uncompressedSize, string)));

	const char* str = LSMessageGetPayload(msg);
	if( !str )
		return false;
	
	//parse parameters
	struct json_object* root = json_tokener_parse(str);
	struct json_object* label = 0;

	if ((root == NULL) || (is_error(root))) {
		root = 0;
		goto Done;
	}
	
	// to keep this method backwards-compatible we look for both appId and packageId
	label = json_object_object_get(root,"appId");
	if ((label == NULL) || (is_error(label))) 
	{
		label = json_object_object_get(root,"packageId");
		if ((label == NULL) || (is_error(label)))
		{
			packageId = "";
		}
		else
		{
			packageId = json_object_get_string(label);
		}
	}
	else 
	{
		packageId = json_object_get_string(label);
	}
	
	if (extractFromJson(root,"size",strsize) == false)
	{
		errorText = "missing size parameter";
		goto Done;
	}

	size = strtouq(strsize.c_str(),NULL,10);
	
	if (size == 0)
	{
		errorText = "invalid size (0)";
		goto Done;
	}
	
	if (extractFromJson(root,"uncompressedSize",strucsize) == false)
	{
		ucSize = 0;
	}
	else
	{
		ucSize = strtouq(strucsize.c_str(),NULL,10);
	}
		
	queryCapacityResult = ApplicationInstaller::instance()->lunasvcQueryInstallCapacity(packageId, size, ucSize, spaceNeeded);
	if (queryCapacityResult < 0) {
		errorText = "calculating free space failed; (check appId/packageId parameter?)";
	}
	
Done:

	if (root) {
		json_object_put(root);
	}

	root = json_object_new_object();
	if (errorText.size()) {
		json_object_object_add(root,"returnValue",json_object_new_boolean(false));
		json_object_object_add(root,"errorCode",json_object_new_string("appinstaller_error"));
		json_object_object_add(root,"errorText",json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(root,"returnValue",json_object_new_boolean(true));
		json_object_object_add(root,"result",json_object_new_int(queryCapacityResult));
		std::string sizeStr = toSTLString<uint64_t>(spaceNeeded);
		//returning as string to workaround cjson's inability to express unsigned int32
		json_object_object_add(root,"spaceNeededInKB",json_object_new_string(sizeStr.c_str()));
	}

	if (!LSMessageReply( lshandle, msg,json_object_to_json_string(root), &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	
	json_object_put(root);
	return true;
}

bool ApplicationInstaller::cbDbgGetPkgInfoFromStatusFile(LSHandle* lshandle,LSMessage *msg,void *user_data)
{
	std::string errorText;
	std::string packageName;
	std::string statusFile;
	std::string info;

    // {"package": string, "statusfile": string}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_2(REQUIRED(package, string), REQUIRED(statusfile, string)));

	const char* str = LSMessageGetPayload(msg);
	
	if( !str )
		return false;

	//parse parameters
	struct json_object* root = json_tokener_parse(str);
	
	if ((root == NULL) || (is_error(root))) {
		errorText = "json parse error";
		root = 0;
		goto Done_cbDbgGetPkgInfoFromStatusFile;
	}

	if (extractFromJson(root,"package",packageName) == false) {
		errorText = "missing package name";
		goto Done_cbDbgGetPkgInfoFromStatusFile;
	}

	if (extractFromJson(root,"statusfile",statusFile) == false) {
		errorText = "missing status file name";
		goto Done_cbDbgGetPkgInfoFromStatusFile;
	}
	
Done_cbDbgGetPkgInfoFromStatusFile:

	if (root)
		json_object_put(root);
	
	json_object * replyJson = json_object_new_object();
	json_object_object_add(replyJson,"returnValue",json_object_new_boolean(true));
	
	LSError lserror;
	LSErrorInit(&lserror);
	if (!LSMessageReply( lshandle, msg, json_object_to_json_string(replyJson), &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	json_object_put(replyJson);
	return true;
}

 
bool ApplicationInstaller::cbDbgFakeFsSize(LSHandle* lshandle,LSMessage *msg,void *user_data)
{
	std::string errorText = "";
	std::string dir;
	std::string sizeStr;
	uint32_t sizeInKB = 0;
	struct stat fsStat;
	struct statfs fsStatFs;
	struct statvfs fsStatVfs;
	uint32_t sizeInSFSBlocks;
	uint32_t sizeInSVFSBlocks;
	
	json_object * persistRoot;
	
	memset(&fsStat,0,sizeof(fsStat));
	memset(&fsStatFs,0,sizeof(fsStatFs));
	memset(&fsStatVfs,0,sizeof(fsStatVfs));

    // {"dir": string, "sizeKB": string}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_2(REQUIRED(dir, string), REQUIRED(sizeKB, string)));

	const char* str = LSMessageGetPayload(msg);

	if( !str )
		return false;

	//parse parameters
	struct json_object* root = json_tokener_parse(str);
	
	if ((root == NULL) || (is_error(root))) {
		errorText = "json parse error";
		root = 0;
		goto Done_cbDbgFakeFsSize;
	}

	if (extractFromJson(root,"dir",dir) == false) {
		errorText = "missing dir param";
		goto Done_cbDbgFakeFsSize;
	}

	if (::stat(dir.c_str(),&fsStat) != 0)
	{
		errorText = std::string("cannot stat fs path given as dir parameter: ")+dir;
		goto Done_cbDbgFakeFsSize;
	}
	
	if (::statfs(dir.c_str(),&fsStatFs) != 0) {
		errorText = std::string("cannot statfs fs path given as dir parameter: ")+dir;
		goto Done_cbDbgFakeFsSize;
	}
	
	if (::statvfs(dir.c_str(),&fsStatVfs) != 0) {
		errorText = std::string("cannot statvfs fs path given as dir parameter: ")+dir;
		goto Done_cbDbgFakeFsSize;
	}
	
	if (extractFromJson(root,"sizeKB",sizeStr) == false) {
		errorText = "missing sizeKB param";
		goto Done_cbDbgFakeFsSize;
	}

	sizeInKB = strtoul(sizeStr.c_str(),NULL,10);
	
	sizeInSFSBlocks = (sizeInKB << 10) / fsStatFs.f_bsize;
	sizeInSVFSBlocks = (sizeInKB << 10) / fsStatVfs.f_frsize;
	
	dbg_statfs_map[fsStat.st_dev] = std::pair<uint32_t,uint32_t>(fsStatFs.f_bfree,sizeInSFSBlocks);
	dbg_statvfs_map[fsStat.st_dev] = std::pair<uint32_t,uint32_t>(fsStatVfs.f_bfree,sizeInSVFSBlocks);
	
	//update the persistent object then write it out to a file to preserve this across reboots
	if (dbg_statxfs_persistent == NULL)
		dbg_statxfs_persistent = json_object_new_object();
	
	persistRoot = json_object_new_object();
	json_object_object_add(persistRoot,"sizeKB",json_object_new_string(sizeStr.c_str()));
	json_object_object_add(dbg_statxfs_persistent,dir.c_str(),persistRoot);
	
	json_object_to_file((char *)"/tmp/DBGFakeFsSizes.json",dbg_statxfs_persistent);
	
	//install shims
	s_statfsFn = dbg_statfs;
	s_statvfsFn = dbg_statvfs;
	
	Done_cbDbgFakeFsSize:

	if (root)
		json_object_put(root);

	json_object * replyJson = json_object_new_object();
	if (errorText.empty()) 
	{
		json_object_object_add(replyJson,"returnValue",json_object_new_boolean(true));
		json_object_object_add(replyJson,"fs_directory",json_object_new_string(dir.c_str()));
		json_object_object_add(replyJson,"size_in_KB_forced_to",json_object_new_int(sizeInKB));
	}
	else 
	{
		json_object_object_add(replyJson,"returnValue",json_object_new_boolean(false));
		json_object_object_add(replyJson,"errorText",json_object_new_string(errorText.c_str()));
	}
	
	LSError lserror;
	LSErrorInit(&lserror);
	if (!LSMessageReply( lshandle, msg, json_object_to_json_string(replyJson), &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	json_object_put(replyJson);
	return true;
}

//static 
int ApplicationInstaller::dbg_restoreFakeFsEntriesAtStartup()
{
	
	if ((dbg_statxfs_persistent) && (!is_error(dbg_statxfs_persistent)))
	{
		json_object_put(dbg_statxfs_persistent);
		dbg_statxfs_persistent = NULL;
	}
	
	dbg_statxfs_persistent = json_object_from_file((char *)"/tmp/DBGFakeFsSizes.json");
	if ((dbg_statxfs_persistent == NULL) || (is_error(dbg_statxfs_persistent)))
	{
		dbg_statxfs_persistent = NULL;
		return 0;
	}
	
	int i=0;
	json_object_object_foreach(dbg_statxfs_persistent, key, val) 
	{
		std::string sizeKB;
		uint32_t isizeKB;
		if (extractFromJson(val,"sizeKB",sizeKB) == false)
			continue;
		
		isizeKB = strtoul(sizeKB.c_str(),NULL,10);

		struct stat fsStat;
		struct statfs fsStatFs;
		struct statvfs fsStatVfs;

		memset(&fsStat,0,sizeof(fsStat));
		memset(&fsStatFs,0,sizeof(fsStatFs));
		memset(&fsStatVfs,0,sizeof(fsStatVfs));

		if (::stat(key,&fsStat) != 0)
		{
			g_warning("%s : cannot stat fs path given as dir parameter: %s",__FUNCTION__,key);
			continue;
		}

		if (::statfs(key,&fsStatFs) != 0) {
			g_warning("%s : cannot statfs fs path given as dir parameter: %s",__FUNCTION__,key);
			continue;
		}

		if (::statvfs(key,&fsStatVfs) != 0) {
			g_warning("%s : cannot statvfs fs path given as dir parameter: %s",__FUNCTION__,key);
			continue;
		}
				
		uint32_t sizeInSFSBlocks = (isizeKB << 10) / fsStatFs.f_bsize;
		uint32_t sizeInSVFSBlocks = (isizeKB << 10) / fsStatVfs.f_frsize;

		dbg_statfs_map[fsStat.st_dev] = std::pair<uint32_t,uint32_t>(fsStatFs.f_bfree,sizeInSFSBlocks);
		dbg_statvfs_map[fsStat.st_dev] = std::pair<uint32_t,uint32_t>(fsStatVfs.f_bfree,sizeInSVFSBlocks);

		++i;
	}
	
	if (i) 
	{
		//install shims
		s_statfsFn = dbg_statfs;
		s_statvfsFn = dbg_statvfs;
	}	
	return 1;
}

bool ApplicationInstaller::cbDbgUnFakeFsSizes(LSHandle* lshandle,LSMessage *msg,void *user_data)
{
    EMPTY_SCHEMA_RETURN(lshandle, msg);

	//un-install shims
	s_statfsFn = ::statfs;
	s_statvfsFn = ::statvfs;

	//clear the maps
	dbg_statfs_map.clear();
	dbg_statvfs_map.clear();
	
	unlink("/tmp/DBGFakeFsSizes.json");
	if ((dbg_statxfs_persistent) && (!is_error(dbg_statxfs_persistent)))
		json_object_put(dbg_statxfs_persistent);
	dbg_statxfs_persistent = NULL;
	
	json_object * replyJson = json_object_new_object();
	json_object_object_add(replyJson,"returnValue",json_object_new_boolean(true));

	LSError lserror;
	LSErrorInit(&lserror);
	if (!LSMessageReply( lshandle, msg, json_object_to_json_string(replyJson), &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	json_object_put(replyJson);
	return true;
		
}

bool ApplicationInstaller::cbGetFsSize(LSHandle* lshandle,LSMessage *msg,void *user_data)
{
	std::string errorText = "";
	std::string dir;
	uint64_t fspace;
	uint64_t bsize;
	uint64_t tsizeMB;

    // {"dir": string}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_1(REQUIRED(dir, string)));

	const char* str = LSMessageGetPayload(msg);

	if( !str )
		return false;

	//parse parameters
	struct json_object* root = json_tokener_parse(str);

	if ((root == NULL) || (is_error(root))) {
		errorText = "json parse error";
		root = 0;
		goto Done_cbGetFsSize;
	}

	if (extractFromJson(root,"dir",dir) == false) {
		errorText = "missing dir parameter";
		goto Done_cbGetFsSize;
	}

	fspace = ApplicationInstaller::getFsFreeSpaceInBlocks(dir,&bsize);

	tsizeMB = (((uint64_t)fspace * (uint64_t)bsize)) >> 20;
	if (tsizeMB == 0)
		tsizeMB =1;
	
	Done_cbGetFsSize:

	if (root)
		json_object_put(root);

	json_object * replyJson = json_object_new_object();
	if (errorText.empty()) 
	{
		json_object_object_add(replyJson,"returnValue",json_object_new_boolean(true));
		json_object_object_add(replyJson,"fs_directory",json_object_new_string(dir.c_str()));
		json_object_object_add(replyJson,"size_in_blocks",json_object_new_string((toSTLString<uint64_t>(fspace)).c_str()));
		json_object_object_add(replyJson,"block_size",json_object_new_string((toSTLString<uint64_t>(bsize)).c_str()));
		json_object_object_add(replyJson,"approx_size_MB",json_object_new_string((toSTLString<uint64_t>(tsizeMB)).c_str()));
	}
	else 
	{
		json_object_object_add(replyJson,"returnValue",json_object_new_boolean(false));
		json_object_object_add(replyJson,"errorText",json_object_new_string(errorText.c_str()));
	}
	LSError lserror;
	LSErrorInit(&lserror);
	if (!LSMessageReply( lshandle, msg, json_object_to_json_string(replyJson), &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	json_object_put(replyJson);
	return true;

}

bool ApplicationInstaller::cbDbgFillSize(LSHandle* lshandle,LSMessage *msg,void *user_data)
{
	std::string errorText = "";
	std::string dir;
	
	std::string strbsize;
	std::string strnblocks;
	uint32_t bsize;
	uint32_t nblocks;
	int ec = 0;

    // {"dir": string, "bsize": string, "nblocks": string}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_3(REQUIRED(dir, string), REQUIRED(bsize, string), REQUIRED(nblocks, string)));

	const char* str = LSMessageGetPayload(msg);

	if( !str )
		return false;

	//parse parameters
	struct json_object* root = json_tokener_parse(str);

	if ((root == NULL) || (is_error(root))) 
	{
		errorText = "json parse error";
		root = 0;
		goto Done_cbDbgFillSize;
	}

	if (extractFromJson(root,"dir",dir) == false) 
	{
		errorText = "missing dir parameter";
		goto Done_cbDbgFillSize;
	}
	
	if (extractFromJson(root,"bsize",strbsize) == false)
	{
		bsize = 0;
	}
	else 
	{
		bsize = strtoul(strbsize.c_str(),NULL,10);
	}
	
	if (extractFromJson(root,"nblocks",strnblocks) == false)
	{
		nblocks = 0;
	}
	else 
	{
		nblocks = strtoul(strnblocks.c_str(),NULL,10);
	}
	
	if ((ec = dbg_fill(dir,bsize,nblocks)) <= 0)
		errorText = "fill failed";
	
	Done_cbDbgFillSize:

	if (root)
		json_object_put(root);

	json_object * replyJson = json_object_new_object();
	if (errorText.empty()) 
	{
		json_object_object_add(replyJson,"returnValue",json_object_new_boolean(true));
		json_object_object_add(replyJson,"path",json_object_new_string(dir.c_str()));
		json_object_object_add(replyJson,"nblocks",json_object_new_string((toSTLString<uint64_t>(nblocks)).c_str()));
		json_object_object_add(replyJson,"block_size",json_object_new_string((toSTLString<uint64_t>(bsize)).c_str()));
	}
	else 
	{
		json_object_object_add(replyJson,"returnValue",json_object_new_boolean(false));
		json_object_object_add(replyJson,"errorText",json_object_new_string(errorText.c_str()));
		json_object_object_add(replyJson,"errorCode",json_object_new_int(ec));
	}
	LSError lserror;
	LSErrorInit(&lserror);
	if (!LSMessageReply( lshandle, msg, json_object_to_json_string(replyJson), &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	json_object_put(replyJson);
	return true;

}

//static 
bool ApplicationInstaller::cbDbgGetAppSizeOnFs(LSHandle* lshandle,LSMessage *msg,void *user_data)
{
	int ec=0;
	std::string errorText = "";
	std::string fsPath;
	std::string appBasePath;
	uint64_t aSize;

    // {"fspath": string, "appbasepath": string}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_2(REQUIRED(fspath, string), REQUIRED(appbasepath, string)));

	const char* str = LSMessageGetPayload(msg);

	if( !str )
		return false;

	//parse parameters
	struct json_object* root = json_tokener_parse(str);

	if ((root == NULL) || (is_error(root))) 
	{
		errorText = "json parse error";
		root = 0;
		goto Done_cbDbgGetAppSizeOnFs;
	}

	if (extractFromJson(root,"fspath",fsPath) == false) 
	{
		errorText = "missing fspath parameter";
		goto Done_cbDbgGetAppSizeOnFs;
	}

	if (extractFromJson(root,"appbasepath",appBasePath) == false)
	{
		errorText = "missing appbasepath parameter";
		goto Done_cbDbgGetAppSizeOnFs;
	}
	
	ec = dbg_getAppSizeOnFs(fsPath,appBasePath,aSize);
	if (ec == 0)
		errorText = "error executing function";
	
	Done_cbDbgGetAppSizeOnFs:

	if (root)
		json_object_put(root);

	json_object * replyJson = json_object_new_object();
	if (errorText.empty()) 
	{
		json_object_object_add(replyJson,"returnValue",json_object_new_boolean(true));
		json_object_object_add(replyJson,"size",json_object_new_string((toSTLString<uint64_t>(aSize)).c_str()));
	}
	else 
	{
		json_object_object_add(replyJson,"returnValue",json_object_new_boolean(false));
		json_object_object_add(replyJson,"errorText",json_object_new_string(errorText.c_str()));
		json_object_object_add(replyJson,"errorCode",json_object_new_int(ec));
	}
	
	json_object_object_add(replyJson,"fspath",json_object_new_string(fsPath.c_str()));
	json_object_object_add(replyJson,"appbasepath",json_object_new_string(appBasePath.c_str()));
			
	LSError lserror;
	LSErrorInit(&lserror);
	if (!LSMessageReply( lshandle, msg, json_object_to_json_string(replyJson), &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	json_object_put(replyJson);
	return true;
}

/*!
\page com_palm_appinstaller
\n
\section com_palm_appinstaller_revoke revoke

\e Public.

com.palm.appinstaller/revoke

Revoke one on more applications.

\subsection com_palm_appinstaller_revoke_syntax Syntax:
\code
{
    "item": {
        "payload": {
            "signature": string,
            "appId": [ string array ]
        }
    }
}
\endcode

\param item Object.
\param payload Object.
\param signature Valid signature required to revoke applications.
\param appId IDs of applications to revoke

\subsection com_palm_appinstaller_revoke_returns Returns:
\code
{
    "returnValue": boolean,
    "errorCode": string
}
\endcode

\param returnValue Indicates if the call was succesful.
\param errorCode Describes the error if call was not succesful.

\subsection com_palm_appinstaller_revoke_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.appinstaller/revoke '{ "item": { "payload": { "signature": "notAValidSignature", "appId": ["com.palm.app.oneApp", "com.palm.app.anotherApp" ] } } }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": false,
    "errorCode": "verify failed"
}
\endcode
*/
bool ApplicationInstaller::cbRevoke(LSHandle* lshandle,LSMessage *msg,void *user_data)
{
	std::string errorText;
	std::string innerPayload;
	std::string appIdGlob;
	std::string appIdGlobTmpFilename;
	std::string signatureBase64;
	std::string signatureRaw;
	std::string signatureTmpFilename;
	struct json_object * appidArray;
	std::string appIdForIdx;
	int listIdx;
	std::vector<std::string> opensslParams;
	std::string pubkeyTmpFilename;

    // {"item": string, "payload": {"signature": string, "appId": array}}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               msg,
                               SCHEMA_2(REQUIRED(item, string), NAKED_OBJECT_REQUIRED_2(payload, signature, string, appId, array)));

	const char* str = LSMessageGetPayload(msg);
	if( !str )
		return false;

	//parse parameters
	struct json_object* root = json_tokener_parse(str);
	struct json_object* item_root = 0;
	struct json_object* payload_root = 0;

	if ((root == NULL) || (is_error(root))) {
		errorText = "json parse error";
		root = 0;
		goto Done;
	}

	if (extractFromJson(root,"item",innerPayload) == false) {
		errorText = "missing item key";
		goto Done;
	}
	
	item_root = json_tokener_parse(innerPayload.c_str());
	if ((item_root == NULL) || (is_error(item_root))) {
		errorText = "item payload parse error";
		item_root = 0;
		goto Done;
	}
	
	payload_root = JsonGetObject(item_root,"payload");
	if (payload_root == NULL) {
		errorText = "payload key missing";
		goto Done;
	}
	
	if (extractFromJson(payload_root,"signature",signatureBase64) == false) {
		errorText = "missing signature key";
		goto Done;
	}
	
	signatureRaw = base64_decode(signatureBase64);
	
	if ((appidArray = JsonGetObject(payload_root,"appId")) == NULL) {
		errorText = "missing appId key";
		goto Done;
	}
	
	if (!(json_object_is_type(appidArray,json_type_array))) {
		errorText = "appId key does not represent a json array object";
		goto Done;
	}
	
	for (listIdx=0;listIdx<json_object_array_length(appidArray);++listIdx) {
		appIdForIdx = json_object_get_string(json_object_array_get_idx(appidArray,listIdx));
		appIdGlob += appIdForIdx;
	}
	
	//write the signature out to a temp file
	if (writeToTempFile(signatureRaw,"/tmp/",signatureTmpFilename) == false) {
		errorText = "couldn't create temp filename for signature";
		goto Done;
	}
	
	//write appid glob to a temp file
	if (writeToTempFile(appIdGlob,"/tmp/",appIdGlobTmpFilename) == false) {
		errorText = "couldn't create temp filename for appid glob";
		goto Done;
	}
	
	//extract the pubkey to a tempfile
	//but first delete any old ones there
	pubkeyTmpFilename = std::string("/tmp/pubsub_pubkey.pem");
	unlink(pubkeyTmpFilename.c_str());
	if (ApplicationInstaller::extractPublicKeyFromCert(s_revocationCertFile,pubkeyTmpFilename) <= 0) {
		errorText = "key extraction from cert failed";
		goto Done;
	}
				
	//verify the signature
	if (ApplicationInstaller::doSignatureVerifyOnFile(appIdGlobTmpFilename,signatureTmpFilename,pubkeyTmpFilename) <= 0) {
		errorText = "verify failed";
		goto Done;
	}
	
	//ok signature verified. Now go through a loop and add remove "sources" for each appid in the list
	for (listIdx=0;listIdx<json_object_array_length(appidArray);++listIdx) {
		appIdForIdx = json_object_get_string(json_object_array_get_idx(appidArray,listIdx));
		RemoveParams * removeParams = new RemoveParams(appIdForIdx,-69,lshandle,msg,APPREMOVED_CAUSE_APPREVOKED);
		ApplicationInstaller::instance()->processOrQueueCommand(removeParams);
	}
	
Done:
	
	if (root)
		json_object_put(root);
	if (item_root)
		json_object_put(item_root);
	
	if (signatureTmpFilename.size())
		deleteFile(signatureTmpFilename.c_str());
	if (pubkeyTmpFilename.size())
		deleteFile(pubkeyTmpFilename.c_str());
	
	std::string reply;
	if (errorText.size()) {
		reply = std::string("{ \"returnValue\":false , \"errorCode\":\"")+errorText+std::string("\"}");
	}
	else {
		reply = std::string("{ \"returnValue\":true }");
	}
	LSError lserror;
	LSErrorInit(&lserror);
	if (!LSMessageReply( lshandle, msg, reply.c_str(), &lserror )) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
	}
	return true;
}

bool ApplicationInstaller::cbPubSubRegister(LSHandle* handle, LSMessage* msg, void* ctxt)
{
    // {"returnValue": boolean}
    VALIDATE_SCHEMA_AND_RETURN(handle,
                               msg,
                               SCHEMA_1(REQUIRED(returnValue, boolean)));

	const char* str = LSMessageGetPayload(msg);
	if (str) {
		json_object* root = json_tokener_parse(str);
		if (!root || is_error(root)) {
			g_warning("%s: %s", __PRETTY_FUNCTION__, str);
		}
		else {
			json_object* prop = json_object_object_get(root, "returnValue");
			if (!prop || !json_object_get_boolean(prop))
				g_warning("%s: %s", __PRETTY_FUNCTION__, str);
			json_object_put(root);
		}
	}
	return true;
}

bool ApplicationInstaller::cbPubSubStatus(LSHandle* handle, LSMessage* msg, void* ctxt)
{
    DEPRECATED_SERVICE_MSG();

	/*
	 *
	 * This now effectively does nothing...
	 *
	 * Changed due to requirements, summarized briefly:
	 *
	 * "There is a new way of registering pubsub handlers for your node, you can just drop a file, in /etc/palm/pubsub_handlers and pubsubservice will pick it up.
	 *  You can remove all the registerItemCallback() API stuff you had in your code. "
	 *
	 */
	const char* str = LSMessageGetPayload(msg);
	if( !str )
		return false;

	//parse parameters
	struct json_object* root = json_tokener_parse(str);
	struct json_object* item = 0;
	LSError lserror;
	LSErrorInit(&lserror);
		
	if ((!root) || is_error(root)) {
		root = 0;
		goto Done_cbPubSubStatus;
	}
	
	item = JsonGetObject(root,"connected");
	if (item == NULL) {
		goto Done_cbPubSubStatus;
	}

	g_message("%s: Pubsub service is now %s",__FUNCTION__,(json_object_get_boolean(item) ? "connected" : "disconnected"));

Done_cbPubSubStatus:

	if (root)
		json_object_put(root);
	
	return true;
}

////---------------------------------------------------- NATIVE INTERFACE FOR DIRECT CALLS --------------------------------------------------------------------------------

bool ApplicationInstaller::downloadAndInstall (LSHandle* lshandle, const std::string& targetPackageFile, 
	struct json_object* authToken, struct json_object* deviceId,
	unsigned long ticket, bool subscribe) 
{
    bool retval;
    LSError lserror;
    LSErrorInit (&lserror);
    json_object* downloadParams = json_object_new_object();

    if (targetPackageFile.empty()) {
	g_warning ("%s: download request for empty url", __PRETTY_FUNCTION__);
	return false;
    }

    json_object_object_add (downloadParams, "target", json_object_new_string (targetPackageFile.c_str()));

    if (authToken)
	json_object_object_add (downloadParams, "authToken", json_object_get(authToken));

    if (deviceId)
	json_object_object_add (downloadParams, "deviceId", json_object_get(deviceId));


    json_object_object_add (downloadParams, "subscribe", json_object_new_boolean (subscribe));

    g_debug ("sending download request to download manager with params: %s\n", json_object_to_json_string (downloadParams));
    ApplicationManager::DownloadRequest* downloadReq = new ApplicationManager::DownloadRequest (ticket, subscribe);

    retval = LSCall (lshandle, "palm://com.palm.downloadmanager/download", json_object_to_json_string (downloadParams),
	    ApplicationManager::cbDownloadManagerUpdate, downloadReq, NULL, &lserror); 
    json_object_put (downloadParams);

    if (!retval) {
	LSErrorPrint (&lserror, stderr);
	LSErrorFree (&lserror);
	delete downloadReq;
	return false;
    }

    return true;
}

bool ApplicationInstaller::install(const std::string& targetPackageFile, unsigned int uncompressedPackageSizeInKB, const unsigned long ticket) {
	
	if (targetPackageFile.size() == 0)
		return false;
	
	//start a detached install procedure
		//set an install to start when the main loop gets the next chance to exec something
	InstallParams * installParams = new InstallParams(targetPackageFile, "", ticket,
													  NULL,NULL,uncompressedPackageSizeInKB);			//MEMALLOC: reclaim:cbInstall_detached
	processOrQueueCommand(installParams);
	return true;
}

void ApplicationInstaller::notifyAppInstalled(const std::string& appId,const std::string& appVersion) {
	
	if (!m_service)
		return;

	//update the "notify status" subscriptions
			
	std::string ls_payload = std::string("{ \"appId\":\"")+appId+std::string("\" , \"version\":\"")+appVersion+std::string("\" , \"statusChange\":\"INSTALLED\" }");
	LSError lserror;
	LSErrorInit(&lserror);
	LSSubscriptionReply(this->m_service,appId.c_str(),ls_payload.c_str(),&lserror);
	if (LSErrorIsSet(&lserror))
		LSErrorFree(&lserror);

	//also reply to the "all" subscription
	LSErrorInit(&lserror);
	LSSubscriptionReply(this->m_service,"*",ls_payload.c_str(),&lserror);
	if (LSErrorIsSet(&lserror))
		LSErrorFree(&lserror);
			
}
	
void ApplicationInstaller::notifyAppRemoved(const std::string& appId,const std::string& appVersion,int cause) {
	
	//update the "notify status" subscriptions

	//FOR NOW, IF IT'S A SYSAPP, DON'T NOTIFY
	if (appId.find("com.palm.sysapp") == 0)
		return;

	std::string causeCode;
	switch (cause) {
	case APPREMOVED_CAUSE_APPREVOKED:
		causeCode = "REVOKED";
		break;
	case APPREMOVED_CAUSE_USERDELETED:
		causeCode = "USER";
		break;
	default:
		causeCode = "UNKNOWN";
	}
	
	std::string ls_payload = std::string("{ \"appId\":\"")+appId+std::string("\" , \"version\":\"")
								+appVersion+std::string("\" , \"statusChange\":\"REMOVED\" , \"cause\":\"")+causeCode+std::string("\" }");
	LSError lserror;
	LSErrorInit(&lserror);
	LSSubscriptionReply(this->m_service,appId.c_str(),ls_payload.c_str(),&lserror);
	if (LSErrorIsSet(&lserror))
		LSErrorFree(&lserror);

	//also reply to the "all" subscription
	LSErrorInit(&lserror);
	LSSubscriptionReply(this->m_service,"*",ls_payload.c_str(),&lserror);
	if (LSErrorIsSet(&lserror))
		LSErrorFree(&lserror);
		
}

//static 
bool ApplicationInstaller::extractVersionFromAppInfo(const std::string& appBaseDir,std::string& r_versionString)
{
	if (appBaseDir.empty())
		return false;
	
	////   (COPIED FROM APPLICATION MANAGER'S scanOneApplicationFolder() and modified
	// Do we have a locale setting
	std::string locale = Preferences::instance()->locale();

	// Look for the language specific appinfo.json
	std::string appJsonPath = appBaseDir + "/resources/" + locale + "/appinfo.json";

	r_versionString = ApplicationDescription::versionFromFile(appJsonPath, appBaseDir);
	if (r_versionString.empty())
	{
		appJsonPath = appBaseDir + "/appinfo.json";
		r_versionString = ApplicationDescription::versionFromFile(appJsonPath, appBaseDir);
		if (r_versionString.empty())
			return false;
	}
	return true;
}


//------------------------------------------------ DEBUG -----------------------------------------------------------
	
//static 
int ApplicationInstaller::dbg_statfs(const char * fsPath, struct statfs * buf)
{
	if ((buf == NULL) || (fsPath == NULL))
		return -1;

	struct stat fsStat;
	memset(buf,0,sizeof(struct statfs));
	memset(&fsStat,0,sizeof(struct stat));
	
	if (::statfs(fsPath,buf) != 0)
		return -1;
	
	if (::stat(fsPath,&fsStat) != 0)
	{
		return -1;
	}
	
	//if there is no override entry for this path in the map, then just return what statfs returned
	std::map<uint32_t,std::pair<uint32_t,uint32_t> >::iterator it = dbg_statfs_map.find(fsStat.st_dev);
	if (it == dbg_statfs_map.end())
		return 0;
	
	//modify the struct with the override size
	/*
	 * f_bsize 	 Optimal block size of the file system.
	 * f_blocks 	Total number of blocks in the system.
	 * f_bfree 	Number of free blocks in the file system. The size of a free block is given in the f_bsize field.
	 * f_bavail 	Number of free blocks available to a nonroot user.
	 */
	
	uint32_t originalFsSize = it->second.first;
	uint32_t forcedFsSize = it->second.second;
	
	uint32_t diff_bfree = 0;
	if (buf->f_bfree < originalFsSize)
		diff_bfree = originalFsSize - buf->f_bfree;
	
	buf->f_bfree = (diff_bfree < forcedFsSize ? (forcedFsSize - diff_bfree) : 0);
	buf->f_bavail = buf->f_bfree;
	if (buf->f_blocks < buf->f_bavail)
		buf->f_blocks = buf->f_bavail;
	
	g_warning("(DEBUG-FN) %s: returning %lu blocks available (%d blocksize) for fs path [%s], id = %u",
				__FUNCTION__,buf->f_bavail,buf->f_bsize,fsPath,it->first);
	return 0;
}

//static 
int ApplicationInstaller::dbg_statvfs(const char * fsPath,struct statvfs * buf)
{
	if ((buf == NULL) || (fsPath == NULL))
		return -1;

	struct stat fsStat;
	memset(buf,0,sizeof(struct statvfs));
	memset(&fsStat,0,sizeof(struct stat));

	if (::statvfs(fsPath,buf) != 0)
		return -1;

	if (::stat(fsPath,&fsStat) != 0)
	{
		return -1;
	}

	//if there is no override entry for this path in the map, then just return what statfs returned
	std::map<uint32_t,std::pair<uint32_t,uint32_t> >::iterator it = dbg_statvfs_map.find(fsStat.st_dev);
	if (it == dbg_statvfs_map.end())
		return 0;

	//modify the struct with the override size - warning: overflow opportunity galore!
	/* u_long      f_bsize;             preferred file system block size 
	 * u_long      f_frsize;            fundamental filesystem block
	                                          (size if supported) 
	 * fsblkcnt_t  f_blocks;            total # of blocks on file system
	                                          in units of f_frsize 
	 * fsblkcnt_t  f_bfree;              total # of free blocks 
	 * fsblkcnt_t  f_bavail;             # of free blocks avail to
	                                          non-privileged user 
	*/
	
	uint32_t originalFsSize = it->second.first;
	uint32_t forcedFsSize = it->second.second;

	uint32_t diff_bfree = 0;
	if (buf->f_bfree < originalFsSize)
		diff_bfree = originalFsSize - buf->f_bfree;

	buf->f_bfree = (diff_bfree < forcedFsSize ? (forcedFsSize - diff_bfree) : 0);
		
	buf->f_bavail = buf->f_bfree;
	if (buf->f_blocks < buf->f_bavail)
		buf->f_blocks = buf->f_bavail;

	g_warning("(DEBUG-FN) %s: returning %lu blocks available (%lu fundamental blocksize, %lu preferred blocksize) for fs path [%s], id = %u",
			__FUNCTION__,buf->f_bavail,buf->f_frsize,buf->f_bsize,fsPath,it->first);
	return 0;
}
	
//static 

int ApplicationInstaller::dbg_fill(std::string& path,uint32_t& bsize,uint32_t& nblocks)
{
	if (bsize == 0)
		bsize = 4096;
	if (nblocks == 0)
		nblocks = 1;
	
	std::string filestr = path + std::string("/")+std::string("fileXXXXXX");
	char * templ = new char[filestr.size()+2];
	strcpy(templ,filestr.c_str());
	char * fillbuff = NULL;
	
	int fd;
	int rc = 1;

	if ((fd = mkstemp(templ)) < 0)
	{
		rc = -1;
		goto Done_dbg_fill;
	}
	
	fillbuff = new char[bsize];
	memset(fillbuff,1,bsize);
	
	for (uint32_t i=0;i<nblocks;++i)
	{
		int n;
		if ( (n = write(fd,fillbuff,bsize)) <= 0)
		{
			rc = -2;
			goto Done_dbg_fill;
		}
		if (n < (int)bsize)
			g_warning("%s - Partial write (%d / %u)",__FUNCTION__,n,bsize);
	}

Done_dbg_fill:

	if (rc > 0)
		path = std::string(templ);
	delete [] templ;
	delete [] fillbuff;
	if (fd > 0)
		close(fd);
	return rc;
}

//static 
int ApplicationInstaller::dbg_getAppSizeOnFs(const std::string& fspath,const std::string& appbasepath,uint64_t& r_size)
{
	r_size = getSizeOfAppOnFs(fspath,appbasepath);
	if (r_size == 0)
		return 0;
	
	return 1;
}

////---------------------------------------------------- ALL NON-CLASS , NON-FRIEND FUNCTIONS BELOW HERE -------------------------------------------------------------------------------

static void util_LSSubReplyWithRelay_IgnoreError(LSHandle* lshandle,const std::string& subscriptionKey,const unsigned long ticket,const std::string& payload) {
	
	//also reply via the relay
	ApplicationManager::instance()->relayStatus(payload,ticket);
		
	if (lshandle == NULL)
		return;
	
	LSError lserror;
	LSErrorInit(&lserror);
	LSSubscriptionReply(lshandle,subscriptionKey.c_str(),payload.c_str(),&lserror);
	if (LSErrorIsSet(&lserror)) {
		g_warning("ApplicationInstaller::util_LSSubReplyWithRelay_IgnoreError(): couldn't send update for subscription key [%s]: payload = [%s]",
				subscriptionKey.c_str(),payload.c_str());
		LSErrorFree(&lserror);
	}
	
}

/*
 * Be very careful! these don't check path validity - Check the path validity completely in the caller!
 * These are not called from anywhere but inside this file 
 */
static void util_cleanup_tempDir(const std::string& tmpDirPath) {
	
	std::string cmdline = std::string("rm -fr ")+tmpDirPath;

	int ret = system(cmdline.c_str());
	Q_UNUSED(ret);
}

/*
 * Be very careful! doesn't check path validity!
 */
static void util_cleanup_installDir(const std::string& installDirPath) {
	
	std::string cmdline = std::string("rm -fr ")+installDirPath;
	int ret = system(cmdline.c_str());
	Q_UNUSED(ret);
}


/**
 * Check for valid URI and ipk
 */
bool ApplicationInstaller::isValidInstallURI(const std::string& url)
{
	QUrl qurl(url.c_str());
	if (qurl.isValid() && ApplicationManager::isAppPackage(qurl.path().toAscii()))
		return true;
	
	return false;
}

void ApplicationInstaller::processOrQueueCommand(CommandParams* cmd)
{
    s_commandParams.push_back(cmd);
	if (s_commandParams.size() == 1) {
		// First command in queue. start executing
		while (processNextCommand()) {}
	}
}

// returns true if it should be called again
bool ApplicationInstaller::processNextCommand()
{
	// Do not process install or remove when in first-use mode. This is ok
	// to do, ApplicationInstallerService will recall when sysmgr restarts
	if (G_UNLIKELY(Settings::LunaSettings()->uiType == Settings::UI_MINIMAL))
		return false;	
	
	if (s_commandParams.empty())
		return false;

	if (m_inBrickMode)
		return false;

	if (m_cmdState.processing)
		return false;
	
	CommandParams* cmd = s_commandParams.front();
	bool ret;
	
    switch (cmd->_type) {
	case (CommandParams::Install): {
		g_warning("Processing install command");
		ret = processInstallCommand(static_cast<InstallParams*>(cmd));		
		break;
	}
	case (CommandParams::Remove): {
		g_warning("Processing remove command");
		ret = processRemoveCommand(static_cast<RemoveParams*>(cmd));	
		break;
	}
	default:
		g_critical("%s:%d Unknown command param type: %d",
				   __PRETTY_FUNCTION__, __LINE__, cmd->_type);
		ret = false;
	}

	if (ret) {
		// Command started. don't call me again
		return false;
	}

	// Command failed
	g_critical("%s:%d Command failed: %d", __PRETTY_FUNCTION__, __LINE__,
			  cmd->_type);
	s_commandParams.pop_front();
	delete cmd;
	return true; // call me again
}

bool ApplicationInstaller::processInstallCommand(InstallParams* params)
{
	closeApp(params->_id);
	
	gchar* argv[16] = {0};			///WARNING! look out below if number of params goes > size of this array (keep them in sync)
	GError* gerr = NULL;
	GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_SEARCH_PATH |
									  G_SPAWN_STDERR_TO_DEV_NULL |
									  G_SPAWN_DO_NOT_REAP_CHILD);
	GPid childPid;
	gint childStdoutFd;
	gboolean result;
	int index = 0;

		
	char tmpBuf[64];
	sprintf(tmpBuf, "%d", params->_uncompressedSizeInKB);

	argv[index++] = (gchar *) "/usr/sbin/setcpushares-task";
	argv[index++] = (gchar *) "/usr/bin/ApplicationInstallerUtility";
 	argv[index++] = (gchar *) "-c";
	argv[index++] = (gchar *) "install";
	argv[index++] = (gchar *) "-p";
	argv[index++] = (gchar *) params->_target.c_str();
	argv[index++] = (gchar *) "-u";
	argv[index++] = (gchar *) tmpBuf;
	if (params->_verify)
	 	argv[index++] = (gchar*) "-v";
	if (params->_sysMode)
		argv[index++] = (gchar*) "-s";
	argv[index] = NULL;

	result = g_spawn_async_with_pipes(NULL,
									  argv,
									  NULL,
									  flags,
									  NULL,
									  NULL,
									  &childPid,
									  NULL,
									  &childStdoutFd,
									  NULL,
									  &gerr);

	if (result) {
		params->_childStdOutChannel = g_io_channel_unix_new(childStdoutFd);
		params->_childStdOutSource = g_io_create_watch(params->_childStdOutChannel, G_IO_IN);
		g_source_set_callback(params->_childStdOutSource, (GSourceFunc) util_ipkgInstallIoChannelCallback,
							  params, NULL);
		g_source_attach(params->_childStdOutSource, g_main_loop_get_context(HostBase::instance()->mainLoop()));

		m_cmdState.processing = true;
		m_cmdState.pid = childPid;
		m_cmdState.sourceId = g_child_watch_add_full(G_PRIORITY_DEFAULT_IDLE, childPid,
													 util_ipkgInstallDone,
													 params, NULL);		
		return true;
	}

	if (gerr) {
		g_critical("%s:%d: Failed to execute ApplicationInstallerUtility command: %s",
				   __FUNCTION__, __LINE__, gerr->message);
		g_error_free(gerr);
	}

	
	std::string message = "FAILED_IPKG_INSTALL";
	std::string ls_sub_key = toSTLString<long>(params->ticketId);
	std::string ls_payload = std::string("{ \"ticket\":") + ls_sub_key
							 +std::string(" , \"status\":\"") + message+std::string("\"")
							 +std::string(" }");
	
	//update the "chained" subscriptions that came off the application manager/download manager
	util_LSSubReplyWithRelay_IgnoreError(params->_lshandle,ls_sub_key,params->ticketId,ls_payload);

	return false;
}

bool ApplicationInstaller::processRemoveCommand(RemoveParams* removeParams)
{
	// FIXME:  Move to ApplicationInstallerUtility

	int rc = ApplicationInstaller::instance()->lunasvcRemove(removeParams);

	if (rc != REMOVER_RETURNC__SUCCESS) {

		std::string message;

		if (rc == REMOVER_RETURNC__FAILEDIPKGREMOVE)
			message = "FAILED_IPKG_REMOVE";
		else {
			message = "FAILED_INTERNAL";
			g_debug("processRemoveCommand: unknown return code from lunasvcremove(): %d\n",rc);
		}
		
		std::string ls_sub_key = toSTLString<long>(removeParams->ticketId);
		std::string ls_payload = std::string("{ \"ticket\":")
		+ls_sub_key
		+std::string(" , \"status\":\"")+message+std::string("\"")
		+std::string(" }");

		util_LSSubReplyWithRelay_IgnoreError((LSHandle*) removeParams->_lshandle,ls_sub_key,removeParams->ticketId,ls_payload);
	}

	return rc == REMOVER_RETURNC__SUCCESS;
}

void ApplicationInstaller::oneCommandProcessed()
{
	luna_assert(!s_commandParams.empty());

	CommandParams* cmd = s_commandParams.front();
	s_commandParams.pop_front();
	delete cmd;

	m_cmdState.reset();
	
	while (processNextCommand()) {}
}

void ApplicationInstaller::slotMediaPartitionAvailable(bool val)
{
	if (val)
		exitBrickMode();
	else
		enterBrickMode();
}

void ApplicationInstaller::enterBrickMode()
{
	if (m_inBrickMode)
		return;

	g_message("%s", __PRETTY_FUNCTION__);
	m_inBrickMode = true;  
	
	if (m_cmdState.processing) {

		g_message("%s: Currently processing command. Stopping it",
				  __PRETTY_FUNCTION__);

		luna_assert(!s_commandParams.empty());

		CommandParams* cmd = s_commandParams.front();
		if (cmd->_childStdOutChannel) {
			g_io_channel_unref(cmd->_childStdOutChannel);
			cmd->_childStdOutChannel = 0;
		}
		
		if (cmd->_childStdOutSource) {
			g_source_destroy(cmd->_childStdOutSource);
			g_source_unref(cmd->_childStdOutSource);
			cmd->_childStdOutSource = 0;			
		}

		g_source_remove(m_cmdState.sourceId);
		
		int status;
		::kill(m_cmdState.pid, SIGKILL);		
		::waitpid(m_cmdState.pid, &status, WNOHANG);

		m_cmdState.reset();
	}
}

void ApplicationInstaller::exitBrickMode()
{
	if (!m_inBrickMode)
		return;

	g_message("%s", __PRETTY_FUNCTION__);
	m_inBrickMode = false;

	util_validateCryptofs();	

	// This should not happen
	if (m_cmdState.processing)
		return;
	
	// Resume any pending install/remove commands
	while (processNextCommand()) {}
}

bool ApplicationInstaller::allowSuspend()
{
	return !m_cmdState.processing;
}

json_object * ApplicationInstaller::packageInfoFileToJson(const std::string& packageId)
{
	if (packageId.size() == 0)
		return 0;

	const std::string packageFolderPath(Settings::LunaSettings()->packageInstallBase + std::string("/") + Settings::LunaSettings()->packageInstallRelative + std::string("/")+packageId);

	// Do we have a locale setting
	std::string locale = Preferences::instance()->locale();

	// Look for the language/region specific packageinfo.json

	std::string language, region;
	std::size_t underscorePos = locale.find("_");
	if (underscorePos != std::string::npos) {
		language = locale.substr(0, underscorePos);
		region = locale.substr(underscorePos+1);
	}

	json_object * root = 0;
	std::string packageJsonPath;

	if (!language.empty() && !region.empty()) {
		packageJsonPath = packageFolderPath + "/resources/" + language + "/" + region +"/packageinfo.json";
		root = json_object_from_file(const_cast<char *>(packageJsonPath.c_str()));
	}

	if ((!root) || is_error(root)) {
		// try the language-only one
		packageJsonPath = packageFolderPath + "/resources/" + language + "/packageinfo.json";
		root = json_object_from_file(const_cast<char *>(packageJsonPath.c_str()));
	}

	if ((!root) || is_error(root)) {
		//try the old version
		packageJsonPath = packageFolderPath + "/resources/" + locale + "/packageinfo.json";
		root = json_object_from_file(const_cast<char *>(packageJsonPath.c_str()));
	}

	if ((!root) || is_error(root)) {
		// FIXME: AppId needs to be based on folder name (and not specified in appinfo.json)
		// try the default one
		packageJsonPath = packageFolderPath + "/packageinfo.json";
		root = json_object_from_file(const_cast<char *>(packageJsonPath.c_str()));
	}

	if ((!root) || is_error(root))
		return 0;
	return root;				//MUST BE DEALLOCATED ELSEWHERE
}

static int runScriptCwd(const std::string& scriptFile,const std::string& cwd)
{
	gchar * argv[4];
	gchar * envp[2];
	GError * gerr = NULL;

	GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL);

	argv[0] = (gchar*)"sh"; 
	argv[1] = (gchar*)"-c"; 
	argv[2] = (gchar*)scriptFile.c_str();
	argv[3] = NULL;

	std::string envCWD = std::string("PWD=")+cwd;
	envp[0] = (gchar *)envCWD.c_str();
	envp[1] = NULL;
	int exit_status;
	
	g_warning("%s: executing: %s %s %s",__FUNCTION__,argv[0],argv[1],argv[2]);

	gboolean resultStatus = g_spawn_sync(cwd.c_str(),
			argv,
			envp,
			flags,
			NULL,
			NULL,
			NULL,
			NULL,
			&exit_status,
			&gerr);

	if (gerr) {
		g_warning("%s: error: %s",__FUNCTION__,gerr->message);
		g_error_free(gerr);
		gerr=NULL;
		return 0;
	}

	if (resultStatus) {
		if (isNonErrorProcExit((int)exit_status) == false) {
			g_warning("%s: error: result status is %d , error code = %d",__FUNCTION__,resultStatus,exit_status);
			return 0;
		}
	}
	else {
		//failed to exec script
		g_warning("%s: error: spawn failed",__FUNCTION__);
		return 0;
	}
	
	return 1;
}

void ApplicationInstaller::closeApp(const std::string& appId)
{
	if (appId.empty())
		return;

	if (WebAppMgrProxy::instance()->connected() == false)
		return;			//WebKit not initialized yet

	bool ret = false;
	
	WebAppMgrProxy::instance()->getIpcChannel()->sendSyncMessage(
		new View_Mgr_SyncKillApp(appId, &ret));
}
