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

#include "WindowServerLuna.h"

#include "WindowManagerBase.h"

#include "CardWindowManager.h"
#include "CardWindow.h"
#include "DashboardWindowManager.h"
#include "EmergencyWindowManager.h"
#include "FullEraseConfirmationWindow.h"
#include "Logging.h"
#include "OverlayWindowManager.h"
#include "MenuWindowManager.h"
#include "TopLevelWindowManager.h"
#include "MetaKeyManager.h"
#include "CoreNaviManager.h"
#include "SystemUiController.h"
#include "SystemService.h"
#include "cjson/json.h"
#include "Preferences.h"
#include "NativeAlertManager.h"
#include "DockModeWindowManager.h"
#include "DockModeMenuManager.h"
#include "AnimationSettings.h"
#include "WebAppMgrProxy.h"
#include "ReticleItem.h"
#include "HostBase.h"
#include "InputWindowManager.h"
#include "Preferences.h"
#include "Localization.h"
#include "MemoryMonitor.h"
#include "UiNavigationController.h"

#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>

#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
#include <QGLContext>
#include <QGLFramebufferObject>
#endif

#include <QCoreApplication>

static const char* kWindowSrvChnl = "WindowServer";
static const int kAlertWindowWidth = 320;


WindowServerLuna::WindowServerLuna()
	: m_overlayMgr(0)
	, m_cardMgr(0)
	, m_dashboardMgr(0)
	, m_menuMgr(0)
	, m_topLevelMgr(0)
	, m_emergencyModeMgr(0)
	, m_fullEraseConfirmationWindow(0)
	, m_fullErasePending(false)
	, m_wallpaperFullScreen(false)
	, m_currWallpaperImg(0)
	, m_drawWallpaper(true)
	, m_screenShot(NULL)
	, m_dockImage (NULL)
	, m_screenShotObject (NULL)
	, m_dockImageObject (NULL)
	, m_dashboardOpenInDockMode (true)
	, m_qmlEngine(0)
	, m_inDockModeTransition(false)
	, m_dockModeTransitionDirection(false)
{
	// Cache the wallpaper in a QPixmapCache to improve speed
        setCacheMode(QGraphicsView::CacheBackground);

	m_qmlEngine = new QDeclarativeEngine;
	m_qmlEngine->rootContext()->setContextProperty("runtime", Runtime::instance());

	//the map is in the base class (WindowServerBase)
	m_cardMgr = new CardWindowManager(m_screenWidth, m_screenHeight);
	m_windowManagerMap.insert(QString(m_cardMgr->metaObject()->className()),m_cardMgr);

	m_menuMgr = new MenuWindowManager(m_screenWidth, m_screenHeight);
	m_windowManagerMap.insert(QString(m_menuMgr->metaObject()->className()),m_menuMgr);

	m_dashboardMgr = new DashboardWindowManager(m_screenWidth, m_screenHeight);
	m_windowManagerMap.insert(QString(m_dashboardMgr->metaObject()->className()),m_dashboardMgr);

	m_topLevelMgr = new TopLevelWindowManager(m_screenWidth, m_screenHeight);
	m_windowManagerMap.insert(QString(m_topLevelMgr->metaObject()->className()),m_topLevelMgr);

	m_emergencyModeMgr = new EmergencyWindowManager(m_screenWidth, m_screenHeight);
	m_windowManagerMap.insert(QString(m_emergencyModeMgr->metaObject()->className()),m_emergencyModeMgr);

	m_overlayMgr = new OverlayWindowManager(m_screenWidth, m_screenHeight);
	m_windowManagerMap.insert(QString(m_overlayMgr->metaObject()->className()),m_overlayMgr);

	m_dockModeMenuMgr = new DockModeMenuManager(m_screenWidth, m_screenHeight);
	m_windowManagerMap.insert(QString(m_dockModeMenuMgr->metaObject()->className()),m_dockModeMenuMgr);

	m_dockModeMgr = new DockModeWindowManager(m_screenWidth, m_screenHeight);
	m_windowManagerMap.insert(QString(m_dockModeMgr->metaObject()->className()),m_dockModeMgr);

	if (Settings::LunaSettings()->virtualKeyboardEnabled)
	{
		m_inputWindowMgr = new InputWindowManager(m_screenWidth, m_screenHeight);
		m_windowManagerMap.insert(QString(m_inputWindowMgr->metaObject()->className()),m_inputWindowMgr);
	}

	m_cardMgr->init();
	m_overlayMgr->init();
	m_dashboardMgr->init();
	m_menuMgr->init();
	m_topLevelMgr->init();
	m_emergencyModeMgr->init();

	m_dockModeMenuMgr->init();
	m_dockModeMgr->init();

	if (m_inputWindowMgr)
		m_inputWindowMgr->init();

	// Keep wallpaper up-to-date
	connect(Preferences::instance(), SIGNAL(signalWallPaperChanged(const char*)),
			this, SLOT(slotWallPaperChanged(const char*)));

	m_currWallpaperImg = &m_normalWallpaperImage;

	m_uiRootItem.setBoundingRect(QRectF(-SystemUiController::instance()->currentUiWidth()/2, -SystemUiController::instance()->currentUiHeight()/2,
						         SystemUiController::instance()->currentUiWidth(), SystemUiController::instance()->currentUiHeight()));

	m_dockModeMgr->setParentItem (&m_uiRootItem);
	m_dockModeMenuMgr->setParentItem (&m_uiRootItem);
	m_cardMgr->setParentItem(&m_uiRootItem);
	m_overlayMgr->setParentItem(&m_uiRootItem);
	m_emergencyModeMgr->setParentItem(&m_uiRootItem);   // Temporarily demoted Emergency Mode WM in Dartfish, placed under Dashboard WM (it is being used for Flash full screen)
														// WARNING: This has to have a matching change in the Direct Rendering layers order, defined in SystemUiController.h (enum DirectRenderingEnabledLayers)
	m_dashboardMgr->setParentItem(&m_uiRootItem);
	m_menuMgr->setParentItem(&m_uiRootItem);
	m_topLevelMgr->setParentItem(&m_uiRootItem);
	if (m_inputWindowMgr)
		m_inputWindowMgr->setParentItem(&m_uiRootItem);

    UiNavigationController* unc = UiNavigationController::instance();
    unc->registerNavigator(m_dockModeMgr);
    unc->registerNavigator(m_emergencyModeMgr);
    unc->registerNavigator(m_topLevelMgr);
    unc->registerNavigator(m_overlayMgr);
    unc->registerNavigator(m_cardMgr);

	m_dockModeMgr->setPos (0,0);
	m_dockModeMenuMgr->setPos (0,0);
	m_cardMgr->setPos(0, 0);
	m_overlayMgr->setPos(0, 0);
	m_menuMgr->setPos(0, 0);
	m_dashboardMgr->setPos(0, 0);
	m_topLevelMgr->setPos(0, 0);
	m_emergencyModeMgr->setPos(0, 0);
	if (m_inputWindowMgr)
		m_inputWindowMgr->setPos(0, 0);

	scene()->addItem(&m_uiRootItem);

	SystemUiController::instance()->setUiRootItemPtr(&m_uiRootItem);

	// always centered
	m_uiRootItem.setPos(SystemUiController::instance()->currentUiWidth()/2, SystemUiController::instance()->currentUiHeight()/2);

	// Trigger the initial layout for the WMs
	SystemUiController::instance()->init();

	connect(WindowServer::instance()->displayManager(), SIGNAL(signalDisplayStateChange(int)),
			this, SLOT(slotDisplayStateChange(int)));

	connect(m_topLevelMgr, SIGNAL(signalScreenLockStatusChanged(bool)),
			this, SLOT(slotScreenLockStatusChanged(bool)));

	initDockModeAnimations();
	reorderWindowManagersForDockMode(false);

	//start the thing that will deal with native alert windows
	NativeAlertManager::instance();

	// Connect to memory monitor to show memory alert when necessary
	connect(MemoryMonitor::instance(),
			SIGNAL(memoryStateChanged(bool)),
			SLOT(slotMemoryStateChanged(bool)));

	connect(WebAppMgrProxy::instance(),
			SIGNAL(signalAppLaunchPreventedUnderLowMemory()),
			SLOT(slotAppLaunchPreventedUnderLowMemory()));

	connect(SystemService::instance(),
			SIGNAL(signalBrickModeFailed()),
			SLOT(slotBrickModeFailed()));

        connect(m_cardMgr,
                        SIGNAL(signalFirstCardRun()),
                        SLOT(slotFirstCardRun()));

	setBackgroundBrush(Qt::darkGray);
}

WindowServerLuna::~WindowServerLuna()
{
}

QRectF WindowServerLuna::mapRectToRoot(const QGraphicsItem* item, const QRectF& rect) const
{
    return (item ? item->mapRectToItem(&m_uiRootItem, rect) : rect);
}


bool WindowServerLuna::okToResizeUi(bool ignorePendingRequests)
{
	if((m_inRotationAnimation != Rotation_NoAnimation) || m_inDockModeTransition) {
		return false;
	}
	if(!ignorePendingRequests && !m_pendingFlipRequests.empty()) {
		return false;
	}
	if(!SystemUiController::instance()->okToResizeUi()) {
		return false;
	}
	if(m_overlayMgr && !m_overlayMgr->okToResize()) {
		return false;
	}
	if(m_cardMgr && !m_cardMgr->okToResize()) {
		return false;
	}
	if(m_dashboardMgr && !m_dashboardMgr->okToResize()) {
		return false;
	}
	if(m_menuMgr && !m_menuMgr->okToResize()) {
		return false;
	}
	if(m_topLevelMgr && !m_topLevelMgr->okToResize()) {
		return false;
	}
	if(m_emergencyModeMgr && !m_emergencyModeMgr->okToResize()) {
		return false;
	}
	if(m_inputWindowMgr && !m_inputWindowMgr->okToResize()) {
		return false;
	}
	if(m_dockModeMenuMgr && !m_dockModeMenuMgr->okToResize()) {
		return false;
	}
	if(m_dockModeMgr && !m_dockModeMgr->okToResize()) {
		return false;
	}

	return true;
}

void WindowServerLuna::resizeWindowManagers(int width, int height)
{
	WebAppMgrProxy::instance()->uiDimensionsChanged(width, height);

	m_uiRootItem.setBoundingRect(QRectF(-SystemUiController::instance()->currentUiWidth()/2, -SystemUiController::instance()->currentUiHeight()/2,
								         SystemUiController::instance()->currentUiWidth(), SystemUiController::instance()->currentUiHeight()));

	// clean up any windows still pending a resize
	m_pendingFlipRequests.clear();

	if(m_overlayMgr)
		m_overlayMgr->resize(width, height);

	if(m_cardMgr)
		m_cardMgr->resize(width, height);

	if(m_dashboardMgr)
		m_dashboardMgr->resize(width, height);

	if(m_menuMgr)
		m_menuMgr->resize(width, height);

	if(m_topLevelMgr)
		m_topLevelMgr->resize(width, height);

	if(m_dockModeMenuMgr)
		m_dockModeMenuMgr->resize(width, height);

	if(m_dockModeMgr)
		m_dockModeMgr->resize(width, height);

	if(m_emergencyModeMgr)
		m_emergencyModeMgr->resize(width, height);

	if(m_inputWindowMgr)
		m_inputWindowMgr->resize(width, height);
}

void WindowServerLuna::prepareAddWindow(Window* win)
{
	WindowManagerBase* wm = windowManagerForWindow(win);
	if (!wm)
		return;

	wm->prepareAddWindow(win);
}


void WindowServerLuna::addWindowTimedOut(Window* win)
{
	WindowManagerBase* wm = windowManagerForWindow(win);
	if (!wm)
		return;

	wm->addWindowTimedOut(win);
}


void WindowServerLuna::removeWindow(Window* win)
{
	WindowManagerBase* wm = windowManagerForWindow(win);
	if (!wm)
		return;

	wm->removeWindow(win);
}

void WindowServerLuna::addWindow(Window* win)
{
	WindowManagerBase* wm = windowManagerForWindow(win);
	if (!wm)
		return;

	wm->addWindow(win);
}

void WindowServerLuna::focusWindow(Window* win)
{
	WindowManagerBase* wm = windowManagerForWindow(win);
	if (!wm)
		return;

	wm->focusWindow(win);
}

void WindowServerLuna::unfocusWindow(Window* win)
{
	WindowManagerBase* wm = windowManagerForWindow(win);
	if (!wm)
		return;

	wm->unfocusWindow(win);
}

WindowManagerBase* WindowServerLuna::windowManagerForWindow(Window* win) const
{
	return windowManagerForWindowType(win->type());
}

WindowManagerBase* WindowServerLuna::windowManagerForWindowType(int type) const
{
	WindowManagerBase* wm = 0;

	switch (type) {
	case (Window::Type_ModalChildWindowCard):
	case (Window::Type_Card):
		wm = m_cardMgr; break;
	case (Window::Type_DockModeWindow):
		wm = m_dockModeMgr; break;
	case (Window::Type_Overlay):
	case (Window::Type_Launcher):
	case (Window::Type_QtNativePaintWindow):
		wm = m_overlayMgr; break;
	case (Window::Type_Dashboard):
	case (Window::Type_PopupAlert):
	case (Window::Type_BannerAlert):
		wm = m_dashboardMgr; break;
	case (Window::Type_StatusBar):
	case (Window::Type_Menu):
		wm = m_menuMgr; break;
	case (Window::Type_PIN):
		wm = m_topLevelMgr; break;
	case (Window::Type_Emergency):
		wm = m_emergencyModeMgr; break;
	default:
		g_critical("Unknown window type (%d) specified\n", type);
		wm = 0; break;
	}

	return wm;
}

void WindowServerLuna::windowUpdated(Window* win)
{
	if (!windowIsRegistered(win))
		return;

	Q_EMIT signalWindowUpdated(win);
}

void WindowServerLuna::startDrag(int x, int y, void* imgRef, const std::string& lpid)

{
}

void WindowServerLuna::endDrag(int x, int y, const std::string& lpid, bool handled)
{
}

/*
 * If the user holds the opt/sym key and then press and release the power key to turn off the sreen,
 * and then release the opt/sym key, then we don't receive the key up event for the opt/sym key.
 * That's why we listen for the display off event to reset the flags for the opt/sym key.
 */
void WindowServerLuna::slotDisplayStateChange(int state)
{
	if (state == DISPLAY_SIGNAL_OFF) {
		g_message ("%s: display signal off", __PRETTY_FUNCTION__);
		m_powerVolumeKeyComboState.reset();
		cancelFullEraseCountdown();
		if(SystemUiController::instance()->isInDockMode()
				&& m_dockModeAnimation.state() != QAbstractAnimation::Running
				&& !DisplayManager::instance()->isOnPuck()) 
		{
			//if the display goes OFF while we are in Dock Mode but NOT onthe Dock, the leave Dock Mode
			dockModeUiTransition(false);
		}
	} else if (state == DISPLAY_SIGNAL_DOCK) {
		g_message ("%s: display signal dock, systemuicontroller is %s dock mode ", __PRETTY_FUNCTION__,
				SystemUiController::instance()->isInDockMode() ? "in" : "not in");
		if (!SystemUiController::instance()->isInDockMode() || m_inDockModeTransition)
			dockModeUiTransition(true);
	} else if (state == DISPLAY_SIGNAL_ON) {
		g_message ("%s: display signal on, systemuicontroller is %s dock mode ", __PRETTY_FUNCTION__,
				SystemUiController::instance()->isInDockMode() ? "in" : "not in");
		if (SystemUiController::instance()->isInDockMode() || m_inDockModeTransition)
			dockModeUiTransition(false);
	}
}

// signaled when System UI is forcing a Dock mode UI change when we are NOT on the Dock.
void WindowServerLuna::slotDockModeEnable(bool enabled)
{
	dockModeUiTransition(enabled);
}

void WindowServerLuna::slotPuckConnected(bool connected)
{
	if((!connected) && (SystemUiController::instance()->isInDockMode())) {
		if(!DisplayManager::instance()->isOn()) {
			//if the device leaves the Dock while in Dock Mode and Display OFF, then trigger a Dock Mode exit transition
			g_warning ("Triggeing dock mode exit from %s", __PRETTY_FUNCTION__);
			dockModeUiTransition(false);
		}
	}
}

void WindowServerLuna::slotScreenLockStatusChanged(bool locked)
{
	// in Dock Mode the Dock Mode settings override the Lock Screen settings for the visibility of the window managers
	if(SystemUiController::instance()->isInDockMode())
		return;
	
	if(locked) {
        cacheFocusedItem();
		// Lock screen is visible, so make all window managers under it invisible
		m_cardMgr->setVisible(false);
		m_emergencyModeMgr->setVisible(false);
		m_menuMgr->setVisible(false);
		m_dashboardMgr->setVisible(false);
		m_overlayMgr->setVisible(false);
        restoreCachedFocusItem();
		m_drawWallpaper = false;
	} else {
		// Lock screen is no longer visible, so make all window managers visible
		m_cardMgr->setVisible(true);
		m_emergencyModeMgr->setVisible(true);
		m_menuMgr->setVisible(true);
		m_dashboardMgr->setVisible(true);
		m_overlayMgr->setVisible(true);
		m_drawWallpaper = true;

		//give overlay window manager focus if there is no app maximized
		if (!SystemUiController::instance()->isCardWindowMaximized() && !SystemUiController::instance()->isInDockMode())
		{
			m_overlayMgr->setFlag(QGraphicsItem::ItemIsFocusable, true);
			m_overlayMgr->setFocus();
			g_message("%s: [NOV-115353] overlaywm getting focus",__FUNCTION__);
		}
	}
        resetCachedContent(); // Reset wallpaper cache
}

void WindowServerLuna::dockModeUiTransition(bool enter)
{
	// if in transition already to the intended direction, ignore this request
	if (m_inDockModeTransition && m_dockModeTransitionDirection == enter)
		return;

	if (m_inDockModeTransition) { // cleanup if already in dock mode animation.

		g_message ("%s: In Dock Mode transition, m_dockModeAnimation is %s", __PRETTY_FUNCTION__, 
				m_dockModeAnimation.state() == QAbstractAnimation::Running ? "running" : "NOT running");
		if (m_dockModeAnimation.state() == QAbstractAnimation::Running) {
			g_message ("%s: stopping dock mode animations", __PRETTY_FUNCTION__);
			m_dockModeAnimation.stop();

			m_screenShotObject->setVisible(false);
			m_dockImageObject->setVisible(false);
			m_uiRootItem.setVisible (true);

			cleanupDockModeAnimations();
		}
		else {
			g_message ("%s: in dock mode transition but unable to stop it", __PRETTY_FUNCTION__);
		}
		m_inDockModeTransition = false;
	}

	if(!DisplayManager::instance()->isOn()) {
		g_message ("%s: Display is Off, instant transition, m_inDockModeTransition %d", __PRETTY_FUNCTION__, m_inDockModeTransition);

		m_inDockModeTransition = true;
		m_dockModeTransitionDirection = enter;

		reorderWindowManagersForDockMode(enter);
		((DockModeWindowManager*)m_dockModeMgr)->setInModeAnimation(false);
		((DockModeWindowManager*)m_dockModeMgr)->setDockModeState(enter);

		m_inDockModeTransition = false;
		update();
	} else {
		g_message ("%s: Animate dock mode %s display is on and dashboard is closed", __PRETTY_FUNCTION__, enter ? "in" : "out");

		// display on, so run the transition animation
		m_inDockModeTransition = true;
		m_dockModeTransitionDirection = enter;

		Q_EMIT signalDockModeAnimationStarted();

		animateDockMode(enter);
	}
}

void WindowServerLuna::reorderWindowManagersForDockMode(bool enabled)
{
	if(enabled) {
		// Move dashboardMgr to top
		m_dashboardMgr->stackBefore(m_emergencyModeMgr);

        cacheFocusedItem();
		//Dock mode is visible, so make all window managers under it invisible
		m_cardMgr->setVisible(false);
		m_menuMgr->setVisible(false);
		m_overlayMgr->setVisible(false);
		m_emergencyModeMgr->setVisible(false);
		m_topLevelMgr->setVisible(false);
		m_dockModeMenuMgr->setVisible(true);
		m_dockModeMgr->setVisible(true);
		m_dashboardMgr->setVisible(true);
        restoreCachedFocusItem();
		m_drawWallpaper = false;
	} else {
		// move dashboardMgr back down
		m_dashboardMgr->stackBefore(m_topLevelMgr);

		// Dock mode is no longer visible, so make all window managers visible
		m_topLevelMgr->setVisible(true);
		m_dockModeMenuMgr->setVisible(false);
		m_dockModeMgr->setVisible(false);

		if(SystemUiController::instance()->isScreenLocked()) {
            cacheFocusedItem();
			m_cardMgr->setVisible(false);
			m_menuMgr->setVisible(false);
			m_dashboardMgr->setVisible(false);
			m_overlayMgr->setVisible(false);
			m_emergencyModeMgr->setVisible(false);
            restoreCachedFocusItem();
			m_drawWallpaper = false;
		} else {
			m_cardMgr->setVisible(true);
			m_menuMgr->setVisible(true);
			m_dashboardMgr->setVisible(true);
			m_overlayMgr->setVisible(true);
			m_emergencyModeMgr->setVisible(true);
			m_drawWallpaper = true;
		}
        
	}
}

void WindowServerLuna::initDockModeAnimations()
{
	// set up all the layer animations here

	m_screenFade.setPropertyName("opacity");
	m_screenFade.setDuration(AS(dockFadeScreenAnimationDuration));
	m_screenFade.setEasingCurve((QEasingCurve::Type)AS(dockFadeAnimationCurve));
	m_screenFade.setStartValue(1.0);
	m_screenFade.setEndValue(0.0);

	m_screenScale.setPropertyName("scale");
	m_screenScale.setDuration(AS(dockFadeScreenAnimationDuration));
	m_screenScale.setEasingCurve((QEasingCurve::Type)AS(dockFadeAnimationCurve));
	m_screenScale.setStartValue(1.0);
	m_screenScale.setEndValue(0.0);

	m_dockFade.setPropertyName("opacity");
	m_dockFade.setDuration(AS(dockFadeDockAnimationDuration));
	m_dockFade.setEasingCurve((QEasingCurve::Type)AS(dockFadeAnimationCurve));
	m_dockFade.setStartValue(0.0);
	m_dockFade.setEndValue(1.0);

	m_dockScale.setPropertyName("scale");
	m_dockScale.setDuration(AS(dockFadeDockAnimationDuration));
	m_dockScale.setEasingCurve((QEasingCurve::Type)AS(dockFadeAnimationCurve));
	m_dockScale.setStartValue(2.0);
	m_dockScale.setEndValue(1.0);

	m_dockGroup.addAnimation(&m_dockFade);
	m_dockGroup.addAnimation(&m_dockScale);

	m_dockSequence.addPause(AS(dockFadeDockStartDelay));
	m_dockSequence.addAnimation(&m_dockGroup);

	m_screenGroup.addAnimation(&m_screenFade);
	m_screenGroup.addAnimation(&m_screenScale);
	m_screenGroup.addAnimation(&m_dockSequence);

	m_dockModeAnimation.addAnimation(&m_screenGroup);
	connect(&m_dockModeAnimation, SIGNAL(finished()), SLOT(slotDockAnimationFinished()));
	connect(SystemUiController::instance(), SIGNAL(signalDockModeEnable(bool)), this, SLOT(slotDockModeEnable(bool)));
	connect(DisplayManager::instance(), SIGNAL(signalPuckConnected(bool)), this, SLOT(slotPuckConnected(bool)));
}

void WindowServerLuna::animateDockMode(bool in)
{
	g_message ("%s: in %d", __PRETTY_FUNCTION__, in);
	
	// this will force a disable direct rendering on any maximized cards
	//SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::DISABLE_ALL_DIRECT_RENDERING, NULL, true);		
	SystemUiController::instance()->enableDirectRendering(false);
	((DockModeWindowManager*)m_dockModeMgr)->setInModeAnimation(true);

	setupDockModeAnimations();

	if(in) {
		m_dockModeAnimation.setDirection(QAbstractAnimation::Forward);
		m_screenShotObject->setOpacity(1.0);
		m_dockImageObject->setOpacity(0.0);

	} else {
		m_dockModeAnimation.setDirection(QAbstractAnimation::Backward);
		m_screenShotObject->setOpacity(0.0);
		m_dockImageObject->setOpacity(1.0);
	}

	m_screenShotObject->setVisible (true);
	m_dockImageObject->setVisible (true);
    cacheFocusedItem();
	m_uiRootItem.setVisible (false);
    restoreCachedFocusItem();
	m_dockModeAnimation.start();
}


void WindowServerLuna::slotDockAnimationFinished()
{
	if(m_dockModeAnimation.direction() == QAbstractAnimation::Forward) {
		((DockModeWindowManager*)m_dockModeMgr)->setDockModeState(true);
		reorderWindowManagersForDockMode(true);
	} else {
		((DockModeWindowManager*)m_dockModeMgr)->setDockModeState(false);
		reorderWindowManagersForDockMode(false);
		
		if(!SystemUiController::instance()->isCardWindowMaximized()
				&& !SystemUiController::instance()->isScreenLocked()) 
		{
			// Card Minimized, screen unlocked, focus the OverlayWM
			m_overlayMgr->setFlag(QGraphicsItem::ItemIsFocusable, true);
			m_overlayMgr->setFocus();
		} 
	}

	((DockModeWindowManager*)m_dockModeMgr)->setInModeAnimation(false);
	SystemUiController::instance()->enableDirectRendering(true);

	m_screenShotObject->setVisible (false);
	m_dockImageObject->setVisible (false);
	m_uiRootItem.setVisible (true);

	m_inDockModeTransition = false;
	Q_EMIT signalDockModeAnimationComplete();

	cleanupDockModeAnimations();
}

void WindowServerLuna::setupDockModeAnimations()
{
	// this function first takes a screen shot of the device out of dockmode and then in dock mode, 
	// irrespective of the direction of animation

	// take a screen shot of outside dock mode
	reorderWindowManagersForDockMode(false);
	m_dockModeMgr->setOpacity(0.0);

	const HostInfo& hostInfo = HostBase::instance()->getInfo();
	int displayWidth = hostInfo.displayWidth;
	int displayHeight = hostInfo.displayHeight;

	m_screenShot = takeScreenShot();
	
	m_screenShotObject = new QGraphicsPixmapObject();
	m_screenShotObject->setPixmap(m_screenShot);
	m_screenShotObject->setPos (displayWidth/2,displayHeight/2);
	m_screenShotObject->setVisible(false);
	m_screenFade.setTargetObject(NULL);
	m_screenFade.setTargetObject(m_screenShotObject);
	m_screenScale.setTargetObject(NULL);
	m_screenScale.setTargetObject(m_screenShotObject);

	// take a screen shot of inside dock mode
	reorderWindowManagersForDockMode(true);
	m_dockModeMgr->setOpacity(1.0);

	m_dockImage = takeScreenShot();

	m_dockImageObject = new QGraphicsPixmapObject();
	m_dockImageObject->setPixmap(m_dockImage);
	m_dockImageObject->setPos (displayWidth/2,displayHeight/2);
	m_dockImageObject->setVisible (false);
	m_dockFade.setTargetObject(NULL);
	m_dockFade.setTargetObject(m_dockImageObject);
	m_dockScale.setTargetObject(NULL);
	m_dockScale.setTargetObject(m_dockImageObject);
	
	// adding it to the scene
	if (m_dockImageObject)
		scene()->addItem (m_dockImageObject);
	if (m_screenShotObject)
		scene()->addItem (m_screenShotObject);
}

void WindowServerLuna::cleanupDockModeAnimations()
{
	m_screenFade.setTargetObject (NULL);
	m_screenScale.setTargetObject (NULL);

	m_dockFade.setTargetObject (NULL);
	m_dockScale.setTargetObject (NULL);

	if (m_screenShotObject) {
		scene()->removeItem (m_screenShotObject);
		m_screenShotObject->setPixmap(NULL);
		delete m_screenShotObject;
		m_screenShotObject = 0;
	}

	if (m_dockImageObject) {
		scene()->removeItem (m_dockImageObject);
		m_dockImageObject->setPixmap(NULL);
		delete m_dockImageObject;
		m_dockImageObject = 0;
	}

	if(m_screenShot) {
		delete m_screenShot;
		m_screenShot = 0;
	}

	if(m_dockImage) {
		delete m_dockImage;
		m_dockImage = 0;
	}
}

bool WindowServerLuna::triggerFullEraseCountdown()
{
	if (m_powerVolumeKeyComboState.fullEraseComboDown()) {
		if (!m_fullEraseConfirmationWindow)
			slotShowFullEraseWindow();
		return true;
	}

	return false;
}

void WindowServerLuna::cancelFullEraseCountdown()
{
	if (m_fullEraseConfirmationWindow && !m_fullErasePending) {

		delete m_fullEraseConfirmationWindow;
		m_fullEraseConfirmationWindow = 0;

		update();
	}
}

void WindowServerLuna::applyLaunchFeedback(int centerX, int centerY)
{
	((OverlayWindowManager*)m_overlayMgr)->applyLaunchFeedback(centerX, centerY);
}

void WindowServerLuna::appLaunchPreventedUnderLowMemory()
{
    slotAppLaunchPreventedUnderLowMemory();
}


void WindowServerLuna::slotShowFullEraseWindow()
{
	Q_ASSERT(m_fullEraseConfirmationWindow == 0);

	m_fullEraseConfirmationWindow = new FullEraseConfirmationWindow();
	scene()->addItem(m_fullEraseConfirmationWindow);
	m_fullEraseConfirmationWindow->setZValue(1000);
	m_fullEraseConfirmationWindow->setPos(sceneRect().center());

	connect(m_fullEraseConfirmationWindow, SIGNAL(signalFullEraseConfirmed()), SLOT(slotFullEraseDevice()));
}

bool WindowServerLuna::cbFullEraseCallback(LSHandle* handle, LSMessage* msg, void* data)
{
	bool success = false;
	const char* str = LSMessageGetPayload(msg);
	if (str) {

		json_object* root = json_tokener_parse(str);
		if (root && !is_error(root)) {

			json_object* prop = json_object_object_get(root, "returnValue");
			if (prop && !is_error(prop)) {
				success = json_object_get_boolean(prop);
			}

			json_object_put(root);
		}
	}

	g_warning("%s: full erase successful? %d", __PRETTY_FUNCTION__, success);
	if (success) {
		exit(-2);
	}

	// in the outside case that the full erase fails, remove the full erase window
	// so the user can attempt it again
	WindowServerLuna* ws = reinterpret_cast<WindowServerLuna*>(data);
	ws->m_fullErasePending = false;

	ws->m_powerVolumeKeyComboState.reset();
	ws->cancelFullEraseCountdown();

	return true;
}

void WindowServerLuna::slotFullEraseDevice()
{
	LSHandle* handle = SystemService::instance()->serviceHandle();
	LSError err;
	LSErrorInit(&err);

	luna_assert(handle != 0);
	LSCall(handle, "palm://com.palm.storage/erase/EraseAll", "{}", &WindowServerLuna::cbFullEraseCallback, this, NULL, &err);

	if (LSErrorIsSet(&err)) {
		LSErrorPrint(&err, stderr);
		LSErrorFree(&err);
	}
	else {
		m_fullErasePending = true;
	}
}


void WindowServerLuna::updateWallpaperForRotation(OrientationEvent::Orientation newOrient)
{
	int wallpaperRotation = 0;

	if(!m_wallpaperFullScreen) {
		m_currWallpaperImg = &m_normalWallpaperImage;
	} else {
		switch(newOrient) {
            case OrientationEvent::Orientation_Up:
            case OrientationEvent::Orientation_Down:
			{
				m_currWallpaperImg = &m_normalWallpaperImage;
			}
			break;

            case OrientationEvent::Orientation_Left:
            case OrientationEvent::Orientation_Right:
			{
				m_currWallpaperImg = &m_rotatedWallpaperImage;
				wallpaperRotation = 90;
			}
			break;

			default:
				return;
		}
	}

	Q_EMIT signalWallpaperImageChanged(m_currWallpaperImg, m_wallpaperFullScreen, wallpaperRotation);
}

void WindowServerLuna::generateWallpaperImages()
{
	qreal hScale, vScale, wallpaperScale, wallpaperScaleRot = 0.0;

	int screenWidth = SystemUiController::instance()->currentUiWidth();
	int screenHeight = SystemUiController::instance()->currentUiHeight();

	QPixmap image = QPixmap(m_wallpaperFileName);
	if (image.isNull())
		return;

	bool desktop(false);
#ifdef TARGET_DESKTOP
	desktop = true;
#endif

	if(((image.width() < screenWidth) || (image.height() < screenHeight)) && ((image.height() < screenWidth) || (image.width() < screenHeight)) && !desktop) {
		// image is not large enough to fill the screen in any orientation, so do not scale it and
		// draw it centered on the screen
		m_normalWallpaperImage = QPixmap(image.width(), image.height());
		m_rotatedWallpaperImage = QPixmap();

		wallpaperScale = 1;
		m_wallpaperFullScreen = false;
		if (m_normalWallpaperImage.isNull())
			return;
	} else {
		// image can fill the screen in some orientations, so scale it to always fill the entire screen
		m_normalWallpaperImage = QPixmap(m_screenWidth, m_screenHeight);
		m_rotatedWallpaperImage = QPixmap(m_screenWidth, m_screenHeight);

		hScale = (qreal)m_screenWidth / image.width();
		vScale = (qreal)m_screenHeight / image.height();

		if(hScale >= vScale)
			wallpaperScale = hScale;
		else
			wallpaperScale = vScale;

		vScale = (qreal)m_screenHeight / image.width();
		hScale = (qreal)m_screenWidth / image.height();

		if(hScale >= vScale)
			wallpaperScaleRot = hScale;
		else
			wallpaperScaleRot = vScale;

		m_wallpaperFullScreen = true;

		if (m_normalWallpaperImage.isNull())
			return;
		if (m_rotatedWallpaperImage.isNull())
			return;
	}

	QRect target;
	QPainter painter;
	painter.begin(&m_normalWallpaperImage);

	target = QRect((-image.rect().width()/2), (-image.rect().height()/2), image.rect().width(), image.rect().height());

	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	painter.translate(m_normalWallpaperImage.width()/2, m_normalWallpaperImage.height()/2);
	painter.scale(wallpaperScale,wallpaperScale);

	painter.drawPixmap(target, image);

	painter.end();

	if(m_wallpaperFullScreen) {// also generate the cropped and rotated version
		painter.begin(&m_rotatedWallpaperImage);

		target = QRect((-image.rect().width()/2), (-image.rect().height()/2), image.rect().width(), image.rect().height());

		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
		painter.translate(m_rotatedWallpaperImage.width()/2, m_rotatedWallpaperImage.height()/2);
		painter.rotate(90);
		painter.scale(wallpaperScaleRot,wallpaperScaleRot);

		painter.drawPixmap(target, image);

		painter.end();
	}

	if(Settings::LunaSettings()->displayUiRotates) {
		updateWallpaperForRotation(m_currentUiOrientation);
	} else {
		m_currWallpaperImg = &m_normalWallpaperImage;
		Q_EMIT signalWallpaperImageChanged(m_currWallpaperImg, m_wallpaperFullScreen, 0);
	}
}

void WindowServerLuna::slotWallPaperChanged(const char* filePath)
{
	if (!filePath)
		return;

	if(m_wallpaperFileName == filePath)
		return; // no change

	m_wallpaperFileName = filePath;

        resetCachedContent(); // Reset wallpaper cache
	generateWallpaperImages();

	update();
}

void WindowServerLuna::drawBackground ( QPainter * painter, const QRectF & rect )
{
	if((m_inRotationAnimation == Rotation_NoAnimation) && !m_inDockModeTransition && !m_drawWallpaper)
		return;

	QRect screenBounds = QRect(0, 0, m_screenWidth, m_screenHeight);

	if(!m_currWallpaperImg) {
		painter->fillRect(screenBounds, Qt::black);
		return;
	}

	QRect imgBounds = QRect(-(int)m_currWallpaperImg->width()/2, -(int)m_currWallpaperImg->height()/2, m_currWallpaperImg->width(), m_currWallpaperImg->height());

	int wallpaperRotation = 0;
	QPixmap* img;


	if((m_inRotationAnimation != Rotation_NoAnimation) || m_dockModeAnimation.state() == QAbstractAnimation::Running) {
		painter->fillRect(screenBounds, Qt::black);
	} else {
		if(!m_currWallpaperImg) {
			WindowServer::drawBackground(painter, screenBounds);
			return;
		}

		if(!m_wallpaperFullScreen)
			WindowServer::drawBackground(painter, screenBounds);

		painter->translate(m_screenWidth/2, m_screenHeight/2);

		if(!m_wallpaperFullScreen) {
            if(m_currentUiOrientation == OrientationEvent::Orientation_Left) {
				wallpaperRotation = 90;
            } else if(m_currentUiOrientation == OrientationEvent::Orientation_Down) {
				wallpaperRotation = 180;
            } else if(m_currentUiOrientation == OrientationEvent::Orientation_Right) {
				wallpaperRotation = 270;
			} else {
				wallpaperRotation = 0;
			}

			if(wallpaperRotation)
				painter->rotate(wallpaperRotation);

			painter->drawPixmap(imgBounds, *m_currWallpaperImg);

			if(wallpaperRotation)
				painter->rotate(-wallpaperRotation);
		} else {

            if((m_currentUiOrientation == OrientationEvent::Orientation_Down) || (m_currentUiOrientation == OrientationEvent::Orientation_Right)) {
				wallpaperRotation = 180;
			} else {
				wallpaperRotation = 0;
			}


			if(wallpaperRotation)
				painter->rotate(wallpaperRotation);

			painter->drawPixmap(imgBounds, *m_currWallpaperImg);

			if(wallpaperRotation)
				painter->rotate(-wallpaperRotation);
		}

		painter->translate(-(int)m_screenWidth/2, -(int)m_screenHeight/2);
	}
}

bool WindowServerLuna::sysmgrEventFilters(QEvent* event)
{
	QEvent::Type type = event->type();

	if (handleEvent(event)) {
		luna_log(kWindowSrvChnl, "event consumed by WindowServer: %d", type);
		return true;
	}

	if (m_displayMgr->handleEvent(event)) {
		luna_log(kWindowSrvChnl, "event consumed by DisplayMgr: %d", type);
		return true;
	}

	if (m_coreNaviMgr->handleEvent(event)) {
		luna_log(kWindowSrvChnl, "event consumed by CoreNaviMgr: %d", type);
		return true;
	}

	if (m_inputMgr->handleEvent(event)) {
		luna_log(kWindowSrvChnl, "event consumed by InputManager: %d", type);
		return true;
	}

	if (SystemService::instance()->brickMode() || m_runningProgress || m_bootingUp) {
		luna_log(kWindowSrvChnl, "event blocked due to brick mode/progress anim/boot up: %d", type);
		return true;
	}

	if (m_metaKeyMgr->handleEvent(event)) {
		luna_log(kWindowSrvChnl, "event consumed by MetaKeyManager: %d", type);
		return true;
	}

	if((type == QEvent::MouseButtonPress) || (type == QEvent::MouseButtonRelease) || (type == QEvent::MouseMove) ||
		(type == QEvent::KeyPress) || (type == QEvent::KeyRelease) || (type == QEvent::GestureOverride))
	{
		if(!SystemUiController::instance()->isInDockMode()) {
			if (m_topLevelMgr && ((TopLevelWindowManager*)m_topLevelMgr)->handleEvent(event)) {
				luna_log(kWindowSrvChnl, "event consumed by TopLevelWindowManager: %d", type);
				return true;
			}
		}
	}

	if (UiNavigationController::instance()->handleEvent(event)) {
		luna_log(kWindowSrvChnl, "event consumed by UiNavigationController: %d", type);
		return true;
	}

	if (SystemUiController::instance()->handleEvent(event)) {
		luna_log(kWindowSrvChnl, "event consumed by SystemUiController: %d", type);
		return true;
	}

	if(((type == QEvent::KeyPress) || (type == QEvent::KeyRelease)) && (((QKeyEvent*)event)->key() == Qt::Key_CoreNavi_QuickLaunch))
	{
		// Quick Launch Key should go to Overlay WM
		if(!SystemUiController::instance()->isInDockMode() && !SystemUiController::instance()->isScreenLocked() && !SystemUiController::instance()->isInEmergencyMode()) {
			if (type == QEvent::KeyPress) {
				if (m_overlayMgr) {
					((OverlayWindowManager*)m_overlayMgr)->keyPressEvent((QKeyEvent*)event);
					luna_log(kWindowSrvChnl, "event consumed by OverlayWindowManager: %d", type);
					return true;
				}
			} else {
				if (m_overlayMgr) {
					((OverlayWindowManager*)m_overlayMgr)->keyReleaseEvent((QKeyEvent*)event);
					luna_log(kWindowSrvChnl, "event consumed by OverlayWindowManager: %d", type);
					return true;
				}
			}
		}
	}

	return false;
}

bool WindowServerLuna::processSystemShortcut(QEvent* event)
{
	if (SystemUiController::instance()->bootFinished() &&
		(event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)) {

		static bool symDown = false;

		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		switch (keyEvent->key()) {
		case Qt::Key_Control:
			symDown = keyEvent->type() == QEvent::KeyPress;
			break;

		case Qt::Key_Power: {
			m_powerVolumeKeyComboState.powerKeyPress = (keyEvent->type() == QEvent::KeyPress);
			if (!m_powerVolumeKeyComboState.powerKeyPress) {
				cancelFullEraseCountdown();
			}
			else if (m_powerVolumeKeyComboState.msmEntryComboDown()) {
				SystemService::instance()->enterMSM();
				return true;
			}
			else if (m_powerVolumeKeyComboState.comboDown())
				return true;
			break;
		}
		case Qt::Key_VolumeDown: {
			m_powerVolumeKeyComboState.volDownKeyPress = (keyEvent->type() == QEvent::KeyPress);
			if (m_powerVolumeKeyComboState.msmEntryComboDown()) {
				SystemService::instance()->enterMSM();
				return true;
			}
			break;
		}
		case Qt::Key_VolumeUp: {
			m_powerVolumeKeyComboState.volUpKeyPress = (keyEvent->type() == QEvent::KeyPress);
			if (!m_powerVolumeKeyComboState.volUpKeyPress) {
				cancelFullEraseCountdown();
			}
			else if (m_powerVolumeKeyComboState.comboDown())
				return true;
			break;
		}
		case Qt::Key_CoreNavi_Home: {
			bool ret = triggerFullEraseCountdown();
			if (ret) {
				// Eat up the home key event
				return true;
			}
			break;
		}
		}
/*
#if !defined(TARGET_DEVICE)
		KeyMapType key = getDetailsForQtKey(keyEvent->key(), keyEvent->modifiers());
		switch (key.qtKey) {

		case Qt::Key_D: {
			// force the device into Dock Mode
			if (symDown && (keyEvent->type() == QEvent::KeyPress) && (!SystemUiController::instance()->isInDockMode())) {
				m_displayMgr->dock();
				return true;
			}
			break;
		}

		case Qt::Key_L: {
			// force the device into Dock Mode
			if (symDown && (keyEvent->type() == QEvent::KeyPress) && (!m_displayMgr->isLocked())) {
				m_displayMgr->lock();
				return true;
			}
			break;
		}
		default:
			break;
		}
#endif*/
	}
	return WindowServer::processSystemShortcut(event);
}


QPixmap* WindowServerLuna::takeScreenShot()
{
	QPixmap* pix = new QPixmap(m_screenWidth, m_screenHeight);
	QPainter painter(pix);
	bool visibleReticle = (m_reticle ? m_reticle->isVisible() : false);
    if (visibleReticle)
    	m_reticle->setVisible(false);
	render(&painter);
	painter.end();
	if(visibleReticle)
		m_reticle->setVisible(true);
	g_message("%s",__PRETTY_FUNCTION__);
	return pix;
}

void WindowServerLuna::slotMemoryStateChanged(bool critical)
{
    g_debug("%s: %s", __PRETTY_FUNCTION__, critical ? "critical" : "non-critical");
	if (critical) {
		createMemoryAlertWindow();
	}
	else {

		if (m_memoryAlert) {
			QmlAlertWindow* win = m_memoryAlert.data();
			m_memoryAlert.clear();

			win->close();
		}
	}
}

void WindowServerLuna::slotAppLaunchPreventedUnderLowMemory()
{
	createMemoryAlertWindow();
}

void WindowServerLuna::createMemoryAlertWindow()
{
    if (m_memoryAlert)
		return;

	std::string qmlSrcPath = Settings::LunaSettings()->lunaQmlUiComponentsPath + "MemoryAlert/alert.qml";

	m_memoryAlert = new QmlAlertWindow(qmlSrcPath.c_str(), kAlertWindowWidth, 160);
	addWindow(m_memoryAlert.data());
}

void WindowServerLuna::createMsmEntryFailedAlertWindow()
{
    if (m_msmEntryFailedAlert)
		return;

	std::string qmlSrcPath = Settings::LunaSettings()->lunaQmlUiComponentsPath + "MsmEntryFailed/alert.qml";

	m_msmEntryFailedAlert = new QmlAlertWindow(qmlSrcPath.c_str(), kAlertWindowWidth, 160);
	addWindow(m_msmEntryFailedAlert.data());
}

void WindowServerLuna::slotBrickModeFailed()
{
    createMsmEntryFailedAlertWindow();
}

void WindowServerLuna::createDismissCardWindow()
{
        if (m_dismissCardDialog)
                return;

        std::string qmlSrcPath = Settings::LunaSettings()->lunaQmlUiComponentsPath + "DismissCardTutorial/dismissDialog.qml";

        m_dismissCardDialog = new QmlAlertWindow(qmlSrcPath.c_str(), kAlertWindowWidth, 170);
        addWindow(m_dismissCardDialog.data());
}

void WindowServerLuna::slotFirstCardRun()
{
    createDismissCardWindow();
}


