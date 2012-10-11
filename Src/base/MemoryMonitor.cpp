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




#include "Common.h"

#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <strings.h>


#include "MemoryMonitor.h"
#if defined(HAS_MEMCHUTE)
#include "IpcServer.h"
#endif

#include "Settings.h"
#include "Time.h"
#include "HostBase.h"

static const int kTimerMs = 5000;
static const int kLowMemExpensiveTimeoutMultiplier = 2;
static const int kNativeMaxMemoryViolationThreshold = 1;

static const std::string sMemTotal("MemTotal");
static const std::string sMemFree("MemFree");
static const std::string sSwapTotal("SwapTotal");
static const std::string sSwapFree("SwapFree");
static const std::string sMemchuteFree("MemchuteFree");
static const std::string sKBLabel("kb");
static const std::string sMBLabel("mb");

static const std::string sProcRSS("VmRSS");
static const std::string sProcSwap("VmSwap");

MemoryMonitor* MemoryMonitor::instance()
{
	static MemoryMonitor* s_instance = 0;
	if (G_UNLIKELY(s_instance == 0))
		s_instance = new MemoryMonitor;

	return s_instance;
}

MemoryMonitor::MemoryMonitor()
	: m_timer(HostBase::instance()->masterTimer(), this, &MemoryMonitor::timerTicked)
	, m_currRssUsage(0)
	, m_state(MemoryMonitor::Normal)
{
	m_fileName[kFileNameLen - 1] = 0;
	snprintf(m_fileName, kFileNameLen - 1, "/proc/%d/statm", getpid());

	char oom_adj[kFileNameLen];
	snprintf(oom_adj, kFileNameLen - 1, "/proc/%d/oom_adj", getpid());
	FILE* f = fopen(oom_adj, "wb");
	if (f) {
		size_t result = fwrite("-17\n", 4, 1, f);
		(void)result;

		fclose(f);	
	}
}

MemoryMonitor::~MemoryMonitor()
{    
}

void MemoryMonitor::start()
{
	if (m_timer.running())
		return;
	
	m_timer.start(kTimerMs);

#if defined(HAS_MEMCHUTE)
	m_memWatch = MemchuteWatcherNew(MemoryMonitor::memchuteCallback);
	if (m_memWatch != NULL) {
		MemchuteGmainAttach(m_memWatch, HostBase::instance()->mainLoop());
		MemchuteGmainSetPriority(m_memWatch, G_PRIORITY_HIGH);
	} else {
		g_warning("Failed to create MemchuteWatcher");
	}
#endif
}

static const char* nameForState(MemoryMonitor::MemState state)
{
	switch (state) {
	case (MemoryMonitor::Low):
		return "Low";
	case (MemoryMonitor::Critical):
		return "Critical";
	case (MemoryMonitor::Medium):
		return "Medium";
	default:
		break;
	}
	
	return "Normal";
}

bool MemoryMonitor::timerTicked()
{
#if defined(HAS_MEMCHUTE)
	if (!memRestrict.empty())
		checkMonitoredProcesses();
#endif

	if (m_state == Normal)	{
		return true;
	}

	m_currRssUsage = getCurrentRssUsage();

	g_warning("SysMgr MemoryMonitor: LOW MEMORY: State: %s, current RSS usage: %dMB\n",
			  nameForState(m_state), m_currRssUsage);

	return true;
}

int MemoryMonitor::getCurrentRssUsage() const
{
	FILE* f = fopen(m_fileName, "rb");
	if (!f)
		return m_currRssUsage;

	int totalSize, rssSize;

	int result = fscanf(f, "%d %d", &totalSize, &rssSize);
	(void)result;

	fclose(f);	
	
    return (rssSize * 4096) / (1024 * 1024);
}

bool MemoryMonitor::getMemInfo(int& lowMemoryEntryRem, int& criticalMemoryEntryRem, int& rebootMemoryEntryRem)
{
    std::ifstream memInfo("/sys/class/memnotify/meminfo");
    if (!memInfo) {
		g_warning("MemoryMonitor::getMemInfo Failed to open /sys/class/memnotify/meminfo");
		return false;
	}

	std::string field;

	// Skip lines till we reach the "Enter Thresholds" section
	bool inEnterThresholdSection = false;
	while (memInfo >> field) {
		if (strcasecmp(field.c_str(), "Enter") == 0) {
			std::getline(memInfo, field);
			inEnterThresholdSection = true;
			break;
		}

		std::getline(memInfo, field);
	}

	if (!inEnterThresholdSection) {
		g_warning("MemoryMonitor::getMemInfo Could not find Enter Threshold section");
		return false;
	}

	/*
	  Sample file contents:

	  Used (Mem+Swap): 160MB
	  Used (Mem): 91MB
	  Used (Swap): 68MB
	  Used Ratio: 79%
	  Free: 40MB
	  Current Threshold: normal
	  Last Threshold: normal
	  Enter Thresholds:
	  normal: 0, 0MB, Rem: -160MB:
	  low: 100, 200MB, Rem: 40MB:
	  critical: 114, 228MB, Rem: 68MB:
	  reboot: 120, 240MB, Rem: 80MB:
	  Leave Thresholds:
	  normal: 0, 0MB, Rem: -160MB:
	  low: 94, 188MB, Rem: 28MB:
	  critical: 108, 216MB, Rem: 56MB:
	  reboot: 112, 224MB, Rem: 64MB:
	*/

	std::string dummyStr;
	int dummyInt;

	lowMemoryEntryRem = -1;
	criticalMemoryEntryRem = -1;
	rebootMemoryEntryRem = -1;

	const int neededEntries = 3;
	int foundEntries = 0;
	
	
	while (memInfo >> field) {
		if (strcasecmp(field.c_str(), "low:") == 0) {

			memInfo >> dummyStr;
			memInfo >> dummyStr;
			memInfo >> dummyStr;
			memInfo >> lowMemoryEntryRem;
			memInfo >> dummyStr;

			foundEntries++;
		}
		else if (strcasecmp(field.c_str(), "critical:") == 0) {

			memInfo >> dummyStr;
			memInfo >> dummyStr;
			memInfo >> dummyStr;
			memInfo >> criticalMemoryEntryRem;
			memInfo >> dummyStr;

			foundEntries++;
		}
		else if (strcasecmp(field.c_str(), "reboot:") == 0) {

			memInfo >> dummyStr;
			memInfo >> dummyStr;
			memInfo >> dummyStr;
			memInfo >> rebootMemoryEntryRem;
			memInfo >> dummyStr;

			foundEntries++;
			// Done
			break;
		}
		else {
			std::getline(memInfo, field);
		}
	}

	memInfo.close();
    return (neededEntries == foundEntries);
}

int MemoryMonitor::getProcessMemInfo(pid_t pid)
{
	int procRss  = -1;
	int procSwap = -1;
	
 	char fileName[kFileNameLen];
 	fileName[kFileNameLen - 1] = 0;
 	
	snprintf(fileName, kFileNameLen - 1, "/proc/%d/status", pid);
    std::ifstream status(fileName);
   
    if (!status) {
        return -1;
    }
    std::string field;
    std::string label;

    while(status >> field) {
        // strip off the ':' on the end of each label
        field = field.substr(0, field.length() - 1);
        
        if (field == sProcRSS) {
            status >> procRss;
            status >> label;

            //Make sure the value is in megabytes
            if (!strcasecmp(label.c_str(), sKBLabel.c_str())) {
            	procRss /= 1024;
            } else if (strcasecmp(label.c_str(), sMBLabel.c_str())) {
            	procRss /= 1024 * 1024;
            }

            if(procSwap != -1) break;
        } else if (field == sProcSwap) {
            status >> procSwap;
            status >> label;

            //Make sure the value is in megabytes
            if (!strcasecmp(label.c_str(), sKBLabel.c_str())) {
            	procSwap /= 1024;
            } else if (strcasecmp(label.c_str(), sMBLabel.c_str())) {
            	procSwap /= 1024 * 1024;
            }
            
            if(procRss != -1) break;
        }       
    }
    
    status.close();
    
    if ((-1 == procRss) || (-1 == procSwap))
    	return -1;
    
    return procRss + procSwap;
}

void MemoryMonitor::monitorNativeProcessMemory(pid_t pid, int maxMemAllowed, pid_t updateFromPid)
{
#if defined(HAS_MEMCHUTE)
	if(updateFromPid > 0){
		// updating an existing monitor, so find it and remove it first
		ProcMemRestrictions::iterator old = memRestrict.find(updateFromPid);
		if (old != memRestrict.end()){
			ProcMemMonitor *monitor = old->second;
			if (!maxMemAllowed){
				// preserve the maxMemAllowed value from the old monitor
				maxMemAllowed = monitor->maxMemAllowed;
				// remove the old monitor
				memRestrict.erase(old);
				delete monitor;
			}
		}
	} 
	
	ProcMemMonitor *monitor = new ProcMemMonitor;
	
	monitor->pid = pid;
	monitor->maxMemAllowed = maxMemAllowed;
	monitor->violationNumber = 0;
	
	memRestrict[pid] = monitor;
#endif
}

#if defined(HAS_MEMCHUTE)
int MemoryMonitor::getMonitoredProcessesMemoryOffset()
{
	int offset = 0;	
	int takenMem, declaredMem;
	ProcMemRestrictions::iterator it, temp;
	
	it = memRestrict.begin();
	
	// iterate through all monitored processes
	while (it != memRestrict.end()) {
		temp = it;
		++it;
		
		ProcMemMonitor *monitor = temp->second;
		
		// declared memory figure
		declaredMem = monitor->maxMemAllowed;
		
		// find out how much memory the process is actually taking at the moment
		takenMem = getProcessMemInfo(monitor->pid);
		
		if (declaredMem > takenMem){ // if process isn't at or above its declared memory figure
			// add the difference to the memory offset
			offset += declaredMem - takenMem;
		}
	}
	
	return offset;
}


void MemoryMonitor::checkMonitoredProcesses()
{
	int procMem;
	
	ProcMemRestrictions::iterator it, temp;
	it = memRestrict.begin();
	while (it != memRestrict.end()) {
		temp = it;
		++it;
		
		ProcMemMonitor *monitor = temp->second;
		
		procMem = getProcessMemInfo(monitor->pid);
		
		if(-1 == procMem) { // Process doesn't exist (terminated), so remove the entry from the monitor list
			memRestrict.erase(temp);
			delete monitor;
		}
		else {
			// valid process, so check if its memory consumption is within the provided limits
			if (procMem > monitor->maxMemAllowed) {
				if (monitor->violationNumber < kNativeMaxMemoryViolationThreshold) {
					g_warning("MemoryMonitor: Monitored native process # %d exceeded its memory quota. ProcMem = %d, restriction = %d, violation count = %d\n",
							monitor->pid, procMem, monitor->maxMemAllowed, monitor->violationNumber);
					monitor->violationNumber++;
				}
				else {
					// exceeded memory restriction
					if(Normal != m_state) { // Low memory situation, kill the process						
						g_warning("MemoryMonitor: Monitored native process # %d exceeded its memory quota. ProcMem = %d, restriction = %d\nTERMINATING the process!\n",
								monitor->pid, procMem, monitor->maxMemAllowed);
						
						// terminate the process
						IpcServer::instance()->killProcess(monitor->pid, true);
						
						// remove the entry from the monitor list
						memRestrict.erase(temp);
						delete monitor;
					}
					else {
						g_warning("MemoryMonitor: Monitored native process # %d exceeded its memory quota. ProcMem = %d, restriction = %d, violation count = %d\n",
								monitor->pid, procMem, monitor->maxMemAllowed, monitor->violationNumber);						
					}
				}
			}
			else {
				// reset violation counter
				monitor->violationNumber = 0;
			}
		}
	}
	
	
}
#endif

bool MemoryMonitor::allowNewNativeAppLaunch(int appMemoryRequirement)
{
	if (m_state >= Low){
		// already in Low or critical memory states, so do not allow new apps to be launched
		return false;
	}
	
#if defined(HAS_MEMCHUTE)
	int lowMemoryEntryRem, criticalMemoryEntryRem, rebootMemoryEntryRem;
	getMemInfo(lowMemoryEntryRem, criticalMemoryEntryRem, rebootMemoryEntryRem);

	// Check if the new used ratio would put us past the memchute threshold
	int monitoredProcessMemoryOffset = getMonitoredProcessesMemoryOffset();
	if ((appMemoryRequirement + monitoredProcessMemoryOffset) > criticalMemoryEntryRem) {
		// Not enough memory; deny the app launch
		g_warning("MemoryMonitor: Required memory %d, Available memory till critical state: %d",
				  appMemoryRequirement + monitoredProcessMemoryOffset, criticalMemoryEntryRem);
		return false;
	}	
#endif
	
	// OK to launch new app with specified memory requirements
	return true;
}

#if defined(HAS_MEMCHUTE)
void MemoryMonitor::memchuteCallback(MemchuteThreshold threshold)
{
	MemoryMonitor* mw = MemoryMonitor::instance();	

	switch (threshold) {
	case (MEMCHUTE_NORMAL):
		mw->m_state = Normal;
		break;
	case (MEMCHUTE_MEDIUM):
		mw->m_state = Medium;
		break;
	case (MEMCHUTE_LOW):
		mw->m_state = Low;
		break;
	case (MEMCHUTE_CRITICAL):
	case (MEMCHUTE_REBOOT):
		mw->m_state = Critical;
		break;
	default:
		break;
	}

	mw->memchuteStateChanged();
}

void MemoryMonitor::memchuteStateChanged()
{
	Q_EMIT memoryStateChanged(m_state == Critical);
}
#endif
