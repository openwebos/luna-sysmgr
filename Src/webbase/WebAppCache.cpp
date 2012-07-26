/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#include "WebAppCache.h"

#include <list>
#include <algorithm>

#include "WebAppBase.h"

typedef std::list<WebAppBase*> WebAppCacheType;
static WebAppCacheType* s_cache = 0;

static WebAppCacheType* PrvCache()
{
	if (!s_cache)
		s_cache = new WebAppCacheType;
	return s_cache;
}

void WebAppCache::put(WebAppBase* app)
{
	WebAppCacheType* cache = PrvCache();
	
	WebAppCacheType::iterator it = std::find(cache->begin(), cache->end(), app);
	if (it != cache->end()) {
		WebAppBase* a = *it;
		if (a != app) {
			delete a;
		}

		cache->erase(it);
	}
	
	cache->push_back(app);
}

void WebAppCache::remove(WebAppBase* app)
{
	WebAppCacheType* cache = PrvCache();

	WebAppCacheType::iterator it = std::find(cache->begin(), cache->end(), app);
	if (it != cache->end()) {
		cache->erase(it);
		return;
	}
}

void WebAppCache::flush()
{
	WebAppCacheType* cache = PrvCache();
    
	while (!cache->empty()) {
		WebAppBase* a = *(cache->begin());
		cache->pop_front();
		delete a;
	}

}
