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




#ifndef OPERATIONALSETTINGS_H_
#define OPERATIONALSETTINGS_H_

#include "Common.h"

#include <glib.h>
#include <QtGlobal>
#include <QSize>
#include <QSizeF>
#include <QColor>
#include <QPoint>

class OperationalSettings
{
public:

	//see PageSaver and PageRestore classes. Specifying one of the system recognized designators here will
	// lead make the launcher show the first page with this designator that is found (if it is found), when it is
	// FIRST brought up after a restart
	// (default = "favorites")
	QString startOnPageDesignated;

	// similar to startOnPageDesignated, but will be in effect everytime the launcher is hidden and brought back up,
	// not just on a restart; useful for when you ALWAYS want to see a particular page, regardless of where you last
	// left off (e.g. launched an app from) when the launcher disappeared.
	// (default = "" ; feature disabled)
	QString returnToDesignatorOnHide;

	//the place where the launcher will save page info and other state to persist as user data
	// (default = "/var/luna/preferences/launcher3/")
	QString savedPagesDirectory;

	//app blacklist location
	// (default = "/var/luna/preferences/launcher3/control/blacklist.txt")
	QString appBlacklistFilepath;

	// will ignore any saved content and will make the favorites page have all the apps available
	// (default = false)
	bool forceFavoritesToAllContent;

	// useAllContentInsteadOfEmptyFavorites
	//  This is a slightly lax version of forceFavoritesToAllContent. Whereas that one overrides even a found, non-empty
	//	favorites, this one will only override it if it is empty and this is set 'true'
	// (default = false)
	bool useAllContentInsteadOfEmptyFavorites;

	// forces the launcher to save state each time it is hidden/disappears/minimizes
	// (default = false)
	bool forceSaveOnHide;

	// this will use the epoch timestamp rather that the date for saving the launcher master-save file.
	// This should remain set 'false' in most cases; setting to 'true' will create a lot of master files (since
	// a new one will be created for every reorder or other change to the launcher pages)
	// (default = false)
	bool usePreciseTimeForMasterSaveFileName;

	// this will make it so there can only be 1 master file. It precludes some more nifty-er features with
	// multiple configs, but they're not part of the FC features anyways
	// (default = true)
	bool useSingleMasterSaveFileName;

	// this will control whether the AppMonitor recognizes and respects the ApplicationManager's "hidden" flag.
	// if 'true', then an app descriptor with hidden set will have the same effect as the blacklisting of the app id
	// if 'false', the flag will be ignored and the app will get added to the launcher just like any other app
	// unless it's also been blacklisted
	// (default = true)
	bool useApplicationManagerHiddenFlag;

	// this controls whether the rendering, when done manually (e.g. the icon layouts which paint icons on their own), uses
	// "staged rendering", which does multiple passes over the thing to be painted. The potential benefit it avoiding interleaving
	// of things that potentially require a lot of state changes, like textures->text->textures. All this is very platform specific
	// and it's best to just experiment
	// (default = true)
	bool useStagedRendering;

	// this is the path to the file that maps keywords and categories in a WebOSApp Application descriptor (ApplicationDescription)
	// to a Page designator. Its format is in the QSettings INI format.
	// It also defines the designator names for any auxiliary pages, *besides* favorites. In fact, never specify a designator named 'favorites' in this section
	// (default = "/etc/palm/launcher3/app-keywords-to-designator-map.txt" )
	QString appKeywordsToPageDesignatorMapFilepath;

	// this picks the index at which favorites will appear.
	// If it is invalid (i.e. out of range when the pages are created), then index 0 will be used (which is the first, "leftmost" page)
	// (one page is always guaranteed to exist)
	// (default = 1)
	quint32 favoritesPageIndex;

	// this controls the naming of the files for the quicklaunch. Setting to true will assure there is only 1 copy of the quicklaunch save file
	// used (similar to 'useSingleMasterSaveFileName'
	// (default = true)
	bool useSingleQuicklaunchSaveFileName;

	// this picks the index at which the App Installer will place all the user installed applications, as they are installed
	// It's an index, rather than a page name or designator, so that in the face of changing names in configs, this can still "work".
	// It will be up to the product team to assure that this index is always in line with what the file pointed to by 'appKeywordsToPageDesignatorMapFilepath'
	// has listed in terms of auxiliary pages, and appropriately matches this index to what page is the "app page"
	// if it is invalid, then index 0 will be used.
	// (default = 2 ; immediately to the right of Favorites , at default = 1)
	quint32 installedAppsPageIndex;

	// since a requirement is that the app catalog app appear on the page 'installedAppPageIndex' along with the user installed apps, this var holds the WebOS appid
	// (e.g. com.palm.app.findapps) of that app, so that during an INITIAL scan, it can be placed on the correct page
	// leave blank to disable this feature; in that case, the app catalog app will end up on whatever page the default icon placement policy decides
	// (default = com.palm.app.findapps)
	QString appCatalogAppId;

	// similar to above, this is the index of the page that has the system settings...
	// (default = 3 ; this is the right most (last) page on the default 4-page layout)
	quint32 settingsPageIndex;

	// this string is compared with the category() of a WebOS app to determine if that app is a "settings" app that needs to go on the
	// page at index "settingsPageIndex". It will be compared WITH case-sensitivity. If it is blank, it will be ignored (i.e. match none)
	// (default = Settings)
	QString settingsAppCategoryDesignator;

	// this is the index for the page that is for the rest of the built-in apps that come on the factory device
	// (default = 0 ; immediately to the left of Favorites , at default = 1. The system page is by default the first page)
	quint32 systemAppsPageIndex;

	// this flag selects whether or not the app keywords -> page designator mappings in 'appKeywordsToPageDesignatorMapFilepath'
	// should be used prior to attempting to apply the logic of installedAppsPageIndex, settingsPageIndex , systemAppsPageIndex
	// if true, then an app that has stats that match something in the mappings loaded from the map file will use the page pointed to
	// by that map, rather than the more coarse (installed app, settings app, system app) buckets
	// (default = false)
	bool preferAppKeywordsForAppPlacement;

	// this setting only applies to the INITIAL state of the launcher - i.e. how it comes from a fresh flash WITHOUT a restored profile.
	// if set to true, then a multi page configuration will be created based on apps' categories and keywords and various other system properties such
	// as which page designators were defined, etc...essentially a big glob logic of a lot of the variables above and the mechanisms they control
	// (e.g. installedAppsPageIndex , etc)
	// if set to false, then a "static" configurator is used, which uses legacy (json formatted) layout files, subject to customization, to try and create
	// an initial configuration based on the apps that are installed
	// the legacy files ARE expected to be modified with the correct designators, as per contents of appKeywordsToPageDesignatorMapFilepath
	// (default = false  ; using static configurator, per GEMSTONE-RD specs)
	bool useDynamicMultiPageInitialConfigurator;

	// if true, then icons in "reorder mode" (with the decorators 'nd stuff) can be launched normally
	// (default = false)
	bool allowedToLaunchInReorderMode;

	// if true, the old launcher (bfish) customization files for launcher layout and quicklaunch will be ignored (the defaults in /etc/palm/ will not be, and will be used instead)
	// (default = true)
	bool ignoreLegacyCustomizationFiles;

	// for dumping diagnostics into the log dir, to be collected with the rest of the logs
	// (default = /var/log , /tmp on desktop)
	QString logDirPath;

	// what to use for archiving debugs and things
	// (default = /bin/tar)
	QString archiverExeFilename;

	// strictly for debug; simply archives everything in the saved dir when the launcher starts the init
	// (default = false)
	bool debugArchiveLauncherSavesOnLauncherInit;

	// whereas debugArchiveLauncherSavesOnLauncherInit is for debugging the issues with corrupt launcher save files, 'safeLauncherBoot' will
	// take action to prevent a boot with corrupt files. It will delete the entire 'savedPagesDirectory', and then exit LunaSysMgr (a forced "reboot")
	// (default = true).  Change it to false to debug/inspect corruptions. Setting this to true (which should be the setting for Production) will *not* save
	// launcher files, as the debug flag suggests
	bool safeLauncherBoot;

public:
	static OperationalSettings* settings() {

		if (G_UNLIKELY(s_instance == 0))
			new OperationalSettings;

		return s_instance;
	}

private:

	static OperationalSettings* s_instance;

private:

	OperationalSettings();
	~OperationalSettings();

	void readSettings(const char* filePath);
	void verify();
};


#endif /* OPERATIONALSETTINGS_H_ */
