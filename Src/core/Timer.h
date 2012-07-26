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




#ifndef TIMER_H
#define TIMER_H

#include "Common.h"

#include <glib.h>
#include <stdint.h>

class  SingletonTimer;
struct TimerHandle;

class TimerBase {
public:

	TimerBase(SingletonTimer* masterTimer);
	virtual ~TimerBase();

	void start(guint intervalInMs, bool singleShot=false);
	void stop();
	bool running() const;

private:

	static void callback(void* arg);
	virtual bool timeout() = 0;

	SingletonTimer* m_master;
	TimerHandle* m_handle;
	bool m_singleShot;
	uint64_t m_interval;
};

template <class Target>
class Timer : public TimerBase
{
public:

	typedef bool (Target::*TimeoutFunction)();

	Timer(SingletonTimer* masterTimer, Target* target, TimeoutFunction function)
		: TimerBase(masterTimer)
		, m_target(target)
		, m_function(function) {}
	
private:

	virtual bool timeout() { return (m_target->*m_function)(); }

	Target* m_target;
	TimeoutFunction m_function;
};	

#endif /* TIMER_H */
