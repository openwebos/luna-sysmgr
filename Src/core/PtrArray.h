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




#ifndef PTRARRAY_H
#define PTRARRAY_H

#include "Common.h"

#include <glib.h>

template<class T>
class PtrArray
{
public:

	PtrArray() {
		m_array = g_ptr_array_new();
	}

	PtrArray(int initialSize) {
		m_array = g_ptr_array_sized_new(initialSize);
	}

	~PtrArray() {
		g_ptr_array_free(m_array, TRUE);
	}

	PtrArray(const PtrArray<T>& other) {
		m_array = g_ptr_array_sized_new(other.m_array->len);
		for (unsigned int i = 0; i < other.m_array->len; i++) {
			g_ptr_array_add(m_array, g_ptr_array_index(other.m_array, i));
		}
	}

	PtrArray<T>& operator=(const PtrArray<T>& other) {
		if (this != &other) {
			if (m_array)
				g_ptr_array_free(m_array, TRUE);
			m_array = g_ptr_array_sized_new(other.m_array->len);
			for (unsigned int i = 0; i < other.m_array->len; i++) {
				g_ptr_array_add(m_array, g_ptr_array_index(other.m_array, i));
			}
		}
		return *this;
	}
	
	inline void append(T* t) {
		g_ptr_array_add(m_array, t);
	}

	inline bool remove(T* t) {
		return g_ptr_array_remove(m_array, t);
	}

	void addAfter(T* t, T* newT) {

		if (empty() || !contains(t)) {
			append(newT);
			return;
		}

		GPtrArray* oldArray = m_array;
		m_array = g_ptr_array_sized_new(oldArray->len + 1);

		for (unsigned int i = 0; i < oldArray->len; i++) {
			g_ptr_array_add(m_array, g_ptr_array_index(oldArray, i));
			if (g_ptr_array_index(oldArray, i) == t)
				g_ptr_array_add(m_array, newT);
		}

		g_ptr_array_free(oldArray, TRUE);
		
	}

	void addBefore(T* t, T* newT) {

		if (empty() || !contains(t)) {
			append(newT);
			return;
		}

		GPtrArray* oldArray = m_array;
		m_array = g_ptr_array_sized_new(oldArray->len + 1);

		for (unsigned int i = 0; i < oldArray->len; i++) {
			if (g_ptr_array_index(oldArray, i) == t)
				g_ptr_array_add(m_array, newT);
			g_ptr_array_add(m_array, g_ptr_array_index(oldArray, i));
		}

		g_ptr_array_free(oldArray, TRUE);		
	}
	
	inline bool empty() const {
		return m_array->len == 0;
	}

	inline int size() const {
		return m_array->len;
	}

	inline void clear() {
		g_ptr_array_free(m_array, TRUE);
		m_array = g_ptr_array_new();
	}

	inline T* operator[](int i) const {
//		if (i < 0 || i >= m_array->len)
//			return 0;
		return (T*) g_ptr_array_index(m_array, i);
	}

	inline T*& operator[](int i) {
		return (T*&) g_ptr_array_index(m_array, i);
	}

	inline int position(T* t) const {
		for (unsigned int i = 0; i < m_array->len; i++) {
			if (g_ptr_array_index(m_array, i) == t)
				return i;
		}

		return -1;
	}

	inline bool contains(T* t) const {
		for (unsigned int i = 0; i < m_array->len; i++) {
			if (g_ptr_array_index(m_array, i) == t)
				return true;
		}

		return false;
	}
		

	inline T* first() const {
		if (m_array->len)
			return (T*) g_ptr_array_index(m_array, 0);
		return 0;
	}

	inline T* last() const {
		if (m_array->len)
			return (T*) g_ptr_array_index(m_array, m_array->len - 1);
		return 0;
	}
	
private:

	GPtrArray* m_array;
};	

#endif /* PTRARRAY_H */
