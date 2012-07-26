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




#ifndef NEWCONTENTINDICATOREVENTFACTORY_H
#define NEWCONTENTINDICATOREVENTFACTORY_H

#include "Common.h"

#include <NewContentIndicatorEvent.h>

#include <glib.h>
#include <time.h>

#include <string>

class NewContentIndicatorEventFactory
{
public:

	static NewContentIndicatorEvent* createAddEvent(const std::string& _appId) {

		if (_appId.empty())
			return 0;

		NewContentIndicatorEvent* e = new NewContentIndicatorEvent;
		e->eventType = NewContentIndicatorEvent::Add;
		e->appId = _appId;

		// Create a "unique" ID for the msg
		struct timespec time;
		clock_gettime(CLOCK_MONOTONIC, &time);
		double t = time.tv_sec * 1000.0 + time.tv_nsec / 1000000.0;

		char* id = g_strdup_printf("%lf", t);
		e->requestId = id;
		g_free(id);

		return e;
	}

	static NewContentIndicatorEvent* createRemoveEvent(const std::string& _appId, const std::string& _requestId) {

		if (_appId.empty() || _requestId.empty())
			return 0;

		NewContentIndicatorEvent* e = new NewContentIndicatorEvent;
		e->eventType = NewContentIndicatorEvent::Remove;
		e->appId = _appId;
		e->requestId = _requestId;

		return e;
	}

	static NewContentIndicatorEvent* createClearEvent(const std::string& _appId) {

		if (_appId.empty())
			return 0;

		NewContentIndicatorEvent* e = new NewContentIndicatorEvent;
		e->eventType = NewContentIndicatorEvent::Clear;
		e->appId = _appId;

		return e;
	}		
};

#endif /* NEWCONTENTINDICATOREVENTFACTORY_H */
