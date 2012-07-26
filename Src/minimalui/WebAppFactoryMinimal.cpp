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

#include "WebAppFactoryMinimal.h"

#include "AlertWebApp.h"
#include "CardWebApp.h"
#include "Settings.h"
#include "WebAppBase.h"
#include "SysMgrWebBridge.h"
#include "WindowedWebApp.h"
#include "WebAppManager.h"
#include <PIpcChannel.h>

WebAppBase* WebAppFactoryMinimal::createWebApp(Window::Type winType, PIpcChannel* channel, ApplicationDescription* desc)
{
	WebAppBase* app = 0;
	
	const HostInfo& info = WebAppManager::instance()->getHostInfo();

	switch (winType) {
	case (Window::Type_Card):
	case (Window::Type_ChildCard):
		app = new CardWebApp(winType, channel, desc);
		break;
	case (Window::Type_StatusBar):
		app = new WindowedWebApp(info.displayWidth,
								 Settings::LunaSettings()->positiveSpaceTopPadding,
								 winType, channel);
		break;
	case (Window::Type_Overlay):
	case (Window::Type_Launcher):
	case (Window::Type_Menu):
		app = new WindowedWebApp(info.displayWidth,
								 info.displayHeight,
								 winType, channel);
		break;
	case (Window::Type_None):
		app = new WebAppBase();
		break;
	case (Window::Type_Dashboard):
	default:
		g_warning("unsupported window type: %d", winType);
		break;
	}

	return app;    
}

WebAppBase* WebAppFactoryMinimal::createWebApp(Window::Type winType, SysMgrWebBridge* page, PIpcChannel* channel, ApplicationDescription* desc)
{
	WebAppBase* app = 0;
	
	const HostInfo& info = WebAppManager::instance()->getHostInfo();

	switch (winType) {
	case (Window::Type_Card):
	case (Window::Type_ChildCard):
		app = new CardWebApp(winType, channel, desc);
		break;
	case (Window::Type_StatusBar):
		app = new WindowedWebApp(info.displayWidth,
								 Settings::LunaSettings()->positiveSpaceTopPadding,
								 winType, channel);
		break;
	case (Window::Type_Overlay):
	case (Window::Type_Launcher):
	case (Window::Type_Menu):
		app = new WindowedWebApp(info.displayWidth,
								 info.displayHeight,
								 winType, channel);
		break;
	case (Window::Type_None):
		app = new WebAppBase();
		break;
	case (Window::Type_Dashboard):
	default:
		g_warning("unsupported window type: %d", winType);
		break;
	}

	return app;        
}

WebAppBase* WebAppFactoryMinimal::createWebApp(Window::Type winType, int width, int height, PIpcChannel* channel, ApplicationDescription* desc)
{
	WebAppBase* app = 0;

	switch (winType) {
	default:
		app = createWebApp(winType, channel, desc);
		break;
	}

	return app;
}

