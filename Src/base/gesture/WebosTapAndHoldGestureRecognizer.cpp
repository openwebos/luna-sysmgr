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

#include "WebosTapAndHoldGesture.h"
#include "WebosTapAndHoldGestureRecognizer.h"

Qt::GestureType WebosTapAndHoldGesture::type;

QGesture *WebosTapAndHoldGestureRecognizer::create(QObject *target)
{
    if (target && target->isWidgetType()) {
        static_cast<QWidget *>(target)->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    return new WebosTapAndHoldGesture;
}

QGestureRecognizer::Result
WebosTapAndHoldGestureRecognizer::recognize(QGesture *state, QObject *object,
                                        QEvent *event)
{
    QGestureRecognizer::Result result = QGestureRecognizer::CancelGesture;
    WebosTapAndHoldGesture *q = static_cast<WebosTapAndHoldGesture *>(state);

    if (object == state && event->type() == QEvent::Timer) {
        q->stopTapTimer();
        if (q->state() != Qt::NoGesture && q->state() != Qt::GestureCanceled) {
            result = QGestureRecognizer::FinishGesture;
        }
        return result | QGestureRecognizer::ConsumeEventHint;
    }

    const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);

    //Todo Here use a const value to set up the timer interval but maybe need to use the Timeout value
    //enum { TimerInterval = 700 };
    enum { TapRadius = 40 };
    switch (event->type()) {
    case QEvent::TouchBegin:
        q->setPosition(ev->touchPoints().at(0).startScreenPos());
        q->setHotSpot(q->position());
        q->stopTapTimer();
        q->startTapTimer();
        return QGestureRecognizer::TriggerGesture;
    case QEvent::TouchEnd:
        result = QGestureRecognizer::CancelGesture;
        q->stopTapTimer();
        break;
    case QEvent::TouchUpdate:
        if (q->tapTimerId() && ev->touchPoints().size() == 1) {
            QTouchEvent::TouchPoint p = ev->touchPoints().at(0);
            QPoint delta = p.pos().toPoint() - p.startPos().toPoint();
            if (delta.manhattanLength() > TapRadius) {
                result = QGestureRecognizer::CancelGesture;
                q->stopTapTimer();
            } else {
                result = QGestureRecognizer::Ignore;
            }
        } else if (ev->touchPoints().size() > 1) {
            result = QGestureRecognizer::CancelGesture;
            q->stopTapTimer();
        } else {
            result = QGestureRecognizer::Ignore;
        }
        break;
    default:
        return QGestureRecognizer::Ignore;
    }
    return result;
}

void WebosTapAndHoldGestureRecognizer::reset(QGesture *state)
{
    WebosTapAndHoldGesture *q = static_cast< WebosTapAndHoldGesture *>(state);

    q->setPosition(QPointF());
    q->stopTapTimer();

    QGestureRecognizer::reset(state);
}
