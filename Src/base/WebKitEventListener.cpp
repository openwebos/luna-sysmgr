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

#include "WebKitEventListener.h"
#include "BackupManager.h"
#include "Logging.h"


WebKitEventListener::WebKitEventListener(BackupManager* backupMgr) :
	m_backupMgr(backupMgr)
{
	luna_assert(backupMgr != NULL);
}

void WebKitEventListener::dbDumpStarted( const Palm::DbBackupStatus& status, void* userData )
{
	m_backupMgr->dbDumpStarted(status, userData);
}

void WebKitEventListener::dbDumpStopped( const Palm::DbBackupStatus& status, void* userData )
{
	m_backupMgr->dbDumpStopped(status, userData);
}

void WebKitEventListener::dbRestoreStarted( const Palm::DbBackupStatus& status, void* userData )
{
	m_backupMgr->dbRestoreStarted(status, userData);
}

void WebKitEventListener::dbRestoreStopped( const Palm::DbBackupStatus& status, void* userData )
{
	m_backupMgr->dbRestoreStopped(status, userData);
}

void WebKitEventListener::dbMoveStatus( int err )
{
	if (err == 0) {
		g_message("Successfully moved HTML5 databases");
	}
	else {
		g_error("ERROR %d moving HTML5 databases", err);
	}
}
