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

#include <glib.h>
#include <QGraphicsSceneMouseEvent>
#include <QStateMachine>
#include <QState>
#include <QDeclarativeEngine>
#include <QDeclarativeComponent>
#include <QDeclarativeContext>

#include "DashboardWindowManager.h"
#include "AlertWindow.h"
#include "BannerWindow.h"
#include "DashboardWindow.h"
#include "DashboardWindowContainer.h"
#include "DashboardWindowManagerStates.h"
#include "CoreNaviManager.h"
#include "DisplayManager.h"
#include "GraphicsItemContainer.h"
#include "HostBase.h"
#include "Logging.h"
#include "NewContentIndicatorEvent.h"
#include "NotificationPolicy.h"
#include "PersistentWindowCache.h"
#include "Settings.h"
#include "SystemUiController.h"
#include "Time.h"
#include "WebAppMgrProxy.h"
#include "Window.h"
#include "WindowServer.h"
#include "WindowServerLuna.h"
#include "Preferences.h"
#include "Utils.h"
#include "FlickGesture.h"
#include "DockModeWindowManager.h"

#include <QGraphicsPixmapItem>

static const int kTabletAlertWindowPadding       = 5;
static const int kTabletNotificationContentWidth = 320;

DashboardWindowManager::DashboardWindowManager(int maxWidth, int maxHeight)
	: WindowManagerBase(maxWidth, maxHeight)
	,m_stateMachine(0)
	,m_stateDashboardOpen(0)
	,m_stateAlertOpen(0)
	,m_stateClosed(0)
	,m_stateCurrent(0)
	,m_bannerWin(0)
	,m_bgItem(0)
	,m_alertWinContainer(0)
	,m_transientContainer(0)
	,m_dashboardWinContainer(0)
	,m_dashboardRightOffset(0)
	,m_qmlNotifMenu(0)
	,m_menuObject(0)
    ,m_notifMenuRightEdgeOffset(0)
	,m_activeTransientAlert(0)
{
	setObjectName("DashboardWindowManager");

	m_notificationPolicy = new NotificationPolicy();
	m_bannerHasContent = false;
	m_AlertWindowFadeOption = Invalid;
	m_deleteWinAfterAnimation = NULL;
	m_deleteTransientWinAfterAnimation = false;
	m_previousActiveAlertWindow = NULL;
	m_inDockModeAnimation = false;

	// grab all gestures handled by the scenes viewport widget so
	// we can prevent them from being propogated to items below the dashboard
	grabGesture(Qt::TapGesture);
	grabGesture(Qt::TapAndHoldGesture);
	grabGesture(Qt::PinchGesture);
	grabGesture((Qt::GestureType) SysMgrGestureFlick);
	grabGesture((Qt::GestureType) SysMgrGestureSingleClick);

	SystemUiController* suc = SystemUiController::instance();
	m_isOverlay = !suc->dashboardOwnsNegativeSpace();

	// Connect to this signal only if we own the negative space.
	if(!m_isOverlay) {
		connect(suc, SIGNAL(signalNegativeSpaceChanged(const QRect&)),
				this, SLOT(slotNegativeSpaceChanged(const QRect&)));
		connect(suc, SIGNAL(signalNegativeSpaceChangeFinished(const QRect&)),
				this, SLOT(slotNegativeSpaceChangeFinished(const QRect&)));
	}
	else {
		connect(suc, SIGNAL(signalPositiveSpaceChanged(const QRect&)),
				this, SLOT(slotPositiveSpaceChanged(const QRect&)));
		connect(suc, SIGNAL(signalPositiveSpaceChangeFinished(const QRect&)),
				this, SLOT(slotPositiveSpaceChangeFinished(const QRect&)));

		setFlag(QGraphicsItem::ItemHasNoContents, true);
	}

	connect(SystemUiController::instance(), SIGNAL(signalCloseDashboard(bool)),
	                       this, SLOT(slotCloseDashboard(bool)));
	connect(SystemUiController::instance(), SIGNAL(signalOpenDashboard()),
	                       this, SLOT(slotOpenDashboard()));
	connect(SystemUiController::instance(), SIGNAL(signalCloseAlert()),
	                       this, SLOT(slotCloseAlert()));
}

DashboardWindowManager::~DashboardWindowManager()
{
}

void DashboardWindowManager::init()
{
	int width = boundingRect().toRect().width();
	int height = boundingRect().toRect().height();

	if(m_isOverlay) {
		QDeclarativeEngine* qmlEngine = WindowServer::instance()->declarativeEngine();

		 if(qmlEngine) {
			 QDeclarativeContext* context =	qmlEngine->rootContext();
			 m_dashboardWinContainer = new DashboardWindowContainer(this, kTabletNotificationContentWidth, 0);
			 if(context) {
				 context->setContextProperty("DashboardContainer", m_dashboardWinContainer);
			 }

			 Settings* settings = Settings::LunaSettings();
			 std::string systemMenuQmlPath = settings->lunaQmlUiComponentsPath + "DashboardMenu/DashboardMenu.qml";
			 QUrl url = QUrl::fromLocalFile(systemMenuQmlPath.c_str());
			 m_qmlNotifMenu = new QDeclarativeComponent(qmlEngine, url, this);
			 if(m_qmlNotifMenu) {
				 m_menuObject = qobject_cast<QGraphicsObject *>(m_qmlNotifMenu->create());
				 if(m_menuObject) {
					 m_menuObject->setParentItem(this);
					 m_notifMenuRightEdgeOffset = m_menuObject->property("edgeOffset").toInt();

					 QMetaObject::invokeMethod(m_menuObject, "setMaximumHeight", Q_ARG(QVariant, m_dashboardWinContainer->getMaximumHeightForMenu()));
				 }
			 }
		 }
	}

	if(!m_isOverlay)
	{
		m_dashboardWinContainer = new DashboardWindowContainer(this, width, height);
		m_alertWinContainer = new GraphicsItemContainer(width, height, GraphicsItemContainer::SolidRectBackground);
	}
	else {
		m_alertWinContainer = new GraphicsItemContainer(kTabletNotificationContentWidth, 0, GraphicsItemContainer::PopupBackground);
		m_alertWinContainer->setBlockGesturesAndMouse(true);
		m_alertWinContainer->setAcceptTouchEvents(true);
	}
	m_transientContainer = new GraphicsItemContainer(kTabletNotificationContentWidth, 0, GraphicsItemContainer::TransientAlertBackground);
	m_transientContainer->setBlockGesturesAndMouse(true);
	m_transientContainer->setOpacity(0.0);
	m_transientContainer->setVisible(true);
	m_transientContainer->setAcceptTouchEvents(true);

	m_transAlertAnimation.setTargetObject(m_transientContainer);
	m_transAlertAnimation.setPropertyName("opacity");

	// Connect the signal to process events after animation is complete
	connect(&m_transAlertAnimation, SIGNAL(finished()), SLOT(slotTransientAnimationFinished()));

	connect(m_dashboardWinContainer, SIGNAL(signalWindowAdded(DashboardWindow*)),
			SLOT(slotDashboardWindowAdded(DashboardWindow*)));
	connect(m_dashboardWinContainer, SIGNAL(signalWindowsRemoved(DashboardWindow*)),
			SLOT(slotDashboardWindowsRemoved(DashboardWindow*)));
	connect(m_dashboardWinContainer, SIGNAL(signalViewportHeightChanged()),
			SLOT(slotDashboardViewportHeightChanged()));

	connect(SystemUiController::instance()->statusBar(), SIGNAL(signalDashboardAreaRightEdgeOffset(int)),
			this, SLOT(slotDashboardAreaRightEdgeOffset(int)));

	m_bannerWin = new BannerWindow(this, width, Settings::LunaSettings()->positiveSpaceTopPadding);
	m_bgItem = new GraphicsItemContainer(width, height, m_isOverlay ? GraphicsItemContainer::NoBackground : GraphicsItemContainer::SolidRectBackground);

	if(!m_isOverlay) {
		m_dashboardWinContainer->setParentItem(this);
	}


	// Listen for dock mode
	DockModeWindowManager* dmwm = 0;
	dmwm = ((DockModeWindowManager*)((WindowServerLuna*)WindowServer::instance())->dockModeManager());

	// Listen for dock mode animation to ignore dashboard open / close requests
	connect ((WindowServerLuna*)WindowServer::instance(), SIGNAL (signalDockModeAnimationStarted()),
			this, SLOT(slotDockModeAnimationStarted()));
	connect ((WindowServerLuna*)WindowServer::instance(), SIGNAL (signalDockModeAnimationComplete()),
			this, SLOT(slotDockModeAnimationComplete()));

	m_alertWinContainer->setParentItem(this);
	m_bgItem->setParentItem(this);
	m_bannerWin->setParentItem(m_bgItem);

	if(m_isOverlay) {
		m_transientContainer->setParentItem(this);

		// Set the pos of the Alert Containers correctly.
		positionAlertWindowContainer();
		positionTransientWindowContainer();

		if (m_menuObject) {
			int uiHeight = SystemUiController::instance()->currentUiHeight();
			// temporary location just until the status bar tell us where to place the dashboard container
			m_menuObject->setPos(0, -uiHeight/2 + Settings::LunaSettings()->positiveSpaceTopPadding);
		}

		// Connect the signal to process events after animation is complete
		connect(&m_fadeInOutAnimation, SIGNAL(finished()), SLOT(slotDeleteAnimationFinished()));
	}
	else {
		setPosTopLeft(m_alertWinContainer, 0, 0);
		m_alertWinContainer->setBrush(Qt::black);
		m_bgItem->setBrush(Qt::black);
	}

	setPosTopLeft(m_bgItem, 0, 0);
	setPosTopLeft(m_bannerWin, 0, 0);
	setupStateMachine();
}

void DashboardWindowManager::setupStateMachine()
{
	m_stateMachine = new QStateMachine(this);

	m_stateDashboardOpen = new DWMStateOpen(this);
	m_stateAlertOpen = new DWMStateAlertOpen(this);
	m_stateClosed = new DWMStateClosed(this);

	m_stateMachine->addState(m_stateDashboardOpen);
	m_stateMachine->addState(m_stateAlertOpen);
	m_stateMachine->addState(m_stateClosed);


	// ------------------------------------------------------------------------------

	m_stateDashboardOpen->addTransition(this, SIGNAL(signalActiveAlertWindowChanged()), m_stateClosed);
	m_stateAlertOpen->addTransition(this, SIGNAL(signalActiveAlertWindowChanged()), m_stateClosed);
	m_stateDashboardOpen->addTransition(m_dashboardWinContainer, SIGNAL(signalEmpty()), m_stateClosed);


	m_stateClosed->addTransition(new DWMTransitionClosedToAlertOpen(this, m_stateAlertOpen, this, SIGNAL(signalActiveAlertWindowChanged())));
	m_stateClosed->addTransition(this, SIGNAL(signalOpen()), m_stateDashboardOpen);
	m_stateClosed->addTransition(new DWMTransitionClosedToAlertOpen(this, m_stateAlertOpen, m_stateClosed, SIGNAL(signalNegativeSpaceAnimationFinished())));


	m_stateDashboardOpen->addTransition(this, SIGNAL(signalClose(bool)), m_stateClosed);
	m_stateAlertOpen->addTransition(this, SIGNAL(signalClose(bool)), m_stateClosed);
	m_stateClosed->addTransition(this, SIGNAL(signalClose(bool)), m_stateClosed);

	// ------------------------------------------------------------------------------

	m_stateMachine->setInitialState(m_stateClosed);

	m_stateMachine->start();
}

int DashboardWindowManager::sTabletUiWidth()
{
	return kTabletNotificationContentWidth;
}

int DashboardWindowManager::bannerWindowHeight()
{
	return m_bannerWin->boundingRect().height();
}

bool DashboardWindowManager::canCloseDashboard() const
{
	return m_stateCurrent == m_stateDashboardOpen;
}

bool DashboardWindowManager::dashboardOpen() const
{
	return m_stateCurrent == m_stateDashboardOpen;
}

bool DashboardWindowManager::hasDashboardContent() const
{
	return m_bannerHasContent || !m_dashboardWinContainer->empty();
}

void DashboardWindowManager::openDashboard()
{
	if (m_inDockModeAnimation)
		return;

	if(!SystemUiController::instance()->statusBarAndNotificationAreaShown())
		return;

	if (!m_alertWinArray.empty() || m_dashboardWinContainer->empty()) {
		return ;
	}

	Q_EMIT signalOpen();
}

void DashboardWindowManager::slotPositiveSpaceChanged(const QRect& r)
{
	positionAlertWindowContainer(r);
	positionTransientWindowContainer(r);
	positionDashboardContainer(r);
}

void DashboardWindowManager::slotPositiveSpaceChangeFinished(const QRect& r)
{
	positionAlertWindowContainer(r);
	positionTransientWindowContainer(r);
	positionDashboardContainer(r);
}

void DashboardWindowManager::slotNegativeSpaceChanged(const QRect& r)
{
	negativeSpaceChanged(r);
}

void DashboardWindowManager::negativeSpaceChanged(const QRect& r)
{
	setPos(0, r.y());
	update();
}

void DashboardWindowManager::slotNegativeSpaceChangeFinished(const QRect& r)
{
	if (G_UNLIKELY(m_stateCurrent == 0)) {
		g_critical("m_stateCurrent is null");
		return;
	}

	m_stateCurrent->negativeSpaceAnimationFinished();
}

void DashboardWindowManager::slotOpenDashboard()
{
	openDashboard();
}

void DashboardWindowManager::slotCloseDashboard(bool forceClose)
{
	if (!forceClose && !m_stateCurrent->allowsSoftClose())
		return;

	Q_EMIT signalClose(forceClose);
}

void DashboardWindowManager::slotCloseAlert()
{
	AlertWindow* win = topAlertWindow();

	if(!win)
		return;

	win->deactivate();
	notifyActiveAlertWindowDeactivated(win);
	win->close();
}

void DashboardWindowManager::slotDashboardAreaRightEdgeOffset(int offset)
{
    if (m_menuObject) {
    	m_dashboardRightOffset = offset;
	    m_menuObject->setX((boundingRect().width()/2) - m_dashboardRightOffset - m_menuObject->boundingRect().width() + m_notifMenuRightEdgeOffset);
    }
}

void DashboardWindowManager::slotDeleteAnimationFinished()
{
	switch(m_AlertWindowFadeOption) {
	case FadeInAndOut:
	case FadeOutOnly:
		// Reset the dimensions and position of the dashboardwindow container to the orignal one.
		m_alertWinContainer->resize(kTabletNotificationContentWidth, 0);

		positionAlertWindowContainer();

		if(m_previousActiveAlertWindow) {
			m_previousActiveAlertWindow->setParent(NULL);
			m_previousActiveAlertWindow->setVisible(false);
			m_previousActiveAlertWindow = NULL;
		}
		if(m_deleteWinAfterAnimation) {
			delete m_deleteWinAfterAnimation;
			m_deleteWinAfterAnimation = NULL;
		}

		if(FadeOutOnly == m_AlertWindowFadeOption) {
			// Change the state to closed.
			Q_EMIT signalActiveAlertWindowChanged();
		}
		else if(topAlertWindow()) {
			// we need to fade in the new alert window
			m_AlertWindowFadeOption = FadeInOnly;
			resizeAlertWindowContainer(topAlertWindow(), true);
			animateAlertWindow();
			notifyActiveAlertWindowActivated (topAlertWindow());
		} else {
			m_AlertWindowFadeOption = Invalid;
		}

		m_AlertWindowFadeOption = Invalid;
		break;

	default:
		m_AlertWindowFadeOption = Invalid;
		break;
	}
}

void DashboardWindowManager::slotTransientAnimationFinished()
{
	if(m_deleteTransientWinAfterAnimation) {
		if(m_activeTransientAlert) {
			delete m_activeTransientAlert;
			m_activeTransientAlert = NULL;
		}
		m_deleteTransientWinAfterAnimation = false;
	}
	if(m_transientContainer->opacity() == 0.0)
		m_transientContainer->setVisible(false);
}

void DashboardWindowManager::setBannerHasContent(bool val)
{
	m_bannerHasContent = val;

	if (val || !m_dashboardWinContainer->empty()) {
		SystemUiController::instance()->setDashboardHasContent(true);
		if (G_LIKELY(m_stateCurrent))
			m_stateCurrent->dashboardContentAdded();
	}
	else {
		SystemUiController::instance()->setDashboardHasContent(false);
		if (G_LIKELY(m_stateCurrent))
			m_stateCurrent->dashboardContentRemoved();
	}

	update();
}

void DashboardWindowManager::focusWindow(Window* w)
{
	// we listen to focus and blur only for alert windows
	if (!(w->type() == Window::Type_PopupAlert ||
		  w->type() == Window::Type_BannerAlert))

		return;

	AlertWindow* win = static_cast<AlertWindow*>(w);

	// And only for persistable windows
	PersistentWindowCache* persistCache = PersistentWindowCache::instance();
	if (!persistCache->shouldPersistWindow(win))
		return;

	addAlertWindow(win);
}

void DashboardWindowManager::unfocusWindow(Window* w)
{
	// we listen to focus and blur only for alert windows
	if (w->type() != Window::Type_PopupAlert &&
		w->type() != Window::Type_BannerAlert)
		return;

	AlertWindow* win = static_cast<AlertWindow*>(w);

	// Is this window in our current list of windows? If no,
	// nothing to do
	if (!m_alertWinArray.contains(win))
		return;

	// Look only for persistable windows
	PersistentWindowCache* persistCache = PersistentWindowCache::instance();
	if (!persistCache->shouldPersistWindow(win))
		return;

	// Add to persistent cache if not already in there
	if (!persistCache->hasWindow(win))
		persistCache->addWindow(win);

	hideOrCloseAlertWindow(win);
}

void DashboardWindowManager::animateAlertWindow()
{
	// We are running on a tablet and we need to fade the AlertWindows in or out. There are two cases:
	// 1: If there are no more alert windows, just animate this one out and be done with it.
	// 2: If we have other alert windows, activate them.

	// Stop the existing animation
	m_fadeInOutAnimation.stop();
	m_fadeInOutAnimation.clear();

	AlertWindow* w = NULL;
	qreal endValue = 0.0;

	// Set the opacity of the window to 0;
	switch(m_AlertWindowFadeOption) {
	case FadeInOnly:
		w = topAlertWindow();
		m_alertWinContainer->setOpacity(0.0);
		w->show();
		w->activate();
		endValue = 1.0;
		break;

	case FadeInAndOut:
	case FadeOutOnly:
		if(m_previousActiveAlertWindow) {
			w = m_previousActiveAlertWindow;
		}
		else {
			w = m_deleteWinAfterAnimation;
		}
		endValue = 0.0;
		break;

	default:
		return;
	}

	// Start the animation

	QPropertyAnimation* a  = new QPropertyAnimation(m_alertWinContainer, "opacity");
	a->setEndValue(endValue);
	// TODO - CHANGE DURATION TO READ FROM ANIMATIONSETTINGS.H
	a->setDuration(400);
	a->setEasingCurve(QEasingCurve::Linear);
	m_fadeInOutAnimation.addAnimation(a);
	m_fadeInOutAnimation.start();
}

void DashboardWindowManager::animateTransientAlertWindow(bool in)
{
	// Stop the existing animation
	m_transAlertAnimation.stop();

	if(!m_activeTransientAlert)
		return;

	qreal endValue = 0.0;

	if(in) {
		m_transientContainer->setVisible(true);
		m_activeTransientAlert->show();
		m_activeTransientAlert->activate();
		endValue = 1.0;

	} else {
		endValue = 0.0;
	}

	// Start the animation

	m_transAlertAnimation.setStartValue(m_transientContainer->opacity());
	m_transAlertAnimation.setEndValue(endValue);
	m_transAlertAnimation.setDuration(400);
	m_transAlertAnimation.setEasingCurve(QEasingCurve::Linear);
	m_transAlertAnimation.start();
}

void DashboardWindowManager::hideOrCloseAlertWindow(AlertWindow* win)
{
	bool isActiveAlert = false;
	if (!m_alertWinArray.empty() && (m_alertWinArray[0] == win))
		isActiveAlert = true;

	int removeIndex = m_alertWinArray.indexOf(win);
	if (removeIndex != -1)
		m_alertWinArray.remove(removeIndex);

	// Is this a window that we want to persist?
	PersistentWindowCache* persistCache = PersistentWindowCache::instance();
	if (!persistCache->shouldPersistWindow(win)) {

		if (isActiveAlert) {
			win->deactivate();
			notifyActiveAlertWindowDeactivated(win);
		}

		// No. Close it
		win->close();
		return;
	}

	if (!persistCache->hasWindow(win))
		persistCache->addWindow(win);

	persistCache->hideWindow(win);

	if (isActiveAlert) {
		win->deactivate();
		notifyActiveAlertWindowDeactivated(win);
		Q_EMIT signalActiveAlertWindowChanged();
	}
}

void DashboardWindowManager::resizeAlertWindowContainer(AlertWindow* w, bool repositionWindow)
{
	if(!w)
		return;

	// This should not be executed if we are running on a phone.
	if(!isOverlay())
		return;

	bool wasResized = false;

	QRectF bRect = m_alertWinContainer->boundingRect();
	if(w->initialHeight() != bRect.height() || w->initialWidth() != bRect.width()) {
		m_alertWinContainer->resize(w->initialWidth(), w->initialHeight());
		positionAlertWindowContainer();
		wasResized = true;
	}

	if(true == repositionWindow) {
		if(true == wasResized) {
			w->setPos(0,0);
		}
	}
}

void DashboardWindowManager::addAlertWindow(AlertWindow* win)
{
	if(win->isTransientAlert()) {
		// transient alerts are handled separately
		addTransientAlertWindow(win);
		return;
	}

	// This window may have already been added because a persistent
	// window called focus before the corresponding webapp was ready
	if (m_alertWinArray.contains(win))
	 	return;

	bool reSignalOpenAlert = false;

	PersistentWindowCache* persistCache = PersistentWindowCache::instance();
	if (persistCache->shouldPersistWindow(win)) {
		if (!persistCache->hasWindow(win)) {
			persistCache->addWindow(win);
		}
	}

	AlertWindow* oldActiveWindow = 0;
	if (!m_alertWinArray.empty())
		oldActiveWindow = m_alertWinArray[0];

	addAlertWindowBasedOnPriority(win);

	// Set the flags depending on whether we are running on phone/tablet.
	if(isOverlay()) {
		if(Invalid == m_AlertWindowFadeOption) {
			m_AlertWindowFadeOption = FadeInOnly;
		}

		// resize the alertwindowcontainer if there are no other active alert windows.
		if(m_stateCurrent != m_stateAlertOpen) {
			resizeAlertWindowContainer(win, false);
		}
	}

	if(!isOverlay()) {
		if(win->boundingRect().width() != SystemUiController::instance()->currentUiWidth())
		{
			win->resizeEventSync(SystemUiController::instance()->currentUiWidth(), win->initialHeight());
			setPosTopLeft(win, 0, 0);
		}
	}
	else {
		win->setPos(0,0);
	}

	if (!win->parentItem()) {
		win->setParentItem(m_alertWinContainer);
		if(!m_isOverlay)
			setPosTopLeft(win, 0, 0);
		else
			win->setPos(0,0);
		// Initially set the visibility to false
		win->setVisible(false);
	}

	AlertWindow* newActiveWindow = m_alertWinArray[0];
	if (oldActiveWindow && (oldActiveWindow == newActiveWindow)) {
		// Adding the new window did not displace the current active window.
		// Nothing to do further
		return;
	}

	if (oldActiveWindow) {
		reSignalOpenAlert = true;
		m_previousActiveAlertWindow = oldActiveWindow;
		// Displaced the current active window. Need to hide it
		if (persistCache->shouldPersistWindow(oldActiveWindow)) {
			if (!persistCache->hasWindow(oldActiveWindow))
				persistCache->addWindow(oldActiveWindow);
			persistCache->hideWindow(oldActiveWindow);
		}
		else {
			oldActiveWindow->deactivate();
		}
		// Set that we need to fade the existing window out and the new window in.
		m_AlertWindowFadeOption = FadeInAndOut;
	} else if(FadeOutOnly == m_AlertWindowFadeOption) {
		// currently in the process of closing the dashboard (last alert just got deactivated before the new window was added),
		// so mark the fade option as Fade Ina dn Out to bring it back with the new alert window
		m_AlertWindowFadeOption = FadeInAndOut;
		return;
	}

	if(!isOverlay()) {
		Q_EMIT signalActiveAlertWindowChanged();
	}
	else {
		// Check if we need to resignal to show the latest alert.
		if(false == reSignalOpenAlert) {
			// Dashboard was open. The first signal will close the dashbaord. Resignal to open the alert
			if(m_stateCurrent == m_stateDashboardOpen)
				reSignalOpenAlert = true;
		}

		// Signal once to move the state to closed
		Q_EMIT signalActiveAlertWindowChanged();

		// signal again to move the state to display the active alert window
		if(true == reSignalOpenAlert) {
			Q_EMIT signalActiveAlertWindowChanged();
		}
	}
}

void DashboardWindowManager::addTransientAlertWindow(AlertWindow* win)
{
	if (m_activeTransientAlert == win)
	 	return;

	if(m_activeTransientAlert) {
		// only one transient alert is allowed at any given time, so close the current one
		m_activeTransientAlert->deactivate();

		m_deleteTransientWinAfterAnimation = NULL;
		m_transAlertAnimation.stop();

		notifyTransientAlertWindowDeactivated(m_activeTransientAlert);
		delete m_activeTransientAlert;
		m_activeTransientAlert = 0;
	}

	m_activeTransientAlert = win;

	win->setParentItem(m_transientContainer);
	win->setVisible(true);
	win->setOpacity(1.0);
	win->setPos(0,0);
	win->resizeEventSync(win->initialWidth(), win->initialHeight());

	// resize the transient alert window container if there are no other active alert windows.
	QRectF bRect = m_transientContainer->boundingRect();
	if(win->initialHeight() != bRect.height() || win->initialWidth() != bRect.width()) {
		m_transientContainer->resize(win->initialWidth(), win->initialHeight());
	}

	positionTransientWindowContainer();

	notifyTransientAlertWindowActivated(m_activeTransientAlert);

	animateTransientAlertWindow(true);
}

void DashboardWindowManager::removeAlertWindow(AlertWindow* win)
{
	if(win->isTransientAlert()) {
		// transient alerts are handled separately
		removeTransientAlertWindow(win);
		return;
	}

	PersistentWindowCache* persistCache = PersistentWindowCache::instance();
	if (!m_alertWinArray.contains(win)) {

		// This can happen in two cases:
		// * a window was stuffed int the persistent window cache on hiding
		// * a window was closed before being added to the window manager
		if (persistCache->hasWindow(win)) {
			persistCache->removeWindow(win);
		}
		delete win;
		return;
	}

	if (persistCache->hasWindow(win))
		persistCache->removeWindow(win);

	bool isActiveAlert = false;
	if (!m_alertWinArray.empty() && (m_alertWinArray[0] == win))
		isActiveAlert = true;

	int removeIndex = m_alertWinArray.indexOf(win);
	if (removeIndex != -1) {
		m_alertWinArray.remove(removeIndex);
	}

	if (isActiveAlert) {
		win->deactivate();
		notifyActiveAlertWindowDeactivated(win);
	}

	// If we are running on the phone, just signal the state to enter closed.
	if(!isOverlay()) {
		delete win;
		if(isActiveAlert) {
			Q_EMIT signalActiveAlertWindowChanged();
		}
	}
	else {
		m_deleteWinAfterAnimation = win;
		if (isActiveAlert) {
			// Change the state if there are more alert windows in the system
			if(!m_alertWinArray.empty()) {
				// This will cause the state to enter stateClosed.
				Q_EMIT signalActiveAlertWindowChanged();

				// If we are running on a tablet and if this flag is set to true, signal that we need to display the next alert
				if (G_UNLIKELY(m_stateCurrent == 0)) {
					g_critical("m_stateCurrent is null");
					return;
				}

				// Set that we need to fade the existing window out and the new window in.
				m_AlertWindowFadeOption = FadeInAndOut;
				// Signal that we need to bring the next active window in
				m_stateCurrent->negativeSpaceAnimationFinished();
			}
			else {
				m_AlertWindowFadeOption = FadeOutOnly;
				animateAlertWindow();
			}
		}
	}
}

void DashboardWindowManager::removeTransientAlertWindow(AlertWindow* win)
{
	if (m_activeTransientAlert != win) {
		delete win;
		return;
	}

	win->deactivate();
	notifyTransientAlertWindowDeactivated(win);

	m_deleteTransientWinAfterAnimation = true;
	animateTransientAlertWindow(false);
}

void DashboardWindowManager::addAlertWindowBasedOnPriority(AlertWindow* win)
{
	if (m_alertWinArray.empty()) {
		m_alertWinArray.append(win);
		return;
	}

	AlertWindow* activeAlertWin = m_alertWinArray[0];

	// Add the window to the list of alert windows, sorted by priority
	int newWinPriority = m_notificationPolicy->popupAlertPriority(win->appId(), win->name());

	if (newWinPriority == m_notificationPolicy->defaultPriority()) {
		// optimization. if we are at default priority we don't need to walk the list
		// we will just queue up this window
		int oldSize = m_alertWinArray.size();
		m_alertWinArray.append(win);
	}
	else if (newWinPriority < m_notificationPolicy->popupAlertPriority(activeAlertWin->appId(),
																	   activeAlertWin->name())) {
		// if the priority of new window is higher than the currently shown window, then we push
		// both the windows in the queue with the new window in front
		QVector<AlertWindow*>::iterator it = qFind(m_alertWinArray.begin(), m_alertWinArray.end(), activeAlertWin);
		Q_ASSERT(it != m_alertWinArray.end());
		m_alertWinArray.insert(it, win);
	}
	else {
		bool added = false;

		QMutableVectorIterator<AlertWindow*> it(m_alertWinArray);
		while (it.hasNext()) {
			AlertWindow* a = it.next();
			if (newWinPriority < m_notificationPolicy->popupAlertPriority(a->appId(), a->name())) {
				it.insert(win);
				added = true;
				break;
			}
		}

		if (!added)
			m_alertWinArray.append(win);
	}
}

void DashboardWindowManager::addWindow(Window* win)
{
	if (win->type() == Window::Type_PopupAlert || win->type() == Window::Type_BannerAlert)
		addAlertWindow(static_cast<AlertWindow*>(win));
	else if (win->type() == Window::Type_Dashboard)
		m_dashboardWinContainer->addWindow(static_cast<DashboardWindow*>(win));
}

void DashboardWindowManager::removeWindow(Window* win)
{
	if (win->type() == Window::Type_PopupAlert || win->type() == Window::Type_BannerAlert)
		removeAlertWindow(static_cast<AlertWindow*>(win));
	else if (win->type() == Window::Type_Dashboard)
		m_dashboardWinContainer->removeWindow(static_cast<DashboardWindow*>(win));

	/* if dashboard is empty, all throb requests should have been dismissed
	 * this is defensive coding to ensure that all requests are cleared
	 */
	if (m_dashboardWinContainer->empty() && m_alertWinArray.empty())
		CoreNaviManager::instance()->clearAllStandbyRequests();
}

int DashboardWindowManager::activeAlertWindowHeight()
{
	const Window* alert = getActiveAlertWin();
	if(!alertOpen() || !alert) {
		if(!isOverlay())
			return Settings::LunaSettings()->positiveSpaceBottomPadding;
		else
			return 0;
	}
	return alert->boundingRect().height();
}

bool DashboardWindowManager::doesMousePressFallInBannerWindow(QGraphicsSceneMouseEvent* event)
{
	QPointF pos = mapToItem(m_bannerWin, event->pos());

	if(m_bannerWin->boundingRect().contains(pos)) {
		return true;
	}
	else {
		return false;
	}
}

/*
 	 BannerWindow is invisible when the DashboardWinContainer is up. So if the user clicks in the BannerWindow area, it will not get any notifications.
 	 So, we need to have this workaround in DashboardWinMgr::mousePressEvent that will detect if the user clicked on BannerWindow area and prevent it
 	 from launching the dashboard again as a part of Qt::GestureComplete
*/

void DashboardWindowManager::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if(!m_isOverlay) {
		event->accept();
	}
	else {
		// if dashboard has content and we get a mousepress event, we need to close the dashboardwindow container.
		if(true == dashboardOpen()) {
			event->accept();
		} else {
			event->ignore();
		}
	}
}

void DashboardWindowManager::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	if(!m_isOverlay) {
		event->accept();
	}
	else {
		// if dashboard has content and we get a mousepress event, we need to close the dashboardwindow container.
		if(true == dashboardOpen()) {
			event->accept();
		} else {
			event->ignore();
		}
	}
}

void DashboardWindowManager::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if(!m_isOverlay) {
		event->accept();
	}
	else {
		// if dashboard has content and we get a mousepress event, we need to close the dashboardwindow container.
		if(true == dashboardOpen()) {
			hideDashboardWindow();
			event->accept();

			if(true == doesMousePressFallInBannerWindow(event)) {
				m_bannerWin->resetIgnoreGestureUpEvent();
			}

			m_dashboardWinContainer->resetLocalState();
		} else {
			event->ignore();
		}
	}
}

bool DashboardWindowManager::sceneEvent(QEvent* event)
{
	switch (event->type()) {
	case QEvent::GestureOverride: {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QList<QGesture*> activeGestures = ge->activeGestures();
		Q_FOREACH(QGesture* g, activeGestures) {
			if (g->hasHotSpot()) {
				QPointF pt = ge->mapToGraphicsScene(g->hotSpot());
				if (dashboardOpen()) {
					ge->accept(g);
				}
				else {
					ge->ignore(g);
				}
			}
		}
		break;
	}

	default:
		break;
	}

	return WindowManagerBase::sceneEvent(event);
}


void DashboardWindowManager::raiseAlertWindow(AlertWindow* window)
{
	g_message("%s", __PRETTY_FUNCTION__);
	m_alertWinContainer->show();
	m_alertWinContainer->raiseChild(window);
	m_bgItem->hide();
	if(!m_isOverlay)
		m_dashboardWinContainer->hide();
}

void DashboardWindowManager::raiseDashboardWindowContainer()
{
	g_message("%s", __PRETTY_FUNCTION__);
	if(!m_isOverlay)
		m_dashboardWinContainer->show();
	m_alertWinContainer->hide();
	m_bgItem->hide();
}

void DashboardWindowManager::raiseBackgroundWindow()
{
	if(m_stateCurrent != m_stateAlertOpen) {
		g_message("%s", __PRETTY_FUNCTION__);
		m_alertWinContainer->hide();
		m_bgItem->show();
		m_bannerWin->show();
		if(!m_isOverlay)
			m_dashboardWinContainer->hide();
	} else {
		g_message("%s: In alert state. raise alert window container", __PRETTY_FUNCTION__);
		m_alertWinContainer->show();
		m_bgItem->hide();
		if(!m_isOverlay)
			m_dashboardWinContainer->hide();
	}
}

void DashboardWindowManager::hideDashboardWindow()
{
	if(m_stateCurrent == m_stateDashboardOpen) {
		Q_EMIT signalClose(true);
	}

	if(!m_isOverlay)
		m_dashboardWinContainer->hide();
}

void DashboardWindowManager::sendClicktoDashboardWindow(int num, int x, int y, bool whileLocked)
{
	m_dashboardWinContainer->sendClickToDashboardWindow(num, QPointF(x, y), true);
}

void DashboardWindowManager::notifyActiveAlertWindowActivated(AlertWindow* win)
{
	if (win->isIncomingCallAlert())
		DisplayManager::instance()->alert(DISPLAY_ALERT_PHONECALL_ACTIVATED);
	else
		DisplayManager::instance()->alert(DISPLAY_ALERT_GENERIC_ACTIVATED);

	// NOTE: order matters, the DisplayManager needs to act on the alert before anyone else
	SystemUiController::instance()->notifyAlertActivated();
	Q_EMIT signalAlertWindowActivated(win);
}

void DashboardWindowManager::notifyActiveAlertWindowDeactivated(AlertWindow* win)
{
	if (win->isIncomingCallAlert())
		DisplayManager::instance()->alert(DISPLAY_ALERT_PHONECALL_DEACTIVATED);
	else
		DisplayManager::instance()->alert(DISPLAY_ALERT_GENERIC_DEACTIVATED);

	SystemUiController::instance()->notifyAlertDeactivated();
	Q_EMIT signalAlertWindowDeactivated();
}

void DashboardWindowManager::notifyTransientAlertWindowActivated(AlertWindow* win)
{
	if(!alertOpen())
		DisplayManager::instance()->alert(DISPLAY_ALERT_GENERIC_ACTIVATED);

	// NOTE: order matters, the DisplayManager needs to act on the alert before anyone else
	SystemUiController::instance()->notifyTransientAlertActivated();
}

void DashboardWindowManager::notifyTransientAlertWindowDeactivated(AlertWindow* win)
{
	if(!alertOpen())
		DisplayManager::instance()->alert(DISPLAY_ALERT_GENERIC_DEACTIVATED);

	SystemUiController::instance()->notifyTransientAlertDeactivated();
}

void DashboardWindowManager::slotDashboardWindowAdded(DashboardWindow* w)
{
	SystemUiController::instance()->setDashboardHasContent(true);
	m_bgItem->update();

	if (G_LIKELY(m_stateCurrent))
		m_stateCurrent->dashboardContentAdded();
}

void DashboardWindowManager::slotDashboardWindowsRemoved(DashboardWindow* w)
{
	m_bgItem->update();

	if (m_bannerHasContent || !m_dashboardWinContainer->empty())
		return;

	SystemUiController::instance()->setDashboardHasContent(false);

    if (G_LIKELY(m_stateCurrent))
		m_stateCurrent->dashboardContentRemoved();
}

void DashboardWindowManager::slotDashboardViewportHeightChanged()
{
    if (G_LIKELY(m_stateCurrent))
		m_stateCurrent->dashboardViewportHeightChanged();
}

void DashboardWindowManager::resize(int width, int height)
{
	QRectF bRect = boundingRect();

	// accept requests for resizing to the current dimensions, in case we are doing a force resize

	WindowManagerBase::resize(width, height);

	if(!m_isOverlay) {
		m_dashboardWinContainer->resize(width, height);
		setPosTopLeft(m_dashboardWinContainer, 0, 0);
		m_dashboardWinContainer->resizeWindowsEventSync(width);

		m_alertWinContainer->resize(width, height);
		setPosTopLeft(m_alertWinContainer, 0, 0);
		resizeAlertWindows(width);
	}
	else {

		int yLocCommon = -(height/2) + Settings::LunaSettings()->positiveSpaceTopPadding;
		int xLocDashWinCtr;
		if (m_menuObject) {
			xLocDashWinCtr = (boundingRect().width()/2) - m_dashboardRightOffset - m_menuObject->boundingRect().width() + m_notifMenuRightEdgeOffset;
		} else {
			xLocDashWinCtr = (boundingRect().width()/2) - m_dashboardRightOffset + m_notifMenuRightEdgeOffset;
		}
		int yLocDashWinCtr = yLocCommon;
		int yLocAlertWinCtr = (m_stateCurrent != m_stateAlertOpen)?yLocCommon:(yLocCommon + topAlertWindow()->initialHeight()/2 + kTabletAlertWindowPadding);

		// Set the new position of the m_dashboardWinContainer
		if (m_menuObject) {
			m_menuObject->setPos(xLocDashWinCtr, yLocDashWinCtr);
		}

		positionAlertWindowContainer();
	}

	if(!m_isOverlay) {
		// reposition the banner window and the backgound item
		m_bgItem->resize(width, height);
		setPosTopLeft(m_bgItem, 0, 0);

		m_bannerWin->resize(width, Settings::LunaSettings()->positiveSpaceTopPadding);
		setPosTopLeft(m_bannerWin, 0, 0);

		if((alertOpen())) {
			setPos(0, (SystemUiController::instance()->currentUiHeight()/2 + height/2 - activeAlertWindowHeight() - Settings::LunaSettings()->positiveSpaceBottomPadding));
		} else if (hasDashboardContent()) {
			setPos(0, (SystemUiController::instance()->currentUiHeight()/2 + height/2 - Settings::LunaSettings()->positiveSpaceBottomPadding));
		} else {
			setPos(0, SystemUiController::instance()->currentUiHeight()/2 + height/2);
		}
	}

	m_stateCurrent->handleUiResizeEvent();

	// resize all cached Alert Windows
	PersistentWindowCache* persistCache =  PersistentWindowCache::instance();

	std::set<Window*>* cachedWindows = persistCache->getCachedWindows();
	if(cachedWindows) {
		std::set<Window*>::iterator it;

		it=cachedWindows->begin();

		for ( it=cachedWindows->begin() ; it != cachedWindows->end(); it++ ) {
			Window* w = *it;
			if(w->type() == Window::Type_PopupAlert || w->type() == Window::Type_BannerAlert) {
				((AlertWindow*)w)->resizeEventSync((m_isOverlay ? kTabletNotificationContentWidth : width), 
													((AlertWindow*)w)->initialHeight());
			}
		}
	}
}

void DashboardWindowManager::resizeAlertWindows(int width)
{
	Q_FOREACH(AlertWindow* a, m_alertWinArray) {
		if (a) {
			a->resizeEventSync(width, a->initialHeight());
			if(!m_isOverlay)
				setPosTopLeft(a, 0, 0);
			else
				a->setPos(0,0);
		}
	}
}


void DashboardWindowManager::slotDockModeAnimationStarted()
{
	m_inDockModeAnimation = true;
}

void DashboardWindowManager::slotDockModeAnimationComplete()
{
	m_inDockModeAnimation = false;
}

void DashboardWindowManager::positionDashboardContainer(const QRect& posSpace)
{
    if (m_menuObject) {
    	QRect positiveSpace;

    	if(posSpace.isValid())
    		positiveSpace = posSpace;
    	else
    		positiveSpace = SystemUiController::instance()->positiveSpaceBounds();

    	int uiHeight = SystemUiController::instance()->currentUiHeight();

    	m_menuObject->setY(-uiHeight/2 + positiveSpace.y());
    }
}

void DashboardWindowManager::positionAlertWindowContainer(const QRect& posSpace)
{
	QRect positiveSpace;

	if(posSpace.isValid())
		positiveSpace = posSpace;
	else
		positiveSpace = SystemUiController::instance()->positiveSpaceBounds();

	int uiHeight = SystemUiController::instance()->currentUiHeight();


	m_alertWinContainer->setPos(QPoint(positiveSpace.width()/2 - kTabletAlertWindowPadding - m_alertWinContainer->width()/2,
			                           -uiHeight/2 + positiveSpace.y() + kTabletAlertWindowPadding + m_alertWinContainer->height()/2));
}

void DashboardWindowManager::positionTransientWindowContainer(const QRect& posSpace)
{
	QRect positiveSpace;

	if(posSpace.isValid())
		positiveSpace = posSpace;
	else
		positiveSpace = SystemUiController::instance()->positiveSpaceBounds();

	int uiHeight = SystemUiController::instance()->currentUiHeight();
	int uiWidth = SystemUiController::instance()->currentUiWidth();

	m_transientContainer->setPos(QPoint(positiveSpace.center().x() - uiWidth/2, positiveSpace.center().y() - uiHeight/2));
}
