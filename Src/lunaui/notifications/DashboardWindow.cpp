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

#include "DashboardWindow.h"

#include "Settings.h"
#include <PIpcBuffer.h>
#include <PIpcChannel.h>
#include "IpcClientHost.h"
#include "ApplicationDescription.h"
#include "WebAppMgrProxy.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>


DashboardWindow::DashboardWindow(HostWindowData* data, IpcClientHost* clientHost)
	: HostWindow(Window::Type_Dashboard, data, clientHost)
	, m_clickableWhenLocked(false)
	, m_persistent(false)
	, m_manualDragMode(false)
{
    
}

DashboardWindow::~DashboardWindow()
{
}


void DashboardWindow::onMessageReceived(const PIpcMessage& msg)
{
	bool msgIsOk;
	
	IPC_BEGIN_MESSAGE_MAP(DashboardWindow, msg, msgIsOk)
		IPC_MESSAGE_HANDLER(ViewHost_Dashboard_SetClickableWhenLocked, setClickableWhenLocked)
		IPC_MESSAGE_HANDLER(ViewHost_Dashboard_SetPersistent, setPersistent)
		IPC_MESSAGE_HANDLER(ViewHost_Dashboard_SetIcon, onSetIcon)
		// not handled here, forward it to the base class
		IPC_MESSAGE_UNHANDLED( HostWindow::onMessageReceived(msg); ) 
	IPC_END_MESSAGE_MAP()
}

void DashboardWindow::onSetIcon(const std::string& iconName)
{
	setIcon(appDescription()->folderPath(), iconName);
	
}

void DashboardWindow::setIcon(const std::string& applicationPath, const std::string& iconName)
{
	if (iconName.empty())
		return;

	m_icon = QPixmap();

	std::string iconPath;
	
	if (iconName.compare(0, 7, "file://") == 0) {
		iconPath = iconName;
		iconPath.erase(0, 7);
	}
	else if (g_path_is_absolute(iconName.c_str()))
		iconPath = iconName;
	else {
		if (applicationPath.at(applicationPath.size()-1) == '/')
			iconPath = applicationPath + iconName;
		else
			iconPath = applicationPath + "/"+ iconName;
	}
	
	m_icon = QPixmap(iconPath.c_str());
}

QPixmap DashboardWindow::icon()
{
	if (m_icon.isNull()) {
		if (appDescription())
			m_icon = appDescription()->miniIcon();
	}

	return m_icon;
}

void DashboardWindow::setWindowProperties(const WindowProperties& props)
{
	// we only care about the manual drag mode for Dashboard windows
	m_manualDragMode = props.dashboardManualDrag;
}

void DashboardWindow::inputEvent(Event* e)
{
	if (Event::isPenEvent(e) && !e->mainFinger())
		return;

	WebAppMgrProxy::instance()->inputEvent(this, e);
}

