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

#include "WebAppFactory.h"

#include "Settings.h"
#include "WebAppFactoryMinimal.h"
#include "WebAppFactoryLuna.h"
#include "Window.h"

WebAppFactory* WebAppFactory::instance()
{
	static WebAppFactory* s_instance = 0;
	if (!s_instance) {
		switch (Settings::LunaSettings()->uiType) {
		case (Settings::UI_MINIMAL):
			s_instance = new WebAppFactoryMinimal;
			break;
		case (Settings::UI_LUNA):
		default:
			s_instance = new WebAppFactoryLuna;
			break;
		}
	}
		
	return s_instance;   
}

WebAppFactory::WebAppFactory()
{   
}

WebAppFactory::~WebAppFactory()
{    
}

QString WebAppFactory::nameForWindowType(Window::Type winType)
{
    switch (winType) {
	case Window::Type_StatusBar:
		return "statusbar";
	case Window::Type_ChildCard:
		return "childcard";
	case Window::Type_Card:
		return "card";
	case Window::Type_Launcher:
		return "launcher";
	case Window::Type_Dashboard:
		return "dashboard";
	case Window::Type_PopupAlert:
		return "popupalert";
	case Window::Type_BannerAlert:
		return "menu";
	case Window::Type_PIN:
		return "pin";
	case Window::Type_Emergency:
		return "emergency";
	case Window::Type_None:
		return "headless";
	case Window::Type_ModalChildWindowCard:
		return "modalChildWindowCard";
	default:
		return "unknown";
	}
}
