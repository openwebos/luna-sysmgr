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




#ifndef __EventThrottlerIme_h__
#define __EventThrottlerIme_h__

#include "Common.h"

#include "Event.h"

class EventThrottlerIme
{
public:
	EventThrottlerIme();
	~EventThrottlerIme();

	static EventThrottlerIme* instance();

	bool shouldDropEvent(Event* e);

private:
	int m_curPenMoveFreq;
	int m_curGestureMoveFreq;
	bool m_penTracking;

	uint32_t m_lastPostedPenMoveTime;
	uint32_t m_lastPostedPenDownTime;
	int m_lastPostedPenMoveX;
	int m_lastPostedPenMoveY;

	int m_lastPostedGestureChangeX;
	int m_lastPostedGestureChangeY;
	uint32_t m_lastPostedGestureChangeTime;

	void updatePenLocation(Event* e);
	bool shouldDropMove(Event* e);
	bool shouldDropGestureChange(Event* e);
};

#endif  // __EventThrottlerIme_h__

