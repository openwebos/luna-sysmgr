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




#ifndef EVENT_H
#define EVENT_H

#include "Common.h"

#include <stdint.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <qnamespace.h>

#include <SysMgrEvent.h>

#include "sptr.h"

class Event : public SysMgrEvent,
			  public RefCounted			  
{
public:

	Event() {
		// The following is needed so that we can initialize all
		// of the member variables to zero. if Event is created on
		// the stack, the operator new will not get called
		// 
		// SysmgEvent is guaranteed to
		// be a data only, no virtual functions class
		SysMgrEvent* base = static_cast<SysMgrEvent*>(this);
		::memset(base, 0, sizeof(SysMgrEvent));
	}

	void* operator new(size_t size) {
		Event* e = (Event*) g_slice_alloc0(size);
		e->m_blockSize = size;
		return e;
	}

	void operator delete(void* p) {
		Event* e = (Event*) p;
		g_slice_free1(e->m_blockSize, p);
	}

	static inline bool isPenEvent(Event* e) {
		return (e->type & PenMask);
	}

	static inline bool isKeyEvent(Event* e) {
		return (e->type & KeyMask);
	}

	static inline bool isGestureEvent(Event* e) {
		return (e->type & GestureMask);
	}

	static inline bool isPenOffEvent(Event* e) {
		return ((e->type & PenMask) &&
				((e->type == PenUp) ||
				 (e->type == PenCancel) ||
				 (e->type == PenCancelAll)));
	}

	inline void setClicked(bool val) {
		if (val)
			attributes |= Clicked;
		else if (attributes & Clicked)
			attributes ^= Clicked;
	}

	inline bool clicked() const {
		return (attributes & Clicked);
	}

	inline void setRejected(bool val) {
		if (val)
			attributes |= Rejected;
		else if (attributes & Rejected)
			attributes ^= Rejected;
	}
	
	inline bool rejected() const {
		return (attributes & Rejected);
	}

	inline void setMainFinger(bool val) {
		if (val)
			attributes |= MainFinger;
		else if (attributes & MainFinger)
			attributes ^= MainFinger;
	}

	inline bool mainFinger() const {
		return (attributes & MainFinger);
	}

	inline void setGestureKey(bool val) {
		if (val)
			attributes |= GestureKey;
		else if (attributes & GestureKey)
			attributes ^= GestureKey;
	}

	inline bool gestureKey() const {
		return (attributes & GestureKey);
	}

	static uint16_t modifiersFromQt(const Qt::KeyboardModifiers& m) {
		return (m & Qt::ShiftModifier ? Event::Shift : 0) |
			(m & Qt::ControlModifier ? Event::Control : 0) |
			(m & Qt::AltModifier ? Event::Alt : 0) |
			(m & Qt::MetaModifier ? Event::Meta : 0);
	}
	
private:

	Event(const Event&);
	Event& operator=(const Event&);
	int m_blockSize;
};
	

#endif /* EVENT_H */
