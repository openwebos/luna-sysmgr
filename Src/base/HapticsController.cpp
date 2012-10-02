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

#include "HapticsController.h"


#include "Common.h"
#include "HapticsController.h"
#include "HostBase.h"
#include "JSONUtils.h"
#include "Time.h"
#include "cjson/json.h"

#include <glib.h>
#include <lunaservice.h>
#include <pthread.h>
#include <sched.h>
#include <string>
#include <sys/prctl.h>


#if defined(HAPTICS)
#include <HapticsControllerCastle.cpp>
#endif

/*! \page com_palm_vibrate Service API com.palm.vibrate/
 *
 * Public methods:
 * - \ref com_palm_vibrate_vibrate
 * - \ref com_palm_vibrate_vibrate_named_effect
 */

HapticsController* HapticsController::s_instance = NULL;

HapticsController * HapticsController::instance() {
	if(!HapticsController::s_instance) {
		#if defined(HAPTICS)
		new HapticsControllerCastle();
		#else
		new HapticsController();
		#endif
	}
	return HapticsController::s_instance;
}

HapticsController::HapticsController()
{
	m_mappingTable = g_hash_table_new(g_str_hash,g_str_equal);
	s_instance = this;
}

/*!
\page com_palm_vibrate
\n
\section com_palm_vibrate_vibrate vibrate

\e Public.

com.palm.vibrate/vibrate

Vibrate the device in periods for a specified duration or infinitely.

\subsection com_palm_vibrate_vibrate_syntax Syntax:
\code
{
    "period": int,
    "duration": int
}
\endcode

\param interval Period of the vibration in ms. \e Required.
\param duration Duration of the vibration in ms. If not specified, device will vibrate until stopped with LSCallCancel.

\subsection com_palm_vibrate_vibrate_returns Returns:
\code
{
    "returnValue": boolean,
    "errorText": string
}
\endcode

\param returnValue Indicates if the call was succesful or not.
\param errorText Describes the error if call was not succesful

\subsection com_palm_vibrate_vibrate_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.vibrate/vibrate '{ "period": 500, "duration": 4000 }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": false,
    "errorText": "Invalid arguments"
}
\endcode
*/
static bool cbVibrate(LSHandle *lh, LSMessage *m,void *ctx)
{
    // {"period": integer, "duration": integer}
    VALIDATE_SCHEMA_AND_RETURN(lh,
                               m,
                               SCHEMA_2(REQUIRED(perios, integer), REQUIRED(duration, integer)));

	HapticsController *hc = (HapticsController *) ctx;
	const char *str = LSMessageGetPayload( m );
	struct json_object *root = json_tokener_parse(str);
	struct json_object *json_period;
	struct json_object *json_duration;
	int duration = 0; //infinity
	int period = 0;
	int id;
	char reply[1024] = "{\"returnValue\":false,\"errorText\":\"Invalid arguments\"}";
	LSError lsError;
	LSErrorInit(&lsError);

	if(is_error(root)) {
		root = NULL;
		goto error;
	}
	
	json_period = json_object_object_get(root, "period");
	if(!json_period) {
		goto error;
	}
	period = json_object_get_int(json_period);

	json_duration = json_object_object_get(root, "duration");
	if(json_duration) {
		duration = json_object_get_int(json_duration);  

	}
	id = hc->vibrate(period,duration); 
	if(id < 0) {
		snprintf(reply,1024,"{\"returnValue\":false,\"errorText\":\"Unable to vibrate\"}");
		goto error;
	}
	if(duration == 0) {
		if(!LSSubscriptionAdd (lh, "com.palm.vibrate/vibrate", m, &lsError)) {
			LSErrorPrint (&lsError, stderr);
			LSErrorFree(&lsError);
		} else {
		    hc->addMapping(LSMessageGetUniqueToken(m),(void*)id);
        }
	}

	snprintf(reply,1024,"{\"returnValue\":true }");
error:
	if(root)
		json_object_put(root);
	if (!LSMessageReply( lh, m, reply, &lsError )) {
		LSErrorPrint (&lsError, stderr);
		LSErrorFree(&lsError);
	}
	return true; 
}

/*!
\page com_palm_vibrate
\n
\section com_palm_vibrate_vibrate_named_effect vibrateNamedEffect

\e Public.

com.palm.vibrate/vibrateNamedEffect

Vibrate an effect.

\subsection com_palm_vibrate_vibrate_named_effect_syntax Syntax:
\code
{
    "name": string,
    "continous": boolean
}
\endcode

\param name Name of the effect.
\param continuos If true, effect is played until stopped with LSCallCancel.

\subsection com_palm_vibrate_vibrate_named_effect_returns Returns:
\code
{
    "returnValue": boolean,
    "errorText": string
}
\endcode

\param returnValue Indicates if the call was succesful or not.
\param errorText Describes the error if call was not succesful

\subsection com_palm_vibrate_vibrate_named_effect_examples Examples:
\code
luna-send -n 1 -f luna://com.palm.vibrate/vibrateNamedEffect '{ "name": "ringtone", "continous": false }'
\endcode

Example response for a succesful call:
\code
{
    "returnValue": true
}
\endcode

Example response for a failed call:
\code
{
    "returnValue": false,
    "errorText": "Unable to vibrate"
}
\endcode
*/
static bool cbVibrateNamedEffect(LSHandle *lh, LSMessage *m, void *ctx)
{
    // {"name": string, "continous": boolean}
    VALIDATE_SCHEMA_AND_RETURN(lh,
                               m,
                               SCHEMA_2(REQUIRED(name, string), REQUIRED(continous, boolean)));

	HapticsController *hc = (HapticsController *) ctx;
	const char *str = LSMessageGetPayload( m );
	struct json_object *root = json_tokener_parse(str);
	struct json_object *json_name;
	struct json_object *json_continous;
	char *name;
	int id;
	bool continous;
	char reply[1024] = "{\"returnValue\":false,\"errorText\":\"Invalid arguments\"}";
	LSError lsError;
	LSErrorInit(&lsError);

	if(is_error(root)) {
		root = NULL;
		goto error;
	}
	
	json_name = json_object_object_get(root, "name");
	if(!json_name) {
		goto error;
	}
	name = json_object_get_string(json_name);

	id = hc->vibrate(name); 
	if(id < 0) {
		snprintf(reply,1024,"{\"returnValue\":false,\"errorText\":\"Unable to vibrate\"}");
		goto error;
	}

	json_continous = json_object_object_get(root,"continous");
	if(json_continous) {
		continous = json_object_get_boolean(json_continous);
		if(continous) {
			if(!LSSubscriptionAdd (lh, "com.palm.vibrate/vibrate", m, &lsError)) {
				LSErrorPrint (&lsError, stderr);
				LSErrorFree(&lsError);
			} else {
				hc->addMapping(LSMessageGetUniqueToken(m),(void*)id);
			}
		}
	}

	snprintf(reply,1024,"{\"returnValue\":true}");
error:
	if(root)
		json_object_put(root);
	if (!LSMessageReply( lh, m, reply, &lsError )) {
		LSErrorPrint (&lsError, stderr);
		LSErrorFree(&lsError);
	}
	return true; 
}

static bool cbCancelSubscription(LSHandle *lh, LSMessage *message, void *ctx)
{
	HapticsController *hc = (HapticsController *)ctx; 
	int id;
	id = hc->getAndRemoveMapping(LSMessageGetUniqueToken(message));
	hc->cancel(id);
	return true;
}

static LSMethod s_methods[]  = {
	{ "vibrate",	 	cbVibrate },
	//Required arguments
	//period <int> //Period in MS
	//duration <int> //In Ms, leave out for infinity
	//For infinity, use LSCallCancel to stop.
	//
	//Returns: status: true, or false together with an errorMessage. 
	{ "vibrateNamedEffect",   cbVibrateNamedEffect }, //For devices with vibeTonz? */
	//required arguments
	//name <str>
	//
	//optional arguments
	//continous <boolean> , allows the effect to be canceled by LSCallCancel()'ing the call.
	//
	//Returns: status: true, or false together with an errorMessage. 
	{ 0, 0 },
};

void HapticsController::startService()
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	GMainLoop *mainLoop = HostBase::instance()->mainLoop();

	result = LSRegister("com.palm.vibrate", &m_service, &lsError);
	if (!result)
		goto Done;

	result = LSRegisterCategory(m_service, "/", s_methods, NULL, NULL, &lsError);
	if (!result)
		goto Done;

	result = LSCategorySetData(m_service,"/", this,&lsError);
	if(!result)
		goto Done;
	
	LSSubscriptionSetCancelFunction(m_service,cbCancelSubscription,this,&lsError);
	if(!result)
		goto Done;

	result = LSGmainAttach(m_service, mainLoop, &lsError);
	if (!result)
		goto Done;

	result = LSGmainSetPriority(m_service, G_PRIORITY_HIGH, &lsError);
	if (!result)
		goto Done;

	return;
	Done:
		printf("Can't start vibrate luna service!\n");
   		LSErrorPrint (&lsError, stderr);
		LSErrorFree(&lsError); 
}
