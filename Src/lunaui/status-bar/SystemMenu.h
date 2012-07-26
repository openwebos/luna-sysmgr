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




#ifndef SYSTEMMENU_H
#define SYSTEMMENU_H


#include <QGraphicsObject>
#include <QPropertyAnimation>
#include <QTimer>
#include "StatusBarServicesConnector.h"

class QDeclarativeEngine;
class QDeclarativeComponent;
class MenuHandler;

class SystemMenu : public QGraphicsObject
{
	Q_OBJECT

public:
	SystemMenu(int width, int height, bool restricted = false);
	~SystemMenu();

	void init();

    void setOpened(bool opened);
    bool isOpened() { return m_opened; }

	QRectF boundingRect() const;  // This item is Left Aligned (The position  of the icon is the position of the LEFT EDGE of the bounding rect)
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	int getRightEdgeOffset() { return m_rightEdgeOffset; }

private Q_SLOTS:
	void slotCloseSystemMenu();

	void slotWifiMenuOpened();
	void slotWifiMenuClosed();
	void slotWifiOnOffTriggered();
	void slotWifiPrefsTriggered();
	void slotWifiNetworkSelected(int index, QString name, int profileId, QString securityType, QString connStatus);

	void slotBluetoothMenuOpened();
	void slotBluetoothMenuClosed();
	void slotBluetoothOnOffTriggered();
	void slotBluetoothPrefsTriggered();
	void slotBluetoothDeviceSelected(int index);

	void slotVpnMenuOpened();
	void slotVpnMenuClosed();
	void slotVpnPrefsTriggered();
	void slotVpnNetworkSelected(QString name, QString status, QString profInfo);

	void slotAirplaneModeTriggered();
	void slotRotationLockTriggered(bool isLocked);
	void slotMuteToggleTriggered(bool isMuted);
    void slotRotationLockChanged(OrientationEvent::Orientation rotationLock);
	void slotMuteSoundChanged(bool muteOn);
	void slotDisplayMaxBrightnessChanged(int brightness);

	void slotPowerdConnectionStateChanged(bool connected);
	void slotBatteryLevelUpdated(int percentage);

	void slotWifiStateChanged(bool wifiOn, bool wifiConnected, std::string wifiSSID, std::string wifiConnState);
	void slotWifiAvailableNetworksListUpdate(int numNetworks, t_wifiAccessPoint* list);

	void slotBluetoothTurnedOn();
	void slotBluetoothPowerStateChanged(t_radioState radioState);
	void slotBluetoothConnStateChanged(bool btConnected, std::string deviceName);
	void slotBluetoothTrustedDevicesUpdate(int numTrustedDevices, t_bluetoothDevice* list);
	void slotBluetoothParedDevicesAvailable(bool available);
	void slotBluetoothUpdateDeviceStatus(t_bluetoothDevice* deviceStatus);
	void slotVpnProfileListUpdate(int numProfiles, t_vpnProfile* list);
	void slotVpnStateChanged(bool enabled);
	void slotAirplaneModeState(t_airplaneModeState state);
	void slotMenuBrightnessChanged(qreal value, bool save);

	void slotSystemTimeChanged();
	void slotPositiveSpaceChangeFinished(QRect rect);
 	void tick();

 	Q_SIGNALS:
	void signalCloseMenu();

private:

	bool sceneEvent(QEvent* event);

	void setAirplaneModeState(t_airplaneModeState state);
	void launchApp(std::string appId, std::string params);

	void connectBtAudioDevice(std::string address, unsigned int cod);

    QRect m_bounds;
	bool m_restricted;
	bool m_opened;
	int m_rightEdgeOffset;
	QDeclarativeComponent* m_qmlMenu;
	// Top Level Menu Object
	QGraphicsObject* m_menuObject;

	// Children Menu Objects
	QGraphicsObject* m_wifiMenu;
	QGraphicsObject* m_vpnMenu;
	QGraphicsObject* m_bluetoothMenu;

	bool m_wifiMenuOpened;
	bool m_wifiOn;

	bool m_bluetoothMenuOpened;
	t_radioState m_bluetoothPower;
	bool m_btPairedDevicesAvailable;
	bool m_btTurnOnRequested;
	std::vector<t_bluetoothDevice> m_trustedDevices;
	std::string m_pendingDevAddress;
	int         m_pendingCod;

	QTimer *m_dateTimer;

	bool m_vpnMenuOpened;

	MenuHandler* m_menuHandler; // provides an interface to interact with the QML menu

	t_airplaneModeState m_airplaneModeState;

	struct {
		int year;
		int month;
		int day;
	} m_lastUpdateDate;
};



 class MenuHandler : public QObject
 {
    Q_OBJECT
  public:
    MenuHandler(SystemMenu *parent = 0);
    ~MenuHandler();

    void updateBatteryLevel(QString level) { Q_EMIT batteryLevelUpdated(level); }
    void signalWifiListUpdated() { Q_EMIT wifiListUpdated(); }

Q_SIGNALS:

	void batteryLevelUpdated(QString batteryLevel);
	void wifiStateChanged(bool wifiOn, QString wifiString);
	void wifiListUpdated();

 private:

	SystemMenu* m_systemMenu;
 };



 class AnimatedSpinner : public QGraphicsObject
  {
      Q_OBJECT
      Q_PROPERTY(int duration READ duration WRITE setDuration)
      Q_PROPERTY(bool on READ on WRITE setOn)
      Q_PROPERTY(int currentFrame READ currentFrame WRITE setCurrentFrame)
  public:
      AnimatedSpinner(QObject *parent = 0);
      ~AnimatedSpinner();

      int duration() const { return m_duration; }
      void setDuration(const int duration );

      int currentFrame() const { return m_currentFrame; }
      void setCurrentFrame(const int frame );

      int on() const { return m_on; }
      void setOn(const bool on );

  	QRectF boundingRect() const;
  	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

  private:
    int m_duration;
    bool m_on;
    QPixmap m_img;
    QRectF m_bounds;
    int m_nFrames;
    int m_currentFrame;

    QPropertyAnimation m_anim;
  };



#endif /* SYSTEMMENU_H */
