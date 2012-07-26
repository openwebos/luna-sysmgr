/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "FullEraseConfirmationWindow.h"
#include "Localization.h"
#include "Settings.h"
#include "WindowServerLuna.h"
#include "HostBase.h"
#include "QtUtils.h"
#include "SystemUiController.h"

static const int kTitleFontHeight = 26;
static const int kDescriptionFontHeight = 16;
static const int kCountDownFontHeight = 20;

const qreal titleRatio = 0.05;
const qreal iconRatio = 0.167;
const qreal countRatio = 0.50;
const qreal descRatio = 0.70;

static const char* kFullEraseConfirmationTitle = "Full Erase";
static const char* kFullEraseConfirmationDescription = "Erases applications you installed and all your files on the USB drive.";
static const char* kFullEraseCountDownPlural = "Continue holding for %0 seconds...";
static const char* kFullEraseCountDownSingular = "Continue holding for 1 second...";

FullEraseConfirmationWindow::FullEraseConfirmationWindow()
{
	m_boundingRect.setRect(-SystemUiController::instance()->currentUiWidth()/2, -SystemUiController::instance()->currentUiHeight()/2,
			               SystemUiController::instance()->currentUiWidth(), SystemUiController::instance()->currentUiHeight());

	m_secondsLeft = 6;

	m_timer.setInterval(1000);
	m_timer.setSingleShot(false);
	connect(&m_timer, SIGNAL(timeout()), SLOT(slotTimerTicked()));

	std::string iconPath = Settings::LunaSettings()->lunaSystemResourcesPath + "/warning-system.png";
	m_warningPix.load(qFromUtf8Stl(iconPath));
	if (m_warningPix.isNull()) {
		g_warning("FullEraseConfirmationWindow failed to load the icon");
	}
	m_warningOffsetY = m_boundingRect.y() + (m_boundingRect.height() * iconRatio);

	m_font = QFont(QString::fromStdString(Settings::LunaSettings()->fontBanner));

	setupLabels();

	m_timer.start();
}

FullEraseConfirmationWindow::~FullEraseConfirmationWindow()
{
	m_timer.stop();
}

void FullEraseConfirmationWindow::slotTimerTicked()
{
	if (m_secondsLeft == 0) {
		m_timer.stop();
		Q_EMIT signalFullEraseConfirmed();
		return;
	}

	m_secondsLeft--;

	updateCountDownText();

	update();
}

void FullEraseConfirmationWindow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	painter->fillRect(m_boundingRect, Qt::black);

	QPointF center = m_boundingRect.center();
	painter->drawPixmap(center.x() - (m_warningPix.width()/2),
						m_warningOffsetY,
						m_warningPix);

	QFont oldFont = painter->font();
	QPen oldPen = painter->pen();
	painter->setPen(Qt::white);

	QTextOption txtOpt(Qt::AlignCenter);
	txtOpt.setWrapMode(QTextOption::WordWrap);

	m_font.setPixelSize(kTitleFontHeight);
	painter->setFont(m_font);
	painter->drawText(m_titleBounds, m_title, txtOpt);

	m_font.setPixelSize(kCountDownFontHeight);
	painter->setFont(m_font);
	painter->drawText(m_countDownBounds, m_countDown, txtOpt);

	m_font.setPixelSize(kDescriptionFontHeight);
	painter->setFont(m_font);
	painter->drawText(m_descBounds, m_description, txtOpt);

	painter->setFont(oldFont);
	painter->setPen(oldPen);
}

void FullEraseConfirmationWindow::setupLabels()
{
	QRect bounds = m_boundingRect.toRect();

	m_title = qFromUtf8Stl(LOCALIZED(kFullEraseConfirmationTitle));
	m_font.setPixelSize(kTitleFontHeight);
	QFontMetrics metrics(m_font);
	m_titleBounds = metrics.boundingRect(bounds, Qt::AlignCenter|Qt::TextWordWrap, m_title);
	m_titleBounds.moveTop(m_boundingRect.y() + (m_boundingRect.height() * titleRatio));
	
	m_description = qFromUtf8Stl(LOCALIZED(kFullEraseConfirmationDescription));
	m_font.setPixelSize(kDescriptionFontHeight);
	metrics = QFontMetrics(m_font);
	m_descBounds = metrics.boundingRect(bounds, Qt::AlignCenter|Qt::TextWordWrap, m_description);
	m_descBounds.moveTop(m_boundingRect.y() + (m_boundingRect.height() * descRatio));

	updateCountDownText();
}

void FullEraseConfirmationWindow::updateCountDownText()
{
	if (m_secondsLeft == 1) {
		m_countDown = qFromUtf8Stl(LOCALIZED(kFullEraseCountDownSingular));
	} else {
		m_countDown = qFromUtf8Stl(LOCALIZED(kFullEraseCountDownPlural)).arg(m_secondsLeft);
	}

	m_font.setPixelSize(kCountDownFontHeight);
	QFontMetrics metrics(m_font);
	m_countDownBounds = metrics.boundingRect(m_boundingRect.toRect(), Qt::AlignCenter|Qt::TextWordWrap, m_countDown);
	m_countDownBounds.moveTop(m_boundingRect.y() + (m_boundingRect.height() * countRatio));
}

