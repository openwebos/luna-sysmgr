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




#ifndef SYSTEMSERVICE_H
#define SYSTEMSERVICE_H

#include "Common.h"

#include <string>

#include <lunaservice.h>
#include <QTimer>
#include <QObject>

struct json_object;

class SystemService : public QObject
{
	Q_OBJECT

public:

	typedef struct __ActiveModalDialogInfo {
		bool m_isModalAppBeingLaunched;
		bool m_isModalAppLaunched;
		bool m_isModalAppBeingRemoved;
		std::string m_activeDialogApp;
		std::string m_activeDialogCaller;
		std::string m_returnValueToPost;
	} ActiveModalDialogInfo;

	static SystemService* instance();

	static void buildSubscriptionModalId(std::string& caller, std::string& launchApp);
	static void resetModalDialogInfo();
	static void setActiveModalBeingLaunched();
	static void setActiveModalBeingRemoved();
	static void setActiveModalInfo();
	static bool isModalActive();
	static std::string getStrFromJSON(json_object *root, const char *id);
	static std::string getModalWindowSubscriptionId();
	static std::string getModalDismissReturnValue();
	static bool initiateAppLaunch(LSHandle* lshandle, LSMessage *message, std::string& callerId, const char *messageStr, void *user_data, const char *modalId, bool isHeadless);
	static json_object* buildParamsForAppLaunch(std::string params, std::string& launchId, bool& success, std::string& errMsg);
	static void saveLauncherAndCallerInfo(std::string& caller, std::string& launchedApp);
	static void setReturnValueToPost(const std::string& retValue);
	static bool isParentPdk();
	static void setParentAppPdk(bool isSdk);

	~SystemService();

	void init();

	bool brickMode() const { return m_brickMode; }
	LSHandle* serviceHandle() const { return m_service; }

	void postForegroundApplicationChange(const std::string& name,const std::string& menuname, const std::string& id);
	void postApplicationHasBeenTerminated(const std::string& title, const std::string& menuname, const std::string& id);
	
	void postLockStatus(bool locked);
	void postDeviceLockMode();
	void postLockButtonTriggered();

	void shutdownDevice();

	void enterMSM();

	void vibrate(const char* soundClass);

	void postLaunchModalResult(bool timedOut = false);
	void postDismissModalResult(bool timedOut = false);
	void postAppRestoredNeeded();
	
	void postMessageToSystemUI(const char* jsonStr);

    void postSystemStatus();
	
	void notifyDeviceUnlocked() { Q_EMIT signalDeviceUnlocked(); }
	void notifyCancelPinEntry() { Q_EMIT signalCancelPinEntry(); }
	void notifyTouchToShareCanTap(bool val) { Q_EMIT signalTouchToShareCanTap(val); }
	void notifyTouchToShareAppUrlTransfered(const std::string& str) { Q_EMIT signalTouchToShareAppUrlTransfered(str); }
	void notifyDismissModalDialog() {Q_EMIT signalDismissModalDialog(); }
	void notifyDismissModalTimerStopped() {Q_EMIT signalStopModalDismissTimer(); }

	bool showCardLoadingAnimation() const { return m_cardLoadingAnimation; }
	void setShowCardLoadingAnimation(bool val) { m_cardLoadingAnimation = val; }

private Q_SLOTS:

	void postBootFinished();
	void postDockModeStatus(bool enabled);
	void slotModalWindowAdded();
	void slotModalWindowRemoved();
	void slotModalDialogTimerFired();

Q_SIGNALS:

	void signalIncomingPhoneCall();
	
	void signalEnterBrickMode(bool mediaSync);
	void signalExitBrickMode();
	void signalBrickModeFailed();

	void signalMediaPartitionAvailable(bool);
	
	void signalDeviceUnlocked();
	void signalCancelPinEntry();

	void signalTouchToShareCanTap(bool);
	void signalTouchToShareAppUrlTransfered(const std::string&);
	void signalDismissModalDialog();
	void signalStopModalDismissTimer();

private:

	SystemService();

	void startService();
	void stopService();

	static bool msmAvailCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool msmProgressCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool msmEntryCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool msmFsckingCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool msmPartitionAvailCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool telephonyServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool telephonyEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool touchToShareCanTapStatusCallback(LSHandle* handle, LSMessage* message, void* ctxt);

	static void initModalTimerInfo();

	bool msmAvail(LSMessage* message);
	bool msmProgress(LSMessage* message);
	bool msmEntry(LSMessage* message);
	bool msmFscking(LSMessage* message);
	bool msmPartitionAvail(LSMessage* message);
	bool touchToShareCanTapStatus(LSHandle* handle, LSMessage* message, void* ctxt);
	void postNovacomStatus();

private:
	LSHandle* m_service;
	LSMessageToken m_storageDaemonToken;
	bool m_brickMode;
	bool m_msmExitClean;
	bool m_fscking;
	bool m_cardLoadingAnimation;
	static ActiveModalDialogInfo sActiveModalInfo;
	static std::string sTempCaller;
	static std::string sTempLaunchApp;
	static std::string sModalWindowSubscriptionId;
	static QTimer sModalLauchCheckTimer;
	static int sModalWindowIndex;
	static bool sIsParentPdkApp;
};


	
#endif /* SYSTEMSERVICE_H */
