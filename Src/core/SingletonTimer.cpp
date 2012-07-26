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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "Logging.h"
#include "SingletonTimer.h"

//#define USE_SYS_MALLOC 1

#ifdef USE_SYS_MALLOC
#define New(x)       (x*) malloc(sizeof(x))
#define New0(x)      (x*) calloc(1, sizeof(x))
#define Free(type,x) free(x)
#else
#define New(x)       g_slice_new(x)
#define New0(x)      g_slice_new0(x)
#define Free(type,x) g_slice_free(type,x)
#endif

struct TimerHandle
{
    int64_t             fireTime;
    TimerCallback       callback;
    void*               userArg;
	int                 refCount;
	bool                inActiveList;
};

struct TimerSource
{
	GSource            source;
	SingletonTimer*    parent;
};

static GSourceFuncs sTimerFuncs =
{
	SingletonTimer::timerPrepare,
	SingletonTimer::timerCheck,
	SingletonTimer::timerDispatch,
    NULL,
    NULL,
    NULL
};


SingletonTimer::SingletonTimer(GMainLoop* loop)
{
    m_loop   = loop;

	m_source = (TimerSource*) g_source_new(&sTimerFuncs, sizeof(TimerSource));
    g_source_set_priority(&(m_source->source), G_PRIORITY_DEFAULT);
    g_source_set_can_recurse(&(m_source->source), TRUE);
    g_source_attach(&(m_source->source), g_main_loop_get_context(m_loop));

	m_source->parent = this;
	
    m_activeList = 0;
}

SingletonTimer::~SingletonTimer()
{
    // FIXME: NO-OP
}

uint64_t SingletonTimer::currentTime()
{
    struct timespec currTime;
    clock_gettime(CLOCK_MONOTONIC, &currTime);

	return ((uint64_t) currTime.tv_sec * 1000 +
			(uint64_t) currTime.tv_nsec / 1000000);    
}

TimerHandle* SingletonTimer::create(TimerCallback callback, void* userArg)
{
    TimerHandle* timer = New0(TimerHandle);
    timer->callback = callback;
    timer->userArg  = userArg;
	timer->refCount = 1;
	timer->inActiveList = false;

    return timer;    
}

void SingletonTimer::fire(TimerHandle* timer, uint64_t timeInMs)
{
    if (!timer)
        return;

    timer->fireTime = timeInMs > 0 ? timeInMs : 0;

	if (timer->inActiveList) {
		m_activeList = g_list_remove(m_activeList, timer);
		timer->inActiveList = false;
	}

	// re-insert sorted based on
	// 1. ascending based on firetime
	// 2. firetime being equal, order of this call.
	if (!m_activeList) {
		m_activeList = g_list_append(m_activeList, timer);
	}
	else {

		bool inserted = false;
		for (GList* iter = g_list_first(m_activeList); iter;
			 iter = g_list_next(iter)) {

			TimerHandle* t = (TimerHandle*) iter->data;
			if (t->fireTime > timer->fireTime) {
				m_activeList = g_list_insert_before(m_activeList, iter, timer);
				inserted = true;
				break;
			}
		}

		if (!inserted)
			m_activeList = g_list_append(m_activeList, timer);
	}

	timer->inActiveList = true;

    // wake up main loop if it is suspended in a poll
    g_main_context_wakeup(g_main_loop_get_context(m_loop));    
}

void SingletonTimer::ref(TimerHandle* timer)
{
	timer->refCount++;
}

void SingletonTimer::deref(TimerHandle* timer)
{
	timer->refCount--;
	if (timer->refCount <= 0)
		destroy(timer);
}

void SingletonTimer::destroy(TimerHandle* timer)
{
    if (!timer)
        return;

	if (timer->inActiveList)
		m_activeList = g_list_remove(m_activeList, timer);
    Free(TimerHandle, timer);    
}

static TimerHandle* PrvFindMinTimer(GList* list)
{
	if (!list || g_list_length(list) == 0)
        return 0;

	GList* iter = g_list_first(list);
	return (TimerHandle*) iter->data;
}	

gboolean SingletonTimer::timerPrepare(GSource* source, gint* timeout)
{
	SingletonTimer* st = ((TimerSource*)(source))->parent;
	
	TimerHandle* minTimer = PrvFindMinTimer(st->m_activeList);
	if (!minTimer) {
		*timeout = -1;
		return FALSE;
	}
	
    struct timespec currTime;
    clock_gettime(CLOCK_MONOTONIC, &currTime);

    int64_t diff = minTimer->fireTime -
                   (int64_t) currTime.tv_sec * 1000 -
                   (int64_t) currTime.tv_nsec / 1000000;
    if (diff <= 0) {
        return TRUE;
    }

    *timeout  = (gint) (diff);
    return FALSE;    
}

gboolean SingletonTimer::timerCheck(GSource* source)
{
	SingletonTimer* st = ((TimerSource*)(source))->parent;

	TimerHandle* minTimer = PrvFindMinTimer(st->m_activeList);
	if (!minTimer)
		return FALSE;

    struct timespec currTime;
    clock_gettime(CLOCK_MONOTONIC, &currTime);

    int64_t diff = minTimer->fireTime -
                   (int64_t) currTime.tv_sec * 1000 -
                   (int64_t) currTime.tv_nsec / 1000000;
    return (diff <= 0);    
}

gboolean SingletonTimer::timerDispatch(GSource* source, GSourceFunc callback, gpointer userData)
{
	SingletonTimer* st = ((TimerSource*)(source))->parent;

	TimerHandle* minTimer = PrvFindMinTimer(st->m_activeList);
	if (!minTimer)
		return TRUE;

	// Currently in active list. Remove from there
	st->m_activeList = g_list_remove(st->m_activeList, minTimer);
	minTimer->inActiveList = false;
	
    if (minTimer->callback) {
        minTimer->callback(minTimer->userArg);
    }

    return TRUE;
}
