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




#ifndef SINGLECLICKGESTURE_H
#define SINGLECLICKGESTURE_H

#include "Common.h"

#include <QGesture>
#include <QPoint>

#include <SysMgrDefs.h>
#include "Settings.h"

class SingleClickGesture : public QGesture
{
    Q_OBJECT

public:
	SingleClickGesture(QObject* parent = 0)
	    : QGesture(parent, (Qt::GestureType) SysMgrGestureSingleClick)
	    , m_timerId(0), m_triggerSingleClickOnRelease (false), m_mouseDown (false)
	    , m_modifiers (0)
            {
	        m_timerValue = Settings::LunaSettings()->tapDoubleClickDuration;
		if (m_timerValue <= 0)
		    m_timerValue = 300;
	    }

	virtual ~SingleClickGesture() {
	    stopSingleClickTimer();
	}

	Qt::KeyboardModifiers modifiers() const { return m_modifiers; }

private:
	int m_timerId;
	int m_timerValue;
	QPointF m_penDownPos;
	bool m_mouseDown;
	bool m_triggerSingleClickOnRelease;
	Qt::KeyboardModifiers m_modifiers;

	void startSingleClickTimer() {
	    m_timerId = startTimer(Settings::LunaSettings()->tapDoubleClickDuration);
	}

	void stopSingleClickTimer() {
	    if (m_timerId) {
		killTimer(m_timerId);
		m_timerId = 0;
	    }
	}
private:
	friend class SingleClickGestureRecognizer;

};

#endif /* SINGLECLICKGESTURE_H */
