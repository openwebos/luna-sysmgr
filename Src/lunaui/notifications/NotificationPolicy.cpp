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

#include <cjson/json.h>

#include "NotificationPolicy.h"

static const char* kPolicyFile = "/etc/palm/notificationPolicy.conf";

NotificationPolicy::NotificationPolicy()
{
	init();    
}

NotificationPolicy::~NotificationPolicy()
{
    
}

int NotificationPolicy::popupAlertPriority(const std::string& appId, const std::string& windowName)
{
	return getPriority(appId, windowName, m_popupAlertPriority);
}

int NotificationPolicy::dashboardPriority(const std::string& appId, const std::string& windowName)
{
	return getPriority(appId, windowName, m_dashboardPriority);
}

int NotificationPolicy::activeBannerPriority(const std::string& appId, const std::string& windowName)
{
	return getPriority(appId, windowName, m_activeBannerPriority);
}

void NotificationPolicy::init()
{
	json_object* json = json_object_from_file((char*) kPolicyFile);
	if (!json || is_error(json))
		return;

    setPriority(json, "popupalert", m_popupAlertPriority);
	setPriority(json, "dashboard", m_dashboardPriority);
	setPriority(json, "activeBanner", m_activeBannerPriority);

	json_object_put(json);    
}

void NotificationPolicy::setPriority(json_object* json, const char* typeName,
									 PriorityMap& priorityMap)
{
	json_object* label = json_object_object_get(json, typeName);
	if (label && !is_error(json) && json_object_is_type(label, json_type_array)) {

		int priority = 0;		
		for (int i = 0; i < json_object_array_length(label); i++) {

			json_object* item = json_object_array_get_idx(label, i);
			if (item && !is_error(json) && json_object_is_type(item, json_type_object)) {

				json_object_object_foreach(item, key, val) {

					char* appId = key;
					char* windowName = json_object_get_string(val);
					priorityMap[AppWindowNamePair(appId, windowName)] = priority++;
				}
			}
		}
	}    
}

int NotificationPolicy::getPriority(const std::string& appId, 
									const std::string& windowName,
									PriorityMap& priorityMap)								
{
	PriorityMap::iterator it = priorityMap.find(AppWindowNamePair(appId, windowName));
	if (it != priorityMap.end())
		return (*it).second;

	// try with an empty window name
	it = priorityMap.find(AppWindowNamePair(appId, ""));
	if (it != priorityMap.end())
		return (*it).second;

	return defaultPriority();    
}
