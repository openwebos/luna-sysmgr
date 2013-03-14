/* @@@LICENSE
*
*      Copyright (c) 2013 Hewlett-Packard Development Company, L.P.
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
#include <QTime>

#include "FlickGesture.h"

#include "Time.h"
#include "HostBase.h"
#include "FlickEvent.h"

Qt::GestureType FlickGesture::type = Qt::CustomGesture;

static QTime timer;

// These are used by touch event based flicks and can be tuned if it is
// too hard/easy to trigger a flick
static const qreal kFlickTriggerThresholdMin = 2.5f;
static const qreal kFlickTriggerThresholdMax = 11.0f;
static const int kFlickVelocityMultiplier = 100;

FlickGestureRecognizer::FlickGestureRecognizer()
{
}

FlickGestureRecognizer::~FlickGestureRecognizer()
{
}

QGesture* FlickGestureRecognizer::create(QObject* target)
{
	return new FlickGesture;
}

QGestureRecognizer::Result FlickGestureRecognizer::recognize(QGesture* gesture, QObject* watched, QEvent* event)
{
    FlickGesture* g = static_cast<FlickGesture*>(gesture);
    QGestureRecognizer::Result result = QGestureRecognizer::Ignore;
    QTouchEvent *te = static_cast<QTouchEvent *>(event);

    switch (event->type()) {
        case SysMgrGestureFlick: {
            FlickEvent* ev = static_cast<FlickEvent*>(event);
            if (ev->state() == Qt::GestureStarted) {
                g->m_velocity = ev->velocity();
                g->m_endPos = ev->velocity();
                g->m_startPos = ev->startPos();
                result = QGestureRecognizer::TriggerGesture;
            } else if (ev->state() == Qt::GestureFinished) {
                g->m_velocity = ev->velocity();
                g->m_endPos = ev->velocity();
                g->m_startPos = ev->startPos();
                result = QGestureRecognizer::FinishGesture;
            }

            break;
        }

        case QEvent::TouchBegin:
            if (te->touchPoints().size() == 1) {
                g->m_startPos = te->touchPoints().first().scenePos().toPoint();
                g->setHotSpot(g->m_startPos);
                timer.start();
                result = QGestureRecognizer::TriggerGesture;
            }

            break;

        case QEvent::TouchEnd:
            result = QGestureRecognizer::CancelGesture;

            if (te->touchPoints().size() == 1 && timer.elapsed() > 0) {
                QTouchEvent::TouchPoint tp = te->touchPoints().first();

                if (tp.startScenePos() != tp.scenePos()) {
                    QPointF vel = (tp.scenePos() - tp.startScenePos()) / (qreal)timer.elapsed();
                    qreal delta = vel.manhattanLength();

                    if (delta >= kFlickTriggerThresholdMin &&
                        delta <= kFlickTriggerThresholdMax) {
                        g->m_endPos = tp.scenePos().toPoint();
                        g->m_velocity = vel.toPoint() * kFlickVelocityMultiplier;
                        result = QGestureRecognizer::FinishGesture;
                    }
                }
            }

            break;

        default:
            break;
    }

    return result;
}

void FlickGestureRecognizer::reset (QGesture* state)
{
    FlickGesture *gesture = static_cast<FlickGesture *>(state);
    gesture->m_velocity = QPoint(0,0);
    gesture->m_endPos = QPoint(0,0);
    gesture->m_startPos = QPoint(0,0);
    QGestureRecognizer::reset(state);
}
