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




#include "SystemMenu.h"

#include <QPainter>
#include <QDeclarativeEngine>
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
#include <QTimer>
#include <QKeyEvent>
#include <QApplication>
#include <QGraphicsView>
#include <algorithm>
#include <string>
#include "stdio.h"
#include "Settings.h"
#include "Localization.h"
#include "QtUtils.h"
#include "Logging.h"
#include "StatusBarServicesConnector.h"
#include "WebAppMgrProxy.h"
#include "DeviceInfo.h"
#include "Preferences.h"
#include "BtDeviceClass.h"
#include "DisplayManager.h"
#include "WindowServer.h"
#include "SystemUiController.h"

#define SYS_UI_APP_ID      "com.palm.systemui"
#define WIFI_PREFS_APP_ID  "com.palm.app.wifi"
#define BLUETOOTH_PREFS_APP_ID  "com.palm.app.bluetooth"
#define VPN_PREFS_APP_ID   "com.palm.app.vpn"

#define MINIMUM_BRIGHTNESS  10

static const int kDateTimerInterval = 30000; // Tick every 30 seconds

static std::string airplaneModeStateText[4] = { // $$$ LOCALIZE
		"Turn on Airplane Mode",
		"Turning on Airplane Mode",
		"Turning off Airplane Mode",
		"Turn off Airplane Mode"
};

SystemMenu::SystemMenu(int width, int height, bool restricted)
	: m_qmlMenu(0)
	, m_menuObject(0)
	, m_wifiMenu(0)
	, m_vpnMenu(0)
	, m_bluetoothMenu(0)
	, m_menuHandler(0)
	, m_rightEdgeOffset(0)
	, m_airplaneModeState(AirplaneModeOff)
	, m_opened(false)
	, m_wifiMenuOpened(false)
	, m_wifiOn(false)
	, m_bluetoothPower(RADIO_OFF)
	, m_bluetoothMenuOpened(false)
	, m_btPairedDevicesAvailable(false)
	, m_btTurnOnRequested(false)
	, m_vpnMenuOpened(false)
	, m_restricted (restricted)
	, m_dateTimer(new QTimer(this))
{
	qmlRegisterType<AnimatedSpinner>("SystemMenu", 1,0, "AnimatedSpinner");

	m_bounds = QRect(-160, -240, 320, 480);

	m_menuHandler = new MenuHandler(this);

	m_pendingDevAddress.clear();
	m_pendingCod = 0;
	setAcceptTouchEvents(true);
}


SystemMenu::~SystemMenu()
{
	if(m_dateTimer->isActive())
		 m_dateTimer->stop();

	delete m_dateTimer;
}

void SystemMenu::setOpened(bool opened)
{
	m_opened = opened;

	if(!m_opened) {
		// reset on close
		QMetaObject::invokeMethod(m_menuObject, "flagMenuReset");
	}

	if(m_opened) {
		if(m_bluetoothPower == RADIO_ON)
				StatusBarServicesConnector::instance()->requestTrustedBluetoothDevicesList();
			else
				StatusBarServicesConnector::instance()->requestBluetoothNumProfiles();
	}

	if(m_opened) {
		tick();
 		m_dateTimer->start();
 	} else {
 		m_dateTimer->stop();
 	}
}

void SystemMenu::init()
{
	QDeclarativeEngine* qmlEngine = WindowServer::instance()->declarativeEngine();

	// Instantiates and initialized the services connector
	StatusBarServicesConnector* svcConnector = StatusBarServicesConnector::instance();

	if(svcConnector) {
		connect(svcConnector,SIGNAL(signalPowerdConnectionStateChanged(bool)), this, SLOT(slotPowerdConnectionStateChanged(bool)));
		connect(svcConnector,SIGNAL(signalBatteryLevelUpdated(int)), this, SLOT(slotBatteryLevelUpdated(int)));
		connect(svcConnector,SIGNAL(signalWifiStateChanged(bool, bool, std::string, std::string)), this, SLOT(slotWifiStateChanged(bool, bool, std::string, std::string)));
		connect(svcConnector,SIGNAL(signalWifiAvailableNetworksListUpdate(int, t_wifiAccessPoint*)), this, SLOT(slotWifiAvailableNetworksListUpdate(int, t_wifiAccessPoint*)));
		connect(svcConnector,SIGNAL(signalBluetoothTurnedOn()), this, SLOT(slotBluetoothTurnedOn()));
		connect(svcConnector,SIGNAL(signalBluetoothPowerStateChanged(t_radioState)), this, SLOT(slotBluetoothPowerStateChanged(t_radioState)));
		connect(svcConnector,SIGNAL(signalBluetoothConnStateChanged(bool, std::string)), this, SLOT(slotBluetoothConnStateChanged(bool, std::string)));
		connect(svcConnector,SIGNAL(signalBluetoothTrustedDevicesUpdate(int, t_bluetoothDevice*)), this, SLOT(slotBluetoothTrustedDevicesUpdate(int, t_bluetoothDevice*)));
		connect(svcConnector,SIGNAL(signalBluetoothParedDevicesAvailable(bool)), this, SLOT(slotBluetoothParedDevicesAvailable(bool)));
		connect(svcConnector,SIGNAL(signalBluetoothUpdateDeviceStatus(t_bluetoothDevice*)), this, SLOT(slotBluetoothUpdateDeviceStatus(t_bluetoothDevice*)));
		connect(svcConnector,SIGNAL(signalVpnProfileListUpdate(int, t_vpnProfile*)), this, SLOT(slotVpnProfileListUpdate(int, t_vpnProfile*)));
		connect(svcConnector,SIGNAL(signalVpnStateChanged(bool)), SLOT(slotVpnStateChanged(bool)));
		connect(svcConnector,SIGNAL(signalAirplaneModeState(t_airplaneModeState)), this, SLOT(slotAirplaneModeState(t_airplaneModeState)));
		connect(svcConnector,SIGNAL(signalSystemTimeChanged()), this, SLOT(slotSystemTimeChanged()));
	}

	 if(qmlEngine) {
		 QDeclarativeContext* context =	qmlEngine->rootContext();
		 if(context) {
			 context->setContextProperty("NativeSystemMenuHandler", m_menuHandler);
		 }

		 Settings* settings = Settings::LunaSettings();
		 std::string systemMenuQmlPath = settings->lunaQmlUiComponentsPath + "SystemMenu/SystemMenu.qml";
		 QUrl url = QUrl::fromLocalFile(systemMenuQmlPath.c_str());
		 m_qmlMenu = new QDeclarativeComponent(qmlEngine, url, this);
		 if(m_qmlMenu) {
			 m_menuObject = qobject_cast<QGraphicsObject *>(m_qmlMenu->create());
			 if(m_menuObject) {
				 m_menuObject->setParentItem(this);
				 m_rightEdgeOffset = m_menuObject->property("edgeOffset").toInt();
				 prepareGeometryChange();
				 m_bounds = QRect(-m_menuObject->boundingRect().width()/2, -m_menuObject->boundingRect().height()/2,
						          m_menuObject->boundingRect().width(), m_menuObject->boundingRect().height());
				 m_menuObject->setPos(m_bounds.x(),m_bounds.y());

				 // locate the children menu objects
				 m_wifiMenu = m_menuObject->findChild<QGraphicsObject*>("wifiMenu");
				 m_bluetoothMenu = m_menuObject->findChild<QGraphicsObject*>("bluetoothMenu");
				 m_vpnMenu = m_menuObject->findChild<QGraphicsObject*>("vpnMenu");

				 // Connect the menu signals
				 connect(m_menuObject,SIGNAL(closeSystemMenu()), SLOT(slotCloseSystemMenu()));
				 connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceChangeFinished(QRect)), SLOT(slotPositiveSpaceChangeFinished(QRect)));

                 if(DeviceInfo::instance()->wifiAvailable() && m_wifiMenu) {
                	 m_wifiMenu->setProperty("visible", true);
			 if (!m_restricted) {
					 connect(m_wifiMenu,SIGNAL(menuOpened()), SLOT(slotWifiMenuOpened()));
					 connect(m_wifiMenu,SIGNAL(menuClosed()), SLOT(slotWifiMenuClosed()));
					 connect(m_wifiMenu,SIGNAL(onOffTriggered()), SLOT(slotWifiOnOffTriggered()));
					 connect(m_wifiMenu,SIGNAL(prefsTriggered()), SLOT(slotWifiPrefsTriggered()));
					 connect(m_wifiMenu,SIGNAL(itemSelected(int, QString, int, QString, QString)),
							 SLOT(slotWifiNetworkSelected(int, QString, int, QString, QString)));
			 }
			 else {
				 m_wifiMenu->setProperty ("active", false);
			 }
                 }

                 if(DeviceInfo::instance()->bluetoothAvailable() && m_bluetoothMenu) {
                	 m_bluetoothMenu->setProperty("visible", true);
			 if (!m_restricted) {
					 connect(m_bluetoothMenu,SIGNAL(menuOpened()), SLOT(slotBluetoothMenuOpened()));
					 connect(m_bluetoothMenu,SIGNAL(menuClosed()), SLOT(slotBluetoothMenuClosed()));
					 connect(m_bluetoothMenu,SIGNAL(onOffTriggered()), SLOT(slotBluetoothOnOffTriggered()));
					 connect(m_bluetoothMenu,SIGNAL(prefsTriggered()), SLOT(slotBluetoothPrefsTriggered()));
					 connect(m_bluetoothMenu,SIGNAL(itemSelected(int)), SLOT(slotBluetoothDeviceSelected(int)));
			 }
			 else {
				 m_bluetoothMenu->setProperty ("active", false);
			 }
                 }

                 if(m_vpnMenu) {
                	 m_vpnMenu->setProperty("visible", true);

             		QMetaObject::invokeMethod(m_vpnMenu, "setVpnState",
             								  Q_ARG(QVariant, false),
             								  Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Off"))));

			if (!m_restricted) {
					 connect(m_vpnMenu,SIGNAL(menuOpened()), SLOT(slotVpnMenuOpened()));
					 connect(m_vpnMenu,SIGNAL(menuClosed()), SLOT(slotVpnMenuClosed()));
					 connect(m_vpnMenu,SIGNAL(prefsTriggered()), SLOT(slotVpnPrefsTriggered()));
					 connect(m_vpnMenu,SIGNAL(itemSelected(QString, QString, QString)), SLOT(slotVpnNetworkSelected(QString, QString, QString)));

					 svcConnector->requestVpnProfilesList();
			}
			 else {
				 m_vpnMenu->setProperty ("active", false);
			 }
                 }

		 if (!m_restricted) {
				 connect(m_menuObject,SIGNAL(airplaneModeTriggered()), SLOT(slotAirplaneModeTriggered()));
		 }
		 else {
				 QGraphicsObject* airplaneMode = m_menuObject->findChild<QGraphicsObject*>("airplaneMode");
				 if (airplaneMode)
					 airplaneMode->setProperty ("selectable", false);
		 }
				 connect(m_menuObject,SIGNAL(rotationLockTriggered(bool)), SLOT(slotRotationLockTriggered(bool)));
				 connect(m_menuObject,SIGNAL(muteToggleTriggered(bool)), SLOT(slotMuteToggleTriggered(bool)));
				 connect(m_menuObject,SIGNAL(menuBrightnessChanged(qreal, bool)), SLOT(slotMenuBrightnessChanged(qreal, bool)));

                 connect(Preferences::instance(),SIGNAL(signalRotationLockChanged(OrientationEvent::Orientation)), SLOT(slotRotationLockChanged(OrientationEvent::Orientation)));
				 connect(Preferences::instance(),SIGNAL(signalMuteSoundChanged(bool)), SLOT(slotMuteSoundChanged(bool)));
				 connect(DisplayManager::instance(),SIGNAL(signalDisplayMaxBrightnessChanged(int)), SLOT(slotDisplayMaxBrightnessChanged(int)));

				 setAirplaneModeState(AirplaneModeOff);

				 int currBrightness = DisplayManager::instance()->getMaximumBrightness();
				 QMetaObject::invokeMethod(m_menuObject, "setSystemBrightness", Q_ARG(QVariant, (qreal)(((qreal)currBrightness) / 100.0)));

			 }
		 }
	 }

	m_dateTimer->setInterval(kDateTimerInterval);
	connect(m_dateTimer, SIGNAL(timeout()), this, SLOT(tick()));
	::memset(&m_lastUpdateDate, 0, sizeof(m_lastUpdateDate));
}

QRectF SystemMenu::boundingRect() const
{
	return m_bounds;
}

void SystemMenu::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{

}

bool SystemMenu::sceneEvent(QEvent* event)
{
	if(m_opened) {
		if (event->type() == QEvent::GestureOverride) {
			QGestureEvent* ge = static_cast<QGestureEvent*>(event);
			ge->accept();
			return true;
		}
		else if (event->type() == QEvent::TouchBegin)
		{
			return true;
		}
		else if (event->type() == QEvent::TouchUpdate)
		{
			return true;
		}
		else if (event->type() == QEvent::TouchEnd)
		{
			return true;
		}
	}

	return QGraphicsObject::sceneEvent(event);
}


void SystemMenu::launchApp(std::string appId, std::string params)
{
	std::string launchingAppId = SYS_UI_APP_ID;
	std::string launchingProcId;
	std::string errMsg;

	std::string procId = WebAppMgrProxy::instance()->appLaunch(
	                         appId,
	                         params,
	                         launchingAppId,
	                         launchingProcId,
	                         errMsg);

	if (procId.empty()) {
		g_warning("Failed to launch %s : %s", appId.c_str(), errMsg.c_str());
	}

}

void SystemMenu::slotCloseSystemMenu()
{
	Q_EMIT signalCloseMenu();
	StatusBarServicesConnector::instance()->cancelWifiNetworksListRequest();
}

// ----------------------------------  WiFi functionality -----------------------------------------

void SystemMenu::slotWifiMenuOpened()
{
	m_wifiMenuOpened = true;
	if(m_wifiOn)
		StatusBarServicesConnector::instance()->requestWifiAvailableNetworksList();
}

void SystemMenu::slotWifiMenuClosed()
{
	m_wifiMenuOpened = false;
	if(m_wifiOn)
		StatusBarServicesConnector::instance()->cancelWifiNetworksListRequest();
}

void SystemMenu::slotWifiOnOffTriggered()
{
	if(m_wifiOn) {
		StatusBarServicesConnector::instance()->setWifiOnState(false);
		StatusBarServicesConnector::instance()->cancelWifiNetworksListRequest();
	} else {
		StatusBarServicesConnector::instance()->setWifiOnState(true);
	}
}

void SystemMenu::slotWifiPrefsTriggered()
{
	launchApp(WIFI_PREFS_APP_ID, "");
}

void SystemMenu::slotWifiNetworkSelected(int index, QString name, int profileId, QString securityType, QString connStatus)
{
	if (m_restricted)
		return;

	if(!connStatus.isEmpty() && (connStatus == "connecting")) {
		return; // already connecting, do nothing
	} else if(!connStatus.isEmpty() && ((connStatus == "ipConfigured") || (connStatus == "associated") ||
			                            (connStatus == "ipFailed") || (connStatus == "associationFailed"))  ) {
		//Launch WiFi Panel with Target Parameter.
		char params[255];
		sprintf(params,"{\"target\": {\"ssid\": \"%s\", \"securityType\": \"%s\", \"profileId\": %d, \"connectState\": \"%s\"},}",
				       name.toAscii().data(), securityType.toAscii().data(), profileId, connStatus.toAscii().data());
		launchApp(WIFI_PREFS_APP_ID, params);
	} else {
		if(profileId) {
			// Network already has a profile
			StatusBarServicesConnector::instance()->connectToWifiNetwork(name.toStdString(), profileId, securityType.toStdString());
			QMetaObject::invokeMethod(m_wifiMenu, "wifiConnectStateUpdate", Q_ARG(QVariant, false), Q_ARG(QVariant, name), Q_ARG(QVariant, "userSelected"));
		} else {
			if(!securityType.isEmpty()) {
				//Launch WiFi Panel with Target Parameter.
				char params[255];
				sprintf(params,"{\"target\": {\"ssid\": \"%s\", \"securityType\": \"%s\"},}",
						       name.toAscii().data(), securityType.toAscii().data());
				launchApp(WIFI_PREFS_APP_ID, params);
			} else {
				StatusBarServicesConnector::instance()->connectToWifiNetwork(name.toStdString(), profileId, securityType.toStdString());
				QMetaObject::invokeMethod(m_wifiMenu, "wifiConnectStateUpdate", Q_ARG(QVariant, false), Q_ARG(QVariant, name), Q_ARG(QVariant, "userSelected"));
			}
		}
	}
}

void SystemMenu::slotWifiStateChanged(bool wifiOn, bool wifiConnected, std::string wifiSSID, std::string wifiConnState)
{
	if(m_menuHandler) {

		std::string wifiMsg;
		if(!wifiOn) {
			wifiMsg = LOCALIZED("OFF");  // $$$ LOCALIZE
		} else {
			if (wifiSSID.empty()) {
				wifiMsg = LOCALIZED("ON");  // $$$ LOCALIZE
			} else {
				if((wifiConnState == "associated") ||
				   (wifiConnState == "associating") ||
				   (wifiConnState == "ipConfigured")  ) {
					wifiMsg = wifiSSID;
				} else {
					wifiMsg = LOCALIZED("ON");  // $$$ LOCALIZE
				}
			}
		}

		QMetaObject::invokeMethod(m_wifiMenu, "setWifiState",
								  Q_ARG(QVariant, wifiOn),
								  Q_ARG(QVariant, fromStdUtf8(wifiMsg)));

		if(m_wifiMenuOpened) {
			QMetaObject::invokeMethod(m_wifiMenu, "wifiConnectStateUpdate",
									  Q_ARG(QVariant, wifiConnected),
									  Q_ARG(QVariant, wifiSSID.c_str()),
									  Q_ARG(QVariant, wifiConnState.c_str()));
		}

		if(!m_wifiOn && wifiOn && m_wifiMenuOpened) {
			StatusBarServicesConnector::instance()->requestWifiAvailableNetworksList();
		}

		m_wifiOn = wifiOn;
	}
}

void SystemMenu::slotWifiAvailableNetworksListUpdate(int numNetworks, t_wifiAccessPoint* list)
{
	StatusBarServicesConnector::instance()->cancelWifiNetworksListRequest();
	if(m_menuHandler) {
		QMetaObject::invokeMethod(m_wifiMenu, "clearWifiList");

		if(m_wifiOn && (numNetworks > 0)) {
			for(int x = 0; x < numNetworks; x++) {
				QMetaObject::invokeMethod(m_wifiMenu, "addWifiNetworkEntry",
										  Q_ARG(QVariant, QString::fromUtf8(list[x].ssid)),
										  Q_ARG(QVariant, list[x].profileId),
										  Q_ARG(QVariant, CLAMP(list[x].signalBars, 0, 3)),
										  Q_ARG(QVariant, list[x].securityType),
										  Q_ARG(QVariant, list[x].connectionState),
										  Q_ARG(QVariant, list[x].connected));
			}
		}

		if(m_wifiMenuOpened) {
			QMetaObject::invokeMethod(m_wifiMenu, "adjustViewIfNecessary");
		}

		m_menuHandler->signalWifiListUpdated();
	}
}

// ----------------------------------  Bluetooth functionality -----------------------------------------

void SystemMenu::slotBluetoothMenuOpened()
{
	m_bluetoothMenuOpened = true;
	m_btTurnOnRequested = false;
	if(m_bluetoothPower == RADIO_ON)
		StatusBarServicesConnector::instance()->requestTrustedBluetoothDevicesList();
	else
		StatusBarServicesConnector::instance()->requestBluetoothNumProfiles();
}

void SystemMenu::slotBluetoothMenuClosed()
{
	m_bluetoothMenuOpened = false;
}


void SystemMenu::slotBluetoothOnOffTriggered()
{
	m_btTurnOnRequested = false;
	if(m_bluetoothPower == RADIO_ON) {
		StatusBarServicesConnector::instance()->setBluetoothOnState(false);
	} else if(m_bluetoothPower != RADIO_TURNING_ON) {
		m_btTurnOnRequested = true;
		StatusBarServicesConnector::instance()->setBluetoothOnState(true);
	}
}

void SystemMenu::slotBluetoothPrefsTriggered()
{
	launchApp(BLUETOOTH_PREFS_APP_ID, "");
}

void SystemMenu::slotBluetoothDeviceSelected(int index)
{
	if(m_bluetoothPower == RADIO_ON) {
		if(m_trustedDevices.size() > (unsigned int)index) {
			g_message("Bluetooth Device Selected. Name = %s, address = %s, state = %s, cod = %d", m_trustedDevices[index].displayName, m_trustedDevices[index].btAddress, m_trustedDevices[index].connectionState,  m_trustedDevices[index].cod);
			if(!strcmp(m_trustedDevices[index].connectionState, "connected")) {
				// Device is connected so disconnect all profiles it is connected on
				StatusBarServicesConnector::instance()->disconnectAllBtMenuProfiles(m_trustedDevices[index].btAddress);
			} else if(!strcmp(m_trustedDevices[index].connectionState, "connecting")) {
				// Is this device waiting to connect?
				if (m_trustedDevices[index].btAddress == m_pendingDevAddress) {
					// There is no longer a pending connection attempt to this device
					m_pendingDevAddress.clear();
					m_pendingCod = 0;
				}
				else {
					// Abort the connection attempt on the supported profiles
					StatusBarServicesConnector::instance()->disconnectAllBtMenuProfiles(m_trustedDevices[index].btAddress);
				}
			} else if(!strcmp(m_trustedDevices[index].connectionState, "disconnected")) {
				// Is there a pending connection attempt to another device?
				if ((!m_pendingDevAddress.empty()) && (m_pendingDevAddress != m_trustedDevices[index].btAddress )) {
					// A connection to the previously pending device is no longer desired

					// Update the UI to reflect that there is no longer a pending connection attempt to the old device
					// Iterate through the list array to find out the index
					for(unsigned int x = 0; x < m_trustedDevices.size(); x++) {
						if(strcmp(m_trustedDevices[x].btAddress, m_pendingDevAddress.c_str())) {
							sprintf(m_trustedDevices[x].connectionState, "disconnected");
							slotBluetoothUpdateDeviceStatus(&(m_trustedDevices[index]));
						}
					}
					m_pendingDevAddress.clear();
					m_pendingCod = 0;
				}

				// Iterate through the list and disconnect the audio profiles
				for(unsigned int x = 0; x < m_trustedDevices.size(); x++) {
					if(strcmp(m_trustedDevices[x].connectionState, "disconnected")) {
						StatusBarServicesConnector::instance()->disconnectAllBtMenuProfiles(m_trustedDevices[x].btAddress);
						m_pendingDevAddress = m_trustedDevices[index].btAddress;
						m_pendingCod = m_trustedDevices[index].cod;
					}
				}

				// The device the user tapped on must show "connecting"
				sprintf(m_trustedDevices[index].connectionState, "connecting");
				slotBluetoothUpdateDeviceStatus(&(m_trustedDevices[index]));

				// If the connection isn't pending then connect now
				if (m_pendingDevAddress.empty())
					connectBtAudioDevice(m_trustedDevices[index].btAddress, m_trustedDevices[index].cod);
			}
		} else {
			g_warning("Bluetooth Device index Not Found: %d", index);
		}
	}
}

void SystemMenu::slotBluetoothTurnedOn()
{
	if(!m_btPairedDevicesAvailable && m_btTurnOnRequested) {
		launchApp(BLUETOOTH_PREFS_APP_ID, "");
	}
	m_btTurnOnRequested = false;
}

void SystemMenu::slotBluetoothPowerStateChanged(t_radioState radioState)
{
	if(m_menuHandler) {

		std::string btMsg;
		bool turningOn = false;

		if(radioState != RADIO_ON) {
			btMsg = LOCALIZED("OFF");  // $$$ LOCALIZE
			if(radioState == RADIO_TURNING_ON)
				turningOn = true;

			QMetaObject::invokeMethod(m_bluetoothMenu, "clearBluetoothList");
			m_trustedDevices.clear();
		} else {
			btMsg = LOCALIZED("ON");  // $$$ LOCALIZE
			if(m_bluetoothMenuOpened) {
				StatusBarServicesConnector::instance()->requestTrustedBluetoothDevicesList();
			}
		}

		QMetaObject::invokeMethod(m_bluetoothMenu, "setBluetoothState",
				  	  	  	  	  Q_ARG(QVariant, (radioState == RADIO_ON)),
				  	  	  	  	  Q_ARG(QVariant, turningOn),
								  Q_ARG(QVariant, fromStdUtf8(btMsg)));

		m_bluetoothPower = radioState;
	}
}

void SystemMenu::slotBluetoothConnStateChanged(bool btConnected, std::string deviceName)
{
	bool turningOn = false;
	std::string btMsg;

	if(btConnected) {
		if(m_bluetoothPower == RADIO_TURNING_ON) {
			//This is the case where notifyradioon was missed and got notifyconnected directly.
			slotBluetoothPowerStateChanged(RADIO_ON);
		}
		btMsg = deviceName;
	} else {
		if(m_bluetoothPower != RADIO_ON) {
			btMsg = LOCALIZED("OFF");  // $$$ LOCALIZE
			if(m_bluetoothPower == RADIO_TURNING_ON)
				turningOn = true;
		} else {
			btMsg = LOCALIZED("ON");  // $$$ LOCALIZE

			// device just got Disconnected
			if(!m_pendingDevAddress.empty()) {
				// connect pending device
				connectBtAudioDevice(m_pendingDevAddress, m_pendingCod);
				m_pendingDevAddress.clear();
				m_pendingCod = 0;
			}
		}
	}

	QMetaObject::invokeMethod(m_bluetoothMenu, "setBluetoothState",
			  	  	  	  	  Q_ARG(QVariant, (m_bluetoothPower == RADIO_ON)),
			  	  	  	  	  Q_ARG(QVariant, turningOn),
							  Q_ARG(QVariant,  QString::fromUtf8(btMsg.c_str())));
}

void SystemMenu::slotBluetoothTrustedDevicesUpdate(int numTrustedDevices, t_bluetoothDevice* list)
{
	StatusBarServicesConnector::instance()->cancelBluetoothDevicesListRequest();
	if(m_menuHandler) {
		bool updateMenu = false;

		if((int)(m_trustedDevices.size()) != numTrustedDevices) {
			updateMenu = true;
		} else {
			for(int x = 0; x < numTrustedDevices; x++) {
				if(strcmp(list[x].displayName, m_trustedDevices[x].displayName)) {
					updateMenu = true;
					break;
				}
				if(strcmp(list[x].btAddress, m_trustedDevices[x].btAddress)) {
					updateMenu = true;
					break;
				}
				if(list[x].cod != m_trustedDevices[x].cod) {
					updateMenu = true;
					break;
				}
				if(strcmp(list[x].connectionState, m_trustedDevices[x].connectionState)) {
					updateMenu = true;
					break;
				}
				if(list[x].showConnected != m_trustedDevices[x].showConnected) {
					updateMenu = true;
					break;
				}
			}
		}

		if(updateMenu) {
			QMetaObject::invokeMethod(m_bluetoothMenu, "clearBluetoothList");
			m_trustedDevices.clear();
			m_trustedDevices.resize(numTrustedDevices);

			if((m_bluetoothPower == RADIO_ON) && numTrustedDevices && list) {
				for (int x = 0; x < numTrustedDevices; x++) {
					QMetaObject::invokeMethod(m_bluetoothMenu, "addBluetoothEntry",
											  Q_ARG(QVariant, QString::fromUtf8(list[x].displayName)),
											  Q_ARG(QVariant, list[x].btAddress),
											  Q_ARG(QVariant, list[x].cod),
											  Q_ARG(QVariant, list[x].connectionState),
											  Q_ARG(QVariant, list[x].showConnected));
					memcpy(&(m_trustedDevices[x]), &(list[x]), sizeof(t_bluetoothDevice));
				}

				if(m_bluetoothMenuOpened) {
					QMetaObject::invokeMethod(m_bluetoothMenu, "adjustViewIfNecessary");
				}
			}
		}
	}
}

void SystemMenu::slotBluetoothUpdateDeviceStatus(t_bluetoothDevice* deviceStatus)
{
	bool skipUiUpdate = false;
	int profIndex = -1;

	for (unsigned int x = 0; x < m_trustedDevices.size(); x++) {
		if(!strcmp(deviceStatus->btAddress, m_trustedDevices[x].btAddress)) {
			profIndex = x;
			break;
		}
	}

	if(profIndex >= 0) {

		if(strcmp(m_trustedDevices[profIndex].connectionState, "connected") && !strcmp(deviceStatus->connectionState, "connected")) {
			// device just got connected
			if(!m_pendingDevAddress.empty() && strcmp(m_pendingDevAddress.c_str(), deviceStatus->btAddress)) {
				// The device is connected, but if there is a pending connection to a
				// different device then disconnect this connection
				StatusBarServicesConnector::instance()->disconnectAllBtMenuProfiles(m_trustedDevices[profIndex].btAddress);
				skipUiUpdate = true;
			}
		}

		if(!skipUiUpdate) {
			QMetaObject::invokeMethod(m_bluetoothMenu, "updateBluetoothEntry",
									  Q_ARG(QVariant, QString::fromUtf8(deviceStatus->displayName)),
									  Q_ARG(QVariant, deviceStatus->btAddress),
									  Q_ARG(QVariant, deviceStatus->cod),
									  Q_ARG(QVariant, deviceStatus->connectionState),
									  Q_ARG(QVariant, deviceStatus->showConnected));
		}

		// update our local records
		unsigned int cod = m_trustedDevices[profIndex].cod;
		memcpy(&(m_trustedDevices[profIndex]), deviceStatus, sizeof(t_bluetoothDevice));
		m_trustedDevices[profIndex].cod = cod;
	}
}

void SystemMenu::slotBluetoothParedDevicesAvailable(bool available)
{
	m_btPairedDevicesAvailable = available;
}

void SystemMenu::connectBtAudioDevice(std::string address, unsigned int cod)
{
//	this.btConnectedDeviceAddr = null;
	// If HFG is supported then do associate and connect to that profile
	if (BtDeviceClass::isHFGSupported(cod)) {
		StatusBarServicesConnector::instance()->bluetoothProfileConnect("hfg", address);
	}

	// If it is a phone, HF and MAP are supported, then connect to those profile
	if (BtDeviceClass::isPhone(cod)) {
		StatusBarServicesConnector::instance()->bluetoothProfileConnect("hf", address);
		StatusBarServicesConnector::instance()->bluetoothProfileConnect("mapc", address);
	}

	// If A2DP is supported then do associate and connect to that profile
	if (BtDeviceClass::isA2DPSupported(cod)) {
		StatusBarServicesConnector::instance()->bluetoothProfileConnect("a2dp", address);
	}

}

// ----------------------------------  VPN functionality -----------------------------------------

void SystemMenu::slotVpnMenuOpened()
{
	m_vpnMenuOpened = true;
}

void SystemMenu::slotVpnMenuClosed()
{
	m_vpnMenuOpened = false;
}

void SystemMenu::slotVpnPrefsTriggered()
{
	launchApp(VPN_PREFS_APP_ID, "");
}

void SystemMenu::slotVpnNetworkSelected(QString name, QString status, QString profInfo)
{
	if(status == "connecting") {
		return;
	} if (status == "connected") {
		StatusBarServicesConnector::instance()->vpnConnectionRequest(false, profInfo.toStdString());
	} else {
		StatusBarServicesConnector::instance()->vpnConnectionRequest(true, profInfo.toStdString());
	}
}

void SystemMenu::slotVpnProfileListUpdate(int numProfiles, t_vpnProfile* list)
{
	if(m_vpnMenu) {
		bool connected = false;
		std::string connString = "";

		QMetaObject::invokeMethod(m_vpnMenu, "clearVpnList");

		if(numProfiles > 0) {
			for(int x = 0; x < numProfiles; x++) {
				QMetaObject::invokeMethod(m_vpnMenu, "addVpnEntry",
										  Q_ARG(QVariant, list[x].displayName),
										  Q_ARG(QVariant, list[x].connectionState),
										  Q_ARG(QVariant, list[x].profInfo));

				if(!connected) {
					if(!strcmp(list[x].connectionState, "connected") || !strcmp(list[x].connectionState, "connecting")) {
						connected = true;
						connString = list[x].displayName;
					}
				}
			}
		}

		QMetaObject::invokeMethod(m_vpnMenu, "setVpnState",
								  Q_ARG(QVariant, connected),
								  Q_ARG(QVariant, QString::fromUtf8(connString.c_str())));

		if(m_vpnMenuOpened) {
			QMetaObject::invokeMethod(m_vpnMenu, "adjustViewIfNecessary");
		}
	}
}

void SystemMenu::slotVpnStateChanged(bool enabled)
{
	if(!enabled) {
		// force a disconnect on all VPN profiles
		QMetaObject::invokeMethod(m_vpnMenu, "setVpnState",
								  Q_ARG(QVariant, false),
								  Q_ARG(QVariant, ""));

		QMetaObject::invokeMethod(m_vpnMenu, "forceDisconnectAllProfiles");

	}
}



// ----------------------------------  Airplane Mode functionality -----------------------------------------

void SystemMenu::slotAirplaneModeTriggered()
{
	if((m_airplaneModeState == AirplaneModeTurningOn) || (m_airplaneModeState == AirplaneModeTurningOff))
		return;

	// setting the preference will trigger a signal that will enable/disable the airplane mode
	Preferences::instance()->setAirplaneMode(m_airplaneModeState != AirplaneModeOn);
}

void SystemMenu::slotAirplaneModeState(t_airplaneModeState state)
{
	setAirplaneModeState(state);
}

void SystemMenu::setAirplaneModeState(t_airplaneModeState state)
{
	if ((state < AirplaneModeOff) || (state > AirplaneModeOn))
		return;

	if(m_menuObject) {
		m_airplaneModeState = state;
		QMetaObject::invokeMethod(m_menuObject, "setAirplaneModeStatus",
				                  Q_ARG(QVariant, fromStdUtf8(LOCALIZED(airplaneModeStateText[m_airplaneModeState]))),
				                  Q_ARG(QVariant, state));
	}
}


// ----------------------------------  System Settings Functionality -----------------------------------------

void SystemMenu::slotRotationLockTriggered(bool isLocked)
{
	if(isLocked) {
		// mark it as invalid (not rotation locked)
        Preferences::instance()->setRotationLockPref(OrientationEvent::Orientation_Invalid);
	} else {
		// lock it to the current UI orientation
		Preferences::instance()->setRotationLockPref(WindowServer::instance()->getUiOrientation());
	}
}

void SystemMenu::slotMuteToggleTriggered(bool isMuted)
{
	Preferences::instance()->setMuteSoundPref(!isMuted);
}

void SystemMenu::slotMenuBrightnessChanged(qreal value, bool save)
{
	DisplayManager::instance()->setMaximumBrightness((int)(value * (100 - MINIMUM_BRIGHTNESS)) + MINIMUM_BRIGHTNESS, save);
}

void SystemMenu::slotDisplayMaxBrightnessChanged(int brightness)
{
	QMetaObject::invokeMethod(m_menuObject, "setSystemBrightness",
			                                Q_ARG(QVariant, (qreal)(((qreal)(brightness - MINIMUM_BRIGHTNESS)) / (100.0 - MINIMUM_BRIGHTNESS))));
}

void SystemMenu::slotRotationLockChanged(OrientationEvent::Orientation rotationLock)
{
    bool locked = rotationLock != OrientationEvent::Orientation_Invalid;

	if(locked)
	{
		QMetaObject::invokeMethod(m_menuObject, "setRotationLockText",
				                                Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Turn off Rotation Lock"))),
				                                Q_ARG(QVariant, locked));
	} else {
		QMetaObject::invokeMethod(m_menuObject, "setRotationLockText",
				                                Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Turn on Rotation Lock"))),
				                                Q_ARG(QVariant, locked));
	}
}

void SystemMenu::slotMuteSoundChanged(bool muteOn)
{
	if(muteOn)
	{
		QMetaObject::invokeMethod(m_menuObject, "setMuteControlText",
				                                Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Unmute Sound"))),
				                                Q_ARG(QVariant, muteOn));
	} else {
		QMetaObject::invokeMethod(m_menuObject, "setMuteControlText",
				                                Q_ARG(QVariant, fromStdUtf8(LOCALIZED("Mute Sound"))),
				                                Q_ARG(QVariant, muteOn));
	}

	QWidget* window = QApplication::focusWidget();
	QApplication::postEvent(window, new QKeyEvent(muteOn ? QEvent::KeyPress : QEvent::KeyRelease, Qt::Key_Ringer, Qt::NoModifier));
}

void SystemMenu::slotPowerdConnectionStateChanged(bool connected)
{
	if(m_menuHandler) {
		if(!connected)
			m_menuHandler->updateBatteryLevel(fromStdUtf8(LOCALIZED("Error"))); // $$$ LOCALIZE
	}
}

void SystemMenu::slotBatteryLevelUpdated(int percentage)
{
	if(m_menuHandler) {
		char text[30];
		std::string textStr;
		if((percentage >= 0) && (percentage <= 100))
			sprintf(text, "%d%%", percentage);
		else
			sprintf(text, "%s", LOCALIZED("Not Available").c_str()); // $$$ LOCALIZE

		textStr = text;
		m_menuHandler->updateBatteryLevel(fromStdUtf8(textStr));
	}
}

void SystemMenu::slotSystemTimeChanged()
{
	tick();
}

void SystemMenu::slotPositiveSpaceChangeFinished(QRect rect)
{
	if(m_menuObject) {
		int height = rect.height()+10;
		int currentHeight = m_menuObject->property("height").toInt()+10;
		QMetaObject::invokeMethod(m_menuObject, "setHeight", Q_ARG(QVariant, height));
		QGraphicsObject* flickable = m_menuObject->findChild<QGraphicsObject*>("flickableArea");
	}
}

void SystemMenu::tick()
{
	time_t rawTime;
	struct tm* timeinfo = 0;

	::time(&rawTime);
	timeinfo = ::localtime(&rawTime);

	// If local date matches we don't need to update anything
	if (timeinfo->tm_year == m_lastUpdateDate.year &&
	    timeinfo->tm_mon == m_lastUpdateDate.month &&
	    timeinfo->tm_mday == m_lastUpdateDate.day)
	{
		return;
	}

	m_lastUpdateDate.year  = timeinfo->tm_year;
	m_lastUpdateDate.month = timeinfo->tm_mon;
	m_lastUpdateDate.day   = timeinfo->tm_mday;

	QMetaObject::invokeMethod(m_menuObject, "updateDate");
}


// -----------------------------------------------------------------------------------
    // Menu Handler Object exposed to QML for the System Menu
// -----------------------------------------------------------------------------------

 MenuHandler::MenuHandler(SystemMenu *parent)
 	 : m_systemMenu(parent)
 {

 }

 MenuHandler::~MenuHandler()
 {

 }

 // -----------------------------------------------------------------------------------
     // Animated Spinner element exposed to QML for the System Menu
 // -----------------------------------------------------------------------------------
 AnimatedSpinner::AnimatedSpinner(QObject *parent)
 	: m_anim(this, "currentFrame", this)
 	, m_duration(1000)
 {
 	Settings* settings = Settings::LunaSettings();
 	std::string spinnerImagePath = settings->lunaSystemResourcesPath + "/spinner.png";

 	m_img.load(spinnerImagePath.c_str());

    m_nFrames = 60;
    m_currentFrame = 0;

    m_bounds = QRectF(0, 0, m_img.width(), m_img.width());

    m_anim.setLoopCount(-1);
    m_anim.setDuration(m_duration);
    m_anim.setStartValue(0);
    m_anim.setEndValue(m_nFrames);

    m_anim.start();
    m_anim.setPaused(true);
 }

 AnimatedSpinner::~AnimatedSpinner()
 {

 }

 void AnimatedSpinner::setDuration(const int duration )
 {
 	m_duration = duration;
 	m_anim.setDuration(m_duration);
 }

 void AnimatedSpinner::setCurrentFrame(const int frame )
 {
 	m_currentFrame = frame;
 	if(isVisible()) {
 		update();
 	}
 }

 void AnimatedSpinner::setOn(const bool on ) {
 	m_on = on;
 	m_anim.setPaused(!m_on);
 	setVisible(m_on);
 }

 QRectF AnimatedSpinner::boundingRect() const
 {
 	return m_bounds;
 }

 void AnimatedSpinner::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
 {
     qreal angle = m_currentFrame * 360.0 / m_nFrames;
     painter->translate(m_bounds.width()/2, m_bounds.height()/2);
     painter->rotate(angle);

     QPainter::RenderHints savedRenderHints = painter->renderHints();

     painter->setRenderHints(savedRenderHints | QPainter::SmoothPixmapTransform);

     painter->drawPixmap(m_bounds.x() - m_bounds.width()/2,
                         m_bounds.y() - m_bounds.height()/2,
                         m_bounds.width(),
                         m_bounds.height(),
                         m_img,
                         0, 0, m_img.width(), m_img.height());

     painter->setRenderHints(savedRenderHints);

     painter->rotate(-angle);
     painter->translate(-m_bounds.width()/2, -m_bounds.height()/2);
 }


