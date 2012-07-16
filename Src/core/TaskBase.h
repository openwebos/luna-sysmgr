/**
 * @file
 * 
 * Main entry point for Luna
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




#ifndef TASKBASE_H
#define TASKBASE_H

#include "Common.h"

#include <glib.h>
#include <list>

#include "sptr.h"
#include "AsyncCaller.h"
#include "Event.h"
#include "Mutex.h"
#include "SingletonTimer.h"

/**
 * Abstract base class for runnable tasks
 * 
 * Manages an event queue and feeds each event (in order) to an event handler method to be defined by derived classes.
 * 
 * This class cannot be instantiated by itself but must be derived from.
 */
class TaskBase : public RefCounted
{
public:

	/**
	 * Constructs a new TaskBase
	 * 
	 * Since this class is designed to be built upon, it mainly just sets the internal members to null.
	 */
	TaskBase();
	
	/**
	 * Cleans up the current TaskBase instance
	 * 
	 * Since this class is designed to be built upon, this is here to define it as virtual
	 * so TaskBase pointers can correctly call their derived class's destructor.
	 */
	virtual ~TaskBase();

	/**
	 * Runs this task
	 * 
	 * The bulk of derived classes' funcionality should reside within this method.  This method should
	 * not return until the task is complete.  For example, WebAppManager runs a QCoreApplication within
	 * this method, and when it terminates, this function returns.
	 */
	virtual void run() = 0;

	/**
	 * Terminates this task
	 * 
	 * At this point, it doesn't appear this is used by any deriving classes.  Ideally this would do
	 * cleanup from {@link TaskBase::run() TaskBase::run()}.
	 */
	virtual void quit() = 0;

	/**
	 * Posts an event to this task's event queue
	 * 
	 * Normally events in the queue are fed to the derived class's event handler in order from first to
	 * last.  This method is how you add add events to the queue.  If highPriority is set to true, the
	 * new event is added to the beginning of the queue so it is processed as soon as possible.  Normally
	 * you would post events with highPriority set to false so they're posted to the end of the queue
	 * and processed in the order received.
	 * 
	 * @see sptr
	 * 
	 * @param	event			Smart pointer to an Event object to post
	 * @param	highPriority		Whether or not this event is urgent and should be posted to the start of the queue
	 */
	void postEvent(sptr<Event> event, bool highPriority=false);
	
	/**
	 * Gets this task's GLib main loop struct
	 * 
	 * @todo Figure out why this is needed and document it in more detail - not very familiar with GLib.
	 * 
	 * @return				Pointer to GMainLoop structure filled with info about the GLib main loop
	 */
	GMainLoop* mainLoop() const { return m_mainLoop; }

	/**
	 * Gets a pointer to the master timer for this task
	 * 
	 * Note: This pointer is set to 0 by default by {@link TaskBase::TaskBase() TaskBase::TaskBase()} and
	 * cannot be used until a derived class sets it up.
	 * 
	 * @todo After documenting a few derived classes, figure out what this is used for and document it here.
	 * 
	 * @return				Pointer to the master timer for this task
	 */
    SingletonTimer* masterTimer() const { return m_masterTimer; }
	
protected:

	/**
	 * Event handler
	 * 
	 * Called by the event loop to handle each event, in order, from the event queue
	 * 
	 * @see sptr
	 * 
	 * @param	event			Smart pointer to an Event with info about the event to handle
	 */
	virtual void handleEvent(sptr<Event> event) = 0;
	
	/**
	 * Main GLib context
	 * 
	 * @todo Figure out what this points to.
	 */
	GMainContext* m_mainCtxt;
	
	/**
	 * Pointer to main GLib event loop structure
	 * 
	 * @todo Commonly used throughout the code - needs to be filled in as to what it's for.
	 */
	GMainLoop* m_mainLoop;
	
	/**
	 * Mutex for derived classes to use
	 * 
	 * @todo Fill this in after documenting some derived classes.
	 */
	Mutex m_mutex;
	
	/**
	 * Mutex for the event queue
	 * 
	 * Prevents other threads using the same event queue from modifying the queue while it's being actively
	 * processed.  When one thread is about ready to process the queue and handle events, it will lock this
	 * mutex until it's done modifying the queue at which point it will unlock it so other threads know it's
	 * safe for them to modify it.
	 * 
	 * Note: Mutex stands for "mutually exclusive".
	 */
	Mutex m_eventsMutex;
	
	/**
	 * Event queue
	 * 
	 * This standard library list object stores a list of smart pointers to events that are in the event
	 * processing queue (see {@link sptr sptr} for more information on smart pointers).  Events are stored
	 * in this list in order they should be processed - first list item is the first event that the event
	 * handling loop should feed to the derived class's event handler method.
	 */
	std::list<sptr<Event> > m_eventsList;
	
	/**
	 * Asynchronous caller to handle all events in the event queue
	 * 
	 * Allows the internal event handler loop callback method to be called without stalling out the caller.
	 * 
	 * @todo Check this description for accuracy once AsyncCaller is documented.
	 */
	AsyncCaller<TaskBase>* m_asyncCaller;
	
	/**
	 * Master timer for this task
	 * 
	 * @todo Document this further as derived classes are documented and we figure out what this is used for.
	 */
	SingletonTimer* m_masterTimer;

private:

	/**
	 * Handles all events until the queue is empty
	 * 
	 * Pops events off of the event list and hands them off to {@link TaskBase::handleEvent() handleEvent()}
	 * until the queue is empty.  Once the queue is empty, it returns.  Normally called by {@link m_asyncCaller m_asyncCaller}.
	 */
	void eventCallback();
};

#endif /* TASKBASE_H */
