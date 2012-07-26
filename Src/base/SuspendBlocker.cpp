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

#include "SuspendBlocker.h"
#include "DisplayManager.h"

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <cjson/json.h>

static int s_counter = 0;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

SuspendBlockerBase::SuspendBlockerBase(GMainLoop* mainLoop)
	: m_name(0)
	, m_id(0)
	, m_mainLoop(mainLoop)
	, m_service(0)
	, m_nestedService(0)
	, m_nestedCtxt(0)
	, m_nestedLoop(0)
	, m_registeredSuspendRequest(false)
	, m_registeredPrepareSuspend(false)
{
	pthread_mutex_lock(&s_mutex);
	s_counter++;
	if (s_counter < 0)
		s_counter = 0;
	asprintf(&m_name, "sysmgr-suspend-%08d", s_counter);
	pthread_mutex_unlock(&s_mutex);
	
	m_nestedCtxt = g_main_context_new();
	m_nestedLoop = g_main_loop_new(m_nestedCtxt, FALSE);
	m_nestedService = 0;

	setupService(m_service, m_mainLoop);
	setupService(m_nestedService, m_nestedLoop);

	if (!m_service || !m_nestedService)
		return;

	// Register with bus to known when powerd is up
	callService(m_service, SuspendBlockerBase::cbPowerdUp,
				"palm://com.palm.lunabus/signal/registerServerStatus",
				"{\"serviceName\":\"com.palm.power\"}");

	callService(m_service, SuspendBlockerBase::cbSuspendRequest,
				"palm://com.palm.bus/signal/addmatch",
				"{\"category\":\"/com/palm/power\", \"method\":\"suspendRequest\"}");
	
	callService(m_service, SuspendBlockerBase::cbPrepareSuspend,
				"palm://com.palm.bus/signal/addmatch",
				"{\"category\":\"/com/palm/power\", \"method\":\"prepareSuspend\"}");

	// FIXME: Temporarily disable. We are seeing hangs with the new LS
	callService(m_service, SuspendBlockerBase::cbResume,
				"palm://com.palm.bus/signal/addmatch",
				"{\"category\":\"/com/palm/power\", \"method\":\"resume\"}");
}

SuspendBlockerBase::~SuspendBlockerBase()
{
	// Shouldn't reach here
}

bool SuspendBlockerBase::cbSuspendRequest(LSHandle* sh, LSMessage* msg, void* ctx)
{
	if (strcmp(LSMessageGetCategory(msg), "/com/palm/power") == 0) {

		SuspendBlockerBase* s = (SuspendBlockerBase*) ctx;
		bool val = s->allowSuspend();

		char* message = 0;

		asprintf(&message, "{\"ack\":%s,\"clientId\":\"%s\"}",
				 val ? "true" : "false", s->m_id);
		
		s->callService(s->m_service, NULL,
					   "palm://com.palm.power/com/palm/power/suspendRequestAck",
					   message);

		free(message);
	}

	return true;
}

bool SuspendBlockerBase::cbPrepareSuspend(LSHandle* sh, LSMessage* msg, void* ctx)
{
	if (strcmp(LSMessageGetCategory(msg), "/com/palm/power") == 0) {

		SuspendBlockerBase* s = (SuspendBlockerBase*) ctx;
		bool val = s->allowSuspend();

		char* message = 0;
		
		asprintf(&message, "{\"ack\":%s,\"clientId\":\"%s\"}",
				 val ? "true" : "false", s->m_id);

		s->callService(s->m_service, NULL,
					   "palm://com.palm.power/com/palm/power/prepareSuspendAck",
					   message);

		// FIXME: Temporarily disable. We are seeing hangs with the new LS
		//g_main_loop_run(s->m_nestedLoop);
		s->setSuspended (true);
	}

	return true;
}

bool SuspendBlockerBase::cbResume(LSHandle* sh, LSMessage* msg, void* ctx)
{
	if (strcmp(LSMessageGetCategory(msg), "/com/palm/power") == 0) {

		SuspendBlockerBase* s = (SuspendBlockerBase*) ctx;
		
		g_warning("%s:%d %s: quitting nested loop", __PRETTY_FUNCTION__, __LINE__,
				  s->m_name);
		
		// FIXME: Temporarily disable. We are seeing hangs with the new LS
		//g_main_loop_quit(s->m_nestedLoop);
		s->setSuspended (false);
	}
	
	return true;
}

void SuspendBlockerBase::setupService(LSHandle*& service, GMainLoop* loop)
{
	bool result;
	LSError lsErr;
	LSErrorInit(&lsErr);
	
	result = LSRegister(NULL, &service, &lsErr);
	if (!result) {
		g_critical("%s:%d Failed to register SuspendBlocker service: %s",
				   __PRETTY_FUNCTION__, __LINE__, lsErr.message);
		LSErrorFree(&lsErr);
		return;
	}

	result = LSGmainAttach(service, loop, &lsErr);
	if (!result) {
		g_critical("%s:%d Failed to attach SuspendBlocker service to mainLoop: %s",
				   __PRETTY_FUNCTION__, __LINE__, lsErr.message);
		LSErrorFree(&lsErr);
		return;
	}	 
}

static bool dummyCallback(LSHandle* sh, LSMessage* message, void* ctx)
{
	return true;
}

void SuspendBlockerBase::callService(LSHandle* service, LSFilterFunc callback, const char* url, const char* message)
{
	bool result;
	LSError lsErr;
	LSErrorInit(&lsErr);

	result = LSCall(service, url, message, callback ? callback : dummyCallback,					
					this, NULL, &lsErr);
	if (!result) {
		g_critical("%s:%d Failed in LSCall with url: %s, message: %s, Error: %s",
				   __PRETTY_FUNCTION__, __LINE__, url, message, lsErr.message);
		LSErrorFree(&lsErr);
	}
}

bool SuspendBlockerBase::cbPowerdUp(LSHandle* sh, LSMessage* msg, void* ctx)
{
	struct json_object* json = json_tokener_parse(LSMessageGetPayload(msg));
	if (json && !is_error(json))  {

		json_object* label = json_object_object_get(json, "connected");
		if (label && json_object_is_type(label, json_type_boolean)) {

			bool connected = json_object_get_boolean(label);
			SuspendBlockerBase* s = (SuspendBlockerBase*) ctx;

			if (connected) {

				// Powerd up. Get our identifier
				char* message = 0;
				asprintf(&message, "{\"subscribe\":true,\"clientName\":\"%s\"}",
						 s->m_name);
			
				s->callService(s->m_service, SuspendBlockerBase::cbIdentify,
							   "palm://com.palm.power/com/palm/power/identify",
							   message);
				free(message);
			}
			else {
				
				s->m_registeredPrepareSuspend = false;
				s->m_registeredSuspendRequest = false;
			}
		}
			
		json_object_put(json);
	}

	return true;
}

bool SuspendBlockerBase::cbIdentify(LSHandle* sh, LSMessage* msg, void* ctx)
{
	struct json_object* json = json_tokener_parse(LSMessageGetPayload(msg));
	if (!json || is_error(json))
		return true;

	json_object* label = 0;
	bool subscribed = false;
	const char* clientId = 0;

	label = json_object_object_get(json, "subscribed");
	if (label && json_object_is_type(label, json_type_boolean))
		subscribed = json_object_get_boolean(label);

	label = json_object_object_get(json, "clientId");
	if (label && json_object_is_type(label, json_type_string))
		clientId = json_object_get_string(label);

	if (!subscribed || !clientId) {
		g_critical("%s: %d Failed to subscribe to powerd %s",
				   __PRETTY_FUNCTION__, __LINE__, LSMessageGetPayload(msg));	   
	}
	else {

		SuspendBlockerBase* s = (SuspendBlockerBase*) ctx;

		free(s->m_id);
		s->m_id = ::strdup(clientId);
		
		s->registerSuspendRequest();
		s->registerPrepareSuspend();


		g_warning("%s: %d Got clientId %s from powerd for %s",
				  __PRETTY_FUNCTION__, __LINE__, clientId, s->m_name);
	}

	json_object_put(json);
	return true;
}

void SuspendBlockerBase::registerSuspendRequest()
{
	if (m_registeredSuspendRequest)
		return;

	m_registeredSuspendRequest = true;
	
	// Register ourselves a client of powerd suspendRequests
	char* message = 0;
	asprintf(&message, "{\"register\":true,\"clientId\":\"%s\"}", m_id);
	callService(m_service, NULL,
				"palm://com.palm.power/com/palm/power/suspendRequestRegister",
				message);
	free(message);	  
}

void SuspendBlockerBase::registerPrepareSuspend()
{
	if (m_registeredPrepareSuspend)
		return;

	m_registeredPrepareSuspend = true;
	
	// Register ourselves a client of powerd prepareSuspendRequests
	char* message = 0;
	asprintf(&message, "{\"register\":true,\"clientId\":\"%s\"}", m_id);	 
	callService(m_service, NULL,
				"palm://com.palm.power/com/palm/power/prepareSuspendRegister",
				message);
	free(message);				
}
