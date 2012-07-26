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




#ifndef PERSISTENTWINDOWCACHE_H
#define PERSISTENTWINDOWCACHE_H

#include "Common.h"

#include <set>
#include <memory>
#include <string>
#include "Window.h"

class PersistentWindowCache
{
public:

	static PersistentWindowCache* instance();

	bool shouldPersistWindow(Window* w);
	bool addWindow(Window* w);
	bool removeWindow(Window* w);
	bool hasWindow(Window* w);

	void showWindow(Window* w);
	bool hideWindow(Window* w);

	std::set<Window*>* getCachedWindows() { return &m_windowCache; }

private:

	PersistentWindowCache();
	~PersistentWindowCache();

private:

	typedef std::pair<std::string, std::string> AppWindowNamePair;

	std::set<Window*> m_windowCache;
	std::set<AppWindowNamePair> m_persistableWindowIdentifiers;
};

#endif /* PERSISTENTWINDOWCACHE_H */
