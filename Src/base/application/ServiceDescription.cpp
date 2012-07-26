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

#include <cjson/json.h>

#include "ServiceDescription.h"
#include "Utils.h"

ServiceDescription::ServiceDescription()
	: m_id("")
{
}

ServiceDescription::~ServiceDescription()
{
}

// static
ServiceDescription* ServiceDescription::fromFile(const std::string& filePath)
{
	bool success = false;
	ServiceDescription* serviceDesc = NULL;
	char* jsonStr = NULL;

	jsonStr = readFile(filePath.c_str());
	if (!jsonStr || !g_utf8_validate(jsonStr, -1, NULL)) {
		return NULL;
	}

	struct json_object* root = NULL;
	struct json_object* label = NULL;

	root = json_tokener_parse(jsonStr);
	if (!root || is_error( root )) {
		g_warning("Failed to parse '%s' into a JSON string", filePath.c_str());
		goto Done;
	}

	serviceDesc = new ServiceDescription();

	// ID: mandatory
	label = JsonGetObject(root, "id");
	if (label) {
		serviceDesc->m_id = json_object_get_string(label);
	} else {
		g_warning("service %s does not have an ID", filePath.c_str());
		goto Done;
	}

	serviceDesc->m_jsonString = json_object_to_json_string(root);

	success = true;

Done:

	if (root && !is_error(root))
		json_object_put(root);

	delete[] jsonStr;

	if (!success) {
		delete serviceDesc;
		return NULL;
	}

	return serviceDesc;
}

json_object* ServiceDescription::toJSON() const
{
	json_object* json = json_tokener_parse(m_jsonString.c_str());
	if (!json || is_error(json)) {
		g_warning("%s: Failed to parse '%s' into a JSON string", __PRETTY_FUNCTION__, m_jsonString.c_str());
		return NULL;
	}
	return json;
}

bool ServiceDescription::operator==(const ServiceDescription& cmp) const {
	return (m_id == cmp.id());
}
bool ServiceDescription::operator!=(const ServiceDescription& cmp) const {
	return (m_id != cmp.id());
}
