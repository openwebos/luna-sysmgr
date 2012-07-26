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




/* ============================================================
 * Code derived from smart_ptr.h by Thatcher Ulrich, 2003.
 * Original copyright follows:
 *
 * smart_ptr.h  -- by Thatcher Ulrich <tu@tulrich.com> 2003
 *
 * This source code has been donated to the Public Domain.  Do
 * whatever you want with it.
 * ============================================================ */

#ifndef SPTR_H
#define SPTR_H

#include "Common.h"

#include <glib.h>

class RefCounted
{
public:

	RefCounted() : m_refCount(0) {}
	virtual ~RefCounted() {}

	void ref()
	{
		g_atomic_int_inc(&m_refCount);
	}

	void deref()
	{
		if (g_atomic_int_dec_and_test(&m_refCount)) {
			delete this;
		}
	}

private:

	gint m_refCount;

	RefCounted(const RefCounted&);
	RefCounted& operator=(const RefCounted&);
};
	

// A smart (strong) pointer asserts that the pointed-to object will
// not go away as long as the strong pointer is valid.  "Owners" of an
// object should keep strong pointers; other objects should use a
// strong pointer temporarily while they are actively using the
// object, to prevent the object from being deleted.
template<class T>
class sptr
{
public:

	sptr(T* ptr) : m_ptr(ptr)
	{
		if (m_ptr)
		{
			m_ptr->ref();
		}
	}

	sptr() : m_ptr(0) {}

	sptr(const sptr<T>& s) : m_ptr(s.m_ptr)
	{
		if (m_ptr)
		{
			m_ptr->ref();
		}
	}

	~sptr()
	{
		if (m_ptr)
		{
			m_ptr->deref();
		}
	}

	void	operator=(const sptr<T>& s) { setRef(s.m_ptr); }
	void	operator=(T* ptr) { setRef(ptr); }
	T*		operator->() const { return m_ptr; }
	T*		get() const { return m_ptr; }
	bool	operator==(const sptr<T>& p) const { return m_ptr == p.m_ptr; }
	bool	operator!=(const sptr<T>& p) const { return m_ptr != p.m_ptr; }
	bool	operator==(T* p) const { return m_ptr == p; }
	bool	operator!=(T* p) const { return m_ptr != p; }

private:
	
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
