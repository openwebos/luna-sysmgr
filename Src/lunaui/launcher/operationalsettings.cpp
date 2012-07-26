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




#include "Common.h"
#include "operationalsettings.h"
#include "page.h"
#include <qglobal.h>
#include <QFileInfo>
#include <QDir>

static const char* kSettingsFile = "/etc/palm/launcher3/launcher_operational_settings.conf";
static const char* kSettingsFilePlatform = "/etc/palm/launcher3/launcher_operational_settings-platform.conf";

OperationalSettings* OperationalSettings::s_instance = 0;

#define KEY_UINTEGER(cat,name,var) \
{\
		quint32 _v;\
		GError* _error = 0;\
		_v=(quint32)g_key_file_get_integer(keyfile,cat,name,&_error);\
		if( !_error ) { var=_v; }\
		else { g_error_free(_error); }\
}

#define KEY_STRING(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs ) { var=(const char*)_vs; g_free(_vs); }\
	else g_error_free(_error); \
}

#define KEY_QSTRING(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs ) { var=QString((const char*)_vs); g_free(_vs); }\
	else g_error_free(_error); \
}

#define KEY_BOOLEAN(cat,name,var) \
{\
	gboolean _vb;\
	GError* _error = 0;\
	_vb=g_key_file_get_boolean(keyfile,cat,name,&_error);\
	if( !_error ) { var=_vb; }\
	else g_error_free(_error); \
}

#define KEY_INTEGER(cat,name,var) \
{\
	int _v;\
	GError* _error = 0;\
	_v=g_key_file_get_integer(keyfile,cat,name,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
}

#define KEY_DOUBLE(cat,name,var) \
{\
	double _v;\
	GError* _error = 0;\
	_v=g_key_file_get_double(keyfile,cat,name,&_error);\
	if( !_error ) { var=_v; }\
	else g_error_free(_error); \
}

#define KEY_COLOR(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs )  \
		{ 	var=QColor((const char*)_vs); \
			if (!var.isValid()) { var = QColor(Qt::white);} \
			g_free(_vs); \
		}\
	else g_error_free(_error); \
}

#define KEYS_SIZEF(cat,namew,nameh,var) \
{ \
	qreal _v; \
	GError * _error = 0;\
	_v = (qreal)g_key_file_get_double(keyfile,cat,namew,&_error);\
	if (!_error) { \
		var.setWidth(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
	_error = 0; \
	_v = (qreal)g_key_file_get_double(keyfile,cat,nameh,&_error);\
	if (!_error) { \
		var.setHeight(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
}

#define KEYS_SIZE(cat,namew,nameh,var) \
{ \
	qint32 _v; \
	GError * _error = 0;\
	_v = (qint32)g_key_file_get_integer(keyfile,cat,namew,&_error);\
	if (!_error) { \
		var.setWidth(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
	_error = 0; \
	_v = (qint32)g_key_file_get_integer(keyfile,cat,nameh,&_error);\
	if (!_error) { \
		var.setHeight(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
}

#define KEYS_POINT(cat,namew,nameh,var) \
{ \
	qint32 _v; \
	GError * _error = 0;\
	_v = (qint32)g_key_file_get_integer(keyfile,cat,namew,&_error);\
	if (!_error) { \
		var.setX(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
	_error = 0; \
	_v = (qint32)g_key_file_get_integer(keyfile,cat,nameh,&_error);\
	if (!_error) { \
		var.setY(_v); \
	} \
	else { \
		g_error_free(_error); \
	} \
}

#define KEY_COLOR(cat,name,var) \
{\
	gchar* _vs;\
	GError* _error = 0;\
	_vs=g_key_file_get_string(keyfile,cat,name,&_error);\
	if( !_error && _vs )  \
		{ 	var=QColor((const char*)_vs); \
			if (!var.isValid()) { var = QColor(Qt::white);} \
			g_free(_vs); \
		}\
	else g_error_free(_error); \
}


OperationalSettings::OperationalSettings()
: startOnPageDesignated(Page::PageDesignatorAll())
, returnToDesignatorOnHide(QString())
, savedPagesDirectory(QString("/var/luna/preferences/launcher3/"))
, appBlacklistFilepath(QString("/etc/palm/launcher3/app_blacklist.conf"))
, forceFavoritesToAllContent(false)
, useAllContentInsteadOfEmptyFavorites(true)
, forceSaveOnHide(false)
, usePreciseTimeForMasterSaveFileName(false)
, useSingleMasterSaveFileName(true)
, useApplicationManagerHiddenFlag(true)
, useStagedRendering(true)
, appKeywordsToPageDesignatorMapFilepath("/etc/palm/launcher3/app-keywords-to-designator-map.txt")
, useSingleQuicklaunchSaveFileName(true)
, favoritesPageIndex(2)
, installedAppsPageIndex(1)
, appCatalogAppId("com.palm.app.enyo-findapps")
, settingsPageIndex(3)
, settingsAppCategoryDesignator("Settings")
, systemAppsPageIndex(0)
, preferAppKeywordsForAppPlacement(false)
, useDynamicMultiPageInitialConfigurator(false)
, allowedToLaunchInReorderMode(false)
, ignoreLegacyCustomizationFiles(false)
, logDirPath(QString("/var/log"))
, archiverExeFilename("/bin/tar")
, debugArchiveLauncherSavesOnLauncherInit(false)
, safeLauncherBoot(true)
{
	s_instance = this;

	readSettings(kSettingsFile);
	readSettings(kSettingsFilePlatform);

	verify();

	g_mkdir_with_parents(savedPagesDirectory.toAscii().constData(),0755);
}

OperationalSettings::~OperationalSettings()
{
    s_instance = 0;
}

void OperationalSettings::readSettings(const char* filePath)
{
    GKeyFile* keyfile = g_key_file_new();
    qint32 v = INT_MIN;
    qreal vf = 0.0;
	if (!g_key_file_load_from_file(keyfile, filePath,
								   G_KEY_FILE_NONE, NULL)) {
		goto Done;
	}

	KEY_QSTRING("Main","StartOnPageDesignated",startOnPageDesignated);
	KEY_QSTRING("Main","ReturnToPageDesignatedOnHide",returnToDesignatorOnHide);
	KEY_QSTRING("Main","SavedPagesDirectory",savedPagesDirectory);
	KEY_QSTRING("Main","AppBlacklistFilepath",appBlacklistFilepath);
	KEY_BOOLEAN("Main","ForceFavoritesToAllContent",forceFavoritesToAllContent);
	KEY_BOOLEAN("Main","UseAllContentInsteadOfEmptyFavorites",useAllContentInsteadOfEmptyFavorites);
	KEY_BOOLEAN("Main","ForceSaveOnHide",forceSaveOnHide);
	KEY_BOOLEAN("Main","UsePreciseTimeForMasterFilename",usePreciseTimeForMasterSaveFileName);
	KEY_BOOLEAN("Main","UseSingleMasterFilename",useSingleMasterSaveFileName);
	KEY_BOOLEAN("Main","UseApplicationManagerHiddenFlag",useApplicationManagerHiddenFlag);
	KEY_BOOLEAN("Main","UseStagedRendering",useStagedRendering);
	KEY_QSTRING("Main","AppKeywordsToPageDesignatorMapFilepath",appKeywordsToPageDesignatorMapFilepath);
	KEY_BOOLEAN("Main","UseSingleQuicklaunchSaveFilename",useSingleQuicklaunchSaveFileName);
	KEY_UINTEGER("Main","FavoritesPageIndex",favoritesPageIndex);
	KEY_UINTEGER("Main","InstalledAppsPageIndex",installedAppsPageIndex);
	KEY_QSTRING("Main","AppCatalogAppId",appCatalogAppId);
	KEY_UINTEGER("Main","SettingsPageIndex",settingsPageIndex);
	KEY_QSTRING("Main","SettingsAppCategoryDesignator",settingsAppCategoryDesignator);
	KEY_UINTEGER("Main","SystemAppsPageIndex",systemAppsPageIndex);
	KEY_BOOLEAN("Main","PreferAppKeywordsForAppPlacement",preferAppKeywordsForAppPlacement);
	KEY_BOOLEAN("Main","UseDynamicMultiPageInitialConfigurator",useDynamicMultiPageInitialConfigurator);
	KEY_BOOLEAN("Main","AllowedToLaunchInReorderMode",allowedToLaunchInReorderMode);
	KEY_BOOLEAN("Main","IgnoreLegacyCustomizationFiles",ignoreLegacyCustomizationFiles);
	KEY_BOOLEAN("Main","SafeLauncherBoot",safeLauncherBoot);
	KEY_QSTRING("Debug","LogDirPath",logDirPath);
	KEY_QSTRING("Debug","ArchiverExeFilename",archiverExeFilename);

	verify();
Done:

	if (keyfile) {
		g_key_file_free(keyfile);
	}
}

void OperationalSettings::verify()
{
	//TODO: IMPLEMENT (unfinished)
	//check and correct any inconsistencies in settings

	//TODO: better validation for paths
	if (savedPagesDirectory.isEmpty())
	{
		savedPagesDirectory = QString("/var/luna/preferences/launcher3/");
	}
	(void)g_mkdir_with_parents(savedPagesDirectory.toAscii().constData(),0755);
}
