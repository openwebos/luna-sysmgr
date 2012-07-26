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




#ifndef BACKUP_MANAGER_H
#define BACKUP_MANAGER_H

#include "Common.h"

#include <string>
#include <list>
#include <map>
#include "lunaservice.h"
#include <QObject>
#include <QList>
#include <QString>

struct LSHandle;
struct LSMessage;

//FIXME-qtwebkit
namespace Palm
{
	struct DbBackupStatus { std::string url; int err;};
};

/**
 * Manages the backup and restore of the Mojo applications (files and HTML5 databases)
 * for the system.
 */
class BackupManager
{

public:

	bool	init			(GMainLoop* mainLoop);
	
	//FIXME-qtwebkit
	void	dbDumpStarted	(const Palm::DbBackupStatus& status, void* userData);
	void	dbDumpStopped	(const Palm::DbBackupStatus& status, void* userData);
	void	dbRestoreStarted(const Palm::DbBackupStatus& status, void* userData);
	void	dbRestoreStopped(const Palm::DbBackupStatus& status, void* userData);

	static BackupManager*	instance();

private:

	BackupManager	();
	~BackupManager	();

	static LSMethod	s_BackupServerMethods[];
	static BackupManager* s_instance;

	GMainLoop*		m_mainLoop;
	LSHandle*		m_clientService;		// The client's connection to the backup service.
	LSPalmService*		m_serverService;		// The methods we expose to the backup service.
	std::string		m_strBackupServiceName;

	bool	m_doBackupFiles;
	bool	m_doBackupCookies;
	std::list<std::string>	m_backupFiles;	///< List of items I'm managing the backup/restore of.

	void initFilesForBackup();

	static bool preBackupCallback( LSHandle* lshandle, LSMessage *message, void *user_data);
	static bool postRestoreCallback( LSHandle* lshandle, LSMessage *message, void *user_data);
};


#endif // BACKUP_MANAGER_H
