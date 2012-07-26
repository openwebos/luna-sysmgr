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

#include "TaskBase.h"

TaskBase::TaskBase()
	: m_mainCtxt(0)
	, m_mainLoop(0)
	, m_asyncCaller(0)
	, m_masterTimer(0)
{    
}

TaskBase::~TaskBase()
{
    delete m_asyncCaller;
}

void TaskBase::postEvent(sptr<Event> event, bool highPriority)
{
	m_eventsMutex.lock();

	// FIXME: This is not right. the high priority events should be maintained in a separate queue
	if (G_UNLIKELY(highPriority))
		m_eventsList.push_front(event);
	else
		m_eventsList.push_back(event);

	if (G_UNLIKELY(m_asyncCaller == 0))
		m_asyncCaller = new AsyncCaller<TaskBase>(this, &TaskBase::eventCallback, m_mainLoop);
	
	m_eventsMutex.unlock();

	m_asyncCaller->call();
}

void TaskBase::eventCallback()
{
	while (true) {

		m_eventsMutex.lock();

		if (m_eventsList.empty()) {
			m_eventsMutex.unlock();
			break;
		}

		sptr<Event> e = m_eventsList.front();
		m_eventsList.pop_front();

		// unlock mutex before calling the callback,
		// otherwise we may have a deadlock
		m_eventsMutex.unlock();
		
		handleEvent(e);
	}
}
