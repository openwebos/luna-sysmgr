/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#include "timedelaytransition.h"
#include <QStateMachine>

////public:

TimeDelayTransition::TimeDelayTransition(const quint32 delayMs)
: m_currentDelayMs(delayMs)
{
	m_timer.setSingleShot(true);
	m_timer.setInterval(delayMs);
	QSignalTransition::setSenderObject(&m_timer);
	QSignalTransition::setSignal(SIGNAL(timeout()));
}

//virtual
TimeDelayTransition::~TimeDelayTransition()
{
}

//virtual
void TimeDelayTransition::setDelayMs(const quint32 delayMs)
{
	m_currentDelayMs = delayMs;
	m_timer.setInterval(delayMs);
}

////public Q_SLOTS:

//virtual
void TimeDelayTransition::slotRestartTimer()
{
	if (m_timer.isActive())
	{
		m_timer.stop();
	}
	m_timer.start(m_currentDelayMs);
}

//virtual
void TimeDelayTransition::slotAbort()
{
	m_timer.stop();
}

///protected:

//virtual
bool TimeDelayTransition::eventTest(QEvent *e)
{
	if (!QSignalTransition::eventTest(e))
		return false;
	QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(e);
	return se->sender() == (QObject *)&m_timer;
}
