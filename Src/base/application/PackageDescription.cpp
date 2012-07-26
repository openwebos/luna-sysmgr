/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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




#include <stdio.h>
#include <glib.h>

#include "PackageDescription.h"
#include "ApplicationDescription.h"
#include "Utils.h"
#include "Settings.h"

PackageDescription::PackageDescription()
	: m_id("")
	, m_version("1.0")
	, m_folderPath("")
	, m_packageSize(0)
	, m_fsBlockSize(0)
	, m_isOldStyle(false)
{
}

PackageDescription::~PackageDescription()
{
}

// static
PackageDescription* PackageDescription::fromFile(const std::string& filePath, const std::string& folderPath)
{
	PackageDescription* packageDesc = NULL;
	char* jsonStr = NULL;

	jsonStr = readFile(filePath.c_str());
	if (!jsonStr || !g_utf8_validate(jsonStr, -1, NULL)) {
		return NULL;
	}

	struct json_object* root = json_tokener_parse(jsonStr);
	if (!root || is_error( root )) {
		g_warning("Failed to parse '%s' into a JSON string", filePath.c_str());
		goto Done;
	}

	packageDesc = fromJson(root, folderPath);

Done:

	if (root && !is_error(root))
		json_object_put(root);

	delete[] jsonStr;

	return packageDesc;
}

//static
PackageDescription* PackageDescription::fromJson(json_object* root, const std::string& folderPath)
{
	if (!root)
		return NULL;

	bool success = false;

	PackageDescription* packageDesc = new PackageDescription();

	packageDesc->m_folderPath = folderPath;

	struct json_object* label = NULL;
	// ID: mandatory
	label = JsonGetObject(root, "id");
	if (label) {
		packageDesc->m_id = json_object_get_string(label);
		if (packageDesc->m_id == "") {
			g_warning("package %s has an empty ID field", folderPath.c_str());
			goto Done;
		}
	} else {
		g_warning("package %s does not have an ID", folderPath.c_str());
		goto Done;
	}

	// app (or apps) json array: optional
	label = JsonGetObject(root, "app");
	if (label) {
		packageDesc->m_appIds.push_back(std::string(json_object_get_string(label)));
	} else {
		label = JsonGetObject(root, "apps");
		if (label) {
			for (int i = 0; i < json_object_array_length(label); i++) {
				struct json_object* app = json_object_array_get_idx(label, i);
				if (app && !is_error(app)) {
					packageDesc->m_appIds.push_back(std::string(json_object_get_string(app)));
				}
			}
		}
	}

	// services json array: optional
	label = JsonGetObject(root, "services");
	if (label) {
		for (int i = 0; i < json_object_array_length(label); i++) {
			struct json_object* service = json_object_array_get_idx(label, i);
			if (service && !is_error(service)) {
				packageDesc->m_serviceIds.push_back(std::string(json_object_get_string(service)));
			}
		}
	}

	if (packageDesc->m_appIds.empty() && packageDesc->m_serviceIds.empty()) {
		g_warning("package %s does not have any apps or services", folderPath.c_str());
		goto Done;
	}

	// Optional parameters
	success = true;

	// version: optional
	label = JsonGetObject(root, "version");
	if (label) {
		packageDesc->m_version = json_object_get_string(label);
	}

	// accounts json array: optional
	label = JsonGetObject(root, "accounts");
	if (label) {
		for (int i = 0; i < json_object_array_length(label); i++) {
			struct json_object* accountId = json_object_array_get_idx(label, i);
			if (accountId && !is_error(accountId)) {
				packageDesc->m_accountIds.push_back(std::string(json_object_get_string(accountId)));
			}
		}
	}

	packageDesc->m_jsonString = json_object_to_json_string(root);

	Done:

	if (!success) {
		delete packageDesc;
		return NULL;
	}

	return packageDesc;
}

// static
PackageDescription* PackageDescription::fromApplicationDescription(ApplicationDescription* appDesc)
{
	if (appDesc == NULL)
		return NULL;

	PackageDescription* packageDesc = new PackageDescription();
	packageDesc->m_id = appDesc->id();
	packageDesc->m_version = appDesc->version();
	packageDesc->m_folderPath = appDesc->folderPath();
	packageDesc->m_isOldStyle = true;
	packageDesc->m_appIds.push_back(appDesc->id());

	return packageDesc;
}

json_object* PackageDescription::toJSON() const
{
	json_object* json = NULL;
	if (m_isOldStyle) {
		json = json_object_new_object();
		json_object_object_add(json, (char*) "id",   json_object_new_string((char*) m_id.c_str()));
		json_object_object_add(json, (char*) "version", json_object_new_string(m_version.c_str()));
		json_object_object_add(json, (char*) "size", json_object_new_int((int)m_packageSize));

		json_object* apps = json_object_new_array();
		std::vector<std::string>::const_iterator appIdIt, appIdItEnd;
		for (appIdIt = m_appIds.begin(), appIdItEnd = m_appIds.end(); appIdIt != appIdItEnd; ++appIdIt) {
			json_object_array_add(apps, json_object_new_string((*appIdIt).c_str()));
		}
		json_object_object_add(json, (char*) "apps", apps);

	} else {
		json = json_tokener_parse(m_jsonString.c_str());
		if (!json || is_error(json)) {
			g_warning("%s: Failed to parse '%s' into a JSON string", __PRETTY_FUNCTION__, m_jsonString.c_str());
			return NULL;
		}
		json_object_object_add(json, (char*) "size", json_object_new_int((int)m_packageSize));

		json_object* label = JsonGetObject(json, "icon");
		if (label) {
			std::string icon = json_object_get_string(label);
			json_object_object_del(json, (char*) "icon");
			json_object_object_add(json, (char*) "icon", json_object_new_string((char*) (m_folderPath + "/" + icon).c_str()));
		}

		label = JsonGetObject(json, "miniicon");
		if (label) {
			std::string miniicon = json_object_get_string(label);
			json_object_object_del(json, (char*) "miniicon");
			json_object_object_add(json, (char*) "miniicon", json_object_new_string((char*) (m_folderPath + "/" + miniicon).c_str()));
		}

	}
	return json;
}

bool PackageDescription::operator==(const PackageDescription& cmp) const {
	return (m_id == cmp.id());
}
bool PackageDescription::operator!=(const PackageDescription& cmp) const {
	return (m_id != cmp.id());
}
