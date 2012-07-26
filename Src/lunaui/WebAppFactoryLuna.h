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




#ifndef WEBAPPFACTORYLUNA_H
#define WEBAPPFACTORYLUNA_H

#include "Common.h"

#include "WebAppFactory.h"

class WebAppFactoryLuna : public WebAppFactory
{
public:

	WebAppFactoryLuna();
	virtual ~WebAppFactoryLuna() {}

	virtual WebAppBase* createWebApp(Window::Type winType, PIpcChannel* channel, ApplicationDescription* desc = 0);
	virtual WebAppBase* createWebApp(Window::Type winType, SysMgrWebBridge* page, PIpcChannel* channel, ApplicationDescription* desc = 0);
	virtual WebAppBase* createWebApp(Window::Type winType, int width, int height, PIpcChannel* channel, ApplicationDescription* desc = 0);

private:
	bool m_dashboardOwnsNegativeSpace;

};	

#endif /* WEBAPPFACTORYLUNA_H */
