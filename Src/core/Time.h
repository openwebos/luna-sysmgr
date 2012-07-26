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




#ifndef TIME_H
#define TIME_H

#include "Common.h"

#include <sys/time.h>
#include <time.h>
#include <stdint.h>

class Time
{
public:

	static inline uint32_t convertToMs(const struct timeval* time)
	{
		return (uint32_t) (time->tv_sec * 1000 + time->tv_usec / 1000);
	}

	static inline uint32_t convertToMs(const struct timespec* time)
	{
		return (uint32_t) (time->tv_sec * 1000 + time->tv_nsec / 1000000);
	}

	static inline uint32_t curTimeMs()
	{
		struct timespec curTime;
		::clock_gettime(CLOCK_MONOTONIC, &curTime);
		return convertToMs(&curTime);    
	}
/*	
	static inline uint32_t curUpTimeMs()
	{
		struct timespec curTime;
		::clock_gettime(CLOCK_UPTIME, &curTime);
		return convertToMs(&curTime);    
	}
*/
	static inline int curTime(struct timespec* time)
	{
		return ::clock_gettime(CLOCK_MONOTONIC, time);
	}

	static inline int curTime(struct timeval* time)
	{
		struct timespec tsTime;
		if (!curTime(&tsTime)) {
			time->tv_sec = tsTime.tv_sec;
			time->tv_usec = tsTime.tv_nsec / 1000;
			return 0;
		}
		return -1;
	}

	static inline uint32_t curSysTimeMs() {
		struct timeval currTime;
		::gettimeofday(&currTime, NULL);
		return convertToMs(&currTime);
	}
};

#endif	/* TIME_H */
