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

#include "EventThrottlerIme.h"
#include "Settings.h"

static EventThrottlerIme* s_instanceIme = 0;

EventThrottlerIme::EventThrottlerIme()
	: m_lastPostedPenMoveTime(0)
	, m_lastPostedPenDownTime(0)
	, m_lastPostedPenMoveX (-1)
	, m_lastPostedPenMoveY (-1)
	, m_lastPostedGestureChangeX(-1)
	, m_lastPostedGestureChangeY(-1)
	, m_lastPostedGestureChangeTime (0)
	  , m_penTracking (false)
{
	m_curPenMoveFreq = 60; // Hz
	m_curGestureMoveFreq = Settings::LunaSettings()->maxGestureChangeFreq; // Hz
}

EventThrottlerIme::~EventThrottlerIme()
{
}

EventThrottlerIme* EventThrottlerIme::instance()
{
	if (s_instanceIme)
		return s_instanceIme;
	s_instanceIme = new EventThrottlerIme();
	return s_instanceIme;
}


bool EventThrottlerIme::shouldDropEvent(Event* event)
{
#if defined(TARGET_DEVICE)
	if(event->type == Event::PenDown || event->type == Event::PenUp) {
		updatePenLocation (event);
	}
	else if(event->type == Event::PenMove) {
		return shouldDropMove(event);
	}
	if(event->type == Event::GestureChange) {
		return shouldDropGestureChange(event);
	}
#endif
	return false;
}

// Update pen location
void EventThrottlerIme::updatePenLocation (Event* event)
{
	if (event->type == Event::PenDown) {
		m_lastPostedPenMoveX = event->x;
		m_lastPostedPenMoveY = event->y;
		m_lastPostedPenDownTime = event->time;
		m_lastPostedPenMoveTime = event->time;
		m_penTracking = true;
	}
	if (event->type == Event::PenUp) {
		m_lastPostedPenMoveX = 0;
		m_lastPostedPenMoveY = 0;
		m_lastPostedPenDownTime = 0;
		m_lastPostedPenMoveTime = 0;
		m_penTracking = false;
	}
}

// Doesn't actually drop the event. It just tells you if the load is high and
// we should drop the event (we may not want to drop certain events;
// i.e., to guarantee that we always get a move before a flick)
bool EventThrottlerIme::shouldDropMove(Event* event)
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
		int moveMin = 10;
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

		if (hz > m_curPenMoveFreq) {
			// printf ("ignore : %d,%d hz %d max %d cur %d\n", event->x, event->y, hz,
			//		Settings::LunaSettings()->maxPenMoveFreq, m_curPenMoveFreq );
			return true; // we should drop this event
		}
	}

	//printf("  move : %d,%d\n", event->x, event->y );

	m_lastPostedPenMoveX = event->x;
	m_lastPostedPenMoveY = event->y;
	m_lastPostedPenMoveTime = event->time;

	return false;
}

bool EventThrottlerIme::shouldDropGestureChange(Event* event)
{
	int hz=0;

	m_curGestureMoveFreq  = Settings::LunaSettings()->maxGestureChangeFreq;

	int delta_ms = (event->time - m_lastPostedGestureChangeTime);
	if( delta_ms ) {
		hz = ( 1000 ) / delta_ms;
	}

	if ((hz > Settings::LunaSettings()->maxGestureChangeFreq)
			|| (ABS(m_lastPostedGestureChangeX - event->x) < Settings::LunaSettings()->tapRadius
				&& ABS(m_lastPostedGestureChangeY - event->y) < Settings::LunaSettings()->tapRadius)) {
		return true; // we should drop this event
	}

	m_lastPostedGestureChangeX = event->x;
	m_lastPostedGestureChangeY = event->y;
	m_lastPostedGestureChangeTime = event->time;

	return false;
}
