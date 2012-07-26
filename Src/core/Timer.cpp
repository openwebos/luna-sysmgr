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

#include "SingletonTimer.h"
#include "Timer.h"

TimerBase::TimerBase(SingletonTimer* masterTimer)
	: m_master(masterTimer)
	, m_handle(0)
	, m_singleShot(true)
	, m_interval(0)
{
}

TimerBase::~TimerBase()
{
    stop();
}

void TimerBase::start(guint intervalInMs, bool singleShot)
{
	stop();

	m_interval = intervalInMs;
	m_singleShot = singleShot;
	
	m_handle = m_master->create(callback, this);	
	m_master->fire(m_handle, SingletonTimer::currentTime() + m_interval);
}

void TimerBase::stop()
{
    if (m_handle) {
		m_master->deref(m_handle);
		m_handle = 0;
	}
}

void TimerBase::callback(void* arg)
{
/*	
	TimerBase* t = (TimerBase*) arg;

	t->stop();

	bool ret = t->timeout();
	if (!ret || t->m_singleShot) {
		return;
	}

	// periodic timer. restart
	t->start(t->m_interval, false);
*/

	TimerBase* t = (TimerBase*) arg;

	SingletonTimer* master = t->m_master;
	TimerHandle* handle = t->m_handle;

	master->ref(handle);
	
	bool ret = t->timeout();
	// stop if:
	// a. this is a singleshot timer
	// b. callback returned false
	// 
	// but if it stopped and restarted the timer
	// (which will cause the handle to change), we need
	// to continue
	if (!ret || t->m_singleShot) {

		if (!t->m_handle || (t->m_handle == handle)) {
			master->deref(handle);
			t->stop();
			return;
		}
	}

	// periodic timer. reschedule	
	master->deref(handle);

	// Need to check if the handle is valid here because
	// someone could have called stop in the callback but still
	// returned true from the callback
	if (t->m_handle) {
		master->fire(t->m_handle, SingletonTimer::currentTime() + t->m_interval);
	}
}

bool TimerBase::running() const
{
	return m_handle != 0;    
}
