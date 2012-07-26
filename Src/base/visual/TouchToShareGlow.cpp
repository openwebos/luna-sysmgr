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




#include "TouchToShareGlow.h"

#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QPropertyAnimation>
#include <QPainter>
#include <QRadialGradient>

#include "AnimationSettings.h"
#include "HostBase.h"
#include "Settings.h"
#include "SystemService.h"

TouchToShareGlow::TouchToShareGlow()
{
	setVisible(false);

	const HostInfo& info = HostBase::instance()->getInfo();
	int width = info.displayWidth;
	int height = info.displayHeight;

	int angle = Settings::LunaSettings()->homeButtonOrientationAngle;
	
	if (angle == 90 || angle == 270)
		qSwap(width, height);
	
 	m_boundingRect = QRectF(-width/2, -height/2, width, height);

	switch (angle) {
	case 90:
		setPos(0, width/2);
		break;
	case 270:
		setPos(height, width/2);
		break;
	case 180:
		setPos(width/2, 0);
		break;
	case 0:
	default:
		setPos(width/2, height);
		break;
	}
		
	setRotation(angle);

	int r = m_boundingRect.height()/4;
	
    m_gradient = QRadialGradient(0, 0, r, 0, -r/4);

	QGradientStops stops;
	stops.append(QGradientStop(0, QColor(0xff,0xff,0xff,0xAf)));
	stops.append(QGradientStop(0.25, QColor(0xff, 0xff ,0xff, 0x1f)));
	stops.append(QGradientStop(0.5, QColor(0xff, 0xff ,0xff, 0xff)));
	stops.append(QGradientStop(0.75, QColor(0xff, 0xff ,0xff, 0x1f)));
	stops.append(QGradientStop(1.0,  QColor(0xff, 0xff, 0xff, 0x01)));
	m_gradient.setStops(stops);

	connect(SystemService::instance(), SIGNAL(signalTouchToShareCanTap(bool)),
			SLOT(slotCanTap(bool)));
}

TouchToShareGlow::~TouchToShareGlow()
{
}

QRectF TouchToShareGlow::boundingRect() const
{
	return m_boundingRect;   
}

void TouchToShareGlow::paint(QPainter* painter, const QStyleOptionGraphicsItem*,
							 QWidget*)
{
	painter->fillRect(m_boundingRect, m_gradient);
}

void TouchToShareGlow::start()
{
	if (m_animation)
		return;

	setVisible(true);	
	setOpacity(1);
	setScale(1);

	QPropertyAnimation* opacityAnimation = new QPropertyAnimation(this, "opacity");
	opacityAnimation->setDuration(1000);
	opacityAnimation->setStartValue(1.0);
	opacityAnimation->setEndValue(0.0);
	opacityAnimation->setEasingCurve(AS_CURVE(reticleCurve));

	QPropertyAnimation* scaleAnimation = new QPropertyAnimation(this, "scale");
	scaleAnimation->setDuration(1000);
	scaleAnimation->setStartValue(1.0);
	scaleAnimation->setEndValue(4.0);
	scaleAnimation->setEasingCurve(AS_CURVE(reticleCurve));

	QParallelAnimationGroup* reticleAnimation = new QParallelAnimationGroup;
	reticleAnimation->addAnimation(opacityAnimation);
	reticleAnimation->addAnimation(scaleAnimation);

	m_animation = reticleAnimation;
	m_animation->setLoopCount(-1);
	m_animation->start();
}

void TouchToShareGlow::stop()
{
    setVisible(false);
	if (m_animation)
		delete m_animation;
}

void TouchToShareGlow::slotCanTap(bool val)
{
	if (val)
		start();
	else
		stop();    
}
