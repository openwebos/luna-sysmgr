/**
 * @file
 * 
 * This file contains the definitions for the TaskBase class, which is a base class meant to serve
 * as an event queue for building "task" classes.
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




#ifndef SYNCTASK_H
#define SYNCTASK_H

#include "Common.h"

#include "TaskBase.h"

/**
 * Base class for tying a TaskBase to GLib
 * 
 * Constructs a {@link TaskBase TaskBase} and ties it to
 * the proper GLib structures to it's relatively usable.
 * Also sets up a the master timer and ties it to GLib.
 * 
 * If no GLib info is given upon construction, it asks
 * for it from HostBase, thereby insulating any users of
 * this class from needing to know anything about the
 * underlying GLib structures.
 * 
 * Naming rationale is unknown at this point, since it
 * doesn't appear to have anything to do with
 * synchronization or anything.
 */
class SyncTask : public TaskBase
{
public:

	/**
	 * Constructs a SyncTask and ties it to the process's global GLib context
	 * 
	 * Since a GLib context is not given, this
	 * constructor fetches one from the global
	 * state in HostBase and uses it.
	 */
	SyncTask();
	
	/**
	 * Constructs a SyncTask and ties it to a given GLib context
	 * 
	 * Allows a SyncTask to be constructed and
	 * tied to a different GLib context than the
	 * process's global GLib context.  This would
	 * be handy in the case where a new GLib
	 * conext was constructed after the main
	 * process starts and sets up its GLib
	 * context.
	 * 
	 * @param	ctxt			GLib context to tie this task to.
	 */
	SyncTask(GMainContext* ctxt);
	
	/**
	 * Cleans up the resources allocated at construction
	 * 
	 * Since this ties into GLib and timers at
	 * construction, this cleans it all up.
	 * Lets the reference counted objects know
	 * that the pointers to them that we
	 * constructed will no longer be used.
	 */
	virtual ~SyncTask();
	
	virtual void run();
	virtual void quit();

private:
	/**
	 * Handles an event from GLib
	 * 
	 * Doesn't do anything currently since this
	 * class is designed to be used as a base.
	 * 
	 * @param	event			The GLib event to handle.
	 */
	virtual void handleEvent(sptr<Event> event) { /* NO-OP */ }
	
	/**
	 * Whether or not to clean up the main loop when destructed
	 * 
	 * If set to true,
	 * {@link SyncTask::~SyncTask() SyncTask::~SyncTask()}
	 * will clean up the GLib context
	 * automatically.  This is set by the
	 * constructor depending on whether a
	 * context was given or we're using
	 * the process-global context.  If
	 * using the process-global context,
	 * it sets this to false so one
	 * SyncTask user doesn't take down
	 * the entire program.  If a context
	 * was given at construction time,
	 * it's assumed that it was
	 * allocated specifically for this
	 * SyncTask and should be cleaned
	 * up when we're done with it.
	 */
	bool destroyMainLoop;
};

#endif /* SYNCTASK_H */
