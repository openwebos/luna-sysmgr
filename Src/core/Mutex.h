/**
 * @file
 * 
 * Mutex class for handling mutually-exclusive access
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




#ifndef MUTEX_H
#define MUTEX_H

#include "Common.h"

#include <sptr.h>
#include <glib.h>

/**
 * Manages mutual exclusivity
 * 
 * In a single-threaded program, you know (with a few exceptions) that
 * when you read from a variable that it will have the same value until
 * you change it.  The same is true in a multi-threaded program, except
 * that the other thread might change it.  Without some kind of a
 * locking mechanism, you could have two threads operating on the same
 * data at the same time, which can cause very bad problems unless you
 * plan for it.
 * 
 * This class implements a data lock that can be used to make sure that
 * no one else is currently trying to use the data you want to change.
 * It does not care what data it's protecting, nor does it have any
 * link to it.  It's purely a locking system to let threads know what
 * other threads are doing.
 * 
 * At this point, this class is mainly a wrapper for GLib's
 * GStaticRecMutex class, but implements it in friendly terms that are
 * a bit shorter than using it directly.  It also allows for
 * portability to other mutex systems.
 */
class Mutex : public RefCounted
{
public:

	/**
	 * Constructs a new mutex
	 * 
	 * Constructs and initializes a new mutex to a state of
	 * "nobody's using this resource right now".
	 */
	Mutex();
	
	/**
	 * Cleans up the mutex
	 * 
	 * Cleans up all resources used by the mutex itself.  Declared
	 * as virtual so if a class derives from it, the correct
	 * derived class's destructor is called, keeping memory clean.
	 */
	virtual ~Mutex();

	/**
	 * Lock the mutex to prevent locking by other threads
	 * 
	 * Try to place a lock on the mutex to signal other mutex
	 * users that it's being used.  If another thread has already
	 * locked the mutex, wait until they unlock it before grabbing
	 * the lock and returning.
	 */
	void lock();
	
	/**
	 * Try to lock the mutex unless another thread has already locked it
	 * 
	 * Attempt locking of this mutex.  If another thread already
	 * has it locked, that's fine - this method will just return FALSE.
	 * Otherwise, if no other threads have a lock, it places a new lock on
	 * this mutex and returns TRUE to indicate that the caller now has
	 * exclusive access to whatever this mutex is supposed to protect.
	 * 
	 * @return			FALSE if another thread already has this mutex locked, TRUE if a new lock was acquired.
	 */
	bool tryLock();
	
	/**
	 * Release this thread's lock on the mutex
	 * 
	 * Unlock this mutex, indicating to other threads that we're done
	 * with whatever this mutex is supposed to protect and that they
	 * are safe to read/write to it.
	 */
	void unlock();

private:

	/**
	 * The underlying GLib mutex object that this class runs on top of
	 * 
	 * This GLib object is what actually does the work of this class.
	 */
	GStaticRecMutex* m_mutex;
};

#endif /* MUTEX_H */
