/* @@@LICENSE
*
*      Copyright (c) 2010-2013 Hewlett-Packard Development Company, L.P.
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
#include <QWidget>
#include <QTouchEvent>
#include <QGesture>
#include "WebosTapGestureRecognizer.h"
#include "FlickGesture.h"
#include "ScreenEdgeFlickGesture.h"


QGesture *WebosTapGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    return new QTapGesture;
}

QGestureRecognizer::Result WebosTapGestureRecognizer::recognize(QGesture *state,
                                                            QObject *,
                                                            QEvent *event)
{
    QTapGesture *q = static_cast<QTapGesture *>(state);
    const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);

    QGestureRecognizer::Result result = QGestureRecognizer::CancelGesture;

    switch (event->type()) {
        case QEvent::TouchBegin: {
            QTouchEvent::TouchPoint p = ev->touchPoints().at(0);
            q->setPosition(p.pos());
            q->setHotSpot(p.screenPos());
            result = QGestureRecognizer::TriggerGesture;
            break;
        }
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd: {
            if (q->state() != Qt::NoGesture && ev->touchPoints().size() == 1) {
                QTouchEvent::TouchPoint p = ev->touchPoints().at(0);
                q->setHotSpot(p.screenPos());
                QPoint delta = p.pos().toPoint() - p.startPos().toPoint();
                enum { TapRadius = 40 };
                if (delta.manhattanLength() <= TapRadius) {
                    if (event->type() == QEvent::TouchEnd)
                        result = QGestureRecognizer::FinishGesture;
                    else
                        result = QGestureRecognizer::TriggerGesture;
                }
            }
            break;
        }
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease:
            result = QGestureRecognizer::Ignore;
            break;

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
        case QEvent::Gesture:
        {
            QGesture* g = static_cast<QGestureEvent*>(event)->gesture(Qt::SysMgrGestureFlick);
            if (g && g->state() == Qt::GestureFinished && q->state() != Qt::NoGesture ) {
                result = QGestureRecognizer::CancelGesture;
                break;
            }
            g = static_cast<QGestureEvent*>(event)->gesture(Qt::SysMgrGestureScreenEdgeFlick);
            if (g && g->state() == Qt::GestureFinished && q->state() != Qt::NoGesture ) {
                result = QGestureRecognizer::CancelGesture;
                break;
            }
        }
#else
        case QEvent::Gesture:
        {
            QGesture* g = static_cast<QGestureEvent*>(event)->gesture(FlickGesture::gestureType());
            if (g && g->state() == Qt::GestureFinished && q->state() != Qt::NoGesture ) {
                result = QGestureRecognizer::CancelGesture;
                break;
            }
            g = static_cast<QGestureEvent*>(event)->gesture(ScreenEdgeFlickGesture::gestureType());
            if (g && g->state() == Qt::GestureFinished && q->state() != Qt::NoGesture ) {
                result = QGestureRecognizer::CancelGesture;
                break;
            }
        }
#endif
        default:
            result = QGestureRecognizer::Ignore;
            break;
    }
    return result;
}

void WebosTapGestureRecognizer::reset(QGesture *state)
{
    QTapGesture *q = static_cast<QTapGesture *>(state);
    q->setPosition(QPointF());
    QGestureRecognizer::reset(state);
}

