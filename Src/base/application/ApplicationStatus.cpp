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




#include "Common.h"

#include "ApplicationStatus.h"

#include <glib.h>
#include <cjson/json.h>

ApplicationStatus::ApplicationStatus(json_object* item)
	: id("")
	, version("")
	, title("")
	, vendor("")
	, vendorUrl("")
	, iconPath("")
	, state(State_Unknown)
	, progress(0)
{
	json_object* prop = 0;
	
	prop = json_object_object_get(item, "id");
	if (prop) {
		id = getJsonString(prop);
	}
	
	prop = json_object_object_get(item, "details");
	if (prop && json_object_is_type(prop, json_type_object)) {
		json_object* detail = 0;

		detail = json_object_object_get(prop, "version");
		if (detail)
			version = getJsonString(detail);
		
		detail = json_object_object_get(prop, "title");
		if (detail)
			title = getJsonString(detail);

		detail = json_object_object_get(prop, "vendor");
		if (detail)
			vendor = getJsonString(detail);

		detail = json_object_object_get(prop, "vendorUrl");
		if (detail)
			vendorUrl = getJsonString(detail);

		detail = json_object_object_get(prop, "icon");
		if (detail)
			iconPath = getJsonString(detail);

		detail = json_object_object_get(prop, "progress");
		if (detail)
			progress = json_object_get_int(detail);

		detail = json_object_object_get(prop, "state");
		if (detail) {
			std::string stateStr = getJsonString(detail);

			if (stateStr == "icon download current") {
				state = State_Unknown;
				progress = 0;
			}
			else if (stateStr == "icon download complete") {
				state = State_IconDownload;
				progress = 0;
			}
			else if (stateStr == "ipk download current") {
				state = State_IpkDownloadCurrent;
			}
			else if (stateStr == "ipk download complete") {
				state = State_IpkDownloadComplete;
				// since progress is not given to us when in this state,
				// we will fake it in case this is the state it's in when
				// we receive our first state update
				progress = 100;
			}
			else if (stateStr == "ipk download paused") {
				state = State_IpkDownloadPaused;
			}
			else if (stateStr == "installing") {
				state = State_Installing;
				// since progress is not given to us when in this state,
				// we will fake it.
				progress = 100;
			}
			else if (stateStr == "canceled") {
				state = State_Canceled;
			}
			else if (stateStr == "install failed" ||
					stateStr == "remove failed" ||
					stateStr == "download failed") {
				state = State_Failed;
			}
			else 
				state = State_Unknown;
		}
	}
}

const char* ApplicationStatus::stateToStr() const
{
	switch (state) {
	case State_IconDownload: return "icon retrieved";
	case State_IpkDownloadCurrent: return "current";
	case State_IpkDownloadComplete: return "complete";
	case State_IpkDownloadPaused: return "paused";
	case State_Installing: return "installing";
	case State_Canceled: return "canceled";
	case State_Failed: return "failed";
	default: return "unknown";
	}
}

std::string ApplicationStatus::getJsonString(json_object* prop)
{
	const char* cstr = json_object_get_string(prop);
	return cstr ? cstr : "";
}

