/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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




#include "StatusBar.h"
#include "StatusBarServicesConnector.h"
#include "cjson/json.h"
#include "HostBase.h"
#include "Localization.h"
#include "Preferences.h"
#include "DeviceInfo.h"
#include "BtDeviceClass.h"
#include "SystemService.h"
#include <string.h>
#include <map>

#define AP_MODE_PHONE      1
#define AP_MODE_WIFI       2
#define AP_MODE_BLUETOOTH  3


static StatusBarServicesConnector* s_instance = 0;

const char* StatusBarServicesConnector::m_chargeSource[StatusBarServicesConnector::kNumChargeSources] = {"usb", "inductive"};

StatusBarServicesConnector::StatusBarServicesConnector()
{
	unsigned int codes[] = {2,3,6};
	m_SIMRejectCodes = std::vector<unsigned int>(codes, codes + sizeof(codes) / sizeof(int));

	// all profiles
	m_bluetoothProfiles.resize(7);
	m_bluetoothProfiles[0] = "hfg";
	m_bluetoothProfiles[1] = "a2dp";
	m_bluetoothProfiles[2] = "pan";
	m_bluetoothProfiles[3] = "hid";
	m_bluetoothProfiles[4] = "spp";
	m_bluetoothProfiles[5] = "hf";
	m_bluetoothProfiles[6] = "mapc";

	// profiles for Device Menu
	m_bluetoothMenuProfiles.clear();
	m_bluetoothMenuProfiles.insert("hfg");
	m_bluetoothMenuProfiles.insert("a2dp");
	m_bluetoothMenuProfiles.insert("hf");
	m_bluetoothMenuProfiles.insert("mapc");

	m_bluetoothProfileStates.clear();

	m_powerdConnected = false;
	m_batteryLevel = 0;
	m_charging = false;

	//Flags to hold the SIM Status
	m_simBad    = false;
	m_simLocked = false;
	m_ruim      = false;

	m_callFwdStatusRequested = false;

	//Flag to hold the limited service status
	m_phoneInLimitedService = false;
	m_phoneRadioState = false;
	m_phoneService = NoService;

	m_airplaneMode = false;
	m_initialAirplaneModeStatus = true;
	m_airplaneModeTriggered = false;
	m_apModePhone = false;
	m_apModeWifi = false;
	m_apModeBluetooth = false;
	m_msmStartingRadiosInProgress = false;
	m_msmModePhone = false;
	m_msmModeWifi = false;
	m_msmModeBluetooth = false;

	m_phoneType = PHONE_TYPE_UNKNOWN;
	m_demoBuild = false;

	m_rssi = 0;

	sprintf(m_carrierText, "Palm");

	m_showBlankStatusOnLimited = false;
	m_bluetoothRadionOn = false;
	m_btRadioTurningOn = false;
	m_wifiSSID.clear();
	m_wifiRadioOn = false;
	m_wifiConnected = false;
	m_wifiFindNetworksReq = 0;
	m_bluetoothDeviceListReq = 0;

	m_vpnConnected = false;
	m_connectedVpnInfo.clear();
	m_pendingVpnProfile.clear();
	m_phoneEventNetworkPayload.clear();

	m_hideDataIcon = false;
	m_isInternetConnectionAvailable = false;

	m_cmPayloadBuffer.clear();

	init();
}

StatusBarServicesConnector* StatusBarServicesConnector::instance()
{
	if(s_instance)
		return s_instance;

	s_instance = new StatusBarServicesConnector();
	return s_instance;
}


StatusBarServicesConnector::~StatusBarServicesConnector()
{

}

void StatusBarServicesConnector::init()
{
	LSError lserror;
	LSErrorInit(&lserror);

	bool ret = LSRegister(NULL, &m_service, &lserror);
	if (!ret) goto error;

	ret = LSGmainAttach(m_service, HostBase::instance()->mainLoop(), &lserror);
	if (!ret) goto error;


	// Register for powerd service coming on bus
	ret = LSCall(m_service,
				 "palm://com.palm.bus/signal/registerServerStatus",
				 "{\"serviceName\":\"com.palm.power\"}",
				 statusBarPowerdServiceUpCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Register for battery status updates
	ret = LSCall(m_service,
				 "palm://com.palm.bus/signal/addmatch",
				 "{\"category\":\"/com/palm/power\",\"method\":\"batteryStatus\"}",
				 statusBarPowerdBatteryEventsCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Register for charger status updates
	ret = LSCall(m_service,
				 "palm://com.palm.bus/signal/addmatch",
				 "{\"category\":\"/com/palm/power\",\"method\":\"USBDockStatus\"}",
				 statusBarPowerdChargerEventsCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Register for telephony service coming on bus
	ret = LSCall(m_service,
			  	 "palm://com.palm.lunabus/signal/registerServerStatus",
				 "{\"serviceName\":\"com.palm.telephony\"}",
				 statusBarTelephonyServiceUpCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Register for BT Monitor service coming on bus
	ret = LSCall(m_service,
			  	 "palm://com.palm.lunabus/signal/registerServerStatus",
				 "{\"serviceName\":\"com.palm.btmonitor\"}",
				 statusBarBtMonitorServiceUpCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Register for Bluetooth service coming on bus
	ret = LSCall(m_service,
			  	 "palm://com.palm.lunabus/signal/registerServerStatus",
				 "{\"serviceName\":\"com.palm.bluetooth\"}",
				 statusBarBluetoothServiceUpCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Register for WAN service coming on bus
	ret = LSCall(m_service,
			  	 "palm://com.palm.lunabus/signal/registerServerStatus",
				 "{\"serviceName\":\"com.palm.wan\"}",
				 statusBarWanServiceUpCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Register for ConnectionManager service coming on bus
	ret = LSCall(m_service,
			  	 "palm://com.palm.lunabus/signal/registerServerStatus",
				 "{\"serviceName\":\"com.palm.connectionmanager\"}",
				 statusBarConnMgrServiceUpCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Register for WiFi service coming on bus
	if(DeviceInfo::instance()->wifiAvailable()) {
		ret = LSCall(m_service,
					 "palm://com.palm.lunabus/signal/registerServerStatus",
					 "{\"serviceName\":\"com.palm.wifi\"}",
					 statusBarWiFiServiceUpCallback, NULL, NULL, &lserror);
		if (!ret) goto error;
	}

	// Register for VPN service coming on bus
	ret = LSCall(m_service,
				 "palm://com.palm.lunabus/signal/registerServerStatus",
				 "{\"serviceName\":\"com.palm.vpn\"}",
				 statusBarVpnServiceUpCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Query the build name
	ret = LSCall(m_service,
			  	 "palm://com.palm.preferences/systemProperties/Get",
				 "{\"key\":\"com.palm.properties.buildName\"}",
				 statusBarGetBuildNameCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Query the initial battery status
	ret = LSCall(m_service,
			  	 "palm://com.palm.power/com/palm/power/batteryStatusQuery",
				 "{ }",
				 statusBarPowerdBatteryEventsCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Query the initial charger status
	ret = LSCall(m_service,
			  	 "palm://com.palm.power/com/palm/power/chargerStatusQuery",
				 "{ }",
				 statusBarPowerdChargerEventsCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	// Query the initial charger status
	ret = LSCall(m_service,
			     "palm://com.palm.systemservice/time/getSystemTime",
				 "{\"subscribe\":true}",
				 statusBarGetSystemTimeCallback, NULL, NULL, &lserror);
	if (!ret) goto error;

	connect(Preferences::instance(),SIGNAL(signalAirplaneModeChanged(bool)), SLOT(slotAirplaneModeChanged(bool)));
	connect(Preferences::instance(),SIGNAL(signalRoamingIndicatorChanged()), SLOT(slotRoamingIndicatorChanged()));

	connect(SystemService::instance(), SIGNAL(signalEnterBrickMode(bool)),
			SLOT(slotEnterBrickMode(bool)));
	connect(SystemService::instance(), SIGNAL(signalExitBrickMode()),
			SLOT(slotExitBrickMode()));

error:
	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}
}

void StatusBarServicesConnector::setDemoBuild(bool demo)
{
	m_demoBuild = true;
	sprintf(m_carrierText, "Palm");
	Q_EMIT signalCarrierTextChanged(m_carrierText);

	m_rssi = 5;
	updateRSSIIcon(true, (StatusBar::IndexRSSI)(StatusBar::RSSI_0 + m_rssi));
}

void StatusBarServicesConnector::slotAirplaneModeChanged(bool enabled)
{
	bool prev = m_airplaneMode;
	m_airplaneMode = enabled;

	if(!m_initialAirplaneModeStatus) {
		// Airplane Mode was changed via the Device Menu, so turn the radios ON/OFF.
		setAirplaneMode(m_airplaneMode);
	} else {
		m_initialAirplaneModeStatus = false;

		if(m_phoneRadioState == false) {
			if(!prev && m_airplaneMode) {
				// System Prefs response came after we received the Radio Power state, so update the icon to reflect
				// the fact that we are in Airplane Mode (at boot time)
				// Note that we have a separate icon for airplane mode, so we will ask the icon to
				// disable itself
				sprintf(m_carrierText, "%s", LOCALIZED("Airplane Mode").c_str());
				updateRSSIIcon(false, StatusBar::RSSI_FLIGHT_MODE);
				Q_EMIT signalCarrierTextChanged(m_carrierText);
			}
		}

		// update the system menu
		if(m_airplaneMode)
			Q_EMIT signalAirplaneModeState(AirplaneModeOn);
		else
			Q_EMIT signalAirplaneModeState(AirplaneModeOff);
	}
}

void StatusBarServicesConnector::setAirplaneMode(bool on)
{
	m_airplaneModeTriggered = true;
	m_apModePhone = false;
	m_apModeWifi = false;
	m_apModeBluetooth = false;

	if(on) {
		// Turn ON airplane mode
		g_message("StatusBar - Enabling Airplane Mode");
		Q_EMIT signalAirplaneModeState(AirplaneModeTurningOn);

		Preferences::instance()->saveWifiState(m_wifiRadioOn);
		Preferences::instance()->saveBluetoothState(m_bluetoothRadionOn);

		//Turn off Wifi BT and Phone
		if (m_bluetoothRadionOn || m_btRadioTurningOn) {
			g_message("StatusBar - BT is on. Turning it off");
			setBluetoothOnState(false);
		}
		else {
			g_message("StatusBar - BT is off.");
			updateAirplaneModeProgress(AP_MODE_BLUETOOTH);
		}

		if(PHONE_TYPE_UNKNOWN != m_phoneType && PHONE_TYPE_NONE != m_phoneType) {
			g_message("StatusBar - Phone is on. Turning it off");
			setTelephonyPowerState(false, true);
		}
		else {
			//For WiFi only devices, modem type is NONE. Update the Airplane Mode progess so that it won't be stuck waiting for the radio status.
			g_message("StatusBar - Phone Type is None.");
			updateAirplaneModeProgress(AP_MODE_PHONE);
		}

		if (m_wifiRadioOn) {
			g_message("StatusBar - WiFi is on. Turning it off");
			setWifiOnState(false);
		}
		else {
			g_message("StatusBar - WiFi is off");
			updateAirplaneModeProgress(AP_MODE_WIFI);
		}
	} else {
		// Turn OFF airplane mode
		g_message("StatusBar - Disabling Airplane Mode");
		Q_EMIT signalAirplaneModeState(AirplaneModeTurningOff);

		if(PHONE_TYPE_UNKNOWN != m_phoneType && PHONE_TYPE_NONE != m_phoneType) {
			g_message("StatusBar UI - Turning on phone radio");
			setTelephonyPowerState(true, true);
		}

		//If Radio was turned on while in Airplane Mode, update the airplane progress
		if(m_phoneRadioState) {
			updateAirplaneModeProgress(AP_MODE_PHONE);
		}

		//Turn on Wifi Bt
		if(Preferences::instance()->wifiState()) {
			// Wifi was ON before, so turn it back on
			g_message("StatusBar - Turning on WiFi");
			setWifiOnState(true);
		} else {
			updateAirplaneModeProgress(AP_MODE_WIFI);

		}

		if(Preferences::instance()->bluetoothState()) {
			// Bluetooth was ON before, so turn it back on
			g_message("StatusBar - Turning on BT");
			setBluetoothOnState(true);
		} else {
			updateAirplaneModeProgress(AP_MODE_BLUETOOTH);
		}

	}
}

void StatusBarServicesConnector::setRadioStatesForMSMMode(bool on)
{
	if(on) {
		// Turn ON radios for MSM mode
		g_message("StatusBar - Leaving MSM mode, Turning radion ON");

		if(m_msmStartingRadiosInProgress)
			return;
		m_msmStartingRadiosInProgress = true;

		if(PHONE_TYPE_UNKNOWN != m_phoneType && PHONE_TYPE_NONE != m_phoneType) {
			if(!m_airplaneMode) {
				if(m_msmModePhone) {
					g_message("StatusBar UI - Turning on phone radio");
					setTelephonyPowerState(true, false, true);
				}
			}
		}

		if(m_msmModeBluetooth) {
			g_message("StatusBar - Turning on BT");
			setBluetoothOnState(true);
		}

		if(m_msmModeWifi) {
			g_message("StatusBar - Turning on WiFi");
			setWifiOnState(true);
		}

		m_msmModeBluetooth = false;
		m_msmModeWifi = false;
		m_msmModePhone = false;
	} else {
		g_message("StatusBar - Entering MSM mode, Turning radios OFF");

		m_msmStartingRadiosInProgress = false;

		if(PHONE_TYPE_UNKNOWN != m_phoneType && PHONE_TYPE_NONE != m_phoneType) {
			if(!m_airplaneMode) {
				if(m_phoneRadioState) {
					m_msmModePhone = true;
					g_message("StatusBar - Phone is on. Turning it off");
					setTelephonyPowerState(false, false, true);
				}
				else {
					m_msmModePhone = false;
				}
			}
		}

		if (m_bluetoothRadionOn || m_btRadioTurningOn) {
			m_msmModeBluetooth = true;
			m_bluetoothRadionOn = false;
			g_message("StatusBar - BT is on. Turning it off");
			setBluetoothOnState(false);
		}
		else {
			m_msmModeBluetooth = false;
		}

		if (m_wifiRadioOn) {
			m_msmModeWifi = true;
			g_message("StatusBar - WiFi is on. Turning it off");
			setWifiOnState(false);
		}
		else {
			m_msmModeWifi = false;
		}

		//Clear all VPN state as well.
		Q_EMIT signalVpnStateChanged(false);
	}
}

void StatusBarServicesConnector::updateAirplaneModeProgress(int radio)
{
	bool done = false;

	if(!m_airplaneModeTriggered)
		return;

	g_message("StatusBar - Updating Airplane Mode Progress: %d", radio);

	if(radio == AP_MODE_PHONE) {
		m_apModePhone = true;
	} else if(radio == AP_MODE_WIFI) {
		m_apModeWifi = true;
	} else if(radio == AP_MODE_BLUETOOTH) {
		m_apModeBluetooth = true;
	}

	if (m_phoneType != PHONE_TYPE_NONE &&
		m_phoneType != PHONE_TYPE_UNKNOWN)
		done = m_apModePhone && m_apModeWifi && m_apModeBluetooth;
	else
		done = m_apModeWifi && m_apModeBluetooth;

	if (done) {
		g_message("StatusBar - Enable / Disable Airplane Mode complete - Updating Device Menu");

		m_airplaneModeTriggered = false;

		if(m_airplaneMode)
			Q_EMIT signalAirplaneModeState(AirplaneModeOn);
		else
			Q_EMIT signalAirplaneModeState(AirplaneModeOff);
	}

}

void StatusBarServicesConnector::slotRoamingIndicatorChanged()
{
	if(Preferences::instance()->roamingIndicator() == "none") {
		m_showBlankStatusOnLimited = true;
	}
}

bool StatusBarServicesConnector::statusBarGetBuildNameCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;

	g_message("StatusBar - statusBarGetBuildNameCallback %s", LSMessageGetPayload(message));

	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if(json_object_get_boolean(label)) {
			const char* string = 0;

			label = json_object_object_get(root, "com.palm.properties.buildName");
			if (label && !is_error(label) && json_object_is_type(label, json_type_string))
			{
				string = json_object_get_string(label);
				if(string) {
					if(!strcasecmp(string, "nova-demo")) {
						s_instance->setDemoBuild(true);
					}
				}
			}
		}
	}

	if (root && !is_error(root))
		json_object_put(root);

	return true;
}

bool StatusBarServicesConnector::statusBarPowerdServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->powerdServiceUpCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::powerdServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - powerdServiceUpCallback %s", LSMessageGetPayload(message));

	struct json_object* connectedObj = NULL;

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (json && !is_error(json)) {
		if (!json_object_object_get_ex(json, "connected", &connectedObj)) {
			g_critical("Unable to connected obj");
			goto exit;
		}

		bool origState = m_powerdConnected;
		m_powerdConnected = json_object_get_boolean(connectedObj);

		if(!m_powerdConnected) {
			Q_EMIT signalBatteryLevelUpdated(-1);
		}

		if (origState != m_powerdConnected) {
			Q_EMIT signalPowerdConnectionStateChanged(m_powerdConnected);
		}
	}

exit:
	if (json && !is_error(json)) json_object_put(json);
	return true;
}

bool StatusBarServicesConnector::statusBarPowerdBatteryEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->powerdBatteryEventsCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::powerdBatteryEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - powerdBatteryEventsCallback %s", LSMessageGetPayload(message));

	// Look for percent_ui
	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* percentObj = NULL;

	if (json && !is_error(json)) {
		if (!json_object_object_get_ex(json, "percent_ui", &percentObj)) {
			g_critical("Unable to get percent_ui for battery state");
			goto exit;
		}

		m_batteryLevel = json_object_get_int(percentObj);

		Q_EMIT signalBatteryLevelUpdated(m_batteryLevel);
	}

exit:
	if (json && !is_error(json)) json_object_put(json);
	return true;
}

bool StatusBarServicesConnector::statusBarPowerdChargerEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->powerdChargerEventsCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::powerdChargerEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - powerdChargerEventsCallback %s", LSMessageGetPayload(message));

	struct json_object* chargingObj = NULL;
	bool origState = m_charging;

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (json && !is_error(json)) {
		if (!json_object_object_get_ex(json, "Charging", &chargingObj)) {
			g_critical("Charging field is missing!");
			goto exit;
		}

		// Default to not charging unless we find out otherwise
		m_charging = json_object_get_boolean(chargingObj);

		// Check if we're charging -- usb or inductive
		/*for (int j = 0; j < kNumChargeSources; j++) {
			if (strcmp(json_object_get_string(typeObj), m_chargeSource[j]) == 0) {
				if (json_object_object_get_ex(json, "connected", &connectedObj)) {
					if (json_object_get_boolean(connectedObj)) {
						m_charging = true;
					}
				}
			}
		}*/

		if (m_charging != origState) {
			Q_EMIT signalChargingStateUpdated(m_charging);
		}
	}

exit:
	if (json && !is_error(json)) json_object_put(json);
	return true;
}

bool StatusBarServicesConnector::statusBarPlatformQueryCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->platformQueryCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::platformQueryCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;

	g_message("StatusBar - platformQueryCallback %s", LSMessageGetPayload(message));

	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if(json_object_get_boolean(label)) {
			const char* string = 0;

			label = json_object_object_get(root, "extended");
			if (label && !is_error(label) && json_object_is_type(label, json_type_object))
			{
				struct json_object* ext = label;
				label = json_object_object_get(ext, "platformType");
				if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
					const char* type = json_object_get_string(label);
					if(type) {
						if(!strcmp(type, "gsm")) {
							m_phoneType = PHONE_TYPE_GSM;
						}else if(!strcmp(type, "cdma")) {
							m_phoneType = PHONE_TYPE_CDMA;
						}else if(!strcmp(type, "none")) {
							m_phoneType = PHONE_TYPE_NONE;
						}

						Q_EMIT signalPhoneTypeUpdated();
					}
				}

				label = json_object_object_get(ext, "capabilities");
				if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {
					label = json_object_object_get(label, "ruim");
					if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
						m_ruim = json_object_get_boolean(label);
					}
				}

			}
		}
	}

	if (root && !is_error(root))
		json_object_put(root);

	if((m_phoneType != PHONE_TYPE_UNKNOWN) && !m_signalMsgPayloadBuffer.empty()) {
		telephonySignalEventsCallback(NULL, m_signalMsgPayloadBuffer.c_str(), NULL);
		m_signalMsgPayloadBuffer.clear();
	}

	return true;
}


void StatusBarServicesConnector::updateRSSIIcon(bool show, StatusBar::IndexRSSI index)
{
	if(!Preferences::instance()->useDualRSSI()) {
		Q_EMIT signalRssiIndexChanged(show, index);
	} else {
		if((!show) || (index == StatusBar::RSSI_FLIGHT_MODE) || (index == StatusBar::RSSI_ERROR)) {
			Q_EMIT signalRssiIndexChanged(show, index);
			Q_EMIT signalRssi1xIndexChanged(false, StatusBar::RSSI_1X_0);
		} else if ((index == StatusBar::RSSI_EV_0) || (index == StatusBar::RSSI_0)) {
			Q_EMIT signalRssiIndexChanged(show, StatusBar::RSSI_EV_0);
			Q_EMIT signalRssi1xIndexChanged(show, StatusBar::RSSI_1X_0);
		} else {
			Q_EMIT signalRssiIndexChanged(show, index);
		}
	}
}

void StatusBarServicesConnector::updateRSSI1xIcon(bool show, StatusBar::IndexRSSI1x index)
{
	Q_EMIT signalRssi1xIndexChanged(show, index);
}


bool StatusBarServicesConnector::statusBarTelephonyServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->telephonyServiceUpCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::telephonyServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - telephonyServiceUpCallback %s", LSMessageGetPayload(message));

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (json && !is_error(json))  {

		json_object* label = json_object_object_get(json, "connected");
		if (label && json_object_is_type(label, json_type_boolean)) {

			bool connected = json_object_get_boolean(label);

			if (connected) {
				// telephony service up.

				//Flag to hold the SIM Status
				m_simBad = false;
				m_simLocked = false;

				//Flag to hold the limited service status
				m_phoneInLimitedService = false;

				//callForwardNotificationSession = false

				bool result;
				LSError lsError;
				LSErrorInit(&lsError);

				// Get the Phone Type
				result = LSCall(handle, "palm://com.palm.telephony/platformQuery", "{ }",
						statusBarPlatformQueryCallback, NULL, NULL, &lsError);
				if (!result) goto error;

				// Get the Initial Power Radio State and subscribe to Power events
				result = LSCall(handle, "palm://com.palm.telephony/powerQuery", "{\"subscribe\":true}",
						statusBarTelephonyPowerEventsCallback, NULL, NULL, &lsError);
				if (!result) goto error;

				// Subscribe for Telephony Signal notifications
				result = LSCall(handle, "palm://com.palm.telephony/subscribe", "{\"events\":\"signal\"}",
						statusBarTelephonySignalEventsCallback, NULL, NULL, &lsError);
				if (!result) goto error;

				// Subscribe for Telephony Network notifications
				result = LSCall(handle, "palm://com.palm.telephony/subscribe", "{\"events\":\"network\"}",
						statusBarTelephonyNetworkEventsCallback, NULL, NULL, &lsError);
				if (!result) goto error;

				// Get the Initial Values for SIM Status
				result = LSCall(m_service, "palm://com.palm.telephony/simStatusQuery", "{\"subscribe\": true}",
						statusBarTelephonySIMEventsCallback, NULL, NULL, &lsError);
				if (!result) goto error;

				// Get the Initial Values for TTY Status
				result = LSCall(m_service, "palm://com.palm.telephony/ttyQuery", "{\"subscribe\": true}",
						statusBarTelephonyTTYEventsCallback, NULL, NULL, &lsError);
				if (!result) goto error;

				// Get the Initial Values for HAC Status
				result = LSCall(m_service, "palm://com.palm.telephony/hacQuery", "{\"subscribe\": true}",
						statusBarTelephonyHACEventsCallback, NULL, NULL, &lsError);
				if (!result) goto error;

				m_callFwdStatusRequested = false;

error:
				if (LSErrorIsSet(&lsError)) {
					LSErrorPrint(&lsError, stderr);
					LSErrorFree(&lsError);
				}
			} else {
				// TIL disconnected
				handleTILDisconnected();
			}
		}

		json_object_put(json);
	}

	return true;
}

void StatusBarServicesConnector::handleTILDisconnected()
{
	g_warning("StatusBar - Telephony Service not available on the bus!");
	// simulate a power off event
	handlePowerStatus("off", false);
}

void StatusBarServicesConnector::requestNetworkAndSignalStatus()
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	// Request current Network Status
	result = LSCall(m_service, "palm://com.palm.telephony/networkStatusQuery", "{ }",
			statusBarTelephonyNetworkEventsCallback, NULL, NULL, &lsError);
	if (!result) goto error;

	// Request current Signal Strength status
	result = LSCall(m_service, "palm://com.palm.telephony/signalStrengthQuery", "{ }",
			statusBarTelephonySignalEventsCallback, NULL, NULL, &lsError);
	if (!result) goto error;

error:
	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}



bool StatusBarServicesConnector::statusBarTelephonyPowerEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->telephonyPowerEventsCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::telephonyPowerEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - telephonyPowerEventsCallback %s", LSMessageGetPayload(message));

	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	label = json_object_object_get(root, "eventPower");
	if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
		// Event
		const char* radioState = 0;
		radioState = json_object_get_string(label);

		if(radioState) {
			handlePowerStatus(radioState);
		}
	} else {
		// response to Query
		label = json_object_object_get(root, "returnValue");
		if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
			bool result = json_object_get_boolean(label);

			if(result) {
				label = json_object_object_get(root, "extended");
				if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {

					label = json_object_object_get(label, "powerState");
					if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
						const char* radioState = 0;
						radioState = json_object_get_string(label);

						if(radioState) {
							handlePowerStatus(radioState, true);
						}
					}
				}
			} else {

			}
		} else {
		}
	}

	if (root && !is_error(root))
		json_object_put(root);

	return true;
}

void StatusBarServicesConnector::handlePowerStatus(const char* radioState, bool queryResponse)
{
	if (strcmp(radioState, "on") == 0) {
		if(m_phoneRadioState && !queryResponse) {
			g_warning("StatusBarI - Phone Notification - Duplicate eventPower? (ON)");
			return;
		}
		m_phoneRadioState = true;
		if (m_phoneType == PHONE_TYPE_NONE) {
			sprintf(m_carrierText, "Open webOS");
		}
		else if (m_phoneType == PHONE_TYPE_GSM) {
			sprintf(m_carrierText, "%s", LOCALIZED("Network search...").c_str());
		}
		else {
			sprintf(m_carrierText, "%s", LOCALIZED("Searching...").c_str());
		}

		Q_EMIT signalCarrierTextChanged(m_carrierText);

		if(queryResponse) {
			requestNetworkAndSignalStatus();
		}

		updateRSSIIcon(true, StatusBar::RSSI_0);

		if(!m_phoneEventNetworkPayload.empty()) {
			telephonyNetworkEventsCallback(NULL, m_phoneEventNetworkPayload.c_str(), NULL);
			m_phoneEventNetworkPayload.clear();
		}
	} else {
		m_phoneRadioState = false;

			if (m_airplaneMode) {
				//Airplane mode now has its own icon in the status bar, so hide
				//the rssi icon when we go into airplane mode
				sprintf(m_carrierText, "%s", LOCALIZED("Airplane Mode").c_str());
				updateRSSIIcon(false, StatusBar::RSSI_FLIGHT_MODE);
			} else {
				sprintf(m_carrierText, "%s", LOCALIZED("Phone offline").c_str());
				updateRSSIIcon(false, StatusBar::RSSI_0);
			}

			Q_EMIT signalCarrierTextChanged(m_carrierText);


			Q_EMIT signalWanIndexChanged(false, StatusBar::WAN_OFF);
			Q_EMIT signalCallForwardStateChanged(false);
			Q_EMIT signalRoamingStateChanged(false);
				if(m_callFwdStatusRequested) {
//					this.callForwardNotificationSession.cancel(); // $$$
					m_callFwdStatusRequested = false;
				}
	}

}


bool StatusBarServicesConnector::statusBarTelephonyNetworkEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	const char* payload = LSMessageGetPayload(message);
	return s_instance->telephonyNetworkEventsCallback(handle, payload, ctxt);
}

bool StatusBarServicesConnector::telephonyNetworkEventsCallback(LSHandle* handle, const char* payload, void* ctxt)
{
	g_message("StatusBar - telephonyNetworkEventsCallback %s", payload);

	struct json_object* root = json_tokener_parse(payload);
	struct json_object* label = 0;

	label = json_object_object_get(root, "eventNetwork");
	if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {
		// Event
		if(m_phoneRadioState) {
			const char* string = 0;
			struct json_object* event = label;

			label = json_object_object_get(event, "state");
			if (label && !is_error(label) && json_object_is_type(label, json_type_string))
			{
				string = json_object_get_string(label);
				if(string) {
					handleNetworkStatus(string, event);
				}

			}
		} else {
			// didn't receive the power on notification yet, so save the message until then
			m_phoneEventNetworkPayload = payload;
		}
	} else {
		// response to Query
		label = json_object_object_get(root, "returnValue");
		if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
			bool result = json_object_get_boolean(label);

			if(result) {
				label = json_object_object_get(root, "extended");
				if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {
					struct json_object* event = label;

					label = json_object_object_get(event, "state");
					if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
						const char* state = 0;
						state = json_object_get_string(label);

						if(state) {
							handleNetworkStatus(state, event);
						}
					}
				}
			}
		}
	}

	if (root && !is_error(root))
		json_object_put(root);

	return true;
}

bool StatusBarServicesConnector::validSIMRejectCode(unsigned int code)
{
	for(unsigned int x = 0; x < m_SIMRejectCodes.size(); x++) {
		if(m_SIMRejectCodes[x] == code)
			return true;
	}
	return false;
}


void StatusBarServicesConnector::handleNetworkStatus(const char* networkState, struct json_object* event)
{
	if(!m_phoneRadioState)
		return;

	//If it is wifi only device, update the status bar with default carrier text and return.
	if (m_phoneType == PHONE_TYPE_NONE) {
		sprintf(m_carrierText, "Open webOS");
		Q_EMIT signalCarrierTextChanged(m_carrierText);
		return;
	}

	struct json_object* label = 0;
	const char* string = 0;
	const char* registration = 0;

	m_phoneInLimitedService = false;

	label = json_object_object_get(event, "registration");
	if (label && !is_error(label) && json_object_is_type(label, json_type_string))
	{
		registration = json_object_get_string(label);
	}


	if(strcmp(networkState, "service") == 0) {
		m_phoneService = Service;

		if(m_demoBuild) {
			sprintf(m_carrierText, "Palm");
		} else {
			label = json_object_object_get(event, "networkName");
			if (label && !is_error(label) && json_object_is_type(label, json_type_string))
			{
				string = json_object_get_string(label);
				if(string) {
					sprintf(m_carrierText, "%s",string);
				}
			}

			if(registration) {
				if(!strcmp(registration, "home")) {
					Q_EMIT signalRoamingStateChanged(false);
				} else if(!strcmp(registration, "roaming") || !strcmp(registration, "roamblink")){
					Q_EMIT signalRoamingStateChanged(true);
				}
			}
		}

		m_simLocked = false;

		if(!m_callFwdStatusRequested && m_phoneType == PHONE_TYPE_GSM)
			requestCallForwardStatus();
	} else if(strcmp(networkState, "noservice") == 0) {
		m_phoneService = NoService;

		if (m_simLocked) {
			sprintf(m_carrierText, "%s", LOCALIZED("SIM lock").c_str());
		} else if(registration) {
			if(!strcmp(registration, "searching")) {
				// SEARCHING
				m_phoneService = Searching;
				if (m_phoneType == PHONE_TYPE_GSM) {
					sprintf(m_carrierText, "%s", LOCALIZED("Network search...").c_str());
				}
				else {
					sprintf(m_carrierText, "%s", LOCALIZED("Searching...").c_str());
				}
			} else {
				if (m_simBad && (m_phoneType == PHONE_TYPE_GSM || m_ruim)) {
					sprintf(m_carrierText, "%s", LOCALIZED("Check SIM").c_str());
				} else {
					sprintf(m_carrierText, "%s", LOCALIZED("No service").c_str());
				}
			}
		}

		Q_EMIT signalRoamingStateChanged(false);
		Q_EMIT signalCallForwardStateChanged(false);
		if(m_callFwdStatusRequested) {
			//this.callForwardNotificationSession.cancel(); // $$$
			m_callFwdStatusRequested = false;
		}

	} else if(strcmp(networkState, "limited") == 0) {
		m_phoneService = Limited;

		if (m_simBad) {
			sprintf(m_carrierText, "%s", LOCALIZED("Check SIM-SOS only").c_str());
		} else if(m_simLocked) {
			sprintf(m_carrierText, "%s", LOCALIZED("SIM lock-SOS only").c_str());
		} else {
			sprintf(m_carrierText, "%s", LOCALIZED("SOS Only").c_str());
		}

		if (registration && !strcmp(registration, "denied")) {
			label = json_object_object_get(event, "causeCode");
			if (label && !is_error(label) && json_object_is_type(label, json_type_string))
			{
				string = json_object_get_string(label);
				if(string) {
					int cause = atoi(string);
					if(m_showBlankStatusOnLimited && cause == 0) {
						sprintf(m_carrierText, " ");
					}
				}
			}
		}
		Q_EMIT signalRoamingStateChanged(false);
		m_phoneInLimitedService = true;
		updateRSSIIcon(true, StatusBar::RSSI_0);
	} else {
		sprintf(m_carrierText, "%s", LOCALIZED("No service").c_str());
		Q_EMIT signalRoamingStateChanged(false);

	}
	Q_EMIT signalCarrierTextChanged(m_carrierText);

}



bool StatusBarServicesConnector::statusBarTelephonySignalEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	const char* payload = LSMessageGetPayload(message);
	return s_instance->telephonySignalEventsCallback(handle, payload, ctxt);
}

bool StatusBarServicesConnector::telephonySignalEventsCallback(LSHandle* handle, const char* messagePayload, void* ctxt)
{
	struct json_object* root = json_tokener_parse(messagePayload);
	struct json_object* label = 0;
	struct json_object* signalValues = 0;

	g_message("StatusBar - telephonySignalEventsCallback %s", messagePayload);

	label = json_object_object_get(root, "eventSignal");
	if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {
		// Event
		if(m_phoneRadioState) {
			const char* string = 0;
			//struct json_object* event = label;

			if(m_phoneType == PHONE_TYPE_UNKNOWN) {
				updateRSSIIcon(true, StatusBar::RSSI_0);
				if(handle != NULL || ctxt != NULL) // to avoid re-buffering the same data
					m_signalMsgPayloadBuffer = messagePayload;
				// Request the phone type
				bool result;
				LSError lsError;
				LSErrorInit(&lsError);

				result = LSCall(handle, "palm://com.palm.telephony/platformQuery", "{ }",
						statusBarPlatformQueryCallback, NULL, NULL, &lsError);

				if (LSErrorIsSet(&lsError)) {
					LSErrorPrint(&lsError, stderr);
					LSErrorFree(&lsError);
				}
			} else if(m_phoneInLimitedService) {
				updateRSSIIcon(true, StatusBar::RSSI_0);
			} else if(m_demoBuild) {
				m_rssi = 5;
				updateRSSIIcon(true, (StatusBar::IndexRSSI)(StatusBar::RSSI_0 + m_rssi));
			} else if(m_phoneType == PHONE_TYPE_GSM) {
				label = json_object_object_get(label, "bars");
				if (label && !is_error(label) && json_object_is_type(label, json_type_int))
				{
					m_rssi = json_object_get_int(label);
					if(m_rssi < 0)
						m_rssi = 0;
					updateRSSIIcon(true, (StatusBar::IndexRSSI)(StatusBar::RSSI_0 + m_rssi));
				}
			} else if(m_phoneType == PHONE_TYPE_CDMA){
				signalValues = label;
				label = json_object_object_get(signalValues, "value");
				if (label && !is_error(label) && json_object_is_type(label, json_type_int))
				{
					m_rssi = json_object_get_int(label);
					if(!Preferences::instance()->useDualRSSI()) {
						if (m_rssi >= 0) {
							if(m_rssi > 5)
								m_rssi = 5; // display at most 5 bars
							updateRSSIIcon(true, (StatusBar::IndexRSSI)(StatusBar::RSSI_0 + m_rssi));
						} else {
							updateRSSIIcon(true, StatusBar::RSSI_ERROR);
						}
					} else {
						int rssiev = 0;
						int rssi1x = 0;

						label = json_object_object_get(signalValues, "value1x");
						if (label && !is_error(label) && json_object_is_type(label, json_type_int))
						{
							rssi1x = json_object_get_int(label);
							if(rssi1x < 0) rssi1x = 0;
							if(rssi1x > 5) rssi1x = 5;
						}

						label = json_object_object_get(signalValues, "valueEvdo");
						if (label && !is_error(label) && json_object_is_type(label, json_type_int))
						{
							rssiev = json_object_get_int(label);
							if(rssiev < 0) rssiev = 0;
							if(rssiev > 5) rssiev = 5;
						}
						updateRSSIIcon(true, (StatusBar::IndexRSSI)(StatusBar::RSSI_EV_0 + rssiev));
						updateRSSI1xIcon(true, (StatusBar::IndexRSSI1x)(StatusBar::RSSI_1X_0 + rssiev));
					}
				}
			}
		}
	} else {
		// response to Query
		label = json_object_object_get(root, "returnValue");
		if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
			bool result = json_object_get_boolean(label);

			if(result) {

				if(m_phoneInLimitedService) {
					updateRSSIIcon(true, StatusBar::RSSI_0);
				} else if(m_demoBuild) {
					m_rssi = 5;
					updateRSSIIcon(true, (StatusBar::IndexRSSI)(StatusBar::RSSI_0 + m_rssi));
				} else if(m_phoneType == PHONE_TYPE_GSM) {
					label = json_object_object_get(root, "bars");
					if (label && !is_error(label) && json_object_is_type(label, json_type_int))
					{
						m_rssi = json_object_get_int(label);
						if(m_rssi < 0)
							m_rssi = 0;
						updateRSSIIcon(true, (StatusBar::IndexRSSI)(StatusBar::RSSI_0 + m_rssi));
					}
				} else if(m_phoneType == PHONE_TYPE_CDMA){
					label = json_object_object_get(root, "extended");
					if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {
						signalValues = label;
						label = json_object_object_get(signalValues, "value");
						if (label && !is_error(label) && json_object_is_type(label, json_type_int))
						{
							m_rssi = json_object_get_int(label);
							if(!Preferences::instance()->useDualRSSI()) {
								if (m_rssi >= 0) {
									if(m_rssi > 5)
										m_rssi = 5; // display at most 5 bars
									updateRSSIIcon(true, (StatusBar::IndexRSSI)(StatusBar::RSSI_0 + m_rssi));
								} else {
									updateRSSIIcon(true, StatusBar::RSSI_ERROR);
								}
							} else {
								int rssiev = 0;
								int rssi1x = 0;

								label = json_object_object_get(signalValues, "value1x");
								if (label && !is_error(label) && json_object_is_type(label, json_type_int))
								{
									rssi1x = json_object_get_int(label);
									if(rssi1x < 0) rssi1x = 0;
									if(rssi1x > 5) rssi1x = 5;
								}

								label = json_object_object_get(signalValues, "valueEvdo");
								if (label && !is_error(label) && json_object_is_type(label, json_type_int))
								{
									rssiev = json_object_get_int(label);
									if(rssiev < 0) rssiev = 0;
									if(rssiev > 5) rssiev = 5;
								}
								updateRSSIIcon(true, (StatusBar::IndexRSSI)(StatusBar::RSSI_EV_0 + rssiev));
								updateRSSI1xIcon(true, (StatusBar::IndexRSSI1x)(StatusBar::RSSI_1X_0 + rssiev));
							}
						}
					}
				}
			}
		}
	}

	if (root && !is_error(root))
		json_object_put(root);

	return true;
}


bool StatusBarServicesConnector::statusBarTelephonySIMEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->telephonySIMEventsCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::telephonySIMEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	g_message("StatusBar - handleNetworkStatusQueryNotification %s", LSMessageGetPayload(message));

	if(m_phoneRadioState) {
		label = json_object_object_get(root, "extended");
		if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {
			label = json_object_object_get(label, "state");
			if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
				char* state = json_object_get_string(label);
				if (state) {
					if (!strcmp(state, "simnotfound") || !strcmp(state, "siminvalid")) {
						m_simBad = true;
						if (m_phoneService == NoService) {
							sprintf(m_carrierText, "%s", LOCALIZED("Check SIM").c_str());
						}
						else if (m_phoneService == Limited) {
							sprintf(m_carrierText, "%s", LOCALIZED("Check SIM-SOS only").c_str());
						}
					} else if (!strcmp(state, "simready")) {
						if(m_phoneService == Limited) {
							sprintf(m_carrierText, "%s", LOCALIZED("SOS Only").c_str());
						} else if(m_phoneService == NoService) {
							sprintf(m_carrierText, "%s", LOCALIZED("No service").c_str());
						} else if(m_phoneService == Searching) {
							if (m_phoneType == PHONE_TYPE_GSM) {
								sprintf(m_carrierText, "%s", LOCALIZED("Network search...").c_str());
							} else {
								sprintf(m_carrierText, "%s", LOCALIZED("Searching...").c_str());
							}
						}
						m_simBad = false;
						m_simLocked = false;
					} else if (!strcmp(state, "pinrequired") || !strcmp(state, "pukrequired") || !strcmp(state, "pinpermblocked")) {
						m_simLocked = true;
						if (m_phoneInLimitedService) {
							sprintf(m_carrierText, "%s", LOCALIZED("Check SIM-SOS only").c_str());
						}
						else {
							sprintf(m_carrierText, "%s", LOCALIZED("SIM lock").c_str());
						}
					}

					Q_EMIT signalCarrierTextChanged(m_carrierText);
				}
			}
		}
	}

	if (root && !is_error(root))
		json_object_put(root);

	return true;
}

void StatusBarServicesConnector::setTelephonyPowerState(bool on, bool saveState, bool msmMode)
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	char params[64];
	char power[10];
	char save[10];

	sprintf(power, on ? "\"on\"" : (!msmMode ? "\"off\"" : "\"default\""));
	sprintf(save, saveState ? "true" : "false");

	sprintf(params, "{\"state\":%s,\"save\":%s}", power, save);

	result = LSCall(m_service, "palm://com.palm.telephony/powerSet", params,
			        statusBarTelephonyPowerStateChangeCallback, NULL, NULL, &lsError);

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

bool StatusBarServicesConnector::statusBarTelephonyPowerStateChangeCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->telephonyPowerStateChangeCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::telephonyPowerStateChangeCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	g_message("StatusBar - telephonyPowerStateChangeCallback %s", LSMessageGetPayload(message));

	updateAirplaneModeProgress(AP_MODE_PHONE);

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		bool success = json_object_get_boolean(label);

		if(!success)
			g_warning("StatusBar - Error Phone Radio : %s", LSMessageGetPayload(message));
	}

	if (root && !is_error(root))
		json_object_put(root);

		return true;
}

bool StatusBarServicesConnector::statusBarTelephonyTTYEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->telephonyTTYEventsCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::telephonyTTYEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	g_message("StatusBar - telephonyTTYEventsCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		bool success = json_object_get_boolean(label);
		if(!success)
			goto Done;
	}


	label = json_object_object_get(root, "extended");
	if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {
		label = json_object_object_get(label, "mode");
		if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
			char* mode = json_object_get_string(label);
			if (mode) {
				if (!strcmp(mode, "full")) {
					Q_EMIT signalTTYStateChanged(true);
				} else {
					Q_EMIT signalTTYStateChanged(false);
				}
			}
		}
	}

Done:
	if (root && !is_error(root))
		json_object_put(root);

	return true;
}

bool StatusBarServicesConnector::statusBarTelephonyHACEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->telephonyHACEventsCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::telephonyHACEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	g_message("StatusBar - telephonyHACEventsCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		bool success = json_object_get_boolean(label);
		if(!success)
			goto Done;
	}


	label = json_object_object_get(root, "extended");
	if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {
		label = json_object_object_get(label, "enabled");
		if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
			if (json_object_get_boolean(label)) {
				Q_EMIT signalHACStateChanged(true);
			} else {
				Q_EMIT signalHACStateChanged(false);
			}
		} else {
			Q_EMIT signalHACStateChanged(false);
		}
	}

Done:
	if (root && !is_error(root))
		json_object_put(root);

	return true;
}

bool StatusBarServicesConnector::statusBarWanServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->wanServiceUpCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::wanServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - wanServiceUpCallback %s", LSMessageGetPayload(message));

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (json && !is_error(json))  {

		json_object* label = json_object_object_get(json, "connected");
		if (label && json_object_is_type(label, json_type_boolean)) {

			bool connected = json_object_get_boolean(label);

			if (connected) {
				// WAN service up.
				bool result;
				LSError lsError;
				LSErrorInit(&lsError);

				//Subscribe to wan status Notifications.
				result = LSCall(handle, "palm://com.palm.wan/getstatus", "{\"subscribe\":true}",
						statusBarWanStatusEventsCallback, NULL, NULL, &lsError);
				if (!result) goto error;

error:
				if (LSErrorIsSet(&lsError)) {
					LSErrorPrint(&lsError, stderr);
					LSErrorFree(&lsError);
				}
			} else {
				// WAN disconnected
				Q_EMIT signalWanIndexChanged(false, StatusBar::WAN_OFF);
				m_cmPayloadBuffer.clear();
			}
		}

		json_object_put(json);
	}

	return true;
}

bool StatusBarServicesConnector::statusBarWanStatusEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	const char* payload = LSMessageGetPayload(message);
	return s_instance->wanStatusEventsCallback(handle, payload, ctxt);
}

bool StatusBarServicesConnector::wanStatusEventsCallback(LSHandle* handle, const char* messagePayload, void* ctxt)
{
	struct json_object* root = json_tokener_parse(messagePayload);
	struct json_object* label = 0;
	json_object* array = 0;
	const char *string = 0;

	g_message("StatusBar - wanStatusEventsCallback %s", messagePayload);

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		bool success = json_object_get_boolean(label);
		if(!success)
			goto Done;
	}


	label = json_object_object_get(root, "networkstatus");
	if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
		string = json_object_get_string(label);
		if(!strcmp(string, "attached")){
			const char *networkType = 0;
			label = json_object_object_get(root, "networktype");
			if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
				networkType = json_object_get_string(label);
			}

			array = json_object_object_get(root, "connectedservices");
			if (array && !is_error(array) && json_object_is_type(array, json_type_array) && (json_object_array_length(array) > 0)) {
				for (int i=0; i<json_object_array_length(array); i++) {
					label = json_object_array_get_idx(array, i);
					if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {
						const char *connectstatus = 0;
						json_object* serviceArray = 0;

						label = json_object_object_get(label, "connectstatus");
						if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
							connectstatus = json_object_get_string(label);
						}

						label = json_object_array_get_idx(array, i);
						serviceArray = json_object_object_get(label, "service");
						if (serviceArray && !is_error(serviceArray) && json_object_is_type(serviceArray, json_type_array) && (json_object_array_length(serviceArray) > 0)) {
							int internetIndex = -1;
							for (int s=0; s<json_object_array_length(serviceArray); s++) {
								string = json_object_get_string(json_object_array_get_idx(serviceArray, s));
								if (string && !strcmp(string, "internet")) {
									internetIndex = s;
									break;
								}
							}
							if(internetIndex != -1) {
								if(handle != NULL || ctxt != NULL) // to avoid re-buffering the same data
									m_cmPayloadBuffer = messagePayload;

								if(m_wifiConnected && m_hideDataIcon)
									goto Done;

								const char *dataaccess = 0;

								label = json_object_object_get(root, "dataaccess");
								if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
									dataaccess = json_object_get_string(label);
								}

								if(!strcmp(connectstatus, "active") && !strcmp(dataaccess, "usable")) {
									Q_EMIT signalWanIndexChanged(true, getWanIndex(true, networkType));
								}
								else if(!strcmp(connectstatus, "dormant") && !strcmp(dataaccess, "usable")) {
									Q_EMIT signalWanIndexChanged(true, getWanIndex(false, networkType));
								}
								else {
									Q_EMIT signalWanIndexChanged(false, StatusBar::WAN_OFF);
								}
								break;
							}
						}
					}
				}
			} else {
				Q_EMIT signalWanIndexChanged(false, StatusBar::WAN_OFF);
				m_cmPayloadBuffer.clear();
			}
		} else {
			Q_EMIT signalWanIndexChanged(false, StatusBar::WAN_OFF);
			m_cmPayloadBuffer.clear();
		}
	}

Done:
	if (root && !is_error(root))
		json_object_put(root);

	return true;
}

StatusBar::IndexWAN StatusBarServicesConnector::getWanIndex(bool connected, const char* type)
{
	StatusBar::IndexWAN index = StatusBar::WAN_OFF;

	if(connected) {
		if(!strcmp(type, "1x")) {
			index = StatusBar::WAN_CONNECTED_1X;
		} else if(!strcmp(type, "edge")) {
			index = StatusBar::WAN_CONNECTED_EDGE;
		} else if(!strcmp(type, "evdo")) {
			if(!Preferences::instance()->show3GForEvdo())
				index =StatusBar:: WAN_CONNECTED_EVDO;
			else
				index = StatusBar::WAN_CONNECTED_EVDO3G;
		} else if(!strcmp(type, "gprs")) {
			index = StatusBar::WAN_CONNECTED_GPRS;
		} else if(!strcmp(type, "umts")) {
			index = StatusBar::WAN_CONNECTED_UMTS;
		} else if(!strcmp(type, "hsdpa")) {
			index = StatusBar::WAN_CONNECTED_HSDPA;
		} else if(!strcmp(type, "hspa-4g")) {
			index = StatusBar::WAN_CONNECTED_HSPA_4G;
		}
	} else { // DORMANT
		if(!strcmp(type, "1x")) {
			index = StatusBar::WAN_DORMANT_1X;
		} else if(!strcmp(type, "evdo")) {
			if(!Preferences::instance()->show3GForEvdo())
				index = StatusBar::WAN_DORMANT_EVDO;
			else
				index = StatusBar::WAN_DORMANT_EVDO3G;
		}
	}

	return index;
}


bool StatusBarServicesConnector::statusBarBtMonitorServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->btMonitorServiceUpCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::btMonitorServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - btMonitorServiceUpCallback %s", LSMessageGetPayload(message));

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (json && !is_error(json))  {

		json_object* label = json_object_object_get(json, "connected");
		if (label && json_object_is_type(label, json_type_boolean)) {

			bool connected = json_object_get_boolean(label);

			if (connected) {
				// BT Monitor service up.
				bool result;
				LSError lsError;
				LSErrorInit(&lsError);

				m_btRadioTurningOn = false;
				m_bluetoothRadionOn = false;

				updateAirplaneModeProgress(AP_MODE_BLUETOOTH);

				//Subscribe to BT Monitor Notifications.
				result = LSCall(handle, "palm://com.palm.btmonitor/monitor/subscribenotifications", "{\"subscribe\":true}",
						statusBarBtMonitorEventsCallback, NULL, NULL, &lsError);
				if (!result) goto error;

error:
				if (LSErrorIsSet(&lsError)) {
					LSErrorPrint(&lsError, stderr);
					LSErrorFree(&lsError);
				}
			} else {
				// BT Monitor disconnected
				// Simulate Radio off.  Remove the Bluetooth icon from the status bar
				m_bluetoothRadionOn = false;
				Q_EMIT signalBluetoothIndexChanged(m_bluetoothRadionOn, StatusBar::BLUETOOTH_OFF);
				Q_EMIT signalBluetoothPowerStateChanged(RADIO_OFF);
				// All profiles must be disconnected
				m_bluetoothProfileStates.clear();
			}
		}

		json_object_put(json);
	}

	return true;
}

bool StatusBarServicesConnector::statusBarBtMonitorEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->btMonitorEventsCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::btMonitorEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;
	const char *string = 0;
	bool updateBtIcon = false;

	g_message("StatusBar - btMonitorEventsCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "radio");
	if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
		const char *radio = json_object_get_string(label);
		if(radio) {
			if(!strcmp(radio, "on")) {
				// For now, show that the radio is on and assume nothing is connected
				m_bluetoothRadionOn = true;
				m_btRadioTurningOn = false;
				Q_EMIT signalBluetoothIndexChanged(m_bluetoothRadionOn, StatusBar::BLUETOOTH_ON);
				Q_EMIT signalBluetoothPowerStateChanged(RADIO_ON);

				// Determine if any profiles are connected
				requestBluetoothConnectedProfilesInfo();
			} else if(!strcmp(radio, "turningon")) {
				m_btRadioTurningOn = true;
				Q_EMIT signalBluetoothPowerStateChanged(RADIO_TURNING_ON);
			} else if(!strcmp(radio, "turningoff") || !strcmp(radio, "off")) {
				m_bluetoothRadionOn = false;
				m_btRadioTurningOn = false;
				Q_EMIT signalBluetoothIndexChanged(m_bluetoothRadionOn, StatusBar::BLUETOOTH_OFF);
				Q_EMIT signalBluetoothPowerStateChanged(RADIO_OFF);
				// All profiles must be disconnected
				m_bluetoothProfileStates.clear();
			}
		}
	}

	label = json_object_object_get(root, "notification");
	if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
		const char *notif = json_object_get_string(label);
		if(notif) {
			if(!strcmp(notif, "notifnradioturningon")) {
				m_btRadioTurningOn = true;
				Q_EMIT signalBluetoothPowerStateChanged(RADIO_TURNING_ON);
			} else if(!strcmp(notif, "notifnradioon")) {
				// Radio is on, but notification can be sent even when connections are present
				m_bluetoothRadionOn = true;
				m_btRadioTurningOn = false;
				Q_EMIT signalBluetoothIndexChanged(m_bluetoothRadionOn, StatusBar::BLUETOOTH_ON);
				Q_EMIT signalBluetoothPowerStateChanged(RADIO_ON);

				updateAirplaneModeProgress(AP_MODE_BLUETOOTH);

				updateBluetoothIcon();
			} else if(!strcmp(notif, "notifnradiooff")) {
				// Radio is off.  Remove the Bluetooth icon from the status bar
				m_bluetoothRadionOn = false;
				Q_EMIT signalBluetoothIndexChanged(m_bluetoothRadionOn, StatusBar::BLUETOOTH_OFF);
				Q_EMIT signalBluetoothPowerStateChanged(RADIO_OFF);

				updateAirplaneModeProgress(AP_MODE_BLUETOOTH);

				// All profiles must be disconnected
				m_bluetoothProfileStates.clear();
			}
		}
	}

	if (root && !is_error(root)) json_object_put(root);

	return true;
}

void StatusBarServicesConnector::requestBluetoothConnectedProfilesInfo()
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	result = LSCall(m_service, "palm://com.palm.bluetooth/prof/profgetstate", "{\"profile\":\"all\"}",
			statusBarBtConnectedProfilesInfoCallback, NULL, NULL, &lsError);

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

bool StatusBarServicesConnector::statusBarBtConnectedProfilesInfoCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->btConnectedProfilesInfoCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::btConnectedProfilesInfoCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* profArray = 0;
	struct json_object* prof = 0;
	struct json_object* label = 0;
	const char* state;
	const char* address;
	const char* name;

	bool connected = false;

	g_message("StatusBar - btConnectedProfilesInfoCallback %s", LSMessageGetPayload(message));

	m_bluetoothProfileStates.clear();

	for(unsigned int i = 0; i < m_bluetoothProfiles.size(); i++) {
		state = 0;
		address = 0;
		name = 0;

		profArray = json_object_object_get(root, m_bluetoothProfiles[i].c_str());
		if (profArray && !is_error(profArray) && json_object_is_type(profArray, json_type_array)) {
			for(int x = 0; x < json_object_array_length(profArray); x++) {
				prof = json_object_array_get_idx(profArray, x);
				if (prof && !is_error(prof) && json_object_is_type(prof, json_type_object)) {
					label = json_object_object_get(prof, "state");
					if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
						state = json_object_get_string(label);
					}
					label = json_object_object_get(prof, "address");
					if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
						address = json_object_get_string(label);
					}
					label = json_object_object_get(prof, "name");
					if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
						name = json_object_get_string(label);
					}

					if(state) {
						if(!strcmp(state, "connected") && address) {
							connected = true;
							m_bluetoothProfileStates[m_bluetoothProfiles[i] + std::string(address)].status = BT_CONNECTED;
							m_bluetoothProfileStates[m_bluetoothProfiles[i] + std::string(address)].address = std::string(address);
							m_bluetoothProfileStates[m_bluetoothProfiles[i] + std::string(address)].name = name ? std::string(name) : "";

							if (m_bluetoothMenuProfiles.find(m_bluetoothProfiles[i]) != m_bluetoothMenuProfiles.end()) {
								Q_EMIT signalBluetoothConnStateChanged(true, name ? name : "");
							}
						}
					}
				}
			}
		}
	}

	if(connected) {
		Q_EMIT signalBluetoothIndexChanged(true, StatusBar::BLUETOOTH_CONNECTED);
	}

	if (root && !is_error(root)) json_object_put(root);

	return true;
}

void StatusBarServicesConnector::requestCallForwardStatus()
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	result = LSCall(m_service, "palm://com.palm.telephony/forwardQuery", "{\"condition\": \"unconditional\", \"bearer\": \"defaultbearer\", \"subscribe\":true, \"network\":false}",
			statusBarCallForwardRequestCallback, NULL, NULL, &lsError);

	m_callFwdStatusRequested = true;

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

bool StatusBarServicesConnector::statusBarCallForwardRequestCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->callForwardRequestCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::callForwardRequestCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;
	struct json_object* ext = 0;

	bool callFwdActive = false;
	bool found = false;

	g_message("StatusBar - callForwardRequestCallback %s", LSMessageGetPayload(message));

	//For CDMA, do not show the Call Forwarding Icon.
	if(m_phoneType == PHONE_TYPE_CDMA)
		return true;

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		bool success = json_object_get_boolean(label);
		if(success) {
			ext = json_object_object_get(root, "extended");
			if (ext && !is_error(ext) && json_object_is_type(ext, json_type_object)) {
				const char* condition = 0;
				label = json_object_object_get(ext, "condition");
				if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
					condition = json_object_get_string(label);
				}
				if(condition && (!strcmp(condition, "unconditional") || !strcmp(condition, "allforwarding"))) {
					// only update icon for unconditional and all forwarding;
					// others aren't interesting
					struct json_object* statusArray = 0;
					struct json_object* status = 0;
					const char* bearer = 0;
					bool activated = false;
					statusArray = json_object_object_get(ext, "status");
					if (statusArray && !is_error(statusArray) && json_object_is_type(statusArray, json_type_array)) {
						for(int i = 0; i < json_object_array_length(statusArray); i++) {
							status = json_object_array_get_idx(statusArray, i);
							if (status && !is_error(status) && json_object_is_type(status, json_type_object)) {
								label = json_object_object_get(status, "bearer");
								if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
									bearer = json_object_get_string(label);
								}

								label = json_object_object_get(status, "activated");
								if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
									activated = json_object_get_boolean(label);

									if(bearer && (!strcmp(bearer, "voice") || !strcmp(bearer, "default") || !strcmp(bearer, "defaultbearer"))) {
										// we only care about voice/default bearers
										if(activated)
											callFwdActive = true;
										found = true;
									}
								}
							}
							if(found)
								break;
						}
					}

				}
			}
		}
	}

	if(found)
		Q_EMIT signalCallForwardStateChanged(callFwdActive);

	if (root && !is_error(root)) json_object_put(root);

	return true;
}

bool StatusBarServicesConnector::statusBarBluetoothServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->bluetoothServiceUpCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::bluetoothServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - bluetoothServiceUpCallback %s", LSMessageGetPayload(message));

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (json && !is_error(json))  {

		json_object* label = json_object_object_get(json, "connected");
		if (label && json_object_is_type(label, json_type_boolean)) {

			bool connected = json_object_get_boolean(label);

			if (connected) {
				// Bluetooth service up.
				bool result;
				LSError lsError;
				LSErrorInit(&lsError);

				//Subscribe to Bluetooth Profile Notifications.
				result = LSCall(handle, "palm://com.palm.bluetooth/prof/subscribenotifications", "{\"subscribe\":true}",
						statusBarBluetoothEventsCallback, NULL, NULL, &lsError);
				if (!result) goto error;

				//Subscribe to Bluetooth Gap Notifications.
				result = LSCall(handle, "palm://com.palm.bluetooth/gap/subscribenotifications", "{\"subscribe\":true}",
						statusBarBluetoothEventsCallback, NULL, NULL, &lsError);

error:
				if (LSErrorIsSet(&lsError)) {
					LSErrorPrint(&lsError, stderr);
					LSErrorFree(&lsError);
				}
			} else {
				// Bluetooth disconnected
				// Simulate Power Off.  Remove the Bluetooth icon from the status bar
				m_bluetoothRadionOn = false;
				Q_EMIT signalBluetoothIndexChanged(m_bluetoothRadionOn, StatusBar::BLUETOOTH_OFF);
				Q_EMIT signalBluetoothPowerStateChanged(RADIO_OFF);

				// All profiles must be disconnected
				m_bluetoothProfileStates.clear();
			}
		}

		json_object_put(json);
	}

	return true;
}

bool StatusBarServicesConnector::statusBarBluetoothEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->bluetoothEventsCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::bluetoothEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;
	const char *string = 0;
	int error = 0;
	bool updateIcon = false;

	g_message("StatusBar - bluetoothEventsCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		bool success = json_object_get_boolean(label);
		if(!success) {
			// Simulate Power Off.  Remove the Bluetooth icon from the status bar
			m_bluetoothRadionOn = false;
			Q_EMIT signalBluetoothIndexChanged(m_bluetoothRadionOn, StatusBar::BLUETOOTH_OFF);
			Q_EMIT signalBluetoothPowerStateChanged(RADIO_OFF);

			// All profiles must be disconnected
			m_bluetoothProfileStates.clear();
			goto Done;
		}
	}

	m_btRadioTurningOn = false;

	label = json_object_object_get(root, "notification");
	if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
		const char *notification = 0;
		const char *profile = 0;
		const char *address = 0;
		const char *name = 0;
		int profIndex = -1;
		std::string profKey;

		notification = json_object_get_string(label);

		label = json_object_object_get(root, "profile");
		if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
			profile = json_object_get_string(label);
			if(profile) {
				for(unsigned int i = 0; i < m_bluetoothProfiles.size(); i++) {
					if(!strcmp(profile, m_bluetoothProfiles[i].c_str())) {
						profIndex = i;
						break;
					}
				}
			}
		}

		label = json_object_object_get(root, "address");
		if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
			address = json_object_get_string(label);
		}

		label = json_object_object_get(root, "name");
		if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
			name = json_object_get_string(label);
		}

		if(address && (profIndex != -1)) {
				profKey = m_bluetoothProfiles[profIndex] + address;
		}

		label = json_object_object_get(root, "error");
		if (label && !is_error(label) && json_object_is_type(label, json_type_int)) {
			error = json_object_get_int(label);
		}

		if(notification) {
			if(!strcmp(notification, "notifndevremoved")) {
				// Device Removed
			} else if(!strcmp(notification, "notifnconnecting")) {
				if((profIndex != -1) && (address != 0)) {
					Q_EMIT signalBluetoothIndexChanged(true, StatusBar::BLUETOOTH_CONNECTING);

					m_bluetoothProfileStates[profKey].status = BT_CONNECTING;
					m_bluetoothProfileStates[profKey].address = address;
					m_bluetoothProfileStates[profKey].name = name ? name : "";


					if (m_bluetoothMenuProfiles.find(m_bluetoothProfiles[profIndex]) != m_bluetoothMenuProfiles.end()) {
						if(!isDeviceConnectedOnMenuProfiles(address))
							updateBtDeviceInfo(&(m_bluetoothProfileStates[profKey]));

						Q_EMIT signalBluetoothConnStateChanged(true, name ? name : "");
					}
				}
			} else if(!strcmp(notification, "notifnconnected")) {
				if((profIndex != -1) && (address != 0)) {
					if(error == 0) {
						m_bluetoothProfileStates[profKey].status = BT_CONNECTED;
						m_bluetoothProfileStates[profKey].address = address;
						m_bluetoothProfileStates[profKey].name = name ? name : "";

						if (m_bluetoothMenuProfiles.find(m_bluetoothProfiles[profIndex]) != m_bluetoothMenuProfiles.end()) {
							updateBtDeviceInfo(&(m_bluetoothProfileStates[profKey]));

							Q_EMIT signalBluetoothConnStateChanged(true, name ? name : "");
						}
					} else {
						// Error reported

						const char* alreadyConnectedAddress = 0;
						label = json_object_object_get(root, "alreadyconnectedaddr");
						if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
							alreadyConnectedAddress = json_object_get_string(label);
						}

						if(alreadyConnectedAddress) {
							m_bluetoothProfileStates[profKey].status = BT_CONNECTED;
							m_bluetoothProfileStates[profKey].address = alreadyConnectedAddress;

							if (m_bluetoothMenuProfiles.find(m_bluetoothProfiles[profIndex]) != m_bluetoothMenuProfiles.end()) {
								updateBtDeviceInfo(&(m_bluetoothProfileStates[profKey]));

								Q_EMIT signalBluetoothConnStateChanged(true, name ? name : "");
							}
						} else {
							if(m_bluetoothProfileStates.find(profKey) != m_bluetoothProfileStates.end()) {
								m_bluetoothProfileStates[profKey].status = BT_DISCONNECTED;
								if (m_bluetoothMenuProfiles.find(m_bluetoothProfiles[profIndex]) != m_bluetoothMenuProfiles.end()) {
									if(!isDeviceConnectedOnMenuProfiles(address)) {
										updateBtDeviceInfo(&(m_bluetoothProfileStates[profKey]));

										Q_EMIT signalBluetoothConnStateChanged(false, "");
									}
								}
								m_bluetoothProfileStates.erase(profKey);
							}
						}
					}

					updateIcon = true;
				}
			} else if(!strcmp(notification, "notifndisconnecting")) {
				if((profIndex != -1) && (address != 0)) {
					m_bluetoothProfileStates[profKey].status = BT_DISCONNECTING;
					if (m_bluetoothMenuProfiles.find(m_bluetoothProfiles[profIndex]) != m_bluetoothMenuProfiles.end()) {
						if(!isDeviceConnectedOnMenuProfiles(address))
							updateBtDeviceInfo(&(m_bluetoothProfileStates[profKey]));
					}
					updateIcon = true;
				}
			} else if(!strcmp(notification, "notifndisconnected")) {
				if((profIndex != -1) && (address != 0)) {
					if(m_bluetoothProfileStates.find(profKey) != m_bluetoothProfileStates.end()) {
						m_bluetoothProfileStates[profKey].status = BT_DISCONNECTED;
						if (m_bluetoothMenuProfiles.find(m_bluetoothProfiles[profIndex]) != m_bluetoothMenuProfiles.end()) {
							if(!isDeviceConnectedOnMenuProfiles(address)) {
								updateBtDeviceInfo(&(m_bluetoothProfileStates[profKey]));
								Q_EMIT signalBluetoothConnStateChanged(false, "");
							}
						}
						m_bluetoothProfileStates.erase(profKey);
					}
					updateIcon = true;
				}
			} else if(!strcmp(notification, "notifnconnectacceptrequest")) {


			}else if(!strcmp(notification, "notifndevrenamed")) {
				std::map<std::string, BluetoothProfState>::iterator it;
				for(it = m_bluetoothProfileStates.begin(); it != m_bluetoothProfileStates.end(); it++) {
					if(!strcmp((*it).second.address.c_str(), address)) {
						(*it).second.name = name ? name : "";
						updateBtDeviceInfo(&((*it).second));
						if(((*it).second.status == BT_CONNECTED) || ((*it).second.status == BT_CONNECTING)) {
							Q_EMIT signalBluetoothConnStateChanged(true, name ? name : "");
						}
					}
				}
				updateIcon = true;
			}
		}
	}

Done:
	if(updateIcon)
		updateBluetoothIcon();

	if (root && !is_error(root)) json_object_put(root);

	return true;
}

void StatusBarServicesConnector::updateBluetoothIcon()
{
	StatusBar::IndexBluetooth state = StatusBar::BLUETOOTH_ON;

	std::map<std::string, BluetoothProfState>::iterator it;

	for(it = m_bluetoothProfileStates.begin(); it != m_bluetoothProfileStates.end(); it++) {
		if((*it).second.status == BT_CONNECTING)
			state = StatusBar::BLUETOOTH_CONNECTING;

		if((*it).second.status == BT_CONNECTED) {
			state = StatusBar::BLUETOOTH_CONNECTED;
			break;
		}
	}

	Q_EMIT signalBluetoothIndexChanged(true, state);
}

void StatusBarServicesConnector::updateBtDeviceInfo(BluetoothProfState* info)
{
	t_bluetoothDevice deviceInfo;
	sprintf(deviceInfo.displayName, "%s", info->name.c_str());
	sprintf(deviceInfo.btAddress, "%s", info->address.c_str());
	deviceInfo.cod = 0;
	switch(info->status) {
	case BT_DISCONNECTED: {
			sprintf(deviceInfo.connectionState, "disconnected");
			deviceInfo.showConnected = false;
		}
		break;
		case BT_CONNECTING: {
			sprintf(deviceInfo.connectionState, "connecting");
			deviceInfo.showConnected = false;
		}
		break;
		case BT_CONNECTED: {
			sprintf(deviceInfo.connectionState, "connected");
			deviceInfo.showConnected = true;
		}
		break;
		case BT_DISCONNECTING: {
			sprintf(deviceInfo.connectionState, "disconnecting");
			deviceInfo.showConnected = false;
		}
		break;
	}
	Q_EMIT signalBluetoothUpdateDeviceStatus(&deviceInfo);
}

void StatusBarServicesConnector::setBluetoothOnState(bool on)
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	char params[64];
	if(on) {
		sprintf(params, "{\"visible\":false,\"connectable\":true}");
		result = LSCall(m_service, "palm://com.palm.btmonitor/monitor/radioon", params,
				statusBarBluetoothTurnOnCallback, NULL, NULL, &lsError);
	} else {
		result = LSCall(m_service, "palm://com.palm.btmonitor/monitor/radiooff", "{}",
				statusBarBluetoothTurnOffCallback, NULL, NULL, &lsError);
	}

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

bool StatusBarServicesConnector::statusBarBluetoothTurnOnCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->bluetoothTurnOnCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::bluetoothTurnOnCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* header = 0;
	struct json_object* label = 0;

	g_message("StatusBar - bluetoothTurnOnCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if(json_object_get_boolean(label)) {
			Q_EMIT signalBluetoothTurnedOn();
			goto Done;
		} else {
			g_warning("StatusBar - Error Bluetooth Radio : %s", LSMessageGetPayload(message));
			updateAirplaneModeProgress(AP_MODE_BLUETOOTH);
		}
	}

Done:
	if (root && !is_error(root)) json_object_put(root);

	return true;
}

bool StatusBarServicesConnector::statusBarBluetoothTurnOffCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->bluetoothTurnOffCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::bluetoothTurnOffCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	g_message("StatusBar - bluetoothTurnOffCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if(!json_object_get_boolean(label)) {
			g_warning("StatusBar - Error Bluetooth Radio : %s", LSMessageGetPayload(message));
			updateAirplaneModeProgress(AP_MODE_BLUETOOTH);
		}
	}

	if (root && !is_error(root)) json_object_put(root);

	return true;
}


void StatusBarServicesConnector::requestTrustedBluetoothDevicesList()
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	if(m_bluetoothDeviceListReq) {
		LSCallCancel(m_service, m_bluetoothDeviceListReq, &lsError);
		m_bluetoothDeviceListReq = 0;
	}

	result = LSCall(m_service, "palm://com.palm.bluetooth/gap/gettrusteddevices", "{ }",
			statusBarBluetoothTruestedListCallback, NULL, &m_bluetoothDeviceListReq, &lsError);

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

void StatusBarServicesConnector::cancelBluetoothDevicesListRequest()
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	if(m_bluetoothDeviceListReq) {
		LSCallCancel(m_service, m_bluetoothDeviceListReq, &lsError);
		m_bluetoothDeviceListReq = 0;
	}

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}


bool StatusBarServicesConnector::statusBarBluetoothTruestedListCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->bluetoothTruestedListCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::bluetoothTruestedListCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;
	struct json_object* devicesArray = 0;
	struct json_object* deviceInfo = 0;

	g_message("StatusBar - bluetoothTruestedListCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if(!json_object_get_boolean(label)) {
			Q_EMIT signalBluetoothTrustedDevicesUpdate(0, NULL);
			goto Done;
		}
	}

	devicesArray = json_object_object_get(root, "trusteddevices");
	if (devicesArray && !is_error(devicesArray) && json_object_is_type(devicesArray, json_type_array)) {
		int nEntries =  json_object_array_length(devicesArray);
		t_bluetoothDevice devices[nEntries];
		int nDevicesOnMenu = 0;

		for(int x = 0; x < nEntries; x++) {
			sprintf(devices[nDevicesOnMenu].displayName, "%s", "");
			sprintf(devices[nDevicesOnMenu].btAddress, "%s", "");
			sprintf(devices[nDevicesOnMenu].connectionState, "%s", "");
			devices[nDevicesOnMenu].cod = 0;
			devices[nDevicesOnMenu].showConnected = false;

			deviceInfo = json_object_array_get_idx(devicesArray, x);
			if (deviceInfo && !is_error(deviceInfo) && json_object_is_type(deviceInfo, json_type_object)) {
				label = json_object_object_get(deviceInfo, "address");
				if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
					sprintf(devices[nDevicesOnMenu].btAddress, "%s", json_object_get_string(label));
				}

				label = json_object_object_get(deviceInfo, "name");
				if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
					sprintf(devices[nDevicesOnMenu].displayName, "%s", json_object_get_string(label));
				}

				label = json_object_object_get(deviceInfo, "status");
				if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
					sprintf(devices[nDevicesOnMenu].connectionState, "%s", json_object_get_string(label));
				}

				label = json_object_object_get(deviceInfo, "cod");
				if (label && !is_error(label) && json_object_is_type(label, json_type_int)) {
					devices[nDevicesOnMenu].cod = json_object_get_int(label);
				}

				if(BtDeviceClass::isHFGSupported(devices[nDevicesOnMenu].cod) ||
				   BtDeviceClass::isPhone(devices[nDevicesOnMenu].cod) ||
				   BtDeviceClass::isA2DPSupported(devices[nDevicesOnMenu].cod)) {
					if(!strcmp(devices[nDevicesOnMenu].connectionState, "connected")) {
						std::string hfgKey = std::string("hfg") + devices[nDevicesOnMenu].btAddress;
						std::string a2dpKey = std::string("a2dp") + devices[nDevicesOnMenu].btAddress;
						std::string hfKey = std::string("hf") + devices[nDevicesOnMenu].btAddress;
						std::string mapKey = std::string("mapc") + devices[nDevicesOnMenu].btAddress;
						if(((m_bluetoothProfileStates.find(hfgKey) != m_bluetoothProfileStates.end()) && (m_bluetoothProfileStates[hfgKey].status = BT_CONNECTED)) ||
						   ((m_bluetoothProfileStates.find(a2dpKey) != m_bluetoothProfileStates.end()) && (m_bluetoothProfileStates[a2dpKey].status = BT_CONNECTED)) ||
						   ((m_bluetoothProfileStates.find(mapKey) != m_bluetoothProfileStates.end()) && (m_bluetoothProfileStates[mapKey].status = BT_CONNECTED)) ||
						   ((m_bluetoothProfileStates.find(hfKey) != m_bluetoothProfileStates.end()) && (m_bluetoothProfileStates[hfKey].status = BT_CONNECTED))  )
							devices[nDevicesOnMenu].showConnected = true;
						else {
							devices[nDevicesOnMenu].showConnected = false;
							sprintf(devices[nDevicesOnMenu].connectionState, "disconnected");
						}
					}
					else if(!strcmp(devices[nDevicesOnMenu].connectionState, "connecting")) {
						devices[nDevicesOnMenu].showConnected = false;
					}
					else {
						devices[nDevicesOnMenu].showConnected = false;
					}

					nDevicesOnMenu++;
				}
			}
		}

		Q_EMIT signalBluetoothTrustedDevicesUpdate(nDevicesOnMenu, devices);
	}


Done:
	if (root && !is_error(root)) json_object_put(root);

	return true;
}

void StatusBarServicesConnector::requestBluetoothNumProfiles()
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	result = LSCall(m_service, "palm://com.palm.preferences/appProperties/getAppProperty",
			        "{\"appId\":\"com.palm.bluetooth\",\"key\":\"header\"}",
			        statusBarBluetoothNumProfilesCallback, NULL, NULL, &lsError);

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

bool StatusBarServicesConnector::statusBarBluetoothNumProfilesCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->bluetoothNumProfilesCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::bluetoothNumProfilesCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* header = 0;
	struct json_object* label = 0;

	g_message("StatusBar - bluetoothNumProfilesCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if(!json_object_get_boolean(label)) {
			Q_EMIT signalBluetoothParedDevicesAvailable(true);
			goto Done;
		}
	}

	header = json_object_object_get(root, "header");
	if (header && !is_error(header) && json_object_is_type(header, json_type_object)) {
		label = json_object_object_get(header, "checknum");
		if (label && !is_error(label) && json_object_is_type(label, json_type_int)) {
			int checknum = json_object_get_int(label);
			if(checknum == 16432047) {
				label = json_object_object_get(header, "numofprofiles");
				if (label && !is_error(label) && json_object_is_type(label, json_type_int)) {
					int numProfiles = json_object_get_int(label);
					if(numProfiles > 0) {
						Q_EMIT signalBluetoothParedDevicesAvailable(true);
					} else {
						Q_EMIT signalBluetoothParedDevicesAvailable(false);
					}
				} else {
					Q_EMIT signalBluetoothParedDevicesAvailable(false);
				}
			} else {
				Q_EMIT signalBluetoothParedDevicesAvailable(true);
			}
		} else {
			Q_EMIT signalBluetoothParedDevicesAvailable(true);
		}
	} else {
		Q_EMIT signalBluetoothParedDevicesAvailable(true);
	}

Done:
	if (root && !is_error(root)) json_object_put(root);

	return true;
}

void StatusBarServicesConnector::bluetoothProfileConnect(std::string profile, std::string address)
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);
	std::string params = "{\"profile\":\"" + profile + "\",\"address\":\"" + address + "\"}";

	result = LSCall(m_service, "palm://com.palm.bluetooth/prof/profconnect", params.c_str(), NULL, NULL, NULL, &lsError);

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

void StatusBarServicesConnector::bluetoothProfileDisconnect(std::string profile, std::string address)
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);
	std::string params = "{\"profile\":\"" + profile + "\",\"address\":\"" + address + "\"}";

	result = LSCall(m_service, "palm://com.palm.bluetooth/prof/profdisconnect", params.c_str(), NULL, NULL, NULL, &lsError);

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

bool StatusBarServicesConnector::isDeviceConnectedOnMenuProfiles(std::string address)
{
	int profIndex = -1;
	std::string profKey;
	bool connected = false;
	std::set<std::string>::iterator it;

	for(it = m_bluetoothMenuProfiles.begin(); it != m_bluetoothMenuProfiles.end(); it++) {
		std::string prof = *it;
		profKey = "";

		profKey = prof + address;
		if(m_bluetoothProfileStates.find(profKey) != m_bluetoothProfileStates.end()) {
			if ((m_bluetoothProfileStates[profKey].address == address) &&
				((m_bluetoothProfileStates[profKey].status == BT_CONNECTED) || (m_bluetoothProfileStates[profKey].status == BT_CONNECTING))) {
				connected = true;
				break;
			}
		}
	}
	return connected;
}

void StatusBarServicesConnector::disconnectAllBtMenuProfiles(std::string address)
{
	std::string profKey;
	std::set<std::string>::iterator it;

	for(it = m_bluetoothMenuProfiles.begin(); it != m_bluetoothMenuProfiles.end(); it++) {
		std::string prof = *it;
		profKey = "";

		profKey = prof + address;
		if(m_bluetoothProfileStates.find(profKey) != m_bluetoothProfileStates.end()) {
			if ((m_bluetoothProfileStates[profKey].address == address) && (m_bluetoothProfileStates[profKey].status != BT_DISCONNECTED)) {
				bluetoothProfileDisconnect(prof, address);
			}
		}
	}
}

bool StatusBarServicesConnector::isDeviceConnected(std::string address)
{
	int profIndex = -1;
	std::string profKey;
	bool connected = false;
	std::set<std::string>::iterator it;

	for(unsigned int i = 0; i < m_bluetoothProfiles.size(); i++) {
		std::string prof = m_bluetoothProfiles[i];
		profKey = "";

		profKey = prof + address;
		if(m_bluetoothProfileStates.find(profKey) != m_bluetoothProfileStates.end()) {
			if ((m_bluetoothProfileStates[profKey].address == address) && (m_bluetoothProfileStates[profKey].status != BT_DISCONNECTED)) {
				connected = true;
				break;
			}
		}
	}
	return connected;
}

bool StatusBarServicesConnector::statusBarConnMgrServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->connMgrServiceUpCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::connMgrServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - connMgrServiceUpCallback %s", LSMessageGetPayload(message));

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (json && !is_error(json))  {

		json_object* label = json_object_object_get(json, "connected");
		if (label && json_object_is_type(label, json_type_boolean)) {

			bool connected = json_object_get_boolean(label);

			if (connected) {
				// Bluetooth service up.
				bool result;
				LSError lsError;
				LSErrorInit(&lsError);

				//Subscribe to Connection Manager Route Status Notifications.
				result = LSCall(handle, "palm://com.palm.connectionmanager/getstatus", "{\"subscribe\":true}",
						statusBarConnMgrEventsCallback, NULL, NULL, &lsError);
				if (!result) goto error;

error:
				if (LSErrorIsSet(&lsError)) {
					LSErrorPrint(&lsError, stderr);
					LSErrorFree(&lsError);
				}
			}
		}
		json_object_put(json);
	}

	return true;
}

bool StatusBarServicesConnector::statusBarConnMgrEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->connMgrEventsCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::connMgrEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;
	const char *string = 0;

	g_message("StatusBar - connMgrEventsCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "isInternetConnectionAvailable");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		m_isInternetConnectionAvailable = json_object_get_boolean(label);

		if(m_isInternetConnectionAvailable) {
			struct json_object* wifiObj = 0;

			wifiObj = json_object_object_get(root, "wifi");
			if (wifiObj && !is_error(wifiObj) && json_object_is_type(wifiObj, json_type_object)) {
				const char* state = 0;
				const char* onInternet = 0;

				label = json_object_object_get(wifiObj, "state");
				if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
					state = json_object_get_string(label);
				}

				label = json_object_object_get(wifiObj, "onInternet");
				if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
					onInternet = json_object_get_string(label);
				}

				if(!strcmp(state, "connected") && !strcmp(onInternet, "yes")) {
					//hide the WAN icon now.
					Q_EMIT signalWanIndexChanged(false, StatusBar::WAN_OFF);
					m_hideDataIcon = true;
				}
				else {
					m_hideDataIcon = false;
				}
			}
		}
	}

	if (root && !is_error(root)) json_object_put(root);

	return true;
}

bool StatusBarServicesConnector::statusBarWiFiServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->wiFiServiceUpCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::wiFiServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - wiFiServiceUpCallback %s", LSMessageGetPayload(message));

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (json && !is_error(json))  {

		json_object* label = json_object_object_get(json, "connected");
		if (label && json_object_is_type(label, json_type_boolean)) {

			bool connected = json_object_get_boolean(label);

			if (connected) {
				// WiFi service up.
				bool result;
				LSError lsError;
				LSErrorInit(&lsError);

				m_wifiSSID.clear();
				m_wifiRadioOn = false;
				m_wifiConnected = false;
				Q_EMIT signalWifiStateChanged(m_wifiRadioOn, m_wifiConnected, m_wifiSSID, "");

				//Subscribe to Connection Manager Route Status Notifications.
				result = LSCall(handle, "palm://com.palm.wifi/getstatus", "{\"subscribe\":true}",
						statusBarWifiEventsCallback, NULL, NULL, &lsError);

				if (LSErrorIsSet(&lsError)) {
					LSErrorPrint(&lsError, stderr);
					LSErrorFree(&lsError);
				}
			} else {
				if(m_wifiRadioOn) {
					//Simulate WiFi Service Disabled Notification.
					m_wifiConnected = false;
					m_wifiSSID.clear();
					m_wifiRadioOn = false;
					Q_EMIT signalWifiIndexChanged(false, StatusBar::WIFI_OFF);
					Q_EMIT signalWifiStateChanged(m_wifiRadioOn, m_wifiConnected, m_wifiSSID, "");
				}
			}
		}
		json_object_put(json);
	}

	return true;
}


bool StatusBarServicesConnector::statusBarWifiEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->wifiEventsCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::wifiEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;
	const char *status = 0;

	g_message("StatusBar - wifiEventsCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "status");
	if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
		status = json_object_get_string(label);

		if(status) {
			if(!strcmp(status, "connectionStateChanged")) {
				struct json_object* networkInfo = 0;
				m_wifiRadioOn = true;

				networkInfo = json_object_object_get(root, "networkInfo");
				if (networkInfo && !is_error(networkInfo) && json_object_is_type(networkInfo, json_type_object)) {
					const char* connectState = 0;
					const char* ssid = 0;

					label = json_object_object_get(networkInfo, "connectState");
					if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
						connectState = json_object_get_string(label);
					}

					label = json_object_object_get(networkInfo, "ssid");
					if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
						ssid = json_object_get_string(label);
					}

					if(connectState) {
						if(!strcmp(connectState, "associating") || !strcmp(connectState, "associated")) {
							m_wifiSSID = std::string(ssid);
							Q_EMIT signalWifiIndexChanged(true, StatusBar::WIFI_CONNECTING);
							Q_EMIT signalWifiStateChanged(m_wifiRadioOn, m_wifiConnected, m_wifiSSID, connectState);
						}
						else if(!strcmp(connectState, "ipFailed") || !strcmp(connectState, "notAssociated") || !strcmp(connectState, "associationFailed")) {
							m_wifiSSID.clear();
							m_wifiConnected = false;
							Q_EMIT signalWifiIndexChanged(true, StatusBar::WIFI_ON);
							Q_EMIT signalWifiStateChanged(m_wifiRadioOn, m_wifiConnected, m_wifiSSID, connectState);

						} else if(!strcmp(connectState, "ipConfigured")) {
							m_wifiConnected = true;
							m_wifiSSID = std::string(ssid);
							Q_EMIT signalWifiStateChanged(m_wifiRadioOn, m_wifiConnected, m_wifiSSID, connectState);

							label = json_object_object_get(networkInfo, "signalBars");
							if (label && !is_error(label) && json_object_is_type(label, json_type_int)) {
								int bars = json_object_get_int(label) - 1;
								if(bars < 1) bars = 1;
								if(bars > 3) bars = 3;

								Q_EMIT signalWifiIndexChanged(true, (StatusBar::IndexWiFi )(StatusBar::WIFI_BAR_1 + bars));
							}
						}
					}
				}
			} else if(!strcmp(status, "signalStrengthChanged")) {
				if (m_wifiConnected) {
					label = json_object_object_get(root, "signalBars");
					if (label && !is_error(label) && json_object_is_type(label, json_type_int)) {
						int bars = json_object_get_int(label);
						if(bars < 1) bars = 1;
						if(bars > 3) bars = 3;

						Q_EMIT signalWifiIndexChanged(true, (StatusBar::IndexWiFi )(StatusBar::WIFI_BAR_1 + bars));
					}
				}
			} else if(!strcmp(status, "serviceEnabled")) {
				m_wifiConnected = false;
				m_wifiSSID.clear();
				m_wifiRadioOn = true;
				Q_EMIT signalWifiIndexChanged(true, StatusBar::WIFI_ON);
				Q_EMIT signalWifiStateChanged(m_wifiRadioOn, m_wifiConnected, m_wifiSSID, "");
				updateAirplaneModeProgress(AP_MODE_WIFI);
			} else if(!strcmp(status, "serviceDisabled")) {
				m_wifiConnected = false;
				m_wifiSSID.clear();
				m_wifiRadioOn = false;
				Q_EMIT signalWifiIndexChanged(false, StatusBar::WIFI_OFF);
				Q_EMIT signalWifiStateChanged(m_wifiRadioOn, m_wifiConnected, m_wifiSSID, "");
				updateAirplaneModeProgress(AP_MODE_WIFI);
			}

			if (m_wifiConnected && m_hideDataIcon) {
				signalWanIndexChanged(false, StatusBar::WAN_OFF);
			}

			if(!m_cmPayloadBuffer.empty() && !m_wifiConnected) {
				wanStatusEventsCallback(NULL, m_cmPayloadBuffer.c_str(), NULL);
			}
		}
	}

	if (root && !is_error(root)) json_object_put(root);

	return true;
}

void StatusBarServicesConnector::requestWifiAvailableNetworksList()
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	if(m_wifiFindNetworksReq) {
		LSCallCancel(m_service, m_wifiFindNetworksReq, &lsError);
		m_wifiFindNetworksReq = 0;
	}

	result = LSCall(m_service, "palm://com.palm.wifi/findnetworks", "{ }",
			statusBarWifiAvailableNetworksListCallback, NULL, &m_wifiFindNetworksReq, &lsError);

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

void StatusBarServicesConnector::cancelWifiNetworksListRequest()
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	if(m_wifiFindNetworksReq) {
		LSCallCancel(m_service, m_wifiFindNetworksReq, &lsError);
		m_wifiFindNetworksReq = 0;
	}

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

bool StatusBarServicesConnector::statusBarWifiAvailableNetworksListCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->wifiAvailableNetworksListCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::wifiAvailableNetworksListCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;
	const char *status = 0;
	struct json_object* netArray = 0;
	struct json_object* networkInfo = 0;

	g_message("StatusBar - wifiAvailableNetworksListCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if(!json_object_get_boolean(label)) {
			Q_EMIT signalWifiAvailableNetworksListUpdate(0, NULL);
			goto Done;
		}
	}

	netArray = json_object_object_get(root, "foundNetworks");
	if (netArray && !is_error(netArray) && json_object_is_type(netArray, json_type_array)) {
		int nEntries =  json_object_array_length(netArray);
		t_wifiAccessPoint accessPoints[nEntries];

		for(int x = 0; x < json_object_array_length(netArray); x++) {
			accessPoints[x].profileId = 0;
			accessPoints[x].signalBars = 0;
			accessPoints[x].connected = false;
			sprintf(accessPoints[x].ssid, "%s", "");
			sprintf(accessPoints[x].securityType, "%s", "");
			sprintf(accessPoints[x].connectionState, "%s", "");
			label = json_object_array_get_idx(netArray, x);
			if (label && !is_error(label) && json_object_is_type(label, json_type_object)) {
				networkInfo = json_object_object_get(label, "networkInfo");
				if (networkInfo && !is_error(networkInfo) && json_object_is_type(networkInfo, json_type_object)) {
					label = json_object_object_get(networkInfo, "ssid");
					if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
						sprintf(accessPoints[x].ssid, "%s", json_object_get_string(label));
					}

					label = json_object_object_get(networkInfo, "profileId");
					if (label && !is_error(label) && json_object_is_type(label, json_type_int)) {
						accessPoints[x].profileId = json_object_get_int(label);
					}

					label = json_object_object_get(networkInfo, "signalBars");
					if (label && !is_error(label) && json_object_is_type(label, json_type_int)) {
						accessPoints[x].signalBars = json_object_get_int(label);
					}

					label = json_object_object_get(networkInfo, "securityType");
					if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
						sprintf(accessPoints[x].securityType, "%s", json_object_get_string(label));
					}

					label = json_object_object_get(networkInfo, "connectState");
					if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
						sprintf(accessPoints[x].connectionState, "%s", json_object_get_string(label));
						if((!strcmp(accessPoints[x].connectionState, "ipConfigured")) || (!strcmp(accessPoints[x].connectionState, "associated"))) {
							accessPoints[x].connected = true;
						}
					}
				}
			}
		}

		Q_EMIT signalWifiAvailableNetworksListUpdate(nEntries, accessPoints);
	}

Done:
	if (root && !is_error(root)) json_object_put(root);

	return true;
}

void StatusBarServicesConnector::connectToWifiNetwork(std::string ssid, int profileId, std::string security)
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	char params[255];
	if(profileId) {
		sprintf(params, "{\"profileId\":%d}", profileId);
	} else {
		if(security.empty())
			sprintf(params, "{\"ssid\":\"%s\"}", ssid.c_str());
		else
			sprintf(params, "{\"ssid\":\"%s\",\"securityType\":\"%s\"}", ssid.c_str(), security.c_str());
	}

	result = LSCall(m_service, "palm://com.palm.wifi/connect", params,
			statusBarWifiConnectCallback, NULL, NULL, &lsError);

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

bool StatusBarServicesConnector::statusBarWifiConnectCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->wifiConnectCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::wifiConnectCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	g_message("StatusBar - wifiConnectCallback %s", LSMessageGetPayload(message));


	if (root && !is_error(root)) json_object_put(root);

	return true;
}

void StatusBarServicesConnector::setWifiOnState(bool on)
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	char params[32];
	if(on) {
		sprintf(params, "{\"state\":\"enabled\"}");
	} else {
		sprintf(params, "{\"state\":\"disabled\"}");
	}

	result = LSCall(m_service, "palm://com.palm.wifi/setstate", params,
			statusBarWifiPowerStateChangeCallback, NULL, NULL, &lsError);

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

bool StatusBarServicesConnector::statusBarWifiPowerStateChangeCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->wifiPowerStateChangeCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::wifiPowerStateChangeCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	g_message("StatusBar - wifiPowerStateChangeCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		bool success = json_object_get_boolean(label);

		if(!success) {
			g_warning("StatusBar - Error Wifi Radio : %s", LSMessageGetPayload(message));
			updateAirplaneModeProgress(AP_MODE_WIFI);
		}
	}

	if (root && !is_error(root)) json_object_put(root);

	return true;
}


bool StatusBarServicesConnector::statusBarVpnServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->vpnServiceUpCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::vpnServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	g_message("StatusBar - vpnServiceUpCallback %s", LSMessageGetPayload(message));

	struct json_object* json = json_tokener_parse(LSMessageGetPayload(message));
	if (json && !is_error(json))  {

		json_object* label = json_object_object_get(json, "connected");
		if (label && json_object_is_type(label, json_type_boolean)) {

			bool connected = json_object_get_boolean(label);

			if (connected) {
				// VPN service up.
				bool result;
				LSError lsError;
				LSErrorInit(&lsError);

				result = LSCall(handle, "palm://com.palm.vpn/getProfileList", "{\"subscribe\":true}",
						statusBarHandleVPNStatusNotification, NULL, NULL, &lsError);

				if (LSErrorIsSet(&lsError)) {
					LSErrorPrint(&lsError, stderr);
					LSErrorFree(&lsError);
				}
			} else {
				Q_EMIT signalVpnStateChanged(false);
				m_vpnConnected = false;
				m_connectedVpnInfo.clear();
			}
		}
		json_object_put(json);
	}

	return true;
}

void StatusBarServicesConnector::requestVpnProfilesList()
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);

	result = LSCall(m_service, "palm://com.palm.vpn/getProfileList", "{\"subscribe\":true}",
			statusBarHandleVPNStatusNotification, NULL, NULL, &lsError);

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}

}

bool StatusBarServicesConnector::statusBarHandleVPNStatusNotification(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->handleVPNStatusNotification(handle, message, ctxt);
}

bool StatusBarServicesConnector::handleVPNStatusNotification(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));

	g_message("StatusBar - handleVPNStatusNotification %s", LSMessageGetPayload(message));

	struct json_object* label = 0;
	json_object* item = NULL;
	json_object* array = NULL;
	const char* state = 0;
	bool vpnConnected = false;

	array = json_object_object_get(root, "vpnProfiles");
	if (array && !is_error(array) && json_object_is_type(array, json_type_array)) {
		int numProfiles = json_object_array_length(array);
		int nValidProf = 0;
		t_vpnProfile profiles[numProfiles];

		m_vpnConnected = false;
		m_connectedVpnInfo.clear();

		for(int i = 0; i < numProfiles; i++) {
			sprintf(profiles[nValidProf].displayName, "%s", "");
			sprintf(profiles[nValidProf].connectionState, "%s", "");
			sprintf(profiles[nValidProf].profInfo, "%s", "");

			state = 0;
			item = json_object_array_get_idx(array, i);
			if (item && !is_error(item) && json_object_is_type(item, json_type_object)) {
				char* infoString = json_object_to_json_string(item);

				if(infoString)
					sprintf(profiles[nValidProf].profInfo, "%s", infoString);

				label = json_object_object_get(item, "vpnProfileName");
				if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
					sprintf(profiles[nValidProf].displayName, "%s", json_object_get_string(label));
				}

				label = json_object_object_get(item, "vpnProfileConnectState");
				if (label && !is_error(label) && json_object_is_type(label, json_type_string)) {
					sprintf(profiles[nValidProf].connectionState, "%s", json_object_get_string(label));
					if(label && !strcmp(profiles[nValidProf].connectionState, "connected")) {
						// VPN is connected
						vpnConnected = true;
						m_vpnConnected = true;
						m_connectedVpnInfo = infoString;
					}
				}
				nValidProf++;
			}
		}
		Q_EMIT signalVpnStateChanged(vpnConnected);
		Q_EMIT signalVpnProfileListUpdate(nValidProf, profiles);
	}

	if (root && !is_error(root)) json_object_put(root);

	return true;
}

void StatusBarServicesConnector::vpnConnectionRequest(bool connect, std::string profileInfo)
{
	bool result;
	LSError lsError;
	LSErrorInit(&lsError);
	std::string params = profileInfo;

	if(connect) {
		if(!m_vpnConnected) {
			result = LSCall(m_service, "palm://com.palm.vpn/connect", params.c_str(), NULL, NULL, NULL, &lsError);
		} else {
			m_pendingVpnProfile = profileInfo;
			result = LSCall(m_service, "palm://com.palm.vpn/disconnect", params.c_str(), statusBarVpnDisconnectResponse, NULL, NULL, &lsError);
		}
	} else {
		result = LSCall(m_service, "palm://com.palm.vpn/disconnect", params.c_str(), NULL, NULL, NULL, &lsError);
	}

	if (LSErrorIsSet(&lsError)) {
		LSErrorPrint(&lsError, stderr);
		LSErrorFree(&lsError);
	}
}

bool StatusBarServicesConnector::statusBarVpnDisconnectResponse(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->vpnDisconnectResponse(handle, message, ctxt);
}

bool StatusBarServicesConnector::vpnDisconnectResponse(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	g_message("StatusBar - vpnDisconnectResponse %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		if(json_object_get_boolean(label)) {
			// disconnected from VPN
			if(!m_pendingVpnProfile.empty()) {
				vpnConnectionRequest(true, m_pendingVpnProfile);
				m_pendingVpnProfile.clear();
			}
		}
	}

	return true;
}

bool StatusBarServicesConnector::statusBarGetSystemTimeCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	if(!s_instance)
		return true;
	return s_instance->getSystemTimeCallback(handle, message, ctxt);
}

bool StatusBarServicesConnector::getSystemTimeCallback(LSHandle* handle, LSMessage* message, void* ctxt)
{
	struct json_object* root = json_tokener_parse(LSMessageGetPayload(message));
	struct json_object* label = 0;

	g_message("StatusBar - getSystemTimeCallback %s", LSMessageGetPayload(message));

	label = json_object_object_get(root, "returnValue");
	if (label && !is_error(label) && json_object_is_type(label, json_type_boolean)) {
		// this is the subscription return value, safe to ignore
		return true;
	}

	// if we get this but it isn't the subscription return value, it is then due to the fact that the
	// system time was updated

	Q_EMIT signalSystemTimeChanged();

	return true;
}

void StatusBarServicesConnector::slotEnterBrickMode(bool)
{
	setRadioStatesForMSMMode(false);
}

void StatusBarServicesConnector::slotExitBrickMode()
{
	setRadioStatesForMSMMode(true);
}
