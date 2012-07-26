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




#include "IMEView.h"
#include "HostBase.h"
#include "IMEController.h"
#include "WindowServer.h"

#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QWidget>
#include <QGestureEvent>
#include <QGesture>
#include <QTapGesture>
#include "SingleClickGesture.h"
#include "FlickGesture.h"
#include "ScreenEdgeFlickGesture.h"

IMEView::IMEView(QGraphicsItem* parent)
	: QGraphicsObject(parent)
	, m_imeDataInterface(0)
	, m_acceptingInput(false)
	, m_lastTouchBegin(0)
{
	setAcceptTouchEvents(true);

	grabGesture(Qt::TapGesture);
	grabGesture(Qt::TapAndHoldGesture);
	grabGesture(Qt::PinchGesture);
	grabGesture((Qt::GestureType) SysMgrGestureFlick);
	grabGesture((Qt::GestureType) SysMgrGestureSingleClick);
	grabGesture((Qt::GestureType) SysMgrGestureScreenEdgeFlick);

    setVisible(false);
}

void IMEView::attach(IMEDataInterface* imeDataInterface)
{
	m_imeDataInterface = imeDataInterface;
	if (m_imeDataInterface)
	{
		const HostInfo& info = HostBase::instance()->getInfo();
		m_imeDataInterface->m_screenSize.set(QSize(info.displayWidth, info.displayHeight));
		m_imeDataInterface->m_availableSpace.set(QRect(0, 0, info.displayWidth, info.displayHeight));

		connect(m_imeDataInterface, SIGNAL(signalInvalidateRect(const QRect&)), SLOT(invalidateRect(const QRect&)));
	}
}

void IMEView::setBoundingRect(const QRectF& r)
{
	m_bounds = r;
	prepareGeometryChange();
}

QRectF IMEView::boundingRect() const
{
	return m_bounds;
}

void IMEView::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	if (m_imeDataInterface) {
		m_imeDataInterface->paint(*painter);
	}
}

void IMEView::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if (!IMEController::instance()->isIMEOpened()) {
		event->ignore();
	}
	else {
		if (m_acceptingInput || acceptPoint(event->pos()))
			event->accept();
		else
			event->ignore();
	}
}

bool IMEView::sceneEvent(QEvent* event)
{
	switch (event->type()) {
	case QEvent::GestureOverride: {
		// consume all gestures if we are handling touches
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* g = 0;

		if ((g = ge->gesture(Qt::TapGesture))) {
			if (acceptPoint(mapFromScene(ge->mapToGraphicsScene(g->hotSpot())))) {
				event->accept();
				return true;
			}
		}
		else if ((g = ge->gesture(Qt::TapAndHoldGesture))) {
			if (acceptPoint(mapFromScene(ge->mapToGraphicsScene(g->hotSpot())))) {
				event->accept();
				return true;
			}
		}
		else if ((g = ge->gesture(Qt::PinchGesture))) {
			if (acceptPoint(mapFromScene(ge->mapToGraphicsScene(g->hotSpot())))) {
				event->accept();
				return true;
			}
		}
		else if ((g = ge->gesture((Qt::GestureType) SysMgrGestureFlick))) {
			if (acceptPoint(mapFromScene(ge->mapToGraphicsScene(g->hotSpot())))) {
				event->accept();
				return true;
			}
		}
		else if ((g = ge->gesture((Qt::GestureType) SysMgrGestureSingleClick))) {
			if (acceptPoint(mapFromScene(ge->mapToGraphicsScene(g->hotSpot())))) {
				event->accept();
				return true;
			}
		}
		else if ((g = ge->gesture((Qt::GestureType) SysMgrGestureSingleClick))) {
			if (acceptPoint(mapFromScene(ge->mapToGraphicsScene(g->hotSpot())))) {
				event->accept();
				return true;
			}
		}
		}
		break;
	case QEvent::Gesture: {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* g = ge->gesture(Qt::TapGesture);
		if (g && g->state() == Qt::GestureFinished) {
			tapEvent(static_cast<QTapGesture*>(g));
			return true;
		}
		if (!g) {
			QGesture* g = ge->gesture((Qt::GestureType) SysMgrGestureScreenEdgeFlick);
			if (g && g->state() == Qt::GestureFinished) {
				screenEdgeFlickEvent(static_cast<ScreenEdgeFlickGesture*>(g));
				return true;
			}
		}
		}
		break;
	case QEvent::TouchBegin:
		touchBegin(static_cast<QTouchEvent*>(event));
		return event->isAccepted();
	case QEvent::TouchUpdate:
		touchUpdate(static_cast<QTouchEvent*>(event));
		return event->isAccepted();
	case QEvent::TouchEnd:
		touchEnd(static_cast<QTouchEvent*>(event));
		return event->isAccepted();
	default: 
		break;
	}
	return QGraphicsObject::sceneEvent(event);
}

void IMEView::touchBegin(QTouchEvent* te)
{
	m_acceptingInput = false;

	// we only starting consuming input if the primary touch falls within the
	// IME's soft input area
	const QList<QTouchEvent::TouchPoint>& points = te->touchPoints();
	Q_FOREACH(QTouchEvent::TouchPoint tp, points) {

		if (!tp.isPrimary())
			continue;
		if (acceptPoint(tp.pos())) {
			m_acceptingInput = true;
			m_lastTouchBegin = SingletonTimer::currentTime();
			break;
		}
	}

	te->setAccepted(m_acceptingInput);
	if (m_acceptingInput && m_imeDataInterface)
		m_imeDataInterface->touchEvent(*te);
}

void IMEView::touchUpdate(QTouchEvent* te)
{
	te->setAccepted(m_acceptingInput);
	if (!m_acceptingInput || !m_imeDataInterface)
		return;

	m_imeDataInterface->touchEvent(*te);
}

void IMEView::touchEnd(QTouchEvent* te)
{
	te->setAccepted(m_acceptingInput);
	if (!m_acceptingInput || !m_imeDataInterface)
		return;

	m_imeDataInterface->touchEvent(*te);

	m_acceptingInput = false;
}

void IMEView::tapEvent(QTapGesture* tap)
{
	if (!m_acceptingInput || !m_imeDataInterface || !tap)
		return;

	QPoint tapPt = mapFromScene(tap->position()).toPoint();
	m_imeDataInterface->tapEvent(tapPt);
}

void IMEView::screenEdgeFlickEvent(ScreenEdgeFlickGesture* g)
{
	if (!m_acceptingInput || !m_imeDataInterface || !g)
		return;

    OrientationEvent::Orientation orientation = WindowServer::instance()->getUiOrientation();
	switch (orientation) {
    case OrientationEvent::Orientation_Up: {
		if (g->edge() != ScreenEdgeFlickGesture::EdgeBottom)
			return;
		break;
	}
    case OrientationEvent::Orientation_Down: {
		if (g->edge() != ScreenEdgeFlickGesture::EdgeTop)
			return;
		break;
	}
    case OrientationEvent::Orientation_Left: {
		if (g->edge() != ScreenEdgeFlickGesture::EdgeLeft)
			return;
		break;
	}
    case OrientationEvent::Orientation_Right: {
		if (g->edge() != ScreenEdgeFlickGesture::EdgeRight)
			return;
		break;
	}
	default: {
		g_warning("Unknown UI orientation");
		return;
	}
	}
	
	m_imeDataInterface->screenEdgeFlickEvent(); 
}

bool IMEView::acceptPoint(const QPointF& pt)
{
	if (!IMEController::instance()->isIMEOpened())
		return false;
	if (!m_imeDataInterface)
		return false;
	if (m_imeDataInterface->m_hitRegion.get().contains(pt.toPoint()))
		return true;
	int graceZone = (SingletonTimer::currentTime() < m_lastTouchBegin + 500) ? 40 : 0;
	return (pt.y() >= (m_bounds.height() - m_imeDataInterface->m_keyboardHeight.get() - graceZone));
}

void IMEView::invalidateRect(const QRect &  rect)
{
	update(rect);
}
