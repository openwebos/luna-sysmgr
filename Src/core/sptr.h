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
 * Implements reference counting for objects
 * 
 * Keeps track of how many references there are to an object and deletes
 * the object when that number gets to zero.
 */
class RefCounted
{
public:

	/**
	 * Initializes the reference count to zero
	 */
	RefCounted() : m_refCount(0) {}
	
	/**
	 * Cleans up the reference count
	 */
	virtual ~RefCounted() {}

	/**
	 * Increments the reference count
	 * 
	 * Calling of this method denotes that there is a new reference to this object somewhere in memory.
	 */
	void ref()
	{
		g_atomic_int_inc(&m_refCount);
	}

	/**
	 * Decrements the reference count and cleans up the object if there are no more references
	 * 
	 * Calling of this method denotes that a previously-used reference to this object will no longer be used.
	 */
	void deref()
	{
		if (g_atomic_int_dec_and_test(&m_refCount)) {
			delete this;
		}
	}

private:

	/**
	 * Number of open references to this object
	 */
	gint m_refCount;

	/**
	 * Block the copy constructor from being used publicly
	 */
	RefCounted(const RefCounted&);
	
	/**
	 * Block the copy operator from being used publicly
	 */
	RefCounted& operator=(const RefCounted&);
};
	

/**
 * Smart pointer object
 * 
 * @todo Limit this class to only being used on derivatives of RefCounted.
 * 
 * A smart (strong) pointer asserts that the pointed-to object will
 * not go away as long as the strong pointer is valid.  "Owners" of an
 * object should keep strong pointers; other objects should use a
 * strong pointer temporarily while they are actively using the
 * object, to prevent the object from being deleted.
 */
template<class T>
class sptr
{
public:

	/**
	 * Initializes a reference counted object and increments its reference count
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
	 */
	sptr() : m_ptr(0) {}

	/**
	 * Copy another smart pointer and increment the target object's reference count
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
	 * Clean up smart pointer and decrement the target object's reference count
	 */
	~sptr()
	{
		if (m_ptr)
		{
			m_ptr->deref();
		}
	}

	/**
	 * Copy operator - copies from another smart pointer
	 * 
	 * @param	s		Smart pointer to copy
	 */
	void	operator=(const sptr<T>& s) { setRef(s.m_ptr); }
	
	/**
	 * Copy operator - switches smart pointer to point to a new object
	 * 
	 * @param	s		Pointer to object to point to
	void	operator=(T* ptr) { setRef(ptr); }
	
	/**
	 * Dereference operator - allows caller to access object pointed to by this smart pointer
	 * 
	 * @return			Pointer to object being pointed to
	 */
	T*		operator->() const { return m_ptr; }
	
	/**
	 * Gets a pointer to the target object for this smart pointer
	 * 
	 * @return			Pointer to object being pointed to
	 */
	T*		get() const { return m_ptr; }
	
	/**
	 * Smart pointer equality operator
	 * 
	 * Checks to see if this smart pointer points to the same class instance as another smart pointer
	 * 
	 * @param	p		Smart pointer to check for equality
	 */
	bool	operator==(const sptr<T>& p) const { return m_ptr == p.m_ptr; }
	
	/**
	 * Inequality operator
	 * 
	 * Checks to see if this smart pointer does not point to the same class instance as another smart pointer
	 * 
	 * @param	p		Smart pointer to check for inequality
	 */
	bool	operator!=(const sptr<T>& p) const { return m_ptr != p.m_ptr; }
	
	/**
	 * Regular pointer equality operator
	 * 
	 * Checks to see if this smart pointer points to the same class instance as a regular pointer
	 * 
	 * @param	p		Pointer to check for equality
	 */
	bool	operator==(T* p) const { return m_ptr == p; }
	
	/**
	 * Regular pointer inequality operator
	 * 
	 * Checks to see if this smart pointer does not point to the same class instance as a regular pointer
	 * 
	 * @param	p		Pointer to check for inequality
	 */
	bool	operator!=(T* p) const { return m_ptr != p; }

private:
	
	/**
	 * Point to a different object
	 * 
	 * If the new object to point to is different than the current object, decrement
	 * the old object's reference count and increment the new one's.
	 * 
	 * @param	ptr		New pointer to point to
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
	 * Pointer to the object this smart pointer should point to
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
