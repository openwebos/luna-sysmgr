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





#undef min
#undef max

#include "ApplicationDescription.h"
#include "ApplicationInstaller.h"
#include "ApplicationManager.h"
#include "Common.h"
#include "HostBase.h"
#include "JSONUtils.h"
#include "MimeSystem.h"
#include "PackageDescription.h"
#include "ServiceDescription.h"
#include "Settings.h"
#include "Utils.h"
#include "WebAppMgrProxy.h"
#include "WindowServerLuna.h"

#include <cjson/json_object.h>
#include "lunaservice.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>
#include <QBitArray>
#include <QList>
#include <QUrl>


#include <cjson/json.h>
#include <glib.h>
#include <map>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <vector>

// in bytes
#define		APPINFO_SIZEOF_APPDB	(1024*1024)

#define TEST_INSTALLER_UPDATE_MANUALLY 1

/*
 * Uncomment me to enable MIME testing functions
 * 
 * (see tests/mime.cpp)
 * 
 */
// #define AMS_TEST_MIME 1

#define AMS_LOGEXTRA(c,...) do { if (c) { g_warning("ApplicationManagerService::" ); g_warning(__VA_ARGS__); }} while (0)

extern "C" bool LSMessageGetSubscribe(LSMessage *message);			//THIS SHOULD BE IN lunaservice.h


static bool s_extraLogging = false;

static std::string getAbsolutePath(const std::string& inStr,
		const std::string& parentDirectory);

#ifdef AMS_TEST_MIME
#include "../tests/mime.cpp"
#endif

static LSHandle*  listRunningApps_lshandle = 0;
QList<LSMessage*> listRunningApps_messages;

static bool servicecallback_listRunningApps( LSHandle* lshandle,
		LSMessage *message, void *user_data)
{

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_ANY);
	// Reference the message and save pointer to be used by the response callback
	LSMessageRef(message);
	listRunningApps_messages.append(message);
	listRunningApps_lshandle = lshandle;

	// only make the request if we are the only pendingRequest
	if (listRunningApps_messages.size() == 1)
		WebAppMgrProxy::instance()->serviceRequestHandler_listRunningApps(true);
	
	return true;
}

void appManagerCallback_listRunningApps( const std::string& runnigAppsJsonArray )
{
	LSError lserror;
	LSErrorInit(&lserror);

	Q_FOREACH(LSMessage* msg, listRunningApps_messages) {

		if (!LSMessageReply( listRunningApps_lshandle, msg, runnigAppsJsonArray.c_str(), &lserror))
			LSErrorFree (&lserror);
	
		LSMessageUnref(msg);
	}

	listRunningApps_messages.clear();
}


/**
	\fn com.palm.applicationManager/close (previously kill)
	\brief close an application in the system manager.
 */
static bool servicecallback_close( LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	std::string errMsg;
	struct json_object* json = 0;
	struct json_object* root = 0;
	struct json_object* processid = 0;
	bool success = false;

    // {"processId": string}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_1(REQUIRED(processId, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errMsg = "No payload provided";
		goto done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto done;
	}

	processid = json_object_object_get(root,"processId");
	if(processid) {
		WebAppMgrProxy::instance()->sendAsyncMessage(new View_Mgr_CloseByProcessId(json_object_get_string(processid)));
		// FIXME: $$$ this was now made asynchronous, so we can't find out if the call succeeded or failed. 
		success = true;
	}

	if (!success)
		errMsg = "Must provide a valid processId to close";

	done:

	json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if (!success)
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror ))
		LSErrorFree (&lserror); 

	if (root && !is_error(root))
		json_object_put(root);
	json_object_put(json);
	return true;	
}

static const char * meaninglessMimeTypes[] =
{
		"application/octet-stream",
		"application/unknown"
};

static bool isMeaninglessMimeType(const std::string& mimeStr)
{
	for (unsigned int i=0;i<sizeof(meaninglessMimeTypes)/sizeof(char *);++i)
	{
		if (mimeStr == meaninglessMimeTypes[i])
			return true;
	}
	return false;
}

/**
	\fn com.palm.applicationManager/getResourceInfo
	\brief get the content type and appid(s) of the handler(s) for this type.
 */
static bool servicecallback_getresourceinfo( LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	std::string errMsg;
	struct json_object* json=0;
	struct json_object* root=0;
	struct json_object* uri=0;
	struct json_object* mime=0;
	char * pUri_cstr=NULL;
	std::string uriStr;
	std::string mimeStr;
	bool success=false;
	std::string appId;
	std::string mimeByExtn;
	std::string extn;
	bool canStream = false;
	RedirectHandler redirectHandler;
	ResourceHandler resourceHandler;

    // {"uri": string, "mime": string }

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_2(REQUIRED(uri, string), OPTIONAL(mime, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errMsg = "No payload provided";
		goto done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto done;
	}

	uri=json_object_object_get(root,"uri");
	if (!uri) {
		errMsg = "Must provide a uri";
		goto done;
	}

	pUri_cstr = json_object_get_string(uri);
	if (!pUri_cstr) {
		errMsg = "Must provide a uri";
		goto done;
	}

	mime=json_object_object_get(root,"mime");
	if (mime) {
		char* pMime_cstr = json_object_get_string(mime);
		if (pMime_cstr) {
			mimeStr = std::string(pMime_cstr);
		}
	}

	uriStr = std::string(pUri_cstr);

	redirectHandler = MimeSystem::instance()->getActiveHandlerForRedirect(uriStr,false,true);
	if (redirectHandler.valid()) {
		success = true;
		appId = redirectHandler.appId();
		canStream = true;
		goto done;
	}

	//try a ResourceHandler
	
	if (mimeStr.size() && !isMeaninglessMimeType(mimeStr)) {
		resourceHandler = MimeSystem::instance()->getActiveHandlerForResource(mimeStr);
		if (resourceHandler.valid()) {
			success = true;
			appId = resourceHandler.appId();
			canStream = resourceHandler.stream();
			goto done;
		}
	}

	// try extension now
	if (MimeSystem::getExtensionFromUrl(uriStr,extn)) {
		if (MimeSystem::instance()->getMimeTypeByExtension(extn,mimeByExtn)) {
			resourceHandler = MimeSystem::instance()->getActiveHandlerForResource(mimeByExtn);
			if (resourceHandler.valid()) {
				success = true;
				appId = resourceHandler.appId();
				canStream = resourceHandler.stream();
				goto done;
			}
		}
	}

	redirectHandler = MimeSystem::instance()->getActiveHandlerForRedirect(uriStr,false,false);			//false parameter allows searching for command (scheme) handlers in the mime system
	// (It will only find command handlers at this point because redirect handlers were already
	//	searched for earlier)
	if (redirectHandler.valid()) {
		success = true;
		appId = redirectHandler.appId();
		canStream = resourceHandler.stream();
		goto done;
	}
	
		
	errMsg = "can't find a handler for the given uri";

	done:

	json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if (success) {
		json_object_object_add(json, "uri", json_object_new_string(uriStr.c_str()));
		if (!appId.empty()) {
			json_object_object_add(json, "appIdByExtension", json_object_new_string(appId.c_str()));
			if (!mimeByExtn.empty()) {
				json_object_object_add(json, "mimeByExtension", json_object_new_string(mimeByExtn.c_str()));
			}
			json_object_object_add(json, "canStream", json_object_new_boolean(canStream));
		}
		else {
			json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));
		}
	}
	else {
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror ))
		LSErrorFree (&lserror); 

	if (root && !is_error(root))
		json_object_put(root);
	json_object_put(json);

	return true;
}

/**
	\fn com.palm.applicationManager/getAppInfo
	\brief return the app information (title,...) for a given appid
 */
static bool servicecallback_getappinfo( LSHandle* lshandle, LSMessage *message,
		void *user_data)
{

	LSError lserror;
	LSErrorInit(&lserror);
	std::string errMsg;
	struct json_object* json=0;
	struct json_object* root=0;
	struct json_object* label=0;
	std::string appId;
	ApplicationDescription * appDesc=NULL;
	LaunchPoint * defaultLp=NULL;
	bool success=false;

    // {"appID": string }

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_1(REQUIRED(appID, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errMsg = "No payload provided";
		goto done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto done;
	}

	label = json_object_object_get(root,"appId");
	if (!label || is_error(label)) {
		errMsg = "Provide an appId";
		goto done;
	}

	appId = json_object_get_string(label);
	appDesc = ApplicationManager::instance()->getAppById(appId);
	if (appDesc == NULL) {
		errMsg = "Invalid appId specified: " + appId;
		goto done;
	}

	defaultLp = const_cast<LaunchPoint*>(appDesc->getDefaultLaunchPoint());		//to get around the goto restrictions 
	if (defaultLp == NULL) {
		errMsg = "application id: " + appId + " has no title";
		goto done;
	}

	success=true;

	done:

	json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if (success) {
		json_object_object_add(json, "appId", json_object_new_string(appId.c_str()));
		json_object_object_add(json, "appInfo", defaultLp->appDesc()->toJSON());
	}
	else {
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror))
		LSErrorFree (&lserror); 

	if (root && !is_error(root))
		json_object_put(root);
	json_object_put(json);
	return true;
}


/**
	\fn com.palm.applicationManager/install
	\brief Install an application in the system manager.
 */
static bool servicecallback_install( LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	std::string processId = "";
	std::string errMsg;
	struct json_object* root=0;
	struct json_object* label=0;
	struct json_object* target=0;
	struct json_object* auth=0;
	struct json_object* devid=0;
	struct json_object* json=0;
	std::string params;
	std::string targetAppId;
	std::string callerAppId;
	std::string callerProcessId;
	std::string pszTarget;

	const char* callerId = LSMessageGetApplicationID(message);
	unsigned long ticket;
	std::string ticketStr;
	unsigned int uncompressedAppSize = 0;

    // {"target": string, "authToken": string, "deviceId": string}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_3(REQUIRED(target, string), REQUIRED(authToken, string), REQUIRED(deviceId, string)));
	
	//rather than what's in the resource handler file
	// (currently command-resource-handlers.json)
	if (callerId)
		(void) splitWindowIdentifierToAppAndProcessId(callerId, callerAppId, callerProcessId);

	// this will be our return payload object
	json = json_object_new_object();

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errMsg = "No payload provided";
		goto done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto done;
	}

	target = json_object_object_get(root,"target");
	auth = json_object_object_get(root,"authToken");
	devid = json_object_object_get(root,"deviceId");

	if (!target)
	{
		errMsg = "No target ipkg specified";
		goto done;
	}

	// We have a resource URL to open. Try to find the correct application to launch

	pszTarget = json_object_get_string(target);
	if( pszTarget.empty())
	{
		errMsg = "Not a valid target to install";
		goto done;
	}

	g_debug ("target, argument #%s#\n", pszTarget.c_str() );
	if (!ApplicationInstaller::isValidInstallURI (pszTarget))
	{
		errMsg = "Not a valid install target";
		goto done;
	}

	if (!ApplicationManager::instance()->isTrustedInstallerApp (callerAppId)) {
		errMsg = "Unauthorized application, cannot install";
		goto done;
	}

	if ((label = JsonGetObject(root,"uncompressedSize")) != NULL)
	{
		uncompressedAppSize = json_object_get_int(label);
	}

	ticket = ApplicationManager::generateNewTicket();
	g_warning( "ApplicationManager: attempting to install [%s] (ticket = %lu)",pszTarget.c_str(), ticket);
	ticketStr = toSTLString<int>(ticket);

	if (ApplicationManager::isRemoteFile (pszTarget.c_str())) {
		g_debug ("Remote file detected, download and install");

		if (!ApplicationInstaller::instance()->downloadAndInstall(lshandle, pszTarget, auth, devid, ticket, LSMessageIsSubscription (message))) {
			errMsg = "Failure to download and install";
			goto done;
		}

		// if download request succeeded, then add subscription here
		if (LSMessageIsSubscription (message) && !LSSubscriptionAdd (lshandle, ticketStr.c_str(), message, &lserror)) {
			LSErrorPrint (&lserror, stderr);
			LSErrorFree (&lserror);
			errMsg  = "subscription add failed";
			goto done;
		}
	}
	else
	{
		if (!ApplicationInstaller::instance()->install(pszTarget,uncompressedAppSize,ticket)) {
			errMsg = "Failure in installation";
			goto done;
		}
	}

	done:
	if (!errMsg.empty()) {
		json_object_object_add(json, "returnValue", json_object_new_boolean(false));
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));
	}
	else {
		json_object_object_add(json, "returnValue", json_object_new_boolean(true));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror ))
		LSErrorFree (&lserror); 

	if (root && !is_error(root))
		json_object_put( root );
	json_object_put(json);
	return true;
}



/**
  \fn com.palm.applicationManager/open
  \brief Open/Launch an application in the system manager.
 */
static bool servicecallback_open( LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	std::string processId = "";
	std::string errMsg;
	bool success = false;
	struct json_object* root=0;
	struct json_object* appid=0;
	struct json_object* paramblock=0;
	struct json_object* target=0;
	struct json_object* targetMime=0;
	struct json_object* auth=0;
	struct json_object* devid=0;
	struct json_object* ovrhandlerappid=0;
	struct json_object* json=0;
	struct json_object* activityMgrParam=0;
	struct json_object* trueFileNameParam=0;

	std::string params;
	const char* callerId = LSMessageGetApplicationID(message);
	std::string targetAppId;
	std::string callerAppId;
	std::string callerProcessId;
	std::string ovrHandlerAppId ="";			//if specified, this appid will be used to open a resource
	std::string targetUri;
	ResourceHandler resourceHandler;
	std::string appArgUrl;
	RedirectHandler redirectHandler;
	std::string strMime;
	std::string strExtn;
	std::string trueFileName;

    // {"id": string, "target": string, "mime": string, "params": [ string, object ], "authToken": object, "deviceId": object, "overrideHandlerAppId": string, "fileName": string}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_8(REQUIRED(id, string), REQUIRED(target, string), REQUIRED(mime, string),
                                        REQUIRED_UNION_2(params, object, string), REQUIRED(authToken, object),
                                        REQUIRED(deviceId, object), REQUIRED(overrideHandlerAppId, string),
                                        REQUIRED(fileName, string)));

	//rather than what's in the resource handler file
	// (currently command-resource-handlers.json)
	if (callerId)
		(void) splitWindowIdentifierToAppAndProcessId(callerId, callerAppId, callerProcessId);

	// this will be our return payload object
	json = json_object_new_object();

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errMsg = "No payload provided";
		goto done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto done;
	}

	appid = json_object_object_get(root,"id");
	target = json_object_object_get(root,"target");
	targetMime = json_object_object_get(root,"mime");
	paramblock = json_object_object_get(root,"params");
	auth = json_object_object_get(root,"authToken");
	devid = json_object_object_get(root,"deviceId");
	ovrhandlerappid = json_object_object_get(root,"overrideHandlerAppId");
	activityMgrParam = json_object_object_get(root,"$activity");
	trueFileNameParam = json_object_object_get(root,"fileName");

	if (is_error(trueFileNameParam))
		trueFileNameParam = 0;
	if (is_error(activityMgrParam))
		activityMgrParam  = 0;
	if (is_error(paramblock))
		paramblock = 0;

	if (paramblock) {
		if (json_object_is_type(paramblock,json_type_object))
		{
			if (activityMgrParam)
				json_object_object_add(paramblock,"$activity",json_object_get(activityMgrParam));	//inject it into the param block
			params = json_object_to_json_string( paramblock );
		}
		else
		{
			//the param object was not a json object...just treat it as a string and don't do anything to it
			params = json_object_get_string(paramblock);
		}
	}
	else
	{
		//create a param block
		paramblock = json_object_new_object();
		if (activityMgrParam)
			json_object_object_add(paramblock,"$activity",json_object_get(activityMgrParam));
		// add it to the root so that it'll be deleted automatically later
		json_object_object_add(root,"params",paramblock);
		params = json_object_to_json_string( paramblock );
	}

	if ((ovrhandlerappid) && (!is_error(ovrhandlerappid)))
		ovrHandlerAppId=json_object_get_string(ovrhandlerappid);

	if (appid)
	{
		// we'll assume this is an appId, and we'll launch it.
		std::string url = json_object_get_string(appid);
		processId = WebAppMgrProxy::instance()->appLaunch(url, params, callerAppId, callerProcessId, errMsg);
		if (!processId.empty()) {
			success = true;
			json_object_object_add(json, "processId", json_object_new_string(processId.c_str()));
		}
		goto done;
	}

	if (!target) {
		errMsg = "Unable to process command. Provide a valid \"id\" or \"target\" field";
		goto done;
	}
	// We have a resource URL to open. Try to find the correct application to launch

	if (targetMime) {
		const char* mimeStr = json_object_get_string(targetMime);
		if (mimeStr) {
			strMime = mimeStr;
		}
	}

	targetUri = json_object_get_string(target);
	if (targetUri.empty()) {
		errMsg = "empty target name, or not-a-string";
		goto done;
	}

	if (ApplicationManager::isAppPackage (targetUri.c_str()) && !ApplicationManager::instance()->isTrustedInstallerApp (callerAppId))
	{
		errMsg = "Unauthorized call to open an ipk";
		goto done;
	}

	if (trueFileNameParam)
	{
		trueFileName = json_object_get_string(trueFileNameParam);
		appArgUrl = std::string("{ \"target\":\"") + targetUri.c_str() + std::string("\" , \"fileName\":\"")+trueFileName+std::string("\"}");
	}
	else
	{
		appArgUrl = std::string("{ \"target\":\"") + targetUri.c_str() + std::string("\"}");
	}
	redirectHandler = MimeSystem::instance()->getActiveHandlerForRedirect(targetUri,false,true);			//true argument limits to only redirects (and not command (scheme) handlers)
	if (redirectHandler.valid()) {
		// A redirected URL implies streaming.
		g_message( "LAUNCH APP ID %s with %s", redirectHandler.appId().c_str(), targetUri.c_str() );
		processId = WebAppMgrProxy::instance()->appLaunch(redirectHandler.appId(), appArgUrl, callerAppId, callerProcessId, errMsg);
		if (!processId.empty()) {
			success = true;
			json_object_object_add(json, "processId", json_object_new_string(processId.c_str()));
		}
		goto done;
	}
	g_debug ("No redirect handler");

	if (strMime.empty()) {
		MimeSystem::getExtensionFromUrl(targetUri,strExtn);
		//map to mime type
		MimeSystem::instance()->getMimeTypeByExtension(strExtn,strMime);
	}

	resourceHandler = MimeSystem::instance()->getActiveHandlerForResource(strMime);
	if (resourceHandler.valid()) {
		if (resourceHandler.stream()) {
			// Oct. 2008: in the command-resource-handlers.json file, the "stream" argument currently means the following:
			// "Don't try to download the resource...just launch the given app listed as the handler, 
			// and pass { "target":<passed in URL/URI> } as the parameter

			if (ovrHandlerAppId.empty()) {
				targetAppId = resourceHandler.appId();
			}
			else {
				targetAppId = ovrHandlerAppId;
			}

			processId = WebAppMgrProxy::instance()->appLaunch(targetAppId, appArgUrl, callerAppId, callerProcessId, errMsg);
			if (!processId.empty()) {
				success = true;
				json_object_object_add(json, "processId", json_object_new_string(processId.c_str()));
			}
			goto done;
		}
		else {
			if (ApplicationManager::isRemoteFile (targetUri.c_str())) {
				/* download this file */
				g_debug ("%s:%d detected remote file\n", __FILE__, __LINE__);

				unsigned long ticket = ApplicationManager::generateNewTicket();
				std::string ticketStr = toSTLString<unsigned long>(ticket);

				bool retval;
				LSError lserror;
				LSErrorInit (&lserror);
				json_object* downloadParams = json_object_new_object();
				json_object_object_add (downloadParams, "target", json_object_new_string (targetUri.c_str()));
				if (!strMime.empty())
					json_object_object_add (downloadParams, "mime", json_object_new_string (strMime.c_str()));
				if (auth && devid) {
					json_object_object_add (downloadParams, "authToken", auth);
					json_object_object_add (downloadParams, "deviceId", devid);
				}

				json_object_object_add (downloadParams, "subscribe", json_object_new_boolean (true));

				g_debug ("sending download request to download manager with params: %s\n", json_object_to_json_string (downloadParams));

				ApplicationManager::DownloadRequest* req = new ApplicationManager::DownloadRequest (ticket, ovrHandlerAppId, strMime, LSMessageIsSubscription (message));
				retval = LSCall (lshandle, "palm://com.palm.downloadmanager/download", json_object_to_json_string (downloadParams),
						ApplicationManager::cbDownloadManagerUpdate, req, NULL, &lserror); 
				json_object_put (downloadParams);
				if (!retval) {
					LSErrorPrint (&lserror, stderr);
					LSErrorFree (&lserror);
					delete req;
					errMsg = "Download failed";
					goto done;
				}

				// if download request succeeded, then add subscription here
				if (LSMessageIsSubscription (message) && !LSSubscriptionAdd (lshandle, ticketStr.c_str(), message, &lserror)) {
					LSErrorPrint (&lserror, stderr);
					LSErrorFree (&lserror);
					errMsg  = "subscription add failed";
					goto done;
				}

				json_object_object_add(json, "returnValue", json_object_new_boolean(true));
				json_object_object_add(json, "ticket", json_object_new_string (ticketStr.c_str()));
				json_object_object_add(json, "result", json_object_new_string("Download requested"));
				if (LSMessageIsSubscription (message)) 
					json_object_object_add(json, "subscribed", json_object_new_boolean (true));
				else
					json_object_object_add(json, "subscribed", json_object_new_boolean (false));
				goto done;

			}
			else {
				//targetAppId = resourceHandler->appId();
				targetAppId = resourceHandler.appId();
				processId = WebAppMgrProxy::instance()->appLaunch(targetAppId, appArgUrl, callerAppId, callerProcessId, errMsg);
				if (!processId.empty()) {
					success = true;
					json_object_object_add(json, "processId", json_object_new_string(processId.c_str()));
				}
				goto done;

			}
		}
	}

	redirectHandler = MimeSystem::instance()->getActiveHandlerForRedirect(targetUri,false,false);			//false parameter allows searching for command (scheme) handlers in the mime system
	// (It will only find command handlers at this point because redirect handlers were already
	//	searched for earlier)
	if (redirectHandler.valid()) {
		g_debug ("Command handler detected");
		targetAppId = redirectHandler.appId();
		processId = WebAppMgrProxy::instance()->appLaunch(targetAppId, appArgUrl, callerAppId, callerProcessId, errMsg);
		if (!processId.empty()) {
			success = true;
			json_object_object_add(json, "processId", json_object_new_string(processId.c_str()));
		}
		goto done;
	}
	g_debug ("No command handler");

	// exhausted all choices
	errMsg = "No handler for " + targetUri;

	done:

	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if (!success) {
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror ))
		LSErrorFree (&lserror); 

	if (root && !is_error(root))
		json_object_put( root );
	json_object_put(json);
	return true;
}

/**
	\fn com.palm.applicationManager/launch
	\brief Launch an application in the system manager.

	This differs from open in that it JUST blindly launches the specified id
 */
static bool servicecallback_launch( LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	std::string errMsg;
	std::string processId;
	json_object * label = 0;
	json_object * root = 0;
	json_object * activityMgrParam = 0;
	std::string id;
	std::string params;
	const char* caller = LSMessageGetApplicationID(message);
	std::string callerAppId;
	std::string callerProcessId;
	bool success=false;

    // {"id": object { "lable" :string }, "params": [ string, object ]}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_2(NAKED_OBJECT_REQUIRED_1(id, label, string), REQUIRED_UNION_2(params, object, string)));
	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errMsg = "No payload provided";
		goto Done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto Done;
	}

	label = json_object_object_get(root,"id");
	if (!label) {
		g_message("ApplicationManagerService:: servicecallback_launch(): can't parse input json - id");
		errMsg = "Unable to process command. Provide a valid \"id\" field";
		goto Done;
	}
	id = json_object_get_string(label);
	if (id.length() == 0) {
		g_message("ApplicationManagerService:: servicecallback_launch(): invalid id specified");
		errMsg = "Invalid id specified";
		goto Done;
	}

	label = json_object_object_get(root,"params");
	activityMgrParam = json_object_object_get(root,"$activity");

	if (is_error(activityMgrParam))
		activityMgrParam  = 0;
	if (is_error(label))
		label = 0;

	if (label) {
		if (json_object_is_type(label,json_type_object))
		{
			if (activityMgrParam)
				json_object_object_add(label,"$activity",json_object_get(activityMgrParam));	//inject it into the param block
			params = json_object_to_json_string( label );
		}
		else
		{
			//the param object was not a json object...just treat it as a string and don't do anything to it
			params = json_object_get_string(label);
		}
	}
	else
	{
		//create a param block
		label = json_object_new_object();
		if (activityMgrParam)
			json_object_object_add(label,"$activity",json_object_get(activityMgrParam));
		// add it to the root so that it'll be deleted automatically later
		json_object_object_add(root,"params",label);
		params = json_object_to_json_string( label );
	}

	if (caller)
		splitWindowIdentifierToAppAndProcessId(caller, callerAppId, callerProcessId);

	g_message("ApplicationManagerService:: servicecallback_launch(): launching as: appId = [%s] , param json = [%s]\n",
			id.c_str(), params.c_str());

	processId = WebAppMgrProxy::instance()->appLaunch(id, params, callerAppId, callerProcessId, errMsg);
	success = !processId.empty();

	Done:

	if( root && !is_error(root) ) 
		json_object_put( root );

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if (success)
		json_object_object_add(json, "processId", json_object_new_string(processId.c_str()));
	else
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror ))
		LSErrorFree (&lserror);

	json_object_put(json);

	return true;

}

/**
	ListApps: This returns all the registered apps
 */
static bool servicecallback_listApps(LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* json = 0;
	json_object* array = 0;

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_ANY);

	ApplicationManager* appMgr  = ApplicationManager::instance();
	std::vector<ApplicationDescription*> apps = appMgr->allApps();

	json = json_object_new_object();
	if (json == NULL)
		return false;

	array = json_object_new_array();
	if (array) {
		json_object_object_add(json, "returnValue", json_object_new_boolean(true));
		for (std::vector<ApplicationDescription*>::iterator it = apps.begin(); it != apps.end(); ++it) {

			json_object_array_add(array, (*it)->toJSON());
		}
		json_object_object_add(json, "apps", array);
	}
	else {
		json_object_object_add(json, "returnValue", json_object_new_boolean(false));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror ))
		LSErrorFree (&lserror); 

	json_object_put(json);
	return true;
}

/**
	ListPackages: This returns all the registered packages and their apps and service descriptions
 */
static bool servicecallback_listPackages(LSHandle* lshandle, LSMessage *message, void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* json = NULL;
	json_object* array = NULL;

	ApplicationManager* appMgr  = ApplicationManager::instance();
	std::map<std::string, PackageDescription*> packages = appMgr->allPackages();

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_ANY);

	json = json_object_new_object();
	if (json == NULL)
		return false;

	array = json_object_new_array();
	if (array) {
		json_object_object_add(json, "returnValue", json_object_new_boolean(true));
	    for (std::map<std::string, PackageDescription*>::const_iterator it = packages.begin(); it != packages.end(); ++it) {
			PackageDescription* packageDesc = (*it).second;
			json_object* packageJson = packageDesc->toJSON();
			if (!packageJson) {
				continue;
			}

			// App catalog wants us to copy over some of the app properties to the package.
			std::string appId = packageDesc->appIds().front();
			if (appId != "") {
				ApplicationDescription* appDesc = appMgr->getAppById(appId);
				if (appDesc) {
					if (packageDesc->isOldStyle()) {
						json_object_object_add(packageJson, (char*) "loc_name", json_object_new_string(appDesc->title().c_str()));
						json_object_object_add(packageJson, (char*) "vendor", json_object_new_string(appDesc->vendorName().c_str()));
						json_object_object_add(packageJson, (char*) "vendorUrl", json_object_new_string(appDesc->vendorUrl().c_str()));
						json_object_object_add(packageJson, (char*) "icon", json_object_new_string(appDesc->launchPoints().front()->iconPath().c_str()));
						json_object_object_add(packageJson, (char*) "miniicon", json_object_new_string(appDesc->miniIconUrl().c_str()));
					}
					json_object_object_add(packageJson, (char*) "userInstalled",json_object_new_boolean(appDesc->isRemovable() && !appDesc->isUserHideable()));
				}
			}


			// We remove the app/apps and services arrays of IDs (that came from packageinfo.json) and instead add them with the full descriptions
			// i.e. (we need to include an array of app descriptors instead of an array of app ids for listPackages)
			json_object* label = JsonGetObject(packageJson, "app");
			if (label) {
				json_object_object_del(packageJson, (char*) "app");
			} else {
				label = JsonGetObject(packageJson, (char*) "apps");
				if (label) {
					json_object_object_del(packageJson, (char*) "apps");
				}
			}
			label = JsonGetObject(packageJson, "services");
			if (label) {
				json_object_object_del(packageJson, (char*) "services");
			}

			// Add the array of app descriptions
			json_object* apps = json_object_new_array();
			std::vector<std::string>::const_iterator appIdIt, appIdItEnd;
			for (appIdIt = packageDesc->appIds().begin(), appIdItEnd = packageDesc->appIds().end(); appIdIt != appIdItEnd; ++appIdIt) {
				ApplicationDescription* appDesc = appMgr->getAppById(*appIdIt);
				if (appDesc) {
					json_object_array_add(apps, appDesc->toJSON());
				} else {
					g_warning("%s: Application with appId %s was not found", __PRETTY_FUNCTION__, (*appIdIt).c_str());
				}
			}
			json_object_object_add(packageJson, "apps", apps);

			// Add the array of service descriptions
			json_object* services = json_object_new_array();
			std::vector<std::string>::const_iterator serviceIdIt, serviceIdItEnd;
			for (serviceIdIt = packageDesc->serviceIds().begin(), serviceIdItEnd = packageDesc->serviceIds().end(); serviceIdIt != serviceIdItEnd; ++serviceIdIt) {
				ServiceDescription* serviceDesc = appMgr->getServiceInfoByServiceId(*serviceIdIt);
				if (serviceDesc) {
					json_object_array_add(services, serviceDesc->toJSON());
				} else {
					g_warning("%s: Service with serviceId %s was not found", __PRETTY_FUNCTION__, (*serviceIdIt).c_str());
				}
			}
			json_object_object_add(packageJson, "services", services);


			json_object_array_add(array, packageJson);
	    }
		json_object_object_add(json, "packages", array);
	}
	else {
		json_object_object_add(json, "returnValue", json_object_new_boolean(false));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror ))
		LSErrorFree (&lserror);

	json_object_put(json);
	return true;

}

static bool servicecallback_getSizeOf(LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	json_object* label = 0;
	array_list* appIdArray = 0;
	std::string appId;
	std::string errorText;
	std::vector<std::pair<std::string,uint32_t> > sizes;
	bool includeDbSize = true;

    // {"includeDbSize": bool, "appIds": array}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_2(REQUIRED(includeDbSize, boolean), REQUIRED(appIds, array)));
	
	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_getSizeOf;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_getSizeOf;
	}

	if ((label = JsonGetObject(root,"includeDbSize")) != NULL)
		includeDbSize = json_object_get_boolean(label);
	
	//extract the appIds array:
	//  "appIds":["appid1","appid2",...]

	if ((label = JsonGetObject(root,"appIds")) == NULL) {
		//check for the singular "appId" parameter
		if (extractFromJson(root,"appId",appId) == false) {
			errorText = "Missing 'appIds' array parameter (or alternately, the 'appId' string parameter)";
			goto Done_servicecallback_getSizeOf;
		}
		else {
			PackageDescription* packageDesc = ApplicationManager::instance()->getPackageInfoByAppId(appId);
			if (!packageDesc) {
				errorText = "Could not find the PackageDescription for the appId";
				goto Done_servicecallback_getSizeOf;
			}
			uint32_t s = ApplicationInstaller::getSizeOfPackageById(packageDesc->id());
			if (includeDbSize && s > 0)
				s += APPINFO_SIZEOF_APPDB;
			sizes.push_back(std::pair<std::string,uint32_t>(appId,s));
		}
	}
	else {
		appIdArray = json_object_get_array(label);
		if (!appIdArray || is_error(appIdArray)) {
			errorText = "Bad 'appIds' array parameter specified";
			goto Done_servicecallback_getSizeOf;
		}

		if (array_list_length(appIdArray) <= 0) {
			errorText = "Bad 'appIds' array parameter specified";
			goto Done_servicecallback_getSizeOf;
		}
		
		for (int i = 0; i < array_list_length(appIdArray); i++) {
			json_object* obj = (json_object*) array_list_get_idx(appIdArray, i);
			appId = json_object_get_string(obj);
			if (appId.length()) {
				uint32_t s = ApplicationInstaller::getSizeOfPackageById(appId);
				if (includeDbSize && s > 0)
					s += APPINFO_SIZEOF_APPDB;
				sizes.push_back(std::pair<std::string,uint32_t>(appId,s));
			}
		}
	}
	
	Done_servicecallback_getSizeOf:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		for (std::vector<std::pair<std::string,uint32_t> >::iterator it = sizes.begin();it != sizes.end();++it) 
		{
			json_object_object_add(reply,(char*)(*it).first.c_str(),json_object_new_int((*it).second));
		}
	}

	if (!LSMessageReply(lshandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

/**
 * 
 * List apps by prefix match of the partial/whole search term provided against the keywords of each app
 * 
 * AND against the title/long name and "menu name"/short name of each app
 * 
 * 
 */

static bool servicecallback_searchForApps(LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	json_object* root = 0;
	json_object* returnObj = json_object_new_object();
	json_object* array = 0;

	SearchSet matchedByTitle, matchedByKeyword;
	SearchSet::const_iterator it, itEnd;

	std::string keyword, errMsg;

    // {"keyword": string}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_1(REQUIRED(keyword, string)));

	const char* str = LSMessageGetPayload(message);
	if (!str) {
		errMsg = "No payload provided";
		goto Done;
	}

	root = json_tokener_parse(str);
	if (!root || is_error(root)) {
		root = NULL;
		errMsg = "Malformed JSON detected in payload";
		goto Done;
	}

	if (!(extractFromJson(root, "keyword", keyword))) {
		errMsg = "parameters must contain a 'keyword' (string)";
		goto Done;
	}

	ApplicationManager::instance()->searchLaunchPoints(matchedByTitle, matchedByKeyword, keyword);

	array = json_object_new_array();

	g_message("title match ordering:");
	// first include title sorted matches from the title string
	for (it = matchedByTitle.begin(), itEnd = matchedByTitle.end(); it != itEnd; ++it) {

		const LaunchPoint* lp = *it;
		if (!lp)
			continue;
		json_object * result = json_object_new_object();
		g_message("\t%s", lp->title().c_str());
		json_object_object_add(result, "launchPoint", json_object_new_string(lp->launchPointId().c_str()));
		json_object_array_add(array, result);
	}

	g_message("keyword match ordering:");
	// followed by title sorted matches from the keyword/appmenu string
	for (it = matchedByKeyword.begin(), itEnd = matchedByKeyword.end(); it != itEnd; ++it) {

		const LaunchPoint* lp = *it;
		if (!lp)
			continue;
		json_object * result = json_object_new_object();
		g_message("\t%s", lp->title().c_str());
		json_object_object_add(result, "launchPoint", json_object_new_string(lp->launchPointId().c_str()));
		json_object_array_add(array, result);
	}

	json_object_object_add(returnObj, "apps", array);

	Done:

	json_object_object_add(returnObj, "returnValue", json_object_new_boolean(errMsg.empty()));
	if (!errMsg.empty()) {
		json_object_object_add(returnObj, "errorCode", json_object_new_string(errMsg.c_str()));
	}

	LSError lserror;
	LSErrorInit(&lserror);
	if (!LSMessageReply(lshandle, message, json_object_to_json_string(returnObj), &lserror))
		LSErrorFree (&lserror); 

	if (returnObj)
		json_object_put(returnObj);

	if (root && !is_error(root))
		json_object_put(root);

	return true;
}


/**
	ListLaunchPoints: This returns all the launchPoints
 */
static bool servicecallback_listLaunchPoints(LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* json = 0;
	json_object* array = 0;

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_ANY);

	ApplicationManager* appMgr  = ApplicationManager::instance();
	std::vector<const LaunchPoint*> launchPoints = appMgr->allLaunchPoints();

	json = json_object_new_object();
	if (json == NULL)
		return false;

	array = json_object_new_array();
	if (array) {

		json_object_object_add(json, "returnValue", json_object_new_boolean(true));
		for (std::vector<const LaunchPoint*>::iterator it = launchPoints.begin();
			it != launchPoints.end(); ++it) {

			json_object_array_add(array, (*it)->toJSON());
		}
		json_object_object_add(json, "launchPoints", array);
	}
	else {
		json_object_object_add(json, "returnValue", json_object_new_boolean(false));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror )) {
		LSErrorFree (&lserror);
	}

	json_object_put(json);
	return true;
}

/**
	ListLaunchPoints: This returns all the dockLaunchPoints
 */
static bool servicecallback_listDockModeLaunchPoints(LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* json = 0;
	json_object* array = 0;

	ApplicationManager* appMgr  = ApplicationManager::instance();
	std::vector<const LaunchPoint*> launchPoints = appMgr->allDockModeLaunchPoints();
	const std::set<const LaunchPoint*>& enabledLaunchPoints = appMgr->enabledDockModeLaunchPoints();

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_ANY);

	json = json_object_new_object();
	if (json == NULL)
		return false;

	array = json_object_new_array();
	if (array) {

		json_object_object_add(json, "returnValue", json_object_new_boolean(true));
		for (std::vector<const LaunchPoint*>::iterator it = launchPoints.begin();
			it != launchPoints.end(); ++it) {
			json_object* obj = (*it)->toJSON();
			if (enabledLaunchPoints.find (*it) == enabledLaunchPoints.end())
				json_object_object_add (obj, "enabled", json_object_new_boolean(false));
			else
				json_object_object_add (obj, "enabled", json_object_new_boolean(true));

			json_object_array_add(array, obj);
		}
		json_object_object_add(json, "launchPoints", array);
	}
	else {
		json_object_object_add(json, "returnValue", json_object_new_boolean(false));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror )) {
		LSErrorFree (&lserror);
	}

	json_object_put(json);
	return true;
}

/**
	ListPendingLaunchPoints: This returns all the launchPoints which are pending an update or a new install
 */
static bool servicecallback_listPendingLaunchPoints(LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* json = 0;
	json_object* array = 0;

	ApplicationManager* appMgr  = ApplicationManager::instance();
	std::vector<const LaunchPoint*> launchPoints = appMgr->allPendingLaunchPoints();

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_ANY);

	json = json_object_new_object();
	if (json == NULL)
		return false;

	array = json_object_new_array();
	if (array != NULL) {

		json_object_object_add(json, "returnValue", json_object_new_boolean(true));
		for (std::vector<const LaunchPoint*>::iterator it = launchPoints.begin();
			it != launchPoints.end(); ++it) {

			json_object_array_add(array, (*it)->toJSON());
		}
		json_object_object_add(json, "launchPoints", array);
	}
	else {
		json_object_object_add(json, "returnValue", json_object_new_boolean(false));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror )) {
		LSErrorFree (&lserror);
	}

	json_object_put(json);
	return true;
}

/**
	listDockPoints: This returns the list of items sitting on the quick launch bar
 */
static bool servicecallback_listDockPoints(LSHandle* lsHandle, LSMessage* message,
		void* user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* json = 0;
	json_object* array = 0;

	ApplicationManager* appMgr = ApplicationManager::instance();
	std::vector<const LaunchPoint*> dockPoints = appMgr->dockLaunchPoints();

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_ANY);

	json = json_object_new_object();
	if (json == NULL)
		return false;

	array = json_object_new_array();
	if (array) {

		json_object_object_add(json, "returnValue", json_object_new_boolean(true));
		for (std::vector<const LaunchPoint*>::iterator it = dockPoints.begin();
			it != dockPoints.end(); ++it) {

			json_object_array_add(array, (*it)->toJSON());
		}
		json_object_object_add(json, "dockPoints", array);

	}
	else {
		json_object_object_add(json, "returnValue", json_object_new_boolean(false));
	}

	if (!LSMessageReply( lsHandle, message, json_object_to_json_string(json), &lserror )) {
		LSErrorFree(&lserror);
	}

	json_object_put(json);
	return true;
}

/**
	addDockModeLaunchPoint: Adds the launchpoint represented by the passed appId to the dock mode
 */
static bool servicecallback_addDockModeLaunchPoint(LSHandle* lsHandle, LSMessage* message, void* user_data) {
	json_object* root = 0;
	json_object* appId = 0;
	json_object* returnJson = 0;
	bool returnValue = true;
	LSError lserror;
	LSErrorInit(&lserror);

    // {"appID": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(appID, string)));

	const char* str = LSMessageGetPayload(message);
	if (!str)
		return false;

	root = json_tokener_parse(str);
	if (is_error(root)) {
		g_warning("%s: malformed json\n", __PRETTY_FUNCTION__);
		return true;
	}

	appId = json_object_object_get(root, "appId");
	if (appId && !is_error(appId)) {
		returnValue = ApplicationManager::instance()->enableDockModeLaunchPoint(json_object_get_string(appId));
	}

	returnJson = json_object_new_object();
	json_object_object_add(returnJson, "returnValue", json_object_new_boolean(returnValue));
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(returnJson), &lserror)) {
		LSErrorFree(&lserror);
	}

	if (root && !is_error(root))
		json_object_put(root);

	json_object_put(returnJson);
	return true;
}

/**
	removeDockModeLaunchPoint: Adds the launchpoint represented by the passed appId to the dock mode
 */
static bool servicecallback_removeDockModeLaunchPoint(LSHandle* lsHandle, LSMessage* message, void* user_data) {
	json_object* root = 0;
	json_object* appId = 0;
	json_object* returnJson = 0;
	bool returnValue = false;
	LSError lserror;
	LSErrorInit(&lserror);

    // {"appID": string }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(appID, string)));

	const char* str = LSMessageGetPayload(message);
	if (!str)
		return false;

	root = json_tokener_parse(str);
	if (is_error(root)) {
		g_warning("%s: malformed json\n", __PRETTY_FUNCTION__);
		return true;
	}

	appId = json_object_object_get(root, "appId");
	if (appId) {
		returnValue = ApplicationManager::instance()->disableDockModeLaunchPoint(json_object_get_string(appId));
	}

	returnJson = json_object_new_object();
	json_object_object_add(returnJson, "returnValue", json_object_new_boolean(returnValue));
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(returnJson), &lserror)) {
		LSErrorFree(&lserror);
	}

	if (root && !is_error(root))
		json_object_put(root);

	json_object_put(returnJson);
	return true;
}

/**
	Rescan

	rescan/load the running applications.
 */
static bool servicecallback_rescan( LSHandle* lshandle,
		LSMessage * message, void * /*user_data*/)
{
	json_object* json = 0;
	LSError lserror;
	LSErrorInit(&lserror);

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_ANY);

	ApplicationManager::instance()->scan();

	json = json_object_new_object();
	if (!json || is_error(json))
		goto Done;

	json_object_object_add(json, "returnValue",json_object_new_boolean(true));

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror )) {
		LSErrorFree(&lserror);
	}

	Done:
	if (json)
		json_object_put(json);

	return true;
}


static bool servicecallback_inspect( LSHandle* lshandle, LSMessage* message, void* user_data )
{
	std::string result;
	struct json_object* root=0;
	struct json_object* processid=0;

    // {"processID": string }

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_1(REQUIRED(processID, string)));

	const char* str = LSMessageGetPayload(message);
	if( !str )
		return false;

	root = json_tokener_parse(str);
	if( is_error(root) )
	{
		g_message("servicecallback_inspect: malformed json\n");
		return true;
	}

	processid = json_object_object_get(root,"processId");
	if( processid )
	{
		WebAppMgrProxy::instance()->inspect( json_object_get_string(processid) );
		//json_object_put( processid );
	}

	if( root && !is_error(root) ) json_object_put( root );
	return true;
}

/**
	addLaunchPoint: Add a Dynamic launchpoint
 */
static bool servicecallback_addLaunchPoint(LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	g_message("servicecallback_addLaunchPoint\n");
	LSError lserror;
	LSErrorInit(&lserror);
	std::string errMsg;
	struct json_object* root = 0;
	struct json_object* label = 0;
	struct json_object* json = 0;
	ApplicationDescription* appDesc = 0;	
	std::string id, title, icon, params,menuName;
	std::string launchId;
	gchar* dirPath = 0;
	bool removable = true;
	bool success = false;
	char* paramsStr = 0;

    // {"id": string, "title": string, OPTIONAL: "appMenu": string, "icon": string, "params": string, "removable": boolean}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_6(REQUIRED(id, string), REQUIRED(title, string), OPTIONAL(appMenu, string),
                                        REQUIRED(icon, string), REQUIRED(params, string), REQUIRED(removable, boolean)));

	const char* str = LSMessageGetPayload(message);
	if (!str) {
		errMsg = "No payload provided";
		goto Done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto Done;
	}

	label = json_object_object_get(root, "id");
	if( !label || is_error(label))
		goto Done;
	id = json_object_get_string(label);

	label = json_object_object_get(root, "title");
	if( !label || is_error(label))
		goto Done;
	title = json_object_get_string(label);

	label = json_object_object_get(root, "appmenu");
	if ( label && (!is_error(label)))
		menuName = json_object_get_string(label);
	else
		menuName = title;

	label = json_object_object_get(root, "icon");
	if( !label || is_error(label))
		goto Done;
	icon = json_object_get_string(label);

	label = json_object_object_get(root, "params");
	if( !label || is_error(label))
		goto Done;
	paramsStr = json_object_get_string(label);
	if (!paramsStr)
		goto Done;
	params = paramsStr;

	label = json_object_object_get(root, "removable");
	if(label && (!is_error(label)))
		removable = json_object_get_boolean(label);

	appDesc = ApplicationManager::instance()->getAppById(id);
	if (!appDesc || appDesc->isRemoveFlagged()) {
		errMsg = "Unable to find id: " + id;
		goto Done;
	}
	else if (appDesc->status() != ApplicationDescription::Status_Ready) {
		errMsg = "Application is not ready";
		goto Done;
	}

	// Get the absolute path for the icon
	dirPath = g_path_get_dirname(appDesc->entryPoint().c_str());
	icon = getAbsolutePath(icon, dirPath);
	g_free(dirPath);

	launchId = ApplicationManager::instance()->addLaunchPoint(id, title,menuName, icon, params, removable);
	success = true;

	Done:

	json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if (success) {
		if (!launchId.empty()) {
			json_object_object_add(json, "launchPointId", json_object_new_string(launchId.c_str()));
		}
		else {
			json_object_object_add(json, "errorText", json_object_new_string("Failed to save launch point"));
		}
	}
	else {
		if (errMsg.empty())
			errMsg = "Invalid arguments";
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror))
		LSErrorFree (&lserror);

	if (root && !is_error(root))
		json_object_put(root);
	json_object_put(json);

	return true;
}

/**
	removeLaunchPoint: Remove a Dynamic launchpoint
 */
static bool servicecallback_removeLaunchPoint(LSHandle* lshandle, LSMessage *message,
		void *user_data)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* json = 0;
	std::string errMsg;
	std::string launchPointId;
	json_object* root = 0;
	json_object* label = 0;
	bool success = false;

    // {"launchPointId": string}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_1(REQUIRED(launchPointId, string)));

	const char* str = LSMessageGetPayload(message);
	if (!str) {
		errMsg = "No payload provided";
		goto Done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto Done;
	}

	label = json_object_object_get(root, "launchPointId");
	if (!label || is_error(label)) {
		errMsg = "Must provide a launchPointId";
		goto Done;
	}
	launchPointId = json_object_get_string(label);

	if (ApplicationManager::instance()->removeLaunchPoint(launchPointId, errMsg))
		success = true;

	Done:

	json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if (!success)
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror)) 
		LSErrorFree (&lserror);

	if (root && !is_error(root)) 
		json_object_put(root);
	json_object_put(json);

	return true;
}

/**
	updateLaunchPointIcon: Update a launch points icon
 */
static bool servicecallback_updateLaunchPointIcon(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* json = 0;
	std::string errMsg;
	std::string launchPointId;
	std::string iconPath;
	bool success = false;

	const char* cstr = NULL;
	std::string appId = "";
	size_t index = std::string::npos;
	const LaunchPoint* lp = 0;

	struct json_object* root = 0;
	struct json_object* lpid = 0;
	struct json_object* icon = 0;

    // {"launchPointId": string, "icon": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_2(REQUIRED(launchPointId, string), REQUIRED(icon, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errMsg = "No payload provided";
		goto Done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto Done;
	}

	// parse the launchPointId
	lpid = json_object_object_get(root, "launchPointId");
	if (!lpid || is_error(lpid)) {
		errMsg = "Must provide launchPointId";
		goto Done;
	}
	launchPointId = json_object_get_string(lpid);

	// parse the new image path
	icon = json_object_object_get(root, "icon");
	if (!icon || is_error(icon)) {
		errMsg = "Must provide icon path";
		goto Done;
	}
	iconPath = json_object_get_string(icon);

	// who's trying to change this launch points icon?
	cstr = LSMessageGetApplicationID(message);
	if (cstr)
		appId = cstr;
	index = appId.find(' ');
	if (index == std::string::npos) {
		errMsg = "Unrecognized application sender";
		goto Done;
	}
	appId = appId.substr(0, index);

	// we only allow applications to change their own launch points icon (default or dynamic)
	lp = ApplicationManager::instance()->getLaunchPointById(launchPointId);
	if (!lp) {
		errMsg = "launchPointId \"" + launchPointId + "\" was not found";
		goto Done;
	}

	if (lp->id() != appId) {
		errMsg = "Attempted to change another application's launch point icon";
		goto Done;
	}

	// get absolute path for icon
	if (lp->appDesc())
		iconPath = getAbsolutePath(iconPath, lp->appDesc()->folderPath());

	// go ahead and try to update the LaunchPoint's icon/iconPath
	success = ApplicationManager::instance()->updateLaunchPointIcon(launchPointId, iconPath);
	if (!success) {
		errMsg = "Unable to update launch point's icon";
	}

	Done:

	json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if (!success)
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lserror))
		LSErrorFree(&lserror);

	if (root && !is_error(root))
		json_object_put(root);
	json_object_put(json);

	return true;
}

/**
	launchPointChanges: Subscription method to be informed when changes occur in launchPoints
 */
static bool servicecallback_launchPointChanges(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError     lsError;
	LSErrorInit(&lsError);
	std::string errMsg;
	json_object* json = 0;
	bool success = false;
	bool subscribed = false;

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_ANY);

	if (!LSMessageIsSubscription(message)) {
		errMsg = "Only supports subscriptions";
		goto Done;
	}

	success = LSSubscriptionProcess(lsHandle, message, &subscribed, &lsError);
	if (!success) {
		LSErrorFree (&lsError);
		errMsg = "Failed to process subscription";
	}

	Done:

	json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));
	if (!success)
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree (&lsError);

	json_object_put(json);

	return true;
}

static std::string getAbsolutePath(const std::string& inStr,
		const std::string& parentDirectory)
{
	int pos = inStr.find("://");
	if (pos != -1) {
		// absolute path with url protocol. strip off the protocol part
		return inStr.substr(pos + 3);
	}

	if (inStr[0] == '/') {
		// absolute path
		return inStr;
	}

	// relative path. Prepend the parent folder
	return parentDirectory + "/" + inStr;
}

static bool servicecallback_listResourceHandlers(LSHandle* lsHandle, LSMessage *message, void *userData)
{
    // {}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_ANY);
	return true;
}

static bool servicecallback_listRedirectHandlers(LSHandle* lsHandle, LSMessage *message, void *userData)
{
    // {}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_ANY);
	return true;
}

static bool servicecallback_dumpMimeTable(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError     lsError;
	LSErrorInit(&lsError);

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_ANY);

	std::string s = ApplicationManager::instance()->mimeTableAsJsonString();
	if (!LSMessageReply(lsHandle, message, s.c_str(), &lsError))
		LSErrorFree (&lsError);

	return true;
}

static bool servicecallback_addResourceHandler(LSHandle* lsHandle, LSMessage *message, void *userData)
{

	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	json_object * label = 0;
	std::string errorText;
	std::string mime;
	std::string extension;
	std::string appid;
	bool shouldDownload;

    // {"appID": string, "shouldDownload": boolean, "mimeType": string or "extension": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_4(REQUIRED(keyword, string), REQUIRED(shouldDownload, boolean),
                                        OPTIONAL(mimeType, string), OPTIONAL(extension, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_addResourceHandler;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_addResourceHandler;
	}

	if (extractFromJson(root,"appId",appid) == false) {
		errorText = "Missing appId parameter";
		goto Done_servicecallback_addResourceHandler;
	}

	label = JsonGetObject(root,"shouldDownload");
	if (!label)
		shouldDownload = false;
	else
		shouldDownload = json_object_get_boolean(label);

	if (extractFromJson(root,"mimeType",mime) == false) { 
		//mime type not provided
		if (extractFromJson(root,"extension",extension) == false) {
			//neither provided...error
			errorText = "Neither extension or mime type provided";
			goto Done_servicecallback_addResourceHandler;
		}
		else {
			//try and add it by extension
			if (MimeSystem::instance()->addResourceHandler(extension,shouldDownload,appid,NULL,false) <= 0) {
				errorText = std::string("Cannot find mime type for extension [")+extension+std::string("]");
				goto Done_servicecallback_addResourceHandler;
			}
		}
	}
	else {
		//have the mime type
		if (extractFromJson(root,"extension",extension) == false) {
			//no extension provided. Fake it (use "" for extension)
			extension = "";
			if (MimeSystem::instance()->addResourceHandler(extension,mime,shouldDownload,appid,NULL,false) <= 0) {
				errorText = "adding handler failed";
			}
		}
		else {
			//add
			if (MimeSystem::instance()->addResourceHandler(extension,mime,shouldDownload,appid,NULL,false) <= 0) {
				errorText = "adding handler failed";
			}
		}
	}

	Done_servicecallback_addResourceHandler:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		MimeSystem::instance()->saveMimeTableToActiveFile(errorText);	//ignore errors
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

static bool servicecallback_addRedirectHandler(LSHandle* lsHandle, LSMessage *message, void *userData)
{

	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	json_object * label = 0;
	std::string errorText;
	std::string urlPattern;
	std::string appid;
	bool schemeForm;

    // {"appID": string, "urlPattern": string, "schemeForm": boolean }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_3(REQUIRED(appID, string), REQUIRED(urlPattern, string), REQUIRED(schemeForm, boolean)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_addRedirectHandler;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_addRedirectHandler;
	}

	if (extractFromJson(root,"appId",appid) == false) {
		errorText = "Missing appId parameter";
		goto Done_servicecallback_addRedirectHandler;
	}

	if (extractFromJson(root,"urlPattern",urlPattern) == false) { 
		errorText = "Missing urlPattern parameter";
		goto Done_servicecallback_addRedirectHandler;
	}

	label = json_object_object_get(root,"schemeForm");
	if (!label || is_error(label)) {
		errorText = "Missing schemeForm parameter";
		goto Done_servicecallback_addRedirectHandler;
	}
	if (json_object_is_type(label,json_type_boolean) == false) {
		errorText = "schemeForm parameter incorrectly specified (should be a boolean value)";
		goto Done_servicecallback_addRedirectHandler;
	}
	schemeForm = json_object_get_boolean(label);

	if (MimeSystem::instance()->addRedirectHandler(urlPattern,appid,NULL,schemeForm,false) == 0) {
		errorText = "Couldn't add handler";
		goto Done_servicecallback_addRedirectHandler;
	}

	Done_servicecallback_addRedirectHandler:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		MimeSystem::instance()->saveMimeTableToActiveFile(errorText);	//ignore errors
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

static bool servicecallback_getMimeTypeForExtension(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string mime;
	std::string extension;

    // {"extension": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(extension, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_getMimeTypeForExtension;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_getMimeTypeForExtension;
	}

	if (extractFromJson(root,"extension",extension) == false) {
		errorText = "Missing extension parameter";
		goto Done_servicecallback_getMimeTypeForExtension;
	}

	if (MimeSystem::instance()->getMimeTypeByExtension(extension,mime) == false) {
		errorText = "No mime mapped to this extension";
		goto Done_servicecallback_getMimeTypeForExtension;
	}

	Done_servicecallback_getMimeTypeForExtension:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		json_object_object_add(reply, "mimeType", json_object_new_string(mime.c_str()));
		json_object_object_add(reply, "extension", json_object_new_string(extension.c_str()));
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

static bool servicecallback_getHandlerForMimeType(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string mime;
	ResourceHandler rsrcHandler;

    // {"mimeType": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(mimeType, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_getHandlerForMimeType;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_getHandlerForMimeType;
	}

	if (extractFromJson(root,"mimeType",mime) == false) {
		errorText = "Missing mimeType parameter";
		goto Done_servicecallback_getHandlerForMimeType;
	}

	rsrcHandler = MimeSystem::instance()->getActiveHandlerForResource(mime);

	if (rsrcHandler.valid() == false) {
		errorText = "No handler mapped to this mimeType";
		goto Done_servicecallback_getHandlerForMimeType;
	}

	Done_servicecallback_getHandlerForMimeType:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		json_object_object_add(reply, "mimeType", json_object_new_string(mime.c_str()));
		json_object_object_add(reply, "appId",json_object_new_string(rsrcHandler.appId().c_str()));
		json_object_object_add(reply, "download", json_object_new_boolean(!(rsrcHandler.stream())));
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;

}

static bool servicecallback_getHandlerForExtension(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string extension;
	std::string mime;
	ResourceHandler rsrcHandler;

    // {"extension": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(extension, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_getHandlerForExtension;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_getHandlerForExtension;
	}

	if (extractFromJson(root,"extension",extension) == false) {
		errorText = "Missing extension parameter";
		goto Done_servicecallback_getHandlerForExtension;
	}

	//map mime to extension
	if (MimeSystem::instance()->getMimeTypeByExtension(extension,mime) == false) {
		errorText = std::string("No mime type mapped to extension ")+extension;
		goto Done_servicecallback_getHandlerForExtension;
	}

	rsrcHandler = MimeSystem::instance()->getActiveHandlerForResource(mime);

	if (rsrcHandler.valid() == false) {
		errorText = "No handler mapped to this mimeType";
		goto Done_servicecallback_getHandlerForExtension;
	}

	Done_servicecallback_getHandlerForExtension:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		json_object_object_add(reply, "mimeType", json_object_new_string(mime.c_str()));
		json_object_object_add(reply, "appId",json_object_new_string(rsrcHandler.appId().c_str()));
		json_object_object_add(reply, "download", json_object_new_boolean(!(rsrcHandler.stream())));
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;

}

static bool servicecallback_getHandlerForUrl(LSHandle* lsHandle, LSMessage *message, void *userData)
{

	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string url;
	std::string mime;
	std::string extension;
	std::string::size_type extPos;
	QUrl qurl;

	ResourceHandler rsrcHandler;
	RedirectHandler redirHandler;

    // {"url": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(url, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_getHandlerForUrl;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_getHandlerForUrl;
	}

	if (extractFromJson(root,"url",url) == false) {
		errorText = "Missing url parameter";
		goto Done_servicecallback_getHandlerForUrl;
	}

	qurl.setUrl(url.c_str());
	if (!qurl.isValid()) {
		errorText = "Invalid URI format";
		goto Done_servicecallback_getHandlerForUrl;
	}

	//try and find a redirect handler for it...disallow scheme forms of url matching (i.e. matching only on http: , https:, etc)
	redirHandler = MimeSystem::instance()->getActiveHandlerForRedirect(url,false,true);
	if (redirHandler.valid()) {
		//this is what I'll answer with
		goto Done_servicecallback_getHandlerForUrl;
	}

	//try and extract the extension
	extension = getResourceNameFromUrl(qurl);
	if ((extPos = extension.rfind('.')) != std::string::npos) {
		extension = extension.substr(extPos+1);
		//map mime to extension
		if (MimeSystem::instance()->getMimeTypeByExtension(extension,mime)) {
			rsrcHandler = MimeSystem::instance()->getActiveHandlerForResource(mime);
			//answer with this
			goto Done_servicecallback_getHandlerForUrl;
		}
	}

	//finally, try to find a scheme form of a redirect handler
	redirHandler = MimeSystem::instance()->getActiveHandlerForRedirect(url,false,false);

	if (redirHandler.valid() == false) 
		errorText = std::string("No handler found for url [")+url+std::string("]");

	Done_servicecallback_getHandlerForUrl:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		if (rsrcHandler.valid()) {
			json_object_object_add(reply, "mimeType", json_object_new_string(mime.c_str()));
			json_object_object_add(reply, "appId",json_object_new_string(rsrcHandler.appId().c_str()));
			json_object_object_add(reply, "download", json_object_new_boolean(!(rsrcHandler.stream())));
		}
		else {
			json_object_object_add(reply, "appId",json_object_new_string(redirHandler.appId().c_str()));
			json_object_object_add(reply, "download", json_object_new_boolean(false));
		}
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;

}

static bool servicecallback_getHandlerByVerb(LSHandle* lsHandle, LSMessage *message, void *userData)
{

	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string url;
	std::string mime;
	std::string verb;
	std::string handlerParams;	
	ResourceHandler rsrcHandler;
	RedirectHandler redirHandler;	

    // {"url": string or "mime": string, "verb": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_3(OPTIONAL(url, string), OPTIONAL(mime, string), REQUIRED(verb, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_getHandlerByVerb;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_getHandlerByVerb;
	}

	if ((extractFromJson(root,"url",url) == false) && (extractFromJson(root,"mime",mime) == false)) {
		errorText = "Must have either an url or a mime parameter";
		goto Done_servicecallback_getHandlerByVerb;
	}

	if (extractFromJson(root,"verb",verb) == false) {
		errorText = "Missing verb parameter";
		goto Done_servicecallback_getHandlerByVerb;
	}

	if (!url.empty()) {
		//validate it
		if (!QUrl(url.c_str()).isValid()) {
			errorText = "Invalid URI format";
			goto Done_servicecallback_getHandlerByVerb;
		}
		//try the redirect forms first
		redirHandler = MimeSystem::instance()->getHandlerByVerbForRedirect(url,true,verb);
		if (redirHandler.valid() == false) {
			//try the scheme forms
			redirHandler = MimeSystem::instance()->getHandlerByVerbForRedirect(url,false,verb);
			if (redirHandler.valid() == false) {
				//none found
				errorText = std::string("No redirect handler found for ")+url;
				goto Done_servicecallback_getHandlerByVerb;
			}
		}
	}
	else {
		//must be mime
		rsrcHandler = MimeSystem::instance()->getHandlerByVerbForResource(mime,verb);
		if (rsrcHandler.valid() == false) {
			errorText = std::string("No resource handler found for ")+mime;
		}
	}

	Done_servicecallback_getHandlerByVerb:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		if (rsrcHandler.valid()) {
			json_object_object_add(reply, "mimeType", json_object_new_string(mime.c_str()));
			json_object_object_add(reply, "appId",json_object_new_string(rsrcHandler.appId().c_str()));
			json_object_object_add(reply, "download", json_object_new_boolean(!(rsrcHandler.stream())));
			json_object_object_add(reply, "index", json_object_new_int(rsrcHandler.index()));
		}
		else {
			json_object_object_add(reply, "appId",json_object_new_string(redirHandler.appId().c_str()));
			json_object_object_add(reply, "download", json_object_new_boolean(false));
			json_object_object_add(reply, "index", json_object_new_int(redirHandler.index()));
		}
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;

}

static bool servicecallback_listAllHandlers(LSHandle* lsHandle, LSMessage *message, void *userData)
{

	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string url;
	std::string mime;	
	RedirectHandler activeRedirHandler;
	ResourceHandler activeRsrcHandler;
	std::vector<RedirectHandler> redirHandlers;
	std::vector<ResourceHandler> rsrcHandlers;

    // {"url": string or "mime": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_2(OPTIONAL(url, string), OPTIONAL(mime, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_listAllHandlers;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_listAllHandlers;
	}

	if ((extractFromJson(root,"url",url) == false) && (extractFromJson(root,"mime",mime) == false)) {
		errorText = "Must have either an url or a mime parameter";
		goto Done_servicecallback_listAllHandlers;
	}

	if (!url.empty()) {
		//validate it
		if (!QUrl(url.c_str()).isValid()) {
			errorText = "Invalid URI format";
			goto Done_servicecallback_listAllHandlers;
		}
		if (MimeSystem::instance()->getAllHandlersForRedirect(url,false,activeRedirHandler,redirHandlers) == 0) {
			errorText = std::string("No handlers found for ")+url;
			goto Done_servicecallback_listAllHandlers;
		}
		else if (activeRedirHandler.valid() == false) {
			errorText = std::string("Error getting handlers for ")+url;
			goto Done_servicecallback_listAllHandlers;
		}
	}
	else {
		//must be mime
		if (MimeSystem::instance()->getAllHandlersForResource(mime,activeRsrcHandler,rsrcHandlers) == 0) {
			errorText = std::string("No handlers found for ")+mime;
			goto Done_servicecallback_listAllHandlers;
		}
		else if (activeRsrcHandler.valid() == false) {
			errorText = std::string("Error getting handlers for ")+mime;
			goto Done_servicecallback_listAllHandlers;
		}
	}

	Done_servicecallback_listAllHandlers:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (url.size() > 0) {
		json_object_object_add(reply,"url",json_object_new_string(url.c_str()));
	}
	if (mime.size() > 0) {
		json_object_object_add(reply,"mime",json_object_new_string(mime.c_str()));
	}
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object * jinnerobj = json_object_new_object();
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		if (activeRsrcHandler.valid()) 
		{
			json_object * handlerJobj = activeRsrcHandler.toJson();
			ApplicationDescription * appDesc = ApplicationManager::instance()->getAppById(activeRsrcHandler.appId());
			if (appDesc != NULL) {
				json_object_object_add(handlerJobj,"appName",json_object_new_string(appDesc->menuName().c_str()));
			}
			json_object_object_add(jinnerobj,"activeHandler",handlerJobj);
			if (rsrcHandlers.size() > 0) {
				json_object * jarray = json_object_new_array();
				for (std::vector<ResourceHandler>::iterator it = rsrcHandlers.begin();it != rsrcHandlers.end();++it) {
					handlerJobj = (*it).toJson();
					appDesc = ApplicationManager::instance()->getAppById((*it).appId());
					if (appDesc != NULL) {
						json_object_object_add(handlerJobj,"appName",json_object_new_string(appDesc->menuName().c_str()));
					}
					json_object_array_add(jarray,handlerJobj);
				}
				json_object_object_add(jinnerobj,"alternates",jarray);
			}
			json_object_object_add(reply,"resourceHandlers",jinnerobj);
		}
		else 
		{		//redirect handlers were found

			json_object * handlerJobj = activeRedirHandler.toJson();
			ApplicationDescription * appDesc = ApplicationManager::instance()->getAppById(activeRedirHandler.appId());
			if (appDesc != NULL) {
				json_object_object_add(handlerJobj,"appName",json_object_new_string(appDesc->menuName().c_str()));
			}
			json_object_object_add(jinnerobj,"activeHandler",handlerJobj);
			if (redirHandlers.size() > 0) {
				json_object * jarray = json_object_new_array();
				for (std::vector<RedirectHandler>::iterator it = redirHandlers.begin();it != redirHandlers.end();++it) {
					handlerJobj = (*it).toJson();
					appDesc = ApplicationManager::instance()->getAppById((*it).appId());
					if (appDesc != NULL) {
						json_object_object_add(handlerJobj,"appName",json_object_new_string(appDesc->menuName().c_str()));
					}
					json_object_array_add(jarray,handlerJobj);
				}
				json_object_object_add(jinnerobj,"alternates",jarray);
			}
			json_object_object_add(reply,"redirectHandlers",jinnerobj);
		}
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;

}


static bool servicecallback_listAllHandlersMultipleMime(LSHandle* lsHandle, LSMessage *message, void *userData)
{

	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	json_object* label = 0;
	array_list* mimeArray = 0;
	std::string errorText;
	std::map<std::string,ResourceHandler> activeHandlersMap;
	std::map<std::string,std::vector<ResourceHandler> > altHandlersMap;

    // {"mimes": array}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(mimes, array)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_listAllHandlersMultipleMime;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_listAllHandlersMultipleMime;
	}

	//extract the mimes array:
	//  "mimes":["mime1","mime2",...]

	if ((label = JsonGetObject(root,"mimes")) == NULL) {
		errorText = "Missing 'mimes' array parameter";
		goto Done_servicecallback_listAllHandlersMultipleMime;
	}

	mimeArray = json_object_get_array(label);
	if (!mimeArray || is_error(mimeArray)) {
		errorText = "Bad 'mimes' array parameter specified";
		goto Done_servicecallback_listAllHandlersMultipleMime;
	}

	if (array_list_length(mimeArray) <= 0) {
		errorText = "Bad 'mimes' array parameter specified";
		goto Done_servicecallback_listAllHandlersMultipleMime;
	}

	for (int i = 0; i < array_list_length(mimeArray); i++) {
		json_object* obj = (json_object*) array_list_get_idx(mimeArray, i);
		std::string mime = json_object_get_string(obj);
		if (mime.length()) {
			ResourceHandler active;
			std::vector<ResourceHandler> alts;
			if (MimeSystem::instance()->getAllHandlersForResource(mime,active,alts) != 0) {
				activeHandlersMap[mime] = active;
				altHandlersMap[mime] = alts;
			}
		}
	}

	Done_servicecallback_listAllHandlersMultipleMime:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		json_object * handlerJobj;
		ApplicationDescription * appDesc;	
		for (std::map<std::string,ResourceHandler>::iterator active_map_it = activeHandlersMap.begin();
		active_map_it != activeHandlersMap.end();++active_map_it) 
		{
			json_object * jinnerobj = json_object_new_object();
			handlerJobj = active_map_it->second.toJson();
			appDesc = ApplicationManager::instance()->getAppById(active_map_it->second.appId());
			if (appDesc != NULL) {
				json_object_object_add(handlerJobj,"appName",json_object_new_string(appDesc->menuName().c_str()));
			}
			json_object_object_add(jinnerobj,"activeHandler",handlerJobj);
			std::map<std::string,std::vector<ResourceHandler> >::iterator alt_map_it = altHandlersMap.find(active_map_it->first);
			if (alt_map_it != altHandlersMap.end()) {
				if (alt_map_it->second.size() > 0) {
					json_object * jarray = json_object_new_array();
					for (std::vector<ResourceHandler>::iterator it = alt_map_it->second.begin();it != alt_map_it->second.end();++it) 
					{
						handlerJobj = (*it).toJson();
						appDesc = ApplicationManager::instance()->getAppById((*it).appId());
						if (appDesc != NULL) {
							json_object_object_add(handlerJobj,"appName",json_object_new_string(appDesc->menuName().c_str()));
						}
						json_object_array_add(jarray,handlerJobj);
					}
					json_object_object_add(jinnerobj,(char *)"alternates",jarray);
				}
				json_object_object_add(reply,(char *)(active_map_it->first.c_str()),jinnerobj);
			}
			else {
				json_object_put(jinnerobj);
			}
		} //end activeHandlersMap for loop
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;

}

static bool servicecallback_listAllHandlersMultipleUrlPattern(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	json_object* label = 0;
	array_list* urlArray = 0;
	std::string errorText;	
	std::map<std::string,RedirectHandler> activeHandlersMap;
	std::map<std::string,std::vector<RedirectHandler> > altHandlersMap;

    // {"urls": array}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(urls, array)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_listAllHandlersMultipleMime;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_listAllHandlersMultipleMime;
	}

	//extract the urls array:
	//  "urls":["url pattern 1","url pattern 2",...]

	if ((label = JsonGetObject(root,"urls")) == NULL) {
		errorText = "Missing 'urls' array parameter";
		goto Done_servicecallback_listAllHandlersMultipleMime;
	}

	urlArray = json_object_get_array(label);
	if (!urlArray || is_error(urlArray)) {
		errorText = "Bad 'urls' array parameter specified";
		goto Done_servicecallback_listAllHandlersMultipleMime;
	}

	if (array_list_length(urlArray) <= 0) {
		errorText = "Bad 'urls' array parameter specified";
		goto Done_servicecallback_listAllHandlersMultipleMime;
	}

	for (int i = 0; i < array_list_length(urlArray); i++) {
		json_object* obj = (json_object*) array_list_get_idx(urlArray, i);
		std::string url = json_object_get_string(obj);
		if (url.length()) {
			RedirectHandler active;
			std::vector<RedirectHandler> alts;
			if (MimeSystem::instance()->getAllHandlersForRedirect(url,true,active,alts) != 0) {
				activeHandlersMap[url] = active;
				altHandlersMap[url] = alts;
			}
		}
	}

	Done_servicecallback_listAllHandlersMultipleMime:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		json_object * handlerJobj;
		ApplicationDescription * appDesc;	
		for (std::map<std::string,RedirectHandler>::iterator active_map_it = activeHandlersMap.begin();
		active_map_it != activeHandlersMap.end();++active_map_it) 
		{
			json_object * jinnerobj = json_object_new_object();
			handlerJobj = active_map_it->second.toJson();
			appDesc = ApplicationManager::instance()->getAppById(active_map_it->second.appId());
			if (appDesc != NULL) {
				json_object_object_add(handlerJobj,"appName",json_object_new_string(appDesc->menuName().c_str()));
			}
			json_object_object_add(jinnerobj,"activeHandler",handlerJobj);
			std::map<std::string,std::vector<RedirectHandler> >::iterator alt_map_it = altHandlersMap.find(active_map_it->first);
			if (alt_map_it != altHandlersMap.end()) {
				if (alt_map_it->second.size() > 0) {
					json_object * jarray = json_object_new_array();
					for (std::vector<RedirectHandler>::iterator it = alt_map_it->second.begin();it != alt_map_it->second.end();++it) 
					{
						handlerJobj = (*it).toJson();
						appDesc = ApplicationManager::instance()->getAppById((*it).appId());
						if (appDesc != NULL) {
							json_object_object_add(handlerJobj,"appName",json_object_new_string(appDesc->menuName().c_str()));
						}
						json_object_array_add(jarray,handlerJobj);
					}
					json_object_object_add(jinnerobj,(char *)"alternates",jarray);
				}
				json_object_object_add(reply,(char *)(active_map_it->first.c_str()),jinnerobj);
			}
			else {
				json_object_put(jinnerobj);
			}
		} //end activeHandlersMap for loop
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

static bool servicecallback_listAllUrlHandlersByPattern(LSHandle* lsHandle, LSMessage *message, void *userData)
{

	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string url;	
	RedirectHandler activeRedirHandler;
	ResourceHandler activeRsrcHandler;
	std::vector<RedirectHandler> redirHandlers;
	std::vector<ResourceHandler> rsrcHandlers;

    // {"urlPattern": url}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(urlPattern, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_listAllUrlHandlersByPattern;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_listAllUrlHandlersByPattern;
	}

	if (extractFromJson(root,"urlPattern",url) == false) {
		errorText = "Missing urlPattern parameter";
		goto Done_servicecallback_listAllUrlHandlersByPattern;
	}


	if (MimeSystem::instance()->getAllHandlersForRedirect(url,true,activeRedirHandler,redirHandlers) == 0) {
		errorText = std::string("No handlers found for ")+url;
		goto Done_servicecallback_listAllUrlHandlersByPattern;
	}
	else if (activeRedirHandler.valid() == false) {
		errorText = std::string("Error getting handlers for ")+url;
		goto Done_servicecallback_listAllUrlHandlersByPattern;
	}


	Done_servicecallback_listAllUrlHandlersByPattern:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (url.size() > 0) {
		json_object_object_add(reply,"urlPattern",json_object_new_string(url.c_str()));
	}
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object * jinnerobj = json_object_new_object();
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		if (activeRsrcHandler.valid()) {
			json_object_object_add(jinnerobj,"activeHandler",activeRsrcHandler.toJson());
			if (rsrcHandlers.size() > 0) {
				json_object * jarray = json_object_new_array();
				for (std::vector<ResourceHandler>::iterator it = rsrcHandlers.begin();it != rsrcHandlers.end();++it)
					json_object_array_add(jarray,(*it).toJson());
				json_object_object_add(jinnerobj,(char *)"alternates",jarray);
			}
			json_object_object_add(reply,"resourceHandlers",jinnerobj);
		}
		else {		//redirect handlers were found
			json_object_object_add(jinnerobj,"activeHandler",activeRedirHandler.toJson());
			if (redirHandlers.size() > 0) {
				json_object * jarray = json_object_new_array();
				for (std::vector<RedirectHandler>::iterator it = redirHandlers.begin();it != redirHandlers.end();++it)
					json_object_array_add(jarray,(*it).toJson());
				json_object_object_add(jinnerobj,(char *)"alternates",jarray);
			}
			json_object_object_add(reply,"redirectHandlers",jinnerobj);
		}
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;

}

static bool servicecallback_listAllHandlersByVerb(LSHandle* lsHandle, LSMessage *message, void *userData)
{

	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string url;
	std::string mime;	
	std::string verb;
	std::vector<RedirectHandler> redirHandlers;
	std::vector<ResourceHandler> rsrcHandlers;

    // {"verb": string, "url": string or "mime": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_3(REQUIRED(verb, string), OPTIONAL(url, string), OPTIONAL(mime, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_listAllHandlersByVerb;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_listAllHandlersByVerb;
	}

	if (extractFromJson(root,"verb",verb) == false) {
		errorText = "Missing verb parameter";
		goto Done_servicecallback_listAllHandlersByVerb;
	}

	if ((extractFromJson(root,"url",url) == false) && (extractFromJson(root,"mime",mime) == false)) {
		errorText = "Must have either an url or a mime parameter";
		goto Done_servicecallback_listAllHandlersByVerb;
	}

	if (!url.empty()) {
		//validate it
		if (!QUrl(url.c_str()).isValid()) {
			errorText = "Invalid URI format";
			goto Done_servicecallback_listAllHandlersByVerb;
		}
		if (MimeSystem::instance()->getAllHandlersByVerbForRedirect(url,verb,redirHandlers) == 0) {
			errorText = std::string("No handlers found for ")+url;
			goto Done_servicecallback_listAllHandlersByVerb;
		}
	}
	else {
		//must be mime
		if (MimeSystem::instance()->getAllHandlersByVerbForResource(mime,verb,rsrcHandlers) == 0) {
			errorText = std::string("No handlers found for ")+mime;
			goto Done_servicecallback_listAllHandlersByVerb;
		}
	}

	Done_servicecallback_listAllHandlersByVerb:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (url.size() > 0) {
		json_object_object_add(reply,"url",json_object_new_string(url.c_str()));
	}
	if (mime.size() > 0) {
		json_object_object_add(reply,"mime",json_object_new_string(mime.c_str()));
	}
	if (verb.size() > 0) {
		json_object_object_add(reply,"verb",json_object_new_string(verb.c_str()));
	}

	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		json_object * jinnerobj = json_object_new_object();
		if (rsrcHandlers.size() > 0) {		///mime handlers
			json_object * jarray = json_object_new_array();
			for (std::vector<ResourceHandler>::iterator it = rsrcHandlers.begin();it != rsrcHandlers.end();++it)
				json_object_array_add(jarray,(*it).toJson());
			json_object_object_add(jinnerobj,(char *)"alternates",jarray);
			json_object_object_add(reply,"resourceHandlers",jinnerobj);
		}
		else {	///redirect handlers
			json_object * jarray = json_object_new_array();
			for (std::vector<RedirectHandler>::iterator it = redirHandlers.begin();it != redirHandlers.end();++it)
				json_object_array_add(jarray,(*it).toJson());
			json_object_object_add(jinnerobj,(char *)"alternates",jarray);
			json_object_object_add(reply,"redirectHandlers",jinnerobj);
		}
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;

}


static bool servicecallback_listExtensionMap(LSHandle* lsHandle, LSMessage *message, void *userData) 
{
	
	LSError lserror;
	LSErrorInit(&lserror);

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_ANY);
		
	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
	json_object_object_add(reply, "extensionMap",MimeSystem::instance()->extensionMapAsJsonArray());

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
		
}

static bool servicecallback_swapResourceHandler(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	json_object * label = 0;
	std::string errorText;
	std::string mime;
	uint32_t index;

    // {"mimeType": string, "index": integer }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_2(REQUIRED(mimeType, string), REQUIRED(index, integer)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_swapResourceHandler;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_swapResourceHandler;
	}

	if (extractFromJson(root,"mimeType",mime) == false) {
		errorText = "Missing mimeType parameter";
		goto Done_servicecallback_swapResourceHandler;
	}

	label = JsonGetObject(root,"index");
	if (!label) {
		errorText = "Missing index parameter";
		goto Done_servicecallback_swapResourceHandler;
	}
	index = json_object_get_int(label);

	//attempt a swap 
	if (MimeSystem::instance()->swapResourceHandler(mime,index) <= 0) {
		errorText = "swap failed (incorrect index for mime type, perhaps?)";
	}

	Done_servicecallback_swapResourceHandler:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		MimeSystem::instance()->saveMimeTableToActiveFile(errorText);	//ignore errors
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

static bool servicecallback_swapRedirectHandler(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	json_object * label = 0;
	std::string errorText;
	std::string url;
	uint32_t index;

    // {"url": string, "index": int }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_2(REQUIRED(url, string), REQUIRED(index, integer)));


	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_swapRedirectHandler;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_swapRedirectHandler;
	}

	if (extractFromJson(root,"url",url) == false) {
		errorText = "Missing url parameter";
		goto Done_servicecallback_swapRedirectHandler;
	}

	label = JsonGetObject(root,"index");
	if (!label) {
		errorText = "Missing index parameter";
		goto Done_servicecallback_swapRedirectHandler;
	}
	index = json_object_get_int(label);

	//attempt a swap 
	if (MimeSystem::instance()->swapRedirectHandler(url,index) <= 0) {
		errorText = "swap failed (incorrect index for url, perhaps?)";
	}

	Done_servicecallback_swapRedirectHandler:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		MimeSystem::instance()->saveMimeTableToActiveFile(errorText);	//ignore errors
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

static bool servicecallback_registerVerbs(LSHandle* lsHandle, LSMessage *message, void *userData)
{

	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string appid;
	std::string mime;
	std::string url;
	std::map<std::string,std::string> verbs;

    // {"appId": string, "mime": string or "url": string }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_3(REQUIRED(appId, string), OPTIONAL(mime, string), OPTIONAL(url, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_registerVerbs;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_registerVerbs;
	}

	if (extractFromJson(root,"appId",appid) == false) {
		errorText = "Missing appId parameter";
		goto Done_servicecallback_registerVerbs;
	}

	extractFromJson(root,"mime",mime);
	extractFromJson(root,"url",url);

	if ((url.empty()) && (mime.empty())) {
		errorText = "Missing either the mime or the url parameter";
		goto Done_servicecallback_registerVerbs;
	}

	//grab the verbs
	//...even though this isn't a "handler entry" per-se, the verbs parameter is formatted the same way, so the MimeSystem::extractVerbsFromHandlerEntryJson() function doesn't care

	if (MimeSystem::extractVerbsFromHandlerEntryJson(root,verbs) == 0) {
		errorText = "No verbs specified/invalid verb list format";
		goto Done_servicecallback_registerVerbs;
	}

	if (mime.empty() == false) {
		if (MimeSystem::instance()->addVerbsToResourceHandler(mime,appid,verbs) == 0) {
			errorText = std::string("Adding verbs to APPID = ")+appid+ std::string(" for MIMETYPE = ")+mime+std::string(" failed");
			goto Done_servicecallback_registerVerbs;
		}
	}
	else {
		if (MimeSystem::instance()->addVerbsToRedirectHandler(url,appid,verbs) == 0) {
			errorText = std::string("Adding verbs to APPID = ")+appid+ std::string(" for URL = ")+url+std::string(" failed");
			goto Done_servicecallback_registerVerbs;
		}
	}

	Done_servicecallback_registerVerbs:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		MimeSystem::instance()->saveMimeTableToActiveFile(errorText);	//ignore errors
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

static bool servicecallback_removeHandlersForAppId(LSHandle* lsHandle, LSMessage *message, void *userData)
{

	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string appid;

    // {"appId": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(appId, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_removeHandlersForAppId;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_removeHandlersForAppId;
	}

	if (extractFromJson(root,"appId",appid) == false) {
		errorText = "Missing appId parameter";
		goto Done_servicecallback_removeHandlersForAppId;
	}

	if (MimeSystem::instance()->removeAllForAppId(appid) <= 0) {
		errorText = "Removal of handlers by appId failed";
	}

	Done_servicecallback_removeHandlersForAppId:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		//save the table to active file
		MimeSystem::instance()->saveMimeTableToActiveFile(errorText);	//ignore errors
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

static bool servicecallback_saveMimeTable(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string file;
	std::string saveErr;
	bool rc;

    // {"file": string }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(file, string)));
	
	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_saveMimeTable;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_saveMimeTable;
	}

	if (extractFromJson(root,"file",file))
		rc = MimeSystem::instance()->saveMimeTable(file,saveErr);
	else
		rc = MimeSystem::instance()->saveMimeTableToActiveFile(saveErr);
	
	if (rc == false) {
		errorText = "Saving mime table failed - "+saveErr;
		goto Done_servicecallback_saveMimeTable;
	}

	Done_servicecallback_saveMimeTable:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

static bool servicecallback_restoreMimeTable(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string file;
	std::string restoreErr;

    // {"file": string }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(file, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_restoreMimeTable;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_restoreMimeTable;
	}

	if (extractFromJson(root,"file",file) == false) {
		errorText = "Missing file parameter";
		goto Done_servicecallback_restoreMimeTable;
	}

	if (MimeSystem::instance()->restoreMimeTable(file,restoreErr) == false) {
		errorText = "Restoring mime table failed - "+restoreErr;
		goto Done_servicecallback_restoreMimeTable;
	}

	Done_servicecallback_restoreMimeTable:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}


static bool servicecallback_clearMimeTable(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	std::string errorText;

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_ANY);

	if (MimeSystem::instance()->clearMimeTable() == false)
		errorText = "clearing mime tables failed";

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;

}

static bool servicecallback_deleteSavedTable(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	std::string errorText;

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_ANY);

	MimeSystem::instance()->deleteSavedMimeTable();

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	json_object_object_add(reply, "returnValue", json_object_new_boolean(true));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;

}

static bool servicecallback_forceSingleAppScan(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);

	json_object* root = 0;
	json_object* label = 0;
	bool success=false;
	std::string resultStr;
	std::string appId;


    // {"id": string }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(id, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str)
	{
		return false;
	}
	root = json_tokener_parse( str );
	if (root && !is_error(root))
	{
		if ( (label = JsonGetObject(root,"id")) )
		{
			const char * cstr = json_object_get_string(label);
			appId = std::string((cstr ? cstr : ""));
			ApplicationManager::instance()->postInstallScan(appId);
			success = true;
		}
		else
		{
			resultStr = std::string("no key 'id' provided");
		}
	}
	else
	{
		resultStr = std::string("invalid json in message");
	}

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	json_object_object_add(reply, "returnValue", json_object_new_boolean(success));
	if (!success)
	{
		json_object_object_add(reply,"errorText",json_object_new_string(resultStr.c_str()));
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

// Remove the application manager from the service bus.
void ApplicationManager::stopService()
{
	LSError lserror;
	LSErrorInit(&lserror);
	bool result;

	result = LSUnregisterPalmService(m_service, &lserror);
	if (!result)
		LSErrorFree(&lserror);
}

void ApplicationManager::postLaunchPointChange(const LaunchPoint* lp, const std::string& change)
{

	//LAUNCHER3-ADDED: (function mods)

	LSError lsError;
	LSErrorInit(&lsError);
	json_object* json = 0;


	if (change == "removed") {
		Q_EMIT signalLaunchPointRemoved(lp);
	}
	else if (change == "updated") {
		QBitArray statusBits = QBitArray(LaunchPointUpdatedReason::SIZEOF);
		statusBits.setBit(LaunchPointUpdatedReason::Status);
		statusBits.setBit(LaunchPointUpdatedReason::Icon);
		Q_EMIT signalLaunchPointUpdated(lp,statusBits);
	}
	else if (change == "added") {
		//temp flag - see (Launcher3) AppMonitor::slotLaunchPointAdded
		QBitArray statusBits = QBitArray(LaunchPointAddedReason::SIZEOF);
		statusBits.setBit(LaunchPointAddedReason::PostLaunchPointUpdateAdded);
		Q_EMIT signalLaunchPointAdded(lp,statusBits);
	}

	json = lp->toJSON();
	json_object_object_add(json, "change", json_object_new_string(change.c_str()));
	g_message("%s: Posting LaunchPoint change %s", __PRETTY_FUNCTION__, json_object_to_json_string(json));
	if (!LSSubscriptionPost(m_serviceHandlePrivate, "/", "launchPointChanges", 
			json_object_to_json_string(json), &lsError))
		LSErrorFree (&lsError);

	if (json && !is_error(json))
		json_object_put(json);
}

/*!
 *	\fn com.palm.applicationManager/running
 *	\brief List all running applications in the system manager.
 *
 *	Return a list of running applications. The result will look like:
 *	{
 *		running: [
 *			{
 *				id: "..."
 *				processId: "..."
 *			}
 *			...
 *		]	
 *	}
 * 
 * 
 */

/*
 * This is used for allowing the other 2 services involved in app installation (download manager and app installer) to reply on a message that subscribed to applicationManager/open
 * 
 * Also, it's used for anyone that wants to track the downloading of an object before it is opened (also by subscribing to applicationManager/open)
 * 
 */
void ApplicationManager::relayStatus (const std::string& jsonPayload,const unsigned long ticketId)
{
	if (!m_service) {
		g_warning ("No service handle, cannot relay status");
		return;
	}

	LSError lserror;
	LSErrorInit(&lserror);
	//create the key for the subscription reply (this has to be in agreement with the key created in applicationManager/open )
	std::string ls_sub_key = toSTLString<unsigned long>(ticketId);

	bool retVal = LSSubscriptionRespond (m_service, ls_sub_key.c_str(), jsonPayload.c_str(), &lserror);
	if (!retVal) {
		LSErrorPrint (&lserror, stderr);
		LSErrorFree(&lserror);
		return;
	}

}

/**
	\fn com.palm.applicationManager/getAppBasePath
	\brief return the path to the app for a given appid
 */
static bool servicecallback_getappbasepath( LSHandle* lshandle, LSMessage *message,
		void *user_data)
{

	LSError lserror;
	LSErrorInit(&lserror);
	std::string errMsg;
	struct json_object* json=0;
	struct json_object* root=0;
	struct json_object* label=0;
	std::string appId;
	ApplicationDescription * appDesc=NULL;
	LaunchPoint * defaultLp=NULL;
	bool success=false;

    // {"appID": string}

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_1(REQUIRED(appID, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errMsg = "No payload provided";
		goto done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto done;
	}

	label = json_object_object_get(root,"appId");
	if (!label || is_error(label)) {
		errMsg = "Provide an appId";
		goto done;
	}

	appId = json_object_get_string(label);
	appDesc = ApplicationManager::instance()->getAppById(appId);
	if (appDesc == NULL) {
		errMsg = "Invalid appId specified: " + appId;
		goto done;
	}

	defaultLp = const_cast<LaunchPoint*>(appDesc->getDefaultLaunchPoint());		//to get around the goto restrictions
	if (defaultLp == NULL) {
		errMsg = "application id: " + appId + " has no launch point";
		goto done;
	}

	success=true;

	done:

	json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if (success) {
		json_object_object_add(json, "appId", json_object_new_string(appId.c_str()));
		json_object_object_add(json, "basePath", json_object_new_string(defaultLp->appDesc()->entryPoint().c_str()));
	}
	else {
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));
	}

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror))
		LSErrorFree (&lserror);

	if (root && !is_error(root))
		json_object_put(root);
	json_object_put(json);
	return true;
}


////////////////////////////// ----- DEBUG SECTION ----- ///////////////////////////////////////////////////////////////



static bool servicecallback_dbg_getAppEntryPoint(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError lserror;
	LSErrorInit(&lserror);
	json_object* root = 0;
	std::string errorText;
	std::string basedir;
	std::string entrypoint;

    // {"basedir": string }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(basedir, string)));

	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done_servicecallback_dbg_getAppEntryPoint;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done_servicecallback_dbg_getAppEntryPoint;
	}

	if (extractFromJson(root,"basedir",basedir) == false) {
		errorText = "Missing basedir parameter";
		goto Done_servicecallback_dbg_getAppEntryPoint;
	}

	if (ApplicationManager::getAppEntryPointFromAppinfoFile(basedir,entrypoint) == false) {
		errorText = "getAppEntryPointFromAppinfoFile failed";
		goto Done_servicecallback_dbg_getAppEntryPoint;
	}

	Done_servicecallback_dbg_getAppEntryPoint:

	if (root)
		json_object_put(root);

	json_object * reply = json_object_new_object();
	json_object_object_add(reply, "subscribed", json_object_new_boolean(false));
	if (errorText.size() > 0) {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(false));
		json_object_object_add(reply, "errorCode", json_object_new_string(errorText.c_str()));
	}
	else {
		json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
		json_object_object_add(reply, "entryPoint",json_object_new_string(entrypoint.c_str()));
		
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lserror))
		LSErrorFree (&lserror);

	json_object_put(reply);

	return true;
}

static bool servicecallback_dbg_extralogging(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError     lsError;
	LSErrorInit(&lsError);
	json_object* json = 0;

    // {}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_ANY);

	s_extraLogging = !s_extraLogging;

	json = json_object_new_object();

	json_object_object_add(json, "returnValue", json_object_new_boolean(true));
	json_object_object_add(json, "subscribed", json_object_new_boolean(false));
	json_object_object_add(json, "extraLogging", json_object_new_boolean(s_extraLogging));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree (&lsError);

	json_object_put(json);

	return true;
}

#ifdef TEST_INSTALLER_UPDATE_MANUALLY

//static QString stringToQString(const std::string& str)
//{
//	return QString::fromUtf8(str.c_str(), str.size());
//}

static bool testcallback_switchAppIdToUpdateMode(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError     lsError;
	LSErrorInit(&lsError);
	json_object* reply = 0;
	json_object* root = 0;
	std::string errorText;
	std::string appidparam;
	QString appId;
	ApplicationDescription * pAppdescriptor = 0;
	const LaunchPoint * pMainlp = 0;
	QBitArray statusBits = QBitArray(LaunchPointUpdatedReason::SIZEOF);

    // {"appid": string }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(appid, string)));

	reply = json_object_new_object();
	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done;
	}

	if (extractFromJson(root,"appid",appidparam) == false)
	{
		errorText = "Missing appid parameter";
		goto Done;
	}

	pAppdescriptor = ApplicationManager::instance()->getAppById(appidparam);
	if (!pAppdescriptor)
	{
		errorText = "No such app with appId: " + appidparam;
		goto Done;
	}
	pMainlp = pAppdescriptor->getDefaultLaunchPoint();
	if (!pMainlp)
	{
		errorText = "App with appId: " + appidparam + " has no default launchpoint!";
		goto Done;
	}

	pAppdescriptor->setStatus(ApplicationDescription::Status_Installing);
	pAppdescriptor->dbgSetProgressManually(0);
	statusBits.setBit(LaunchPointUpdatedReason::Status);
	ApplicationManager::instance()->dbgEmitSignalLaunchPointUpdated(pMainlp,statusBits);

Done:
	json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
	if (!errorText.empty())
	{
		json_object_object_add(reply, "errorText", json_object_new_string(errorText.c_str()));
	}
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lsError))
		LSErrorFree (&lsError);

	if (root)
	{
		json_object_put(root);
	}
	json_object_put(reply);
	return true;
}

static bool testcallback_switchAppIdToReady(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError     lsError;
	LSErrorInit(&lsError);
	json_object* reply = 0;
	json_object* root = 0;
	std::string errorText;
	std::string appidparam;
	QString appId;
	ApplicationDescription * pAppdescriptor = 0;
	const LaunchPoint * pMainlp = 0;
	QBitArray statusBits = QBitArray(LaunchPointUpdatedReason::SIZEOF);

    // {"appid": string }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(appid, string)));

	reply = json_object_new_object();
	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done;
	}

	if (extractFromJson(root,"appid",appidparam) == false)
	{
		errorText = "Missing appid parameter";
		goto Done;
	}

	pAppdescriptor = ApplicationManager::instance()->getAppById(appidparam);
	if (!pAppdescriptor)
	{
		errorText = "No such app with appId: " + appidparam;
		goto Done;
	}
	pMainlp = pAppdescriptor->getDefaultLaunchPoint();
	if (!pMainlp)
	{
		errorText = "App with appId: " + appidparam + " has no default launchpoint!";
		goto Done;
	}

	pAppdescriptor->setStatus(ApplicationDescription::Status_Ready);
	pAppdescriptor->dbgSetProgressManually(0);
	statusBits.setBit(LaunchPointUpdatedReason::Status);
	ApplicationManager::instance()->dbgEmitSignalLaunchPointUpdated(pMainlp,statusBits);

Done:
	json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
	if (!errorText.empty())
	{
		json_object_object_add(reply, "errorText", json_object_new_string(errorText.c_str()));
	}
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lsError))
		LSErrorFree (&lsError);

	if (root)
	{
		json_object_put(root);
	}
	json_object_put(reply);
	return true;
}
static bool testcallback_switchAppIdToUpdateFailed(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError     lsError;
	LSErrorInit(&lsError);
	json_object* reply = 0;
	json_object* root = 0;
	std::string errorText;
	std::string appidparam;
	QString appId;
	ApplicationDescription * pAppdescriptor = 0;
	const LaunchPoint * pMainlp = 0;
	QBitArray statusBits = QBitArray(LaunchPointUpdatedReason::SIZEOF);

    // {"appid": string }

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(appid, string)));

	reply = json_object_new_object();
	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done;
	}

	if (extractFromJson(root,"appid",appidparam) == false)
	{
		errorText = "Missing appid parameter";
		goto Done;
	}

	pAppdescriptor = ApplicationManager::instance()->getAppById(appidparam);
	if (!pAppdescriptor)
	{
		errorText = "No such app with appId: " + appidparam;
		goto Done;
	}
	pMainlp = pAppdescriptor->getDefaultLaunchPoint();
	if (!pMainlp)
	{
		errorText = "App with appId: " + appidparam + " has no default launchpoint!";
		goto Done;
	}

	pAppdescriptor->setStatus(ApplicationDescription::Status_Failed);
	pAppdescriptor->dbgSetProgressManually(0);
	statusBits.setBit(LaunchPointUpdatedReason::Status);
	ApplicationManager::instance()->dbgEmitSignalLaunchPointUpdated(pMainlp,statusBits);

Done:
	json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
	if (!errorText.empty())
	{
		json_object_object_add(reply, "errorText", json_object_new_string(errorText.c_str()));
	}
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lsError))
		LSErrorFree (&lsError);

	if (root)
	{
		json_object_put(root);
	}
	json_object_put(reply);
	return true;
}
static bool testcallback_changeAppIdProgress(LSHandle* lsHandle, LSMessage *message, void *userData)
{
	LSError     lsError;
	LSErrorInit(&lsError);
	json_object* reply = 0;
	json_object* root = 0;
	std::string errorText;
	std::string appidparam;
	QString appId;
	ApplicationDescription * pAppdescriptor = 0;
	const LaunchPoint * pMainlp = 0;
	QBitArray statusBits = QBitArray(LaunchPointUpdatedReason::SIZEOF);
	int progv=0;

    // {"appid": string, "v": string}

    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_2(REQUIRED(appid, string), REQUIRED(v, string)));

	reply = json_object_new_object();
	const char* str = LSMessageGetPayload( message );
	if (!str) {
		errorText = "No payload provided";
		goto Done;
	}

	root = json_tokener_parse( str );
	if (!root || is_error(root)) {
		errorText = "Malformed JSON detected in payload";
		root = 0;
		goto Done;
	}

	if (extractFromJson(root,"appid",appidparam) == false)
	{
		errorText = "Missing appid parameter";
		goto Done;
	}

	if (extractFromJson(root,"v",progv) == false)
	{
		errorText = "Missing v parameter (progress value)";
		goto Done;
	}

	pAppdescriptor = ApplicationManager::instance()->getAppById(appidparam);
	if (!pAppdescriptor)
	{
		errorText = "No such app with appId: " + appidparam;
		goto Done;
	}
	pMainlp = pAppdescriptor->getDefaultLaunchPoint();
	if (!pMainlp)
	{
		errorText = "App with appId: " + appidparam + " has no default launchpoint!";
		goto Done;
	}

	pAppdescriptor->setStatus(ApplicationDescription::Status_Installing);
	pAppdescriptor->dbgSetProgressManually(progv);
	statusBits.setBit(LaunchPointUpdatedReason::Status);
	statusBits.setBit(LaunchPointUpdatedReason::Progress);
	ApplicationManager::instance()->dbgEmitSignalLaunchPointUpdated(pMainlp,statusBits);

Done:
	json_object_object_add(reply, "returnValue", json_object_new_boolean(true));
	if (!errorText.empty())
	{
		json_object_object_add(reply, "errorText", json_object_new_string(errorText.c_str()));
	}
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lsError))
		LSErrorFree (&lsError);

	if (root)
	{
		json_object_put(root);
	}
	json_object_put(reply);
	return true;
}
#endif

////////////////////////////// ----- SERVICE INIT  ----- ///////////////////////////////////////////////////////////////

static LSMethod appMgrMethodsPublic[]  = {
		{ "open", servicecallback_open },
		{ "launch" , servicecallback_launch },
		{ "addLaunchPoint", servicecallback_addLaunchPoint },
		{ "removeLaunchPoint", servicecallback_removeLaunchPoint },
		{ "updateLaunchPointIcon", servicecallback_updateLaunchPointIcon },
		{ "mimeTypeForExtension" , servicecallback_getMimeTypeForExtension },
		{ "getHandlerForMimeType" , servicecallback_getHandlerForMimeType },
		{ "getHandlerForExtension" , servicecallback_getHandlerForExtension },
		{ "getHandlerForUrl" , servicecallback_getHandlerForUrl },
		{ "getHandlerForMimeTypeByVerb" , servicecallback_getHandlerByVerb },
		{ "getHandlerForUrlByVerb" , servicecallback_getHandlerByVerb },
		{ "listAllHandlersForMime" , servicecallback_listAllHandlers },
		{ "listAllHandlersForUrl" , servicecallback_listAllHandlers },
		{ "listAllHandlersForMultipleMime" , servicecallback_listAllHandlersMultipleMime },
		{ "listAllHandlersForMultipleUrlPattern" , servicecallback_listAllHandlersMultipleUrlPattern },
		{ "listAllHandlersForUrlPattern" , servicecallback_listAllUrlHandlersByPattern },
		{ "listAllHandlersForMimeByVerb" , servicecallback_listAllHandlersByVerb },
		{ "listAllHandlersForUrlByVerb" , servicecallback_listAllHandlersByVerb },
		{ "listExtensionMap"			, servicecallback_listExtensionMap },
		{ "getAppBasePath", servicecallback_getappbasepath},
		{ 0, 0 }
};

static LSMethod appMgrMethodsPrivate[] = {
		{ "install", servicecallback_install },
		{ "running", servicecallback_listRunningApps },
		{ "close", servicecallback_close },
		{ "listApps", servicecallback_listApps },
		{ "listPackages", servicecallback_listPackages },
		{ "getSizeOfApps" , servicecallback_getSizeOf },
		{ "searchApps" , servicecallback_searchForApps },
		{ "listLaunchPoints", servicecallback_listLaunchPoints },
		{ "listDockModeLaunchPoints", servicecallback_listDockModeLaunchPoints },
		{ "listPendingLaunchPoints", servicecallback_listPendingLaunchPoints },
		{ "listDockPoints", servicecallback_listDockPoints },
		{ "addResourceHandler", servicecallback_addResourceHandler },
		{ "addRedirectHandler", servicecallback_addRedirectHandler },
		{ "addDockModeLaunchPoint", servicecallback_addDockModeLaunchPoint },
		{ "removeDockModeLaunchPoint", servicecallback_removeDockModeLaunchPoint },
		{ "rescan", servicecallback_rescan },
		{ "launchPointChanges", servicecallback_launchPointChanges },
		{ "inspect", servicecallback_inspect },
		{ "getResourceInfo", servicecallback_getresourceinfo },
		{ "getAppInfo", servicecallback_getappinfo},
		{ "_dbg_extraLogging", servicecallback_dbg_extralogging},
		{ "listResourceHandlers", servicecallback_listResourceHandlers},
		{ "listRedirectHandlers", servicecallback_listRedirectHandlers},
		{ "dumpMimeTable",servicecallback_dumpMimeTable},
		{ "swapResourceHandler", servicecallback_swapResourceHandler },
		{ "swapRedirectHandler", servicecallback_swapRedirectHandler },
		{ "registerVerbsForResource", servicecallback_registerVerbs },
		{ "registerVerbsForRedirect", servicecallback_registerVerbs },
		{ "removeHandlersForAppId", servicecallback_removeHandlersForAppId },
		{ "saveMimeTable",			servicecallback_saveMimeTable },
		{ "restoreMimeTable",		servicecallback_restoreMimeTable },
		{ "clearMimeTable",			servicecallback_clearMimeTable},
		{ "resetToMimeDefaults",	servicecallback_deleteSavedTable},
		{ "_dbg_getAppEntryPoint",   servicecallback_dbg_getAppEntryPoint},
		{ "forceSingleAppScan",		servicecallback_forceSingleAppScan},

#ifdef AMS_TEST_MIME
		{ "TESTMIME_interleaveAddRemove" , testcallback_mimeAddRemoveInterleave },
		{ "TESTMIME_addEntries" , testcallback_mimeAddEntries },
		{ "TESTMIME_removeEntriesByAppId" , testcallback_mimeRemoveEntriesByAppId },
		{ "TESTMIME_swapResourceEntries" , testcallback_mimeSwapResourceEntries },
		{ "TESTMIME_swapRedirectEntries" , testcallback_mimeSwapRedirectEntries },
		{ "DBGMIME_printVerbCacheTable", dbgcallback_printVerbCacheTable },
#endif
#ifdef TEST_INSTALLER_UPDATE_MANUALLY
		{ "TESTINSTALL_switchAppIdToUpdateMode" , testcallback_switchAppIdToUpdateMode },
		{ "TESTINSTALL_switchAppIdToReady", testcallback_switchAppIdToReady },
		{ "TESTINSTALL_switchAppIdToUpdateFailed", testcallback_switchAppIdToUpdateFailed },
		{ "TESTINSTALL_changeAppIdProgress", testcallback_changeAppIdProgress },
#endif

		{ 0, 0 }
};

// Place the application manager onto the service bus.
bool ApplicationManager::startService()
{
	LSError lserror;
	LSErrorInit(&lserror);
	bool result;

	GMainLoop *mainLoop = HostBase::instance()->mainLoop();

	g_debug ("ApplicationManager started");

	result = LSRegisterPalmService("com.palm.applicationManager", &m_service, &lserror);
	if (!result)
	{
		g_message( "ApplicationManager::startService failed" );
		LSErrorFree(&lserror);

		return false;
	}

	result = LSPalmServiceRegisterCategory( m_service, "/", appMgrMethodsPublic, appMgrMethodsPrivate,
			NULL, NULL, &lserror);
	if (!result)
	{
		g_message( "ApplicationManager::startService failed" );
		LSErrorFree(&lserror);

		return false;
	}

	m_serviceHandlePublic = LSPalmServiceGetPrivateConnection(m_service);
	m_serviceHandlePrivate = LSPalmServiceGetPrivateConnection(m_service);

	result = LSGmainAttachPalmService(m_service, mainLoop, &lserror);
	if (!result)
	{
		g_message( "ApplicationManager::startService failed" );
		LSErrorFree(&lserror);

		return false;
	}

	if (Settings::LunaSettings()->uiType != Settings::UI_MINIMAL) {

		result = LSCall(m_serviceHandlePrivate, "palm://com.palm.bus/signal/registerServerStatus",
						"{\"serviceName\":\"com.palm.appInstallService\"}", 
						ApplicationManager::cbAppInstallServiceConnection, NULL, NULL, &lserror);
		if (!result)
		{
			g_message( "%s failed to connect to the appInstallService", __PRETTY_FUNCTION__ );
			LSErrorFree(&lserror);
		}
	}

	g_message( "ApplicationManager on service bus.");
	return true;
}

