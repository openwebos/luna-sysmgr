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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <QFile>
#include <QByteArray>
#include <QtDebug>

#include "ApplicationManager.h"
#include "ApplicationDescription.h"
#include "Preferences.h"
#include "Utils.h"


std::string getResourcePathFromString(const std::string& entry, const std::string& appId,
									  const std::string& systemResourceFolder)
{
	if (entry.empty())
		return std::string();

	struct stat stBuf;

	// Absolute path?
	if (entry[0] == '/') {

		// check if it exists and is a regular file
		if (::stat(entry.c_str(), &stBuf) != 0 || !S_ISREG(stBuf.st_mode))
			return std::string();

		return entry;
	}

	// relative path. first check in the app folder
	ApplicationDescription* appDesc = ApplicationManager::instance()->getAppById(appId);
	if (appDesc) {

        // First check in the locale specific folder
		std::string filePath = appDesc->folderPath() + "/resources/" + LocalePreferences::instance()->locale() + "/" + entry;
		if (::stat(filePath.c_str(), &stBuf) == 0 && (S_ISREG(stBuf.st_mode) || S_ISLNK(stBuf.st_mode)))
			return filePath;

        // Try in the standard app folder path
		filePath = appDesc->folderPath() + "/" + entry;
		if (::stat(filePath.c_str(), &stBuf) == 0 && (S_ISREG(stBuf.st_mode) || S_ISLNK(stBuf.st_mode)))
			return filePath;
	}

	// Look for it in the system folder
	std::string filePath = systemResourceFolder + "/" + entry;
	if (::stat(filePath.c_str(), &stBuf) == 0 && S_ISREG(stBuf.st_mode))
		return filePath;

	// ah well... we give up
	return std::string();
}

