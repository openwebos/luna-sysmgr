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




#ifndef __ApplicationStatus_h__
#define __ApplicationStatus_h__

#include "Common.h"

#include <string>

struct json_object;

struct ApplicationStatus
{
	ApplicationStatus(json_object* item);

	enum State {
		State_IconDownload,
		State_IpkDownloadCurrent,
		State_IpkDownloadComplete,
		State_IpkDownloadPaused,
		State_Installing,
		// remove pending information
		State_Canceled,
		State_Failed,
		State_Unknown
	};

	const char* stateToStr() const;

	std::string id;
	std::string version;
	std::string title;
	std::string vendor;
	std::string vendorUrl;
	std::string iconPath;
	State state;
	int progress;

private:

	std::string getJsonString(json_object* prop);
};

#endif // __ApplicationStatus_h__
