/* @@@LICENSE
*
*      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include <glib.h>
#include <cjson/json.h>
#include <sys/prctl.h>

#include "EventReporter.h"
#include "Settings.h"
#include "HostBase.h"
#include "MutexLocker.h"

#include "lunaservice.h"


EventReporter::EventReporter(GMainLoop* loop)
	: m_service(0) 
{
	if( Settings::LunaSettings()->collectUseStats )
	{
		char processName[64];
		:: prctl(PR_GET_NAME, (unsigned long)processName, 0, 0, 0);
		processName[sizeof(processName) - 1] = 0;

		gchar* serviceName = g_strdup_printf("com.palm.eventreporter.%s", processName);
		
		// Initialize the LunaService connection for sending app-run information
		LSError err;
		LSErrorInit(&err);
		if( !LSRegister(serviceName, &m_service, &err) ) {
			LSErrorPrint (&err, stderr);
			LSErrorFree(&err);
			m_service=0;
		} else {
			LSGmainAttach(m_service, loop, &err);
		}

		g_free(serviceName);
	}	
}

EventReporter::~EventReporter()
{
}

static EventReporter* sInstance = 0;
static const char* sDbKind = "com.palm.contextupload:1"; 

void EventReporter::init(GMainLoop* loop)
{
	if (sInstance)
		return;
	
	sInstance = new EventReporter(loop);
}

EventReporter* EventReporter::instance() 
{
	g_assert(sInstance != 0);
	return sInstance;
}

bool EventReporter::report( const char* eventName, const char* data )
{
	MutexLocker locker(&m_mutex);
	if( m_service )
	{
		json_object* obj = json_object_new_object();
		json_object_object_add(obj, "_kind", json_object_new_string(sDbKind));
		json_object_object_add(obj, "appid", json_object_new_string(data));
		json_object_object_add(obj, "event", json_object_new_string(eventName));

		json_object* objects = json_object_new_array();
		json_object_array_add(objects, obj);

		json_object* payload = json_object_new_object();
		json_object_object_add(payload, "objects", objects);

		LSError err;
		LSErrorInit(&err);
		int r = LSCall(m_service, "palm://com.palm.db/put",
					   json_object_to_json_string(payload),
					   NULL, NULL, NULL, &err);
		json_object_put(payload);
		
		if( !r ) {
			LSErrorPrint(&err, stderr);
			LSErrorFree(&err);
			return false;
		}
	}
	
	return true;
}



