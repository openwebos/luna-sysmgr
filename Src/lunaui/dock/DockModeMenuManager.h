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




#ifndef DOCKMODEMENUMANAGER_H
#define DOCKMODEMENUMANAGER_H

#include "Common.h"

#include "WindowManagerBase.h"

#include "GraphicsItemContainer.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QVector>
#include "SystemMenu.h"

class Window;
class QGestureEvent;
class StatusBar;
class DockModeAppMenuContainer;
class DockModeLaunchPoint;
class DockModeWindowManager;

class DockModeMenuManager : public WindowManagerBase
{
	Q_OBJECT

public:

	DockModeMenuManager(int maxWidth, int maxHeight);
	virtual ~DockModeMenuManager();

	virtual void init();

	void resize(int width, int height);

	DockModeAppMenuContainer* dockAppContainer() const { return m_appMenuContainer; }
	QGraphicsObject* getAppMenu() const { return m_menuObject; }
	StatusBar* statusBar() const { return m_statusBar; }
	bool isAppMenuOpen() const { return m_appMenuOpened; }

public Q_SLOTS:

	void activateAppMenu(bool open);
	void activateSystemMenu (bool open);
	void closeAllMenus();
	void slotCloseSystemMenu();
	void slotDockModeAppSelected(DockModeLaunchPoint*);
	void slotDockModeAppChanged (DockModeLaunchPoint*);
	void slotDockModeStatusChanged (bool enabled);

protected:

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	void flickGestureEvent(QGestureEvent* event);

	virtual bool sceneEvent(QEvent* event);
																   
private Q_SLOTS:
	
	void slotPositiveSpaceChanged(const QRect& r);
	// void slotSystemMenuStateChanged(bool opened);

private:

	void handleMousePress(QGraphicsSceneMouseEvent* event, int clickCount);

private:

	enum PenDownState {
		PenDownInvalid,
		PenDownInMenu,
	};

	StatusBar* m_statusBar;
	bool m_appMenuOpened;

	SystemMenu* m_sysMenu;
	bool m_systemMenuOpened;

	DockModeAppMenuContainer* m_appMenuContainer;
	QDeclarativeComponent* m_qmlNotifMenu;
	QGraphicsObject* m_menuObject;

	std::string m_currentApp;
};

#endif /* MENUWINDOWMANAGER_H */
