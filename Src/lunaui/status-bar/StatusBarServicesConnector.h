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




#ifndef STATUSBARSERVICESCONNECTOR_H
#define STATUSBARSERVICESCONNECTOR_H


#include <QObject>
#include <lunaservice.h>
#include <set>

#include "StatusBar.h"

struct json_object;


typedef struct WifiAccessPoint {
	char ssid[40];
	int	 profileId;
	int  signalBars;
	char securityType[20];
	bool connected;
	char connectionState[20];
} t_wifiAccessPoint;

typedef struct BluetoothDevice {
	char displayName[80];
	char connectionState[20];
	char btAddress[20];
	unsigned int cod;
	bool showConnected;
} t_bluetoothDevice;

typedef struct VpnProfile {
	char displayName[80];
	char connectionState[20];
	char profInfo[255];
} t_vpnProfile;

typedef enum RadioState {
	RADIO_OFF         = 0,
	RADIO_TURNING_ON  = 1,
	RADIO_TURNING_OFF = 2,
	RADIO_ON          = 3
} t_radioState;

typedef enum AirplaneModeState {
	AirplaneModeOff          = 0,
	AirplaneModeTurningOn    = 1,
	AirplaneModeTurningOff   = 2,
	AirplaneModeOn           = 3
} t_airplaneModeState;

typedef enum PhoneType {
	PHONE_TYPE_UNKNOWN  = 0,
	PHONE_TYPE_GSM,
	PHONE_TYPE_CDMA,
	PHONE_TYPE_NONE
} t_phoneType;


class StatusBarServicesConnector : public QObject
{
	Q_OBJECT

public:
	static StatusBarServicesConnector* instance();

	~StatusBarServicesConnector();

	void requestWifiAvailableNetworksList();
	void cancelWifiNetworksListRequest();
	void connectToWifiNetwork(std::string ssid, int profileId, std::string security);
	void setWifiOnState(bool on);
	void setBluetoothOnState(bool on);
	void requestTrustedBluetoothDevicesList();
	void cancelBluetoothDevicesListRequest();
	void requestBluetoothNumProfiles();
	void bluetoothProfileConnect(std::string profile, std::string address);
	void bluetoothProfileDisconnect(std::string profile, std::string address);
	bool isDeviceConnectedOnMenuProfiles(std::string address);
	void disconnectAllBtMenuProfiles(std::string address);
	bool isDeviceConnected(std::string address);
	void vpnConnectionRequest(bool connect, std::string profileInfo);
	void requestVpnProfilesList();

	t_phoneType getPhoneType() { return m_phoneType; }

private:

	void setDemoBuild(bool demo);
	void requestNetworkAndSignalStatus();
	void handleTILDisconnected();
	void setAirplaneMode(bool on);
	void setRadioStatesForMSMMode(bool on);
	void setTelephonyPowerState(bool on, bool saveState, bool msmMode = false);
	void updateAirplaneModeProgress(int radio);

	static bool statusBarStorageMSMEntryCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarStorageMSMProgressCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarPowerdServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarPowerdBatteryEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarPowerdChargerEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarGetBuildNameCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarPlatformQueryCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarTelephonyServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarTelephonyPowerEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarTelephonyNetworkEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarTelephonySignalEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarTelephonySIMEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarTelephonyPowerStateChangeCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarTelephonyTTYEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarTelephonyHACEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarWanServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarWanStatusEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarBtMonitorServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarBtMonitorEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarBtConnectedProfilesInfoCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarCallForwardRequestCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarBluetoothServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarBluetoothEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarBluetoothTurnOnCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarBluetoothTurnOffCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarBluetoothTruestedListCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarBluetoothNumProfilesCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarConnMgrServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarConnMgrEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarWiFiServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarWifiEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarWifiAvailableNetworksListCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarWifiConnectCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarWifiPowerStateChangeCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarVpnServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarHandleVPNStatusNotification(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarVpnDisconnectResponse(LSHandle* handle, LSMessage* message, void* ctxt);
	static bool statusBarGetSystemTimeCallback(LSHandle* handle, LSMessage* message, void* ctxt);

	bool storageMSMEntryCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool storageMSMProgressCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool powerdServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool powerdBatteryEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool powerdChargerEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool platformQueryCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool telephonyServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool telephonyPowerEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool telephonyNetworkEventsCallback(LSHandle* handle, const char* messagePayload, void* ctxt);
	bool telephonySignalEventsCallback(LSHandle* handle, const char* messagePayload, void* ctxt);
	bool telephonySIMEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool telephonyPowerStateChangeCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool telephonyTTYEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool telephonyHACEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool wanServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool wanStatusEventsCallback(LSHandle* handle, const char* messagePayload, void* ctxt);
	bool btMonitorServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool btMonitorEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool btConnectedProfilesInfoCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool callForwardRequestCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool bluetoothServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool bluetoothEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool bluetoothTurnOnCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool bluetoothTurnOffCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool bluetoothTruestedListCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool bluetoothNumProfilesCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool connMgrServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool connMgrEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool wiFiServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool wifiEventsCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool wifiAvailableNetworksListCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool wifiConnectCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool wifiPowerStateChangeCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool vpnServiceUpCallback(LSHandle* handle, LSMessage* message, void* ctxt);
	bool handleVPNStatusNotification(LSHandle* handle, LSMessage* message, void* ctxt);
	bool vpnDisconnectResponse(LSHandle* handle, LSMessage* message, void* ctxt);
	bool getSystemTimeCallback(LSHandle* handle, LSMessage* message, void* ctxt);

	void handlePowerStatus(const char* radioState, bool queryResponse = false);
	void handleNetworkStatus(const char* networkState, struct json_object* event);


Q_SIGNALS:

	void signalPhoneTypeUpdated();
	void signalPowerdConnectionStateChanged(bool connected);
	void signalBatteryLevelUpdated(int percentage);
	void signalChargingStateUpdated(bool charging);
	void signalCarrierTextChanged(const char* text);
	void signalRssiIndexChanged(bool show, StatusBar::IndexRSSI index);
	void signalRssi1xIndexChanged(bool show, StatusBar::IndexRSSI1x index);
	void signalTTYStateChanged(bool enabled);
	void signalHACStateChanged(bool enabled);
	void signalCallForwardStateChanged(bool enabled);
	void signalRoamingStateChanged(bool enabled);
	void signalVpnStateChanged(bool enabled);
	void signalWanIndexChanged(bool show, StatusBar::IndexWAN index);
	void signalBluetoothIndexChanged(bool show, StatusBar::IndexBluetooth index);
	void signalWifiIndexChanged(bool show, StatusBar::IndexWiFi index);
	void signalSystemTimeChanged();


	// Signals for the System Menu
	void signalWifiStateChanged(bool wifiOn, bool wifiConnected, std::string wifiSSID, std::string wifiConnState);
	void signalWifiAvailableNetworksListUpdate(int numNetworks, t_wifiAccessPoint* list);
	void signalBluetoothTurnedOn();
	void signalBluetoothPowerStateChanged(t_radioState radioState);
	void signalBluetoothConnStateChanged(bool btConnected, std::string deviceName);
	void signalBluetoothTrustedDevicesUpdate(int numTrustedDevices, t_bluetoothDevice* list);
	void signalBluetoothParedDevicesAvailable(bool available);
	void signalBluetoothUpdateDeviceStatus(t_bluetoothDevice* deviceStatus);
	void signalVpnProfileListUpdate(int numProfiles, t_vpnProfile* list);
	void signalAirplaneModeState(t_airplaneModeState state);


private Q_SLOTS:
	void slotAirplaneModeChanged(bool enabled);
	void slotRoamingIndicatorChanged();
	void slotEnterBrickMode(bool);
	void slotExitBrickMode();

private:

	char m_carrierText[kMaxTitleLength];
	char m_appTitle[kMaxTitleLength];
	bool m_appMaximized;
	bool m_showAppTitle;

	enum PhoneService {
		NoService  = 0,
		Service,
		Limited,
		Searching
	};

	enum BluetoothState {
		BT_DISCONNECTED  = 0,
		BT_CONNECTING,
		BT_CONNECTED,
		BT_DISCONNECTING
	};

	struct BluetoothProfState {
		BluetoothState status;
		std::string    address;
		std::string    name;
	};

	std::vector<unsigned int>	     m_SIMRejectCodes;
	std::vector<std::string>         m_bluetoothProfiles;
	std::set<std::string>            m_bluetoothMenuProfiles;
	std::map<std::string, BluetoothProfState> m_bluetoothProfileStates;

	static const int kNumChargeSources = 2;
	static const char* m_chargeSource[kNumChargeSources];
	bool m_powerdConnected;
	int  m_batteryLevel;
	bool m_charging;

	PhoneType m_phoneType;
	bool m_phoneRadioState;
	bool m_simBad;
	bool m_ruim;
	bool m_simLocked;
	bool m_phoneInLimitedService;
	PhoneService m_phoneService;

	bool m_callFwdStatusRequested;

	int m_rssi;

	bool m_demoBuild;
	bool m_airplaneMode;
	bool m_initialAirplaneModeStatus;
	bool m_airplaneModeTriggered;
	bool m_apModePhone;
	bool m_apModeWifi;
	bool m_apModeBluetooth;
	bool m_msmStartingRadiosInProgress;
	bool m_msmModePhone;
	bool m_msmModeWifi;
	bool m_msmModeBluetooth;

	bool m_showBlankStatusOnLimited;
	bool m_bluetoothRadionOn;
	bool m_btRadioTurningOn;
	std::string m_wifiSSID;
	bool m_wifiRadioOn;
	bool m_wifiConnected;
	LSMessageToken m_wifiFindNetworksReq;
	LSMessageToken m_bluetoothDeviceListReq;

	bool m_vpnConnected;
	std::string m_connectedVpnInfo;
	std::string m_pendingVpnProfile;

	bool m_hideDataIcon;
	bool m_isInternetConnectionAvailable;

	LSHandle* m_service;

	std::string m_cmPayloadBuffer;
	std::string m_signalMsgPayloadBuffer;
	std::string m_phoneEventNetworkPayload;


	StatusBarServicesConnector();
	void init();

	void requestBluetoothConnectedProfilesInfo();
	void requestCallForwardStatus();

	void updateBluetoothIcon();
	void updateBtDeviceInfo(BluetoothProfState* info);

	bool validSIMRejectCode(unsigned int code);
	void updateRSSIIcon(bool show, StatusBar::IndexRSSI index);
	void updateRSSI1xIcon(bool show, StatusBar::IndexRSSI1x index);
	StatusBar::IndexWAN getWanIndex(bool connected, const char* type);

};


#endif /* STATUSBARSERVICESCONNECTOR_H */
