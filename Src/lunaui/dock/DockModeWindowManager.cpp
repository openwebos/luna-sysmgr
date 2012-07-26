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




#include "Common.h"

#include <glib.h>
#include <string>

#include "HostBase.h"
#include "Settings.h"
#include "DockModeWindowManager.h"
#include "Window.h"
#include "WindowServer.h"
#include "Logging.h"
#include "WebAppMgrProxy.h"
#include "DockModeWindow.h"
#include "DockModeMenuManager.h"
#include "LaunchPoint.h"
#include "Localization.h"
#include "ApplicationDescription.h"
#include "DashboardWindowManager.h"
#include "DockModePositionManager.h"
#include "DockModeLaunchPoint.h"
#include "ApplicationManager.h"
#include "ApplicationDescription.h"
#include "SystemUiController.h"
#include "WindowServerLuna.h"
#include "TopLevelWindowManager.h"
#include "AnimationSettings.h"
#include "SystemService.h"
#include "Utils.h"
#include "DockModeClock.h"
#include "DockModeAppMenuContainer.h"
#include "DisplayManager.h"
#include "Preferences.h"

#include <QPropertyAnimation>
#include <QEvent>
#include <QGesture>
#include <QGestureEvent>
#include <QPropertyAnimation>
#include <QTextLayout>
#include <QCoreApplication>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>

#include "cjson/json.h"

static const char* kDockLauncherAppId = "com.palm.launcher";

// replace this with the actual APP ID for Preferences
static const char* kSysUiAppId = "com.palm.systemui";
//static const char* kAddAppId   = "com.palm.systemui";

static int kItemsPerRow = 3;
static int kMarginForButtons = 0;

static int kPortraitTopOffeset = 30;
static int kLandscapeTopOffeset = 10;
static int kItemRowSpacing = 45;

static int kMaximWinSpacing = 50;

static int kAppImageWidth = 0;
static int kAppImageHeight = 0;

static int kAppMenuWidth = 320;

#define    LAUNCHER_BUTTONS_Z    0
#define    MINIMIZED_WINDOW_Z    1
#define    APP_IMAGE_Z           2
#define    DRAGGED_WINDOW_Z      3
#define    MAXIMIZED_WINDOW_Z    4
#define    PREFS_APP_WINDOW_Z    5
#define    CORNER_WINDOW_Z       6
#define    FADE_PANNEL_Z         10

#define TOP_LEFT_CORNER 0
#define TOP_RIGHT_CORNER 1
#define BOTTOM_LEFT_CORNER 2
#define BOTTOM_RIGHT_CORNER 3


// Layer to contain the status bar and menus. Needed to enforce Z order of menus and apps in Dock Mode menus
DockModeWindowManager::DockModeWindowManager(uint32_t maxWidth, uint32_t maxHeight)
	: WindowManagerBase(maxWidth, maxHeight)
	, m_dashboardOpened(false)
	, m_dashboardSoftDismissable(true)
	, m_inDockMode(false)
	, m_inTransitionAnimation(false)
	, m_sysUiLpInUse(0)
	, m_newlyAddedLp(0)
	, m_inReorderMode(false)
	, m_background(0)
	, m_compMode(QPainter::CompositionMode_Source)
	, m_previous(QPainter::CompositionMode_SourceOver)
	, m_activeWin(0)
	, m_windowInAnimation(0)
	, m_nextActiveWin(0)
	, m_lastMaximizedWin(0)
	, m_defaultIndex(0)
	, m_dragLp(0)
	, m_systemAppAnimation(0)
	, m_appMenuContainer (0)
	, m_dockModeClock (0)
	, m_dockModePosMgr (0)
	, m_bootFinished(0)
	, m_positiveSpace (0,0, 0, 0)
{
	setObjectName("DockModeWindowManager");
	
	setFlag(QGraphicsItem::ItemHasNoContents, false);
}
	

DockModeWindowManager::~DockModeWindowManager()
{
	ungrabGesture(Qt::TapGesture);

	if(m_background)
		delete m_background;
}

void DockModeWindowManager::init()
{
	setOpacity(1.0);
	setVisible(false);
	
	WindowServerLuna* wsl = static_cast<WindowServerLuna*>(WindowServer::instance());
	connect (wsl, SIGNAL(signalDockModeAnimationStarted()), this, SLOT(slotDockModeAnimationStarted()));
	connect (wsl, SIGNAL(signalDockModeAnimationComplete()), this, SLOT(slotDockModeAnimationComplete()));

	DockModeMenuManager* dmm = (DockModeMenuManager*)(wsl->dockModeMenuManager());
	m_appMenuContainer = dmm->dockAppContainer();
	m_statusBar = dmm->statusBar();

	connect(m_appMenuContainer, SIGNAL(signalDockModeAppSelected(DockModeLaunchPoint*)), this, SLOT(slotDockModeAppSelected(DockModeLaunchPoint*)));

	connect(ApplicationManager::instance(), SIGNAL(signalDockModeLaunchPointEnabled(const LaunchPoint*)), this, SLOT(slotDockModeLaunchPointEnabled(const LaunchPoint*)));
	connect(ApplicationManager::instance(), SIGNAL(signalDockModeLaunchPointDisabled(const LaunchPoint*)), this, SLOT(slotDockModeLaunchPointDisabled(const LaunchPoint*)));
	connect(ApplicationManager::instance(), SIGNAL(signalLaunchPointRemoved(const LaunchPoint*, QBitArray)), SLOT(slotLaunchPointRemoved(const LaunchPoint*, QBitArray)));
	m_dockModePosMgr = DockModePositionManager::instance();

	m_dockModeClock = new DockModeClock(this, MAXIMIZED_WINDOW_Z);
	if (m_dockModeClock) {
		enableDockModeLaunchPointInternal (m_dockModeClock->launchPoint(), true);
		m_dockModeClock->dockWindow()->setParentItem (this);
		addWindow (m_dockModeClock->dockWindow());
	}

	m_dockModePosMgr->load();
	setDockModeState (false);

	DashboardWindowManager* dwm = (DashboardWindowManager*)(static_cast<WindowServerLuna*>(WindowServer::instance())->dashboardWindowManager());

	connect(SystemUiController::instance(), SIGNAL(signalBootFinished()), this, SLOT(slotBootFinished()));

	connect(WebAppMgrProxy::instance(), SIGNAL(signalLowMemoryActionsRequested(bool)), this, SLOT(slotLowMemoryActionsRequested(bool)));
	
	grabGesture(Qt::TapGesture);
	
	connect (SystemUiController::instance(), SIGNAL(signalPositiveSpaceChanged(const QRect&)), this, SLOT(slotPositiveSpaceChanged(const QRect&)));

	// set up positive and negative space defaults
	// tdh -- this assumes portrait mode orientation
	const HostInfo& info = HostBase::instance()->getInfo();

	// Set up the animations
	
	m_windowAnimationCurrent.setPropertyName("opacity");
	m_windowAnimationCurrent.setDuration(AS(dockFadeDockAnimationDuration));
	m_windowAnimationCurrent.setEasingCurve(AS_CURVE(dockFadeAnimationCurve));

	m_windowAnimationNext.setPropertyName("opacity");
	m_windowAnimationNext.setDuration(AS(dockFadeDockAnimationDuration));
	m_windowAnimationNext.setEasingCurve(AS_CURVE(dockFadeAnimationCurve));
	
	m_windowChangeAnimationGroup.addAnimation(&m_windowAnimationCurrent);
	m_windowChangeAnimationGroup.addAnimation(&m_windowAnimationNext);
	connect(&m_windowChangeAnimationGroup, SIGNAL(finished()), SLOT(slotWindowChangeAnimationFinished()));

	connect(this, SIGNAL(signalDockModeStatusChanged(bool)), SystemService::instance(), SLOT(postDockModeStatus(bool)));
	connect(this, SIGNAL(visibleChanged()), SLOT(slotVisibilityChanged()));	

	connect(DisplayManager::instance(), SIGNAL(signalPuckConnected(bool)), this, SLOT(slotPuckConnected(bool)));
}

bool DockModeWindowManager::handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate)
{
    propogate = !m_inDockMode;
    return false;
}

void DockModeWindowManager::slotVisibilityChanged()
{
	if(isVisible()) {
		// just became visible again, so refocus the active window
		if(m_inDockMode && m_activeWin && !m_inTransitionAnimation) {
			m_activeWin->setFocus();
		}
	} 
}

void DockModeWindowManager::slotDockModeLaunchPointEnabled (const LaunchPoint* lp)
{
	enableDockModeLaunchPoint (lp);
}

void DockModeWindowManager::slotDockModeLaunchPointDisabled (const LaunchPoint* lp)
{
	disableDockModeLaunchPoint (lp);
}

void DockModeWindowManager::slotPositiveSpaceChanged (const QRect& rect)
{
	m_positiveSpace = rect;
	if (m_activeWin) {
		m_activeWin->setPos (m_positiveSpace.topLeft());
		((DockModeWindow*)m_activeWin)->resizeEventSync (m_positiveSpace.width(), m_positiveSpace.height());
	}

	for (int i = 0; i < m_dockLpArray.size(); ++i) {
		m_dockLpArray[i]->resize (rect.width(), rect.height());
		Window* win = m_dockLpArray[i]->openWindow();
		if (win && win != m_activeWin) {
			win->setPos (m_positiveSpace.topLeft());
			((DockModeWindow*)win)->resizeEvent (m_positiveSpace.width(), m_positiveSpace.height());
		}
	}
}

void DockModeWindowManager::setDashboardOpened(bool val, bool softDismissable)
{
	m_dashboardOpened = val;
	m_dashboardSoftDismissable = softDismissable;
}

bool DockModeWindowManager::isDashboardOpened()
{
	return m_dashboardOpened;
}

bool DockModeWindowManager::isDashboardSoftDismissable()
{
	return m_dashboardSoftDismissable;
}

void DockModeWindowManager::closeDashboard(bool force)
{
	if(m_dashboardOpened)
		Q_EMIT signalCloseDashboard(force);
}

DockModeLaunchPoint* DockModeWindowManager::launchPointForId(const std::string appId)
{
	for (int i = 0; i < m_dockLpArray.size(); i++) {
		if(m_dockLpArray[i]->launchPoint()->id() == appId) {
			return m_dockLpArray[i];
		}
	}
	
	if (m_sysUiLpInUse && m_sysUiLpInUse->launchPoint()->id() == appId)
		return m_sysUiLpInUse;
	return 0;
}

int DockModeWindowManager::appIndexForId(const std::string appId)
{
	for (int i = 0; i < m_dockLpArray.size(); i++) {
		if(m_dockLpArray[i]->launchPoint()->id() == appId) {
			return i;
		}
	}
	return -1;
}


void DockModeWindowManager::prepareAddWindow(Window* win)
{
	if (win->type() != Window::Type_DockModeWindow)
	    return;
	
	DockModeWindow* dockWin = static_cast<DockModeWindow*>(win);

	// Proxy windows don't have to wait ($$$ needed?)
	if (!dockWin->isHost()) {
		// delay adding a new card 
		if (dockWin->delayPrepare()) {
			return;
		}
	}

	dockWin->setParentItem(this);
	
	DockModeLaunchPoint* dlp = launchPointForId(win->appId());
	
	if(!dlp) {
		// Error: window is not in the list of apps in the launcher
		g_critical("%s: Error: window is not in the list of apps in the launcher: %s", __FUNCTION__, win->appId().c_str());
		closeWindow(win);
		return;
	}
	
	if((dlp->state() == DockModeLaunchPoint::NotLaunched || dlp->state() == DockModeLaunchPoint::Closed))
	{
		g_critical("%s: Error: this app wasn't marked as launching: %s", __FUNCTION__, win->appId().c_str());
		// Error: this app wasn't marked as launching
		closeWindow(win);
		return;
	}

	if(dlp->state() == DockModeLaunchPoint::Running) {
		g_critical("%s: Error: this app is already runnign: %s", __FUNCTION__, win->appId().c_str());
		// Error: this app wasn't marked as launching
		if(dlp->openWindow())
			closeWindow(dlp->openWindow());
		dlp->setState(DockModeLaunchPoint::Closed);
	}
	
	dockWin->setVisible(true);
	dockWin->setScale(1.0);
	dockWin->setOpacity(1.0);
	dockWin->setPos(m_positiveSpace.topLeft());
	dockWin->setPrepareAddedToWindowManager();

	m_statusBar->setMaximizedAppTitle (true, dlp->launchPoint()->menuName().c_str());
	dlp->setOpenWindow(win);
	if(!m_activeWin) {
		m_activeWin = dockWin;
	}
	else if (m_inDockMode) {
		animateWindowChange (dockWin);
	}
	else {
		if (m_activeWin && m_activeWin != dockWin) {
			m_activeWin->setVisible(false);
			m_activeWin->setOpacity(1.0);
		}
		m_activeWin = dockWin;
		m_activeWin->setVisible (true);
		m_activeWin->setOpacity(1.0);
	}

}

void DockModeWindowManager::addWindow(Window* win)
{
	// Dock Mode windows are the only type accepted
	if (win->type() != Window::Type_DockModeWindow)
	    return;
	
	DockModeWindow* dockWin = static_cast<DockModeWindow*>(win);
	DockModeLaunchPoint* dlp = launchPointForId(win->appId());
	
	if(!dlp) {
		// Error: window is not in the list of apps in the launcher
		g_critical("%s: Error: window is not in the list of apps in the launcher: %s", __FUNCTION__, win->appId().c_str());
		closeWindow(win);
		return; 
	}
	
	if((dlp->state() == DockModeLaunchPoint::NotLaunched || dlp->state() == DockModeLaunchPoint::Closed)
			&& dockWin->isHost() && !dlp->isPermanent()) {
		g_critical("%s: Error: this app wasn't marked as launching: %s", __FUNCTION__, win->appId().c_str());
		// Error: this app wasn't marked as launching
		closeWindow(win);
		return; 
	}

	if(dlp->state() == DockModeLaunchPoint::Running) {
		g_critical("%s: Error: this app is already running: %s", __FUNCTION__, win->appId().c_str());
		// Error: this app wasn't marked as launching
		if(dlp->openWindow())
			closeWindow(dlp->openWindow());
		dlp->setState(DockModeLaunchPoint::NotLaunched);
	}

	if(dlp->openWindow() == dockWin) {
		if(m_windowInAnimation == 0) {
			if (!m_activeWin) {
				m_activeWin = dockWin;
			}

			m_activeWin->setVisible(true);
			Q_EMIT signalDockModeAppChanged (dlp);
			// resize the new window area to the positive space size
			dockWin->resizeEvent(m_positiveSpace.width(), m_positiveSpace.height());

			if(m_lastMaximizedWin != dockWin) {
				m_lastMaximizedWin = dockWin;
			}

			if (m_inDockMode) {
				SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, dockWin, m_activeWin == dockWin);
				dockWin->focusEvent(m_activeWin == dockWin);
			}

			dlp->setState(DockModeLaunchPoint::Running);
		}
	}  else {
			dlp->setOpenWindow(win);
			dlp->setState(DockModeLaunchPoint::Running);
			if (m_inDockMode) {
				if (!m_activeWin) {
					m_activeWin = dockWin;
					dockWin->setVisible(true);
					dockWin->setOpacity(1.0);
					dockWin->focusEvent(true);
					SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, (DockModeWindow*)dockWin, !m_inTransitionAnimation);
				} else {
					dockWin->setVisible(false);
					dockWin->setOpacity(1.0);
				}
			}
			else {
				if (m_activeWin && m_activeWin != dockWin) {
					m_activeWin->setVisible(false);
					m_activeWin->setOpacity(1.0);
				}
				m_activeWin = dockWin;
				m_activeWin->setVisible(true);
				m_activeWin->setOpacity(1.0);
			}
	}
	dockWin->setAddedToWindowManager();
}

void DockModeWindowManager::removeWindow(Window* win)
{
	g_warning ("%s: ", __func__);
	if(!win)
		return;
	
	removePendingFocusActionWindow(win);
	
	if(!win->removed()) {
		closeWindow(win);
	}

	if(win == m_activeWin) {
		m_activeWin = 0;
		Q_EMIT signalDockModeAppChanged (0);
	}

	if(win == m_lastMaximizedWin)
		m_lastMaximizedWin = 0;

	delete win;
}

bool DockModeWindowManager::appLoadingTimedOut(DockModeLaunchPoint* dlp)
{
	return true;
}

void DockModeWindowManager::switchApplication(DockModeLaunchPoint* dlp)
{
	g_message ("%s: switching app to %s", __PRETTY_FUNCTION__, dlp->launchPoint()->menuName().c_str());
	if(dlp->openWindow() == 0) {
		launchApp(dlp);
	}
	else if(dlp->openWindow() != m_activeWin) {
		m_statusBar->setMaximizedAppTitle (true, dlp->launchPoint()->menuName().c_str());
		animateWindowChange(dlp->openWindow());
	} 
	else {
		if (m_inDockMode) {
			SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, (DockModeWindow*)m_activeWin, true);
			((DockModeWindow*)m_activeWin)->focusEvent(true);
		}
	}

	if (!m_currentPuckId.empty()) {
		m_puckIdToDlpIndex[m_currentPuckId] = appIndexForId (dlp->launchPoint()->id());
		g_message ("%s: Mapped puckId %s to index %d (app %s)", __PRETTY_FUNCTION__,
				m_currentPuckId.c_str(), m_defaultIndex,
				dlp->launchPoint()->id().c_str());
		m_dockModePosMgr->addAppForPuck (m_currentPuckId, dlp->launchPoint()->id());
	}
}

void DockModeWindowManager::queueFocusAction(Window* win, bool focused)
{
	DockModeWindow* dwin = static_cast<DockModeWindow*>(win);
	
	dwin->aboutToFocusEvent(focused);
	dwin->queueFocusAction(focused);
	std::set<Window*>::const_iterator it = m_pendingActionWinSet.find(win);
	if (it == m_pendingActionWinSet.end())
		m_pendingActionWinSet.insert(win);
}

void DockModeWindowManager::performPendingFocusActions()
{
    for (std::set<Window*>::const_iterator it = m_pendingActionWinSet.begin();
		 it != m_pendingActionWinSet.end(); ++it) {
		static_cast<DockModeWindow*>(*it)->performPendingFocusAction();
	}

	m_pendingActionWinSet.clear();
}

void DockModeWindowManager::removePendingFocusActionWindow(Window* win)
{
	std::set<Window*>::const_iterator it = m_pendingActionWinSet.find(win);
	if (it != m_pendingActionWinSet.end())
		m_pendingActionWinSet.erase(it);
}


void DockModeWindowManager::setInModeAnimation(bool animating)
{
	m_inTransitionAnimation = animating;

	configureAllIconsAndWindows();
}

void DockModeWindowManager::setDockModeState(bool enabled)
{
	if (m_inDockMode == enabled) {
		return;
	}
	
	g_message ("%s: %s dock mode", __PRETTY_FUNCTION__, enabled ? "enabling" : "disabling");
	m_inDockMode = enabled;

	SystemUiController::instance()->setDockMode(m_inDockMode);
	
	if (enabled) {
		if(m_dockLpArray[m_defaultIndex]->openWindow()) {
			// maximize the last used (default) app
			m_activeWin = m_dockLpArray[m_defaultIndex]->openWindow();
			m_activeWin->setVisible(true);
			Q_EMIT signalDockModeAppChanged (m_dockLpArray[m_defaultIndex]);

			m_statusBar->setMaximizedAppTitle (true, m_dockLpArray[m_defaultIndex]->launchPoint()->menuName().c_str());
			queueFocusAction(m_dockLpArray[m_defaultIndex]->openWindow(), true);
			configureAllIconsAndWindows();
		}
		else {
			launchApp(m_dockLpArray[m_defaultIndex], true);
		}

		Q_EMIT signalDockModeStatusChanged(true);
		performPendingFocusActions();

	} else {
		if(m_activeWin) {
			m_defaultIndex = appIndexForId(m_activeWin->appId());
			SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, (DockModeWindow*)m_activeWin, false);
			((DockModeWindow*)m_activeWin)->focusEvent(false);
			configureAllIconsAndWindows(); // sets everything to default state
		} else if(m_lastMaximizedWin) {
			m_defaultIndex = appIndexForId(m_lastMaximizedWin->appId());
		} else {
			m_defaultIndex = 0;
		}
		
		if (!m_currentPuckId.empty()) {
			m_puckIdToDlpIndex[m_currentPuckId] = m_defaultIndex;
			g_message ("%s: Mapped puckId %s to index %d (app %s)", __PRETTY_FUNCTION__,
					m_currentPuckId.c_str(), m_defaultIndex,
					m_dockLpArray[m_defaultIndex]->launchPoint()->id().c_str());
			m_dockModePosMgr->addAppForPuck (m_currentPuckId, m_dockLpArray[m_defaultIndex]->launchPoint()->id());
			m_currentPuckId.clear();
		}

		if(Settings::LunaSettings()->dockModeCloseOnExit) {
			// close all apps but the default one upon exit
			for (int i = 0; i < m_dockLpArray.size(); i++) {
				if(m_dockLpArray[i]->openWindow() && i != m_defaultIndex && !m_dockLpArray[i]->isPermanent())
					closeWindow(m_dockLpArray[i]->openWindow());
			}
			m_lastMaximizedWin = 0;
		}

		Q_EMIT signalDockModeStatusChanged(false);
		clearFocus();		
	}
}

void DockModeWindowManager::slotLowMemoryActionsRequested(bool allowExpensive)
{
	// First action: Close Dock Apps except the Foreground app (in dock mode) or maybe the default app (not in dock mode)
	// Second Action: delete App Screenshots

	int spareAppIndex;

	if(m_inDockMode) {
		if(m_activeWin) {
			spareAppIndex = appIndexForId(m_activeWin->appId());
		} else if(m_lastMaximizedWin && !allowExpensive) {
			spareAppIndex = appIndexForId(m_lastMaximizedWin->appId());
		} else {
			spareAppIndex = -1;
		}
	} else {
		// close all dock apps if we hit low memory and are not in dock mode.
		spareAppIndex = -1;
	}

	for (int i = 0; i < m_dockLpArray.size(); i++) {
		if(!m_dockLpArray[i]->isPermanent() && m_dockLpArray[i]->openWindow() && i != spareAppIndex)
			closeWindow(m_dockLpArray[i]->openWindow());
		
		if(allowExpensive)
			m_dockLpArray[i]->deleteScreenShot();
	}
	
	if(allowExpensive)
		DockModeLaunchPoint::deleteStaticImages();
}

void DockModeWindowManager::slotBootFinished()
{
	m_bootFinished = true;
	/* Disabling launching the default dock app on boot till rendering offset issue is resolved.
	if(m_dockLpArray.size() > m_defaultIndex)
		launchApp(m_dockLpArray[m_defaultIndex]);
		*/
}

void DockModeWindowManager::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if(!m_inDockMode)
		return;
	
	event->accept();
}

void DockModeWindowManager::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{	
}

void DockModeWindowManager::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if(!m_inDockMode)
		return;

	QPoint releasePointParent = mapToParent(event->pos()).toPoint();
	
}

void DockModeWindowManager::resize(int width, int height)
{
	setScreenBounds(-width/2, -height/2, width, height);

	m_positiveSpace.setWidth (width);
	m_positiveSpace.setHeight (height - m_statusBar->boundingRect().height());

	for (int i = 0; i < m_dockLpArray.size(); ++i) {
		m_dockLpArray[i]->resize (m_positiveSpace.width(), m_positiveSpace.height());
		Window* win = m_dockLpArray[i]->openWindow();
		if (win) {
			bool sync = m_inDockMode && (win == m_activeWin);
			((DockModeWindow*)win)->resizeWindowBufferEvent (width, height, m_positiveSpace, sync);
		}
	}
	prepareGeometryChange();
}



bool DockModeWindowManager::enableDockModeLaunchPoint(const LaunchPoint* lp)
{
	// external api, can only add removable apps, permanent apps are within DockModeWM
	return enableDockModeLaunchPointInternal(lp, false);
}

bool DockModeWindowManager::enableDockModeLaunchPointInternal(const LaunchPoint* lp, bool isPermanent)
{
	for(int i=0; i < m_dockLpArray.size(); i++){
		if (m_dockLpArray[i]->launchPoint() == lp) {
			g_debug ("%s: Redundant enable of launchpoint for %s, ignoring the call", __PRETTY_FUNCTION__, lp->menuName().c_str());
			return true;
		}
	}

	DockModeLaunchPoint* dlp = new DockModeLaunchPoint(kAppImageWidth, kAppImageHeight, lp, isPermanent);
	m_dockLpArray.append(dlp);
	if (!isPermanent) // only removable apps will be added to the conf file 
		m_dockModePosMgr->addEnabledApp(dlp->launchPoint()->id());
	// XXX: the index has to be offset by the number of non-removable apps, so change the line above if we have more than one non-removable app

	m_appMenuContainer->addMenuItem (dlp);

	return true;
}

bool DockModeWindowManager::disableDockModeLaunchPoint(const LaunchPoint* lp)
{
	DockModeLaunchPoint* dlp = launchPointForId(lp->id()); 
	
	if(dlp) {

		if(!dlp->isPermanent() && dlp->openWindow()) {
			closeWindow(dlp->openWindow());
		}

		if (m_dockLpArray.position(dlp) == m_defaultIndex) {
			m_defaultIndex = 0;
		}
		else if(m_dockLpArray.position(dlp) < m_defaultIndex) {
			m_defaultIndex--;
		}
		
		m_dockLpArray.remove(dlp);
		m_dockModePosMgr->removeEnabledApp(lp->id());
		m_appMenuContainer->removeMenuItem (dlp);
		
		delete dlp;

		return true;
	}
	return false;
}

void DockModeWindowManager::updateDockModeLaunchPoint(const LaunchPoint* lp)
{
	// $$$ TODO
}

void DockModeWindowManager::addPuckIdAndDefaultApp (const std::string& puckId, const std::string& appId)
{
	if (puckId.empty() || appId.empty()) {
		g_warning ("%s: Invalid parameters puckId: %s appId: %s", __PRETTY_FUNCTION__, puckId.c_str(), appId.c_str());
		return;
	}

	int index = appIndexForId (appId);
	if (index == -1)  {
		g_warning ("%s: Unknown app %s", __PRETTY_FUNCTION__, appId.c_str());
		return;
	}

	m_puckIdToDlpIndex[puckId] = index;
	g_message ("%s: Mapped puckId %s to index %d (app %s)", __PRETTY_FUNCTION__, puckId.c_str(), index, appId.c_str());
	m_dockModePosMgr->addAppForPuck (puckId, appId);
}

bool DockModeWindowManager::launchApp(DockModeLaunchPoint* dockApp, bool maximized)
{
	g_message("%s: appId = %s", __PRETTY_FUNCTION__, dockApp->launchPoint()->id().c_str());
	std::string errMsg, procId, launchingProcId;
	
	if((dockApp->state() == DockModeLaunchPoint::NotLaunched) || 
	   (dockApp->state() == DockModeLaunchPoint::Closed)      ||
	   (dockApp->state() == DockModeLaunchPoint::Launching)    ) {
		json_object* root = json_object_new_object();
		json_object_object_add(root, "windowType", json_object_new_string("dockModeWindow")); // $$$
		json_object_object_add(root, "dockMode", json_object_new_boolean(true));
		
		procId = WebAppMgrProxy::instance()->appLaunch(dockApp->launchPoint()->id(), json_object_to_json_string(root), kDockLauncherAppId, "", errMsg);
		json_object_put(root);
		
		if (procId.empty()) {
			g_warning("%s: Failed to launch app: %s. Error = %s", __FUNCTION__, dockApp->launchPoint()->id().c_str(), errMsg.c_str());
			return false;
		}	
		
		dockApp->setState(DockModeLaunchPoint::Launching);
	}	
	
	return true;
}

void DockModeWindowManager::closeApp(const std::string appId)
{
	DockModeLaunchPoint* dlp = launchPointForId(appId);
	
	if(m_sysUiLpInUse == dlp)
		m_sysUiLpInUse = 0;
	
	if (dlp && !dlp->isPermanent() && dlp->openWindow()) {
		Window* win = dlp->openWindow();
		closeWindow(win);
	}
}

void DockModeWindowManager::closeWindow(Window* win)
{
	if(!win)
		return;
	
	Window* closingWindow = win;
	DockModeLaunchPoint* dlp = launchPointForId(win->appId());
	
	if(m_sysUiLpInUse == dlp)
		m_sysUiLpInUse = 0;

	if(closingWindow->type() == Window::Type_DockModeLoadingWindow) {
		closingWindow->setVisible(false);
		closingWindow->setRemoved();
	
		if(closingWindow == m_activeWin) {
			m_activeWin = 0;
			Q_EMIT signalDockModeAppChanged(0);
			if(m_inDockMode)
				configureAllIconsAndWindows();
		}
		
		if(closingWindow == m_lastMaximizedWin)
			m_lastMaximizedWin = 0;
		
		if (dlp) {
			dlp->setState(DockModeLaunchPoint::Closed);
			
			closingWindow = dlp->openWindow();
		} else {
			closingWindow = 0;
		}
	}

	if(!closingWindow) 
		return;
	
	if(closingWindow->type() == Window::Type_DockModeWindow) {
		closingWindow->setVisible(false);
		SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, (DockModeWindow*)closingWindow, false);
		closingWindow->setRemoved();
	
		if (dlp) {
			dlp->setState (DockModeLaunchPoint::Closed);
			dlp->setOpenWindow(0);
		}
		
		if(closingWindow == m_activeWin) {
			m_activeWin = 0;
			Q_EMIT signalDockModeAppChanged(0);
			if(m_inDockMode)
				configureAllIconsAndWindows();
		}
		
		if(closingWindow == m_lastMaximizedWin)
			m_lastMaximizedWin = 0;
	
		closingWindow->setParentItem(0);	
	
		removePendingFocusActionWindow(closingWindow);
	
		static_cast<DockModeWindow*>(closingWindow)->close();
	} 
	
	if(dlp == m_sysUiLpInUse)
		m_sysUiLpInUse = 0;
}


void DockModeWindowManager::configureAllIconsAndWindows()
{
	if (!m_inDockMode)
		return;

	// A card is Maximized
	// configure the maximized window
	if (!m_activeWin) {
		return;
	}

	SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, (DockModeWindow*)m_activeWin, !m_inTransitionAnimation);
	((DockModeWindow*)m_activeWin)->setPaintCompositionMode(m_inTransitionAnimation ? QPainter::CompositionMode_SourceOver : QPainter::CompositionMode_Source);

	for(int i=0; i < m_dockLpArray.size(); i++){
		if(m_dockLpArray[i]->openWindow()) {
			DockModeWindow* dockWin = (DockModeWindow*)m_dockLpArray[i]->openWindow();
			if(dockWin != m_activeWin) {			
				// hide the minimized windows
				dockWin->setVisible(false);
				SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, dockWin, false);
			}
		}
	}
}


void DockModeWindowManager::animateWindowChange(Window* win)
{
	if(!win || !m_activeWin || !m_inDockMode) {
		g_warning ("%s: cannot animate, win %p, m_activeWin %p, m_inDockMode %d", __PRETTY_FUNCTION__, win, m_activeWin, m_inDockMode);
		return;
	}
	
	if(m_windowInAnimation) { // currently in an animation, so just ignore this one
		g_warning ("%s: cannot animate, window already in animation", __PRETTY_FUNCTION__);
		return;
	}
	
	if(m_dashboardOpened) {
		Q_EMIT signalCloseDashboard(false);
	}

	// first, position the new window correctly offscreen
	win->setPos(m_activeWin->pos());
	win->setVisible(true);
	win->setOpacity (0.0);
	
	queueFocusAction(win, true);
	queueFocusAction(m_activeWin, false);
	
	SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, (DockModeWindow*)win, false);
	SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, (DockModeWindow*)m_activeWin, false);
	((DockModeWindow*)m_activeWin)->setPaintCompositionMode(QPainter::CompositionMode_SourceOver);
	
	m_nextActiveWin = win;
	
	// now animate the window transition
	m_windowInAnimation = m_activeWin;
	m_windowAnimationCurrent.setTargetObject(NULL);
	m_windowAnimationCurrent.setTargetObject(m_activeWin);
	m_windowAnimationCurrent.setStartValue(1.0);
	m_windowAnimationCurrent.setEndValue(0.0);
	
	m_windowAnimationNext.setTargetObject(NULL);
	m_windowAnimationNext.setTargetObject(win);
	m_windowAnimationNext.setStartValue(0.0);
	m_windowAnimationNext.setEndValue(1.0);
	
	m_windowChangeAnimationGroup.start();
}

void DockModeWindowManager::slotWindowChangeAnimationFinished()
{
	if (m_activeWin) {
		m_activeWin->setVisible(false);
		m_activeWin->setOpacity(1.0);
		((DockModeWindow*)m_activeWin)->resizeEvent(m_positiveSpace.width(), m_positiveSpace.height());
	}
	
	// update the active window
	m_activeWin = m_nextActiveWin;

	if (m_activeWin) {
		// resize the new window area to the positive space size
		((DockModeWindow*)m_activeWin)->resizeEvent(m_positiveSpace.width(), m_positiveSpace.height());
		Q_EMIT signalDockModeAppChanged (launchPointForId (m_activeWin->appId()));
	}

	m_nextActiveWin = 0;
	m_windowInAnimation = 0;
	m_windowAnimationCurrent.setTargetObject(NULL);
	m_windowAnimationNext.setTargetObject(NULL);
	
	configureAllIconsAndWindows();
	performPendingFocusActions();
}


void DockModeWindowManager::slotDockModeAppSelected (DockModeLaunchPoint* dlp)
{
	switchApplication (dlp);
}

void DockModeWindowManager::slotDockModeAnimationStarted()
{
	if (m_activeWin) {
		SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, (DockModeWindow*)m_activeWin, false);
		((DockModeWindow*)m_activeWin)->focusEvent(false);
	}
}

void DockModeWindowManager::slotDockModeAnimationComplete()
{
	if (m_inDockMode) {
		int index = -1;
		
		if ((index = findDefaultDlpIndex(DisplayManager::instance()->puckId())) != -1) {
			g_message ("%s: puckId %s default index %d", __PRETTY_FUNCTION__, m_currentPuckId.c_str(), index);
			switchApplication (m_dockLpArray[index]);
		}
		else if (m_activeWin) {
			SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DOCK_MODE_WINDOW_MANAGER, (DockModeWindow*)m_activeWin, true);
			((DockModeWindow*)m_activeWin)->focusEvent(true);
		}
	}
}

void DockModeWindowManager::slotPuckConnected(bool connected)
{
	if (!connected) {
		g_debug ("%s: nothing to do on disconnect", __PRETTY_FUNCTION__);
		return;
	}

	if (m_inDockMode) {
		g_debug ("%s: if already in dock mode, leave the active app running, do nothing", __PRETTY_FUNCTION__);
		return;
	}

	if (isVisible()) {
		g_debug ("%s: if dock mode is already visible, dont muck with it", __PRETTY_FUNCTION__);
		return;
	}

	int index = -1;

	if ((index = findDefaultDlpIndex(DisplayManager::instance()->puckId())) != -1) {
		if (m_dockLpArray[index]->openWindow() == 0 || m_activeWin != m_dockLpArray[index]->openWindow()) 
		{
			m_defaultIndex = index;
			if (m_dockLpArray[m_defaultIndex]->openWindow()) {
				g_message ("%s: setting active window to default app %s", __PRETTY_FUNCTION__, m_dockLpArray[m_defaultIndex]->launchPoint()->menuName().c_str());
				m_statusBar->setMaximizedAppTitle (true, m_dockLpArray[m_defaultIndex]->launchPoint()->menuName().c_str());
				m_activeWin = m_dockLpArray[m_defaultIndex]->openWindow();
				m_activeWin->setVisible(true);
				Q_EMIT signalDockModeAppChanged (m_dockLpArray[m_defaultIndex]);
			}
			else if (m_bootFinished) { // launching the app before boot finished results in a black screen
				g_message ("%s: prelaunching default app %s", __PRETTY_FUNCTION__, m_dockLpArray[m_defaultIndex]->launchPoint()->menuName().c_str());
				launchApp(m_dockLpArray[m_defaultIndex], true);
			}
		}
	}
	else if (!m_currentPuckId.empty()) {
		m_puckIdToDlpIndex[m_currentPuckId] = m_defaultIndex;
		g_message ("%s: Mapped puckId %s to index %d (app %s)", __PRETTY_FUNCTION__,
				m_currentPuckId.c_str(), m_defaultIndex,
				m_dockLpArray[m_defaultIndex]->launchPoint()->id().c_str());
		m_dockModePosMgr->addAppForPuck (m_currentPuckId, m_dockLpArray[m_defaultIndex]->launchPoint()->id());
	}


}

// finding the default index in m_dockLpArray for this specific puck
int DockModeWindowManager::findDefaultDlpIndex(const std::string& puckId)
{
	int index = -1;

	if (!puckId.empty()) {
		m_currentPuckId = puckId;
		if (m_puckIdToDlpIndex.find (puckId) != m_puckIdToDlpIndex.end()) {
			index = m_puckIdToDlpIndex[puckId];
			if (index < 0 || index >= m_dockLpArray.size()) {
				index = 0;
			}
		}
		else {
			g_debug ("%s: unknown puck", __PRETTY_FUNCTION__);
		}
	}
	else {
		g_debug ("%s: no puck id", __PRETTY_FUNCTION__);
	}
	return index;
}

void DockModeWindowManager::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	m_previous = painter->compositionMode();
	painter->setCompositionMode(m_compMode);

	painter->fillRect (boundingRect(), QColor (0, 0, 0, 0xff));

	painter->setCompositionMode(m_previous);

}

void DockModeWindowManager::slotLaunchPointRemoved(const LaunchPoint* lp, QBitArray reasons) {

	//This will remove the DockModeLaunchPoint from the array of launch points
	//and it will close its window and update the current window to something
	//else and perform various cleanup.

	disableDockModeLaunchPoint(lp);

}
