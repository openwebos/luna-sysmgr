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

#include "ReticleItem.h"

#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QPropertyAnimation>
#include <QPainter>

#include "QtUtils.h"
#include "AnimationSettings.h"
#include "Settings.h"

ReticleItem::ReticleItem()
{
	setVisible(false);

	std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/penindicator-ripple.png";
	m_pixmap = QPixmap::fromImage(QImage(qFromUtf8Stl(filePath)));

	m_width = m_pixmap.width();
	m_height = m_pixmap.height();
}

ReticleItem::~ReticleItem()
{
	delete m_animation;
}

void ReticleItem::startAt(const QPoint& pos)
{
	if (m_animation)
		m_animation->stop();
	setPos(pos.x(), pos.y());
	setVisible(true);
	setOpacity(1);
	setScale(1);

	QPropertyAnimation* opacityAnimation = new QPropertyAnimation(this, "opacity");
	opacityAnimation->setDuration(AS(reticleDuration));
	opacityAnimation->setStartValue(1.0);
	opacityAnimation->setEndValue(0.0);
	opacityAnimation->setEasingCurve(AS_CURVE(reticleCurve));

	QPropertyAnimation* scaleAnimation = new QPropertyAnimation(this, "scale");
	scaleAnimation->setDuration(AS(reticleDuration));
	scaleAnimation->setStartValue(1.0);
	scaleAnimation->setEndValue(1.5);
	scaleAnimation->setEasingCurve(AS_CURVE(reticleCurve));

	QParallelAnimationGroup* reticleAnimation = new QParallelAnimationGroup;
	reticleAnimation->addAnimation(opacityAnimation);
	reticleAnimation->addAnimation(scaleAnimation);

	QPropertyAnimation* visibility = new QPropertyAnimation(this, "visible");
	visibility->setEndValue(false);
	visibility->setDuration(0);

	m_animation = new QSequentialAnimationGroup;
	m_animation->addAnimation(reticleAnimation);
	m_animation->addAnimation(visibility);
	m_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ReticleItem::animationFinished()
{
	setVisible(false);
}

QRectF ReticleItem::boundingRect() const
{
	return QRectF(-m_width/2, -m_height/2, m_width, m_height);
}

void ReticleItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->drawPixmap(-m_width/2, -m_height/2, m_pixmap);
}
