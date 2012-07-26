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




#include "Common.h"

#include "Mutex.h"

Mutex::Mutex()
{
	m_mutex = g_new0(GStaticRecMutex, 1);
	g_static_rec_mutex_init(m_mutex);	
}

Mutex::~Mutex()
{
    g_static_rec_mutex_free(m_mutex);
	g_free(m_mutex);
}

void Mutex::lock()
{
	g_static_rec_mutex_lock(m_mutex);
}

bool Mutex::tryLock()
{
	return g_static_rec_mutex_trylock(m_mutex);    
}

void Mutex::unlock()
{
	g_static_rec_mutex_unlock(m_mutex);    
}
