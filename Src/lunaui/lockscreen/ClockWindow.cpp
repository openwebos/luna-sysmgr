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




#include <string>
#include <cmath>
#include <map>

#include "HostBase.h"
#include "Settings.h"
#include "ClockWindow.h"
#include "Localization.h"
#include "Preferences.h"

#include <QObject>
#include <QPropertyAnimation>
#include <QTextLayout>
#include <QVector>
#include <QHash>
#include <QGraphicsPixmapItem>
#include <QPainter>


ClockWindow::ClockWindow()
	: m_width(0)
{
	memset(m_time, 0, MaxTimeChars);

	loadDigits();

	const HostInfo& info = HostBase::instance()->getInfo();

	if (!m_digits.empty()) {
		int height = m_digits.constBegin().value().height();
		m_bounds = QRect(-info.displayWidth/2, -height/2, info.displayWidth, height);
	}
	else {
		m_bounds = QRect(0, 0, 0, 0);
	}

	updateTimeFormat();

	setVisible(true);
}

ClockWindow::~ClockWindow()
{
}

void ClockWindow::resize(int width, int height)
{
	const HostInfo& info = HostBase::instance()->getInfo();

	prepareGeometryChange();
	if (!m_digits.empty()) {
		int height = m_digits.constBegin().value().height();
		m_bounds = QRect(-info.displayWidth/2, -height/2, info.displayWidth, height);
	}
	else {
		m_bounds = QRect(0, 0, 0, 0);
	}
}

void ClockWindow::loadDigits()
{
	QString digitPath = qFromUtf8Stl(Settings::LunaSettings()->lunaSystemResourcesPath) + "/";
	const ImagePath imagePaths[] = {
		{'0', "screen-lock-clock-0.png"},		{'1', "screen-lock-clock-1.png"},
		{'2', "screen-lock-clock-2.png"},		{'3', "screen-lock-clock-3.png"},
		{'4', "screen-lock-clock-4.png"},		{'5', "screen-lock-clock-5.png"},
		{'6', "screen-lock-clock-6.png"},		{'7', "screen-lock-clock-7.png"},
		{'8', "screen-lock-clock-8.png"},		{'9', "screen-lock-clock-9.png"},
		{':', "screen-lock-clock-colon.png"},	{'.', "screen-lock-clock-decimal.png"},
		{0, 0}
	};

	const ImagePath* cur = imagePaths;
	while (cur->path != 0) {
		m_digits[cur->key] = QPixmap(digitPath + cur->path);
		cur++;
	}
}

// QGraphicsItem::boundingRect
QRectF ClockWindow::boundingRect() const
{
	return m_bounds;
}

// QGraphicsItem::paint
void ClockWindow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{

	int len = strlen(m_time);
	int xOff = (-m_width) / 2;
	for (int i=0; i<len; i++) {
		QPixmap surf = m_digits[m_time[i]];
		if (!surf.isNull()) {
			painter->drawPixmap(xOff, m_bounds.y(), surf.width(), m_bounds.height(), surf);
			xOff += surf.width();
		}
	}
}

void ClockWindow::updateTimeFormat()
{
	m_twelveHour = Preferences::instance()->timeFormat() == "HH12";
}

void ClockWindow::tick()
{
	time_t rawTime;
	struct tm* timeinfo = 0;
	time(&rawTime);
	timeinfo = localtime(&rawTime);

	// get current local time
	if (m_twelveHour) {

		strftime(m_time, MaxTimeChars, LOCALIZED("%I:%M").c_str(), timeinfo);

		if (m_time[0] == '0') {
			memmove(m_time, m_time+1, MaxTimeChars-1);
		}
	}
	else {
		strftime(m_time, MaxTimeChars, LOCALIZED("%H:%M").c_str(), timeinfo);
	}

	// recalculate the width of the new time
	m_width = 0;
	int len = strlen(m_time);
	for (int i=0; i<len; i++) {
		QPixmap surf = m_digits[m_time[i]];
		if (!surf.isNull())
			m_width += surf.width();
	}

	update();
}

