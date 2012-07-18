/**
 * @file
 * 
 * A timer which calls a function either once or on a recurring basis after a given period of time elapses
 * 
 * @author Hewlett-Packard Development Company, L.P.
 * @author tyrok1
 *
 * @section LICENSE
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
 */




#ifndef TIMER_H
#define TIMER_H

#include "Common.h"

#include <glib.h>
#include <stdint.h>

class  SingletonTimer;
struct TimerHandle;

/**
 * Abstract base class containing timer functionality for {@link Timer Timer} to use
 * 
 * Since Timer is a templated class, this defines the functionality in a
 * non-templated base class.  Only reason I can think of for this class
 * existing is so that the implementation for it is not duplicated for
 * each type of class that Timer would be templated to.
 * 
 * Since this class has no provisions in its public functions for adding
 * a callback, it must be derived from to be actually used.
 */
class TimerBase {
public:
	/**
	 * Construct a timer and attach it to a parent timer
	 * 
	 * TimerBase makes {@link SingletonTimer SingletonTimer} a bit easier
	 * to use.  As such, it needs to have a SingletonTimer to attach to
	 * so it can keep track of the system clock for us.
	 * 
	 * @param	masterTimer		SingletonTimer to attach to to use as a clock source.
	 */
	TimerBase(SingletonTimer* masterTimer);
	
	/**
	 * Clean up memory
	 * 
	 * Since we're adding another reference to the master timer in the
	 * constructor and it's a reference counted object, it's entirely
	 * possible that once the current class instance it destroyed the
	 * master timer it's attached to can also be.  This does that.
	 * 
	 * It's also declared as virtual so the correct destructor is called
	 * for derived classes.
	 */
	virtual ~TimerBase();

	/**
	 * Start this timer running and define an interval
	 * 
	 * This is where you set how often you want your callback to be
	 * called.
	 * 
	 * @param	intervalInMs		How often you want your callback called (in milliseconds).
	 * @param	singleShot		Whether to only run once after intervalInMs milliseconds or to run every intervalInMs milliseconds until told to stop.
	 */
	void start(guint intervalInMs, bool singleShot=false);
	
	/**
	 * Stop this timer from running
	 * 
	 * Cancel further running of this timer.  Stops the timer from
	 * calling your callback again.
	 */
	void stop();
	
	/**
	 * Checks whether the timer still waiting
	 * 
	 * Returns a boolean value indicating whether or not this timer has
	 * been started and is still waiting to call a callback.  For non-
	 * recurring timers, a true value means that the timer has been
	 * started but the callback has not been called yet.  For recurring
	 * timers, a true value means that it has been started and not
	 * explicitly stopped.
	 * 
	 * @return				Whether this timer is currently waiting.
	 */
	bool running() const;

private:
	/**
	 * Method which can be given to the underlying SingletonTimer which runs the derived class's callback
	 * 
	 * This method is what's given to the underlying master
	 * {@link SingletonTimer SingletonTimer} which calls the derived
	 * class's timeout() method.  It then either sets up the next
	 * recurrence (for recurring timers) or cleans up resources (for
	 * one-shot timers).
	 * 
	 * This method allows the functionality for multiple templated
	 * derived classes to be handled in one place without having to
	 * reimplement the timer-related functionality per-class.
	 * 
	 * @param	arg			Pointer to the derived class which the callback needs to be dispatched to.
	 */
	static void callback(void* arg);
	
	/**
	 * Callback to be overridden by derived classes
	 * 
	 * This method is what callback calls and is not implemented in
	 * this class.  It must be overridden for this class to be
	 * useful, which is what {@link Timer Timer} does.  By
	 * implementing this in a separated form, the code of callback()
	 * is not duplicated per type of Timer.
	 * 
	 * @todo Figure out why this isn't protected instead of private.
	 */
	virtual bool timeout() = 0;
	
	/**
	 * Master SingletonTimer that drives the timers constructed with this class
	 */
	SingletonTimer* m_master;
	
	/**
	 * Timer callback handle given by the master timer when a callback is set up
	 */
	TimerHandle* m_handle;
	
	/**
	 * Whether this timer should only run once instead of running continuously
	 */
	bool m_singleShot;
	
	/**
	 * The time interval on which this timer should fire
	 */
	uint64_t m_interval;
};

/**
 * Templated timer class
 * 
 * Timer class which can be set up to run a method within another class.
 * Allows timers to be set up in two steps:
 * - Construct a Timer<target class> with the method to call
 * - Call {@link Timer::start() Timer::start()} to set up the interval and start the timer running
 */
template <class Target>
class Timer : public TimerBase
{
public:
	/**
	 * Define the type of method that can be used as a timer callback
	 * 
	 * @return					Success of callback - false to indicate failure of the callback and cancel further recurrence of this timer, true to indicate success and that it should continue.
	 */
	typedef bool (Target::*TimeoutFunction)();
	
	/**
	 * Construct a timer and set up the callback
	 * 
	 * Sets up a timer and ties it to a master SingletonTimer
	 * to use as a clock source, a target object, and the
	 * method within that object to use as a callback when the
	 * time specified in {@link Timer::start() Timer::start()}
	 * has elapsed.
	 * 
	 * @param	masterTimer			SingletonTimer to use as a clock source.
	 * @param	target				Object containing the method to call when time elapses.
	 * @param	function			The method within the given class to call when time elapses.
	 */
	Timer(SingletonTimer* masterTimer, Target* target, TimeoutFunction function)
		: TimerBase(masterTimer)
		, m_target(target)
		, m_function(function) {}
	
private:
	/**
	 * Dispatch the timer callback to the callback set up in the constructor
	 * 
	 * This method is called by the base functionality in
	 * {@link TimerBase::callback() TimerBase::callback()}
	 * when the proper amount of time has elapsed.  This in
	 * turn calls the callback given in Timer's constructor.
	 * 
	 * @return					Success of callback - false to indicate failure of the callback and cancel further recurrence of this timer, true to indicate success and that it should continue.
	 */
	virtual bool timeout() { return (m_target->*m_function)(); }

	/**
	 * Object containing the desired callback method
	 */
	Target* m_target;
	
	/**
	 * Pointer to callback method within target object which should be called when the correct amount of time elapses
	 */
	TimeoutFunction m_function;
};	

#endif /* TIMER_H */
