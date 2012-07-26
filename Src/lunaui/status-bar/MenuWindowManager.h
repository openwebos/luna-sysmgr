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




#ifndef MENUWINDOWMANAGER_H
#define MENUWINDOWMANAGER_H

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

class MenuWindowManager : public WindowManagerBase
{
	Q_OBJECT

public:

	MenuWindowManager(int maxWidth, int maxHeight);
	virtual ~MenuWindowManager();

	virtual void init();

	bool showingMenu() const { return !m_winArray.empty(); }

	QGraphicsPixmapItem** cornerWindows() { return m_corners; }

	void showOrHideRoundedCorners(bool show);

	void resize(int width, int height);

public Q_SLOTS:

	void closeMenu();

protected:

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	void flickGestureEvent(QGestureEvent* event);

	virtual bool sceneEvent(QEvent* event);
																   
private Q_SLOTS:
	
	void slotPositiveSpaceChanged(const QRect& r);
	void slotSystemMenuStateChanged(bool opened);
	void slotCloseSystemMenu();

private:

	void handleMousePress(QGraphicsSceneMouseEvent* event, int clickCount);

	virtual void addWindow(Window* win);
	virtual void removeWindow(Window* win);
	virtual void focusWindow(Window* win);
	virtual void unfocusWindow(Window* win);
	
	void positionCornerWindows(const QRect& r);
	void mapCoordToWindow(Window* win, int& x, int& y);
	void showMenuWindow(Window* win);
	void hideMenuWindow(Window* win);

	void raiseWindow(Window* win);
private:

	enum PenDownState {
		PenDownInvalid,
		PenDownInMenu,
	};

	GraphicsItemContainer* m_cornerContainer;
	QGraphicsPixmapItem* m_corners[4];
	StatusBar* m_statusBar;
	QVector<Window*> m_winArray;
	QRect m_positiveSpace;
	PenDownState m_penDownState;
	int m_penDownX;
	int m_penDownY;

	SystemMenu* m_sysMenu;
	bool m_systemMenuOpened;
};

#endif /* MENUWINDOWMANAGER_H */
