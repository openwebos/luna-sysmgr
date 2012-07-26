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

#include "FlickGestureRecognizer.h"

#include <QEvent>
#include <QTouchEvent>
#include <QTransform>
#include <QDebug>

#include "FlickGesture.h"

#include "Time.h"
#include "HostBase.h"

FlickGestureRecognizer::FlickGestureRecognizer()
	: m_maxSamples(3)
{
}

FlickGestureRecognizer::~FlickGestureRecognizer()
{
}

QGesture* FlickGestureRecognizer::create(QObject* target)
{
	return new FlickGesture;
}

void FlickGestureRecognizer::addSample(const QPoint& position, int timestamp)
{
	if (m_points.size() >= m_maxSamples) {
		m_points.dequeue();
		m_timestamps.dequeue();
	}

	m_points.enqueue(position);
	m_timestamps.enqueue(timestamp);
}

QGestureRecognizer::Result FlickGestureRecognizer::recognize(QGesture* gesture, QObject* watched, QEvent* event)
{
	FlickGesture* g = static_cast<FlickGesture*>(gesture);
	QGestureRecognizer::Result result = QGestureRecognizer::Ignore;

    switch (event->type()) {
	case QEvent::TouchBegin: {
		QTouchEvent::TouchPoint tp = static_cast<QTouchEvent*>(event)->touchPoints().at(0);
		g->setHotSpot(tp.screenPos().toPoint());
		m_points.clear();
		m_timestamps.clear();

		break;
		// Fall through intentional here
	}
	case QEvent::TouchUpdate: {
        result = QGestureRecognizer::MayBeGesture;
		QTouchEvent::TouchPoint tp = static_cast<QTouchEvent*>(event)->touchPoints().at(0);
		addSample(tp.screenPos().toPoint(), Time::curTimeMs());

		break;
	}
	case QEvent::TouchEnd: {
		QTouchEvent::TouchPoint tp = static_cast<QTouchEvent*>(event)->touchPoints().at(0);
//		addSample(tp.screenPos().toPoint(), Time::curTimeMs());

		if (m_points.size() < 2) {
			result = QGestureRecognizer::CancelGesture;
		}
		else {
			qreal xVel = m_points.last().x() - m_points.first().x();
			qreal yVel = m_points.last().y() - m_points.first().y();

			QPointF moved = QPointF(xVel, yVel);

			qreal elapsed = (m_timestamps.last() - m_timestamps.first()) / 1000.0;

			int movedBy = moved.manhattanLength();
			qreal swipeSpeed = movedBy / elapsed; // pixels / second
			if (swipeSpeed > 500) {

				elapsed *= m_points.size();
				if (qFuzzyIsNull(elapsed)) {
					xVel = 500;
					yVel = 500;
				}
				else {
					xVel = (xVel) / elapsed;
					yVel = (yVel) / elapsed;
				}

				g->m_velocity = HostBase::instance()->map(QPoint(xVel, yVel));
				g->m_endPos = tp.screenPos().toPoint();

				result = QGestureRecognizer::FinishGesture;
			}
			else {
				result = QGestureRecognizer::CancelGesture;
			}
		}

		m_points.clear();
		m_timestamps.clear();

		break;
	}
	default:
		result = QGestureRecognizer::Ignore;
		break;
	}

	return result;
}
