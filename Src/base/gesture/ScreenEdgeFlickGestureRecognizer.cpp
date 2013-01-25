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

#include "ScreenEdgeFlickGesture.h"
#include "ScreenEdgeFlickGestureRecognizer.h"

#include "Time.h"
#include "HostBase.h"
#include "FlickEvent.h"
#include "ScreenEdgeFlickGesture.h"

Qt::GestureType ScreenEdgeFlickGesture::type = Qt::CustomGesture;


QGesture* ScreenEdgeFlickGestureRecognizer::create (QObject* target)
{
    return new ScreenEdgeFlickGesture;
}


QGestureRecognizer::Result ScreenEdgeFlickGestureRecognizer::recognize(QGesture* gesture, QObject* watched, QEvent* event)
{
    ScreenEdgeFlickGesture* g = static_cast<ScreenEdgeFlickGesture*>(gesture);
    QGestureRecognizer::Result result = QGestureRecognizer::Ignore;

    switch (event->type()) {
    case SysMgrGestureScreenEdgeFlick: {
        ScreenEdgeFlickEvent* ev = static_cast<ScreenEdgeFlickEvent*>(event);
        if (ev->state() == Qt::GestureStarted) {
            g->m_edge = (ScreenEdgeFlickGesture::Edge)ev->edge();
            g->m_yDistance = ev->yDistance();
            result = QGestureRecognizer::TriggerGesture;
        } else if (ev->state() == Qt::GestureFinished) {
            g->m_edge = (ScreenEdgeFlickGesture::Edge)ev->edge();
            g->m_yDistance = ev->yDistance();
            result = QGestureRecognizer::FinishGesture;
        }
    }
        break;
    default:
        break;
    }
    return result;

}

void ScreenEdgeFlickGestureRecognizer::reset (QGesture* state)
{
    ScreenEdgeFlickGesture *gesture = static_cast<ScreenEdgeFlickGesture *>(state);
    gesture->m_edge = ScreenEdgeFlickGesture::EdgeUnknown;
    gesture->m_yDistance = 0;
    QGestureRecognizer::reset(state);
}
