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




#include "Common.h"

#include <QGesture>
#include <QCoreApplication>
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>

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
#include "Localization.h"
#include "WindowServerLuna.h"
#include "DockModeMenuManager.h"
#include "DockModeAppMenuContainer.h"
#include "DockModeWindowManager.h"
#include "DockModeLaunchPoint.h"
#include "StatusBar.h"

static const int kTopLeftWindowIndex     = 0;
static const int kTopRightWindowIndex    = 1;
static const int kBottomLeftWindowIndex  = 2;
static const int kBottomRightWindowIndex = 3;
static int kStatusBarTapMoveTolerance    = 0;
static int kAppMenuWidth    = 320;

DockModeMenuManager::DockModeMenuManager(int maxWidth, int maxHeight)
	: WindowManagerBase(maxWidth, maxHeight)
	, m_statusBar(0)
	, m_sysMenu(0)
	, m_systemMenuOpened(false)
	, m_appMenuOpened (false)
{
	setObjectName("DockModeMenuManager");

	connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceChanged(const QRect&)), 
			this, SLOT(slotPositiveSpaceChanged(const QRect&)));
	connect(SystemUiController::instance(), SIGNAL(signalHideMenu()), this, SLOT(closeAllMenus()));

	kStatusBarTapMoveTolerance = HostBase::instance()->getInfo().displayHeight;

	grabGesture(Qt::TapGesture);
	grabGesture((Qt::GestureType) SysMgrGestureFlick);
	grabGesture((Qt::GestureType) SysMgrGestureSingleClick);
	grabGesture((Qt::GestureType) Qt::PinchGesture);
	grabGesture(Qt::TapAndHoldGesture);
}

DockModeMenuManager::~DockModeMenuManager()
{
    if(m_statusBar)
    	delete m_statusBar;

    if(m_sysMenu)
    	delete m_sysMenu;
}

void DockModeMenuManager::init()
{
	QDeclarativeEngine* qmlEngine = WindowServer::instance()->declarativeEngine();
	if(qmlEngine) {
		QDeclarativeContext* context =	qmlEngine->rootContext();
		m_appMenuContainer = new DockModeAppMenuContainer(this, kAppMenuWidth, 0);
		if(context) {
			context->setContextProperty("DockModeAppMenuContainer", m_appMenuContainer);
		}

		Settings* settings = Settings::LunaSettings();
		std::string systemMenuQmlPath = settings->lunaQmlUiComponentsPath + "DockModeAppMenu/DockModeAppMenu.qml";
		QUrl url = QUrl::fromLocalFile(systemMenuQmlPath.c_str());
		m_qmlNotifMenu = new QDeclarativeComponent(qmlEngine, url, this);
		if(m_qmlNotifMenu) {
			m_menuObject = qobject_cast<QGraphicsObject *>(m_qmlNotifMenu->create());
			if(m_menuObject) {
				int offset = m_menuObject->property("edgeOffset").toInt();
				m_menuObject->setPos (boundingRect().x() - offset, boundingRect().y() + Settings::LunaSettings()->positiveSpaceTopPadding);
				m_menuObject->setZValue (100);
				m_menuObject->setParentItem(this);
				QMetaObject::invokeMethod(m_menuObject, "setMaximumHeight", Q_ARG(QVariant, m_appMenuContainer->getMaximumHeightForMenu()));
			}
		}
		connect (m_appMenuContainer, SIGNAL(signalDockModeAppSelected(DockModeLaunchPoint*)),
				this, SLOT(slotDockModeAppSelected(DockModeLaunchPoint*)));
	}

	m_statusBar = new StatusBar(StatusBar::TypeDockMode, boundingRect().width(), Settings::LunaSettings()->positiveSpaceTopPadding);
	if(m_statusBar) {
		m_statusBar->setParentItem(this);
		connect(m_statusBar, SIGNAL(signalSystemMenuStateChanged(bool)), this, SLOT(activateSystemMenu(bool)));
		connect(m_statusBar, SIGNAL(signalDockModeMenuStateChanged(bool)), this, SLOT(activateAppMenu(bool)));
		m_statusBar->setPos(0, boundingRect().y() + m_statusBar->boundingRect().height() / 2);
		m_statusBar->setZValue (100);
		m_statusBar->init();
	}

	m_sysMenu = new SystemMenu(320, 480, true);
	if(m_sysMenu) {
		m_sysMenu->setParentItem(this);
		connect(m_sysMenu, SIGNAL(signalCloseMenu()), this, SLOT(slotCloseSystemMenu()));
		m_sysMenu->init();
		m_sysMenu->setZValue (100);
		m_statusBar->setSystemMenuObject(m_sysMenu);

		int offset = m_sysMenu->getRightEdgeOffset();
		m_sysMenu->setPos(boundingRect().x() + boundingRect().width() - m_sysMenu->boundingRect().width()/2 + offset,
				          boundingRect().y() + m_statusBar->boundingRect().height() + m_sysMenu->boundingRect().height()/2);
	}

	DockModeWindowManager* dmwm = (DockModeWindowManager*)(((WindowServerLuna*)WindowServer::instance())->dockModeManager());
	if (dmwm) {
		connect (dmwm, SIGNAL (signalDockModeAppChanged(DockModeLaunchPoint*)), this, SLOT (slotDockModeAppChanged(DockModeLaunchPoint*)));
		connect (dmwm, SIGNAL (signalDockModeStatusChanged(bool)), this, SLOT (slotDockModeStatusChanged(bool)));
	}

	closeAllMenus();
}

void DockModeMenuManager::resize(int width, int height)
{
	WindowManagerBase::resize(width, height);

	if(m_statusBar && m_sysMenu) {
		int offset = m_sysMenu->getRightEdgeOffset();
		m_sysMenu->setPos(boundingRect().x() + boundingRect().width() - m_sysMenu->boundingRect().width()/2 + offset,
				          boundingRect().y() + m_statusBar->boundingRect().height() + m_sysMenu->boundingRect().height()/2);
	}

	if (m_menuObject) {
		int offset = m_menuObject->property("edgeOffset").toInt();
		m_menuObject->setPos (boundingRect().x() - offset, boundingRect().y() + Settings::LunaSettings()->positiveSpaceTopPadding);
	}

	if(m_statusBar) {
		m_statusBar->resize(width, Settings::LunaSettings()->positiveSpaceTopPadding);
		m_statusBar->setPos(boundingRect().x(), boundingRect().y() + m_statusBar->boundingRect().height() / 2);
		m_statusBar->update();
	}
}

void DockModeMenuManager::activateAppMenu(bool open)
{
	if(!m_statusBar)
		return;

	if(m_systemMenuOpened) {
		m_systemMenuOpened = false;
		m_statusBar->setSystemMenuOpen(false);
		m_sysMenu->setOpened(m_systemMenuOpened);
		SystemUiController::instance()->setMenuVisible(false);
	}

	m_appMenuOpened = open;
	m_statusBar->setDockModeAppMenuOpen (open);
	SystemUiController::instance()->setMenuVisible (open);

	if (open) {
		m_statusBar->setMaximizedAppTitle (true, LOCALIZED("Choose an App").c_str());
	}
	else {
		m_statusBar->setMaximizedAppTitle (true, m_currentApp.c_str());
	}
}

void DockModeMenuManager::activateSystemMenu (bool open)
{
	if(!m_sysMenu || !m_statusBar)
		return;

	if(m_appMenuOpened) {
		m_appMenuOpened = false;
		m_statusBar->setDockModeAppMenuOpen(false);
		m_statusBar->setMaximizedAppTitle (true, m_currentApp.c_str());
		SystemUiController::instance()->setMenuVisible(false);
	}

	m_systemMenuOpened = open;

	m_sysMenu->setOpened(m_systemMenuOpened);

	SystemUiController::instance()->setMenuVisible(m_systemMenuOpened);
	m_statusBar->setSystemMenuOpen(m_systemMenuOpened);
}

void DockModeMenuManager::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	handleMousePress(event, 1);
}

void DockModeMenuManager::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	handleMousePress(event, 2);
}

void DockModeMenuManager::handleMousePress(QGraphicsSceneMouseEvent* event, int clickCount)
{
	if(m_systemMenuOpened || m_appMenuOpened) {
		// we only get the event in the WM if the mouse press was outside the System Menu object
		event->accept();
		return; // ignore the mouse press and close the menu on the mouse up
	}

	event->ignore();

}

void DockModeMenuManager::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	if(m_systemMenuOpened || m_appMenuOpened) {
		return; // ignore the mouse moves
	}

}

void DockModeMenuManager::flickGestureEvent(QGestureEvent* event)
{
	if(m_systemMenuOpened || m_appMenuOpened) {
		return; // ignore gestures
	}
}

void DockModeMenuManager::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if(m_systemMenuOpened || m_appMenuOpened) {
		// close menu
		closeAllMenus();
		return;
	}
}


void DockModeMenuManager::slotPositiveSpaceChanged(const QRect& r)
{
	if (m_statusBar)
		setPosTopLeft(m_statusBar, 0, r.y() - m_statusBar->boundingRect().height());

	update();
}

void DockModeMenuManager::closeAllMenus()
{
	activateAppMenu (false);
	activateSystemMenu (false);
}

bool DockModeMenuManager::sceneEvent(QEvent* event)
{
	switch (event->type()) {
	case QEvent::GestureOverride: {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QList<QGesture*> activeGestures = ge->activeGestures();
		Q_FOREACH(QGesture* g, activeGestures) {
			if (g->hasHotSpot()) {
				QPointF pt = ge->mapToGraphicsScene(g->hotSpot());
				if (m_systemMenuOpened) {
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


void DockModeMenuManager::slotCloseSystemMenu()
{
	activateSystemMenu(false);
}

void DockModeMenuManager::slotDockModeAppSelected(DockModeLaunchPoint* lp)
{
	closeAllMenus();
}

void DockModeMenuManager::slotDockModeAppChanged (DockModeLaunchPoint* lp)
{
	if (lp) 
		m_currentApp = lp->launchPoint()->menuName();
}

void DockModeMenuManager::slotDockModeStatusChanged (bool enabled)
{
	if (!enabled)
		closeAllMenus();
}
