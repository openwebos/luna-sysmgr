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

#include "WindowManagerBase.h"

#include "Window.h"
#include <QGraphicsScene>

WindowManagerBase::WindowManagerBase(int maxWidth, int maxHeight)
{
	m_boundingRect = QRectF(-maxWidth/2, -maxHeight/2, maxWidth, maxHeight);

	// This is an obsolete API, but needed to make sure child events don't
	// get handled at this layer. See: http://bugreports.qt.nokia.com/browse/QTBUG-6861
	setHandlesChildEvents(false);
	setFlag(QGraphicsItem::ItemHasNoContents, true);
}

WindowManagerBase::~WindowManagerBase()
{   
}

void WindowManagerBase::setScreenBounds(int x, int y, int w, int h)
{
	prepareGeometryChange();
	m_boundingRect = QRectF(-w/2, -h/2, w, h);
}

void WindowManagerBase::resize(int width, int height)
{
	setScreenBounds(-width/2, -height/2, width, height);
}

QRectF WindowManagerBase::boundingRect() const
{
	return m_boundingRect;
}

bool WindowManagerBase::handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate)
{
    propogate = true;
    return false;
}

void WindowManagerBase::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
}

void WindowManagerBase::addWindow(Window* win)
{
	if (win)
		win->setParentItem(this);
}

void WindowManagerBase::prepareAddWindow(Window* win)
{
}

void WindowManagerBase::addWindowTimedOut(Window* win)
{
}

void WindowManagerBase::removeWindow(Window* win)
{
	if (win) {
		win->setParentItem(0);
		delete win;
	}
}

void WindowManagerBase::focusWindow(Window* win)
{
    
}

void WindowManagerBase::unfocusWindow(Window* win)
{
    
}

void WindowManagerBase::raiseChild(QGraphicsItem* child)
{
	// need a better system for promoting a sibling instead of
	// 	remove/add to the scene
	bool restoreFocus = child->hasFocus();
	scene()->removeItem(child);
	child->setParentItem(this);
	if (restoreFocus)
		child->setFocus();
}

