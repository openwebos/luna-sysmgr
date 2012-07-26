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




#include "StatusBarInfo.h"
#include "Settings.h"
#include "Preferences.h"
#include "StatusBarServicesConnector.h"

#include <QPainter>

StatusBarInfoItem::StatusBarInfoItem(StatusBarIconContainer* parent)
	: StatusBarIcon(parent)
	, m_imgIndex(0)
{
	m_images.resize(1);
}

StatusBarInfoItem::~StatusBarInfoItem()
{

}

void StatusBarInfoItem::loadImage(int index, std::string imgPath)
{
	QPixmap img;

	if(!imgPath.empty()) {
		img.load(imgPath.c_str());
	}

	m_images.insert(index, img);
}

void StatusBarInfoItem::selectImage(unsigned int index)
{
	if(index < (unsigned int)m_images.size()) {
		m_imgIndex = index;

		if(!m_images[index].isNull()) {
			setImage(&m_images[index]);
		} else {
			setImage(NULL);
		}

		updateBoundingRect();
	}
}

// =============================================================================


StatusBarInfo::StatusBarInfo(StatusBar::StatusBarType type)
	:  StatusBarItem(AlignRight) // Right Aligned item
	, m_type(type)
	, m_rssi(0)
	, m_rssi1x(0)
	, m_wan(0)
	, m_bluetooth(0)
	, m_wifi(0)
	, m_tty(0)
	, m_hac(0)
	, m_callForward(0)
	, m_roaming(0)
	, m_vpn(0)
	, m_rotationLock(0)
	, m_mute(0)
	, m_airplaneMode(0)
{

}

StatusBarInfo::~StatusBarInfo()
{
	if(m_rssi)
		delete m_rssi;

	if(m_rssi1x)
		delete m_rssi1x;

	if(m_wan)
		delete m_wan;

	if(m_bluetooth)
		delete m_bluetooth;

	if(m_wifi)
		delete m_wifi;

	if(m_tty)
		delete m_tty;

	if(m_hac)
		delete m_hac;

	if(m_callForward)
		delete m_callForward;

	if(m_roaming)
		delete m_roaming;

	if(m_vpn)
		delete m_vpn;

	if(m_rotationLock)
		delete m_rotationLock;

	if(m_mute)
		delete m_mute;

	if (m_airplaneMode)
		delete m_airplaneMode;
}

QRectF StatusBarInfo::boundingRect() const
{
	return m_bounds;
}

// This item is Right Aligned (The position  of the icon is the position of the RIGHT EDGE of the bounding rect)
void StatusBarInfo::updateBoundingRect(bool forceRepaint)
{
	QRectF rect;

	int width=0, height=0;
	for(int x = 0; x < m_icons.size(); x++) {
		if(m_icons.at(x)->isVisible()) {
			width += m_icons.at(x)->boundingRect().width() + (ICON_SPACING * m_icons.at(x)->visiblePortion());
			if(height < m_icons.at(x)->boundingRect().height())
				height = m_icons.at(x)->boundingRect().height();
		}
	}

	rect = QRect(-width, -height/2, width, height);

	if(rect != m_bounds) {
		prepareGeometryChange();
		m_bounds = rect;
		Q_EMIT signalBoundingRectChanged();
	} else if(forceRepaint) {
		update();
	}
}

void StatusBarInfo::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QPoint centerRight = QPoint(0, 0);

	for(int x = 0; x < m_icons.size(); x++) {
		if(m_icons.at(x)->isVisible()) {
			m_icons.at(x)->paint(painter, centerRight);
			centerRight.setX(centerRight.x() - (m_icons.at(x)->boundingRect().width() + (ICON_SPACING * m_icons.at(x)->visiblePortion())));
		}
	}
}

void StatusBarInfo::init()
{
	Settings* settings = Settings::LunaSettings();
	std::string statusBarImagesPath = settings->lunaSystemResourcesPath + "/statusBar/";

	// initialize all status items

	// Signal Strength (RSSI) (available in all configurations)
	m_rssi = new StatusBarInfoItem(this);
	m_icons.append(m_rssi);
	m_rssi->loadImage((int)StatusBar::RSSI_0, statusBarImagesPath + "rssi-0.png");
	m_rssi->loadImage((int)StatusBar::RSSI_1, statusBarImagesPath + "rssi-1.png");
	m_rssi->loadImage((int)StatusBar::RSSI_2, statusBarImagesPath + "rssi-2.png");
	m_rssi->loadImage((int)StatusBar::RSSI_3, statusBarImagesPath + "rssi-3.png");
	m_rssi->loadImage((int)StatusBar::RSSI_4, statusBarImagesPath + "rssi-4.png");
	m_rssi->loadImage((int)StatusBar::RSSI_5, statusBarImagesPath + "rssi-5.png");
	m_rssi->loadImage((int)StatusBar::RSSI_FLIGHT_MODE, statusBarImagesPath + "rssi-flightmode.png");
	m_rssi->loadImage((int)StatusBar::RSSI_ERROR, statusBarImagesPath + "rssi-error.png");
	connect(Preferences::instance(),SIGNAL(signalDualRssiEnabled()), SLOT(slotDualRssiEnabled()));
	setRSSI(false, StatusBar::RSSI_0);

	if((m_type == StatusBar::TypeNormal) || (m_type == StatusBar::TypeLockScreen) || (m_type == StatusBar::TypeDockMode) || (m_type == StatusBar::TypeFirstUse)) { // Icons for status
		// WAN Status
		m_wan = new StatusBarInfoItem(this);
		m_icons.append(m_wan);
		m_wan->loadImage((int)StatusBar::WAN_CONNECTED_1X, statusBarImagesPath + "network-1x-connected.png");
		m_wan->loadImage((int)StatusBar::WAN_DORMANT_1X, statusBarImagesPath + "network-1x-dormant.png");
		m_wan->loadImage((int)StatusBar::WAN_CONNECTED_EDGE, statusBarImagesPath + "network-edge-connected.png");
		m_wan->loadImage((int)StatusBar::WAN_CONNECTED_EVDO, statusBarImagesPath + "network-evdo-connected.png");
		m_wan->loadImage((int)StatusBar::WAN_DORMANT_EVDO, statusBarImagesPath + "network-evdo-dormant.png");
		m_wan->loadImage((int)StatusBar::WAN_CONNECTED_EVDO3G, statusBarImagesPath + "network-3g-connected.png");
		m_wan->loadImage((int)StatusBar::WAN_DORMANT_EVDO3G, statusBarImagesPath + "network-3g-dormant.png");
		m_wan->loadImage((int)StatusBar::WAN_CONNECTED_GPRS, statusBarImagesPath + "network-gprs-connected.png");
		m_wan->loadImage((int)StatusBar::WAN_CONNECTED_UMTS, statusBarImagesPath + "network-3g-connected.png");
		m_wan->loadImage((int)StatusBar::WAN_CONNECTED_HSDPA, statusBarImagesPath + "network-3g-connected.png");
		m_wan->loadImage((int)StatusBar::WAN_CONNECTED_HSPA_4G, statusBarImagesPath + "network-hsdpa-plus-connected.png");
		setWAN(false, StatusBar::WAN_OFF);

		// Bluetooth Status
		m_bluetooth = new StatusBarInfoItem(this);
		m_icons.append(m_bluetooth);
		m_bluetooth->loadImage((int)StatusBar::BLUETOOTH_ON, statusBarImagesPath + "bluetooth-on.png");
		m_bluetooth->loadImage((int)StatusBar::BLUETOOTH_CONNECTING, statusBarImagesPath + "bluetooth-connecting.png");
		m_bluetooth->loadImage((int)StatusBar::BLUETOOTH_CONNECTED, statusBarImagesPath + "bluetooth-connected.png");
		setBluetooth(false, StatusBar::BLUETOOTH_OFF);

		// WiFi Status
		m_wifi = new StatusBarInfoItem(this);
		m_icons.append(m_wifi);
		m_wifi->loadImage((int)StatusBar::WIFI_ON, statusBarImagesPath + "wifi-0.png");
		m_wifi->loadImage((int)StatusBar::WIFI_CONNECTING, statusBarImagesPath + "wifi-connecting.png");
		m_wifi->loadImage((int)StatusBar::WIFI_BAR_1, statusBarImagesPath + "wifi-1.png");
		m_wifi->loadImage((int)StatusBar::WIFI_BAR_2, statusBarImagesPath + "wifi-2.png");
		m_wifi->loadImage((int)StatusBar::WIFI_BAR_3, statusBarImagesPath + "wifi-3.png");
		setWifi(false, StatusBar::WIFI_OFF);

		// TTY
		m_tty = new StatusBarInfoItem(this);
		m_icons.append(m_tty);
		m_tty->loadImage(0, statusBarImagesPath + "tty.png");
		setTTY(false);

		// HAC
		m_hac = new StatusBarInfoItem(this);
		m_icons.append(m_hac);
		m_hac->loadImage(0, statusBarImagesPath + "hac.png");
		setHAC(false);

		// Call Forward
		m_callForward = new StatusBarInfoItem(this);
		m_icons.append(m_callForward);
		m_callForward->loadImage(0, statusBarImagesPath + "call-forward.png");
		setCallForward(false);

		// Roaming
		m_roaming = new StatusBarInfoItem(this);
		m_icons.append(m_roaming);
		m_roaming->loadImage(0, statusBarImagesPath + "network-roaming.png");
		connect(Preferences::instance(),SIGNAL(signalRoamingIndicatorChanged()), SLOT(slotRoamingIndicatorChanged()));
		setRoaming(false);

		// VPN
		m_vpn = new StatusBarInfoItem(this);
		m_icons.append(m_vpn);
		m_vpn->loadImage(0, statusBarImagesPath + "vpn-status-icon.png");
		setVpn(false);

		// Rotation Lock
		m_rotationLock = new StatusBarInfoItem(this);
		m_icons.append(m_rotationLock);
		m_rotationLock->loadImage(0, statusBarImagesPath + "icon-rotation-lock.png");
		setRotationLock(false);

		// Mute indicator
		m_mute = new StatusBarInfoItem(this);
		m_icons.append(m_mute);
		m_mute->loadImage(0, statusBarImagesPath + "icon-mute.png");
		setMute(false);

		m_airplaneMode = new StatusBarInfoItem(this);
		m_icons.append(m_airplaneMode);
		m_airplaneMode->loadImage(0, statusBarImagesPath + "icon-airplane.png");
		connect(StatusBarServicesConnector::instance(), SIGNAL(signalAirplaneModeState(t_airplaneModeState)), SLOT(slotAirplaneModeState(t_airplaneModeState)));
		setAirplaneMode(Preferences::instance()->airplaneMode());
	}
	updateBoundingRect();
}

std::string StatusBarInfo::getRoamingImageName(std::string roamingIndicatorName)
{
	if(roamingIndicatorName == "triangle") {
		return std::string("network-roaming-triangle.png");
	} else if(roamingIndicatorName == "none") {
		return std::string();
	} else {
		// default
		return std::string("network-roaming.png");
	}
}

void StatusBarInfo::slotRoamingIndicatorChanged()
{
	if(m_roaming) {
		std::string image = getRoamingImageName(Preferences::instance()->roamingIndicator());
		m_roaming->loadImage(0, Settings::LunaSettings()->lunaSystemResourcesPath + "/statusBar/" + image);
	}
}

void StatusBarInfo::slotDualRssiEnabled()
{
	Settings* settings = Settings::LunaSettings();
	std::string statusBarImagesPath = settings->lunaSystemResourcesPath + "/statusBar/";

	// Dual RSSI in use, so init the images for Dual RSSI
	if(m_rssi) {
		m_rssi->loadImage((int)StatusBar::RSSI_EV_0, statusBarImagesPath + "rssi-3G-0.png");
		m_rssi->loadImage((int)StatusBar::RSSI_EV_1, statusBarImagesPath + "rssi-3G-1.png");
		m_rssi->loadImage((int)StatusBar::RSSI_EV_2, statusBarImagesPath + "rssi-3G-2.png");
		m_rssi->loadImage((int)StatusBar::RSSI_EV_3, statusBarImagesPath + "rssi-3G-3.png");
		m_rssi->loadImage((int)StatusBar::RSSI_EV_4, statusBarImagesPath + "rssi-3G-4.png");
		m_rssi->loadImage((int)StatusBar::RSSI_EV_5, statusBarImagesPath + "rssi-3G-5.png");
	}

	if(m_rssi1x) {
		m_rssi = new StatusBarInfoItem(this);
		m_icons.insert(1, m_rssi1x);
		m_rssi1x->loadImage((int)StatusBar::RSSI_1X_0, statusBarImagesPath + "rssi-1x-0.png");
		m_rssi1x->loadImage((int)StatusBar::RSSI_1X_1, statusBarImagesPath + "rssi-1x-1.png");
		m_rssi1x->loadImage((int)StatusBar::RSSI_1X_2, statusBarImagesPath + "rssi-1x-2.png");
		m_rssi1x->loadImage((int)StatusBar::RSSI_1X_3, statusBarImagesPath + "rssi-1x-3.png");
		m_rssi1x->loadImage((int)StatusBar::RSSI_1X_4, statusBarImagesPath + "rssi-1x-4.png");
		m_rssi1x->loadImage((int)StatusBar::RSSI_1X_5, statusBarImagesPath + "rssi-1x-5.png");
		setRSSI1x(false, StatusBar::RSSI_1X_0);
	}

}

void StatusBarInfo::updateItem(StatusBarInfoItem * item, bool shown, int index)
{
	if(!item)
		return;

	if(shown) {
		item->selectImage(index);
		if(!item->isVisible()) {
			item->show();
		}
	} else {
		if(item->isVisible()) {
			item->hide();
		}
	}

	update();
}

void StatusBarInfo::setRSSI(bool shown, StatusBar::IndexRSSI index)
{
	updateItem(m_rssi, shown, (int) index);
}

void StatusBarInfo::setRSSI1x(bool shown, StatusBar::IndexRSSI1x index)
{
	updateItem(m_rssi1x, shown, (int) index);
}

void StatusBarInfo::setTTY(bool enabled)
{
	updateItem(m_tty, enabled, 0);
}

void StatusBarInfo::setHAC(bool enabled)
{
	updateItem(m_hac, enabled, 0);
}

void StatusBarInfo::setCallForward(bool enabled)
{
	updateItem(m_callForward, enabled, 0);
}

void StatusBarInfo::setRoaming(bool enabled)
{
	updateItem(m_roaming, enabled, 0);
}

void StatusBarInfo::setVpn(bool enabled)
{
	updateItem(m_vpn, enabled, 0);
}

void StatusBarInfo::setWAN(bool shown, StatusBar::IndexWAN index)
{
	updateItem(m_wan, shown, (int) index);
}

void StatusBarInfo::setBluetooth(bool shown, StatusBar::IndexBluetooth index)
{
	updateItem(m_bluetooth, shown, (int) index);
}

void StatusBarInfo::setWifi(bool shown, StatusBar::IndexWiFi index)
{
	updateItem(m_wifi, shown, (int) index);
}

void StatusBarInfo::setRotationLock(bool locked)
{
	updateItem(m_rotationLock, locked, 0);
}

void StatusBarInfo::setMute(bool muteOn)
{
	updateItem(m_mute, muteOn, 0);
}

void StatusBarInfo::setAirplaneMode(bool airplaneModeOn)
{
	updateItem(m_airplaneMode, airplaneModeOn, 0);
}

void StatusBarInfo::slotAirplaneModeState(t_airplaneModeState state) {
	if (state == AirplaneModeOn) {
		setAirplaneMode(true);
	} else if (state == AirplaneModeOff) {
		setAirplaneMode(false);
	}
}
