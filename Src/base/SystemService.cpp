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

#include <glib.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <pbnjson.hpp>
#include <map>
#include <set>

#include "ApplicationDescription.h"
#include "ApplicationManager.h"
#include "ApplicationDescription.h"
#include "AnimationSettings.h"
#include "DisplayManager.h"
#include "HostBase.h"
#include "Logging.h"
#include "Settings.h"
#include "SystemService.h"
#include "SystemUiController.h"
#include "Utils.h"
#include "WindowServer.h"
#include "WebAppMgrProxy.h"
#include "MemoryMonitor.h"
#include "Security.h"
#include "EASPolicyManager.h"
#include "StatusBarServicesConnector.h"
#include "IMEController.h"
#include "VirtualKeyboardPreferences.h"
#include "SingleClickGestureRecognizer.h"
#include "CardWindow.h"

#include "cjson/json.h"
#include <pbnjson.hpp>
#include "JSONUtils.h"

#ifdef USE_HEAP_PROFILER
#include <google/heap-profiler.h>
#endif

static SystemService* s_instance = 0;
static ApplicationManager* s_AppManInstance = 0;
static const int sModalTimerInterval = 5000;
static const int sModalResetCounterLimit = 100;

static const std::string sModalWindowSubscriptionPrefix = "MODAL_WINDOW_";
static const std::string sCreateModalTag = ", \"createAsModal\": true }";
static const std::string sModalLaunchTimedOut = "Modal window launch took more than 5 seconds.";
static const std::string sModalDimissTimedOut = "Modal window dismiss took more than 5 seconds.";

QTimer SystemService::sModalLauchCheckTimer;
SystemService::ActiveModalDialogInfo SystemService::sActiveModalInfo;
std::string SystemService::sTempCaller;
std::string SystemService::sTempLaunchApp;
std::string SystemService::sModalWindowSubscriptionId;
int SystemService::sModalWindowIndex = 0;
bool SystemService::sIsParentPdkApp = false;

static bool cbTakeScreenShot(LSHandle* lshandle, LSMessage *message,
							 void *user_data);
static bool cbClearCache(LSHandle* lshandle, LSMessage *message,
							 void *user_data);
static bool cbSystemUi(LSHandle* lshandle, LSMessage *message,
							 void *user_data);
static bool cbSystemUiDbg(LSHandle* lshandle, LSMessage *message,
							 void *user_data);
static bool cbGetAppRestoreNeeded(LSHandle* lshandle, LSMessage *message,
								  void *user_data);
static bool cbGetForegroundApplication(LSHandle* lsHandle, LSMessage *message,
									   void *user_data);
static bool cbApplicationHasBeenTerminated(LSHandle* lsHandle, LSMessage *message,
									       void *user_data);
static bool cbGetLockStatus(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbGetDockModeStatus(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbSetDevicePasscode(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbMatchDevicePasscode(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbGetDeviceLockMode(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbGetSecurityPolicy(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbUpdatePinAppState(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbGetBootStatus(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbRunProgressAnimation(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbDebugger(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbSetJavascriptFlags(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbSetAnimationValues(LSHandle* lsHandle, LSMessage *message,
								void *user_data);
static bool cbGetAnimationValues(LSHandle* lsHandle, LSMessage *message,
								 void *user_data);
static bool cbPublishToSystemUI(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
static bool cbSubscribeToSystemUI(LSHandle* lsHandle, LSMessage *message,
							void *user_data);
#ifdef USE_HEAP_PROFILER
static bool cbDumpHeapProfiler(LSHandle* lsHandle, LSMessage *message,
								 void *user_data);
#endif

static bool cbLockButtonTriggered(LSHandle* lsHandle, LSMessage *message,
								void *user_data);
static bool cbMonitorProcessMemory(LSHandle* lsHandle, LSMessage *message,
								void *user_data);

static bool cbEnableFpsCounter(LSHandle* lsHandle, LSMessage *message,
							   void *user_data);

bool cbEnableTouchPlot(LSHandle* lsHandle, LSMessage *message,
								void *user_data);

static bool cbLogTouchEvents(LSHandle* lsHandle, LSMessage *message,
							   void *user_data);

static bool cbSetBenchmarkFlags(LSHandle* lsHandle, LSMessage *message,
                                void *user_data);

static bool cbDumpRasters(LSHandle* lsHandle, LSMessage *message,
						  void *user_data);

static bool cbDumpJemallocHeap(LSHandle* lsHandle, LSMessage *message,
							   void *user_data);

static bool cbTouchToShareDeviceInRange(LSHandle* lsHandle, LSMessage* message,
									   void* user_data);

static bool cbTouchToShareAppUrlTransferred(LSHandle* lsHandle, LSMessage* message,
											void* user_data);

static bool cbGetSystemStatus(LSHandle* lsHandle, LSMessage* message,
                                void* user_data);

static bool cbLaunchModalApp(LSHandle* lshandle, LSMessage *message,
							 	 void *user_data);

static bool cbDismissModalApp(LSHandle* lshandle, LSMessage *message,
							 	 void *user_data);

static bool cbSubscribeTurboMode(LSHandle* lshandle, LSMessage *message, void *user_data);

static bool cbSubscriptionCancel(LSHandle *lshandle, LSMessage *message, void *user_data);

static LSMethod s_methods[]  = {
	{ "takeScreenShot",    cbTakeScreenShot },
	{ "clearCache", 	   cbClearCache },
	{ "systemUi",          cbSystemUi },
	{ "getAppRestoreNeeded", cbGetAppRestoreNeeded },
	{ "applicationHasBeenTerminated", cbApplicationHasBeenTerminated},
	{ "getForegroundApplication", cbGetForegroundApplication },
	{ "getLockStatus", cbGetLockStatus },
	{ "getDockModeStatus", cbGetDockModeStatus },
	{ "setDevicePasscode", cbSetDevicePasscode },
	{ "matchDevicePasscode", cbMatchDevicePasscode },
	{ "getDeviceLockMode", cbGetDeviceLockMode },
	{ "getSecurityPolicy", cbGetSecurityPolicy },
	{ "updatePinAppState", cbUpdatePinAppState },
	{ "getBootStatus",     cbGetBootStatus },
	{ "runProgressAnimation", cbRunProgressAnimation },
	{ "debugger", cbDebugger },
	{ "setJavascriptFlags", cbSetJavascriptFlags },
	{ "setAnimationValues", cbSetAnimationValues },
	{ "getAnimationValues", cbGetAnimationValues },
	{ "publishToSystemUI", cbPublishToSystemUI },
	{ "subscribeToSystemUI", cbSubscribeToSystemUI },
#ifdef USE_HEAP_PROFILER
	{ "dumpHeapProfile", cbDumpHeapProfiler },
#endif
	{ "lockButtonTriggered", cbLockButtonTriggered},
	{ "monitorProcessMemory", cbMonitorProcessMemory},
        { "logTouchEvents", cbLogTouchEvents},
	{ "enableFpsCounter", cbEnableFpsCounter },
	{ "enableTouchPlot", cbEnableTouchPlot },
	{ "setBenchmarkFlags", cbSetBenchmarkFlags },
	{ "systemUiDbg",	   cbSystemUiDbg },
	{ "dumpRasters", cbDumpRasters },
	{ "dumpJemallocHeap", cbDumpJemallocHeap },
	{ "touchToShareDeviceInRange", cbTouchToShareDeviceInRange },
	{ "touchToShareAppUrlTransferred", cbTouchToShareAppUrlTransferred },
    { "getSystemStatus", cbGetSystemStatus },
    { "launchModalApp", cbLaunchModalApp },
    { "dismissModalApp", cbDismissModalApp },
    { "subscribeTurboMode", cbSubscribeTurboMode },
    { 0, 0 },
};


SystemService* SystemService::instance()
{
    if (!s_instance)
		new SystemService;

	return s_instance;
}

SystemService::SystemService()
{
    s_instance = this;
	m_brickMode = false;
	m_msmExitClean = false;
	m_fscking = false;
	m_cardLoadingAnimation = true;
	s_instance->resetModalDialogInfo();
	s_instance->initModalTimerInfo();
	s_AppManInstance = ApplicationManager::instance();
}

SystemService::~SystemService()
{
	// Should never reach here
    s_instance = 0;
}

void SystemService::init()
{
	startService();

#ifdef USE_HEAP_PROFILER
	HeapProfilerStart("sysmgr");
#endif

	connect(SystemUiController::instance(), SIGNAL(signalBootFinished()), this, SLOT(postBootFinished()));
	connect(SystemUiController::instance(), SIGNAL(signalModalWindowAdded()), this, SLOT(slotModalWindowAdded()));
	connect(SystemUiController::instance(), SIGNAL(signalModalWindowRemoved()), this, SLOT(slotModalWindowRemoved()));
	connect(&sModalLauchCheckTimer, SIGNAL(timeout()), SLOT(slotModalDialogTimerFired()));
}

void SystemService::startService()
{
	bool result;
    LSError lsError;
    LSErrorInit(&lsError);

	GMainLoop *mainLoop = HostBase::instance()->mainLoop();

	g_debug("SystemService starting...");

    result = LSRegister("com.palm.systemmanager", &m_service, &lsError);
	if (!result)
		goto Done;

    result = LSRegisterCategory(m_service, "/", s_methods, NULL, NULL, &lsError);
    if (!result)
		goto Done;

    result = LSSubscriptionSetCancelFunction(m_service, cbSubscriptionCancel, NULL, &lsError);
    if (!result)
		goto Done;

    result = LSGmainAttach(m_service, mainLoop, &lsError);
    if (!result)
		goto Done;

	// register for storage daemon signals
	result = LSCall(m_service,
					"palm://com.palm.lunabus/signal/addmatch",
					"{\"category\":\"/storaged\", \"method\":\"MSMAvail\"}",
					msmAvailCallback, NULL, &m_storageDaemonToken, &lsError);
	if (!result)
		goto Done;

	result = LSCall(m_service,
					"palm://com.palm.lunabus/signal/addmatch",
					"{\"category\":\"/storaged\", \"method\":\"MSMProgress\"}",
					msmProgressCallback, NULL, &m_storageDaemonToken, &lsError);
	if (!result)
		goto Done;

	result = LSCall(m_service,
					"palm://com.palm.lunabus/signal/addmatch",
					"{\"category\":\"/storaged\", \"method\":\"MSMEntry\"}",
					msmEntryCallback, NULL, &m_storageDaemonToken, &lsError);
	if (!result)
		goto Done;

	result = LSCall(m_service,
					"palm://com.palm.lunabus/signal/addmatch",
					"{\"category\":\"/storaged\", \"method\":\"MSMFscking\"}",
					msmFsckingCallback, NULL, &m_storageDaemonToken, &lsError);
	if (!result)
		goto Done;

	result = LSCall(m_service,
					"palm://com.palm.lunabus/signal/addmatch",
					"{\"category\":\"/storaged\", \"method\":\"PartitionAvail\"}",
					msmPartitionAvailCallback, NULL, &m_storageDaemonToken, &lsError);
	if (!result)
		goto Done;

	// Register for telephony service coming on bus
	result = LSCall(m_service,
					"palm://com.palm.lunabus/signal/registerServerStatus",
					"{\"serviceName\":\"com.palm.telephony\"}",
					telephonyServiceUpCallback, NULL, NULL, &lsError);
	if (!result)
		goto Done;

	// Register for tap2share signals coming on bus
	result = LSCall(m_service,
					"palm://com.palm.lunabus/signal/addmatch",
					"{\"category\":\"/com/palm/tap2shared\", \"method\":\"canTapStatus\"}",
					touchToShareCanTapStatusCallback, NULL, NULL, &lsError);
	if (!result)
		goto Done;

	postNovacomStatus();

Done:

	if (!result) {
		g_warning("Failed in System Service: %s", lsError.message);
		LSErrorFree(&lsError);
	}
	else {
		g_debug("SystemService on service bus");
	}
}

void SystemService::stopService()
{
    LSError lsError;
    LSErrorInit(&lsError);
    bool result;

	result = LSUnregister(m_service, &lsError);
    if (!result)
        LSErrorFree(&lsError);

	m_service = 0;
}

void SystemService::postForegroundApplicationChange(const std::string& title,
													const std::string& menuname,
													const std::string& id)
{
    bool    retVal;
	LSError lsError;
	json_object* json = 0;

	LSErrorInit(&lsError);

	json = json_object_new_object();
	if (!title.empty())
		json_object_object_add(json, (char*) "title",
							   json_object_new_string((char*) title.c_str()));
	if (!menuname.empty())
			json_object_object_add(json, (char*) "appmenu",
								   json_object_new_string((char*) menuname.c_str()));

	if (!id.empty())
		json_object_object_add(json, (char*) "id",
							   json_object_new_string((char*) id.c_str()));

	retVal = LSSubscriptionPost(m_service, "/", "getForegroundApplication",
								json_object_to_json_string(json), &lsError);
	if (!retVal)
		LSErrorFree (&lsError);

	json_object_put(json);
}

void SystemService::postApplicationHasBeenTerminated(const std::string& title,
													 const std::string& menuname,
													 const std::string& id)
{
    bool    retVal;
	LSError lsError;
	json_object* json = 0;

	LSErrorInit(&lsError);

	json = json_object_new_object();
	if (!title.empty())
		json_object_object_add(json, (char*) "title",
							   json_object_new_string((char*) title.c_str()));
	if (!menuname.empty())
			json_object_object_add(json, (char*) "appmenu",
								   json_object_new_string((char*) menuname.c_str()));

	if (!id.empty())
		json_object_object_add(json, (char*) "id",
							   json_object_new_string((char*) id.c_str()));

	retVal = LSSubscriptionPost(m_service, "/", "applicationHasBeenTerminated",
								json_object_to_json_string(json), &lsError);
	if (!retVal)
		LSErrorFree (&lsError);

	json_object_put(json);
}

void SystemService::postLockStatus(bool locked)
{
	LSError lsError;
	json_object* json = 0;

	json = json_object_new_object();
	json_object_object_add(json, "locked", json_object_new_boolean(locked));

	LSErrorInit(&lsError);
	if (!LSSubscriptionPost(m_service, "/", "getLockStatus", json_object_to_json_string(json), &lsError))
		LSErrorFree(&lsError);

	json_object_put(json);
}

void SystemService::postDockModeStatus(bool enabled)
{
	LSError lsError;
	json_object* json = 0;

	json = json_object_new_object();
	json_object_object_add(json, "enabled", json_object_new_boolean(enabled));

	LSErrorInit(&lsError);
	if (!LSSubscriptionPost(m_service, "/", "getDockModeStatus", json_object_to_json_string(json), &lsError))
		LSErrorFree(&lsError);

	json_object_put(json);
}

void SystemService::postDeviceLockMode()
{
	LSError lsError;
	json_object* json = 0;

	json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(true));
	json_object_object_add(json, "lockMode", json_object_new_string(Security::instance()->getLockMode().c_str()));
	json_object_object_add(json, "policyState", json_object_new_string(EASPolicyManager::instance()->getPolicyState().c_str()));
	json_object_object_add(json, "retriesLeft", json_object_new_int(EASPolicyManager::instance()->retriesLeft()));

	LSErrorInit(&lsError);
	if (!LSSubscriptionPost(m_service, "/", "getDeviceLockMode", json_object_to_json_string(json), &lsError))
		LSErrorFree(&lsError);

	json_object_put(json);
}

void SystemService::postBootFinished()
{
    bool    retVal;
	LSError lsError;
	json_object* json = 0;

	LSErrorInit(&lsError);

	json = json_object_new_object();
	json_object_object_add(json, (char*) "finished", json_object_new_boolean(true));
	json_object_object_add(json, (char*) "firstUse",
						   json_object_new_boolean(Settings::LunaSettings()->uiType == Settings::UI_MINIMAL));

	retVal = LSSubscriptionPost(m_service, "/", "getBootStatus",
								json_object_to_json_string(json), &lsError);
	if (!retVal)
		LSErrorFree (&lsError);

	json_object_put(json);
}

void SystemService::buildSubscriptionModalId(std::string& caller, std::string& launchApp)
{
	++sModalWindowIndex;

	// reset this if it goes past some limit that we set
	if(sModalWindowIndex > sModalResetCounterLimit)
		sModalWindowIndex = 1;

	// clear the string if it exists.
	if(0 != sModalWindowSubscriptionId.length())
		sModalWindowSubscriptionId.clear();

	// make the modal id
	char idStr[256];
	snprintf(idStr, 256, "%s%s%c%s%c%d", sModalWindowSubscriptionPrefix.c_str(), caller.c_str(), '_', launchApp.c_str(), '_', sModalWindowIndex);
	idStr[255] = 0;

	// This string will be Modal Id Prefix + Caller + App To Be Launched + sModalWindowIndex
	sModalWindowSubscriptionId = idStr;
}

std::string SystemService::getModalWindowSubscriptionId()
{
	return sModalWindowSubscriptionId;
}

std::string SystemService::getModalDismissReturnValue()
{
	return sActiveModalInfo.m_returnValueToPost;
}

void SystemService::resetModalDialogInfo()
{
	sActiveModalInfo.m_isModalAppBeingLaunched = false;
	sActiveModalInfo.m_isModalAppLaunched = false;
	sActiveModalInfo.m_isModalAppBeingRemoved = false;
	sActiveModalInfo.m_activeDialogApp.clear();
	sActiveModalInfo.m_activeDialogCaller.clear();
	sActiveModalInfo.m_returnValueToPost.clear();
	sModalWindowSubscriptionId.clear();

	sTempCaller.clear();
	sTempLaunchApp.clear();
	sIsParentPdkApp = false;
}

void SystemService::initModalTimerInfo()
{
	sModalLauchCheckTimer.setSingleShot(true);
	sModalLauchCheckTimer.setInterval(sModalTimerInterval);
}

bool SystemService::isModalActive()
{
	return sActiveModalInfo.m_isModalAppBeingLaunched;
}

void SystemService::setActiveModalBeingLaunched()
{
	// stop if the timer is active.
	sModalLauchCheckTimer.stop();

	sActiveModalInfo.m_isModalAppBeingLaunched = true;
	sModalLauchCheckTimer.start();
}

void SystemService::setActiveModalBeingRemoved()
{
	// stop if the timer is active.
	sModalLauchCheckTimer.stop();

	sActiveModalInfo.m_isModalAppBeingRemoved = true;
	sModalLauchCheckTimer.start();
}

void SystemService::setActiveModalInfo()
{
	if(true == sActiveModalInfo.m_isModalAppBeingLaunched) {
		sActiveModalInfo.m_isModalAppLaunched = true;
		sActiveModalInfo.m_activeDialogApp = sTempLaunchApp;
		sActiveModalInfo.m_activeDialogCaller = sTempCaller;

		sTempCaller.clear();
		sTempLaunchApp.clear();
	}
}

void SystemService::saveLauncherAndCallerInfo(std::string& caller, std::string& launchedApp)
{
	if(true == sActiveModalInfo.m_isModalAppBeingLaunched) {
		sTempLaunchApp = launchedApp;
		sTempCaller = caller;
	}
}

void SystemService::setReturnValueToPost(const std::string& retValue)
{
	sActiveModalInfo.m_returnValueToPost = retValue;
}

bool SystemService::isParentPdk()
{
	return SystemService::sIsParentPdkApp;
}

void SystemService::setParentAppPdk(bool isPdk)
{
	SystemService::sIsParentPdkApp = isPdk;
}

// This will be called when SysUiController successfully adds a new Modal Window
void SystemService::slotModalWindowAdded()
{
	// If we timedout waiting to create the model && the modal was actually created, then we would have issued a command to dismiss the modal. If the modal got added later, dismiss it.
	if(false == sActiveModalInfo.m_isModalAppBeingLaunched && false == sActiveModalInfo.m_isModalAppLaunched && false == sActiveModalInfo.m_isModalAppBeingRemoved) {
		SystemService::instance()->notifyDismissModalDialog();
		return;
	}

	// If the timer has not fired, then cancel the timer.
	sModalLauchCheckTimer.stop();

	// Set the fact that we are not going to dismiss the modal
	SystemService::instance()->notifyDismissModalTimerStopped();

	// Set the params for the modal dialog
	setActiveModalInfo();

	// post the reply back to the caller.
	postLaunchModalResult();
}

// This will be called when SysUiController successfully removed a new Modal Window
void SystemService::slotModalWindowRemoved()
{
	// If the timer has not fired, then cancel the timer.
	sModalLauchCheckTimer.stop();

	// If we timedout waiting to create the model && the modal was actually created, then we would have issued a command to dismiss the modal. We just got notified of that fact.
	// So just reset the flag and don't post the message.
	if(false == sActiveModalInfo.m_isModalAppBeingLaunched && false == sActiveModalInfo.m_isModalAppLaunched && false == sActiveModalInfo.m_isModalAppBeingRemoved) {
		return;
	}
	else if(true == sActiveModalInfo.m_isModalAppBeingLaunched && false == sActiveModalInfo.m_isModalAppLaunched && false == sActiveModalInfo.m_isModalAppBeingRemoved) {
		// we tried to launch the modal, but it wasnt launched successfully. Post Launch instead of dismiss result.
		postLaunchModalResult();
	}
	else {
		// post the reply back to the caller.
		postDismissModalResult();
	}

	// UnInit.
	resetModalDialogInfo();
}

// This will be called 5 seconds after we issue the command to launch/dismiss the app. If the app isnt the currently active app, we report failure to the service
void SystemService::slotModalDialogTimerFired()
{
	if(true == sActiveModalInfo.m_isModalAppBeingRemoved) {
		// post the reply back to the caller.
		postDismissModalResult(true);
	}
	else if(false == sActiveModalInfo.m_isModalAppLaunched && true == sActiveModalInfo.m_isModalAppBeingLaunched) {
		// post the reply back to the caller.
		postLaunchModalResult(true);
	}

	// UnInit.
	resetModalDialogInfo();
}

json_object* SystemService::buildParamsForAppLaunch(std::string params, std::string& launchId, bool& success, std::string& errMsg)
{
	success = false;
	json_object* json = NULL;
	bool bUseComma = false;
	std::string toAdd;

	// clear this string.
	errMsg.clear();

	if ( strstr(params.c_str(), ":") != NULL ) bUseComma = true;

	if ( bUseComma )
	{
		toAdd = ", \"modalId\": \"";
	}
	else
	{
		toAdd = "\"modalId\": \"";
	}

	// Add the modalId info.
	toAdd += SystemService::instance()->getModalWindowSubscriptionId();

	// Add the closing double quote after the modalId
	toAdd += '\"';

	// Add the fact that we need to create a modal dialog
	toAdd += sCreateModalTag;

	int workingParamsLen = params.length() + toAdd.length() + 1;
	char *workingParams = new char[workingParamsLen];
	sprintf(workingParams, "%s", params.c_str());

	// back up till you reach the close brace (there has to be one. It's json formatted)
	char *p = workingParams + strlen(workingParams);
	while ( (p>=workingParams) && (*p!='}') )
	{
		p--;
	}

	if ( p <= workingParams )
	{
		// couldn't find the close brace. Shouldn't happen.
		success = false;
		errMsg = "Malformed parameters detected";
	}
	else {
		// lay down the rest of the string
		sprintf(p, "%s", toAdd.c_str());
		// now, assemble a launch string suitable for doLaunch
		json = json_object_new_object();
		json_object_object_add(json, "id", json_object_new_string(launchId.c_str()));
		json_object_object_add(json, "params", json_object_new_string(workingParams));
		success = true;
	}

	//cleanup
	delete[] workingParams;
	return json;
}

std::string SystemService::getStrFromJSON(json_object *root, const char *id)
{
	json_object *label = NULL;
	std::string ret = "";

	label = json_object_object_get(root, id);
	if (!label)
	{
		// fail
		return ret;
	}

	ret = json_object_get_string(label);
	return ret;
}

void SystemService::postLockButtonTriggered()
{
	LSError lsError;
	json_object* json = 0;

	LSErrorInit(&lsError);

	json = json_object_new_object();
	json_object_object_add(json, "triggered", json_object_new_boolean(true));

	if (!LSSubscriptionPost(m_service, "/", "lockButtonTriggered",
							json_object_to_json_string(json), &lsError)) {

		LSErrorFree(&lsError);
	}

	json_object_put(json);
}

void SystemService::postLaunchModalResult(bool timedOut)
{
	bool    retVal;
	json_object* json = 0;

	LSError lsError;
	LSErrorInit(&lsError);

	// If we timedout launching a modal, then we would have already posted a reply saying that we errored our creating a modal. If the result of the previous call to launch the modal came in now, just ignore
	if((false == sActiveModalInfo.m_isModalAppBeingLaunched) && (0 >= sModalWindowSubscriptionId.length())) {
		// Signal the modal window to be dismissed.
		SystemService::instance()->notifyDismissModalDialog();
		return;
	}
	else {
		json = json_object_new_object();

		if(false == timedOut) {
			// if we fail to create the modal [another instance of the modal was running or some other error case] we need to add the right error code
			bool added = SystemUiController::instance()->wasModalAddedSuccessfully();
			json_object_object_add(json, "returnValue", json_object_new_boolean(added));
			if(true == added)
				json_object_object_add(json, (char*) "launchResult", json_object_new_string(SystemUiController::instance()->getModalWindowLaunchErrReason().c_str()));
			else {
				json_object_object_add(json, (char*) "errorText", json_object_new_string(SystemUiController::instance()->getModalWindowLaunchErrReason().c_str()));
				json_object_object_add(json, (char*) "errorCode", json_object_new_int(SystemUiController::instance()->getModalWindowLaunchErrReasonCode()));
			}
		}
		else {
			json_object_object_add(json, "returnValue", json_object_new_boolean(false));
			json_object_object_add(json, (char*) "errorText", json_object_new_string(sModalLaunchTimedOut.c_str()));
		}
	}

	retVal = LSSubscriptionReply(m_service, SystemService::instance()->getModalWindowSubscriptionId().c_str(),
			json_object_to_json_string(json), &lsError);

	if (!retVal)
		LSErrorFree (&lsError);

	json_object_put(json);

	// Reset the status of the launch result of the modal dialog
	SystemUiController::instance()->setModalWindowLaunchErrReason(SystemUiController::LaunchUnknown);
}

void SystemService::postDismissModalResult(bool timedOut)
{
	bool    retVal;
	json_object* json = 0;

	LSError lsError;
	LSErrorInit(&lsError);

	// If we timedout dismissing a modal, then we would have already posted a reply saying that we errored our dismissing the modal. If the previous call to dismiss the modal came in now, just ignore
	if((false == sActiveModalInfo.m_isModalAppBeingRemoved) && (0 >= sModalWindowSubscriptionId.length())) {
		return;
	}
	else {
		json = json_object_new_object();

		if(false == timedOut) {
			json_object_object_add(json, "returnValue", json_object_new_boolean(true));
			json_object_object_add(json, (char*) "returnMessage", json_object_new_string(SystemService::instance()->getModalDismissReturnValue().c_str()));
			json_object_object_add(json, (char*) "dismissResult", json_object_new_string(SystemUiController::instance()->getModalWindowDismissErrReason().c_str()));
		}
		else {
			json_object_object_add(json, "returnValue", json_object_new_boolean(false));
			json_object_object_add(json, (char*) "errorText", json_object_new_string(sModalDimissTimedOut.c_str()));
		}
	}

	retVal = LSSubscriptionReply(m_service, SystemService::instance()->getModalWindowSubscriptionId().c_str(),
			json_object_to_json_string(json), &lsError);

	if (!retVal)
		LSErrorFree (&lsError);

	json_object_put(json);

	// Reset the status of the launch result of the modal dialog
	SystemUiController::instance()->setModalWindowDismissErrReason(SystemUiController::DismissUnknown);
}

void SystemService::postAppRestoredNeeded()
{
	bool    retVal;
	LSError lsError;
	json_object* json = 0;

	LSErrorInit(&lsError);

	json = json_object_new_object();
	json_object_object_add(json, (char*) "appRestoreNeeded", json_object_new_boolean(true));

	retVal = LSSubscriptionPost(m_service, "/", "getAppRestoreNeeded",
			json_object_to_json_string(json), &lsError);
	if (!retVal)
		LSErrorFree (&lsError);

	json_object_put(json);
}

void SystemService::shutdownDevice()
{
	// tell the device to shut itself off
	LSError lsError;
	LSErrorInit(&lsError);

	json_object* json = json_object_new_object();
	json_object_object_add(json, "reason", json_object_new_string("PowerOff Selected by User"));

	if (!LSCall(m_service, "palm://com.palm.power/shutdown/machineOff",
				json_object_to_json_string(json),
				NULL, NULL, NULL, &lsError))
	{
		LSErrorFree(&lsError);
	}

	json_object_put(json);

	// shutdown sysmgr ASAP
	WindowServer::instance()->shutdown();
}

void SystemService::enterMSM()
{
	DisplayManager* dm = DisplayManager::instance();
	if (Settings::LunaSettings()->uiType == Settings::UI_MINIMAL ||
		brickMode() ||
		!dm->isUSBCharging() ||
		dm->isOnCall() ||
		SystemUiController::instance()->isScreenLocked())
		return;

	LSError lsError;
	LSErrorInit(&lsError);

	json_object* payload = json_object_new_object();
	json_object_object_add(payload, "user-confirmed", json_object_new_boolean(true));
	json_object_object_add(payload, "enterIMasq", json_object_new_boolean(false));

	if (!LSCall(m_service, "palm://com.palm.storage/diskmode/enterMSM",
				json_object_to_json_string(payload), NULL, NULL, NULL, &lsError))
		LSErrorFree(&lsError);

	json_object_put(payload);
}

//static pbnjson::JSchema cbSystemUiSchema()
//{
//	/* using orderly-json.org to convert
//	object {
//	  boolean quicklaunch?;
//	  boolean launcher?;
//	  boolean "universal search"?
//	};*/
//	// TODO: Move this out to a file
//	std::string schema =
//	"{"
//	    "\"type\": \"object\","
//	    "\"properties\": {"
//	        "\"quicklaunch\": {"
//	            "\"type\": \"boolean\","
//	            "\"optional\": true"
//	        "},"
//	        "\"launcher\": {"
//	            "\"type\": \"boolean\","
//	            "\"optional\": true"
//	        "},"
//	        "\"universal search\": {"
//	            "\"type\": \"boolean\","
//	            "\"optional\": true"
//	        "},"
//			"\"launchermenu\": {"
//				"\"type\": \"string\","
//				"\"optional\": true"
//			"},"
//			"\"launchertitlechange\": {"
//				"\"type\": \"object\","
//				"\"optional\": true,"
//				"\"properties\": {"
//					"\"id\": { \"type\":\"integer\" } ,"
//					"\"label\": { \"type\":\"string\" }"
//				"}"
//			"},"
//			"\"launcheraction\": {"
//				"\"type\": \"string\","
//				"\"optional\": true"
//			"}"
//	    "},"
//	    "\"additionalProperties\": false"
//	"}";
//
//	return pbnjson::JSchemaFragment(schema.c_str());
//}

//workaround until an agreement is reached with activitydamager about message formats
static pbnjson::JSchema cbSystemUiSchema()
{
	return pbnjson::JSchemaFragment("{}");
}

static bool cbSystemUi(LSHandle* lshandle, LSMessage *message, void *user_data)
{
    DEPRECATED_SERVICE_MSG();

    LSError lserror;
    LSErrorInit(&lserror);
	std::string result;
	std::string menuOp;
	int launcherCardId;
	std::string launcherCardLabel;
	pbnjson::JValue payload = pbnjson::Object();

	const char* str = LSMessageGetPayload(message);
    if (!str)
		return false;

	pbnjson::JDomParser parser;
	if (!parser.parse(str, cbSystemUiSchema()))
		return true;
	std::string launcherAction;
	pbnjson::JValue root = parser.getDom();
	bool shown;

	if (CONV_OK == root["launcheraction"].asString(launcherAction))
	{
		if (launcherAction == "showAppMenu")
			SystemUiController::instance()->launcherAction(SystemUiController::LAUNCHERACT_MENUACTIVE);
		else if (launcherAction == "showAppInfo")
			SystemUiController::instance()->launcherAction(SystemUiController::LAUNCHERACT_INFOACTIVE);
		else if (launcherAction == "editPageTitle")
			SystemUiController::instance()->launcherAction(SystemUiController::LAUNCHERACT_EDITINGCARDTITLE);
	}
	else if (root["virtualkeyboard"].isObject()) {
		VirtualKeyboard * kb = VirtualKeyboardPreferences::instance().virtualKeyboard();
		if (kb)
		{
			pbnjson::JValue reply = pbnjson::Object();
			pbnjson::JValue vkb = root["virtualkeyboard"];
			bool	sizeAndHeightDone = false;
			for (pbnjson::JValue::ObjectIterator pair = vkb.begin(); pair != vkb.end(); pair++)
			{
				std::string	name;
				if (VERIFY((*pair).first.asString(name) == CONV_OK))
				{
					if (name == "get")
					{
						std::string	valueName;
						if ((*pair).second.asString(valueName) == CONV_OK)
						{
							std::string value;
							if (kb->getValue(valueName, value))
								reply.put(valueName, value);
							else
								reply.put(valueName, "<unsupported name>");
						}
						else
							reply.put("get", "Invalid value type. Expecting a name (string).");
					}
					else
					{
						bool boolValue;
						int intValue;
						if ((*pair).second.asNumber(intValue) == CONV_OK)
						{
							if (name == "height" || name == "size")
							{
								if (!sizeAndHeightDone)
								{
									sizeAndHeightDone = true;
									int otherInt;
									if (name == "height")
									{
										if (VERIFY(intValue >= 50))
										{
											if (CONV_OK == vkb["size"].asNumber(otherInt))
											{
												kb->changePresetHeightForSize(otherInt, intValue);
												kb->requestSize(otherInt);
											}
											else
												kb->requestHeight(intValue);
										}
									}
									else if (name == "size")
									{
										if (CONV_OK == vkb["height"].asNumber(otherInt) && VERIFY(otherInt >= 50))
											kb->changePresetHeightForSize(intValue, otherInt);
										kb->requestSize(intValue);
									}
								}
							}
							else
							{
								if (!kb->setIntOption(name, intValue))
									reply.put(name, "Virtual keyboard: integer option not supported");
							}
						}
						else if ((*pair).second.asBool(boolValue) == CONV_OK)
						{
							if (name == "hide")
							{
								if (boolValue)
									kb->hide();
							}
							else
							{
								if (!kb->setBoolOption(name, boolValue))
									reply.put(name, "Virtual keyboard: boolean option not supported");
							}
						}
						else
							reply.put(name, "Invalid value type. Need boolean or integer value.");
					}
				}
			}
			if (reply.begin() != reply.end())
				payload.put("virtualkeyboard", reply);
		}
	}
	else
	{
		if (CONV_OK == root["quicklaunch"].asBool(shown)) {
			SystemUiController::instance()->showOrHideDock(shown);
		}
		if (CONV_OK == root["universal search"].asBool(shown)) {
			bool displayLauncher, speedDial = false;
			if (CONV_OK != root["launcher"].asBool(displayLauncher))
				displayLauncher = !shown;
			if (CONV_OK != root["speedDial"].asBool(speedDial))
				speedDial = false;
			payload.put("wasLauncherShown", SystemUiController::instance()->isLauncherShown());
			SystemUiController::instance()->showOrHideUniversalSearch(shown, displayLauncher, speedDial);
		} else if (CONV_OK == root["launcher"].asBool(shown)) {
			SystemUiController::instance()->showOrHideLauncher(shown);
		} else if ((CONV_OK == root["launchermenu"].asString(menuOp)) && (SystemUiController::instance()->isLauncherShown())) {
			if (menuOp == "addcard")
				SystemUiController::instance()->launcherMenuOp(SystemUiController::LAUNCHEROP_ADDCARD);
			else if (menuOp == "reordercards")
				SystemUiController::instance()->launcherMenuOp(SystemUiController::LAUNCHEROP_REORDER);
			else if (menuOp == "deletecard")
				SystemUiController::instance()->launcherMenuOp(SystemUiController::LAUNCHEROP_DELETECARD);
			else if (menuOp == "invokerenamecard")
				SystemUiController::instance()->launcherMenuOp(SystemUiController::LAUNCHEROP_INVOKERENAMECARD);
			else if (menuOp == "other")					//all the other menu options in the launcher menu that don't specifically trigger native launcher code
				SystemUiController::instance()->launcherMenuOp(SystemUiController::LAUNCHEROP_UNKNOWN);
		} else if (root["launchertitlechange"].isObject()) {
			if ((CONV_OK == root["launchertitlechange"]["id"].asNumber(launcherCardId))
					&& (CONV_OK == root["launchertitlechange"]["label"].asString(launcherCardLabel)))
			{
				SystemUiController::instance()->launcherChangeCardTitle(launcherCardId,launcherCardLabel);
			}
		}
	}

	payload.put("returnValue", true);

	pbnjson::JGenerator generator;

	generator.toString(payload, pbnjson::JSchemaFragment("{}"), result);

	if (!LSMessageReply(lshandle, message, result.c_str(), &lserror))
		LSErrorFree(&lserror);

	return true;
}

static pbnjson::JSchema cbSystemUiDbgSchema()
{
	return pbnjson::JSchemaFragment("{}");
}

static bool cbSystemUiDbg(LSHandle* lshandle, LSMessage *message,
							 void *user_data)
{
    // {"state": string, "x": integer, "y", integer}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_3(REQUIRED(state, string), REQUIRED(x, integer), REQUIRED(y, integer)));

	LSError lserror;
	LSErrorInit(&lserror);
	std::string result;
	std::string menuOp;
	pbnjson::JValue payload = pbnjson::Object();

	const char* str = LSMessageGetPayload(message);
	if (!str)
		return false;

	pbnjson::JDomParser parser;
	if (!parser.parse(str, cbSystemUiDbgSchema()))
		return true;
	pbnjson::JValue root = parser.getDom();
	std::string state;

	if (CONV_OK == root["state"].asString(state))
	{
		int xspan;
		int yspan;
		if (state == "on")
		{
			if (CONV_OK != root["x"].asNumber(xspan))
				xspan = 4;
			if (CONV_OK != root["y"].asNumber(yspan))
				yspan = 4;

			SystemUiController::instance()->launcherDbgActionScreenGrid(SystemUiController::LAUNCHERACT_DBG_ACTIVATE_SCREEN_GRID,
																xspan,yspan);
		}
		else
			SystemUiController::instance()->launcherDbgActionScreenGrid(SystemUiController::LAUNCHERACT_DBG_DEACTIVATE_SCREEN_GRID);

	}


	payload.put("returnValue", true);
	pbnjson::JGenerator generator;
	generator.toString(payload, pbnjson::JSchemaFragment("{}"), result);
	if (!LSMessageReply(lshandle, message, result.c_str(), &lserror))
		LSErrorFree(&lserror);

	return true;
}

static bool cbGetAppRestoreNeeded(LSHandle* lsHandle, LSMessage *message,
								  void *user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(lsHandle, message);

    bool        success = true;
	LSError     lsError;
	json_object* json = json_object_new_object();
	bool subscribed = false;

	LSErrorInit(&lsError);

	if (LSMessageIsSubscription(message)) {

		success = LSSubscriptionProcess(lsHandle, message, &subscribed, &lsError);
		if (!success) {
			LSErrorFree (&lsError);
			goto Done;
		}
	}

Done:

	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree (&lsError);

	json_object_put(json);

	return true;
}

static bool cbClearCache(LSHandle* handle, LSMessage * msg, void * data)
{
    EMPTY_SCHEMA_RETURN(handle, msg);

	WebAppMgrProxy::instance()->clearWebkitCache();

	json_object* payload = json_object_new_object();
	json_object_object_add(payload, "returnValue", json_object_new_boolean(true));
	
	LSError err;
	LSErrorInit(&err);
	if (!LSMessageReply(handle, msg, json_object_to_json_string(payload), &err))
		LSErrorFree(&err);

	json_object_put(payload);

	return true;
}

static bool cbTakeScreenShot(LSHandle* lshandle, LSMessage *message,
							 void *user_data)
{
    LSError lserror;
    LSErrorInit(&lserror);

    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_1(REQUIRED(file, string)));

    const char* str = LSMessageGetPayload(message);

    if( !str )
		return false;

	bool success = false;

	struct json_object* root = json_tokener_parse(str);
	struct json_object* label = 0;
	std::string filePath;

	if (is_error(root)) {
		root = 0;
		success = false;
		goto Done;
	}

	label = json_object_object_get(root, "file");
	if (!label) {
		g_warning("%s: Failed to find param file in message", __PRETTY_FUNCTION__);
		goto Done;
	}
	filePath = json_object_get_string(label);

	if (filePath.empty())
		goto Done;

	if (filePath[0] == '/') {
		// absolute path.
	}
	else {
		// relative path. Dump to home folder

		const char* homeFolder = getenv("HOME");
		if (!homeFolder) {
			homeFolder = "/home/root";
		}

		filePath = std::string(homeFolder) + "/" + filePath;
	}

	success = WindowServer::instance()->takeScreenShot(filePath.c_str());

Done:

	if (root)
		json_object_put(root);

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror )) {
		LSErrorFree (&lserror);
	}

	json_object_put(json);

	return true;
}

static bool cbApplicationHasBeenTerminated(LSHandle* lsHandle, LSMessage *message,
									       void *user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(lsHandle, message);

	bool success = true;
	bool subscribed = false;
	LSError lsError;
	json_object* json = json_object_new_object();

	LSErrorInit(&lsError);

	if (LSMessageIsSubscription(message)) {
		success = LSSubscriptionProcess(lsHandle, message, &subscribed, &lsError);
		if (!success) {
			LSErrorFree(&lsError);
		}
	}

	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree(&lsError);

	json_object_put(json);
    
    return true;
}

static bool cbGetForegroundApplication(LSHandle* lsHandle, LSMessage *message,
									   void *user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(lsHandle, message);

    bool        success = true;
	LSError     lsError;
	json_object* json = json_object_new_object();
	std::string title;
	std::string appMenuName;
	std::string id;
	bool subscribed = false;

	LSErrorInit(&lsError);

	if (LSMessageIsSubscription(message)) {

		success = LSSubscriptionProcess(lsHandle, message, &subscribed, &lsError);
		if (!success) {
			LSErrorFree (&lsError);
			goto Done;
		}
	}

	title = SystemUiController::instance()->maximizedApplicationName();
	appMenuName = SystemUiController::instance()->maximizedApplicationMenuName();
	id = SystemUiController::instance()->maximizedApplicationId();

	if (!title.empty())
		json_object_object_add(json, (char*) "title",
							   json_object_new_string((char*) title.c_str()));
	if (!id.empty())
		json_object_object_add(json, (char*) "id",
							   json_object_new_string((char*) id.c_str()));
	if (!appMenuName.empty())
		json_object_object_add(json, (char*) "appmenu",
				json_object_new_string((char*) appMenuName.c_str()));

Done:

	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree (&lsError);

	json_object_put(json);

	return true;
}

static bool cbGetLockStatus(LSHandle* lsHandle, LSMessage* message, void* user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(lsHandle, message);

	bool		success = true;
	LSError		lsError;
	bool subscribed = false;
	json_object* json = json_object_new_object();

	LSErrorInit(&lsError);

	if (LSMessageIsSubscription(message)) {

		success = LSSubscriptionProcess(lsHandle, message, &subscribed, &lsError);
		if (!success) {
			LSErrorFree(&lsError);
			goto Done;
		}
	}

	json_object_object_add(json, (char*) "locked",
						   json_object_new_boolean(SystemUiController::instance()->isScreenLocked()));

Done:

	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree(&lsError);

	json_object_put(json);

	return true;
}

static bool cbGetDockModeStatus(LSHandle* lsHandle, LSMessage* message, void* user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(lsHandle, message);

	bool		success = true;
	LSError		lsError;
	bool subscribed = false;
	json_object* json = json_object_new_object();

	LSErrorInit(&lsError);

	if (LSMessageIsSubscription(message)) {

		success = LSSubscriptionProcess(lsHandle, message, &subscribed, &lsError);
		if (!success) {
			LSErrorFree(&lsError);
			goto Done;
		}
	}

	json_object_object_add(json, (char*) "enabled",
						   json_object_new_boolean(SystemUiController::instance()->isInDockMode()));

Done:

	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree(&lsError);

	json_object_put(json);

	return true;
}

static bool cbSetDevicePasscode(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
	int errorCode = 0;
	json_object* root = 0;
	json_object* prop = 0;
	std::string mode = "";
	std::string passcode = "";
	std::string errorText = "";

    // {"lockMode":string, "passCode":string}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_2(REQUIRED(lockMode, string), REQUIRED(passCode, string)));

	const char* payload = LSMessageGetPayload(message);
	if (!payload)
		return false;

	root = json_tokener_parse(payload);
	if (is_error(root))
		goto Done;

	// which mode are we setting?
	prop = json_object_object_get(root, "lockMode");
	if (!prop || is_error(prop))
		goto Done;
	mode = json_object_get_string(prop);

	// should be a valid string if we are trying to set a pin/password
	prop = json_object_object_get(root, "passCode");
	if (prop && !is_error(prop))
		passcode = json_object_get_string(prop);

	errorCode = Security::instance()->setPasscode(mode, passcode, errorText);

Done:

	json_object* json = json_object_new_object();
	if (errorCode < 0) {
		json_object_object_add(json, "returnValue", json_object_new_boolean(false));
		json_object_object_add(json, "errorText", json_object_new_string(errorText.c_str()));
		json_object_object_add(json, "errorCode", json_object_new_int(errorCode));
	}
	else {
		json_object_object_add(json, "returnValue", json_object_new_boolean(true));
	}

	LSError lsError;
	LSErrorInit(&lsError);
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree(&lsError);

	json_object_put(json);
	if (root && !is_error(root))
		json_object_put(root);

	return true;
}

static bool cbMatchDevicePasscode(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
	bool success = false;
	json_object* root = 0;
	json_object* key = 0;
	std::string passcode = "";
	int retries = 0;
	bool lockedOut = false;

    // {"passCode":string}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(passCode, string)));

	const char* payload = LSMessageGetPayload(message);
	if (!payload)
		return false;

	root = json_tokener_parse(payload);
	if (is_error(root))
		goto Done;

	// get passcode sent by user
	key = json_object_object_get(root, "passCode");
	if (!key || is_error(key))
		goto Done; // bad arguments passed in
    passcode = json_object_get_string(key);

	success = Security::instance()->matchPasscode(passcode, retries, lockedOut);

Done:

	json_object* reply = json_object_new_object();
	json_object_object_add(reply, "returnValue", json_object_new_boolean(success));
	if (!success) {
    	json_object_object_add(reply, "lockedOut", json_object_new_boolean(lockedOut));
	    json_object_object_add(reply, "retriesLeft", json_object_new_int(retries));
	}

	LSError lsError;
	LSErrorInit(&lsError);

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lsError))
		LSErrorFree(&lsError);

	if (root && !is_error(root))
		json_object_put(root);
	json_object_put(reply);

	return true;
}

static bool cbGetDeviceLockMode(LSHandle* lsHandle, LSMessage *message,	void *user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(lsHandle, message);

	bool success = true;
	bool subscribed = false;

	LSError lsError;

	LSErrorInit(&lsError);

	if (LSMessageIsSubscription(message)) {

		success = LSSubscriptionProcess(lsHandle, message, &subscribed, &lsError);
		if (!success) {
			LSErrorFree(&lsError);
			goto Done;
		}
	}

Done:

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));
	if (success) {
		json_object_object_add(json, "lockMode", json_object_new_string(Security::instance()->getLockMode().c_str()));
		json_object_object_add(json, "policyState", json_object_new_string(EASPolicyManager::instance()->getPolicyState().c_str()));
		json_object_object_add(json, "retriesLeft", json_object_new_int(EASPolicyManager::instance()->retriesLeft()));
	}

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree(&lsError);

	json_object_put(json);

	return true;
}

static bool cbGetSecurityPolicy(LSHandle* lsHandle, LSMessage* message, void* user_data)
{
    EMPTY_SCHEMA_RETURN(lsHandle, message);

	bool success = false;
	json_object* json = json_object_new_object();
	EASPolicyManager* pm = EASPolicyManager::instance();
	const EASPolicy * const p = pm->getPolicy();
	json_object* policy = (p != 0 ? p->toJSON() : 0);

	if (policy) {

		json_object* status = pm->getPolicyStatus();
		if (status != 0)
			json_object_object_add(policy, "status", status);

		json_object_object_add(json, "policy", policy);

		success = true;
	}
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));

	LSError	lsError;
	LSErrorInit(&lsError);
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree(&lsError);

	json_object_put(json);

	return true;
}


static bool cbUpdatePinAppState(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
	bool success = false;
	LSError lserror;
    LSErrorInit(&lserror);

    // {"state":string}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(state, string)));

	const char* str = LSMessageGetPayload(message);
    if( !str )
		return false;

	json_object* root = json_tokener_parse(str);
	if (root && !is_error(root)) {

		json_object* state = json_object_object_get(root, "state");
		if (state && !is_error(state)) {

			std::string newState = json_object_get_string(state);
			if (newState == "cancel") {
				success = true;
				SystemService::instance()->notifyCancelPinEntry();
			}
			else if (newState == "unlock") {
				success = true;
				SystemService::instance()->notifyDeviceUnlocked();
			}
		}
		json_object_put(root);
	}

	json_object* payload = json_object_new_object();
	json_object_object_add(payload, "returnValue", json_object_new_boolean(success));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(payload), &lserror))
		LSErrorFree(&lserror);

	json_object_put(payload);

	return true;
}

static bool cbGetBootStatus(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(lsHandle, message);

    bool        success = true;
	LSError     lsError;
	json_object* json = json_object_new_object();
	bool subscribed = false;

	LSErrorInit(&lsError);

	if (LSMessageIsSubscription(message)) {

		success = LSSubscriptionProcess(lsHandle, message, &subscribed, &lsError);
		if (!success) {
			LSErrorFree (&lsError);
			goto Done;
		}
	}

	json_object_object_add(json, (char*) "finished",
						   json_object_new_boolean(SystemUiController::instance()->bootFinished()));
	json_object_object_add(json, (char*) "firstUse",
						   json_object_new_boolean(Settings::LunaSettings()->uiType == Settings::UI_MINIMAL));

Done:

	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree (&lsError);

	json_object_put(json);

	return true;
}

static bool cbRunProgressAnimation(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
	bool retVal = false;
	json_object* root = 0;
	json_object* state = 0;
	json_object* type = 0;
    ProgressAnimation::Type pType = ProgressAnimation::TypeHp;
	LSError lsError;

    // {"type":string, "state":string}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_2(REQUIRED(type, string), REQUIRED(state, string)));

	const char* str = LSMessageGetPayload(message);
    if (!str) {
		goto Done;
	}

	root = json_tokener_parse(str);
	if(!root || is_error(root)) {
		goto Done;
	}

    type = json_object_object_get(root, "type");
    if (type && !is_error(type)) {

        std::string typeStr = json_object_get_string(type);
        if (typeStr == "msm") {
            pType = ProgressAnimation::TypeMsm;
        }
        else if (typeStr == "fsck") {
            pType = ProgressAnimation::TypeFsck;
        }
    }

	state = json_object_object_get(root, "state");
	if (state && !is_error(state)) {

		retVal = true;

		std::string newState = json_object_get_string(state);
		if(newState == "start") {
			WindowServer::instance()->startProgressAnimation(pType);
		}
		else if (newState == "stop") {
			WindowServer::instance()->stopProgressAnimation();
		}
	}

Done:

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(retVal));

    LSErrorInit(&lsError);
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree (&lsError);

	json_object_put(json);
	if (root && !is_error(root))
		json_object_put(root);
	return true;
}

static bool cbDebugger(LSHandle* lsHandle, LSMessage *message,void *user_data)
{
    EMPTY_SCHEMA_RETURN(lsHandle, message);

	WebAppMgrProxy::instance()->enableDebugger( true );

	LSError err;
	LSErrorInit(&err);

	json_object* payload = json_object_new_object();
	json_object_object_add(payload, "returnValue", json_object_new_boolean(true));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(payload), &err))
		LSErrorFree(&err);

	json_object_put(payload);

	return true;
}

static bool cbSetJavascriptFlags(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
	bool success = false;
	json_object* root = 0;
	json_object* jflags =0 ;

    // {"flags":string}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(flags, string)));

	const char* payload = LSMessageGetPayload(message);
	if (!payload)
		return false;

	root = json_tokener_parse(payload);
	if (root && !is_error(root)) {

		jflags = json_object_object_get(root, "flags");
		if (jflags && !is_error(jflags)) {
			std::string flags = json_object_get_string(jflags);
			WebAppMgrProxy::instance()->setJavascriptFlags( flags.c_str() );
			success = true;
		}
		json_object_put(root);
	}

	LSError err;
	LSErrorInit(&err);

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &err))
		LSErrorFree(&err);

	json_object_put(json);

	return true;
}

static bool cbSetAnimationValues(LSHandle* lsHandle, LSMessage *message,
								 void *user_data)
{
	json_object* root = 0;
	bool ret = true;

	const char* payload = LSMessageGetPayload(message);
	if (!payload)
		return false;

	root = json_tokener_parse(payload);
	if (!root || is_error(root)) {
		ret = false;
		goto Done;
	}

	{
		AnimationSettings* as = AnimationSettings::instance();
		json_object_object_foreach(root, key, val)	{

			if (json_object_is_type(val, json_type_int)) {
				ret &= as->setValue(key, json_object_get_int(val));
			}
		}
	}

Done:

	 if(root && !is_error(root))
		json_object_put(root);

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(ret));

    LSError lserror;
    LSErrorInit(&lserror);

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lserror)) {
		LSErrorFree (&lserror);
	}

	json_object_put(json);

	return true;
}

static bool cbGetAnimationValues(LSHandle* lsHandle, LSMessage *message,
								void *user_data)
{
    EMPTY_SCHEMA_RETURN(lsHandle, message);

	AnimationSettings* as = AnimationSettings::instance();
	json_object* json = json_object_new_object();

	std::map<std::string, int> animationValues = as->getAllValues();
	for (std::map<std::string, int>::const_iterator it = animationValues.begin();
		 it != animationValues.end(); ++it) {
		json_object_object_add(json, (char*) (*it).first.c_str(),
							   json_object_new_int((*it).second));
	}
	json_object_object_add(json, "returnValue", json_object_new_boolean(true));

	LSError     lsError;
	LSErrorInit(&lsError);
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree (&lsError);

	json_object_put(json);

	return true;
}

#ifdef USE_HEAP_PROFILER
static bool cbDumpHeapProfiler(LSHandle* lsHandle, LSMessage *message,
								void *user_data)
{
    EMPTY_SCHEMA_RETURN(lsHandle, message);

	HeapProfilerDump("period");

	json_object* payload = json_object_new_object();
	json_object_object_add(payload, "returnValue", json_object_new_boolean(true));

	LSError err;
	LSErrorInit(&err);
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(payload), &err))
		LSErrorFree (&err);

	json_object_put(payload);

	return true;
}
#endif // USE_HEAP_PROFILER


bool cbLockButtonTriggered(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(lsHandle, message);

	bool success = true;
	bool subscribed = false;
	LSError lsError;
	json_object* json = json_object_new_object();

	LSErrorInit(&lsError);

	if (LSMessageIsSubscription(message)) {
		success = LSSubscriptionProcess(lsHandle, message, &subscribed, &lsError);
		if (!success) {
			LSErrorFree(&lsError);
		}
	}

	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));
	json_object_object_add(json, "triggered", json_object_new_boolean(false));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError))
		LSErrorFree(&lsError);

	json_object_put(json);
        return true;
}


bool cbSubscribeToSystemUI(LSHandle* handle, LSMessage* message, void *user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(handle, message);

	bool success = true;
	bool subscribed = false;
	LSError lsError;
	json_object* response = json_object_new_object();

	LSErrorInit(&lsError);

	const char* appId = LSMessageGetApplicationID( message );
	std::string appStr;

	if(appId != NULL && strlen(appId) > 0) {
		appStr = appId;
	}
	else {
		g_warning("SysMgr::SubscribeToSystemUI - Failed to get App Id from the request. ");
		success = false;
		goto Done;
	}

	if(appStr.find("com.palm.systemui", 0) == std::string::npos) {
		success = false;
		json_object_object_add(response, "errorText", json_object_new_string("Subscription Request Denied"));
		goto Done;
	}

	if (LSMessageIsSubscription(message)) {
		success = LSSubscriptionProcess(handle, message, &subscribed, &lsError);
		if (!success) {
			LSErrorFree(&lsError);
		}
	}

	Done:

	json_object_object_add(response, "returnValue", json_object_new_boolean(success));
	json_object_object_add(response, "subscribed", json_object_new_boolean(subscribed));

	if (!LSMessageReply(handle, message, json_object_to_json_string(response), &lsError))
		LSErrorFree(&lsError);

	if(response && !is_error(response))
		json_object_put(response);

	return true;
}

bool cbPublishToSystemUI(LSHandle* handle, LSMessage* message, void *user_data)
{
	bool success = true;
	LSError lsError;
	json_object* response = json_object_new_object();
	json_object* root = NULL;
	json_object* event = NULL;
	json_object* msg = NULL;

    // {"event":"string", "message":"string"}
    VALIDATE_SCHEMA_AND_RETURN(handle,
                               message,
                               SCHEMA_2(REQUIRED(event, string), REQUIRED(message, object)));
	LSErrorInit(&lsError);

	const char* payload = LSMessageGetPayload( message );
	if(!payload) {
		success = false;
		goto Done;
	}

	root = json_tokener_parse(payload);
	if(!root || is_error(root)) {
		success = false;
		goto Done;
	}

	event = json_object_object_get(root, "event");
	if(!event || is_error(event)) {
		g_warning("SysMgr::PublishToSystemUI - event property is missing ");
		success = false;
		goto Done;
	}

	msg = json_object_object_get(root, "message");
	if(!msg || is_error(msg)) {
		g_warning("SysMgr::PublishToSystemUI - message property is missing ");
		success = false;
		goto Done;
	}
	//Posting to SystemUI client to process the message. If the event is unknown then the message will dropped at the client level.
	SystemService::instance()->postMessageToSystemUI(payload);

	Done:

		json_object_object_add(response, "returnValue", json_object_new_boolean(success));

		if (!LSMessageReply(handle, message, json_object_to_json_string(response), &lsError))
			LSErrorFree(&lsError);

		if(root && !is_error(root))
			json_object_put(root);

		if(response && !is_error(response))
			json_object_put(response);

    return true;

}

void SystemService::postMessageToSystemUI(const char* jsonStr)
{
	bool    retVal;
	LSError lsError;
	json_object* json = 0;

	LSErrorInit(&lsError);

	json = json_tokener_parse(jsonStr);
	if(!json || is_error(json)) {
		g_warning("SysMgr::postMessageToSystemUI - Message Parsing error! ");
		return;
	}

	retVal = LSSubscriptionPost(m_service, "/", "subscribeToSystemUI",
			json_object_to_json_string(json), &lsError);

	if (!retVal)
		LSErrorFree (&lsError);

	json_object_put(json);

}

bool cbMonitorProcessMemory(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
	bool success = true;
	LSError lsError;
	struct json_object* root = 0;

    // {"pid":integer, "maxMemory": integer}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_2(REQUIRED(pid, integer), REQUIRED(maxMemory, integer)));

	const char* str = LSMessageGetPayload(message);
	if (!str)
	{
		// couldn't parse the payload, or didn't get one.
		return false;
	}

	// prep the error
	LSErrorInit(&lsError);

	// init these here
	int pid = -1;
	int memMax = -1;

	// parse
	root = json_tokener_parse(str);
	if (!root || is_error(root))
	{
		// couldn't parse
		root = 0;
		success = false;
		goto Done;
	}

	// iterate through the elements and find the pid and the
	// memory maximum
	json_object_object_foreach(root, key, val)
	{
		if (strcmp(key, "pid") == 0)
		{
			if (json_object_is_type(val, json_type_int))
			{
				pid = json_object_get_int(val);
			}
			else
			{
				success = false;
				goto Done;
			}
		}
		if (strcmp(key, "maxMemory") == 0)
		{
			if (json_object_is_type(val, json_type_int))
			{
				memMax = json_object_get_int(val);
			}
			else
			{
				success = false;
				goto Done;
			}
		}
	}

	// did we get values for both required fields?
	if ( (pid==-1) || (memMax==-1) )
	{
		success = false;
		goto Done;
	}

	// it we're here, everything we needed was in the payload. Add this process to
	// the memory monitor. It's ok to send an unheard-of pid to the memory monitor.
	// if it's not there, it just doesn't do anythign about it and falls in to
	// creating the new one.
	MemoryMonitor::instance()->monitorNativeProcessMemory(pid, memMax, pid);

	// response and cleanup.
Done:
	if (root && !is_error(root))
	{
		json_object_put(root);
	}

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));

	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(json), &lsError ))
	{
		LSErrorFree (&lsError);
	}

	json_object_put(json);
	return true;
}

bool cbLogTouchEvents(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
    // {"enable":true}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(enable, boolean)));

	const char* str = LSMessageGetPayload(message);
	if (!str)
		return false;

	struct json_object* root = json_tokener_parse(str);
	struct json_object* label = 0;
	bool failed = true;

	if (root && !is_error(root)){
        // iterate through the elements
		json_object_object_foreach(root, key, val)
		{
			if (strcmp(key, "enable") == 0)
			{
				label = json_object_object_get(root, "enable");

				if (label && json_object_is_type(label, json_type_boolean))
				{
					SingleClickGestureRecognizer::g_logSingleClick = json_object_get_boolean(label);
					failed = false;
				}
			}
		}
	}
	
	json_object* reply = json_object_new_object();
	json_object_object_add(reply, "returnValue", json_object_new_boolean(!failed));

	LSError err;
	LSErrorInit(&err);
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &err))
		LSErrorFree(&err);

	json_object_put(reply);

	if (root && !is_error(root))
		json_object_put(root);

	return true;

}

bool cbEnableFpsCounter(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
    // {"enable":boolean} or {"reset":integer} or {"dump":boolean}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_3(OPTIONAL(enable, boolean), OPTIONAL(reset, integer), OPTIONAL(dump, boolean)));

	const char* str = LSMessageGetPayload(message);
	if (!str)
		return false;

	struct json_object* root = json_tokener_parse(str);
	struct json_object* label = 0;
	bool failed = true;
	int reset = 0;

	if (!root || is_error(root))
		goto Done;


	// iterate through the elements
	json_object_object_foreach(root, key, val)
	{
		if (strcmp(key, "enable") == 0)
		{
			label = json_object_object_get(root, "enable");

			if (label && json_object_is_type(label, json_type_boolean))
			{
				WindowServer::enableFpsCounter(json_object_get_boolean(label));
				failed = false;
			}
		}
		if (strcmp(key, "reset") == 0)
		{
			if (json_object_is_type(val, json_type_int))
			{
				reset = json_object_get_int(val);
				WindowServer::resetFpsBuffer(reset);
				failed = false;
			}
		}
		if (strcmp(key, "dump") == 0)
		{
			WindowServer::dumpFpsHistory();
			failed = false;
		}
	}
	
Done:

	json_object* reply = json_object_new_object();
	json_object_object_add(reply, "returnValue", json_object_new_boolean(!failed));

	LSError err;
	LSErrorInit(&err);
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &err))
		LSErrorFree(&err);	

	json_object_put(reply);

	if (!root && !is_error(root))
		json_object_put(root);

	return true;
}

bool cbEnableTouchPlot(LSHandle* lsHandle, LSMessage *message, void *user_data)
{
    // {"collection":true} or {"trails":true} or {"crosshairs":false}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_3(OPTIONAL(collection, boolean), OPTIONAL(trails, boolean), OPTIONAL(crosshairs, boolean)));

	static struct touchPlotOptions_t {
		TouchPlot::TouchPlotOption_t type;
		const char *name;
	} touchPlotOptions[] = {
		{TouchPlot::TouchPlotOption_Collection, "collection"},
		{TouchPlot::TouchPlotOption_Trails, "trails"},
		{TouchPlot::TouchPlotOption_Crosshairs, "crosshairs"},
	};

	const char* str = LSMessageGetPayload(message);
	if (!str)
		return false;

	struct json_object* root = json_tokener_parse(str);
	struct json_object* label = 0;
	bool failed = true;
	int reset = 0;

	if (!root || is_error(root))
		goto Done;

	// iterate through the elements
	json_object_object_foreach(root, key, val)
	{
		for(uint i = 0; i < sizeof(touchPlotOptions)/sizeof(touchPlotOptions[0]); i++) {
			if (strcmp(key, touchPlotOptions[i].name) == 0)
			{
				label = json_object_object_get(root, touchPlotOptions[i].name);

				if (label && json_object_is_type(label, json_type_boolean))
				{
					WindowServer::enableTouchPlotOption(touchPlotOptions[i].type, json_object_get_boolean(label));
					failed = false;
				}
			}
		}
	}

Done:

	json_object* reply = json_object_new_object();
	json_object_object_add(reply, "returnValue", json_object_new_boolean(!failed));

	LSError err;
	LSErrorInit(&err);
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &err))
		LSErrorFree(&err);

	json_object_put(reply);

	if (!root && !is_error(root))
		json_object_put(root);

	return true;
}


static inline std::string unsafeJsonToStr(const pbnjson::JValue& value)
{
	std::string result;
	pbnjson::JGenerator generator;
	if (!generator.toString(value, pbnjson::JSchemaFragment("{}"), result))
		return "";
	return result;
}

bool cbSetBenchmarkFlags(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
	const char* str = LSMessageGetPayload(message);
	if (!str)
		return false;

	int errCode = -1;
	std::string errMsg;
	bool hasRequest = false;

	pbnjson::JSchemaFragment objSchema = pbnjson::JSchemaFragment("{\"type\": \"object\"}");

	pbnjson::JValue ipcSchemaInfo = pbnjson::Object();
	ipcSchemaInfo.put("type", "object");
	ipcSchemaInfo.put("properties", pbnjson::Object());
	ipcSchemaInfo.put("additionalProperties", false);
	ipcSchemaInfo["properties"].put("animationFps", pbnjson::Object());
	ipcSchemaInfo["properties"].put("vsync", pbnjson::Object());
	ipcSchemaInfo["properties"].put("cardLoadingAnimation", pbnjson::Object());
	ipcSchemaInfo["properties"]["animationFps"].put("type", "number");
	ipcSchemaInfo["properties"]["animationFps"].put("description", "The FPS to drive Qt animations at");
	ipcSchemaInfo["properties"]["animationFps"].put("optional", true);
	ipcSchemaInfo["properties"]["vsync"].put("type", "boolean");
	ipcSchemaInfo["properties"]["vsync"].put("description", "If set to false then disable vsync.  Otherwise drive vsync normally");
	ipcSchemaInfo["properties"]["vsync"].put("optional", true);
	ipcSchemaInfo["properties"]["cardLoadingAnimation"].put("type", "boolean");
	ipcSchemaInfo["properties"]["cardLoadingAnimation"].put("description", "Turn On/Off the Card Loading Animation");
	ipcSchemaInfo["properties"]["cardLoadingAnimation"].put("optional", true);		


	std::string ipcSchemaStr = unsafeJsonToStr(ipcSchemaInfo);
	Q_ASSERT(!ipcSchemaStr.empty());
	pbnjson::JSchemaFragment ipcSchema(ipcSchemaStr);

	pbnjson::JValue request;
	pbnjson::JDomParser parser;
	if (!parser.parse(str, ipcSchema)) {
		errCode = -1;
		errMsg = "Malformed message";
		goto Done;
	}

	errCode = 0;

	request = parser.getDom();

	if (request.hasKey("animationFps")) {
		double animationFps;
		request["animationFps"].asNumber(animationFps);

		int animInterval;
		if (animationFps <= 0)
			animInterval = 1000 / 60;
		else
			animInterval = std::max(1, (int)(1000 / animationFps));

		g_debug("Changing animation interval to %d", animInterval);

		QAbstractAnimation::setAnimationTimerInterval(animInterval);

		hasRequest = true;
	}

	if (request.hasKey("vsync")) {
		bool allowVsync;
        request["vsync"].asBool(allowVsync);

		DisplayManager::instance()->forceVsyncOff(!allowVsync);

		if (allowVsync)
			g_debug("regular vsync mode");
		else
			g_debug("vsync turned off");

		hasRequest = true;
	}

	if (request.hasKey("cardLoadingAnimation")) {
		bool cardLoadingAnimation;
		request["cardLoadingAnimation"].asBool(cardLoadingAnimation);

		if (cardLoadingAnimation)
			g_message("Card loading animation on");
		else
			g_message("Card loading animation off");

		SystemService::instance()->setShowCardLoadingAnimation(cardLoadingAnimation);

		hasRequest = true;
	}

	if (!hasRequest) {
		errCode = -1;
		errMsg = "No request. Must match schema: " + ipcSchemaStr;
	}

Done:
	std::string reply;
	pbnjson::JValue replyObj = pbnjson::Object();
	replyObj.put("returnValue", pbnjson::JValue(errCode == 0));
	if (!errMsg.empty()) {
		replyObj.put("errorCode", errCode);
		replyObj.put("errorText", errMsg);
	}

	pbnjson::JGenerator generator;
	if (!generator.toString(replyObj, objSchema, reply))
		return false;

	LSError error;
	LSErrorInit(&error);
	if (!LSMessageReply(lsHandle, message, reply.c_str(), &error)) {
		LSErrorFree(&error);
	}

	return true;
}

void constructSystemStatusPayload(pbnjson::JValue& obj)
{
    pbnjson::JValue imeObj = pbnjson::Object();
    imeObj.put("visible", IMEController::instance()->isIMEOpened());
    obj.put("ime", imeObj);

    // report the ui orientation of the device
    pbnjson::JValue orientationObj = pbnjson::Object();
    switch (WindowServer::instance()->getUiOrientation()) {
    case OrientationEvent::Orientation_Up:     orientationObj.put("ui", "up"); break;
    case OrientationEvent::Orientation_Down:   orientationObj.put("ui", "down"); break;
    case OrientationEvent::Orientation_Left:   orientationObj.put("ui", "left"); break;
    case OrientationEvent::Orientation_Right:  orientationObj.put("ui", "right"); break;
    case OrientationEvent::Orientation_FaceUp: orientationObj.put("ui", "faceup"); break;
    case OrientationEvent::Orientation_FaceDown: orientationObj.put("ui", "facedown"); break;
    default: orientationObj.put("ui", "unknown"); break;
    }
    // report the device orientation
    switch (WindowServer::instance()->getOrientation()) {
    case OrientationEvent::Orientation_Up:     orientationObj.put("device", "up"); break;
    case OrientationEvent::Orientation_Down:   orientationObj.put("device", "down"); break;
    case OrientationEvent::Orientation_Left:   orientationObj.put("device", "left"); break;
    case OrientationEvent::Orientation_Right:  orientationObj.put("device", "right"); break;
    case OrientationEvent::Orientation_FaceUp: orientationObj.put("device", "faceup"); break;
    case OrientationEvent::Orientation_FaceDown: orientationObj.put("device", "facedown"); break;
    default: orientationObj.put("device", "unknown"); break;
    }
    obj.put("orientation", orientationObj);
}

bool cbGetSystemStatus(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(lsHandle, message);

    bool subscribed = false;
    const char* str = LSMessageGetPayload(message);
    if (!str)
        return false;

    LSError error;
    LSErrorInit(&error);

    if (LSMessageIsSubscription(message)) {
        if (!LSSubscriptionProcess(lsHandle, message, &subscribed, &error))
            LSErrorFree(&error);
    }

    pbnjson::JValue replyObj = pbnjson::Object();

    constructSystemStatusPayload(replyObj);

    replyObj.put("subscribed", subscribed);
    replyObj.put("returnValue", true);

    std::string replyStr;
    pbnjson::JGenerator generator;
    generator.toString(replyObj, pbnjson::JSchemaFragment("{}"), replyStr);

    if (!LSMessageReply(lsHandle, message, replyStr.c_str(), &error)) {
        LSErrorFree(&error);
    }

    return true;
}

void SystemService::postSystemStatus()
{
    LSError lsError;
    LSErrorInit(&lsError);
    
    pbnjson::JValue replyObj = pbnjson::Object();

    constructSystemStatusPayload(replyObj);

    std::string replyStr;
    pbnjson::JGenerator generator;
    generator.toString(replyObj, pbnjson::JSchemaFragment("{}"), replyStr);
    if (!LSSubscriptionPost(m_service, "/", "getSystemStatus", replyStr.c_str(), &lsError))
        LSErrorFree (&lsError);
}

bool cbDismissModalApp(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
    // {"subscribe":true, "modalId": "dlg"}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_2(REQUIRED(subscribe, boolean), REQUIRED(modalId, string)));

	bool subscribed = true;
	bool success = false;
	std::string errMsg, modalId, retValue = "";
	json_object *root = NULL;
	SystemUiController* uiController = NULL;

	LSError lserror;
	LSErrorInit(&lserror);

	const char* messageStr = LSMessageGetPayload(message);
	if (!messageStr)
	{
		errMsg = "Error getting payload";
		goto done;
	}

	if (!LSMessageIsSubscription(message))
	{
		errMsg = "Missing parameter: subscribe";
		goto done;
	}

	// Check if we actully have a modal window active.
	if(false == SystemService::instance()->isModalActive())
	{
		errMsg = "No modal window is active";
		goto done;
	}

	// Get a handle to SystemUiController
	uiController = SystemUiController::instance();
	if(!uiController)
	{
		errMsg = "Unable to get SystemUiController instance";
		goto done;
	}

	// Check to see if SystemUiController thinks it has an active modal window.
	if(false == uiController->isModalWindowActive())
	{
		errMsg = "No modal window is active";
		// reset the fact that we have a modal window active.
		SystemService::instance()->resetModalDialogInfo();
		goto done;
	}

	// Get the json string
	root = json_tokener_parse( messageStr );
	if (!root || is_error(root))
	{
		errMsg = "Malformed JSON detected in payload";
		goto done;
	}

	modalId = SystemService::instance()->getStrFromJSON(root, "modalId");
	if(0 >= modalId.length()) {
		errMsg = "Modal Id to be dismissed needs to be specified";
		goto done;
	}

	if(0 != modalId.compare(SystemService::instance()->getModalWindowSubscriptionId())) {
		errMsg = "Modal Id to be dismissed is incorrect";
		goto done;
	}

	// Set the return value to be posted back to the app.
	retValue = SystemService::instance()->getStrFromJSON(root, "returnMessage");
	if(retValue.length() > 0) {
		SystemService::instance()->setReturnValueToPost(retValue);
	}

	// Add a subscription to the service for this modal window.cbLaunch
	if (!LSSubscriptionAdd (lsHandle, SystemService::instance()->getModalWindowSubscriptionId().c_str(), message, &lserror))
	{
		subscribed = false;
		errMsg = "could not setup subscription";
		goto done;
	}

	// Set the appropriate return message
	errMsg = "Initiating removal of active modal window";
	success = true;

done:
	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if(false == success)
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));
	else
		json_object_object_add(json, "dismissResult", json_object_new_string(errMsg.c_str()));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));

	if (!LSMessageReply( lsHandle, message, json_object_to_json_string(json), &lserror ))
	{
		LSErrorFree (&lserror);
	}

	json_object_put(json);

	if(true == success) {
		// Emit the signal to dismiss the modal dialog..
		SystemService::instance()->notifyDismissModalDialog();

		// Start the timer that listens for the window to be actually deleted.
		SystemService::instance()->setActiveModalBeingRemoved();
	}

	return true;
}

bool cbLaunchModalApp(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
	bool subscribed = true;
	bool returnError = true;
	std::string errMsg, modalId, launchId, callerId, params, parentAppId;
	SystemUiController* uiController = NULL;
	Window* maximizedCardWindow = NULL;
	ApplicationDescription* appDesc = NULL;
	json_object *root, *label, *launchParams = NULL;
	ApplicationDescription* desc = NULL;
	int launchErr = SystemUiController::MalformedRequest;
	CardWindow* cWin = NULL;

	LSError lserror;
	LSErrorInit(&lserror);

    // {"subscribe": true, "callerId":"1234", "launchId": 547, "params":{"p1 p2"}}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_4(REQUIRED(subscribe, boolean), REQUIRED(callerId, string), REQUIRED(launchId, string), REQUIRED(params, object)));

	const char* messageStr = LSMessageGetPayload(message);
	if (!messageStr)
	{
		errMsg = "Unable to get payload";
		goto done;
	}

	if (!LSMessageIsSubscription(message))
	{
		errMsg = "Missing parameter: subscribe";
		goto done;
	}

	root = json_tokener_parse( messageStr );
	if (!root || is_error(root))
	{
		errMsg = "Malformed JSON detected in payload";
		goto done;
	}

	// Ensure that there are no active modal windows.
	if(true == SystemService::instance()->isModalActive())
	{
		launchErr = SystemUiController::AnotherModalActive;
		errMsg = "Another modal window is already active";
		goto done;
	}

	uiController = SystemUiController::instance();
	if(!uiController)
	{
		launchErr = SystemUiController::InternalError;
		errMsg = "Unable to get SystemUiController instance";
		goto done;
	}

	// Check to see if the window with callerId is indeed the currently maximized card window.
	maximizedCardWindow = uiController->maximizedCardWindow();
	if(!maximizedCardWindow)
	{
		launchErr = SystemUiController::NoMaximizedCard;
		errMsg = "Unable to get maximized window from system";
		goto done;
	}

	// Get the caller Id
	callerId = SystemService::instance()->getStrFromJSON(root, "callerId");
	if ( callerId.length() == 0 )
	{
		errMsg = "Missing parameter: callerId";
		goto done;
	}

	// Get the maximized card window
	cWin = static_cast<CardWindow*>(maximizedCardWindow);

	if(cWin) {
		// Get the maximized card window's appDescription
		appDesc = cWin->appDescription();
		// Check if the maximized card is PDK
		if(cWin->isHost()) {
			SystemService::instance()->setParentAppPdk(true);
		}
	}
	else {
		launchErr = SystemUiController::InternalError;
		errMsg = "Unable to get maximized card window from system";
		goto done;
	}

	// Get the string we need to compare the AppId's
	parentAppId = (NULL != appDesc)?appDesc->id():cWin->appId();

	if(0 != parentAppId.compare(callerId)) {
		launchErr = SystemUiController::ParentDifferent;
		errMsg = "Caller Id Not the same as currently active window";
		goto done;
	}

	// Get the app to be launched
	launchId = SystemService::instance()->getStrFromJSON(root, "launchId");
	if ( launchId.length() == 0 )
	{
		errMsg = "Missing parameter: launchId";
		goto done;
	}

	// Check if this app actually exists/is ready/and isnt headless.
	if((NULL == (desc = s_AppManInstance->getAppById(launchId))))
	{
		launchErr = SystemUiController::AppToLaunchDoesntExist;
		errMsg = "App to launch doesnt exist";
		goto done;
	}
	else {
		if(ApplicationDescription::Status_Ready != desc->status()) {
			launchErr = SystemUiController::AppToLaunchIsntReady;
			errMsg = "App to launch isnt in ready state";
			goto done;
		}
		/*else {
			if(false == desc->isHeadLess()) {
				errMsg = "App to launch is not headless";
				goto done;
			}
		}*/
	}

	label = json_object_object_get(root, "params");
	if (!label || is_error(label)) label = NULL;

	if ( label != NULL )
	{
		// there are params
		if (json_object_is_type(label, json_type_object))
		{
			// the params are a json formatted object
			params = json_object_to_json_string( label );
			if ( params.length() == 0 )
			{
				// not sure what happened there, but back to the default params
				params = "{}";
			}
		}
		else
		{
			// this we don't allow. If they send params at all, it
			// has to be json formatted.
			errMsg = "params must be a valid JSON, or not included at all";
			goto done;
		}
	}
	else
	{
		// no params at all. That's fine. We make our own.
		params = "{}";
	}

	// Build the modal Id for this.
	SystemService::instance()->buildSubscriptionModalId(callerId, launchId);

	// build the launch params
	launchParams = SystemService::instance()->buildParamsForAppLaunch(params, launchId, returnError, errMsg);
	if(!launchParams || !returnError)
		goto done;

	// Add a subscription to the service for this modal window.cbLaunch
	if (!LSSubscriptionAdd (lsHandle, SystemService::instance()->getModalWindowSubscriptionId().c_str(), message, &lserror))
	{
		launchErr = SystemUiController::InternalError;
		subscribed = false;
		errMsg = "could not setup subscription";
		goto done;
	}

	// We can now launch the app.
	returnError = SystemService::instance()->initiateAppLaunch(lsHandle, message, callerId, json_object_to_json_string(launchParams), user_data, SystemService::instance()->getModalWindowSubscriptionId().c_str(), desc->isHeadLess());

	// If the return value is true, save the caller and the launched app Ids
	if(returnError)
		SystemService::instance()->saveLauncherAndCallerInfo(callerId, launchId);

	return returnError;

done:

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(!returnError));
	json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));
	json_object_object_add(json, "subscribed", json_object_new_boolean(subscribed));
	json_object_object_add(json, "errorCode", json_object_new_int(launchErr));

	if (!LSMessageReply( lsHandle, message, json_object_to_json_string(json), &lserror ))
	{
		LSErrorFree (&lserror);
	}

	json_object_put(json);

	if(launchParams)
		json_object_put(launchParams);

	return true;
}

bool SystemService::initiateAppLaunch(LSHandle* lshandle, LSMessage *message, std::string& callerApp, const char *messageStr, void *user_data, const char *modalId, bool isHeadless)
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
	int launchErr = SystemUiController::MalformedRequest;

    // {"id": "5879", "params":{p1 p2}}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_2(REQUIRED(id, string), REQUIRED(params, object)));

	if (!messageStr)
	{
		errMsg = "No payload provided";
		goto Done;
	}

	root = json_tokener_parse( messageStr );
	if (!root || is_error(root)) {
		errMsg = "Malformed JSON detected in payload";
		goto Done;
	}

	label = json_object_object_get(root,"id");
	if (!label) {
		errMsg = "Unable to process command. Provide a valid \"id\" field";
		goto Done;
	}

	id = json_object_get_string(label);
	if (id.length() == 0) {
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
		//HACK: add it to the root so that it'll be deleted automatically later
		json_object_object_add(root,"params",label);
		params = json_object_to_json_string( label );
	}

	if (caller) {
		splitWindowIdentifierToAppAndProcessId(caller, callerAppId, callerProcessId);
	}
	else {
		// Get the name of the AppId.
		callerAppId = callerApp;

		//Get the processId of the running process.
		callerProcessId = SystemUiController::instance()->maximizedCardWindow()->processId();
	}

	// Set the flag that we are creating a modal child window
	SystemService::instance()->setActiveModalBeingLaunched();
	processId = WebAppMgrProxy::instance()->appLaunchModal(id, params, callerAppId, callerProcessId, errMsg, isHeadless, SystemService::instance()->isParentPdk());
	success = !processId.empty();

	// Set the message that we are ready to launch the modal window.
	if(success)
		errMsg = "Modal window launch initiated";
	else
		errMsg = "Error initiating modal window launch";

	Done:

	if( root && !is_error(root) )
		json_object_put( root );

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));
	if (success)
	{
		json_object_object_add(json, "launchResult", json_object_new_string(errMsg.c_str()));
		if ( modalId != NULL )
		{
			json_object_object_add(json, "modalId", json_object_new_string(modalId));
		}
	}
	else {
		json_object_object_add(json, "errorText", json_object_new_string(errMsg.c_str()));
		json_object_object_add(json, "errorCode", json_object_new_int(launchErr));
	}

	json_object_object_add(json, "subscribed", json_object_new_boolean(true));

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lserror ))
		LSErrorFree (&lserror);

	json_object_put(json);

	return true;
}

bool SystemService::msmAvailCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	return SystemService::instance()->msmAvail(message);
}

bool SystemService::msmProgressCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	return SystemService::instance()->msmProgress(message);
}

bool SystemService::msmEntryCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
    return SystemService::instance()->msmEntry(message);
}

bool SystemService::msmFsckingCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
    return SystemService::instance()->msmFscking(message);
}

bool SystemService::msmPartitionAvailCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
    return SystemService::instance()->msmPartitionAvail(message);
}

bool SystemService::msmAvail(LSMessage* message)
{
    // {"mode-avail":boolean}
    VALIDATE_SCHEMA_AND_RETURN(0,
                               message,
                               SCHEMA_1(REQUIRED(mode-avail, boolean)));

	const char* str = LSMessageGetPayload( message );
	if( !str )
		return false;

	struct json_object* payload = json_tokener_parse(str);
	if (is_error(payload))
		return false;

	struct json_object* modeAvail = json_object_object_get(payload, "mode-avail");
	if (!modeAvail) {
		json_object_put(payload);
		return false;
	}

	g_debug("MSM available: %s", json_object_get_boolean(modeAvail) ? "true" : "false");

	DisplayManager* dm = WindowServer::instance()->displayManager();

	if (json_object_get_boolean(modeAvail) == FALSE) {

		if (m_brickMode) {

			// did we have an unclean shutdown of brick mode
			if (!m_msmExitClean) {
				g_warning("%s: user unplugged phone without ejecting", __PRETTY_FUNCTION__);

				// FIXME: Should pop up an error message;
			}

			if (m_brickMode) {
				g_debug("%s: exiting brick mode", __PRETTY_FUNCTION__);
				dm->popDNAST("brickmode-local");
				Q_EMIT signalExitBrickMode();
				m_brickMode = false;
			}

			if (m_fscking) {
				g_debug("%s: fsck ended", __PRETTY_FUNCTION__);
				WindowServer::instance()->stopProgressAnimation();
				m_fscking = false;
			}
		}
	}

	json_object_put(payload);

	return true;
}

bool SystemService::msmProgress(LSMessage* message)
{
    // {"stage": string, "enterIMasq":boolean}
    VALIDATE_SCHEMA_AND_RETURN(0,
                               message,
                               SCHEMA_2(REQUIRED(stage, string), REQUIRED(enterIMasq, boolean)));

	const char* str = LSMessageGetPayload( message );
	if( !str )
		return false;

	struct json_object* payload = json_tokener_parse(str);
	if (is_error(payload))
		return false;

	struct json_object* stage = json_object_object_get(payload, "stage");
	if (!stage ) {
		json_object_put(payload);
		return false;
	}

	char* stageText = json_object_get_string( stage );
	g_debug("MSM Progress: %s", stageText);

	DisplayManager* dm = WindowServer::instance()->displayManager();

	if (strcasecmp(stageText, "attempting") == 0) {
		// going into brick mode

		if (!m_brickMode) {

			// Go into brick mode
			g_debug("%s: entering brick mode", __PRETTY_FUNCTION__);
			dm->pushDNAST("brickmode-local");

			// are we going into media sync or USB drive mode?
			struct json_object* mode = json_object_object_get(payload, "enterIMasq");
			Q_EMIT signalEnterBrickMode(mode && json_object_get_boolean(mode));
			Q_EMIT signalMediaPartitionAvailable(false);

			m_brickMode = true;
			m_msmExitClean = false;
		}
	}
	else if (strcasecmp(stageText, "failed") == 0) {

		// failed going into brick mode.
		if (m_brickMode) {

			g_debug("%s: exiting brick mode", __PRETTY_FUNCTION__);
			dm->popDNAST("brickmode-local");
			Q_EMIT signalExitBrickMode();
			Q_EMIT signalBrickModeFailed();

			m_brickMode = false;
		}

		if (m_fscking) {
			g_debug("%s: fsck ended", __PRETTY_FUNCTION__);
			WindowServer::instance()->stopProgressAnimation();
			m_fscking = false;
		}
	}

	//json_object_put( stage );
	json_object_put( payload );

	return true;
}

bool SystemService::msmEntry(LSMessage* message)
{
    // {"new-mode": string, "enterIMasq": boolean}
    VALIDATE_SCHEMA_AND_RETURN(0,
                               message,
                               SCHEMA_2(REQUIRED(new-mode, string), REQUIRED(enterIMasq, boolean)));

	const char* str = LSMessageGetPayload( message );
	if( !str )
		return false;

	struct json_object* payload = json_tokener_parse(str);
	if (is_error(payload))
		return false;

	struct json_object* mode = json_object_object_get(payload, "new-mode");
	if (!mode ) {
		json_object_put(payload);
		return true;
	}

	char* modeText = json_object_get_string(mode);
	g_debug("MSM Mode: %s", modeText);

	DisplayManager* dm = WindowServer::instance()->displayManager();

	if (strcasecmp(modeText, "phone") == 0) {

		m_msmExitClean = true;

		if (m_brickMode) {
			g_debug("%s: exiting brick mode", __PRETTY_FUNCTION__);
			dm->popDNAST("brickmode-local");
			Q_EMIT signalExitBrickMode();

			m_brickMode = false;
		}

		if (m_fscking) {
			g_debug("%s: fsck ended", __PRETTY_FUNCTION__);
			WindowServer::instance()->stopProgressAnimation();
			m_fscking = false;
		}
	}
	else if (strcasecmp(modeText, "brick") == 0) {

		m_msmExitClean = false;

		if (!m_brickMode) {

			g_debug("%s: entering brick mode", __PRETTY_FUNCTION__);
			dm->pushDNAST("brickmode-local");

			// are we going into media sync or USB drive mode?
			mode = json_object_object_get(payload, "enterIMasq");
			Q_EMIT signalEnterBrickMode(mode && json_object_get_boolean(mode));
			Q_EMIT signalMediaPartitionAvailable(false);

			m_brickMode = true;
		}
	}

	json_object_put(payload );

	return true;
}

bool SystemService::msmFscking(LSMessage* message)
{
	g_warning("%s: received fsck signal from storaged", __PRETTY_FUNCTION__);

	// something bad has happened and storaged is now erasing
	if (!m_brickMode)
		return false;

	m_msmExitClean = false;
	m_fscking = true;

	WindowServer::instance()->startProgressAnimation(ProgressAnimation::TypeFsck);

	return true;
}

bool SystemService::msmPartitionAvail(LSMessage* message)
{
	g_warning("%s: received partition avail callback from storaged", __PRETTY_FUNCTION__);

	Q_EMIT signalMediaPartitionAvailable(true);

	return true;
}

void SystemService::vibrate(const char* soundClass)
{
	bool result;
    LSError lsError;
    LSErrorInit(&lsError);

	bool isAlert = soundClass && ((strcasestr(soundClass, "alerts") != 0) ||
								  (strcasestr(soundClass, "alarm")  != 0) ||
								  (strcasestr(soundClass, "calendar")  != 0));
	json_object* json = json_object_new_object();
	json_object_object_add(json, "name", json_object_new_string(isAlert ? "alert" : "notification"));

	result = LSCall(m_service, "palm://com.palm.vibrate/vibrateNamedEffect",
					json_object_to_json_string(json), NULL, NULL, NULL, &lsError);
	if (!result) {
		g_warning("%s: Failed in vibrateNamedEffect: %s", __PRETTY_FUNCTION__, lsError.message);
		LSErrorFree(&lsError);
	}

	json_object_put(json);
}

bool SystemService::telephonyServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
    // {"serviceName":"com.palm.telephony", "connected": true}
    VALIDATE_SCHEMA_AND_RETURN(handle,
                               message,
                               SCHEMA_2(REQUIRED(serviceName, string), REQUIRED(connected, boolean)));

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (json && !is_error(json))  {

		json_object* label = json_object_object_get(json, "connected");
		if (label && json_object_is_type(label, json_type_boolean)) {

			bool connected = json_object_get_boolean(label);

			if (connected) {

				// telephoney service up.
				bool result;
				LSError lsError;
				LSErrorInit(&lsError);

				result = LSCall(handle, "palm://com.palm.telephony/subscribe", "{\"events\":\"macrocalls\"}",
								telephonyEventsCallback, NULL, NULL, &lsError);
				if (!result) {
					g_warning("Failed in calling telephony service: %s", lsError.message);
					LSErrorFree(&lsError);
				}
			}
		}

		json_object_put(json);
	}

	return true;
}

bool SystemService::telephonyEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
    // {"macroEventCalls":{"event":string}}
    VALIDATE_SCHEMA_AND_RETURN(handle,
                               message,
                               OBJECT_REQUIRED_1(macroEventCalls, event, string));

	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;
	const char* eventName = 0;

	label = json_object_object_get(root, "macroEventCalls");
	if (!label || is_error(label) || !json_object_is_type(label, json_type_object))
		goto Done;

	label = json_object_object_get(label, "event");
	if (!label || is_error(label) || !json_object_is_type(label, json_type_string))
		goto Done;

	eventName = json_object_get_string(label);
	if (eventName && (strcmp(eventName, "incoming") == 0)) {

		g_warning("%s: Incoming call detected", __PRETTY_FUNCTION__);
		Q_EMIT SystemService::instance()->signalIncomingPhoneCall();
	}

Done:

	if (root && !is_error(root))
		json_object_put(root);

	return true;
}

void SystemService::postNovacomStatus()
{
	bool result;
    LSError lsError;
    LSErrorInit(&lsError);

	bool novacomEnabled = false;
	struct stat stBuf;

	if (::stat("/var/gadget/novacom_enabled", &stBuf) == 0)
		novacomEnabled = true;

	result = LSCall(m_service,
					"palm://com.palm.accountservices/updateDeviceProperties",
					novacomEnabled ?
					"{\"novacomEnabled\":\"true\"}" :
					"{\"novacomEnabled\":\"false\"}",
					NULL, NULL, NULL, &lsError);
	if (!result) {
		g_warning("Failed to call palm://com.palm.accountservices/updateDeviceProperties: %s",
				  lsError.message);
		LSErrorFree(&lsError);
	}
}

extern "C" void __attribute__((weak)) qt_dumpAllRasters(const char* path);

static bool cbDumpRasters(LSHandle* lshandle, LSMessage *message,
							   void *userData)
{
    LSError lsError;
    LSErrorInit(&lsError);

    // {"path":string}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_1(REQUIRED(path, string)));

	const char* str = LSMessageGetPayload(message);
    if( !str )
		return false;

	bool success = false;

	struct json_object* root = json_tokener_parse(str);
	struct json_object* label = 0;
	std::string filePath;

	if (!root || is_error(root)) {
		root = 0;
		success = false;
		goto Done;
	}

	label = json_object_object_get(root, "path");
	if (!label)
		goto Done;

	filePath = json_object_get_string(label);
	if (filePath.empty())
		goto Done;

	if (qt_dumpAllRasters) {
		qt_dumpAllRasters(filePath.c_str());
		success = true;
	}

Done:

	if (root)
		json_object_put(root);

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lsError )) {
		LSErrorFree (&lsError);
	}

	json_object_put(json);

	return true;
}

static bool cbDumpJemallocHeap(LSHandle* lshandle, LSMessage *message,
							   void *userData)
{
/*    LSError lsError;
    LSErrorInit(&lsError);

	const char* str = LSMessageGetPayload(message);
    if( !str )
		return false;

	bool success = false;

	struct json_object* root = json_tokener_parse(str);
	struct json_object* label = 0;
	std::string filePath;
	void (*je_dumpHeap)(const char*);
	void* dlHandle = 0;
	char* dlError = 0;


	if (!root || is_error(root)) {
		root = 0;
		success = false;
		goto Done;
	}

	label = json_object_object_get(root, "path");
	if (!label)
		goto Done;

	filePath = json_object_get_string(label);
	if (filePath.empty())
		goto Done;

	dlHandle = dlopen(0, RTLD_LAZY);
	if (!dlHandle) {
		printf("Failed to dlopen 0??\n");
		goto Done;
	}

	dlerror();
	*(void**) (&je_dumpHeap) = dlsym(dlHandle, "je_dumpHeap");         
	if ((dlError = dlerror()) != 0) {
		printf("dl error: %s\n", dlError);
		dlclose((dlHandle));
		goto Done;
	}

	(*je_dumpHeap)(filePath.c_str());
	dlclose(dlHandle);
	success = true;

Done:

	if (root)
		json_object_put(root);

	json_object* json = json_object_new_object();
	json_object_object_add(json, "returnValue", json_object_new_boolean(success));

	if (!LSMessageReply( lshandle, message, json_object_to_json_string(json), &lsError )) {
		LSErrorFree (&lsError);
	}

	json_object_put(json);
*/
	return true;
}

bool SystemService::touchToShareCanTapStatusCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	return SystemService::instance()->touchToShareCanTapStatus(handle, message, ctxt);    
}

bool SystemService::touchToShareCanTapStatus(LSHandle* handle, LSMessage* message, void* ctxt)
{
    // {"canTap" : boolean}
    VALIDATE_SCHEMA_AND_RETURN(handle,
                               message,
                               SCHEMA_1(REQUIRED(canTap, boolean)));

	const char* str = LSMessageGetPayload(message);
	if (!str)
		return true;

	struct json_object* payload = json_tokener_parse(str);
	if (!payload || is_error(payload))
		return true;

	struct json_object* canTap = json_object_object_get(payload, "canTap");
	if (!canTap || !json_object_is_type(canTap, json_type_boolean)) {
		json_object_put(payload);
		return true;
	}

	Q_EMIT signalTouchToShareCanTap(json_object_get_boolean(canTap));

	json_object_put(payload);

	return true;    
}

static bool cbTouchToShareDeviceInRange(LSHandle* lsHandle, LSMessage* message,
									   void* user_data)
{
    // {"inRange" : boolean}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(inRange, boolean)));

	const char* str = LSMessageGetPayload(message);
	if (!str)
		return true;

	struct json_object* payload = json_tokener_parse(str);
	if (!payload || is_error(payload))
		return true;

	struct json_object* canTap = json_object_object_get(payload, "inRange");
	if (!canTap || !json_object_is_type(canTap, json_type_boolean)) {
		json_object_put(payload);
		return true;
	}

	SystemService::instance()->notifyTouchToShareCanTap(json_object_get_boolean(canTap));

	json_object_put(payload);

	json_object* reply = json_object_new_object();
	json_object_object_add(reply, "returnValue", json_object_new_boolean(true));

	LSError lsError;
	LSErrorInit(&lsError);
	
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lsError))
		LSErrorFree (&lsError);

	json_object_put(reply);

	return true;	    
}

static bool cbTouchToShareAppUrlTransferred(LSHandle* lsHandle, LSMessage* message,
											void* user_data)
{
    // {"appid" : string}
    VALIDATE_SCHEMA_AND_RETURN(lsHandle,
                               message,
                               SCHEMA_1(REQUIRED(appid, string)));

	const char* str = LSMessageGetPayload(message);
	if (!str)
		return true;

	struct json_object* payload = json_tokener_parse(str);
	if (!payload || is_error(payload))
		return true;

	struct json_object* label = json_object_object_get(payload, "appid");
	if (!label || !json_object_is_type(label, json_type_string)) {
		json_object_put(payload);
		return true;
	}

	SystemService::instance()->notifyTouchToShareAppUrlTransfered(json_object_get_string(label));

	json_object_put(payload);

	json_object* reply = json_object_new_object();
	json_object_object_add(reply, "returnValue", json_object_new_boolean(true));

	LSError lsError;
	LSErrorInit(&lsError);
	
	if (!LSMessageReply(lsHandle, message, json_object_to_json_string(reply), &lsError))
		LSErrorFree (&lsError);

	json_object_put(reply);

	return true;	    
}

static std::set<LSMessage *> sTurboModeSubscriptions;
static bool cbSubscribeTurboMode(LSHandle* lshandle, LSMessage *message, void *user_data)
{
    SUBSCRIBE_SCHEMA_RETURN(lshandle, message);

	bool success = true;
	bool subscribed = false;
	LSError lsError;
	json_object* response = json_object_new_object();

	LSErrorInit(&lsError);

	if (LSMessageIsSubscription(message)) {
		success = LSSubscriptionProcess(lshandle, message, &subscribed, &lsError);
		if (success) {
			if (subscribed) {
				LSMessageRef(message);
				HostBase::instance()->turboModeSubscription(true);
				sTurboModeSubscriptions.insert(message);
			}
		}
		else {
			LSErrorFree(&lsError);
		}
	}

	json_object_object_add(response, "returnValue", json_object_new_boolean(success));
	json_object_object_add(response, "subscribed", json_object_new_boolean(subscribed));

	if (!LSMessageReply(lshandle, message, json_object_to_json_string(response), &lsError))
		LSErrorFree(&lsError);

	if(response && !is_error(response))
		json_object_put(response);

	return true;
}

static bool cbSubscriptionCancel(LSHandle *lshandle, LSMessage *message, void *user_data)
{
	if (sTurboModeSubscriptions.erase(message) == 1) {
		HostBase::instance()->turboModeSubscription(false);
		LSMessageUnref(message);
	}

	return true;
}
