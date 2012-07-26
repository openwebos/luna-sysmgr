/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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

#include <QEvent>
#include <QTouchEvent>

#include "EventThrottler.h"
#include "Settings.h"
#include "Time.h"

static EventThrottler* s_instance = 0;

	EventThrottler::EventThrottler()
	: m_penTracking (false)
	, m_lastPostedPenMoveTime(0)
	, m_lastPostedPenDownTime(0)
	, m_lastPostedPenMoveX (-1)
	, m_lastPostedPenMoveY (-1)
	, m_lastPostedGestureChangeX(-1)
	, m_lastPostedGestureChangeY(-1)
	, m_lastPostedGestureChangeTime (0)
    , m_touchTracking(false)
    , m_lastPostedTouchDownTime(0)
	, m_lastPostedTouchUpdateTime(0)
{
}

EventThrottler::~EventThrottler()
{
}

EventThrottler* EventThrottler::instance()
{
	if (s_instance)
		return s_instance;
	s_instance = new EventThrottler();
	return s_instance;
}


bool EventThrottler::shouldDropEvent(Event* event)
{
	if(event->type == Event::PenDown || event->type == Event::PenUp) {
		updatePenLocation (event);
	}
	else if(event->type == Event::PenMove) {
		return shouldDropMove(event);
	}
	if(event->type == Event::GestureChange) {
		return shouldDropGestureChange(event);
	} 
	return false;
}

// Update pen location
void EventThrottler::updatePenLocation (Event* event)
{
	if (event->type == Event::PenDown) {
		m_lastPostedPenMoveX = event->x;
		m_lastPostedPenMoveY = event->y;
		m_lastPostedPenDownTime = event->time;
		m_lastPostedPenMoveTime = event->time;
		m_penTracking = true;
	}
	if (event->type == Event::PenUp) {

		if (m_penTracking) {
			event->x = m_lastPostedPenMoveX;
			event->y = m_lastPostedPenMoveY;
		}
		
		m_lastPostedPenMoveX = 0;
		m_lastPostedPenMoveY = 0;
		m_lastPostedPenDownTime = 0;
		m_lastPostedPenMoveTime = 0;
		m_penTracking = false;
	}
}

void EventThrottler::updateTouchLocation(QTouchEvent* event)
{
    if (event->touchPointStates() & Qt::TouchPointPressed) {
        // only filter by tap radius when the user is interacting with 1 finger
        m_touchTracking = event->touchPoints().length() == 1;
        m_lastPostedTouchUpdateTime = Time::curTimeMs();
        m_lastPostedTouchDownTime = m_lastPostedTouchUpdateTime;
    }
    else if (event->touchPointStates() & Qt::TouchPointReleased) {
        m_touchTracking = false;
        m_lastPostedTouchUpdateTime = 0;
        m_lastPostedTouchDownTime = 0;
    }
}

// Doesn't actually drop the event. It just tells you if the load is high and
// we should drop the event (we may not want to drop certain events; 
// i.e., to guarantee that we always get a move before a flick)
bool EventThrottler::shouldDropMove(Event* event)
{
	if( event->type != Event::PenMove )
		return false;

	int delta_ms = event->time - m_lastPostedPenMoveTime;
	int hz = 0;

	if (delta_ms) {
		hz = ( 1000 ) / delta_ms;
	}

	if (m_penTracking) {
		Settings* ss = Settings::LunaSettings();
		int touch_ms = event->time - m_lastPostedPenDownTime;
		int moveMin = ss->tapRadius;
		// heuristic to have the min distance drop by 10% every 100ms since start of touch
		if (touch_ms > ss->tapRadiusShrinkGranMs) {
			moveMin -= (moveMin * ss->tapRadiusShrinkPercent * touch_ms) /
				(ss->tapRadiusShrinkGranMs * 100);
			if (moveMin < ss->tapRadiusMin)
				moveMin = ss->tapRadiusMin;
		}

		int deltaX = m_lastPostedPenMoveX - event->x;
		int deltaY = m_lastPostedPenMoveY - event->y;
		if ((deltaX * deltaX + deltaY * deltaY) < (moveMin * moveMin))
		{
			return true; // we should drop this event
		}

		m_penTracking = false;
	}
	else {
		// Drop move event if there was no movement
		if (ABS(m_lastPostedPenMoveX - event->x) == 0 &&
			ABS(m_lastPostedPenMoveY - event->y) == 0) {
			// No movement. drop the event
			return true;
		}

		if (hz > Settings::LunaSettings()->maxPenMoveFreq) {
			return true; // we should drop this event
		}
	}

	//printf("  move : %d,%d\n", event->x, event->y );

	m_lastPostedPenMoveX = event->x;
	m_lastPostedPenMoveY = event->y;
	m_lastPostedPenMoveTime = event->time;

	return false;
}

bool EventThrottler::shouldDropGestureChange(Event* event)
{
	int hz=0;

	int delta_ms = (event->time - m_lastPostedGestureChangeTime);
	if( delta_ms ) {
		hz = ( 1000 ) / delta_ms;
	}

	Settings* s = Settings::LunaSettings();
	if ((hz > s->maxGestureChangeFreq)
		|| (ABS(m_lastPostedGestureChangeX - event->x) < s->tapRadius 
			&& ABS(m_lastPostedGestureChangeY - event->y) < s->tapRadius)) {
		return true; // we should drop this event
	}

	m_lastPostedGestureChangeX = event->x;
	m_lastPostedGestureChangeY = event->y;
	m_lastPostedGestureChangeTime = event->time;

	return false;
}

bool EventThrottler::shouldDropEvent(QEvent* e)
{
	switch (e->type()) {
    case QEvent::TouchBegin:
	case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
		return shouldDropTouchEvent(static_cast<QTouchEvent*>(e));
	default:
		break;
	}

	return false;    
}

bool EventThrottler::shouldDropTouchEvent(QTouchEvent* e)
{
	// Only TouchUpdate events come in here

	// If all fingers are stationary, we can drop this event
	if (e->touchPointStates() == Qt::TouchPointStationary) {
		return true;
	}
	
	// If we have a touch press or release, we cannot drop this event
	if (e->touchPointStates() & Qt::TouchPointPressed ||
		e->touchPointStates() & Qt::TouchPointReleased) {
        updateTouchLocation(e);
		return false;
    }
   
    uint32_t now = Time::curTimeMs();
    Settings* s = Settings::LunaSettings();
    const QList<QTouchEvent::TouchPoint>& pts = e->touchPoints();

    if (m_touchTracking) {

        if (pts.length() == 1) {

            int touch_ms = Time::curTimeMs() - m_lastPostedTouchDownTime;
            int moveMin = s->tapRadius;
            QPointF delta = pts[0].startPos() - pts[0].pos();

            if (touch_ms > s->tapRadiusShrinkGranMs) {
                moveMin -= (moveMin * s->tapRadiusShrinkPercent * touch_ms) / (s->tapRadiusShrinkGranMs * 100);
                if (moveMin < s->tapRadiusMin)
                    moveMin = s->tapRadiusMin;
            }

            if ((delta.x() * delta.x() + delta.y() * delta.y()) < (moveMin * moveMin)) {
                return true;
            }
        }

        m_touchTracking = false;
    }
    else {
        int hz = 0;
        int deltaMs = now - m_lastPostedTouchUpdateTime;
        if (deltaMs > 0)
            hz = 1000/deltaMs;

        if (hz > s->maxTouchChangeFreq) {
            return true;
        }
    }

	m_lastPostedTouchUpdateTime = now;

	return false;
}
