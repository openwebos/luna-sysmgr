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




#ifndef NOTIFICATIONPOLICY_H
#define NOTIFICATIONPOLICY_H

#include "Common.h"

#include <map>
#include <memory>
#include <string>

struct json_object;

typedef std::pair<std::string, std::string> AppWindowNamePair;

class NotificationPolicy
{
public:

	NotificationPolicy();
	~NotificationPolicy();

	int popupAlertPriority(const std::string& appId, const std::string& windowName);
	int dashboardPriority(const std::string& appId, const std::string& windowName);
	int activeBannerPriority(const std::string& appId, const std::string& windowName);
	int defaultPriority() const { return 1000; }

private:

	typedef std::map<AppWindowNamePair, int> PriorityMap;

	void init();
	void setPriority(json_object* json, const char* typeName,
					 PriorityMap& priorityMap);	
	int getPriority(const std::string& appId, const std::string& windowName,
					PriorityMap& priorityMap);
	
	
	PriorityMap m_popupAlertPriority;
	PriorityMap m_dashboardPriority;
	PriorityMap m_activeBannerPriority;
};

#endif /* NOTIFICATIONPOLICY_H */
