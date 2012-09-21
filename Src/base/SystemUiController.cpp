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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>

#include "SystemUiController.h"

#include "AnimationSettings.h"
#include "ApplicationManager.h"
#include "ApplicationInstaller.h"
#include "CardWindow.h"
#include "DisplayManager.h"
#include "HostBase.h"
#include "MetaKeyManager.h"
#include "Preferences.h"
#include "Settings.h"
#include "SystemService.h"
#include "WindowServer.h"
#include "WebAppMgrProxy.h"
#include "CoreNaviManager.h"
#include "WindowServerLuna.h"
#include "QtUtils.h"
#include "Localization.h"
#include "OverlayWindowManager.h"
#include "IMEController.h"
#include "ScreenEdgeFlickGesture.h"
#include "SoundPlayerPool.h"
#include "DashboardWindowManager.h"
#include "StatusBarServicesConnector.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

static SystemUiController* s_instance = 0;

static const char* kLauncherAppId = "com.palm.launcher";
static const char* kSystemUiAppId = "com.palm.systemui";

static const unsigned int s_statusBarLauncherColor = 0x4f545AFF;
static const unsigned int s_statusBarJustTypeColor = 0x4f545AFF;

static const int kFlickMinimumYLengthWithKeyboardUp = 60;

SystemUiController* SystemUiController::instance()
{
    if (G_UNLIKELY(!s_instance))
		new SystemUiController;
	return s_instance;
}

SystemUiController::SystemUiController()
	: m_dashboardOwnsNegativeSpace(Settings::LunaSettings()->virtualKeyboardEnabled ? false : (Settings::LunaSettings()->showNotificationsAtTop ? false : true))
	, m_anim(0)
	, m_suspendBlocker(HostBase::instance()->mainLoop(),
					   this, &SystemUiController::allowSuspend, &SystemUiController::setSuspended)
{
    s_instance = this;

	// initial state
    m_modalCardWindowActive = false;
    m_modalWindowLaunchErr = LaunchUnknown;
    m_modalWindowDismissErr = DismissUnknown;
	m_cardWindowAboutToMaximize = false;
	m_cardWindowMaximized = false;
	m_dashboardOpened = false;
	m_dashboardSoftDismissable = true;
	m_dashboardHasContent = false;
	m_alertVisible = false;
	m_emergencyMode = false;
	m_inDockMode = false;
    m_universalSearchShown = false;

	m_activeCardWindow = 0;
	m_maximizedCardWindow = 0;
	m_launcherWindow = 0;
	m_universalSearchWindow = 0;
	m_parentOfModalWindow = 0;

	m_launcherShown = false;
	m_dockShown = false;

	m_menuVisible = false;
	m_dockVisible = false;
	m_launcherVisible = false;
	m_deviceLocked = false;

	m_uiRootItemPtr = 0;

	m_statusBarPtr = 0;

	m_statusBarAndNotificationShown = true;

	m_bootFinished = false;

	m_uiRotating = false;
	m_rotationAngle = 0;

	m_isBlockScreenTimeout = false;
	m_isSubtleLightbar = false;
	m_activeTouchpanel = false;
	m_alsDisabled = false;
	m_overlayNotificationPosition = WindowProperties::OverlayNotificationsBottom;
	m_suppressBannerMessages = false;
	m_suppressGestures = false;
	m_dockBrightness = 100;
    
	const HostInfo& info = HostBase::instance()->getInfo();
    m_uiWidth  = info.displayWidth;
    m_uiHeight = info.displayHeight;

    m_currentlyDirectRenderingLayer  = -1;
    m_currentlyDirectRenderingWindow = NULL;
    for (int x=0; x < NUMBER_OF_LAYERS; x++) {
    	m_directRenderLayers[x].requestedDirectRendring = false;
    	m_directRenderLayers[x].activeWindow            = NULL;
    }
}

SystemUiController::~SystemUiController()
{
    s_instance = 0;
}

void SystemUiController::startPositiveSpaceAnimation(const QRect& start, const QRect& end)
{
	stopPositiveSpaceAnimation();

	m_anim = new VariantAnimation<SystemUiController>(this, &SystemUiController::animValueChanged);
	m_anim->setEasingCurve(AS_CURVE(positiveSpaceChangeCurve));
	m_anim->setDuration(AS(positiveSpaceChangeDuration));
	m_anim->setStartValue(start);
	m_anim->setEndValue(end);

	connect(m_anim, SIGNAL(finished()), SLOT(slotAnimFinished()));

	m_anim->start();
}

void SystemUiController::stopPositiveSpaceAnimation()
{
	if (m_anim)
		delete m_anim;
	m_anim = 0;
}

void SystemUiController::init()
{
	WindowServer* ws = WindowServer::instance();

	MetaKeyManager* metaKeyMgr = ws->metaKeyManager();
	connect(metaKeyMgr, SIGNAL(signalCopy()), this, SLOT(slotCopy()));
	connect(metaKeyMgr, SIGNAL(signalCut()), this, SLOT(slotCut()));
	connect(metaKeyMgr, SIGNAL(signalPaste()), this, SLOT(slotPaste()));
	connect(metaKeyMgr, SIGNAL(signalSelectAll()), this, SLOT(slotSelectAll()));

	SystemService* ss = SystemService::instance();

	connect(ss, SIGNAL(signalEnterBrickMode(bool)), this, SLOT(slotEnterBrickMode(bool)));
	connect(ss, SIGNAL(signalExitBrickMode()), this, SLOT(slotExitBrickMode()));

	connect(WebAppMgrProxy::instance(), SIGNAL(signalKeyEventRejected(const SysMgrKeyEvent&)), this, SLOT(slotKeyEventRejected(const SysMgrKeyEvent&)));

	// Initial sizes for positive and negative spaces
    int positiveSpaceFromTop = Settings::LunaSettings()->positiveSpaceTopPadding;
	int positiveSpaceFromBottom = 0;

	m_positiveSpace = QRect(0, positiveSpaceFromTop,
							m_uiWidth,
							m_uiHeight - positiveSpaceFromTop - positiveSpaceFromBottom);
	m_negativeSpace = QRect(0, m_uiHeight - positiveSpaceFromBottom,
							m_uiWidth,
							positiveSpaceFromBottom);

	m_minimumPositiveSpaceHeight = m_positiveSpace.height();
	m_maximumPositiveSpaceHeight = m_uiHeight;

	m_requestedNegativeSpaceTop = m_uiHeight;

	layout();
}

bool SystemUiController::isInFullScreenMode()
{
	return (m_maximizedCardWindow && static_cast<CardWindow*>(m_maximizedCardWindow)->fullScreen());
}

bool SystemUiController::isLauncherShown() const
{
	return m_launcherShown;
}

bool SystemUiController::isDockShown() const
{
	return m_dockShown;
}

bool SystemUiController::handleEvent(QEvent *event)
{
	switch ((int)event->type()) {
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
		return handleKeyEvent(static_cast<QKeyEvent*>(event));
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonRelease:
		return handleMouseEvent(static_cast<QMouseEvent*>(event));
	case QEvent::Gesture:
		return handleGestureEvent (static_cast<QGestureEvent*>(event));
    case orientationEventType:
    {
        return handleCustomEvent(event);
    }
	default:
		break;
	}
	return false;
}

bool SystemUiController::handleMouseEvent(QMouseEvent *event)
{
	return false;
}

bool SystemUiController::handleGestureEvent (QGestureEvent* event)
{
	QGesture* t = event->gesture(Qt::TapGesture);
	if (t) {
		QTapGesture* tap = static_cast<QTapGesture*>(t);
		if (tap->state() == Qt::GestureFinished) {
			if(m_dashboardOwnsNegativeSpace) {
				if (m_dashboardOpened) {
					QPointF p = m_uiRootItemPtr->mapFromScene(tap->hotSpot());
					if((p.y() + m_uiRootItemPtr->boundingRect().height()/2) < m_negativeSpace.y()) {
						Q_EMIT signalCloseDashboard(false);
					}
				}
			}
		}
	}

	if (!t) {
		if (Settings::LunaSettings()->uiType != Settings::UI_MINIMAL && !m_emergencyMode) {
			t = event->gesture((Qt::GestureType) SysMgrGestureScreenEdgeFlick);
			if (t)
				handleScreenEdgeFlickGesture(t);
		}
	}
	
	return false;
}

bool SystemUiController::handleCustomEvent(QEvent* event)
{
    Q_ASSERT(event->type() >= QEvent::User);
    bool handled = false;
    QGraphicsScene* scene = WindowServer::instance()->scene();

    switch ((int)event->type()) {
    case orientationEventType:
    {
        if (scene && scene->focusItem()) {
            handled = scene->sendEvent(scene->focusItem(), event);
        }
        break;
    }
    default: 
        g_debug("%s: ignoring custom event type %d", __PRETTY_FUNCTION__, event->type());
        break;
    }
    return handled;
}

bool SystemUiController::handleKeyEvent(QKeyEvent *event)
{
#if !defined(TARGET_EMULATOR)
	if (!Preferences::instance()->sysUiEnableNextPrevGestures()) {
		if (event->key() == Qt::Key_CoreNavi_Previous) {
			event->setKey(Qt::Key_CoreNavi_Back);
		} else if (event->key() == Qt::Key_CoreNavi_Next) {
			event->setKey(Qt::Key_CoreNavi_Menu);
		}
	}
#endif

	if (event->type() == QEvent::KeyPress) {
		switch (event->key()) {
		case Qt::Key_CoreNavi_Home:
		case Qt::Key_CoreNavi_Launcher:
		case Qt::Key_CoreNavi_QuickLaunch:
		case Qt::Key_CoreNavi_Previous:
		case Qt::Key_CoreNavi_Next:
		case Qt::Key_CoreNavi_SwipeDown:
			// Eat all up corenavi gesture key downs
			return true;

		case Qt::Key_CoreNavi_Back:
		case Qt::Key_CoreNavi_Menu:
			if (m_dashboardOpened || m_menuVisible || m_launcherShown)
				return true;
			else
				return false;

        case Qt::Key_Escape: // maps to the open/close notifications key on BT keyboards
        case Qt::Key_Search: // maps to universal search toggle
        case Qt::Key_Super_L: // maps to the card view key (launcher gesture)
        case Qt::Key_Keyboard:
            return true;

		case Qt::Key_Shift:
		case Qt::Key_Control:
		case Qt::Key_Alt:
		case Qt::Key_Option:
			// Modifiers should not close dashboard
		case Qt::Key_CoreNavi_Meta:
			// Don't use meta to close dashboard
			break;

		default:

			if (m_dashboardOwnsNegativeSpace && m_dashboardOpened && m_dashboardSoftDismissable) {
				g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
				Q_EMIT signalCloseDashboard(false);
			}

			if (m_menuVisible && !m_deviceLocked) {
				Q_EMIT signalHideMenu();
			}

			// Allow event to be passed on to webkit(!)
			break;
		}
	} else {

		// KeyRelease

		// This key has been canceled. Eat it
		if (event->modifiers() & Qt::GroupSwitchModifier)
			return true;

		switch (event->key()) {

		case Qt::Key_CoreNavi_QuickLaunch:

			// Eat away quick launch gesture when suppressGesture is set to true
			if (m_suppressGestures) {
				return true;
			}

			if (m_dashboardOpened) {
				g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
				Q_EMIT signalCloseDashboard(true);
			}

			if (m_menuVisible) {
				Q_EMIT signalHideMenu();
			}
			break;

		case Qt::Key_CoreNavi_Previous:
		case Qt::Key_CoreNavi_Next:

			if (m_dashboardOpened) {
				g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
				Q_EMIT signalCloseDashboard(true);
			}
			if (m_menuVisible) {
				Q_EMIT signalHideMenu();
			}
			if (!m_launcherShown) {
				Q_EMIT signalChangeCardWindow(event->key() == Qt::Key_CoreNavi_Next);
				return true;
			}
			break;

		case Qt::Key_CoreNavi_Menu:

			if(m_deviceLocked && !m_inDockMode)
				return false;

			if (m_dashboardOpened) {
				g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
				Q_EMIT signalCloseDashboard(true);
			}
			if (m_menuVisible) {
				Q_EMIT signalHideMenu();
			}
			break;

		case Qt::Key_CoreNavi_Back:

			if(m_deviceLocked && !m_inDockMode)
				return false;

			if (m_dashboardOpened) {
				Q_EMIT signalCloseDashboard(true);
				return true;
			}

			if (m_menuVisible) {
				Q_EMIT signalHideMenu();
				return true;
			}
			if (m_launcherShown && !m_universalSearchShown) {
				Q_EMIT signalToggleLauncher();
				return true;
			}

			break;

		case (Qt::Key_CoreNavi_Launcher):
        case (Qt::Key_Super_L): {

			if (Settings::LunaSettings()->uiType == Settings::UI_MINIMAL) {
				break;
			}

			if (m_inDockMode) {
				enterOrExitDockModeUi(false);
				return true;
			}

			if(m_deviceLocked)
				return false;

			if (m_emergencyMode) {
				Q_EMIT signalEmergencyModeHomeButtonPressed();
				return true;
			}

			if (m_dashboardOpened) {
				g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
				Q_EMIT signalCloseDashboard(true);
			}

			if (m_menuVisible) {
				Q_EMIT signalHideMenu();
			}

            if (m_universalSearchShown) {
                Q_EMIT signalHideUniversalSearch(false, false);
                return true;
            }

			if (Preferences::instance()->sysUiNoHomeButtonMode()) {
				if (m_launcherShown) {
					Q_EMIT signalToggleLauncher();
					return true;
				}

				if ((m_activeCardWindow && m_maximizedCardWindow) || m_cardWindowAboutToMaximize) {
					if(false == m_modalCardWindowActive)
						Q_EMIT signalShowDock();

					Q_EMIT signalMinimizeActiveCardWindow();
					return true;
				}

				Q_EMIT signalToggleLauncher();
				return true;
			}
			break;
		}

		case (Qt::Key_CoreNavi_SwipeDown): {

			if (Settings::LunaSettings()->uiType == Settings::UI_MINIMAL ||
				m_emergencyMode) {
				break;
			}

			if(m_deviceLocked && !m_inDockMode)
				return false;

			if (Preferences::instance()->sysUiNoHomeButtonMode()) {

				// New HI flow: Down gesture will only bring a card into focus
				if (m_dashboardOpened || m_menuVisible || m_launcherShown) {
					return true;
				}

				if (m_activeCardWindow && !m_maximizedCardWindow) {

					Q_EMIT signalHideDock();
					Q_EMIT signalMaximizeActiveCardWindow();
					return true;
				}
				return true;
			}
			// always consume this event
			return true;
		}

		case Qt::Key_CoreNavi_Home: {

			if (m_inDockMode) {
				enterOrExitDockModeUi(false);
				return true;
			}

			if(m_deviceLocked)
				return false;

			if (m_emergencyMode) {
				Q_EMIT signalEmergencyModeHomeButtonPressed();
				return true;
			}
			
			if (m_dashboardOpened) {
				g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
				Q_EMIT signalCloseDashboard(true);
				return true;
			}

			if (m_alertVisible) {
				g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
				Q_EMIT signalCloseAlert();
				return true;
			}

			if (m_menuVisible) {
				Q_EMIT signalHideMenu();
				return true;
			}

			if (m_launcherShown) {
				Q_EMIT signalToggleLauncher();
				return true;
			}

			if (m_universalSearchShown) {
				Q_EMIT signalHideUniversalSearch(false, false);

				return true;
			}

            // the auto repeat flag set on the Home Key Release, signifies a Double Tap
            if (!event->isAutoRepeat()) {
                if (m_activeCardWindow && (m_maximizedCardWindow || m_cardWindowAboutToMaximize)) {
                	if(false == m_modalCardWindowActive)
                		Q_EMIT signalShowDock();

                    Q_EMIT signalMinimizeActiveCardWindow();
                    return true;
                }
            }

            // when you've reached the desktop, the Home button should act as a launcher show/hide toggle
            Q_EMIT signalToggleLauncher();
			return true;
		}
        case Qt::Key_Escape: {

            if (m_deviceLocked || m_inDockMode || m_emergencyMode)
                return true;

            if (m_alertVisible) {
                Q_EMIT signalCloseAlert();
            }

            if (m_menuVisible) {
                Q_EMIT signalHideMenu();
            }

            if (m_dashboardOpened) {
                closeDashboard(true);
            }
            else {
                openDashboard();
            }
            return true;
        }
        case Qt::Key_Search: {

            if (m_deviceLocked || m_inDockMode || m_emergencyMode)
                return true;

            if (m_universalSearchShown) {
                Q_EMIT signalHideUniversalSearch(false, false);
            }
            else {
                Q_EMIT signalShowUniversalSearch();
            }
            return true;
        }
        case Qt::Key_Keyboard: {
            // toggle the state of the IME
            IMEController::instance()->setIMEActive(!IMEController::instance()->isIMEActive());
            return true;
        }
		default:
			// eat away all keys if a menu window is visible
			if (m_menuVisible)
				return true;

			break;
		}
	}
	return false;
}

void SystemUiController::notifyModalWindowActivated(Window* modalParent)
{
	m_modalCardWindowActive = true;
	m_parentOfModalWindow = modalParent;
	Q_EMIT signalModalWindowAdded();
}

void SystemUiController::notifyModalWindowDeactivated()
{
	m_modalCardWindowActive = false;
	m_parentOfModalWindow = NULL;
	Q_EMIT signalModalWindowRemoved();
}

std::string SystemUiController::getModalWindowLaunchErrReason()
{
	switch(m_modalWindowLaunchErr) {
	case NoErr:
		return "Modal window was launched successfully";
	case NoMaximizedCard:
		return "Modal window could not be launched as there is no active window";
	case ParentDifferent:
		return "Modal window could not be launched as the currently active window is different from what was specified";
	case AnotherInstanceAlreadyRunning:
		return "Another instance of the app to be launched as modal window is already running";
	case MissingAppDescriptor:
		return "Error recreating the app descriptor for the modal window";
	/*case AppToLaunchIsNotHeadless:
		return "Application to be launched is not headless";*/
	case LaunchUnknown:
		return "An Unknown Error occurred launching the modal window";
	default:
		return "";
	}
}

std::string SystemUiController::getModalWindowDismissErrReason()
{
	switch(m_modalWindowDismissErr) {
	case HomeButtonPressed:
		return "Modal card was dismissed as the user pressed home button";
	case ServiceDismissedModalCard:
		return "Modal card was dismissed by the service";
	case ParentCardDismissed:
		return "Modal card was dismissed as the parent of the modal window was dismissed or closed";
	case ActiveCardsSwitched:
		return "Modal card was dismissed as the active card window was switched";
	case UiMinimized:
		return "Modal card was dismissed because system got minimize active card gesture";
	case DismissUnknown:
		return "An Unknown Error occurred dismissing the modal window";
	default:
		return "";
	}
}

void SystemUiController::setCardWindowAboutToMaximize()
{
	if (m_inDockMode) {
		enterOrExitDockModeUi(false);
	}

	m_cardWindowAboutToMaximize = true;

	// hide the dock
	Q_EMIT signalHideDock();

	// hide the launcher
	Q_EMIT signalHideLauncher();

	// close the dashboard
	g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
	if (m_dashboardOpened)
		Q_EMIT signalCloseDashboard(false);
}

void SystemUiController::setCardWindowMaximized(bool val)
{
	m_cardWindowAboutToMaximize = false;
	m_cardWindowMaximized = val;

	if (m_cardWindowMaximized) {

		if (m_inDockMode) {
			enterOrExitDockModeUi(false);
		}

		// hide the dock
		Q_EMIT signalHideDock();

		// hide the launcher
		Q_EMIT signalHideLauncher();

		// close the dashboard
				g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
		if (m_dashboardOpened)
			Q_EMIT signalCloseDashboard(false);

		// let the system know we have maximized a card
		Q_EMIT signalCardWindowMaximized();
	}
	else {

		// show the dock
		Q_EMIT signalShowDock();

		// let the system know we have minimized a card
		Q_EMIT signalCardWindowMinimized();
	}
}

void SystemUiController::setUniversalSearchShown(bool val)
{
	m_universalSearchShown = val;


	if (m_launcherShown) {
		updateStatusBarTitle();
	} else {
		setMaximizedCardWindow(m_maximizedCardWindow);
	}
}


void SystemUiController::setLauncherShown(bool val)
{
	m_launcherShown = val;

	if (val) {
		SoundPlayerPool::instance()->playFeedback("LauncherOpenApp");
	} else {
		SoundPlayerPool::instance()->playFeedback("LauncherCloseApp");
	}

    if (m_launcherShown) {
		ApplicationDescription* appDesc = ApplicationManager::instance()->getAppById(kLauncherAppId);
		if (appDesc && appDesc->getDefaultLaunchPoint()) {
			WindowServerLuna * pWsl = qobject_cast<WindowServerLuna *>(WindowServer::instance());
			if (pWsl)
			{
				// this is still necessary to trigger the Launcher bar Menu
				OverlayWindowManager * pOwm = qobject_cast<OverlayWindowManager *>(pWsl->overlayWindowManager());
				if (pOwm)
				{
					if (m_universalSearchShown) {
						SystemService::instance()->postForegroundApplicationChange(appDesc->getDefaultLaunchPoint()->title(),appDesc->menuName(),
									   kLauncherAppId);
					}
					else {
						SystemService::instance()->postForegroundApplicationChange(appDesc->getDefaultLaunchPoint()->title(),"",
									   kLauncherAppId);
					}
				}
			}
		}
    } else {
    	setMaximizedCardWindow(m_maximizedCardWindow);
    }

	updateStatusBarTitle();

	Q_EMIT signalLauncherShown(val);
}

void SystemUiController::updateStatusBarTitle()
{
	if(m_statusBarPtr) {
		WindowServerLuna * pWsl = qobject_cast<WindowServerLuna *>(WindowServer::instance());
		ApplicationDescription* appDesc = 0;
		const LaunchPoint* launcherAppLp = 0;
		OverlayWindowManager * pOwm = 0;

		if (pWsl)
			pOwm = qobject_cast<OverlayWindowManager *>(pWsl->overlayWindowManager());

		appDesc = ApplicationManager::instance()->getAppById(kLauncherAppId);
		if (appDesc && appDesc->getDefaultLaunchPoint()) {
			launcherAppLp = appDesc->getDefaultLaunchPoint();
		}

		if(m_universalSearchShown && pOwm) {
			m_statusBarPtr->setMaximizedAppTitle(true, appDesc->menuName().c_str(), s_statusBarJustTypeColor, true);
		} else if(m_launcherShown){
			m_statusBarPtr->setMaximizedAppTitle(true, launcherAppLp->title().c_str(), s_statusBarLauncherColor, false);
		} else {
			CardWindow* card = static_cast<CardWindow*>(m_maximizedCardWindow);
			unsigned int color = 0;
			if(m_maximizedCardWindow) {
				if (card->hasCustomStatusBarColor()) {
					color = card->statusBarColor() << 8 | 0x000000FF;
					g_debug ("%s: setting status bar color: 0x%x", __PRETTY_FUNCTION__, color);
				}
				m_statusBarPtr->setMaximizedAppTitle(true, getMenuTitleForMaximizedWindow(m_maximizedCardWindow).c_str(), color);
			} else {
				m_statusBarPtr->setMaximizedAppTitle(false, 0);
			}
		}
	}
}

void SystemUiController::setDockShown(bool val)
{
	m_dockShown = val;
}

void SystemUiController::showOrHideUniversalSearch(bool show, bool showLauncher, bool speedDial)
{
	//first signal that any op or action has ended (been canceled)
	Q_EMIT signalLauncherMenuOpEnd();
	Q_EMIT signalLauncherActionEnd();

	if (show)
		Q_EMIT signalShowUniversalSearch();
	else
		Q_EMIT signalHideUniversalSearch(showLauncher, speedDial);
}

void SystemUiController::showOrHideLauncher(bool show)
{
	//first signal that any op or action has ended (been canceled)
	Q_EMIT signalLauncherMenuOpEnd();
	Q_EMIT signalLauncherActionEnd();

	if (show)
		Q_EMIT signalShowLauncher();
	else
		Q_EMIT signalHideLauncher();
}

void SystemUiController::showOrHideDock(bool show)
{
	if ((isCardWindowMaximized() || m_cardWindowAboutToMaximize) && !m_launcherShown)
		return;

	if (show) {
		Q_EMIT signalShowDock();
	}
	else {
		Q_EMIT signalHideDock();
	}
}

void SystemUiController::enterOrExitCardReorder(bool entered)
{
	Q_EMIT signalFadeDock(entered);
}

void SystemUiController::enterOrExitDockModeUi(bool enter)
{
	g_message ("%s: enter %d", __PRETTY_FUNCTION__, enter);
	if(enter) {
		DisplayManager::instance()->dock();
	}
	else {
		DisplayManager::instance()->undock();
	}
}

void SystemUiController::setDashboardOpened(bool val, bool isSoftDismissable)
{
	m_dashboardSoftDismissable = isSoftDismissable;

    if (m_dashboardOpened == val)
		return;

	m_dashboardOpened = val;

	if(m_statusBarPtr)
		m_statusBarPtr->setNotificationWindowOpen(m_dashboardOpened);

	if(m_dashboardOwnsNegativeSpace) {
		if (m_dashboardOpened) {

			// Hide the dock
			Q_EMIT signalHideDock();
		}
		else {
			// show the dock but only if the
			// the cardWM is not maximized and the launcher is not visible
			if (!m_cardWindowMaximized && !m_cardWindowAboutToMaximize && !m_launcherShown) {
				Q_EMIT signalShowDock();
			}
		}
	}
	else {
		if(m_dashboardOpened) {
			// we can show the dock if launcher is not shown or there is no active card
			if((!m_launcherShown && 0 == m_activeCardWindow) && (!isDockShown()))
				Q_EMIT signalShowDock();
		}
	}
}

void SystemUiController::setAlertVisible(bool val)
{
//	m_dashboardSoftDismissable = isSoftDismissable;

    if (m_alertVisible == val)
		return;

    m_alertVisible = val;

}


void SystemUiController::slotKeyEventRejected(const SysMgrKeyEvent& event)
{
	if((event.key == Qt::Key_CoreNavi_Back) && (event.type == QEvent::KeyRelease)){
		if(m_inDockMode) {
			enterOrExitDockModeUi (false);
		} else if (m_launcherShown){
			// hide the launcher
			Q_EMIT signalHideLauncher();
		} else {
			// launcher not visible, so a card must have rejected the BACK gesture.
			Q_EMIT signalMinimizeActiveCardWindow();
		}
	}
}

void SystemUiController::setDashboardHasContent(bool val)
{
	if (m_dashboardHasContent == val)
		return;

	m_dashboardHasContent = val;
}

void SystemUiController::setEmergencyMode(bool enable)
{
	m_emergencyMode = enable;

	if (m_emergencyMode) {
		if (m_dashboardOpened) {
				g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
			Q_EMIT signalCloseDashboard(true);
		}
	}

	focusMaximizedCardWindow(!enable);

	Q_EMIT signalEmergencyMode(enable);
}

Window* SystemUiController::activeWindow() const
{
	if (!m_launcherShown && !m_universalSearchShown && m_cardWindowMaximized)
		return m_activeCardWindow;
	else
		return m_universalSearchWindow;
}

std::string SystemUiController::activeApplicationName()
{
	std::string name = "Unknown";
	if (m_activeCardWindow) {
		if (m_activeCardWindow->appId().size())
			name = m_activeCardWindow->appId();
		 // find last '.' and turn
	}

	return name;
}

void SystemUiController::setActiveCardWindow(Window* window)
{
    if (m_activeCardWindow) static_cast<CardWindow*>(m_activeCardWindow)->setDimm(true);
	m_activeCardWindow = window;
    if (m_activeCardWindow) static_cast<CardWindow*>(m_activeCardWindow)->setDimm(false);
}


void SystemUiController::setMaximizedCardWindow(Window* window)
{
	bool cardLocksRotation = false;

	if (m_maximizedCardWindow && !isScreenLocked()) {
		m_maximizedCardWindow = 0;
        removeWindowProperties ();
    }

    m_maximizedCardWindow = window;

	std::string name,menuname, id;

	// update the status bar title
	updateStatusBarTitle();

	if (m_maximizedCardWindow && !isScreenLocked()) {
		applyWindowProperties(window);
	}

	if(Settings::LunaSettings()->displayUiRotates) {
		if (m_maximizedCardWindow != NULL) {
			if (m_maximizedCardWindow->type() == Window::Type_Card) {
				CardWindow* cardWindow = static_cast<CardWindow*>(m_maximizedCardWindow);
				setRequestedSystemOrientation(cardWindow->getCardFixedOrientation(), true);
			}

		} else {
			WindowServer::instance()->setUiRotationMode(WindowServer::RotationMode_FreeRotation);
		}
	}

    // Post a message via system service to indicate current window
    if (m_maximizedCardWindow && !m_maximizedCardWindow->appId().empty())  {

            ApplicationDescription* appDesc = ApplicationManager::instance()->getAppById(m_maximizedCardWindow->appId());
            if (appDesc) {
                    name = appDesc->launchPoints().front()->title();
                    menuname = appDesc->menuName();
                    id = appDesc->id();
            }
    }

    SystemService::instance()->postForegroundApplicationChange(name,menuname, id);

    Q_EMIT signalMaximizedCardWindowChanged(window);
}

void SystemUiController::setRequestedSystemOrientation(Event::Orientation orient, bool reasonCardMaximizing, bool skipAnimation)
{
	WindowServer::RotationMode mode = WindowServer::RotationMode_FreeRotation;

		switch(orient) {
		case Event::Orientation_Invalid: {mode = WindowServer::RotationMode_FreeRotation; break;}
		case Event::Orientation_Up: {mode = WindowServer::RotationMode_FixedPortrait; break;}
		case Event::Orientation_Left: {mode = WindowServer::RotationMode_FixedLandscape; break;}
		case Event::Orientation_Right: {mode = WindowServer::RotationMode_FixedLandscapeInverted; break;}
		case Event::Orientation_Down: {mode = WindowServer::RotationMode_FixedPortraitInverted; break;}
		case Event::Orientation_Landscape: {mode = WindowServer::RotationMode_LimitedLandscape; break;}
		case Event::Orientation_Portrait: {mode = WindowServer::RotationMode_LimitedPortrait; break;}
		default:{mode = WindowServer::RotationMode_FreeRotation; break;}
	}

	WindowServer::instance()->setUiRotationMode(mode, reasonCardMaximizing, skipAnimation);
}

void SystemUiController::focusMaximizedCardWindow(bool enable)
{
	if (m_maximizedCardWindow)
		applyWindowProperties (m_maximizedCardWindow);

		// for the converse case -
		// the maximized card can be defocused when a new card is given focus or the display is off
		// for the first case, we remove the window properties in setMaximizedCardWindow
		// for the second case, we assume display off will automatically turn off the current window properties.
		// if that assumption is invalid, we would need to remove window properties when the display goes off
    if (m_maximizedCardWindow || !enable)
        Q_EMIT signalFocusMaximizedCardWindow(enable);
}

void SystemUiController::setDirectRenderingForWindow(DirectRenderingEnabledLayers layer, CardWindow* win, bool enable, bool force)
{
	DirectRenderingEnabledLayers layerToBeEnabled = (DirectRenderingEnabledLayers)-1;
	CardWindow* windowToBeEnabled = NULL;
	
	if(layer >= NUMBER_OF_LAYERS) {
		g_message("%s: Invalid Parameters", __PRETTY_FUNCTION__);
		return;
	}
	
	if (!force && enable && (layer == m_currentlyDirectRenderingLayer) && (win == m_currentlyDirectRenderingWindow)) {
		g_message("%s: Requested Window already in DirectRendering", __PRETTY_FUNCTION__);
		return;
	}
	
	// update the current record for the layer
	if(enable) {
		m_directRenderLayers[layer].requestedDirectRendring = true;
		m_directRenderLayers[layer].activeWindow            = win;
	} else {
		// ignore specified layer as a safety precaution.
        for (int x=0; x < NUMBER_OF_LAYERS; x++) {
            if(m_directRenderLayers[x].activeWindow == win) {
                m_directRenderLayers[x].requestedDirectRendring = false;
                m_directRenderLayers[x].activeWindow            = NULL;
                break;
            }
        }		
    }
	
	// scan for the layer to be enabled
	for(int x = 0; x < NUMBER_OF_LAYERS; x++) {
		if(m_directRenderLayers[x].requestedDirectRendring) {
			layerToBeEnabled = (DirectRenderingEnabledLayers) x;
			windowToBeEnabled = m_directRenderLayers[x].activeWindow;
			break;
		}
	}

	if (force || (m_currentlyDirectRenderingLayer != layerToBeEnabled) ||
		(m_currentlyDirectRenderingWindow != windowToBeEnabled)) {
		// Change the current direct rendering window

		if(m_currentlyDirectRenderingWindow != NULL) {
			// disable direct rendering the currently enabled window
			g_debug("%s: Removing DirectRendering from card window [%d]: 0x%x", __PRETTY_FUNCTION__, m_currentlyDirectRenderingLayer, (int)m_currentlyDirectRenderingWindow);
		 	m_currentlyDirectRenderingWindow->setMaximized(false);
		}
	 	m_currentlyDirectRenderingLayer = -1;
	 	m_currentlyDirectRenderingWindow = NULL;

	 	if((layerToBeEnabled >= 0) && (windowToBeEnabled != NULL)) {
			// Enable direct rendering for the new window
			g_debug("%s: Giving DirectRendering to card window [%d]: 0x%x", __PRETTY_FUNCTION__, layerToBeEnabled, (int)windowToBeEnabled);
	 		windowToBeEnabled->setMaximized(true);

	 		m_currentlyDirectRenderingLayer = layerToBeEnabled;
		 	m_currentlyDirectRenderingWindow = windowToBeEnabled;
	 	}
	}
}

void SystemUiController::enableDirectRendering(bool enable)
{
	if (enable) {
		setDirectRenderingForWindow((DirectRenderingEnabledLayers) m_currentlyDirectRenderingLayer,
									m_currentlyDirectRenderingWindow, true, true);
	}
	else {

		if (m_currentlyDirectRenderingWindow)
			m_currentlyDirectRenderingWindow->setMaximized(false);
	}
}

std::string SystemUiController::maximizedApplicationName() const
{
	std::string name;
	if (isLauncherShown()) {
		ApplicationDescription* appDesc = ApplicationManager::instance()->getAppById(kLauncherAppId);
		if (appDesc && appDesc->getDefaultLaunchPoint())
			name = appDesc->getDefaultLaunchPoint()->title();
	}
	else if (m_maximizedCardWindow && !m_maximizedCardWindow->appId().empty())  {

		ApplicationDescription* appDesc = ApplicationManager::instance()->getAppById(m_maximizedCardWindow->appId());
		if (appDesc)
			name = appDesc->launchPoints().front()->title();
	}

	return name;
}

std::string SystemUiController::maximizedApplicationMenuName() const
{
	std::string name;
	if (isLauncherShown()) {
		ApplicationDescription* appDesc = ApplicationManager::instance()->getAppById(kLauncherAppId);
		if (appDesc && appDesc->getDefaultLaunchPoint())
			name = appDesc->getDefaultLaunchPoint()->menuName();
	}
	else if (m_maximizedCardWindow && !m_maximizedCardWindow->appId().empty())  {

		ApplicationDescription* appDesc = ApplicationManager::instance()->getAppById(m_maximizedCardWindow->appId());
		if (appDesc)
			name = appDesc->launchPoints().front()->menuName();
	}

	return name;
}

std::string SystemUiController::maximizedApplicationId() const
{
	std::string id;

	// Post a message via system service to indicate current window
	if (isLauncherShown()) {
		id = kLauncherAppId;
	}
	else if (m_maximizedCardWindow) {
		id = m_maximizedCardWindow->appId();
	}

	return id;
}

void SystemUiController::layout()
{
	Q_EMIT signalPositiveSpaceChanged(m_positiveSpace);
	Q_EMIT signalNegativeSpaceChanged(m_negativeSpace);
}


bool SystemUiController::okToResizeUi()
{
	if(m_anim && m_anim->state() != VariantAnimation<SystemUiController>::Stopped)
		return false;

	return true;
}

void SystemUiController::resizeAndRotateUi(int width, int height, int rotationAngle)
{
	const HostInfo& info = HostBase::instance()->getInfo();

	m_uiRotating = true;
	m_rotationAngle = rotationAngle;

	int uiTop  = (info.displayHeight - m_uiHeight) / 2;
	int uiLeft = (info.displayWidth - m_uiWidth) / 2;

	m_requestedNegativeSpaceTop =  m_requestedNegativeSpaceTop - m_uiHeight + height;

	m_minimumPositiveSpaceHeight = m_uiHeight - Settings::LunaSettings()->positiveSpaceTopPadding;

	m_uiWidth  = width;
	m_uiHeight = height;

	QRect targetPositiveSpace;
	QRect targetNegativeSpace;

	// recalculate new top and left
	uiTop  = 0;
	uiLeft = 0;

	int positiveSpaceFromTop = Settings::LunaSettings()->positiveSpaceTopPadding;
	int positiveSpaceFromBottom = 0;

	if (m_dashboardOwnsNegativeSpace) {
		if (m_dashboardOpened || m_dashboardHasContent) {
			positiveSpaceFromBottom = m_uiHeight - m_requestedNegativeSpaceTop;
		}
	}
	else if (IMEController::instance()->isIMEOpened()) {
		positiveSpaceFromBottom = m_uiHeight - m_requestedNegativeSpaceTop;
	}

	// Resize the positive and negative spaces

	// Are we in full-screen mode?
	if (!m_statusBarAndNotificationShown) {

        if (m_dashboardOwnsNegativeSpace) {
    		targetPositiveSpace.setRect(uiLeft, uiTop,
	    								m_uiWidth, m_uiHeight);
		    targetNegativeSpace.setRect(uiLeft, uiTop + m_uiHeight,
			    						m_uiWidth,
				    					Settings::LunaSettings()->positiveSpaceBottomPadding);
        }
        else {
    		targetPositiveSpace.setRect(uiLeft, uiTop,
	    								m_uiWidth, m_requestedNegativeSpaceTop);
		    targetNegativeSpace.setRect(uiLeft, m_requestedNegativeSpaceTop,
			    						m_uiWidth,
				    					positiveSpaceFromBottom);
        }
	}
	else {

		targetPositiveSpace.setRect(uiLeft, uiTop + positiveSpaceFromTop,
									m_uiWidth, m_uiHeight - positiveSpaceFromTop - positiveSpaceFromBottom);
		targetNegativeSpace.setRect(uiLeft, m_requestedNegativeSpaceTop,
									m_uiWidth,
									positiveSpaceFromBottom);
	}

	m_positiveSpace = targetPositiveSpace;
	m_negativeSpace = targetNegativeSpace;

	m_maximumPositiveSpaceHeight = m_uiHeight;

	// now resize the Window Managers
	WindowServer::instance()->resizeWindowManagers(width, height);

	Q_EMIT signalPositiveSpaceAboutToChange(targetPositiveSpace, m_statusBarAndNotificationShown == false, true);
	Q_EMIT signalNegativeSpaceAboutToChange(targetNegativeSpace, m_statusBarAndNotificationShown == false, true);

	layout();

	Q_EMIT signalPositiveSpaceChangeFinished(m_positiveSpace);
	Q_EMIT signalNegativeSpaceChangeFinished(m_negativeSpace);
}

void SystemUiController::rotationStarting()
{
	m_uiRotating = true;
	Q_EMIT signalUiRotationAboutToStart();
}

void SystemUiController::rotationComplete()
{
	m_uiRotating = false;
	m_rotationAngle = 0;

	Q_EMIT signalUiRotationCompleted();
}

void SystemUiController::setMinimumPositiveSpaceHeight(int val)
{
	bool oldVal = m_minimumPositiveSpaceHeight;
	m_minimumPositiveSpaceHeight = MIN(val, m_minimumPositiveSpaceHeight);
	if (oldVal != m_minimumPositiveSpaceHeight)
		layout();
}

void SystemUiController::setMaximumPositiveSpaceHeight(int val)
{
	bool oldVal = m_maximumPositiveSpaceHeight;
	m_maximumPositiveSpaceHeight = MAX(val, m_maximumPositiveSpaceHeight);
	if (oldVal != m_maximumPositiveSpaceHeight)
		layout();
}

int SystemUiController::minimumPositiveSpaceHeight() const
{
	return m_minimumPositiveSpaceHeight;
}

int SystemUiController::maximumPositiveSpaceHeight() const
{
	return m_maximumPositiveSpaceHeight;
}

int SystemUiController::minimumPositiveSpaceBottom() const
{
	return m_positiveSpace.y() + m_minimumPositiveSpaceHeight;
}

int SystemUiController::maximumPositiveSpaceBottom() const
{
	return m_positiveSpace.y() + m_maximumPositiveSpaceHeight;
}

bool SystemUiController::changeNegativeSpace(int newTop, bool openingDashboard, bool immediate)
{
	// We want to execute the animation timer all the time to make
	// sure that the post animation handlers get called in the Dashboard WM even
	// if the Dashboard showing is suppressed due to a full screen app

	g_debug("%s: %d", __PRETTY_FUNCTION__, newTop);

	newTop = MIN(newTop, maximumPositiveSpaceBottom());
	m_requestedNegativeSpaceTop = newTop;

	QRect targetPositiveSpace;
	QRect targetNegativeSpace;

	int uiTop  = 0;
	int uiLeft = 0;

	int positiveSpaceFromTop = Settings::LunaSettings()->positiveSpaceTopPadding;
	// negative space is owned by the dashboard if the virtual keyboard is not enabled
	int positiveSpaceFromBottom = (m_dashboardHasContent && m_dashboardOwnsNegativeSpace) ?
			  Settings::LunaSettings()->positiveSpaceBottomPadding : 0;

	m_statusBarAndNotificationShown = true;

	if (openingDashboard) {

		// Are we in full-screen mode?
		if (isInFullScreenMode()) {

			m_statusBarAndNotificationShown = false;

            if (m_dashboardOwnsNegativeSpace) {
    			targetPositiveSpace.setRect(uiLeft, uiTop,
	    									m_uiWidth, m_uiHeight);
		    	targetNegativeSpace.setRect(uiLeft, uiTop + m_uiHeight,
			    							m_uiWidth,
				    						Settings::LunaSettings()->positiveSpaceBottomPadding);
            }
            else {
    			targetPositiveSpace.setRect(uiLeft, uiTop,
    										m_uiWidth, uiTop + newTop);
	    		targetNegativeSpace.setRect(uiLeft, uiTop + newTop,
		    								m_uiWidth,
			    							m_uiHeight - newTop + positiveSpaceFromTop);
            }
		}
		else {

			targetPositiveSpace.setRect(uiLeft, uiTop + positiveSpaceFromTop,
										m_uiWidth, uiTop + newTop - positiveSpaceFromTop);
			targetNegativeSpace.setRect(uiLeft, uiTop + newTop,
										m_uiWidth,
										m_uiHeight - newTop);
		}
	}
	else {

		// Should we be going to full-screen mode?
		if (isInFullScreenMode()) {

			m_statusBarAndNotificationShown = false;

            if (m_dashboardOwnsNegativeSpace) {
    			targetPositiveSpace.setRect(uiLeft, uiTop,
	    									m_uiWidth, m_uiHeight);
    			targetNegativeSpace.setRect(uiLeft, uiTop + m_uiHeight,
	    									m_uiWidth,
		    								Settings::LunaSettings()->positiveSpaceBottomPadding);
            }
            else {
    			targetPositiveSpace.setRect(uiLeft, uiTop,
    										m_uiWidth, uiTop + newTop);
	    		targetNegativeSpace.setRect(uiLeft, uiTop + newTop,
		    								m_uiWidth,
			    							m_uiHeight - newTop + positiveSpaceFromTop);
            }
		}
		else {

			targetPositiveSpace.setRect(uiLeft, uiTop + positiveSpaceFromTop,
										m_uiWidth,
										m_uiHeight - positiveSpaceFromTop - positiveSpaceFromBottom);
			targetNegativeSpace.setRect(uiLeft, uiTop + m_uiHeight - positiveSpaceFromBottom,
										m_uiWidth,
										positiveSpaceFromBottom);
		}
	}

	Q_EMIT signalStatusBarAndNotificationShown(m_statusBarAndNotificationShown);

	Q_EMIT signalPositiveSpaceAboutToChange(targetPositiveSpace, m_statusBarAndNotificationShown == false, false);
	Q_EMIT signalNegativeSpaceAboutToChange(targetNegativeSpace, m_statusBarAndNotificationShown == false, false);
	
	if(!immediate) {
		// animate
		startPositiveSpaceAnimation(m_positiveSpace, targetPositiveSpace);
	} else {
		//immediate
		m_positiveSpace = targetPositiveSpace;
		int bottom = m_positiveSpace.y() + m_positiveSpace.height();
		m_negativeSpace.setRect(0, bottom,
								m_uiWidth,
								m_uiHeight - bottom);

		stopPositiveSpaceAnimation();

		layout();
		Q_EMIT signalPositiveSpaceChangeFinished(m_positiveSpace);
		Q_EMIT signalNegativeSpaceChangeFinished(m_negativeSpace);
	}

	return true;
}

void SystemUiController::setLauncherVisible(bool visible, bool fullyVisible)
{
	m_launcherVisible = visible;
	Q_EMIT signalLauncherVisible(visible, fullyVisible);
}

void SystemUiController::cardWindowAdded()
{
	if (m_inDockMode) {
		enterOrExitDockModeUi(false);
	}

	Q_EMIT signalCardWindowAdded();
}

void SystemUiController::cardWindowTimeout()
{
	Q_EMIT signalCardWindowTimeout();
}

void SystemUiController::setDockVisible(bool val)
{
    m_dockVisible = val;
}

void SystemUiController::setMenuVisible(bool val)
{
    m_menuVisible = val;
}

void SystemUiController::setDeviceLocked(bool val)
{
	m_deviceLocked = val;

	SystemService::instance()->postLockStatus (m_deviceLocked);

// Launcher will be closed when press power button to suspend
//	if (m_launcherShown && val) {
//		Q_EMIT signalToggleLauncher();
//	}
//	else
	if (m_universalSearchShown) {
		Q_EMIT signalUniversalSearchFocusChange(!val);
	}
	else {
		if(!m_emergencyMode)
			focusMaximizedCardWindow(!val);
		else {
			Q_EMIT signalEmergencyModeWindowFocusChange(!val);
		}
	}

}

void SystemUiController::setDockMode(bool inDockMode)
{
	if (m_inDockMode != inDockMode) {
		m_inDockMode = inDockMode;
		if (inDockMode)
			removeWindowProperties();
		else if (!m_deviceLocked)
			focusMaximizedCardWindow(true);
		if (m_menuVisible) {
			Q_EMIT signalHideMenu();
		}
	}
}

void SystemUiController::openDashboard()
{
	if(!m_dashboardOpened) {
		Q_EMIT signalOpenDashboard();
	}
}

void SystemUiController::closeDashboard(bool force)
{
	if(m_dashboardOpened)
		Q_EMIT signalCloseDashboard(force);
}

void SystemUiController::toggleCurrentAppMenu()
{
	if (m_inDockMode) {
		Q_EMIT signalDockModeStatusBarToggle();

	} else if (m_universalSearchShown) {
		std::string appId = kLauncherAppId;
		std::string launchingAppId = kSystemUiAppId;
		std::string launchingProcId = kLauncherAppId;
		std::string errMsg;

		std::string params = "{\"palm-command\":\"open-app-menu\"}";

		WebAppMgrProxy::instance()->appLaunch(appId, params, launchingAppId, launchingProcId, errMsg);
	} else if(m_maximizedCardWindow) {
		std::string appId = m_maximizedCardWindow->appId();
		std::string launchingAppId = kSystemUiAppId;
		std::string launchingProcId = kLauncherAppId;
		std::string errMsg;

		std::string params = "{\"palm-command\":\"open-app-menu\"}";

		WebAppMgrProxy::instance()->appLaunch(appId, params, launchingAppId, launchingProcId, errMsg);
	}
}

void SystemUiController::setInLauncherReorder(bool reorder)
{
	if (reorder && m_dashboardOpened) {
		g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
		Q_EMIT signalCloseDashboard(false);
	}
}

void SystemUiController::slotCopy()
{
	WebAppMgrProxy::instance()->emitCopy( SystemUiController::instance()->activeWindow() );
}

void SystemUiController::slotCut()
{
	WebAppMgrProxy::instance()->emitCut( SystemUiController::instance()->activeWindow() );
}

void SystemUiController::slotPaste()
{
	WebAppMgrProxy::instance()->emitPaste( SystemUiController::instance()->activeWindow() );
}

void SystemUiController::slotSelectAll()
{
	WebAppMgrProxy::instance()->emitSelectAll( SystemUiController::instance()->activeWindow() );
}

void SystemUiController::hideStatusBarAndNotificationArea()
{
	// We expand positive space.

	if (!m_statusBarAndNotificationShown)
		return;

	DashboardWindowManager* pDwm = qobject_cast<DashboardWindowManager *>(WindowServer::instance()->getWindowManagerByClassName("DashboardWindowManager"));
	if (m_dashboardOpened && (m_dashboardSoftDismissable || (pDwm && pDwm->isOverlay()))) {
		g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
		Q_EMIT signalCloseDashboard(pDwm ? pDwm->isOverlay() : false);
	}

	QRect targetPositiveSpace;
	QRect targetNegativeSpace;

    if (m_dashboardOwnsNegativeSpace) {
        targetPositiveSpace.setRect(0, 0, m_uiWidth, m_uiHeight);
        targetNegativeSpace.setRect(0, m_uiHeight, m_uiWidth, Settings::LunaSettings()->positiveSpaceBottomPadding);
    }
    else {
        targetPositiveSpace.setRect(0, 0, m_uiWidth, m_requestedNegativeSpaceTop);
        targetNegativeSpace.setRect(0, m_requestedNegativeSpaceTop, m_uiWidth, m_uiHeight - m_requestedNegativeSpaceTop);
    }

	Q_EMIT signalPositiveSpaceAboutToChange(targetPositiveSpace, true, false);
	Q_EMIT signalNegativeSpaceAboutToChange(targetNegativeSpace, true, false);

	startPositiveSpaceAnimation(m_positiveSpace, targetPositiveSpace);

	m_statusBarAndNotificationShown = false;

	Q_EMIT signalStatusBarAndNotificationShown(m_statusBarAndNotificationShown);
}

void SystemUiController::showStatusBarAndNotificationArea()
{
	// We shrink positive space. FIXME: we need to animate the status bar
	// and notification are coming in or out

	if (m_statusBarAndNotificationShown)
		return;

	if (m_dashboardOpened && !m_dashboardSoftDismissable) {
		// if dashboard is not soft-dismissable we will use the target top as a guideline
		changeNegativeSpace(m_requestedNegativeSpaceTop, true);
		m_statusBarAndNotificationShown = true;
		Q_EMIT signalStatusBarAndNotificationShown(m_statusBarAndNotificationShown);
		return;
	}

	if (m_dashboardOpened) {
		g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
		Q_EMIT signalCloseDashboard(false);
	}

	int positiveSpaceFromTop = Settings::LunaSettings()->positiveSpaceTopPadding;
	int positiveSpaceFromBottom = (m_dashboardHasContent && m_dashboardOwnsNegativeSpace) ?
								  Settings::LunaSettings()->positiveSpaceBottomPadding : 0;

	QRect targetPositiveSpace = QRect(0, positiveSpaceFromTop,
									  m_uiWidth,
									  m_uiHeight - positiveSpaceFromTop - positiveSpaceFromBottom);
	QRect targetNegativeSpace = QRect(0, m_uiHeight - positiveSpaceFromBottom,
									  m_uiWidth,
									  positiveSpaceFromBottom);

	if (m_anim && (m_anim->endValue().toRect() == targetPositiveSpace))
		return;

	Q_EMIT signalPositiveSpaceAboutToChange(targetPositiveSpace, true, false);
	Q_EMIT signalNegativeSpaceAboutToChange(targetNegativeSpace, true, false);

	startPositiveSpaceAnimation(m_positiveSpace, targetPositiveSpace);

	m_statusBarAndNotificationShown = true;
	Q_EMIT signalStatusBarAndNotificationShown(m_statusBarAndNotificationShown);
}

void SystemUiController::animValueChanged(const QVariant& value)
{
	m_positiveSpace = value.toRect();
	int bottom = m_positiveSpace.y() + m_positiveSpace.height();
	m_negativeSpace.setRect(0, bottom, m_uiWidth, m_uiHeight - bottom);

	layout();
}

void SystemUiController::slotAnimFinished()
{
	Q_EMIT signalPositiveSpaceChangeFinished(m_positiveSpace);
	Q_EMIT signalNegativeSpaceChangeFinished(m_negativeSpace);
}

void SystemUiController::setBootFinished()
{
	m_bootFinished = true;
	Q_EMIT signalBootFinished();
}

bool SystemUiController::bootFinished() const
{
    return m_bootFinished;
}


void SystemUiController::updateProperties (Window* win, const WindowProperties& props)
{
	if (win == maximizedCardWindow()) {
		if (props.flags & WindowProperties::isSetBlockScreenTimeout)
		{
			if (props.isBlockScreenTimeout && !m_isBlockScreenTimeout)
			{
				g_debug ("%s: blocking auto-off for %s", __PRETTY_FUNCTION__, maximizedApplicationName().c_str());
				DisplayManager::instance()->pushDNAST(maximizedApplicationName().c_str());
				m_isBlockScreenTimeout = true;
			}
			else if (!props.isBlockScreenTimeout && m_isBlockScreenTimeout) {
				g_debug ("%s: unblocking auto-off for %s", __PRETTY_FUNCTION__, maximizedApplicationName().c_str());
				DisplayManager::instance()->popDNAST (maximizedApplicationName().c_str());
				m_isBlockScreenTimeout = false;
			}
		}

		if (props.flags & WindowProperties::isSetSubtleLightbar)
		{
			if (props.isSubtleLightbar && !m_isSubtleLightbar) {
				g_debug ("%s: setting subtle lightbar", __PRETTY_FUNCTION__);
				CoreNaviManager::instance()->setSubtleLightbar (true);
				m_isSubtleLightbar = true;
			}
			else if (!props.isSubtleLightbar &&  m_isSubtleLightbar) {
				g_debug ("%s: unsetting subtle lightbar", __PRETTY_FUNCTION__);
				CoreNaviManager::instance()->setSubtleLightbar (false);
				m_isSubtleLightbar = false;
			}
		}

		if (props.flags & WindowProperties::isSetActiveTouchpanel)
		{
			if (props.activeTouchpanel && !m_activeTouchpanel) {
				g_debug ("%s: setting active touchpanel", __PRETTY_FUNCTION__);
				DisplayManager::instance()->setActiveTouchpanel (true);
				m_activeTouchpanel = true;
			}
			else if (!props.activeTouchpanel &&  m_activeTouchpanel) {
				g_debug ("%s: unsetting active touchpanel", __PRETTY_FUNCTION__);
				DisplayManager::instance()->setActiveTouchpanel (false);
				m_activeTouchpanel = false;
			}
		}


		if (props.flags & WindowProperties::isSetAlsDisabled)
		{
			if (props.alsDisabled && !m_alsDisabled) {
				g_debug ("%s: setting als disabled", __PRETTY_FUNCTION__);
				DisplayManager::instance()->setAlsDisabled (true);
				m_alsDisabled = true;
			}
			else if (!props.alsDisabled &&  m_alsDisabled) {
				g_debug ("%s: unsetting active touchpanel", __PRETTY_FUNCTION__);
				DisplayManager::instance()->setAlsDisabled (false);
				m_alsDisabled = false;
			}
		}

		if (props.flags & WindowProperties::isSetOverlayNotifications) {
			m_overlayNotificationPosition = props.overlayNotificationsPosition;
			Q_EMIT signalOverlayNotificationPositionChanged(m_overlayNotificationPosition);
		}

		if (props.flags & WindowProperties::isSetFullScreen) {
			if (props.fullScreen)
				hideStatusBarAndNotificationArea();
			else {
				showStatusBarAndNotificationArea();
			}
		}

		if (props.flags & WindowProperties::isSetSuppressBannerMessages) {
			m_suppressBannerMessages = props.suppressBannerMessages;
			Q_EMIT signalOverlayNotificationSuppressBannerMessages(m_suppressBannerMessages);
		}

		if (props.flags & WindowProperties::isSetSuppressGestures) {
			m_suppressGestures = props.suppressGestures;
		}

	}
}


void SystemUiController::applyWindowProperties (Window* win)
{
	if (!win)
		return;

	CardWindow* card = static_cast<CardWindow*>(win);

    if (win == maximizedCardWindow() && NULL != card) {
		if (card->isBlockScreenTimeout() && !m_isBlockScreenTimeout) {
			g_debug ("%s: blocking auto-off for %s", __PRETTY_FUNCTION__, maximizedApplicationName().c_str());
			DisplayManager::instance()->pushDNAST(maximizedApplicationName().c_str());
			m_isBlockScreenTimeout = true;
		}

		if (card->isSubtleLightbar() && !m_isSubtleLightbar) {
			g_debug ("%s: setting subtle lightbar", __PRETTY_FUNCTION__);
			CoreNaviManager::instance()->setSubtleLightbar (true);
			m_isSubtleLightbar = true;
		}

		if (card->activeTouchpanel() && !m_activeTouchpanel) {
			g_debug ("%s: setting active touchpanel", __PRETTY_FUNCTION__);
			DisplayManager::instance()->setActiveTouchpanel (true);
			m_activeTouchpanel = true;
		}

		if (card->alsDisabled() && !m_alsDisabled) {
			g_debug ("%s: setting als disabld", __PRETTY_FUNCTION__);
			DisplayManager::instance()->setAlsDisabled (true);
			m_alsDisabled = true;
		}

		if (card->fullScreen())
			hideStatusBarAndNotificationArea();
		else {
			showStatusBarAndNotificationArea();
		}

		m_overlayNotificationPosition = card->overlayNotificationsPosition();
		Q_EMIT signalOverlayNotificationPositionChanged(m_overlayNotificationPosition);

		m_suppressBannerMessages = card->suppressBannerMessages();
		Q_EMIT signalOverlayNotificationSuppressBannerMessages(m_suppressBannerMessages);

		m_suppressGestures = card->suppressGestures();
    }
}

void SystemUiController::removeWindowProperties ()
{
    if (m_isBlockScreenTimeout) {
        g_debug ("%s: unblocking auto-off for %s", __PRETTY_FUNCTION__, maximizedApplicationName().c_str());
        DisplayManager::instance()->popDNAST(maximizedApplicationName().c_str());
		m_isBlockScreenTimeout = false;
    }

    if (m_isSubtleLightbar) {
        g_debug ("%s: unsetting subtle lightbar", __PRETTY_FUNCTION__);
        CoreNaviManager::instance()->setSubtleLightbar (false);
		m_isSubtleLightbar = false;
    }

    if (m_activeTouchpanel) {
		g_debug ("%s: unsetting active touchpanel", __PRETTY_FUNCTION__);
		DisplayManager::instance()->setActiveTouchpanel (false);
		m_activeTouchpanel = false;
    }

    if (m_alsDisabled) {
		g_debug ("%s: unsetting als disabled", __PRETTY_FUNCTION__);
		DisplayManager::instance()->setAlsDisabled (false);
		m_alsDisabled = false;
    }

	showStatusBarAndNotificationArea();

	m_overlayNotificationPosition = WindowProperties::OverlayNotificationsBottom;
	Q_EMIT signalOverlayNotificationPositionChanged(m_overlayNotificationPosition);

	m_suppressBannerMessages = false;
	Q_EMIT signalOverlayNotificationSuppressBannerMessages(m_suppressBannerMessages);

	m_suppressGestures = false;

}

bool SystemUiController::allowSuspend()
{
	return DisplayManager::instance()->allowSuspend() &&
		ApplicationInstaller::instance()->allowSuspend();
}
void SystemUiController::setSuspended (bool isSuspended)
{
	DisplayManager::instance()->setSuspended(isSuspended);
}


void SystemUiController::slotEnterBrickMode(bool val)
{
	if (m_inDockMode)
		enterOrExitDockModeUi (false);

	if (m_dashboardOpened) {
		g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
		Q_EMIT signalCloseDashboard(false);
	}
}

void SystemUiController::slotExitBrickMode()
{
	if (m_dashboardOpened) {
		g_warning ("%s: %d", __PRETTY_FUNCTION__, __LINE__);
		Q_EMIT signalCloseDashboard(false);
	}
}

void SystemUiController::launcherAction(LauncherActions act)
{
	switch (act)
	{
	case SystemUiController::LAUNCHERACT_EDITINGCARDTITLE:
		Q_EMIT signalLauncherActionEditingCardTitle();
		break;
	case SystemUiController::LAUNCHERACT_INFOACTIVE:
		Q_EMIT signalLauncherActionAppInfoWindowActive();
		break;
	case SystemUiController::LAUNCHERACT_MENUACTIVE:
		Q_EMIT signalLauncherActionMenuActive();
		break;
	default:
		break;
	}

}

void SystemUiController::launcherDbgActionScreenGrid(LauncherActions act,int xspan,int yspan)
{
	Q_EMIT signalLauncherActionDbgScreenGrid((act == SystemUiController::LAUNCHERACT_DBG_ACTIVATE_SCREEN_GRID),xspan,yspan);
}

void SystemUiController::launcherMenuOp(LauncherOperations op)
{
	switch (op)
	{
	case SystemUiController::LAUNCHEROP_ADDCARD:
		Q_EMIT signalLauncherAddCard();
		break;
	case SystemUiController::LAUNCHEROP_REORDER:
		Q_EMIT signalLauncherEnterReorderCardMode();
		break;
	case SystemUiController::LAUNCHEROP_DELETECARD:
		Q_EMIT signalLauncherDeleteCard();
		break;
	case SystemUiController::LAUNCHEROP_INVOKERENAMECARD:
		Q_EMIT signalLauncherInvokeRenameCard();
		break;
	case SystemUiController::LAUNCHEROP_UNKNOWN:
		break;			//do nothing...the Q_EMIT menu op end will do the right things for this case
	}
	
	if(op != SystemUiController::LAUNCHEROP_INVOKERENAMECARD)
		Q_EMIT signalLauncherMenuOpEnd();
}

void SystemUiController::launcherChangeCardTitle(int launcherCardId,const std::string& launcherCardLabel)
{
	Q_EMIT signalLauncherRenameCard(launcherCardId, qFromUtf8Stl(launcherCardLabel));
	Q_EMIT signalLauncherActionEnd();
}

QSharedPointer<QPixmap> SystemUiController::loadingStrip()
{
	QSharedPointer<QPixmap> strip;
	strip = m_iconProgressSurf.toStrongRef();
	if (strip.isNull()) {
		Settings* settings = Settings::LunaSettings();
		QString resourcePath = qFromUtf8Stl(settings->lunaSystemResourcesPath);
		QString filePath = resourcePath + "/loading-strip.png";
		strip = QSharedPointer<QPixmap>(new QPixmap(filePath));
		if (strip.isNull() || strip->isNull()) {
			g_warning("Failed to load image '%s'", filePath.toUtf8().data());
		}
		m_iconProgressSurf = strip.toWeakRef();
	}
	return strip;
}

QSharedPointer<QPixmap> SystemUiController::warningIcon()
{
	QSharedPointer<QPixmap> warning;
	warning = m_iconWarningSurf.toStrongRef();
	if (warning.isNull()) {
		Settings* settings = Settings::LunaSettings();
		QString resourcePath = qFromUtf8Stl(settings->lunaSystemResourcesPath);
		QString filePath = resourcePath + "/warning-icon.png";
		warning = QSharedPointer<QPixmap>(new QPixmap(filePath));
		if (warning.isNull() || warning->isNull()) {
			g_warning("Failed to load image '%s'", filePath.toUtf8().data());
		}
		m_iconWarningSurf = warning.toWeakRef();
	}
	return warning;
}

void SystemUiController::aboutToSendSyncMessage()
{
    Q_EMIT signalAboutToSendSyncMessage();
}


static const char* kPhoneAppId      = "com.palm.app.phone";
static const char* kSimToolkitAppId = "com.palm.app.stk";

std::string SystemUiController::getMenuTitleForMaximizedWindow(Window* win)
{
	std::string name;

	if(!win)
		return std::string(" ");

	if((StatusBarServicesConnector::instance()->getPhoneType() != PHONE_TYPE_NONE) &&
	   (StatusBarServicesConnector::instance()->getPhoneType() != PHONE_TYPE_UNKNOWN)  ) {
		// handle phone app exceptions apps here
		if(win->appId() == kPhoneAppId) {
			return std::string("@CARRIER");
		}
		if(win->appId() == kSimToolkitAppId) {
			return std::string("@CARRIER");
		}
	}

	ApplicationDescription* appDesc = ApplicationManager::instance()->getAppById(win->appId());
	if (appDesc)
		name = appDesc->launchPoints().front()->menuName();


	return name;
}

void SystemUiController::handleScreenEdgeFlickGesture(QGesture* gesture)
{
	ScreenEdgeFlickGesture* g = static_cast<ScreenEdgeFlickGesture*>(gesture);
	if (g->state() != Qt::GestureFinished)
		return;

    OrientationEvent::Orientation orientation = WindowServer::instance()->getUiOrientation();
	switch (orientation) {
    case OrientationEvent::Orientation_Up: {
		if (g->edge() != ScreenEdgeFlickGesture::EdgeBottom)
			return;
		break;
	}
    case OrientationEvent::Orientation_Down: {
		if (g->edge() != ScreenEdgeFlickGesture::EdgeTop)
			return;
		break;
	}
    case OrientationEvent::Orientation_Left: {
		if (g->edge() != ScreenEdgeFlickGesture::EdgeLeft)
			return;
		break;
	}
    case OrientationEvent::Orientation_Right: {
		if (g->edge() != ScreenEdgeFlickGesture::EdgeRight)
			return;
		break;
	}
	default: {
		g_warning("Unknown UI orientation");
		return;
	}
	}
	
	// enforce a larger minimum Y distance for the flick gesture when the keyboard is up
	if(IMEController::instance()->isIMEOpened()) {
		if(g->yDistance() < kFlickMinimumYLengthWithKeyboardUp)
			return; // not long enough, so ignore it
	}

	if (m_inDockMode) {
		enterOrExitDockModeUi(false);
		return;
	}

	if (m_deviceLocked)
		return;

	if (m_dashboardOpened) {
		Q_EMIT signalCloseDashboard(true);
	}

	if (m_menuVisible) {
		Q_EMIT signalHideMenu();
	}

	if (m_universalSearchShown) {
		Q_EMIT signalHideUniversalSearch(false, false);
		return;
	}
		
	if (m_launcherShown) {
		Q_EMIT signalToggleLauncher();
		return;
	}

	if ((m_activeCardWindow && m_maximizedCardWindow) || m_cardWindowAboutToMaximize) {
		if (false == m_modalCardWindowActive)
			Q_EMIT signalShowDock();
		
		Q_EMIT signalMinimizeActiveCardWindow();
		return;
	}

	Q_EMIT signalToggleLauncher();					
}

