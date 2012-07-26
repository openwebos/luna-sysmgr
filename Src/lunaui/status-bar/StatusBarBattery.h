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




#ifndef STATUSBARBATTERY_H
#define STATUSBARBATTERY_H

#include "StatusBarItem.h"
#include <cjson/json.h>

#include <QGraphicsObject>

class StatusBarBattery : public StatusBarItem
{
	Q_OBJECT

public:
	StatusBarBattery(bool showText = false);
	~StatusBarBattery();

	int width() const { return m_bounds.width(); }
	int height() const { return m_bounds.height(); }

	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void init();

private Q_SLOTS:
	void slotPowerdConnectionStateChanged(bool connected);
	void slotBatteryLevelUpdated(int percentage);
	void slotChargingStateUpdated(bool charging);

private:

	void setBatteryLevelText(int percent);

	bool m_powerdConnected;

	int  m_chargeState;
	bool m_charging;
	int  m_batteryLevel;
	static const int kNumChargeSources = 2;
	static const char* m_chargeSource[kNumChargeSources];

	static const int kNumBatteryStates = 13;
	static const int m_chargeLevels[kNumBatteryStates];
	QPixmap m_pixmap[kNumBatteryStates];
	QPixmap m_chargingPixmap[kNumBatteryStates];
	QPixmap m_errorPixmap;
	int     m_textWidth;
	int     m_textHeight;
	int     m_imgWidth;
	int     m_imgHeight;
	QFont*  m_font;
	QString m_batteryText;
	bool    m_showBatteryText;
	static bool	s_playSoundWhenCharged;
};



#endif /* STATUSBARBATTERY_H */
