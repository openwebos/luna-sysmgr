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




#ifndef DASHBOARDWINDOW_H
#define DASHBOARDWINDOW_H

#include "Common.h"
#include "Event.h"

#include "HostWindow.h"

#include <QPixmap>

class PIpcMessage;

class DashboardWindow : public HostWindow
{
public:

	DashboardWindow(HostWindowData* data, IpcClientHost* clientHost);
	~DashboardWindow();

	void setIcon(const std::string& applicationPath, const std::string& iconName);
	QPixmap icon();

	void setClickableWhenLocked(bool val) { m_clickableWhenLocked = val; }
	bool clickableWhenLocked() const { return m_clickableWhenLocked; }

	void setPersistent(bool val) { m_persistent = val; }
	bool persistent() const { return m_persistent; }
	
	virtual void onMessageReceived(const PIpcMessage& msg);
	void onSetIcon(const std::string& iconName);

	void setWindowProperties (const WindowProperties& props);

	bool isManualDragWindow() { return m_manualDragMode; }
	virtual void inputEvent(Event* e);

private:

	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {}
	
	QPixmap m_icon;
	bool m_clickableWhenLocked;
	bool m_persistent;

	bool m_manualDragMode;
};	

#endif /* DASHBOARDWINDOW_H */
