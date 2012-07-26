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




#include "Common.h"

#include "ActiveCallBanner.h"

#include "Settings.h"
#include "HostBase.h"
#include "WebAppMgrProxy.h"
#include "Utils.h"
#include "QtUtils.h"

#include <QPainter>
#include <QFontMetrics>

static const char* kPhoneAppId = "com.palm.app.phone";
const int kFontHeight = 16;
int kSpaceWidth = 0;

#define WINDOW_X_PAD 2
#define WINDOW_Y_PAD 2
#define MAX_TIME "(99:59:59)"

ActiveCallBanner::ActiveCallBanner(int width, int height, const std::string& iconFile, 
								   const std::string& message, uint32_t startTime)
	: m_startTime(startTime)
	, m_originalWidth(0)
{
	m_boundingRect.setRect(-width/2, -height/2, width, height);
	m_originalWidth = width;

	m_font = QFont(QString::fromStdString(Settings::LunaSettings()->fontActiveBanner));
	m_font.setPixelSize(kFontHeight);
	QFontMetrics fm(m_font);
	kSpaceWidth = fm.width(' ');

	m_timer.setInterval(450);
	m_timer.setSingleShot(false);
	connect(&m_timer, SIGNAL(timeout()), SLOT(recomputeTime()));

	m_timer.start();

	// PERFORMANCE: reserve space for the longest time string we support
	m_time.reserve(10);

	updateProperties(startTime, iconFile, message);
}

void ActiveCallBanner::updateProperties(uint32_t startTime, const std::string& iconFile, const std::string& msg)
{
	// convert string
	m_message = qFromUtf8Stl(msg);
	// strip trailing/leading white space
	m_message.simplified();

	// load in icon
	std::string rscPath = getResourcePathFromString(iconFile, kPhoneAppId, Settings::LunaSettings()->lunaSystemResourcesPath);
	m_icon.load(QString::fromStdString(rscPath));

	if (startTime < 0xFFFFFFFF)
		m_startTime = startTime;

	QFontMetrics metrics(m_font);
	int requiredWidth = 0;
	int timeLength = 0;

	requiredWidth += m_icon.width() + kSpaceWidth;
	// use MAX_TIME here to avoid potentially eliding the text differently every second because different digits have different widths.
	timeLength = metrics.width(MAX_TIME);
	requiredWidth += timeLength;

	m_elidedMessage = metrics.elidedText(m_message, Qt::ElideRight, m_originalWidth - timeLength - m_icon.width() - 2 * kSpaceWidth);
	if (!m_elidedMessage.isEmpty()) {
		requiredWidth += metrics.width(m_elidedMessage) + kSpaceWidth;
	}

	prepareGeometryChange();
	m_boundingRect.setWidth(requiredWidth);

	update();
}

void ActiveCallBanner::recomputeTime()
{
	uint32_t cTime = (uint32_t)time(NULL);
	uint32_t rTime = (m_startTime >= cTime ? 0 : cTime - m_startTime);
	
	if (rTime >= 356813) {

		m_time = MAX_TIME;
	}
	else {

		m_time.clear();
		m_time.append('(');

		uint32_t hr = rTime / 3600;
		uint32_t mr = rTime % 3600;
		if (hr > 0)	{
			m_time.append(QString::number(hr));
			m_time.append(':');
			if (mr < 600) {
				m_time.append('0');
			}
		}
		m_time.append(QString::number(mr/60));
		m_time.append(':');

		uint32_t sr = mr % 60;
		if (sr < 10)
			m_time.append('0');
		m_time.append(QString::number(sr));
		m_time.append(')');
	}

	update();
}

void ActiveCallBanner::handleTap()
{
	std::string errMsg;
	WebAppMgrProxy::instance()->appLaunch(kPhoneAppId, "{\"action\":\"activecall\"}", "", "", errMsg);
}

void ActiveCallBanner::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QPen oldPen = painter->pen();
	QFont oldFont = painter->font();
	painter->setPen(Qt::white);
	painter->setFont(m_font);

	painter->fillRect(m_boundingRect, Qt::black);

	QRectF rc = m_boundingRect.adjusted(WINDOW_X_PAD, WINDOW_Y_PAD, -WINDOW_X_PAD, -WINDOW_Y_PAD);

	if (!m_icon.isNull()) {
		painter->drawPixmap(rc.topLeft(), m_icon);
		rc.setX(rc.x() + m_icon.width());
	}

	rc.setX(rc.x() + kSpaceWidth);

	int timeLength = 0;

	if (!m_message.isEmpty()) {
		QFontMetrics metrics(m_font);
		if (!m_time.isEmpty()) {
			// use MAX_TIME here to avoid potentially eliding the text differently every second because different digits have different widths.
			timeLength = metrics.width(MAX_TIME);
		}

		m_elidedMessage = metrics.elidedText(m_message, Qt::ElideRight, m_originalWidth - timeLength - m_icon.width() - 2 * kSpaceWidth);

		painter->drawText(rc, m_elidedMessage, QTextOption(Qt::AlignLeft|Qt::AlignVCenter));

		rc.setX(rc.x() + metrics.width(m_elidedMessage) + kSpaceWidth);
	}

	if (!m_time.isEmpty()) {
		painter->drawText(rc, m_time, QTextOption(Qt::AlignLeft|Qt::AlignVCenter));
	}

	painter->setPen(oldPen);
	painter->setFont(oldFont);
}

