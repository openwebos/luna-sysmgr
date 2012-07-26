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

#include "CardTransition.h"

#include "AnimationEquations.h"
#include "AnimationSettings.h"
#include "CardWindow.h"
#include "Time.h"

CardTransition::CardTransition(CardWindow* win,
							   QPixmap* fromPixmap,
							   const std::string& transitionName)
	: m_win(win)
	, m_fromScenePixmap(fromPixmap)
	, m_transitionType(Transition_Invalid)
	, m_transitionIsPop(false)
	, m_startTime(0)
	, m_completed(false)
	, m_equation(0)
{
	if (strncmp(transitionName.c_str(), "cross-fade", strlen("cross-fade")) == 0)
		m_transitionType = Transition_CrossFade;
	else if (strncmp(transitionName.c_str(), "zoom-fade", strlen("zoom-fade")) == 0)
		m_transitionType = Transition_ZoomAndCrossFade;
	else {
		g_critical("%s: Invalid transition type: %s specified",
				   __PRETTY_FUNCTION__, transitionName.c_str());
		return;
	}

	if (transitionName.find("pop") != std::string::npos)
		m_transitionIsPop = true;

	g_message("Transition %s, type: %d, isPop: %d",
			  transitionName.c_str(), m_transitionType, m_transitionIsPop);

	int animationStrength = AS(cardTransitionCurve);
	m_equation = AS_EASEOUT(animationStrength);
	
	AnimObject& aObj = m_fromSceneAnimObject;
	AnimObject& bObj = m_toSceneAnimObject;
	if (m_transitionIsPop) {
		aObj = m_toSceneAnimObject;
		bObj = m_fromSceneAnimObject;
	}

	float scaleFactor;

	if (m_transitionIsPop)
		scaleFactor = 0.75f;
	else
		scaleFactor = 1.25f;

	aObj.startScale = 1.0f;
	aObj.startAlpha = 0xFF;

	aObj.targetScale = scaleFactor;
	aObj.targetAlpha = 0x00;

	// --------------------------------------------------------------

	if (m_transitionIsPop)
		scaleFactor = 1.25f;
	else
		scaleFactor = 0.75f;

	bObj.targetScale = 1.0f;
	bObj.targetAlpha = 0xFF;
	
	bObj.startScale = scaleFactor;
	bObj.startAlpha = 0x00;
}

CardTransition::~CardTransition()
{
}

void CardTransition::draw(QPainter* painter, const QPainterPath &path, bool maximized)
{
	if (G_UNLIKELY(m_startTime == 0)) {
		m_startTime = Time::curTimeMs();
	}

	switch (m_transitionType) {
	case Transition_ZoomAndCrossFade:
		zoomAndCrossFadeTick(painter, path, maximized);
		break;
	case Transition_CrossFade:
		crossFadeTick(painter, path, maximized);
		break;
	default:
		break;
	}
}

void CardTransition::zoomAndCrossFadeTick(QPainter* painter, const QPainterPath &roundedRectPath, bool maximized)
{
    uint32_t currTime = Time::curTimeMs();

	PValue cfs, cts;
	qreal cfa, cta;

	int durationMs = AS(cardTransitionDuration);
	int animationStrength = AS(cardTransitionCurve);

	if (G_LIKELY(currTime < (m_startTime + durationMs))) {

		PValue dt = (int) (currTime - m_startTime);

		cfs = m_equation(dt, m_fromSceneAnimObject.startScale,
						 m_fromSceneAnimObject.targetScale,
						 durationMs, animationStrength);

		cfa = m_equation(dt, m_fromSceneAnimObject.startAlpha,
						 m_fromSceneAnimObject.targetAlpha,
						 durationMs, animationStrength) / 255;

		cts = m_equation(dt, m_toSceneAnimObject.startScale,
						 m_toSceneAnimObject.targetScale,
						 durationMs, animationStrength);

		cta = m_equation(dt, m_toSceneAnimObject.startAlpha,
						 m_toSceneAnimObject.targetAlpha,
						 durationMs, animationStrength) / 255;
	} else {
		cfs = m_fromSceneAnimObject.targetScale;
		cfa = m_fromSceneAnimObject.targetAlpha / 255;

		cts = m_toSceneAnimObject.targetScale;
		cta = m_toSceneAnimObject.targetAlpha / 255;
	}

	QRect targetRect = m_win->transitionBoundingRect().toRect();
	QRect sourceRect = QRect(0, 0, targetRect.width(), targetRect.height());

	QPainterPath painterPath;
	if (maximized) {
		painterPath.addRect(targetRect);
	} else {
		painterPath = roundedRectPath;
	}

	qreal oldOpacity = painter->opacity();
	QPainter::CompositionMode oldMode = painter->compositionMode();

	painter->fillPath(painterPath, QColor(0x00, 0x00, 0x00, 0xFF));

	const QPixmap* toScenePixmap = m_win->acquireScreenPixmap();
	if (G_UNLIKELY(!toScenePixmap))
		return;
	
	if (G_LIKELY(maximized))
		painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

	if (m_transitionIsPop) {
		painter->setOpacity(cta);
		painter->scale((qreal) cts, (qreal) cts);
		painter->setCompositionMode(QPainter::CompositionMode_Source);
		painter->setBrushOrigin(targetRect.x(), targetRect.y());
		painter->fillPath(painterPath, *toScenePixmap);

		painter->scale(1 / (qreal) cts, 1 / (qreal) cts);
		painter->setOpacity(cfa);
		painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter->fillPath(painterPath, *m_fromScenePixmap);
		painter->setBrushOrigin(0, 0);

	} else {
		painter->setOpacity(cfa);
		painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter->setBrushOrigin(targetRect.x(), targetRect.y());
		painter->fillPath(painterPath, *m_fromScenePixmap);

		painter->setOpacity(cta);
		painter->scale((qreal) cts, (qreal) cts);
		painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
		// the "to" scene of the push transition always uses the rounded rect path, because its borders 
		// are visible regardless of whether the card is maximized or not
		painter->fillPath(painterPath, *toScenePixmap);
		painter->setBrushOrigin(0, 0);

		painter->scale(1 / (qreal) cts, 1 / (qreal) cts);
	}

	if (G_LIKELY(maximized))
		painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

	painter->setCompositionMode(oldMode);
	painter->setOpacity(oldOpacity);
}

void CardTransition::crossFadeTick(QPainter* painter, const QPainterPath &roundedRectPath, bool maximized)
{
    uint32_t currTime = Time::curTimeMs();

    qreal cfa;

	int durationMs = AS(cardTransitionDuration);
	int animationStrength = AS(cardTransitionCurve);

	if (G_LIKELY(currTime < (m_startTime + durationMs))) {
		PValue dt = (int) (currTime - m_startTime);
		cfa = m_equation(dt, m_fromSceneAnimObject.startAlpha, m_fromSceneAnimObject.targetAlpha, durationMs, animationStrength) / 255;
	} else {
		cfa = m_fromSceneAnimObject.targetAlpha / 255;
	}

	QRect targetRect = m_win->transitionBoundingRect().toRect();
	QRect sourceRect = QRect(0, 0, targetRect.width(), targetRect.height());
	QPainterPath painterPath;
	if (maximized) {
		painterPath.addRect(targetRect);
	} else {
		painterPath = roundedRectPath;
	}

	qreal oldOpacity = painter->opacity();
	QPainter::CompositionMode oldMode = painter->compositionMode();

	const QPixmap* toScenePixmap = m_win->acquireScreenPixmap();
	if (G_UNLIKELY(!toScenePixmap))
		return;
	
	if (G_LIKELY(maximized))
		painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

	painter->setOpacity(1);
	painter->setCompositionMode(QPainter::CompositionMode_Source);
	painter->setBrushOrigin(targetRect.x(), targetRect.y());
	painter->fillPath(painterPath, *toScenePixmap);

	painter->setOpacity(cfa);
	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter->fillPath(painterPath, *m_fromScenePixmap);
	painter->setBrushOrigin(0, 0);

	painter->setOpacity(oldOpacity);

	if (G_LIKELY(maximized))
		painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
}

bool CardTransition::finished()
{
	if (G_UNLIKELY(m_startTime == 0)) {
		m_startTime = Time::curTimeMs();
	}

	int durationMs = AS(cardTransitionDuration);
	
	uint32_t currTime = Time::curTimeMs();
	return (currTime > (m_startTime + durationMs));    
}
