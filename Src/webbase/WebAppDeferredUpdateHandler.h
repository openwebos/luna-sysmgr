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




#ifndef WEBAPPDEFERREDUPDATEHANDLER_H
#define WEBAPPDEFERREDUPDATEHANDLER_H

class WindowedWebApp;

class WebAppDeferredUpdateHandler
{
public:

	static void registerApp(WindowedWebApp* app);
	static void unregisterApp(WindowedWebApp* app);

	static void directRenderingActive(WindowedWebApp* app);
	static void directRenderingInactive(WindowedWebApp* app);

private:

	static void startIdleUpdateTimer();
	static void stopIdleUpdateTimer();
	static gboolean paintSourceCallback(gpointer);
	static void suspendApp(WindowedWebApp* app);
	static void resumeApp(WindowedWebApp* app);
};


#endif /* WEBAPPDEFERREDUPDATEHANDLER_H */
