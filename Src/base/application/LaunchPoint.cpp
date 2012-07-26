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
#include <glib.h>
#include <cjson/json.h>

#include "ApplicationDescription.h"
#include "PackageDescription.h"
#include "ApplicationManager.h"
#include "LaunchPoint.h"
#include "Logging.h"
#include "Utils.h"
#include "Settings.h"
#include "Localization.h"
//MDK-LAUNCHER #include "CardLayout.h"

const char* localFileURI = "file://";

LaunchPoint* LaunchPoint::fromFile(ApplicationDescription* appDesc,
								   const std::string& filePath)
{
	char* jsonStr = readFile(filePath.c_str());
	if (!jsonStr)
		return 0;

	// infer the launchpoint ID from the filename
	gchar* fileName = g_path_get_basename(filePath.c_str());

	LaunchPoint* lp = fromJSON(appDesc, jsonStr, fileName);

	delete [] jsonStr;
	g_free(fileName);

	return lp;
}

bool LaunchPoint::toFile() const
{
	// NOTE: we only support persisting dynamic launchpoints at this point
	if (!isRemovable() || isDefault())
		return false;

	// dynamic launch points have appinfo files named by their launchpointid's
	std::string	filePath = Settings::LunaSettings()->lunaLaunchPointsPath;
	if (filePath[filePath.size()-1] != '/')
		filePath += "/";
	filePath += launchPointId();

	// persist a launch points appinfo
	json_object* json = toJSON();
	int res = json_object_to_file((char*)filePath.c_str(), json);
	if (json && !is_error(json))
		json_object_put(json);
	return res != -1;
}

LaunchPoint* LaunchPoint::fromJSON(ApplicationDescription* appDesc,
								   const char* jsonStr,
								   const std::string& launchPointId)
{
	struct json_object* root = 0;
	struct json_object* label = 0;

	std::string title, menuName, icon, params, id;
	bool removable=false;
	bool success = false;

	root = json_tokener_parse(jsonStr);
	if (!root || is_error(root)) {
		fprintf(stderr, "Failed to parse '%s' into a JSON string.\n", launchPointId.c_str());
		goto Done;
	}

	label = json_object_object_get(root, "title");
	if (!label || is_error(label))
		goto Done;
	title = json_object_get_string(label);

	label = json_object_object_get(root, "appmenu");
	if (label && (!is_error(label)))
		menuName = json_object_get_string(label);
	else
		menuName = title;

	label = json_object_object_get(root, "icon");
	if (!label || is_error(label))
		goto Done;
	icon = json_object_get_string(label);

	label = json_object_object_get(root, "params");
	if (!label || is_error(label))
		goto Done;
    if (appDesc && appDesc->type() == ApplicationDescription::Type_Qt)
        params = json_object_get_string(label);
    else
    	params = json_object_to_json_string(label);

	label = json_object_object_get(root, "id");
	if (!label || is_error(label))
		goto Done;
	id = json_object_get_string(label);

	label = json_object_object_get(root, "removable");
	if (label && (!is_error(label)))
		removable = json_object_get_boolean(label);

	success = true;

Done:

	if (root && !is_error(root))
		json_object_put(root);

	if (success)
		return new LaunchPoint(appDesc, id, launchPointId, title, menuName, icon, params,removable);

	return 0;
}

LaunchPoint::LaunchPoint(ApplicationDescription* appDesc,
						 const std::string& id,
						 const std::string& launchPointId,
						 const std::string& title,
						 const std::string& menuName,
						 const std::string& iconPath,
						 const std::string& params,
						 bool removable) :
	m_appDesc(appDesc) ,
	m_id(id) ,
	m_launchPointId(launchPointId) ,
	m_appmenuName(menuName) ,
	m_iconPath(iconPath) ,
	m_params(params) ,
	m_removable(removable)
{
	this->m_bDefault = false;
	if (m_iconPath.compare(0, 7, localFileURI) == 0) {
		m_iconPath.erase(0, 7);
	}
	QImage icon;
    icon.load(qFromUtf8Stl(m_iconPath));
	if (icon.isNull()) {
		// load a default image
		m_iconPath = Settings::LunaSettings()->lunaSystemResourcesPath + "/default-app-icon.png";
//		icon.load(qFromUtf8Stl(m_iconPath));
//		if (m_icon.isNull())
//			g_warning("%s: Failed to load application icon for app %s (original: %s, default: %s)",
//					__PRETTY_FUNCTION__, id.c_str(), iconPath.c_str(), m_iconPath.c_str());
	}

	m_title.set(title);
}

void LaunchPoint::updateTitle(const std::string& titleStr)
{
	if (titleStr.empty())
		return;			//arbitrary decision; won't allow empty titles

	m_title.set(titleStr);
}

LaunchPoint::~LaunchPoint()
{
}

bool LaunchPoint::updateIconPath(std::string newIconPath)
{
	// strip off local file url
	if (newIconPath.compare(0, 7, localFileURI) == 0) {
		newIconPath.erase(0, 7);
	}

	QImage newIcon(qFromUtf8Stl(newIconPath));
	if (newIcon.isNull()) {
		return false;
	}

	m_iconPath = newIconPath;

	// attempt to persist change
	toFile();

	return true;
}

std::string LaunchPoint::category() const
{
	if (m_appDesc)
		return m_appDesc->category();

	return "";
}

bool LaunchPoint::isVisible() const
{
	return (m_appDesc ? m_appDesc->isVisible() : true);
}

std::string LaunchPoint::entryPoint() const
{
	if (m_appDesc)
		return m_appDesc->entryPoint();

	return "";
}

bool LaunchPoint::matchesTitle(const gchar* str) const
{
	if (!str || !m_title.lowercase)
		return false;

	if (g_str_has_prefix(m_title.lowercase, str))
		return true;

	static const gchar* delimiters = " ,._-:;()\\[]{}\"/";
	static size_t len = strlen(delimiters);
	bool matches = false;
	const gchar* start = m_title.lowercase;
	while (start != NULL) {
		start = strstr(start, str);

		// have we hit the end?
		if (start == NULL || start == m_title.lowercase)
			break;

		// is the previous character in our delimiter set?
		const gchar c[] = {*g_utf8_prev_char(start), '\0'};
		if (strcspn(delimiters, c) < len) {
			matches = true;
			break;
		}
		start = g_utf8_find_next_char(start, NULL);
	}
	return matches;
}

int LaunchPoint::compareByKeys(const LaunchPoint* lp) const
{
	return strcmp(this->m_title.keyed, lp->m_title.keyed);
}

//TODO: this is kind of funky; a launchpoint of an app should really just present the info for the app.
json_object* LaunchPoint::toJSON() const
{
	json_object* json = json_object_new_object();
	json_object_object_add(json, (char*) "id",   json_object_new_string((char*) id().c_str()));
	if (m_appDesc) {
		json_object_object_add(json, (char*) "version", json_object_new_string(m_appDesc->version().c_str()));
		json_object_object_add(json, (char*) "appId", json_object_new_string((char*) m_appDesc->id().c_str()));
		json_object_object_add(json, (char*) "vendor", json_object_new_string((char*) m_appDesc->vendorName().c_str()));
		json_object_object_add(json, (char*) "vendorUrl", json_object_new_string((char *) m_appDesc->vendorUrl().c_str()));
		int appSize = 0;
		std::string packageId = m_appDesc->id();
		if (this->isDefault()) {
			PackageDescription* packageDesc = ApplicationManager::instance()->getPackageInfoByAppId(m_appDesc->id());
			if (packageDesc) {
				appSize = packageDesc->packageSize();
				packageId = packageDesc->id();
			}

		}
		json_object_object_add(json, (char*) "size", json_object_new_int(appSize));
		json_object_object_add(json, (char*) "packageId", json_object_new_string((char*) packageId.c_str()));

		if (m_appDesc->dockModeStatus()) {
			json_object_object_add(json, (char*) "exhibitionMode", json_object_new_boolean(true));
			json_object_object_add(json, (char*) "exhibitionModeTitle", json_object_new_string((char*)m_appDesc->dockModeTitle().c_str()));
		}
	} else {
		json_object_object_add(json, (char*) "size", json_object_new_int(0));
		json_object_object_add(json, (char*) "packageId", json_object_new_string((char*) m_appDesc->id().c_str()));
	}
	json_object_object_add(json, (char*) "removable", json_object_new_boolean(m_removable));

	json_object_object_add(json, (char*) "launchPointId", json_object_new_string((char*) launchPointId().c_str()));
	json_object_object_add(json, (char*) "title", json_object_new_string((char*) title().c_str()));
	json_object_object_add(json, (char*) "appmenu", json_object_new_string((char*)menuName().c_str()));
	json_object_object_add(json, (char*) "icon", json_object_new_string((char*) iconPath().c_str()));

	if (!params().empty() && m_appDesc->type() != ApplicationDescription::Type_Qt) {
        if (m_appDesc->type() == ApplicationDescription::Type_Qt) {
            json_object_object_add(json, (char*) "params", json_object_new_string(params().c_str()));
        }
        else {
    		json_object* paramsJson = json_tokener_parse(params().c_str());
	    	json_object_object_add(json, (char*) "params", paramsJson);
        }
	}

	return json;
}
#include <QDebug>
QPixmap LaunchPoint::icon() const
{
	//get size of icon from launcher settings!
	return QPixmap::fromImage(QImage(qFromUtf8Stl(m_iconPath)).scaled(DEFAULT_ICON_W,
																	DEFAULT_ICON_H,
																	Qt::IgnoreAspectRatio,
																	Qt::SmoothTransformation));
}
