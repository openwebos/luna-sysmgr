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




#ifndef WINDOWMANAGERBASE_H
#define WINDOWMANAGERBASE_H

#include "Common.h"

#include <QGraphicsItem>
#include <QPainter>
#include "UiNavigationController.h"

class Window;

//need Q_OBJECT macro for metatype info
class WindowManagerBase : public QGraphicsObject,
                          public KeyNavigator
{
	Q_OBJECT
public:

	WindowManagerBase(int maxWidth, int maxHeight);
	virtual ~WindowManagerBase();

	virtual void init() {}

	void setScreenBounds(int x, int y, int w, int h);
	virtual bool okToResize() { return true; }
	virtual void resize(int width, int height);
	virtual bool doReticle(QPointF pos)					{ return true; }

	virtual QRectF boundingRect() const;
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	virtual void addWindow(Window* win);
	virtual void prepareAddWindow(Window* win);
	virtual void addWindowTimedOut(Window* win);
	virtual void removeWindow(Window* win);
	virtual void focusWindow(Window* win);
	virtual void unfocusWindow(Window* win);

    virtual bool handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate);

protected:
	void raiseChild(QGraphicsItem* child);
	QRectF m_boundingRect;
};

#endif /* WINDOWMANAGERBASE_H */
