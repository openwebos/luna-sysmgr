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




#ifndef TASKBASE_H
#define TASKBASE_H

#include "Common.h"

#include <glib.h>
#include <list>

#include "sptr.h"
#include "AsyncCaller.h"
#include "Event.h"
#include "Mutex.h"
#include "SingletonTimer.h"

class TaskBase : public RefCounted
{
public:

	TaskBase();
	virtual ~TaskBase();

	virtual void run() = 0;

	virtual void quit() = 0;

	void postEvent(sptr<Event> event, bool highPriority=false);
	
	GMainLoop* mainLoop() const { return m_mainLoop; }

    SingletonTimer* masterTimer() const { return m_masterTimer; }
	
protected:

	virtual void handleEvent(sptr<Event> event) = 0;
	
	GMainContext* m_mainCtxt;
	GMainLoop* m_mainLoop;
	Mutex m_mutex;
	Mutex m_eventsMutex;
	std::list<sptr<Event> > m_eventsList;
	AsyncCaller<TaskBase>* m_asyncCaller;
	SingletonTimer* m_masterTimer;

private:

	void eventCallback();
};

#endif /* TASKBASE_H */
