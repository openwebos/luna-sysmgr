/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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




#include "Common.h"

#include <malloc.h>

// We'll use LD_PRELOAD to use the malloc implementation of our choice.

#if 0
//#if defined(TARGET_DEVICE)

static void  initHook(void);
static void  freeHook(void*, const void*);
static void* mallocHook(size_t, const void*);
static void* reallocHook(void*, size_t, const void*);
static void* memalignHook(size_t, size_t, const void*);

extern "C" void* je_malloc(size_t size);
extern "C" void  je_free(void* ptr);
extern "C" void* je_realloc(void* ptr, size_t size);
extern "C" void* je_memalign(size_t boundary, size_t size);

void (*__malloc_initialize_hook) (void) = initHook;

static void
initHook(void)
{
	__free_hook = freeHook;
    __malloc_hook = mallocHook;
    __realloc_hook = reallocHook;
    __memalign_hook = memalignHook;
}

static void
freeHook(void* ptr, const void*)
{
	je_free(ptr);
}

static void*
mallocHook(size_t size, const void*)
{
	return je_malloc(size);
}

static void*
reallocHook(void* ptr, size_t size, const void*)
{
	return je_realloc(ptr, size);
}

static void*
memalignHook(size_t alignment, size_t size, const void*)
{
	return je_memalign(alignment, size);
}

#endif
