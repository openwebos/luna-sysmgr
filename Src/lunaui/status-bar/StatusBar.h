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




#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "Window.h"
#include <QPointer>
#include <QGraphicsObject>
#include "VariantAnimation.h"
#include "CustomEvents.h"

#define MAX_NOTIF_ICONS       10
#define NOTIF_ICON_WIDTH      24  // $$$ constant width??
#define ARROW_SPACING         7

#define kMaxTitleLength  60

#define kStatusBarQtLetterSpacing   90

class StatusBarClock;
class StatusBarBattery;
class StatusBarTitle;
class StatusBarServicesConnector;
class StatusBarInfo;
class StatusBarNotificationArea;
class StatusBarItemGroup;

class StatusBar : public QGraphicsObject
{
	Q_OBJECT

public:

	enum StatusBarType {
		TypeNormal      = 0,
		TypeLockScreen,
		TypeDockMode,
		TypeFirstUse
	};

	// Info Items Index Enumerations
	enum IndexRSSI {
		RSSI_0  = 0,
		RSSI_1,
		RSSI_2,
		RSSI_3,
		RSSI_4,
		RSSI_5,
		RSSI_FLIGHT_MODE,
		RSSI_ERROR,
		RSSI_EV_0, // these EV values are only used for dual-RSSI (China)
		RSSI_EV_1,
		RSSI_EV_2,
		RSSI_EV_3,
		RSSI_EV_4,
		RSSI_EV_5
	};

	// 1x RSSI values for Dual RSSI only
	enum IndexRSSI1x {
		RSSI_1X_0 = 0,
		RSSI_1X_1,
		RSSI_1X_2,
		RSSI_1X_3,
		RSSI_1X_4,
		RSSI_1X_5
	};

	enum IndexWAN {
		WAN_OFF  = 0,
		WAN_CONNECTED_1X,
		WAN_DORMANT_1X,
		WAN_CONNECTED_EDGE,
		WAN_CONNECTED_EVDO,
		WAN_DORMANT_EVDO,
		WAN_CONNECTED_EVDO3G,
		WAN_DORMANT_EVDO3G,
		WAN_CONNECTED_GPRS,
		WAN_CONNECTED_UMTS,
		WAN_CONNECTED_HSDPA,
		WAN_CONNECTED_HSPA_4G
	};

	enum IndexBluetooth {
		BLUETOOTH_OFF  = 0,
		BLUETOOTH_ON,
		BLUETOOTH_CONNECTING,
		BLUETOOTH_CONNECTED
	};

	enum IndexWiFi {
		WIFI_OFF  = 0,
		WIFI_ON,
		WIFI_CONNECTING,
		WIFI_BAR_1,
		WIFI_BAR_2,
		WIFI_BAR_3
	};

	StatusBar(StatusBarType type, int width, int height);
	virtual ~StatusBar();

	void init();

	QRectF boundingRect() const { return m_bounds; }
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void resize(int w, int h);

	bool sceneEvent (QEvent* event);

	StatusBarType getType() { return m_type; }

	void setMaximizedAppTitle(bool appMaximized, const char* title = 0, const unsigned int customColor = 0, bool appTitleActionable = true);
	void setBarOpaque(bool opaque);
	void setCarrierText(const char* carrierTxt);
	void setRssiShown(bool shown);

	void setNotificationWindowOpen(bool open);
	void setSystemMenuOpen(bool open);
	void setDockModeAppMenuOpen(bool open);

	void setSystemMenuObject(QGraphicsObject* menu);

Q_SIGNALS:
	void signalDashboardAreaRightEdgeOffset(int rightOffset);
	void signalSystemMenuStateChanged(bool opened);
	void signalDockModeMenuStateChanged(bool opened);

private Q_SLOTS:
	void slotTimeFormatChanged(const char* format);
	void slotPhoneTypeUpdated();
	void slotCarrierTextChanged(const char* text);
	void slotRssiIndexChanged(bool show, StatusBar::IndexRSSI index);
	void slotRssi1xIndexChanged(bool show, StatusBar::IndexRSSI1x index); // for dual RSSI only
	void slotTTYStateChanged(bool enabled);
	void slotHACStateChanged(bool enabled);
	void slotCallForwardStateChanged(bool enabled);
	void slotRoamingStateChanged(bool enabled);
	void slotVpnStateChanged(bool enabled);
	void slotWanIndexChanged(bool show, StatusBar::IndexWAN index);
	void slotBluetoothIndexChanged(bool show, StatusBar::IndexBluetooth index);
	void slotWifiIndexChanged(bool show, StatusBar::IndexWiFi index);
    void slotRotationLockChanged(OrientationEvent::Orientation rotationLock);
	void slotMuteSoundChanged(bool muteOn);
	void slotChildBoundingRectChanged();
	void slotNotificationArealVisibilityChanged(bool visible);
	void slotBannerMessageActivated();
	void slotNotificationMenuAction(bool active);
	void slotSystemMenuMenuAction(bool active);
	void slotAppMenuMenuAction(bool active);
	void slotBannerActivated();
	void slotBannerDeactivated();
	void slotMenuGroupActivated(StatusBarItemGroup* group);
	void slotDockModeStatusChanged(bool enabled);

private:

	void setBackgroundColor(bool custom, QColor color=Qt::black);

	void layout();
	void fadeAnimValueChanged(const QVariant& value);
	void colorAnimValueChanged(const QVariant& value);

	void fadeBar(bool in);

	QRect m_bounds;
	StatusBarType m_type;

	static QColor s_defaultColor;

	bool m_showRssiIndicators;
	bool m_rssiShown;
	bool m_rssi1xShown;
	StatusBar::IndexRSSI    m_rssiIndex;
	StatusBar::IndexRSSI1x  m_rssi1xIndex;

	StatusBarClock*            m_clock;
	StatusBarBattery*          m_battery;
	StatusBarTitle*            m_title;
	StatusBarInfo*             m_infoItems;
	StatusBarNotificationArea* m_notif;

	StatusBarItemGroup* m_systemUiGroup;
	StatusBarItemGroup* m_titleGroup;
	StatusBarItemGroup* m_notifGroup;

	StatusBarServicesConnector* m_svcConnector;

	bool m_platformHasPhoneRadio;
	std::string m_carrierText;
	std::string m_appTitle;
	bool m_appMaximized;
	bool m_forceOpaque;
	bool m_showAppTitle;

	QPixmap* m_bkgPixmap;
	QColor   m_barColor, m_curColor, m_newColor;
	qreal    m_bkgOpacity;

	typedef VariantAnimation<StatusBar> tStatusBarAnim;
	QPointer<tStatusBarAnim> m_fadeAnimPtr;
	QPointer<tStatusBarAnim> m_colorAnimPtr;
};



#endif /* STATUSBAR_H */
