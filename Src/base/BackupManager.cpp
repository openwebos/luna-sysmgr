/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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
#include <sys/prctl.h>

#include <string>
#include <map>
#include "BackupManager.h"
#include "Settings.h"
#include "HostBase.h"
#include "JSONUtils.h"
#include "Logging.h"
#include <cjson/json.h>

//for launcher3 saving
#include "pagesaver.h"

/* BackupManager implementation is based on the API documented at https://wiki.palm.com/display/ServicesEngineering/Backup+and+Restore+2.0+API
 * On the LunaSysMgr side, this backs up launcher, quick launch and dock mode settings
 * On the WebAppMgr side, this backs up the sysmgr cookies
 */
BackupManager* BackupManager::s_instance = NULL;

/**
 * We use the same API for backing up HTML5 databases as we do the cookie
 * database. This is a phony appid that we use to identify the cookie db entry.
 */
static const char * strCookieAppId = "com.palm.luna-sysmgr.cookies";
static const char * strCookieTempFile = "/tmp/com.palm.luna-sysmgr.cookies-html5-backup.sql";


/* this is the browser db file that was previously backed up by sysmgr, but will now move to mojodb.
 * this is needed only for the first restore after the ota for blowfish
 */
static const char * strBrowserDbFile = "/tmp/com.palm.app.browser-html5-backup.sql";
static const char * strBrowserDbUrl = "file:///usr/palm/applications/com.palm.app.browser/index.html:0";
/*! \page com_palm_app_data_backup Service API com.palm.appDataBackup/
 *  Public methods:
 *  - \ref com_palm_app_data_backup_post_restore
 *  - \ref com_palm_app_data_backup_pre_backup
 */
/**
 * These are the methods that the backup service can call when it's doing a 
 * backup or restore.
 */
LSMethod BackupManager::s_BackupServerMethods[]  = {
	{ "preBackup"  , BackupManager::preBackupCallback },
	{ "postRestore", BackupManager::postRestoreCallback },
    { 0, 0 }
};


BackupManager::BackupManager() :
	m_mainLoop(NULL)
	, m_clientService(NULL)
	, m_serverService(NULL)
{
}

/**
 * Initialize the backup manager.
 */
bool BackupManager::init(GMainLoop* mainLoop)
{
    luna_assert(m_mainLoop == NULL);	// Only initialize once.
    m_mainLoop = mainLoop;

    LSError error;
    LSErrorInit(&error);
    char procName[100];

    // this service is expected to run from WebAppMgr process
    m_strBackupServiceName = "com.palm.appDataBackup";
    m_doBackupFiles = true;
    m_doBackupCookies = true;

    bool succeeded = LSRegisterPalmService(m_strBackupServiceName.c_str(), &m_serverService, &error);
    if (!succeeded) {
	g_warning("Failed registering on service bus: %s", error.message);
	LSErrorFree(&error);
	return false;
    }

    succeeded = LSPalmServiceRegisterCategory( m_serverService, "/", s_BackupServerMethods, NULL,
	    NULL, this, &error);
    if (!succeeded) {
	g_warning("Failed registering with service bus category: %s", error.message);
	LSErrorFree(&error);
	return false;
    }

    succeeded = LSGmainAttachPalmService(m_serverService, m_mainLoop, &error);
    if (!succeeded) {
	g_warning("Failed attaching to service bus: %s", error.message);
	LSErrorFree(&error);
	return false;
    }

    m_clientService = LSPalmServiceGetPrivateConnection(m_serverService);
    if (NULL == m_clientService) {
	g_warning("unable to get private handle to the backup service");
	return false;
    }

    return succeeded;
}

BackupManager::~BackupManager()
{
    if (m_serverService) {
	LSError error;
	LSErrorInit(&error);

	bool succeeded = LSUnregisterPalmService(m_serverService, &error);
	if (!succeeded) {
	    g_warning("Failed unregistering backup service: %s", error.message);
	    LSErrorFree(&error);
	}
    }
}


void BackupManager::initFilesForBackup()
{
//	if (!m_backupFiles.empty())
//		return;

	g_message("%s: entry",__FUNCTION__);
	m_backupFiles.clear();

    if (m_doBackupFiles) {
    	g_message("%s: adding files to backup list",__FUNCTION__);
		m_backupFiles.push_back ("/var/luna/preferences/launcher-cards.json");
		if (g_file_test(Settings::LunaSettings()->firstCardLaunch.c_str(), G_FILE_TEST_EXISTS))
			m_backupFiles.push_back (Settings::LunaSettings()->firstCardLaunch.c_str());
		if (g_file_test(Settings::LunaSettings()->quicklaunchUserPositions.c_str(), G_FILE_TEST_EXISTS))
			m_backupFiles.push_back (Settings::LunaSettings()->quicklaunchUserPositions.c_str());
		if (g_file_test(Settings::LunaSettings()->dockModeUserPositions.c_str(), G_FILE_TEST_EXISTS))
			m_backupFiles.push_back (Settings::LunaSettings()->dockModeUserPositions.c_str());

		QList<QString> fileList;

		DimensionsSystemInterface::PageSaver::filesForBackup(&fileList);
		for (QList<QString>::const_iterator file_it = fileList.constBegin();
				file_it != fileList.constEnd();file_it++)
		{
			if (!(file_it->isEmpty()))
			{
				if (g_file_test(file_it->toUtf8().constData(), G_FILE_TEST_EXISTS))
				{
					m_backupFiles.push_back(file_it->toUtf8().constData());
					g_message("%s: backing up signalled file: %s",__FUNCTION__,file_it->toUtf8().constData());
				}
			}
		}
    }
    else
    {
    	g_message("%s: DID NOT add files to backup list (backup files? flag was false)",__FUNCTION__);
    }
}

BackupManager* BackupManager::instance()
{
	if (NULL == s_instance) {
		s_instance = new BackupManager();
	}

	return s_instance;
}

/*!
\page com_palm_app_data_backup
\n
\section com_palm_app_data_backup_pre_backup preBackup

\e Public.

com.palm.appDataBackup/preBackup

Make a backup of LunaSysMgr.

\subsection com_palm_app_data_backup_pre_backup_syntax Syntax:
\code
{
}
\endcode

\subsection com_palm_app_data_backup_pre_backup_returns Returns:
\code
{
    "description": string,
    "version": string,
    "files": [ string array ]
}
\endcode

\param descrition Describes the backup.
\param version Version information.
\param files String array of files included in the backup.

\subsection com_palm_app_data_backup_pre_backup_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.appDataBackup/preBackup '{}'
\endcode

Example response for a succesful call:
\code
{
    "description": "Backup of LunaSysMgr files for launcher, quicklaunch, dockmode and sysmgr cookies",
    "version": "1.0",
    "files": [
        "\/var\/luna\/preferences\/used-first-card",
        "\/var\/palm\/user-exhibition-apps.json",
        "\/var\/luna\/preferences\/launcher3\/launcher_fixed.msave",
        "\/var\/luna\/preferences\/launcher3\/page_ReorderablePage_APPS_{eb1b2baa-dbe6-4d51-9ec2-2517fdd284ac}",
        "\/var\/luna\/preferences\/launcher3\/page_ReorderablePage_DOWNLOADS_{88540c1e-7dc2-4f0f-b4aa-3721aab97ab7}",
        "\/var\/luna\/preferences\/launcher3\/page_ReorderablePage_FAVORITES_{b83a9aa7-22f4-4ac8-b38a-4a68379ecd31}",
        "\/var\/luna\/preferences\/launcher3\/page_ReorderablePage_SETTINGS_{6890fced-9122-4498-bbb1-50cb31b189b7}",
        "\/var\/luna\/preferences\/launcher3\/quicklaunch_fixed.qlsave"
    ]
}
\endcode
/**
 * Called by the backup service for all four of our callback functions: preBackup, 
 * postBackup, preRestore, postRestore.
 */
bool BackupManager::preBackupCallback( LSHandle* lshandle, LSMessage *message, void *user_data)
{
	BackupManager* pThis = static_cast<BackupManager*>(user_data);
	luna_assert(pThis != NULL);

	// payload is expected to have the following fields - 
	// incrementalKey - this is used primarily for mojodb, backup service will handle other incremental backups
	// maxTempBytes - this is the allowed size of upload, currently 10MB (more than enough for our backups)
	// tempDir - directory to store temporarily generated files (currently unused by us)
	// - Since none of these are used now, we do not need to parse the payload


	// the response has to contain
	// description - what is being backed up
	// files - array of files to be backed up
	// version - version of the service
	struct json_object* response = json_object_new_object();
	if (!response) {
	    g_warning ("Unable to allocate json object");
	    return true;
	}

	json_object_object_add (response, "description", json_object_new_string ("Backup of LunaSysMgr files for launcher, quicklaunch, dockmode and sysmgr cookies"));
	json_object_object_add (response, "version", json_object_new_string ("1.0"));

	// adding the files for backup at the time of request. 
	// if the user has created custom quicklaunch or dockmode settings, those files should be available now.
	pThis->initFilesForBackup();

	struct json_object* files = json_object_new_array();
	GFileTest fileTest = static_cast<GFileTest>(G_FILE_TEST_EXISTS|G_FILE_TEST_IS_REGULAR);

	if (pThis->m_doBackupFiles) {
	    std::list<std::string>::const_iterator i;
	    for (i = pThis->m_backupFiles.begin(); i != pThis->m_backupFiles.end(); ++i) {
			if (g_file_test(i->c_str(), fileTest)) {
				json_object_array_add (files, json_object_new_string(i->c_str()));
				g_debug ("added file %s to the backup list", i->c_str());
			}
	    }
	}

	if (pThis->m_doBackupCookies) {
	    //FIXME-qtwebkit: bool succeeded = Palm::WebGlobal::startDatabaseDump (Palm::k_PhonyCookieUrl, "cookies", strCookieTempFile, NULL);
		if (g_file_test(strCookieTempFile, fileTest)) {
			// for cookies this call is synchronous
			json_object_array_add (files, json_object_new_string (strCookieTempFile));
			g_debug ("added cookies file %s to the backup list", strCookieTempFile);
		}
	}

	json_object_object_add (response, "files", files);

	LSError lserror;
	LSErrorInit(&lserror);

	g_message ("Sending response to preBackupCallback: %s", json_object_to_json_string (response));
	if (!LSMessageReply (lshandle, message, json_object_to_json_string(response), &lserror )) {
	    g_warning("Can't send reply to preBackupCallback error: %s", lserror.message);
	    LSErrorFree (&lserror); 
	}

	json_object_put (response);
	return true;
}

/*!
\page com_palm_app_data_backup
\n
\section com_palm_app_data_backup_post_restore postRestore

\e Public.

com.palm.appDataBackup/postRestore

Restore a backup of LunaSysMgr.

\subsection com_palm_app_data_backup_post_restore_syntax Syntax:
\code
{
    "files" : [string array]
}
\endcode

\param files List of backup files.

\subsection com_palm_app_data_backup_post_restore_returns Returns:
\code
{
    "returnValue": boolean
}
\endcode

\param returnValue Indicates if the call was succesful.

\subsection com_palm_app_data_backup_post_restore_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.appDataBackup/postRestore '{
    "files": [
        "/var/luna/preferences/used-first-card",
        "/var/palm/user-exhibition-apps.json",
        "/var/luna/preferences/launcher3/launcher_fixed.msave",
        "/var/luna/preferences/launcher3/page_ReorderablePage_APPS_{eb1b2baa-dbe6-4d51-9ec2-2517fdd284ac}",
        "/var/luna/preferences/launcher3/page_ReorderablePage_DOWNLOADS_{88540c1e-7dc2-4f0f-b4aa-3721aab97ab7}",
        "/var/luna/preferences/launcher3/page_ReorderablePage_FAVORITES_{b83a9aa7-22f4-4ac8-b38a-4a68379ecd31}",
        "/var/luna/preferences/launcher3/page_ReorderablePage_SETTINGS_{6890fced-9122-4498-bbb1-50cb31b189b7}",
        "/var/luna/preferences/launcher3/quicklaunch_fixed.qlsave"
    ]
}'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode
*/
bool BackupManager::postRestoreCallback( LSHandle* lshandle, LSMessage *message, void *user_data)
{
    BackupManager* pThis = static_cast<BackupManager*>(user_data);
    luna_assert(pThis != NULL);

    // {"files" : array}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_1(REQUIRED(files, array)));

    const char* str = LSMessageGetPayload(message);
    if (!str)
	return true;

    g_warning ("[BACKUPTRACE] %s: received %s", __func__, str);
    json_object* root = json_tokener_parse(str);
    if (!root || is_error(root))
	return true;

    json_object* files = json_object_object_get (root, "files");
    if (!files) {
	g_warning ("No files specified in postRestore message");
	return true;
    }

    array_list* fileArray = json_object_get_array (files);
    if (!fileArray) {
	g_warning ("files is not an array");
	return true;
    }

    int fileArrayLength = array_list_length (fileArray);
    int index = 0;

    g_debug ("%s: fileArrayLength = %d", __func__, fileArrayLength);

    if (pThis->m_doBackupCookies) {
	for (index = 0; index < fileArrayLength; ++index) {
	    json_object* obj = (json_object*) array_list_get_idx (fileArray, index);
	    if (!obj)
		continue;

	    std::string path = json_object_get_string (obj);
	    if (path == strCookieTempFile) {
		/*FIXME-qtwebkit: bool succeeded = Palm::WebGlobal::startDatabaseRestore(Palm::k_PhonyCookieUrl, "cookies", path, NULL);
		// this call is synchronous for cookies
		if (!succeeded) {
		    g_warning ("Unable to restore the cookies");
		}
		else {
		    g_debug ("cookies restored from %s to %s", path.c_str(), Palm::k_PhonyCookieUrl);
		}*/
	    }
	    if (path == strBrowserDbFile) {
		/*FIXME-qtwebkit: bool succeeded = Palm::WebGlobal::startDatabaseRestore(strBrowserDbUrl, "browser_data", path, NULL);
		if (!succeeded) {
		    g_warning ("Unable to restore the browser_data database");
		}
		else {
		    g_debug ("browser_data database restored");
		}*/
	    }
	}
    }

    // no work needed for regular files
    LSError lserror;
    LSErrorInit(&lserror);
    struct json_object* response = json_object_new_object();

    json_object_object_add (response, "returnValue", json_object_new_boolean(true));

    g_message ("Sending response to postRestoreCallback: %s", json_object_to_json_string (response));
    if (!LSMessageReply (lshandle, message, json_object_to_json_string(response), &lserror )) {
	g_warning("Can't send reply to postRestoreCallback error: %s", lserror.message);
	LSErrorFree (&lserror); 
    }

    json_object_put (response);
    return true;

}


void BackupManager::dbDumpStarted( const Palm::DbBackupStatus& status, void* userData )
{
	g_message("Started database dump %s err: %d", status.url.c_str(), status.err);
}

void BackupManager::dbDumpStopped( const Palm::DbBackupStatus& status, void* userData )
{
	g_message("Stopped database dump %s err: %d",  status.url.c_str(), status.err);
}

void BackupManager::dbRestoreStarted( const Palm::DbBackupStatus& status, void* userData )
{
	g_message("Started restore of %s err: %d", status.url.c_str(), status.err);
}

void BackupManager::dbRestoreStopped( const Palm::DbBackupStatus& status, void* userData )
{
	g_message("Stopped restore of %s err: %d", status.url.c_str(), status.err);
}


