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

#include "MenuWindowManager.h"

#include <QGesture>

#include <SysMgrDefs.h>

#include "HostBase.h"
#include "PersistentWindowCache.h"
#include "RoundedCorners.h"
#include "Settings.h"
#include "SystemUiController.h"
#include "WebAppMgrProxy.h"
#include "Window.h"
#include "WindowServer.h"
#include "Utils.h"
#include "Time.h"
#include "SingleClickGesture.h"
#include "FlickGesture.h"
#include "HostWindow.h"
#include "MenuWindow.h"
#include "StatusBar.h"

static const int kTopLeftWindowIndex     = 0;
static const int kTopRightWindowIndex    = 1;
static const int kBottomLeftWindowIndex  = 2;
static const int kBottomRightWindowIndex = 3;
static int kStatusBarTapMoveTolerance    = 0;

MenuWindowManager::MenuWindowManager(int maxWidth, int maxHeight)
	: WindowManagerBase(maxWidth, maxHeight)
	, m_statusBar(0)
	, m_penDownState(PenDownInvalid)
	, m_cornerContainer(0)
	, m_sysMenu(0)
	, m_systemMenuOpened(false)
{
	setObjectName("MenuWindowManager");

	connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceChanged(const QRect&)), 
			this, SLOT(slotPositiveSpaceChanged(const QRect&)));
	connect(SystemUiController::instance(), SIGNAL(signalHideMenu()), this, SLOT(closeMenu()));
	connect(SystemUiController::instance(), SIGNAL(signalCardWindowMaximized()), this, SLOT(closeMenu()));

	kStatusBarTapMoveTolerance = HostBase::instance()->getInfo().displayHeight;

	grabGesture(Qt::TapGesture);
	grabGesture((Qt::GestureType) SysMgrGestureFlick);
	grabGesture((Qt::GestureType) SysMgrGestureSingleClick);
	grabGesture((Qt::GestureType) Qt::PinchGesture);
	grabGesture(Qt::TapAndHoldGesture);

	m_statusBar = new StatusBar(StatusBar::TypeNormal, maxWidth, Settings::LunaSettings()->positiveSpaceTopPadding);
	if(m_statusBar) {
		SystemUiController::instance()->setStatusBar(m_statusBar);
		connect(m_statusBar, SIGNAL(signalSystemMenuStateChanged(bool)), this, SLOT(slotSystemMenuStateChanged(bool)));
	}

	m_sysMenu = new SystemMenu(320, 480, false);
	if(m_sysMenu) {
		m_sysMenu->setParentItem(this);
		connect(m_sysMenu, SIGNAL(signalCloseMenu()), this, SLOT(slotCloseSystemMenu()));
	}
}

MenuWindowManager::~MenuWindowManager()
{
    if(m_statusBar)
    	delete m_statusBar;

    if(m_sysMenu)
    	delete m_sysMenu;
}

void MenuWindowManager::init()
{
	if(m_statusBar) {
		m_statusBar->init();
		m_statusBar->setParentItem(this);
		m_statusBar->setPos(0, m_boundingRect.y() + m_statusBar->boundingRect().height() / 2);
		m_statusBar->setZValue(100);
	}

	if(m_statusBar && m_sysMenu) {
		int offset = 0;
		m_sysMenu->init();
		offset = m_sysMenu->getRightEdgeOffset();
		m_statusBar->setSystemMenuObject(m_sysMenu);
		m_sysMenu->setPos(boundingRect().x() + boundingRect().width() - m_sysMenu->boundingRect().width()/2 + offset,
				          boundingRect().y() + m_statusBar->boundingRect().height() + m_sysMenu->boundingRect().height()/2);
	}

	if(!Settings::LunaSettings()->tabletUi) {
		m_cornerContainer = new GraphicsItemContainer(m_boundingRect.width(),
													  m_boundingRect.height());

		QSize dims = RoundedCorners::topLeft().size();

		m_corners[kTopLeftWindowIndex] = new QGraphicsPixmapItem(RoundedCorners::topLeft(), m_cornerContainer);
		m_corners[kTopRightWindowIndex] = new QGraphicsPixmapItem(RoundedCorners::topRight(), m_cornerContainer);
		m_corners[kBottomLeftWindowIndex] = new QGraphicsPixmapItem(RoundedCorners::bottomLeft(), m_cornerContainer);
		m_corners[kBottomRightWindowIndex] = new QGraphicsPixmapItem(RoundedCorners::bottomRight(), m_cornerContainer);

		for (int i=kTopLeftWindowIndex; i <= kBottomRightWindowIndex; i++) {
			m_corners[i]->setOffset(-dims.width()/2, -dims.height()/2);

			// disable mouse events for the corners
			m_corners[i]->setAcceptedMouseButtons(0);
		}

		m_cornerContainer->setParentItem(this);

		positionCornerWindows(m_positiveSpace);
	}
}

void MenuWindowManager::resize(int width, int height)
{
	WindowManagerBase::resize(width, height);

	if(m_cornerContainer)
		m_cornerContainer->resize(width, height);

	if(m_statusBar && m_sysMenu) {
		int offset = m_sysMenu->getRightEdgeOffset();
		m_sysMenu->setPos(boundingRect().x() + boundingRect().width() - m_sysMenu->boundingRect().width()/2 + offset,
				          boundingRect().y() + m_statusBar->boundingRect().height() + m_sysMenu->boundingRect().height()/2);
	}

	if(m_statusBar) {
		m_statusBar->resize(width, Settings::LunaSettings()->positiveSpaceTopPadding);
		m_statusBar->update();
	}

	for(int i = 0; i < m_winArray.size(); i++){
		((MenuWindow*)m_winArray[i])->resizeEventSync(width, height);
	}

	// resize all cached Menu Windows

	PersistentWindowCache* persistCache =  PersistentWindowCache::instance();

	std::set<Window*>* cachedWindows = persistCache->getCachedWindows();
	if(cachedWindows) {
		std::set<Window*>::iterator it;

		it=cachedWindows->begin();

		for ( it=cachedWindows->begin() ; it != cachedWindows->end(); it++ ) {
			Window* w = *it;
			if(w->type() == Window::Type_Menu) {
				((MenuWindow*)w)->resizeEventSync(width, height - Settings::LunaSettings()->positiveSpaceTopPadding);
			}
		}
	}
}

void MenuWindowManager::addWindow(Window* win)
{
	if (m_winArray.contains(win))
		return;

	if (win->type() == Window::Type_StatusBar)
		return;

	win->setParentItem(this);

	m_winArray.append(win);

	hideMenuWindow(win);
}

void MenuWindowManager::removeWindow(Window* win)
{
	if (win->parent() != this) {
		delete win;
		return;
	}

	PersistentWindowCache::instance()->removeWindow(win);

	win->setParentItem(0);
	int removeIndex = m_winArray.indexOf(win);
	if (removeIndex != -1)
		m_winArray.remove(removeIndex);
	delete win;

	SystemUiController::instance()->setMenuVisible(false);	
}

void MenuWindowManager::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	handleMousePress(event, 1);
}

void MenuWindowManager::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	handleMousePress(event, 2);
}

void MenuWindowManager::handleMousePress(QGraphicsSceneMouseEvent* event, int clickCount)
{
	if(m_systemMenuOpened) {
		// we only get the event in the WM if the mouse press was outside the System Menu object
		event->accept();
		return; // ignore the mouse press and close the menu on the mouse up
	}

	event->ignore();

	m_penDownState = PenDownInvalid;

	Window* targetWin = 0;
	QPointF pos = event->scenePos();

	if (!m_winArray.empty() && m_positiveSpace.contains(event->pos().x() - boundingRect().x(), event->pos().y() - boundingRect().y())) {

		m_penDownState = PenDownInMenu;
		targetWin = m_winArray.first();
	}

	if (targetWin) {
		Event ev;
		ev.type = Event::PenDown;
		ev.setMainFinger(true);
		ev.x = pos.x();
		ev.y = pos.y();
		ev.clickCount = clickCount;
		ev.modifiers = Event::modifiersFromQt(event->modifiers());
		ev.time = Time::curSysTimeMs();
		mapCoordToWindow(targetWin, ev.x, ev.y);
		WebAppMgrProxy::instance()->inputEvent(targetWin, &ev);

		event->accept();
	}
}

void MenuWindowManager::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	if(m_systemMenuOpened) {
		return; // ignore the mouse moves
	}

	if (!m_winArray.empty()) {
		Window* targetWin = m_winArray.first();
		QPointF pos = event->scenePos();

		Event ev;
		ev.type = Event::PenMove;
		ev.setMainFinger(true);
		ev.x = pos.x();
		ev.y = pos.y();
		ev.time = Time::curSysTimeMs();
		ev.modifiers = Event::modifiersFromQt(event->modifiers());
		mapCoordToWindow(targetWin, ev.x, ev.y);
		WebAppMgrProxy::instance()->inputEvent(targetWin, &ev);
	}
}

void MenuWindowManager::flickGestureEvent(QGestureEvent* event)
{
	if(m_systemMenuOpened) {
		return; // ignore gestures
	}

	if (m_penDownState != PenDownInMenu) {
		return;
	}

	if (!m_winArray.empty()) {
		QGesture* g = event->gesture((Qt::GestureType) SysMgrGestureFlick);
		Q_ASSERT(g != 0);
		FlickGesture* flick = static_cast<FlickGesture*>(g);

		Window* targetWin = m_winArray.first();
		QPointF pos = mapFromScene(event->mapToGraphicsScene(flick->hotSpot()));

		Event ev;
		ev.type = Event::PenFlick;
		ev.setMainFinger(true);
		ev.x = pos.x();
		ev.y = pos.y();
		ev.flickXVel = flick->velocity().x();
		ev.flickYVel = flick->velocity().y();
		ev.time = Time::curSysTimeMs();
		mapCoordToWindow(targetWin, ev.x, ev.y);
		WebAppMgrProxy::instance()->inputEvent(targetWin, &ev);
	}
}

void MenuWindowManager::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if(m_systemMenuOpened) {
		// close the system menu
		closeMenu();
		return;
	}

	Window* targetWin = 0;
	QPointF pos = event->scenePos();
	Event ev;

	if(!event->canceled())
		ev.type = Event::PenUp;
	else
		ev.type = Event::PenCancel;

	ev.x = pos.x();
	ev.y = pos.y();
	ev.modifiers = Event::modifiersFromQt(event->modifiers());
	ev.setMainFinger(true);
	ev.clickCount = 0;
	ev.time = Time::curSysTimeMs();

	if (m_penDownState == PenDownInMenu && !m_winArray.empty()) {

		targetWin = m_winArray.first();
	}

	m_penDownState = PenDownInvalid;

	if (targetWin) {
		mapCoordToWindow(targetWin, ev.x, ev.y);
		WebAppMgrProxy::instance()->inputEvent(targetWin, &ev);
	}
}

void MenuWindowManager::positionCornerWindows(const QRect& r)
{
	if(!Settings::LunaSettings()->tabletUi) {
		int i = kTopLeftWindowIndex;
		int trueBottom = r.y() + r.height();
		int trueRight = r.x() + r.width();
		QRectF rect = m_corners[i]->boundingRect();

		setPosTopLeft(m_corners[i], r.x(), r.y());
	
		i = kTopRightWindowIndex;
		setPosTopLeft(m_corners[i], trueRight - rect.width(), r.y());

		i = kBottomLeftWindowIndex;
		setPosTopLeft(m_corners[i], r.x(), trueBottom - rect.height());
	
		i = kBottomRightWindowIndex;
		setPosTopLeft(m_corners[i], trueRight - rect.width(), trueBottom - rect.height());
	}
}

void MenuWindowManager::slotPositiveSpaceChanged(const QRect& r)
{
	m_positiveSpace = r;
	
	positionCornerWindows(r);

	Q_FOREACH(Window* w, m_winArray) {
		if (w)
			setPosTopLeft(w, r.x(), r.y());
	}

	if (m_statusBar) {
		setPosTopLeft(m_statusBar, 0, r.y() - m_statusBar->boundingRect().height());
	}
	update();
}

void MenuWindowManager::mapCoordToWindow(Window* win, int& x, int& y)
{
	QRectF r = win->boundingRect();
	QPointF pos = win->mapFromScene(x, y);
	x = pos.x() - r.x();
	y = pos.y() - r.y();
}

void MenuWindowManager::closeMenu()
{
	if(m_systemMenuOpened) {
		m_systemMenuOpened = false;
		m_statusBar->setSystemMenuOpen(false);
		m_sysMenu->setOpened(m_systemMenuOpened);
		SystemUiController::instance()->setMenuVisible(false);
	}

	if (m_winArray.empty())
		return;

	Window* w = m_winArray[0];
	
	hideMenuWindow(w);
}

void MenuWindowManager::focusWindow(Window* win)
{
	PersistentWindowCache* persistCache = PersistentWindowCache::instance();
	if (!persistCache->shouldPersistWindow(win)) {
		return;
	}

	showMenuWindow(win);

	raiseWindow(win);
}

void MenuWindowManager::unfocusWindow(Window* win)
{
	PersistentWindowCache* persistCache = PersistentWindowCache::instance();
	if (!persistCache->shouldPersistWindow(win)) {
		return;
	}

	hideMenuWindow(win);
}

void MenuWindowManager::showMenuWindow(Window* win)
{
	if (!m_winArray.contains(win))
		m_winArray.append(win);

	PersistentWindowCache* persistCache = PersistentWindowCache::instance();
	if (persistCache->shouldPersistWindow(win)) {

		if (!persistCache->hasWindow(win))
			persistCache->addWindow(win);

		persistCache->showWindow(win);
	}

	raiseWindow(win);
	
	setPosTopLeft(win, m_positiveSpace.x(), m_positiveSpace.y());
	SystemUiController::instance()->setMenuVisible(true);

	update(m_positiveSpace);
}

void MenuWindowManager::hideMenuWindow(Window* win)
{

	PersistentWindowCache* persistCache = PersistentWindowCache::instance();
	if (!persistCache->shouldPersistWindow(win)) {
		WebAppMgrProxy::instance()->closeWindow(m_winArray.first());
		return;
	}

	if (!persistCache->hasWindow(win))
		persistCache->addWindow(win);

	if (!persistCache->hideWindow(win))
		return;

	int removeIndex = m_winArray.indexOf(win);
	if (removeIndex != -1)
		m_winArray.remove(removeIndex);

	SystemUiController::instance()->setMenuVisible(false);	

	update(m_positiveSpace);
}

void MenuWindowManager::showOrHideRoundedCorners(bool show)
{
	if(m_cornerContainer)
		m_cornerContainer->setVisible(show);
}

void MenuWindowManager::raiseWindow(Window* win)
{
	if(m_cornerContainer)
		win->stackBefore(m_cornerContainer);
}

bool MenuWindowManager::sceneEvent(QEvent* event)
{
	switch (event->type()) {
	case QEvent::GestureOverride: {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QList<QGesture*> activeGestures = ge->activeGestures();
		Q_FOREACH(QGesture* g, activeGestures) {
			if (g->hasHotSpot()) {
				QPointF pt = ge->mapToGraphicsScene(g->hotSpot());
				if (m_systemMenuOpened || (!m_winArray.empty() && m_positiveSpace.contains(pt.x() - boundingRect().x(), pt.y() - boundingRect().y()))) {
					ge->accept(g);
				}
				else {
					ge->ignore(g);
				}
			}
		}
		break;
	}
	case QEvent::Gesture: {

		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
		if (g && g->state() == Qt::GestureFinished) {
			flickGestureEvent(ge);
			return true;
		}
	}
	default:
		break;
	}

	return WindowManagerBase::sceneEvent(event);
}

void MenuWindowManager::slotSystemMenuStateChanged(bool opened)
{
	if(!m_sysMenu || !m_statusBar)
		return;
	m_systemMenuOpened = opened;

	m_sysMenu->setOpened(m_systemMenuOpened);

	SystemUiController::instance()->setMenuVisible(m_systemMenuOpened);
	m_statusBar->setSystemMenuOpen(m_systemMenuOpened);
}

void MenuWindowManager::slotCloseSystemMenu()
{
	slotSystemMenuStateChanged(false);
}
