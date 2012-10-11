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




#ifndef MEMORYMONITOR_H
#define MEMORYMONITOR_H

#include "Common.h"

#include <stdint.h>
#include <map>
#include <QObject>

#include "Timer.h"
#include "Mutex.h"

#if defined(HAS_MEMCHUTE)
extern "C" {
#include <memchute.h>
}
#endif

class MemoryMonitor : public QObject
{
	Q_OBJECT

public:

	enum MemState {
		Normal = 0,
		Medium,
		Low,
		Critical
	};

	static MemoryMonitor* instance();

	void start();
	
	MemState state() const { return m_state; }

	bool allowNewNativeAppLaunch(int appMemoryRequirement); // appMemoryRequirement in MB
	
	void monitorNativeProcessMemory(pid_t pid, int maxMemAllowed, pid_t updateFromPid = 0);

	bool getMemInfo(int& lowMemoryEntryRem, int& criticalMemoryEntryRem, int& rebootMemoryEntryRem);

Q_SIGNALS:

	void memoryStateChanged(bool critical);

private:

	MemoryMonitor();
	~MemoryMonitor();

	bool timerTicked();
	int getCurrentRssUsage() const;

	int getProcessMemInfo(pid_t pid);
	
#if defined(HAS_MEMCHUTE)
    static void memchuteCallback(MemchuteThreshold threshold);
	void memchuteStateChanged();
    int getMonitoredProcessesMemoryOffset();
    void checkMonitoredProcesses();
#endif

private:

	Timer<MemoryMonitor> m_timer;
	int m_currRssUsage;

	static const int kFileNameLen = 128;
	char m_fileName[kFileNameLen];

	MemState m_state;	

#if defined(HAS_MEMCHUTE)
	MemchuteWatcher* m_memWatch;
	
	typedef struct 
	{
		pid_t pid;
		int   maxMemAllowed;
		int   violationNumber;
	} ProcMemMonitor;
	
	typedef std::map<pid_t, ProcMemMonitor*> ProcMemRestrictions;

	ProcMemRestrictions memRestrict;

#endif	
	

};

#endif /* MEMORYMONITOR_H */

