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




#include "PalmIMEHelpers.h"
#include "Logging.h"
#include <sys/times.h>
#include <unistd.h>

quint64 currentTime()
{
    struct timespec currTime;
    clock_gettime(CLOCK_MONOTONIC, &currTime);

	return ((quint64) currTime.tv_sec * 1000ULL +
			(quint64) currTime.tv_nsec / 1000000ULL);
}

void PerfMonitor::trace(const char * message, GLogLevelFlags logLevel)
{
	quint64 sys_time, user_time;
	if (takeTime(sys_time, user_time))
	{
//			traceTime("-- ", message, logLevel, sys_time, user_time, m_sys_timeLast, m_user_timeLast);
		m_sys_timeLast = sys_time;
		m_user_timeLast = user_time;
	}
}

void PerfMonitor::reset()
{
	if (takeTime(m_sys_timeFirst, m_user_timeFirst))
	{
		m_sys_timeLast = m_sys_timeFirst;
		m_user_timeLast = m_user_timeFirst;
	}
	else
		m_sys_timeLast = m_sys_timeFirst = m_user_timeLast = m_user_timeFirst = 0;
}

PerfMonitor::~PerfMonitor()
{
//		quint64 sys_time, user_time;
//		if (m_text && takeTime(sys_time, user_time))
//			traceTime(">> ", m_text, G_LOG_LEVEL_WARNING, sys_time, user_time, m_sys_timeFirst, m_user_timeFirst);
}

bool PerfMonitor::takeTime(quint64 & sysTime, quint64 & userTime)
{
	quint64 persec = sysconf(_SC_CLK_TCK);
	struct tms	cputimes;
	if (VERIFY(times(&cputimes) != -1))
	{
		sysTime = quint64(cputimes.tms_stime) * 1000LLU / persec;
		userTime = quint64(cputimes.tms_utime) * 1000LLU / persec;
		return true;
	}
	return false;
}

void PerfMonitor::traceTime(const char * step, const char * message, GLogLevelFlags logLevel, const quint64 & sysTime, const quint64 & userTime, const quint64 & sysTimeRef, const quint64 & userTimeRef)
{
	quint64 sys_time = sysTime - sysTimeRef;
	quint64 user_time = userTime - userTimeRef;
	if (sys_time + user_time > 40)
		g_log(G_LOG_DOMAIN, logLevel, "%s%s: System: %Lums, User: %Lums, Total: %Lums\n", step, message, sys_time, user_time, sys_time + user_time);
}
