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

#include "DashboardWebApp.h"

#include <algorithm>
#include <PIpcBuffer.h>
#include <PIpcChannel.h>
#include "IpcClientHost.h"
#include "WebAppManager.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

static const int kDashboardWindowHeight = 52;

static bool stringIsTrue(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	return s == "true";
}

DashboardWebApp::DashboardWebApp(PIpcChannel *channel)
	: WindowedWebApp(0, 0, Window::Type_Dashboard, channel)
{
	m_width = WebAppManager::instance()->currentUiWidth();
	m_height = kDashboardWindowHeight;
	
	m_windowWidth =  m_width;
	m_windowHeight = m_height;

	init();
}

DashboardWebApp::~DashboardWebApp()
{
    
}

void DashboardWebApp::attach(SysMgrWebBridge* page)
{
	WindowedWebApp::attach(page);

	const StringVariantMap& stageArgs = page->stageArguments();

	StringVariantMap::const_iterator it = stageArgs.find("icon");
	if (it != stageArgs.end()) {

		m_channel->sendAsyncMessage(new ViewHost_Dashboard_SetIcon(routingId(), it.value().toString().toStdString()));
	}

	it = stageArgs.find("clickablewhenlocked");
	if (it == stageArgs.end())
		it = stageArgs.find("clickableWhenLocked");

	if (it != stageArgs.end()) {

		QVariant v = it.value();
		if (v.type() == QVariant::Bool)
			m_channel->sendAsyncMessage(new ViewHost_Dashboard_SetClickableWhenLocked(routingId(), v.toBool()));
		else if (v.type() == QVariant::String)
			m_channel->sendAsyncMessage(new ViewHost_Dashboard_SetClickableWhenLocked(routingId(), stringIsTrue(v.toString().toStdString())));
	}

	// Restricted to systemui
	if (page->appId() == "com.palm.systemui") {

		it = stageArgs.find("persistent");
		if (it != stageArgs.end()) {

			QVariant v = it.value();
			if (v.type() == QVariant::Bool)
				m_channel->sendAsyncMessage(new ViewHost_Dashboard_SetPersistent(routingId(), v.toBool()));	
			else if (v.type() == QVariant::String)
				m_channel->sendAsyncMessage(new ViewHost_Dashboard_SetPersistent(routingId(), stringIsTrue(v.toString().toStdString())));	
		}
	}

	it = stageArgs.find("webosdragmode");
	if (it != stageArgs.end()) {

		QVariant v = it.value();
		if (v.type() == QVariant::String && v.toString() == "manual") {
			WindowProperties prop;
			prop.setDashboardManualDragMode(true);
			setWindowProperties(prop);
		}
	}
}
