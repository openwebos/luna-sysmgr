/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef __COMMON_H__
#define __COMMON_H__

// #define OVERRIDE_NEW to override the default implementation of new.  Doing
// so will allow the heap analysis tool to figure out who called new.
#undef OVERRIDE_NEW
#ifdef OVERRIDE_NEW

#include <new>
#include <cstring>
#include <cstdlib>

#define COMMON_PRIVATE_INLINE inline __attribute__((always_inline))
COMMON_PRIVATE_INLINE void* operator new(size_t size) { return malloc(size); }
COMMON_PRIVATE_INLINE void* operator new(size_t size, const std::nothrow_t&) throw() { return malloc(size); }
COMMON_PRIVATE_INLINE void operator delete(void* p) { free(p); }
COMMON_PRIVATE_INLINE void operator delete(void* p, const std::nothrow_t&) throw() { free(p); }
COMMON_PRIVATE_INLINE void* operator new[](size_t size) { return malloc(size); }
COMMON_PRIVATE_INLINE void* operator new[](size_t size, const std::nothrow_t&) throw() { return malloc(size); }
COMMON_PRIVATE_INLINE void operator delete[](void* p) { free(p); }
COMMON_PRIVATE_INLINE void operator delete[](void* p, const std::nothrow_t&) throw() { free(p); }
#endif

#endif // __COMMON_H__
