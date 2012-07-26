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




#include "Common.h"

#include <set>
#include <glib.h>

#include "WebAppDeferredUpdateHandler.h"
#include "SysMgrWebBridge.h"
#include "WindowedWebApp.h"

typedef std::set<WindowedWebApp*> AppSet;

static AppSet s_registeredApps = AppSet();
static AppSet s_nonActiveApps = AppSet();
static WindowedWebApp* s_activeApp = 0;
static GSource* s_paintSource = 0;
static const int kPaintTimeOutSecs = 1;
static const int kActiveAppThrottle = 100;
static const int kInactiveAppThrottle = 5;
static const int kActiveAppMinTimerIntervalMs = 0;
static const int kInactiveAppMinTimerIntervalMs = 2000;

void WebAppDeferredUpdateHandler::registerApp(WindowedWebApp* app)
{
	//printf("%s: %p\n", __PRETTY_FUNCTION__, app);

	s_registeredApps.insert(app);

	if (s_activeApp && s_activeApp != app) {
		//printf("%s: %p (app direct rendering already active)\n", __PRETTY_FUNCTION__, app);
		s_nonActiveApps.insert(app);
		suspendApp(app);
	}
}

void WebAppDeferredUpdateHandler::unregisterApp(WindowedWebApp* app)
{
	//printf("%s: %p\n", __PRETTY_FUNCTION__, app);

	if (s_activeApp == app) {
		//printf("%s: %p (current direct rendering app unregistered)\n", __PRETTY_FUNCTION__, app);
		directRenderingInactive(app);
	}

    s_registeredApps.erase(app);
	s_nonActiveApps.erase(app);
}

void WebAppDeferredUpdateHandler::directRenderingActive(WindowedWebApp* app)
{
	if (s_registeredApps.find(app) == s_registeredApps.end())
		return;

	if (s_activeApp == app)
		return;

	//printf("%s: %p\n", __PRETTY_FUNCTION__, app);

	stopIdleUpdateTimer();

	s_activeApp = app;
	s_nonActiveApps.clear();
	s_nonActiveApps = s_registeredApps;
	s_nonActiveApps.erase(s_activeApp);

	resumeApp(s_activeApp);

	for (AppSet::const_iterator it = s_nonActiveApps.begin();
		 it != s_nonActiveApps.end(); ++it) {
		//printf("%s: suspending %p\n", __PRETTY_FUNCTION__, *it);
		suspendApp(*it);
	}
}

void WebAppDeferredUpdateHandler::directRenderingInactive(WindowedWebApp* app)
{
	if (s_registeredApps.find(app) == s_registeredApps.end())
		return;

    if (s_activeApp != app)
		return;

	//printf("%s: %p\n", __PRETTY_FUNCTION__, app);

	s_activeApp = 0;

	if (s_nonActiveApps.empty())
		return;

	for (AppSet::const_iterator it = s_nonActiveApps.begin();
		 it != s_nonActiveApps.end(); ++it) {
		//printf("%s: resuming %p\n", __PRETTY_FUNCTION__, *it);
		(*it)->resumeAppRendering();
	}

	startIdleUpdateTimer();
}

void WebAppDeferredUpdateHandler::startIdleUpdateTimer()
{
	if (s_paintSource)
		return;

	//printf("%s", __PRETTY_FUNCTION__);

	s_paintSource = g_timeout_source_new_seconds(kPaintTimeOutSecs);
	g_source_set_callback(s_paintSource, WebAppDeferredUpdateHandler::paintSourceCallback,
						  NULL, NULL);
	g_source_attach(s_paintSource, g_main_context_default());
}

void WebAppDeferredUpdateHandler::stopIdleUpdateTimer()
{
    if (!s_paintSource)
		return;

	//printf("%s", __PRETTY_FUNCTION__);
	g_source_destroy(s_paintSource);
	g_source_unref(s_paintSource);
	s_paintSource = 0;
}

gboolean WebAppDeferredUpdateHandler::paintSourceCallback(gpointer)
{
	//printf("%s", __PRETTY_FUNCTION__);

	if (s_nonActiveApps.empty()) {
		stopIdleUpdateTimer();
		return false;
	}

	AppSet::iterator it = s_nonActiveApps.begin();
	WindowedWebApp* app = *it;
	s_nonActiveApps.erase(it);

    app->paint();

	return true;
}

void WebAppDeferredUpdateHandler::suspendApp(WindowedWebApp* app)
{
	app->suspendAppRendering();
/*
	WebPage* page = app->page();
	if (page && page->webkitPage())
		page->webkitPage()->throttle(kInactiveAppThrottle, kInactiveAppMinTimerIntervalMs);
*/
}

void WebAppDeferredUpdateHandler::resumeApp(WindowedWebApp* app)
{
	s_activeApp->resumeAppRendering();
/*
	WebPage* page = app->page();
	if (page && page->webkitPage())
		page->webkitPage()->throttle(kActiveAppThrottle, kActiveAppMinTimerIntervalMs);
*/
}
