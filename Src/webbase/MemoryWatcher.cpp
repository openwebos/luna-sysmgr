/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <strings.h>
#include <malloc.h>

#include "MemoryWatcher.h"

#include "Settings.h"
#include "Time.h"
#include "WebAppManager.h"

static const int kTimerMs = 5000;
static const int kLowMemExpensiveTimeoutMultiplier = 2;
static const uint32_t kMinIntervalBetweenLowMemActions = 30000;
static const uint32_t kMinIntervalBetweenExpensiveLowMemActions = 300000;
static const int kMinIntervalBetweenMediumMemActionsMs = (15*60*1000)/kTimerMs;

MemoryWatcher* MemoryWatcher::instance()
{
	static MemoryWatcher* s_instance = 0;
	if (G_UNLIKELY(s_instance == 0))
		s_instance = new MemoryWatcher;

	return s_instance;
}

MemoryWatcher::MemoryWatcher()
	: m_timer(WebAppManager::instance()->masterTimer(), this, &MemoryWatcher::timerTicked)
	, m_currRssUsage(0)
	, m_state(MemoryWatcher::Normal)
	, m_lastNotifiedState(MemoryWatcher::Normal)
{
	m_fileName[kFileNameLen - 1] = 0;
	snprintf(m_fileName, kFileNameLen - 1, "/proc/%d/statm", getpid());

	char oom_adj[kFileNameLen];
	snprintf(oom_adj, kFileNameLen - 1, "/proc/%d/oom_adj", getpid());
	FILE* f = fopen(oom_adj, "wb");
	if (f) {
		size_t result = fwrite("-17\n", 4, 1, f);
		Q_UNUSED(result);
		fclose(f);	
	}

	m_timeAtLastLowMemAction = 0;
	m_timeAtLastExpensiveLowMemAction = 0;
}

MemoryWatcher::~MemoryWatcher()
{    
}

void MemoryWatcher::start()
{
	if (m_timer.running())
		return;
	
	m_timer.start(kTimerMs);

#if defined(HAS_MEMCHUTE)
	m_memWatch = MemchuteWatcherNew(MemoryWatcher::memchuteCallback);
	if (m_memWatch != NULL) {
		MemchuteGmainAttach(m_memWatch, WebAppManager::instance()->mainLoop());
		MemchuteGmainSetPriority(m_memWatch, G_PRIORITY_HIGH);
	} else {
		g_warning("Failed to create MemchuteWatcher");
	}
#endif	    
}

static const char* nameForState(MemoryWatcher::MemState state)
{
	switch (state) {
	case (MemoryWatcher::Medium):
		return "Medium";
	case (MemoryWatcher::Low):
		return "Low";
	case (MemoryWatcher::Critical):
		return "Critical";
	default:
		break;
	}
	
	return "Normal";
}

bool MemoryWatcher::timerTicked()
{
	if (m_state != m_lastNotifiedState) {
		m_lastNotifiedState = m_state;
		signalMemoryStateChanged.fire(m_lastNotifiedState);
	}
	
	if (m_state == Normal)	{
		return true;
	} else if (m_state == Medium)	{
		static int count = 0;
		if (count++ > kMinIntervalBetweenMediumMemActionsMs) {
			malloc_trim(0);
			count = 0;
		}
		return true;
	}

	m_currRssUsage = getCurrentRssUsage();

	g_warning("WebKit MemoryWatcher: LOW MEMORY: State: %s, current RSS usage: %dMB\n",
			  nameForState(m_state), m_currRssUsage);

	doLowMemActions(true);
	
	return true;    
}

int MemoryWatcher::getCurrentRssUsage() const
{
	FILE* f = fopen(m_fileName, "rb");
	if (!f)
		return m_currRssUsage;

	int totalSize, rssSize;

	int result = fscanf(f, "%d %d", &totalSize, &rssSize);
	Q_UNUSED(result);
	fclose(f);	
	
    return (rssSize * 4096) / (1024 * 1024);
}


bool MemoryWatcher::allowNewWebAppLaunch()
{
	if (m_state >= Low){
		// already in Low or critical memory states, so do not allow new apps to be launched
		return false;
	}
	
	// OK to launch new app with specified memory requirements
	return true;
}

void MemoryWatcher::doLowMemActions(bool allowExpensive)
{
	uint32_t curTime = Time::curTimeMs();
	if (curTime - m_timeAtLastLowMemAction < kMinIntervalBetweenLowMemActions) {
		return; // too soon to perform the Low mem actions again
	}
	
	malloc_trim(0);
	if(allowExpensive) {
		uint32_t timeout = kMinIntervalBetweenExpensiveLowMemActions;
		if (Low == m_state) {
			timeout *= kLowMemExpensiveTimeoutMultiplier;
		}
		
		allowExpensive = ((curTime - m_timeAtLastExpensiveLowMemAction) >= timeout);
	}
	
	g_warning("MemoryWatcher: Running Low memory actions....\n");

//	Palm::WebGlobal::notifyLowMemory();
	m_timeAtLastLowMemAction = Time::curTimeMs();
		
	if (allowExpensive) {
		g_warning("MemoryWatcher: doing expensive low memory actions.... \n");
		WebAppManager::instance()->restartHeadLessBootApps();
		m_timeAtLastExpensiveLowMemAction = m_timeAtLastLowMemAction;
	}

	m_currRssUsage = getCurrentRssUsage();			
    g_warning("MemoryWatcher: RSS usage after low memory actions: %dMB\n", m_currRssUsage);
}

#if defined(HAS_MEMCHUTE)
void MemoryWatcher::memchuteCallback(MemchuteThreshold threshold)
{
	MemoryWatcher* mw = MemoryWatcher::instance();

	int oldState = mw->m_state;	

	switch (threshold) {
	case (MEMCHUTE_NORMAL):
		if (mw->m_state != Normal) {
			// close dashboard and popup
			WebAppManager::instance()->disableAppCaching(false);
		}		
		mw->m_state = Normal;
		break;
	case (MEMCHUTE_MEDIUM):
		WebAppManager::instance()->disableAppCaching(true);
		mw->m_state = Medium;
		malloc_trim(0);
		break;
	case (MEMCHUTE_LOW):
		WebAppManager::instance()->disableAppCaching(true);
		mw->m_state = Low;
		malloc_trim(0);
		break;
	case (MEMCHUTE_CRITICAL):
	case (MEMCHUTE_REBOOT):
		WebAppManager::instance()->disableAppCaching(true);
		mw->m_state = Critical;
		break;
	default:
		break;
	}

	// Transitioning out of Normal state. Drop the buffer caches
	if ((oldState >= Normal) && (mw->m_state > oldState))
		dropBufferCaches();
}

void MemoryWatcher::dropBufferCaches()
{
	g_message("MemoryWatcher: Out of Normal state. Dropping buffer cache");
	::system("echo 1 > /proc/sys/vm/drop_caches");
}

#endif
