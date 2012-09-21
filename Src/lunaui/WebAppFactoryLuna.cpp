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

#include "WebAppFactoryLuna.h"

#include "AlertWebApp.h"
#include "CardWebApp.h"
#include "DockWebApp.h"
#include "DashboardWebApp.h"
#include "Settings.h"
#include "SysMgrWebBridge.h"
#include "WebAppBase.h"
#include "WebAppManager.h"
#include <PIpcChannel.h>
#include <QDebug>
static const int kTableUiAlertWidth = 320;

WebAppFactoryLuna::WebAppFactoryLuna()
{
	// If we are running on a tablet, update to use a different height
	m_dashboardOwnsNegativeSpace = (Settings::LunaSettings()->virtualKeyboardEnabled ? false : (Settings::LunaSettings()->showNotificationsAtTop ? false : true));
}

WebAppBase* WebAppFactoryLuna::createWebApp(Window::Type winType, PIpcChannel* channel, ApplicationDescription* desc)
{
	WebAppBase* app = 0;
	
	switch (winType) {
	case (Window::Type_Card):
	case (Window::Type_ChildCard):
	case (Window::Type_PIN):
	case (Window::Type_Emergency):
	case (Window::Type_ModalChildWindowCard):
		app = new CardWebApp(winType, channel, desc);
		break;
	case (Window::Type_DockModeWindow):
		app = new DockWebApp(winType, channel);
		break;
	case (Window::Type_Dashboard):
		app = new DashboardWebApp(channel);
		break;
	case (Window::Type_Menu): {
		app = new WindowedWebApp(WebAppManager::instance()->currentUiWidth(),
								 WebAppManager::instance()->currentUiHeight() -
								 Settings::LunaSettings()->positiveSpaceTopPadding,
								 winType, channel);
		break;
	}
	case (Window::Type_Overlay):
	case (Window::Type_Launcher):
		app = new WindowedWebApp(WebAppManager::instance()->currentUiWidth(),
				                 WebAppManager::instance()->currentUiHeight(),
								 winType, channel);
		break;
	case (Window::Type_StatusBar):
		app = new WindowedWebApp(WebAppManager::instance()->currentUiWidth(),
								 Settings::LunaSettings()->positiveSpaceTopPadding,
								 winType, channel);
		break;
	case (Window::Type_None):
		app = new WebAppBase();
		break;
	case (Window::Type_PopupAlert):
		app = new AlertWebApp(QString(), -1, -1, winType, channel);
		break;
	default:
		g_warning("%s: unsupported window type: %d", __PRETTY_FUNCTION__, winType);
		break;
	}

	return app;    
}

WebAppBase* WebAppFactoryLuna::createWebApp(Window::Type winType, SysMgrWebBridge* page, PIpcChannel* channel, ApplicationDescription* desc)
{
    WebAppBase* app = 0;

    switch (winType) {
    case Window::Type_Card:
    case Window::Type_ChildCard:
    case Window::Type_PIN:
    case Window::Type_Emergency:
    case Window::Type_ModalChildWindowCard:
        app = new CardWebApp(winType, channel, desc);
        break;
    case Window::Type_DockModeWindow:
        app = new DockWebApp(winType, channel);
        break;
    case Window::Type_Dashboard:
        app = new DashboardWebApp(channel);
        break;
    case Window::Type_PopupAlert:
    case Window::Type_BannerAlert:
        {
        QRect reqGeom = page->requestedGeometry();
        if (m_dashboardOwnsNegativeSpace)
            app = new AlertWebApp(page->appId(), reqGeom.width(), reqGeom.height(), winType, channel);
        else
            app = new AlertWebApp(page->appId(), kTableUiAlertWidth, reqGeom.height(), winType, channel);
        break;
        }
    case Window::Type_Menu:
        app = new WindowedWebApp(WebAppManager::instance()->currentUiWidth(),
                WebAppManager::instance()->currentUiHeight() -
                Settings::LunaSettings()->positiveSpaceTopPadding,
                winType, channel);
        break;
    case Window::Type_Overlay:
        app = new WindowedWebApp(WebAppManager::instance()->currentUiWidth(),
                WebAppManager::instance()->currentUiHeight(),
                winType, channel);
        break;
    case Window::Type_StatusBar:
        app = new WindowedWebApp(WebAppManager::instance()->currentUiWidth(),
                Settings::LunaSettings()->positiveSpaceTopPadding,
                winType, channel);
        break;
    default:
        g_warning("%s: unsupported window type: %d", __PRETTY_FUNCTION__, winType);
        break;
    }
    return app;
}

WebAppBase* WebAppFactoryLuna::createWebApp(Window::Type winType, int width, int height, PIpcChannel* channel, ApplicationDescription* desc)
{
	WebAppBase* app = 0;

	switch (winType) {
	case (Window::Type_PopupAlert):
		if(true == m_dashboardOwnsNegativeSpace)
			app = new AlertWebApp(QString(), WebAppManager::instance()->currentUiWidth(), height, winType, channel);
		else
			app = new AlertWebApp(QString(), kTableUiAlertWidth, height, winType, channel);
		break;
	default:
		app = createWebApp(winType, channel, desc);
		break;
	}

	return app;    
    
}
