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




#include "StatusBarIcon.h"
#include "Settings.h"
#include "AnimationSettings.h"

#include <QPainter>

StatusBarIcon::StatusBarIcon(StatusBarIconContainer* parent)
	: m_visible(false)
	, m_animState(NO_ANIMATION)
	, m_parent(parent)
	, m_animWidth(0.0)
	, m_animOpacity(0.0)
	, m_curve(AS_CURVE(statusBarItemSlideCurve))
{
	m_imgPtr = NULL;
}

StatusBarIcon::~StatusBarIcon()
{
}

void StatusBarIcon::setImage(QPixmap* img)
{
	m_imgPtr = img;
}

QRect StatusBarIcon::boundingRect() const
{
	return m_bounds;
}

void StatusBarIcon::updateBoundingRect(bool forceRepaint)
{
	if(m_imgPtr && !m_imgPtr->isNull() && m_visible) {
		int imgWidth, imgHeight, maxHeight;
		imgWidth = m_imgPtr->width();
		imgHeight = m_imgPtr->height();
		maxHeight = Settings::LunaSettings()->positiveSpaceTopPadding - 2;

		if(imgHeight > maxHeight) {
			qreal ratio = ((qreal)imgHeight) / ((qreal)maxHeight);
			imgHeight = maxHeight;
			imgWidth = imgWidth / ratio;
		}

		if(m_animState == NO_ANIMATION) {
			m_bounds = QRect(-imgWidth/2, -imgHeight/2, imgWidth, imgHeight);
		} else if(m_animState == SLIDE_ANIMATION) {
			int width = imgWidth * m_animWidth;
			m_bounds = QRect(-width/2, -imgHeight/2, width, imgHeight);
		}
	} else {
		m_bounds = QRect();
	}

	if(m_parent)
		m_parent->updateBoundingRect(forceRepaint);
}

void StatusBarIcon::show()
{
	if(!m_visible) {
		setVisible(true);

		m_animState = SLIDE_ANIMATION;

		if(!m_animPtr.isNull()) {
			m_animPtr->stop();
			m_animPtr = NULL;
		}
		m_animPtr = new tIconAnim(this, &StatusBarIcon::animValueChanged);
		m_animPtr->setEasingCurve(m_curve);
		m_animPtr->setDuration(AS(statusBarItemSlideDuration));
		m_animPtr->setStartValue(0.0);
		m_animPtr->setEndValue(1.0);
		m_animPtr->setDirection(QAbstractAnimation::Forward);
		connect(m_animPtr, SIGNAL(finished()), SLOT(slotAnimFinished()));

		m_animPtr->start(QAbstractAnimation::DeleteWhenStopped);
	} else if((m_animState == SLIDE_ANIMATION) &&
			  (!m_animPtr.isNull()) &&
			  (m_animPtr->state() !=  QAbstractAnimation::Stopped) &&
			  (m_animPtr->direction() != QAbstractAnimation::Forward)) {
		// already animating, so just revert it
		m_animPtr->pause();
		m_animPtr->setDirection(QAbstractAnimation::Forward);
		m_animPtr->resume();
	}

	updateBoundingRect();
}

void StatusBarIcon::hide()
{
	if(m_visible) {

		if((m_animPtr.isNull()) || (m_animPtr->state() ==  QAbstractAnimation::Stopped))
		 {
			m_animState = SLIDE_ANIMATION;

			if(!m_animPtr.isNull()) {
				m_animPtr->stop();
				m_animPtr = NULL;
			}
			m_animPtr = new tIconAnim(this, &StatusBarIcon::animValueChanged);
			m_animPtr->setEasingCurve(m_curve);
			m_animPtr->setDuration(AS(statusBarItemSlideDuration));
			m_animPtr->setStartValue(0.0);
			m_animPtr->setEndValue(1.0);
			m_animPtr->setDirection(QAbstractAnimation::Backward);
			connect(m_animPtr, SIGNAL(finished()), SLOT(slotAnimFinished()));

			m_animPtr->start(QAbstractAnimation::DeleteWhenStopped);
		 } else if ((!m_animPtr.isNull()) &&
				    (m_animPtr->state() !=  QAbstractAnimation::Stopped) &&
				    (m_animPtr->direction() != QAbstractAnimation::Backward)){
			// already animating, so revert it
			m_animPtr->pause();
			m_animPtr->setDirection(QAbstractAnimation::Backward);
			m_animPtr->resume();
		}
	}

	updateBoundingRect();
}

void StatusBarIcon::paint(QPainter* painter, QPoint centerRight, int fraction)
{
	qreal opacity = painter->opacity();
	if(m_visible && painter && m_imgPtr && !m_imgPtr->isNull()) {
		int imgWidth, imgHeight, maxHeight;
		qreal ratio = 1.0;
		bool scaled = false;
		imgWidth = m_imgPtr->width();
		imgHeight = m_imgPtr->height();
		maxHeight = Settings::LunaSettings()->positiveSpaceTopPadding - 2;

		if(imgHeight > maxHeight) {
			ratio = ((qreal)imgHeight) / ((qreal)maxHeight);
			imgHeight = maxHeight;
			imgWidth = imgWidth / ratio;
			scaled = true;
			painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
		}

		if(m_animState == NO_ANIMATION) {
			if(fraction < 0) {
				painter->drawPixmap(centerRight.x() - imgWidth, centerRight.y() - imgHeight/2,
									imgWidth, imgHeight,
									*m_imgPtr,
									0, 0, m_imgPtr->width(), m_imgPtr->height());
			} else {
				painter->setOpacity(0.5 * opacity);
				painter->drawPixmap(centerRight.x() - imgWidth, centerRight.y() - imgHeight/2,
									fraction/ratio, imgHeight,
									*m_imgPtr,
									0, 0, fraction, m_imgPtr->height());
				painter->setOpacity(opacity);
			}
		} else if(m_animState == SLIDE_ANIMATION) {
			int width = imgWidth * m_animWidth;
			if(fraction < 0) {
				painter->setOpacity(m_animOpacity * opacity);
				painter->drawPixmap(centerRight.x() - width, centerRight.y() - imgHeight/2,
									width, imgHeight,
									*m_imgPtr,
									0, 0, m_imgPtr->width() * m_animWidth, m_imgPtr->height());
			} else {
				painter->setOpacity(m_animOpacity * opacity * 0.5);
				painter->drawPixmap(centerRight.x() - width, centerRight.y() - imgHeight/2,
									(fraction/ratio) * m_animWidth, imgHeight,
									*m_imgPtr,
									0, 0, fraction * m_animWidth, m_imgPtr->height());
			}
			painter->setOpacity(opacity);
		}

		if(scaled)
			painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
	}
}

void StatusBarIcon::animValueChanged(const QVariant& value)
{
	m_animWidth   = m_curve.valueForProgress(CLAMP(value.toReal() * 2.0, 0.0, 1.0));
	m_animOpacity = CLAMP(value.toReal(), 0.0, 1.0);
	updateBoundingRect(true);
}

void StatusBarIcon::slotAnimFinished()
{
	if ((m_animState == SLIDE_ANIMATION) && (!m_animPtr.isNull()) && (m_animPtr->direction() == QAbstractAnimation::Forward)) {
		// done sliding in
	} else if ((m_animState == SLIDE_ANIMATION) && (!m_animPtr.isNull()) && (m_animPtr->direction() == QAbstractAnimation::Backward)) {
		// done sliding out
		setVisible(false);
		// WARNING: This can cause a "self delete"
	}

	m_animState = NO_ANIMATION;
	if(m_visible)
		updateBoundingRect();
}




