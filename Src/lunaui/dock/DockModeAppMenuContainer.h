/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#ifndef DOCKMODEMENUCONTAINER_H
#define DOCKMODEMENUCONTAINER_H

#include "Common.h"
#include "CustomEvents.h"
#include "PtrArray.h"
#include <QFont>
#include <QGraphicsObject>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <stdint.h>
#include <QDebug>

class StatusBar;
class DockModeMenuItem;
class DockModeMenuManager;
class DockModeLaunchPoint;

class DockModeAppMenuContainer : public QGraphicsObject
{
	Q_OBJECT

public:
	DockModeAppMenuContainer (DockModeMenuManager* parent, int width, int height);
	~DockModeAppMenuContainer();

	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) { }

	void addMenuItem (DockModeLaunchPoint* dlp);
	void removeMenuItem (DockModeLaunchPoint* dlp);
	void resize (int screenWidth, int screenHeight);

	virtual void mousePressEvent (QGraphicsSceneMouseEvent* mouseEvent);
	virtual void mouseMoveEvent (QGraphicsSceneMouseEvent* mouseEvent);
	virtual void mouseReleaseEvent (QGraphicsSceneMouseEvent* mouseEvent);

	const QFont font() const { return m_font; }

	Q_INVOKABLE const int getWidth() const { return m_bounds.width(); }
	Q_INVOKABLE const int getHeight() const { return m_bounds.height(); }

	Q_INVOKABLE int getMaximumHeightForMenu() const { return m_maxHeight; }
	Q_INVOKABLE void mouseWasGrabbedByParent();

	void appSelected (DockModeLaunchPoint* dlp);

	QPixmap m_menuItemBgSelected;
	QPixmap m_menuItemSeparator;

Q_SIGNALS:
	void signalDockModeAppSelected (DockModeLaunchPoint* dlp);
	void signalContainerSizeChanged();

private:
	QFont   m_font;
	qreal   m_menuHeight;
	qreal   m_maxHeight;
	QRectF  m_bounds;

	bool    m_penDown;
	QPointF m_penDownPos;
	int 	m_currentItemIndex;

	PtrArray<DockModeMenuItem> m_menuItems;
	std::map<DockModeLaunchPoint*, DockModeMenuItem*> m_menuItemMap;

};

#endif
