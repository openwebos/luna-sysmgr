/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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




#include "DockModeClock.h"
#include <cjson/json.h>
#include "HostBase.h"
#include "Settings.h"
#include "Timer.h"
#include "ClockWindow.h"
#include "Localization.h"
#include "DisplayManager.h"
#include "WindowServer.h"

#include <QGesture>
#include <QCoreApplication>
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>


//---------------------------------------- DockModeClockWindow ----------------------------------------------

class DockModeClockWindow : public DockModeWindow
{
public:
	DockModeClockWindow(const QPixmap&, DockModeWindowManager* dm, int zValue);
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) { }
	virtual void setMaximized (bool enable) { }

	bool sceneEvent (QEvent* event);
	void resizeEvent(int w, int h);
	void resizeEventSync(int w, int h);
	virtual void focusEvent (bool enable);
private:
	QGraphicsObject* m_clockObject;
	QDeclarativeComponent* m_qmlNotifMenu;
};

DockModeClockWindow::DockModeClockWindow(const QPixmap& pixmap, DockModeWindowManager* dm, int zValue)
	: DockModeWindow (Window::Type_DockModeWindow, pixmap)
{
	setFlag (QGraphicsItem::ItemHasNoContents);
	setBoundingRect (pixmap.width(), pixmap.height());

	setName (LOCALIZED("Time"));
	setAppId ("com.palm.dockmodetime");

	QDeclarativeEngine* qmlEngine = WindowServer::instance()->declarativeEngine();
	if(qmlEngine) {
		QDeclarativeContext* context =	qmlEngine->rootContext();
		Settings* settings = Settings::LunaSettings();
		std::string systemMenuQmlPath = settings->lunaQmlUiComponentsPath + "DockModeTime/Clocks.qml";
		QUrl url = QUrl::fromLocalFile(systemMenuQmlPath.c_str());
		m_qmlNotifMenu = new QDeclarativeComponent(qmlEngine, url, this);
		if(m_qmlNotifMenu) {
			m_clockObject = qobject_cast<QGraphicsObject *>(m_qmlNotifMenu->create());
			if(m_clockObject) {
				m_clockObject->setPos (boundingRect().x(), boundingRect().y());
				m_clockObject->setParentItem(this);
			}
		}
	}
}

bool DockModeClockWindow::sceneEvent (QEvent* event)
{
	event->accept();
	return true;
}


void DockModeClockWindow::resizeEvent (int w, int h)
{
	resizeEventSync (w, h);
}

void DockModeClockWindow::resizeEventSync (int w, int h)
{
	setBoundingRect (w, h);
}


void DockModeClockWindow::focusEvent (bool enable)
{
	if (enable)
		m_clockObject->setProperty ("mainTimerRunning", true);
	else 
		m_clockObject->setProperty ("mainTimerRunning", false);

	DockModeWindow::focusEvent (enable);

}


//--------------------------------------------- DockModeClock --------------------------------------------------

DockModeClock::DockModeClock (DockModeWindowManager* dm, int zValue)
	: m_launchPoint(0)
	, m_appDesc (0)
	, m_dockWindow (0)
{
	m_appDesc = ApplicationDescription::fromNativeDockApp (std::string ("com.palm.app.dockmodetime"),
			std::string (LOCALIZED("Time")),
			std::string ("1.0"),
			Settings::LunaSettings()->lunaSystemResourcesPath + std::string("/dockmode/time-icon-48x48.png"), // splashIcon
			std::string("Time"), // splashBackgroundName
			Settings::LunaSettings()->lunaSystemResourcesPath + std::string("/dockmode/time-icon-48x48.png"), // miniicon
			std::string("Palm"), 
			std::string("http://www.palm.com"), 
			std::string(LOCALIZED("Time")));

	if (!m_appDesc) {
		g_warning ("appDesc could not be created");
		return;
	}

	m_launchPoint = new LaunchPoint (m_appDesc,
			std::string ("com.palm.dockmodetime"),
			std::string ("com.palm.dockmodetime"),
			std::string (LOCALIZED("Time")),
			std::string (LOCALIZED("Time")),
			Settings::LunaSettings()->lunaSystemResourcesPath + std::string("/dockmode/time-icon-48x48.png"), // iconPath
			std::string(), // params
			false);

	const HostInfo& hostInfo = HostBase::instance()->getInfo();

	QPixmap dockPixmap (hostInfo.displayWidth, hostInfo.displayHeight); // this will be replaced with the background pixmap

	m_dockWindow = new DockModeClockWindow (dockPixmap, dm, zValue);
}


DockModeClock::~DockModeClock()
{
	if (m_launchPoint)
		delete m_launchPoint;

	if (m_dockWindow)
		delete m_dockWindow;

	if (m_appDesc)
		delete m_appDesc;
}

