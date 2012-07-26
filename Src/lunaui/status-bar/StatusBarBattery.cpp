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




#include "StatusBarBattery.h"
#include "Settings.h"
#include "HostBase.h"
#include "StatusBarServicesConnector.h"
#include "SystemUiController.h"
#include "SoundPlayerPool.h"

#include <QPainter>

#define BATTERY_IMAGE_WIDTH_PADDING   2

#define TEXT_BASELINE_OFFSET            (-1)

const int StatusBarBattery::m_chargeLevels[StatusBarBattery::kNumBatteryStates] = {12, 20, 28, 36, 44, 52, 60, 68, 76, 84, 88, 99, 100};

bool StatusBarBattery::s_playSoundWhenCharged = false;

StatusBarBattery::StatusBarBattery(bool showText)
	: m_powerdConnected(false)
	, m_chargeState(0)
	, m_charging(false)
	, m_batteryLevel(0)
	, m_showBatteryText(showText)
{
	int width, height;

	// defaults in case of error while loading
	m_imgWidth  = 17;
	m_imgHeight = 20;

	Settings* settings = Settings::LunaSettings();
	std::string statusBarImagesPath = settings->lunaSystemResourcesPath + "/statusBar/";

	// Load a single image to get the size -- defer all others until init
	std::string errorPath = statusBarImagesPath + "battery-error.png";
	m_errorPixmap = QPixmap(errorPath.c_str());

	if (!m_errorPixmap.isNull()) {
		m_imgWidth = m_errorPixmap.width() + BATTERY_IMAGE_WIDTH_PADDING;
		m_imgHeight = m_errorPixmap.height();
	}

	if(Settings::LunaSettings()->tabletUi && m_showBatteryText) {
		// Set up text
		const char* fontName = Settings::LunaSettings()->fontStatusBar.c_str();
		m_font = new QFont(fontName, 14);
		m_font->setPixelSize(14);

		if (m_font) {
			m_font->setBold(true);
		}

		setBatteryLevelText(100); // maximum length

		width  = m_textWidth + m_imgWidth;
		height = m_imgHeight;

		m_bounds = QRectF(-width/2, -height/2, width, height);
	} else {
		m_textWidth = 0;
		m_textHeight = 0;

		width  = m_imgWidth;
		height = m_imgHeight;

		m_bounds = QRectF(-width/2, -height/2, width, height);
	}


	// implicit assumption that the dimensions of the errorPixmap is the same as the others
}

StatusBarBattery::~StatusBarBattery()
{

}

QRectF StatusBarBattery::boundingRect() const
{
	return m_bounds;
}

void StatusBarBattery::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	if (!m_powerdConnected) {
		if (!m_errorPixmap.isNull()) {
			painter->drawPixmap(m_bounds.x() + (m_bounds.width() - m_pixmap[m_chargeState].width())/2.0,
					            m_bounds.y() + (m_bounds.height() - m_chargingPixmap[m_chargeState].height())/2.0,
					            m_errorPixmap);
		}
	} else {
		if (m_charging) {
			if (!m_chargingPixmap[m_chargeState].isNull()) {
				painter->drawPixmap(m_bounds.x() + (m_bounds.width() - m_pixmap[m_chargeState].width())/2.0,
						            m_bounds.y() + (m_bounds.height() - m_chargingPixmap[m_chargeState].height())/2.0,
						            m_chargingPixmap[m_chargeState]);
			}
		} else {
			if (!m_pixmap[m_chargeState].isNull()) {
				painter->drawPixmap(m_bounds.x() + (m_bounds.width() - m_pixmap[m_chargeState].width())/2.0,
						            m_bounds.y() + (m_bounds.height() - m_chargingPixmap[m_chargeState].height())/2.0,
						            m_pixmap[m_chargeState]);
			}
		}
	}

	if(Settings::LunaSettings()->tabletUi && m_showBatteryText) {
		QPen oldPen = painter->pen();
		QFont origFont = painter->font();
		painter->setFont(*m_font);

		QFontMetrics fontMetrics(*m_font);

		int baseLine = m_textHeight/2 - fontMetrics.descent() + TEXT_BASELINE_OFFSET;

		// paint the text
		painter->setPen(QColor(0xFF, 0xFF, 0xFF, 0xFF));

		painter->drawText(QPointF(m_bounds.width()/2 - m_imgWidth - m_textWidth, baseLine), m_batteryText);

		painter->setPen(oldPen);
		painter->setFont(origFont);

	}
}

void StatusBarBattery::init()
{
	char intBuf[16];
	int i = 0;
	std::string batteryPath;
	std::string chargingPath;

	Settings* settings = Settings::LunaSettings();
	std::string statusBarImagesPath = settings->lunaSystemResourcesPath + "/statusBar/";

	for (i = 0; i < kNumBatteryStates; i++) {
		::snprintf(intBuf, sizeof(intBuf), "%d", i);
		intBuf[sizeof(intBuf)-1] = '\0';

		if (i == (kNumBatteryStates - 1)) {
			chargingPath = statusBarImagesPath + "battery-charged.png";
		} else {
			batteryPath = statusBarImagesPath + "battery-" + intBuf + ".png";
			chargingPath = statusBarImagesPath + "battery-charging-" + intBuf + ".png";
		}

		if (i == (kNumBatteryStates - 1)) {
			m_pixmap[i] = m_pixmap[i-1];
		} else {
			m_pixmap[i] = QPixmap(batteryPath.c_str());
		}

		m_chargingPixmap[i] = QPixmap(chargingPath.c_str());
	}

	std::string errorPath = statusBarImagesPath + "battery-error.png";
	m_errorPixmap = QPixmap(errorPath.c_str());

	// Instantiates and initialized the services connector
	StatusBarServicesConnector* svcConnector = StatusBarServicesConnector::instance();

	if(svcConnector) {
		connect(svcConnector,SIGNAL(signalPowerdConnectionStateChanged(bool)), this, SLOT(slotPowerdConnectionStateChanged(bool)));
		connect(svcConnector,SIGNAL(signalBatteryLevelUpdated(int)), this, SLOT(slotBatteryLevelUpdated(int)));
		connect(svcConnector,SIGNAL(signalChargingStateUpdated(bool)), this, SLOT(slotChargingStateUpdated(bool)));
	}
}

void StatusBarBattery::slotBatteryLevelUpdated(int percentage)
{
	if(G_LIKELY(!Settings::LunaSettings()->demoMode))
		m_batteryLevel = percentage;
	else
		m_batteryLevel = 100;

	setBatteryLevelText(m_batteryLevel);

	int i = 0;
	for (i = 0; i < kNumBatteryStates; i++) {
		if (m_batteryLevel <= m_chargeLevels[i]) {
			break;
		}
	}

	if (i != m_chargeState) {
		m_chargeState = i;
		update();
	}

	if (percentage < 95) {
		s_playSoundWhenCharged = true;
	}

	if (s_playSoundWhenCharged && percentage == 100 && SystemUiController::instance()->bootFinished()) {
		s_playSoundWhenCharged = false;
		SoundPlayerPool::instance()->play("/usr/palm/sounds/battery_full.mp3", "notifications", false, -1);
	}
}


void StatusBarBattery::slotChargingStateUpdated(bool charging)
{
	m_charging = charging;

	update();
}

void StatusBarBattery::slotPowerdConnectionStateChanged(bool connected)
{
	m_powerdConnected = connected;

	if(!m_powerdConnected) {
		setBatteryLevelText(-1);
	}

	update();
}

void StatusBarBattery::setBatteryLevelText(int percent)
{
	if(!Settings::LunaSettings()->tabletUi || !m_showBatteryText)
		return;

	char text[5];
	if(percent >= 0 && percent <= 100) {
		sprintf(text, "%d%%", percent);
	} else{
		sprintf(text, " ? ");
	}

	QFontMetrics fontMetrics(*m_font);

	m_batteryText = QString(text);

	QRect textBounds = fontMetrics.boundingRect(m_batteryText);

	if((m_textWidth != textBounds.width()) || (m_textHeight != textBounds.height())) {
		m_textWidth  = textBounds.width();
		m_textHeight = textBounds.height();

		int width  = m_textWidth + m_imgWidth;
		int height = m_imgHeight;

		prepareGeometryChange();
		m_bounds = QRectF(-width/2, -height/2, width, height);

		Q_EMIT signalBoundingRectChanged();
	}

	update();
}
