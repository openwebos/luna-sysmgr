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
 * Code derived from smart_ptr.h by Thatcher Ulrich, 2003.
 * Original copyright follows:
 *
 * smart_ptr.h  -- by Thatcher Ulrich <tu@tulrich.com> 2003
 *
 * This source code has been donated to the Public Domain.  Do
 * whatever you want with it.
 * 
 */





#ifndef SPTR_H
#define SPTR_H

#include "Common.h"

#include <glib.h>

/**
 * Base class for reference counted objects
 * 
 * This class keeps a "reference count" of how many other objects currently reference this object
 * so it is not deleted from memory while other objects are still using it.  By deriving from this
 * class, subclasses will automatically clean themselves up from memory once they are no longer being
 * used.
 * 
 * Every time a new pointer is created that points to this object, the code managing the pointer should
 * call {@link RefCounted::ref() RefCounted::ref()} to increment the reference count, indicating that
 * there's something new referencing this instance.  When that pointer is no longer going to be used,
 * the code managing the pointer should call {@link RefCounted::deref() RefCounted::deref()} to
 * decrement the reference count, indicating that there is one less pointer to this object floating
 * around in memory.  When this object detects that the reference count reaches zero (meaning that there
 * is nothing currently using the object), {@link RefCounted::deref() RefCounted::deref()} automatically
 * cleans it up from memory.
 * 
 * This class is not intended to be instantiated on its own, though there is nothing preventing that.
 * It is designed as a utility base class for other classes to derive from.  With this class around, all
 * you have to do to have your object automatically clean up its own memory is to derive from this class.
 * The functionality in RefCounted takes care of all of the details for you.
 */
class RefCounted
{
public:

	/**
	 * Initializes the reference count to zero
	 * 
	 * By setting the initial reference count to zero, it indicates that this object is not currently
	 * being used by any other classes or pieces of code.  Shortly after being constructed, in most
	 * cases the code constructing the subclass should call {@link RefCounted::ref() RefCounted::ref()}
	 * to indicate that it is, in fact, being used.
	 */
	RefCounted() : m_refCount(0) {}
	
	/**
	 * Cleans up a reference counted object
	 * 
	 * Since the reference counter is automatically cleaned up at destruction, this serves as a
	 * placeholder to ensure that the destructor is declared as virtual.  This makes it so that when a
	 * RefCounted pointer is deleted, the destructor of the derived class is called instead of just the
	 * destructor of RefCounted.
	 */
	virtual ~RefCounted() {}

	/**
	 * Increments the reference count
	 * 
	 * Calling of this method denotes that there is a new pointer to this object somewhere in memory.
	 * Every time a new pointer is created to a class derived from RefCounted, {@link RefCounted::ref() RefCounted::ref()}
	 * should be called on that object so it can track how many open pointers there are for automatic
	 * memory cleanup.
	 */
	void ref()
	{
		g_atomic_int_inc(&m_refCount);
	}

	/**
	 * Decrements the reference count and cleans up the object if there are no more references
	 * 
	 * Calling of this method denotes that a previously-used pointer to this object will no longer be used.
	 * This method allows for automatic cleanup of a derived class when it is no longer used.
	 */
	void deref()
	{
		if (g_atomic_int_dec_and_test(&m_refCount)) {
			delete this;
		}
	}

private:

	/**
	 * Number of pointers to this object currently in memory
	 */
	gint m_refCount;

	/**
	 * Block the copy constructor from being used publicly
	 * 
	 * Since a reference count to one object is not valid for another instance of that object elsewhere in
	 * memory, a copy constructor for the reference count would cause memory leaks.  As an example,
	 * consider the following scenario:
	 * 
	 * - A reference counted object is created.
	 * - 5 pointers to it are created with {@link RefCounted::ref() RefCounted::ref()} called for each one.  The reference count for the object is now 5.
	 * - The reference counted object is copied, but 3 pointers to it are created.
	 * - At this point, we now have an object in memory with 5 legitimate pointers to it and an object in a different location which thinks there are 5 pointers to it when there are only three.
	 * - All 5 pointers to the first object and all 3 pointers to the second have {@link RefCounted::deref() RefCounted::deref()} called for them.
	 * - First object now cleaned itself up correctly.  Second object cannot remove itself from memory until the program terminates, resulting in a memory leak.
	 * 
	 * @param	copy			Reference to the object to copy
	 */
	RefCounted(const RefCounted&);
	
	/**
	 * Block the copy operator from being used publicly
	 * 
	 * Since a reference count to one object is not valid for another instance of that object elsewhere in
	 * memory, a copy constructor for the reference count would cause memory leaks.  As an example,
	 * consider the following scenario:
	 * 
	 * - A reference counted object is created.
	 * - 5 pointers to it are created with {@link RefCounted::ref() RefCounted::ref()} called for each one.  The reference count for the object is now 5.
	 * - The reference counted object is copied, but 3 pointers to it are created.
	 * - At this point, we now have an object in memory with 5 legitimate pointers to it and an object in a different location which thinks there are 5 pointers to it when there are only three.
	 * - All 5 pointers to the first object and all 3 pointers to the second have {@link RefCounted::deref() RefCounted::deref()} called for them.
	 * - First object now cleaned itself up correctly.  Second object cannot remove itself from memory until the program terminates, resulting in a memory leak.
	 * 
	 * @param	copy			Reference to the object to copy
	 */
	RefCounted& operator=(const RefCounted&);
};
	

/**
 * Smart pointer object
 * 
 * @todo Limit this class to only being used on derivatives of RefCounted.
 * 
 * Smart pointers are a way of having a pointer that automatically tracks use and cleans up the
 * object it points to when nobody is using it.  It's a convenience class so pointer users don't
 * have to worry about cleaning up objects they're pointing to.  If everything that uses a
 * pointer uses this class, memory should stay much cleaner over long running times by preventing
 * memory leaks from code not explicitly cleaning up the memory it uses.
 * 
 * This class is basically a drop-in replacement for a regular pointer which can be used for
 * pointing to any class that derives from {@link RefCounted RefCounted}.  It handles the details
 * of tracking the reference count automatically.  By using this smart pointer object, you should
 * never have to worry about calling {@link RefCounted::ref() RefCounted::ref()},
 * {@link RefCounted::deref() RefCounted::deref()}, or cleaning up memory.
 */
template<class T>
class sptr
{
public:

	/**
	 * Initializes a smart pointer to a reference counted object
	 * 
	 * Since this is a new pointer to the object, its reference count needs to track that
	 * there's a new pointer to it.  By doing that here, we insulate the user of this
	 * smart pointer from having to worry about that.
	 * 
	 * @param	ptr		Pointer to object to make a smart pointer to
	 */
	sptr(T* ptr) : m_ptr(ptr)
	{
		if (m_ptr)
		{
			m_ptr->ref();
		}
	}

	/**
	 * Initialize a null smart pointer
	 * 
	 * There may be times where a function requires a smart pointer but you want to indicate
	 * a null.  This constructor allows that without trying to call
	 * {@link RefCounted::ref() RefCounted::ref()} on a null pointer and crashing the program.
	 */
	sptr() : m_ptr(0) {}

	/**
	 * Copy another smart pointer
	 * 
	 * This copy constructor allows you to copy another smart pointer into this one.  Since
	 * it is a new pointer to the same object, we need to indicate that to the RefCounted
	 * object so it can track when to clean up.  So we increment the reference count here
	 * so if the "copied from" smart pointer is deleted, it's still tracking our new pointer.
	 * 
	 * @param	s		Smart pointer to copy
	 */
	sptr(const sptr<T>& s) : m_ptr(s.m_ptr)
	{
		if (m_ptr)
		{
			m_ptr->ref();
		}
	}

	/**
	 * Clean up smart pointer
	 * 
	 * We're now getting rid of a smart pointer, which means we need to let our target
	 * object know that.  Tell it by calling {@link RefCounted::deref() RefCounted::deref()}
	 * on it.  Once we no longer have any pointers to the object, it may just clean itself
	 * up, but that's up to the target object to decide.
	 */
	~sptr()
	{
		if (m_ptr)
		{
			m_ptr->deref();
		}
	}

	/**
	 * Smart pointer copy operator
	 * 
	 * This operator is called when you set a smart pointer = another smart pointer.  But
	 * it's not quite that simple.  Since we're copying a pointer to another pointer, we're
	 * having a new pointer point to the same object, which means we need to tell that
	 * object about it so it can track it for cleanup.  Without this, it could potentially
	 * delete itself from memory if the "copied from" pointer is cleaned up, leaving the
	 * new copy with an invalid pointer.
	 * 
	 * @param	s		Smart pointer to copy
	 */
	void	operator=(const sptr<T>& s) { setRef(s.m_ptr); }
	
	/**
	 * Pointer copy operator
	 * 
	 * This operator is similar to the smart pointer copy operator, but has one interesting
	 * difference.  This is an overloaded version that is triggered when copying from a
	 * regular pointer instead of another smart pointer.  This allows for compatibility with
	 * non-smart pointer code and largely allows smart pointers to be used as a drop-in
	 * replacement for regular pointers.
	 * 
	 * @param	s		Pointer to object to point to
	 */
	void	operator=(T* ptr) { setRef(ptr); }
	
	/**
	 * Dereference operator
	 * 
	 * This operator allows code to access the object being pointed to by a smart pointer
	 * using exactly the same dereferencing syntax of a regular pointer, once again making
	 * smart pointers largely a drop-in replacement for regular pointers with minimal code
	 * changes.
	 * 
	 * @return			Pointer to object being pointed to
	 */
	T*		operator->() const { return m_ptr; }
	
	/**
	 * Gets a pointer to the target object for this smart pointer
	 * 
	 * Gets the "regular pointer" version of this smart pointer - essentially a regular
	 * pointer straight to the target object without using any reference counting
	 * functionality.
	 * 
	 * @return			Pointer to object being pointed to
	 */
	T*		get() const { return m_ptr; }
	
	/**
	 * Smart pointer equality operator
	 * 
	 * Checks to see if this smart pointer points to the same class instance as another smart pointer.
	 * 
	 * @param	p		Smart pointer to check for equality
	 */
	bool	operator==(const sptr<T>& p) const { return m_ptr == p.m_ptr; }
	
	/**
	 * Smart pointer inequality operator
	 * 
	 * Checks to see if this smart pointer does not point to the same class instance as another smart pointer.
	 * 
	 * @param	p		Smart pointer to check for inequality
	 */
	bool	operator!=(const sptr<T>& p) const { return m_ptr != p.m_ptr; }
	
	/**
	 * Regular pointer equality operator
	 * 
	 * Checks to see if this smart pointer points to the same class instance as a regular pointer.
	 * 
	 * @param	p		Pointer to check for equality
	 */
	bool	operator==(T* p) const { return m_ptr == p; }
	
	/**
	 * Regular pointer inequality operator
	 * 
	 * Checks to see if this smart pointer does not point to the same class instance as a regular pointer.
	 * 
	 * @param	p		Pointer to check for inequality
	 */
	bool	operator!=(T* p) const { return m_ptr != p; }

private:
	
	/**
	 * Point to a different object
	 * 
	 * Switches the smart pointer to point to a different object.  Since we're now
	 * going to be pointing to a different object, we need to tell the old object that
	 * the old pointer will not be pointing to it anymore and that if there are no
	 * other pointers to it, it can clean itself up.  We also need to tell the new
	 * target object that this pointer will now be pointing to it, so don't clean up
	 * until we're no longer pointing to it.
	 * 
	 * @param	ptr		New regular pointer to point to
	 */
	void setRef(T* ptr)
	{
		if (ptr != m_ptr)
		{
			if (m_ptr)
			{
				m_ptr->deref();
			}
			m_ptr = ptr;

			if (m_ptr)
			{
				m_ptr->ref();
			}
		}
	}

	/**
	 * Pointer to the smart pointer's target object
	 */
	T* m_ptr;
};

#if 0
// Usage
class MyClass : public RefCounted {

	int array[100];
};

void func(sptr<MyClass> abc) {
	abc->array[10] = 20;
}

int main() {
	sptr<MyClass> a = new MyClass;
	func(a);
}

#endif // 0

#endif /* SPTR_H */
