/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#include "WSOverlayScreenShotAnimation.h"

#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QPropertyAnimation>
#include <QPainter>

#include "AnimationSettings.h"
#include "HostBase.h"
#include "Settings.h"
#include "SystemService.h"
#include "WindowServer.h"

WSOverlayScreenShotAnimation::WSOverlayScreenShotAnimation()
{
	setVisible(false);

	const HostInfo& info = HostBase::instance()->getInfo();
	int width = info.displayWidth;
	int height = info.displayHeight;

	setPos(width/2, height/2);

	int r = qMin(width, height);
	r = r/2;

	m_boundingRect = QRectF(-width/2, -height/2, width, height);
    m_gradient = QRadialGradient(0, 0, r, 0, 0);

	QGradientStops stops;
	stops.append(QGradientStop(0, QColor(0xFF,0xFF,0xFF,0xFF)));
	stops.append(QGradientStop(0.15, QColor(0xFF, 0xFF, 0xFF, 0xC0)));
	stops.append(QGradientStop(0.5, QColor(0xFF, 0xFF ,0xD0, 0xF0)));
	stops.append(QGradientStop(0.75, QColor(0xFF, 0xFF, 0xD0, 0x0F)));
	stops.append(QGradientStop(1.0,  QColor(0xFF, 0xFF, 0xD0, 0x00)));
	m_gradient.setStops(stops);

	connect(WindowServer::instance(), SIGNAL(signalAboutToTakeScreenShot()),
			SLOT(stop()));
	connect(WindowServer::instance(), SIGNAL(signalTookScreenShot()),
			SLOT(start()));
}

WSOverlayScreenShotAnimation::~WSOverlayScreenShotAnimation()
{
    
}

void WSOverlayScreenShotAnimation::start()
{
	if (m_animation) {
		m_animation->stop();
		delete m_animation;
	}

	setVisible(true);	

	int duration = 900;

	QPropertyAnimation* opacityAnimation = new QPropertyAnimation(this, "opacity");
	opacityAnimation->setDuration(duration);
	opacityAnimation->setStartValue(1.0);
	opacityAnimation->setEndValue(0.0);

	m_animation = opacityAnimation;
	m_animation->start(QAbstractAnimation::DeleteWhenStopped);

	connect(m_animation, SIGNAL(finished()), SLOT(finished()));
}

void WSOverlayScreenShotAnimation::stop()
{
    setVisible(false);
	if (m_animation)
		delete m_animation;    
}

QRectF WSOverlayScreenShotAnimation::boundingRect() const
{
	return m_boundingRect;    
}

void WSOverlayScreenShotAnimation::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->fillRect(m_boundingRect, QColor(0,0,0,0x88));
    painter->fillRect(m_boundingRect, m_gradient);
}

void WSOverlayScreenShotAnimation::finished()
{
	setVisible(false);    
}
