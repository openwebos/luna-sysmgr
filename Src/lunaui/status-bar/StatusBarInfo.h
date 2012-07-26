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




#ifndef STATUSBARINFO_H
#define STATUSBARINFO_H

#include "StatusBar.h"
#include "StatusBarItem.h"
#include "StatusBarIcon.h"
#include "StatusBarServicesConnector.h"

#include <QGraphicsObject>
#include <QPixmap>


class StatusBarInfoItem : public StatusBarIcon
{
	Q_OBJECT

public:
	StatusBarInfoItem(StatusBarIconContainer* parent);
	~StatusBarInfoItem();

	void loadImage(int index, std::string imgPath);
	void selectImage(unsigned int index);
	int  selectedImageIndex() { return m_imgIndex; }

private:

	QVector<QPixmap> m_images;
	int m_imgIndex;

};


class StatusBarInfo : public StatusBarItem
                    , public StatusBarIconContainer
{
	Q_OBJECT

public:
	StatusBarInfo(StatusBar::StatusBarType type);
	~StatusBarInfo();

	int width() const { return m_bounds.width(); }
	int height() const { return m_bounds.height(); }

	QRectF boundingRect() const; // This item is Right Aligned (The position  of the icon is the position of the RIGHT EDGE of the bounding rect)

	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void init();

	StatusBar::StatusBarType getType() { return m_type; }

	void setRSSI(bool shown, StatusBar::IndexRSSI index);
	void setRSSI1x(bool shown, StatusBar::IndexRSSI1x index);
	void setTTY(bool enabled);
	void setHAC(bool enabled);
	void setCallForward(bool enabled);
	void setRoaming(bool enabled);
	void setVpn(bool enabled);
	void setWAN(bool shown, StatusBar::IndexWAN index);
	void setBluetooth(bool shown, StatusBar::IndexBluetooth index);
	void setWifi(bool shown, StatusBar::IndexWiFi index);
	void setRotationLock(bool locked);
	void setMute(bool muteOn);
	void setAirplaneMode(bool airplaneModeOn);

	void updateBoundingRect(bool forceRepaint=false);

private Q_SLOTS:
	void slotRoamingIndicatorChanged();
	void slotDualRssiEnabled();
	void slotAirplaneModeState(t_airplaneModeState state);
private:
	void updateItem(StatusBarInfoItem * item, bool shown, int index);
	std::string getRoamingImageName(std::string roamingIndicatorName);

	StatusBar::StatusBarType m_type;

	QVector<StatusBarInfoItem*>  m_icons; // items order from right (0) to left

	StatusBarInfoItem *m_rssi;
	StatusBarInfoItem *m_rssi1x;
	StatusBarInfoItem *m_wan;
	StatusBarInfoItem *m_bluetooth;
	StatusBarInfoItem *m_wifi;
	StatusBarInfoItem *m_tty;
	StatusBarInfoItem *m_hac;
	StatusBarInfoItem *m_callForward;
	StatusBarInfoItem *m_roaming;
	StatusBarInfoItem *m_vpn;
	StatusBarInfoItem *m_rotationLock;
	StatusBarInfoItem *m_mute;
	StatusBarInfoItem *m_airplaneMode;

};



#endif /* STATUSBARINFO_H */
