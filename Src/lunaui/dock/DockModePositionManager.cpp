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




/*
	This class holds the application position state for the quick launch bar.
*/

#include "Common.h"

#include "ApplicationManager.h"
#include "DockModePositionManager.h"
#include "WindowServerLuna.h"
#include "DockModeWindowManager.h"
#include "Settings.h"

#include <stdio.h>
#include <glib.h>
#include <algorithm>

#include "cjson/json.h"


DockModePositionManager* DockModePositionManager::instance()
{
	static DockModePositionManager* a = 0;
	if( !a )
		a = new DockModePositionManager();
	return a;
}


DockModePositionManager::DockModePositionManager()
	: m_loadFailed(false)
{
}

DockModePositionManager::~DockModePositionManager()
{
}

bool DockModePositionManager::load()
{
	// try to load our user layout
	std::string dockModeFile = Settings::LunaSettings()->dockModeUserPositions;
	json_object* json = json_object_from_file( const_cast<char*>(dockModeFile.c_str()) );
	if ( is_error(json) ) {
		dockModeFile = Settings::LunaSettings()->dockModeCustomPositions;
		// try to load the customization layout
		json = json_object_from_file(const_cast<char*>(dockModeFile.c_str()));
		if( is_error(json) ) {
			// try to load our fallback layout
			dockModeFile = Settings::LunaSettings()->dockModeDefaultPositions;
			json = json_object_from_file(const_cast<char*>(dockModeFile.c_str()) );
			if ( is_error(json) ) {
				fprintf(stderr, "Failed to parse %s\n",dockModeFile.c_str() );
				m_loadFailed = true;
				return false;
			}
		}
	}
	g_message ("%s: Reading Exhibition apps from %s", __PRETTY_FUNCTION__, dockModeFile.c_str());
	
	json_object* dockLauncher = json_object_object_get(json, "exhibitionApps");
	if( dockLauncher )
	{
		int numItems = json_object_array_length(dockLauncher);
		for (int i=0; i<numItems; i++) {
			json_object* item = json_object_array_get_idx(dockLauncher, i);
			char* appId = json_object_get_string(item);
			if (appId) {
				ApplicationManager::instance()->enableDockModeLaunchPoint(appId);
				// enabling this dock mode launch point at application manager, 
				// do not add to m_appIds here, if the appId is valid and exhibition 
				// enabled, insert will get called which will add it to the list and save it 
				// to the file.
			}
		}
	}
	
	json_object* knownPucks = json_object_object_get(json, "knownPucks");
	if (knownPucks)
	{
		json_object *item, *puckObj, *appObj;
		std::string puckId, appId;
		DockModeWindowManager* dmwm = ((DockModeWindowManager*)((WindowServerLuna*)(WindowServer::instance()))->dockModeManager());
		int numItems = json_object_array_length(knownPucks);
		for (int i = 0; i < numItems; ++i) {
			item = json_object_array_get_idx (knownPucks, i);
			puckObj  = json_object_object_get (item, "puckId");
			appObj  = json_object_object_get (item, "appId");
			if (puckObj && !is_error (puckObj) && appObj && !is_error (appObj)) {
				puckId = json_object_get_string (puckObj);
				appId = json_object_get_string (appObj);
				dmwm->addPuckIdAndDefaultApp (puckId, appId);
				puckId.clear();
				appId.clear();
			}
			item = puckObj = appObj = 0;
		}

	}
	json_object_put( json );

	return true;
}

void DockModePositionManager::addEnabledApp (const std::string& appId)
{
	std::vector<std::string>::iterator it;
	it = std::find(m_appIds.begin(), m_appIds.end(), appId);
	if (it != m_appIds.end()) { 
		m_appIds.erase(it);
	}

	m_appIds.push_back (appId);
	save();
}

void DockModePositionManager::addAppForPuck (const std::string& puckId, const std::string& appId)
{
	m_appForPuck[puckId] = appId;
	save();
}

void DockModePositionManager::removeEnabledApp(const std::string& appId)
{
	std::vector<std::string>::iterator it;
	it = std::find(m_appIds.begin(), m_appIds.end(), appId);
	if (it != m_appIds.end()) {
		m_appIds.erase(it);
	}
	
	std::map<std::string,std::string>::iterator it2 = m_appForPuck.begin();
	
	while (it2 != m_appForPuck.end())
	{
		if (it2->second == appId) {
			m_appForPuck.erase (it2);
			it2 = m_appForPuck.begin();
			// resetting iterator after erase to ensure all app-puck associations are processed.
		}
		else {
			++it2;
		}
	}

	save();
}
void DockModePositionManager::clear()
{
	m_appIds.clear();
	save();
}

bool DockModePositionManager::getLaunchPointIds( std::vector<std::string>& outIds )
{
	if (m_loadFailed)
		return false;
	outIds.resize(m_appIds.size());
	std::copy(m_appIds.begin(), m_appIds.end(), outIds.begin());
	return true;
}

bool DockModePositionManager::save()
{
	const std::string& userPath = Settings::LunaSettings()->dockModeUserPositions;
	g_mkdir_with_parents(userPath.substr(0, userPath.rfind("/")).c_str(), 0777);
	
	json_object* root = json_object_new_object();

	json_object* apps = json_object_new_array();
	for( size_t i=0; i<m_appIds.size(); i++ )
	{
		json_object_array_add (apps, json_object_new_string (m_appIds[i].c_str()));
	}
	json_object_object_add (root, "exhibitionApps", apps);


	json_object* knownPucks = json_object_new_array();
	std::map<std::string, std::string>::iterator it;
	for (it = m_appForPuck.begin(); it != m_appForPuck.end(); ++it)
	{
		json_object* mapEntry = json_object_new_object();
		json_object_object_add (mapEntry, "puckId", json_object_new_string ((it->first).c_str()));
		json_object_object_add (mapEntry, "appId", json_object_new_string ((it->second).c_str()));
		json_object_array_add (knownPucks, mapEntry);
	}
	json_object_object_add (root, "knownPucks", knownPucks);

	json_object_to_file ((char*)userPath.c_str(), root);
	json_object_put (root);
	return true;
}

