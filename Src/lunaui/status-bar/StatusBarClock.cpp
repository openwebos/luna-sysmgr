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




#include "StatusBarClock.h"
#include "StatusBar.h"
#include "Settings.h"
#include "Preferences.h"
#include "Localization.h"
#include "StatusBarServicesConnector.h"

#include <QPainter>
#include <QTimer>
#include <QDate>
#include <glib.h>

#define TEXT_BASELINE_OFFSET            (-2)
#define DEMO_MODE_TIME "12:00"

StatusBarClock::StatusBarClock(unsigned int padding)
	: m_clockTimer(new QTimer(this))
	, m_font(0)
	, m_twelveHour(true)
	, m_displayDate(false)
	, m_curTimeStr(0)
	, m_textPadding(padding)
{
	connect(m_clockTimer, SIGNAL(timeout()), this, SLOT(tick()));
	m_clockTimer->start(1000);

	StatusBarServicesConnector* svcConnector = StatusBarServicesConnector::instance();

	if(svcConnector) {
		connect(svcConnector,SIGNAL(signalSystemTimeChanged()), this, SLOT(slotSystemTimeChanged()));
	}

	// Set up text
	const char* fontName = Settings::LunaSettings()->fontStatusBar.c_str();
	m_font = new QFont(fontName, 15);
	m_font->setPixelSize(15);

	if (m_font) {
		m_font->setBold(true);
	}

	::memset(m_timeBuf, 0, sizeof(m_timeBuf));
	::memset(&m_lastUpdateTime, 0, sizeof(m_lastUpdateTime));
	
    setDisplayDate(false); // this will set the correct bounding rect and update the time

    if(Settings::LunaSettings()->demoMode)
    	setTimeText(QString(DEMO_MODE_TIME));
}

StatusBarClock::~StatusBarClock()
{
	m_clockTimer->stop();
	delete m_clockTimer;
	delete m_font;
}

QRectF StatusBarClock::boundingRect() const
{
	return m_bounds;
}

void StatusBarClock::setPadding( unsigned int padding)
{
	m_textPadding = padding;

	// force re-calculation of the bounding rect
	setDisplayDate(m_displayDate);
}

void StatusBarClock::setDisplayDate(bool date)
{
	m_displayDate = date;

	m_clockTimer->stop();
	// Calculate the max width
	if(!m_displayDate) {
		setTimeText("00:00", false);
		m_clockTimer->start (1000);
	} else {
		setTimeText("00/00/00", false);
		m_clockTimer->start (60000);
	}

	prepareGeometryChange();
	m_bounds = QRect(-m_textRect.width()/2 - m_textPadding, -m_textRect.height()/2,
			         m_textRect.width() + m_textPadding * 2, m_textRect.height());
	Q_EMIT signalBoundingRectChanged();

	::memset(&m_lastUpdateTime, 0, sizeof(m_lastUpdateTime));
	tick();
}

void StatusBarClock::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QPen oldPen = painter->pen();

	QFont origFont = painter->font();
	painter->setFont(*m_font);

	QFontMetrics fontMetrics(*m_font);

	int baseLine = m_textRect.height()/2 - fontMetrics.descent() + TEXT_BASELINE_OFFSET;

	// paint the text
	painter->setPen(QColor(0xFF, 0xFF, 0xFF, 0xFF));

	painter->drawText(QPointF(-m_textRect.width()/2, baseLine), m_timeText);

	painter->setPen(oldPen);
	painter->setFont(origFont);

}

void StatusBarClock::setTimeText(const char *time, bool doUpdate)
{
	if(G_LIKELY(!Settings::LunaSettings()->demoMode))
		setTimeText(QString(time));
	else
		setTimeText(QString(DEMO_MODE_TIME));
}

void StatusBarClock::setTimeText(const QString& time, bool doUpdate)
{
	QFontMetrics fontMetrics(*m_font);

	m_timeText = time;

	m_textRect = fontMetrics.boundingRect(m_timeText);

	if (doUpdate) {
		update();
	}
}

void StatusBarClock::slotSystemTimeChanged()
{
	tick();
}

void StatusBarClock::tick()
{
	if(Settings::LunaSettings()->demoMode)
		return;

	time_t rawTime;
	struct tm* timeinfo = 0;

	::time(&rawTime);
	timeinfo = ::localtime(&rawTime);

	// get current local time (or date)
	if(!m_displayDate) {
		// If local time matches we don't need to update anything
		if (timeinfo->tm_hour == m_lastUpdateTime.hour && timeinfo->tm_min == m_lastUpdateTime.min)
		{
			return;
		}

		m_lastUpdateTime.hour = timeinfo->tm_hour;
		m_lastUpdateTime.min = timeinfo->tm_min;

		if (m_twelveHour) {
			::strftime(m_timeBuf, kMaxTimeChars, LOCALIZED("%I:%M").c_str(), timeinfo);

			if (m_timeBuf[0] == '0') {
				m_curTimeStr = m_timeBuf + 1;
			} else {
				m_curTimeStr = m_timeBuf;
			}
		} else {
			::strftime(m_timeBuf, kMaxTimeChars, LOCALIZED("%H:%M").c_str(), timeinfo);
			m_curTimeStr = m_timeBuf;
		}
		setTimeText(m_curTimeStr);
	} else{
		// If local time matches we don't need to update anything
		if (timeinfo->tm_year == m_lastUpdateTime.year && timeinfo->tm_mon == m_lastUpdateTime.month && timeinfo->tm_mday == m_lastUpdateTime.day)
		{
			return;
		}

		m_lastUpdateTime.year  = timeinfo->tm_year;
		m_lastUpdateTime.month = timeinfo->tm_mon;
		m_lastUpdateTime.day   = timeinfo->tm_mday;

		setTimeText(QDate::currentDate().toString(Qt::DefaultLocaleShortDate));
	}

	update();
}

void StatusBarClock::updateTimeFormat(const char* format)
{
	m_twelveHour = (strcmp(format, "HH12") == 0);
	::memset(&m_lastUpdateTime, 0, sizeof(m_lastUpdateTime));
	tick();
	update();
}

