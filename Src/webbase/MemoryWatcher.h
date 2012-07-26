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




#ifndef MEMORYWATCHER_H
#define MEMORYWATCHER_H

#include "Common.h"

#include <stdint.h>
#include <map>

#include "SignalSlot.h"
#include "Timer.h"

#if defined(TARGET_DEVICE)
extern "C" {
#include <memchute.h>
}
#endif

class MemoryWatcher
{
public:

	enum MemState {
		Normal = 0,
		Medium,
		Low,
		Critical
	};

	static MemoryWatcher* instance();

	void start();
	
	MemState state() const { return m_state; }

	void doLowMemActions(bool allowExpensive=false);
	
	bool allowNewWebAppLaunch();

	static void dropBufferCaches();

private:

	MemoryWatcher();
	~MemoryWatcher();

	bool timerTicked();
	int getCurrentRssUsage() const;

#if defined(TARGET_DEVICE)    
    static void memchuteCallback(MemchuteThreshold threshold);
#endif

public:

	Signal<MemState> signalMemoryStateChanged;
	
private:

	Timer<MemoryWatcher> m_timer;
	int m_currRssUsage;

	int m_timeAtLastLowMemAction;
	int m_timeAtLastExpensiveLowMemAction;

	static const int kFileNameLen = 128;
	char m_fileName[kFileNameLen];

	MemState m_state;
	MemState m_lastNotifiedState;
	

#if defined(TARGET_DEVICE)
	int m_memchuteCriticalThreshold;
	MemchuteWatcher* m_memWatch;
#endif	

};

#endif /* MEMORYWATCHER_H */

