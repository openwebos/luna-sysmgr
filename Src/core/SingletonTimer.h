/**
 * @file
 * 
 * A timer which calls a function after a given amount of time elapses.  Also allows querying of the system clock.
 * 
 * @todo Clean up the mess of goofy memory management macros in SingletonTimer.cpp.
 * 
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




#ifndef SINGLETONTIMER_H
#define SINGLETONTIMER_H

#include "Common.h"

#include <glib.h>
#include <stdint.h>

struct TimerHandle;
struct TimerSource;

typedef void (*TimerCallback)(void* userArg);

/**
 * Timer management utility class
 * 
 * Allows for creation and management of timers
 * which call a callback after a specified time.
 * Also allows for querying of the system clock.
 * 
 * Despite the name, this class is not designed as
 * a true singleton.  It's designed to be
 * instantiated once per GLib main loop object to
 * generate timers associated with it.
 */
class SingletonTimer
{
public:
	/**
	 * Constructs a new timer generator
	 * 
	 * SingletonTimer is largely implemented
	 * using GLib functionality.  As such, it
	 * needs a GLib main loop structure to tie
	 * GLib's timer functions to.  That's where
	 * this constructor comes in.  This ties
	 * the SingletonTimer object to GLib so
	 * other timer users don't need to worry
	 * about GLib.
	 * 
	 * @param	loop		The GLib main loop structure to tie the timers to
	 */
	SingletonTimer(GMainLoop* loop);
	
	/**
	 * Clean up resources allocated for timers
	 * 
	 * Currently unused, but declared
	 * nonetheless.
	 */
	~SingletonTimer();

	/**
	 * Sets up a new timer callback
	 * 
	 * Timers are set up in a two-stage process.
	 * Firstm you set up the function to call,
	 * then you set up the timing.  This method
	 * sets up the callback, which can then be
	 * passed into
	 * {@link SingletonTimer::fire() SingletonTimer::fire()}
	 * to activate that callback to run at a
	 * given time.
	 * 
	 * @param	callback		Pointer to the function to call.
	 * @param	userArg			Pointer to data to pass to the callback function.
	 * 
	 * @return				A TimerHandle pointer representing the callback function.
	 */
	TimerHandle* create(TimerCallback callback, void* userArg);
	
	/**
	 * Activate a timer callback to be called at after a given time period elapses
	 * 
	 * This method is what actually sets up the
	 * timer to run.  It takes a callback set up
	 * by {@link SingletonTimer::create() SingletonTimer::create()}
	 * and runs it after the given number of
	 * milliseconds have passed.
	 * 
	 * @param	timer			A TimerHandle pointer from {@link SingletonTimer::create() SingletonTimer::create()} containing information about the code to run.
	 * @param	timeInMs		How many milliseconds to wait before calling the function from the "timer" parameter.
	 */
	void         fire(TimerHandle* timer, uint64_t timeInMs);
	
	/**
	 * Indicates to a timer callback that there is a new pointer to it
	 * 
	 * TimerHandle objects are reference
	 * counted like in the {@link sptr sptr}
	 * system.  SingletonTimer::ref() and
	 * SingletonTimer::deref() are the correct
	 * way to indicate how many pointers there
	 * are to a timer calkback so they an be
	 * automatically cleaned up from memory
	 * when everything is done using them.
	 * 
	 * @see sptr
	 * @todo Change this to have it use actual smart pointers instead of duplicating the functionality here.
	 * 
	 * @param	timer			The timer callback that has a new pointer to it.
	 */
	void         ref(TimerHandle* timer);
	
	/**
	 * Indicates the a timer callback that a previous pointer to it has been destroyed
	 * 
	 * Primitive form of reference counting.  Keeps
	 * track of how many things are using a TimerHandle
	 * and automatically deletes it when nothing needs
	 * it any longer.
	 * 
	 * @see SingletonTimer::ref()
	 * @see RefCounted
	 * @see sptr
	 * 
	 * @param	timer				The timer callback that has had a pointer to it destroyed.
	 */
	void         deref(TimerHandle* timer);
	

	/**
	 * Gets the number of milliseconds since some system-wide unknown time
	 * 
	 * Returns the number of milliseconds since
	 * some unspecified point in time as given
	 * by the system clock.  While the "zero"
	 * point of this value is arbitrary, it's
	 * consistent between every call to it while
	 * the program is running.  Because of this,
	 * it can be used as a very accurate clock
	 * for calculating time differences but not
	 * absolute time/date values.
	 * 
	 * Implemented as static since it uses the
	 * system-wide clock rather than using GLib.
	 */
	static uint64_t currentTime();
	
	// Internal functions. don't use
	/**
	 * Event source function for GLib timers
	 * 
	 * DO NOT USE.  This function should only be
	 * called by the GLib functionality within
	 * this class.
	 * 
	 * @todo Clean this up - having non-callable public methods is a messy way to do things.
	 * @todo Figure out how/why this works - GLib documentation on the subject seems to be pretty sparse.
	 */
	static gboolean timerPrepare(GSource* source, gint* timeout);
	
	/**
	 * Event source function for GLib timers
	 * 
	 * DO NOT USE.  This function should only be
	 * called by the GLib functionality within
	 * this class.
	 * 
	 * @todo Clean this up - having non-callable public methods is a messy way to do things.
	 * @todo Figure out how/why this works - GLib documentation on the subject seems to be pretty sparse.
	 */
	static gboolean timerCheck(GSource* source);
	
	/**
	 * Event source function for GLib timers
	 * 
	 * DO NOT USE.  This function should only be
	 * called by the GLib functionality within
	 * this class.
	 * 
	 * @todo Clean this up - having non-callable public methods is a messy way to do things.
	 * @todo Figure out how/why this works - GLib documentation on the subject seems to be pretty sparse.
	 */
	static gboolean timerDispatch(GSource* source, GSourceFunc callback, gpointer userData);
	
private:
	/**
	 * Deactivates and deletes a running timer
	 * 
	 * This method is the correct way to delete
	 * an active timer from memory, since it
	 * deactivates it in GLib first.
	 * 
	 * @param	handle			The timer callback to deactivate.
	 */
	void destroy(TimerHandle* handle);
	
	/**
	 * The GLib main loop structure to tie timers to
	 */
	GMainLoop*   m_loop;
	
	/**
	 * GLib event source for the associated main loop
	 */
	TimerSource* m_source;
	
	/**
	 * List of TimerHandle pointers of all currently active timer callback functions
	 * 
	 * If a TimerHandle* is returned by
	 * {@link SingletonTimer::create() SingletonTimer::create()}
	 * and it has not been destroyed by
	 * {@link SingletonTimer::destroy() SingletonTimer::destroy()},
	 * then that pointer is on this list.
	 */
	GList*       m_activeList;

private:
	/**
	 * Block the copy constructor
	 * 
	 * Since this class is designed as a per-
	 * GLib main loop singleton, it should never
	 * be copied.  Declaring the copy
	 * constructor as private helps prevent
	 * that.
	 * 
	 * @param	from			Other class instance to copy from.
	 */
	SingletonTimer(const SingletonTimer&);
	
	/**
	 * Block the assignment operator
	 * 
	 * Since this class is designed as a per-
	 * GLib main loop singleton, it should never
	 * be copied.  Declaring the assignment
	 * operator as private helps prevent
	 * that by blocking "a = b;" statements.
	 * 
	 * @param	from			Other class instance to copy from.
	 * @return				The new class instance.
	 */
	SingletonTimer& operator=(const SingletonTimer&);
};

#endif /* SINGLETONTIMER_H */
