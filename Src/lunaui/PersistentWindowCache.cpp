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

#include <glib.h>
#include <cjson/json.h>

#include "PersistentWindowCache.h"
#include "WebAppMgrProxy.h"
#include "Window.h"
#include "MenuWindow.h"

static const char* kPersistentWindowConf = "/etc/palm/persistentWindows.conf";

PersistentWindowCache* PersistentWindowCache::instance()
{
	static PersistentWindowCache* s_instance = 0;
	if (G_UNLIKELY(s_instance == 0))
		s_instance = new PersistentWindowCache();

	return s_instance;
}

PersistentWindowCache::PersistentWindowCache()
{
	json_object* json = json_object_from_file((char*) kPersistentWindowConf);
	if (!json || is_error(json))
		return;

	json_object_object_foreach(json, key, value) {
		int numItems = json_object_array_length(value);
		for (int i = 0; i < numItems; i++) {
			const char* val = json_object_get_string(json_object_array_get_idx(value, i));;;;
			g_message("Adding persistent window pair: appid: %s, windowname: %s",
				   key, val);
			AppWindowNamePair entry(key, val);
			m_persistableWindowIdentifiers.insert(entry);
		}
	}

	json_object_put(json);
}

PersistentWindowCache::~PersistentWindowCache()
{
    // NO-OP
}

bool PersistentWindowCache::shouldPersistWindow(Window* w)
{
	if (!w)
		return false;

	AppWindowNamePair id(w->appId(), w->name());
	if (m_persistableWindowIdentifiers.find(id) == m_persistableWindowIdentifiers.end())
		return false;

	return true;
}

bool PersistentWindowCache::addWindow(Window* w)
{
	if (!w || !shouldPersistWindow(w))
		return false;

	// Do we already have this window?
	if (m_windowCache.find(w) != m_windowCache.end())
		return false;

	m_windowCache.insert(w);

	return true;
}

bool PersistentWindowCache::removeWindow(Window* w)
{
	// Do we have this window
	if (!hasWindow(w))
		return false;

	m_windowCache.erase(w);

	return true;
}

bool PersistentWindowCache::hasWindow(Window* w)
{
	if (!w || !shouldPersistWindow(w))
		return false;

	if (m_windowCache.find(w) == m_windowCache.end())
		return false;	

	return true;
}

void PersistentWindowCache::showWindow(Window* w)
{
	if (!hasWindow(w))
		return;

	if (w->isVisible())
		return;

	w->setVisible(true);
	WebAppMgrProxy::instance()->focusEvent(w, true);
}

bool PersistentWindowCache::hideWindow(Window* w)
{
	if (!hasWindow(w))
		return false;

	// NOTE: the window can technically be "visible" even though its parent is currently not!
	if (!w->isVisible())
		return false;

	w->setVisible(false);
	WebAppMgrProxy::instance()->focusEvent(w, false);
	return true;
}
