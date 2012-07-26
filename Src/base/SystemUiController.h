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




#ifndef SYSTEMUICONTROLLER_H
#define SYSTEMUICONTROLLER_H

#include "Common.h"

#include <string>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QObject>
#include <QRect>
#include <QSharedPointer>
#include <QWeakPointer>

#include "ApplicationDescription.h"
#include "Event.h"
#include "SuspendBlocker.h"
#include "VariantAnimation.h"
#include "Window.h"
#include "CustomEvents.h"
#include "StatusBar.h"

class CardWindow;
class SysMgrKeyEvent;

QT_BEGIN_NAMESPACE
class QPixmap;
QT_END_NAMESPACE

class QGesture;

class SystemUiController : public QObject
{
	Q_OBJECT

public:

	static SystemUiController* instance();
	~SystemUiController();

	void init();

    bool handleEvent(QEvent *event);
    bool handleKeyEvent(QKeyEvent *event);
    bool handleMouseEvent(QMouseEvent *event);
    bool handleGestureEvent(QGestureEvent *event);
    bool handleCustomEvent(QEvent *event);

	void setCardWindowAboutToMaximize();

	void setCardWindowMaximized(bool val);

	void updateStatusBarTitle();

	void setLauncherShown(bool val);

	void setUniversalSearchShown(bool val);

	void setDockShown(bool val);

	void showOrHideUniversalSearch(bool show, bool showLauncher, bool speedDial);

	void showOrHideLauncher(bool show);

	void showOrHideDock(bool show);

	void setDashboardOpened(bool val, bool isSoftDismissable);

	void setDashboardHasContent(bool val);

	void setAlertVisible(bool val);

	void setEmergencyMode(bool enable);

	void setActiveCardWindow(Window* window);

	void setMaximizedCardWindow(Window* window);

	void setRequestedSystemOrientation(Event::Orientation orient, bool reasonCardMaximizing = false, bool skipAnimation = false);

	void setLauncherWindow(Window* window) { m_launcherWindow = window; }

	void setUniversalSearchWindow(Window* window) { m_universalSearchWindow = window; }

	void focusMaximizedCardWindow(bool enable);
	
	void enterOrExitCardReorder(bool entered);

	bool doesDashboardHaveContent() const {return m_dashboardHasContent;}

	void enterOrExitDockModeUi(bool enter);

	// The order of the layers below defines their priorities in terms of direct rendering
	enum DirectRenderingEnabledLayers
	{
		DISABLE_ALL_DIRECT_RENDERING = 0, // virtual layer used to temporarily completely disable direct rendering  
		DOCK_MODE_WINDOW_MANAGER,
		TOP_LEVEL_WINDOW_MANAGER,
		EMERGENCY_MODE_WINDOW_MANAGER,   // Temporarily demoted Emergency Mode WM in Dartfish (it is being used for Flash full screen)
		OVERLAY_WINDOW_MANAGER,
		CARD_WINDOW_MANAGER,
		NUMBER_OF_LAYERS
	};

	void setDirectRenderingForWindow(DirectRenderingEnabledLayers layer, CardWindow* win, bool enable, bool force=false);
	void enableDirectRendering(bool enable);

	void setLauncherVisible(bool visible, bool fullyVisible);
	void setDockVisible(bool val);
	void setMenuVisible(bool val);
	void setDeviceLocked(bool val);
	void setDockMode(bool inDockMode);
	void setInLauncherReorder(bool reorder);
	
	void toggleCurrentAppMenu();

	void openDashboard();
	void closeDashboard(bool force);

	bool isCardWindowMaximized( ) const { return m_cardWindowMaximized; }
	bool isLauncherShown( ) const;
	bool isUniversalSearchShown() { return m_universalSearchShown; }
	bool isDockShown( ) const;
	bool isDashboardOpened( ) const { return m_dashboardOpened; }
	bool isInDockMode( ) const { return m_inDockMode; }
	bool isScreenLocked() const { return (m_deviceLocked); }
    bool isInEmergencyMode() const {return m_emergencyMode; }
    bool isInFullScreenMode();
    bool isModalWindowActive() {return m_modalCardWindowActive;}

	const Window* launcherWindow() const { return m_launcherWindow; }
	const Window* universalSearchWindow() const { return m_universalSearchWindow; }

	Window* activeCardWindow() const { return m_activeCardWindow; }

	Window* activeWindow() const;

	std::string activeApplicationName();

	Window* maximizedCardWindow() const { return m_maximizedCardWindow; }

	std::string maximizedApplicationName() const;
	std::string maximizedApplicationMenuName() const;
	std::string maximizedApplicationId() const;

	// this call lays out the positive and negative spaces
	void layout();

	bool okToResizeUi();
	void resizeAndRotateUi(int width, int height, int rotationAngle);
	void rotationStarting();
	void rotationComplete();
	bool isUiRotating() { return m_uiRotating; }
	int  getRotationAngle() { return m_uiRotating ? m_rotationAngle : 0; }

	bool dashboardOwnsNegativeSpace() const { return m_dashboardOwnsNegativeSpace; }

	void setUiRootItemPtr(QGraphicsItem* ptr) { m_uiRootItemPtr = ptr; }

	int currentUiWidth() { return m_uiWidth; }
	int currentUiHeight() { return m_uiHeight; }
	bool isUiInPortraitMode() { return (m_uiWidth < m_uiHeight); }

	void setMinimumPositiveSpaceHeight(int val);
	void setMaximumPositiveSpaceHeight(int val);
	int minimumPositiveSpaceHeight() const;
	int maximumPositiveSpaceHeight() const;
	int minimumPositiveSpaceBottom() const;
	int maximumPositiveSpaceBottom() const;
	bool changeNegativeSpace(int newTop, bool openingDashboard, bool immediate=false);

	const QRect& positiveSpaceBounds() { return m_positiveSpace; }
	const QRect& negativeSpaceBounds() { return m_negativeSpace; }

	void hideStatusBarAndNotificationArea();
	void showStatusBarAndNotificationArea();
	bool statusBarAndNotificationAreaShown() const { return m_statusBarAndNotificationShown; }

	void setBootFinished();
	bool bootFinished() const;

	void updateProperties (Window* win, const WindowProperties& props);

	void notifyAlertActivated() { Q_EMIT signalAlertActivated(); }
	void notifyAlertDeactivated() { Q_EMIT signalAlertDeactivated(); }
	void notifyTransientAlertActivated() { Q_EMIT signalTransientAlertActivated(); }
	void notifyTransientAlertDeactivated() { Q_EMIT signalTransientAlertDeactivated(); }
	void notifyBannerActivated() { Q_EMIT signalBannerActivated(); }
	void notifyBannerDeactivated() { Q_EMIT signalBannerDeactivated(); }
	void notifyBannerAboutToUpdate(QRect& scrBounds) { Q_EMIT signalBannerAboutToUpdate(scrBounds); }
	void notifyModalWindowActivated(Window* modalParent);
	void notifyModalWindowDeactivated();
	Window* getModalParent() const {return m_parentOfModalWindow; }
	void notifyEditorFocusChanged(bool focus) { Q_EMIT signalEditorFocusChanged(focus); }

	QSharedPointer<QPixmap> loadingStrip();
	QSharedPointer<QPixmap> warningIcon();

	void aboutToSendSyncMessage();

	void setStatusBar(StatusBar* statusBar) { m_statusBarPtr = statusBar; }
	const StatusBar* statusBar() const { return m_statusBarPtr; }

	enum ModalWinLaunchErrorReason {
		LaunchUnknown = -1,
		NoErr = 0,
		MalformedRequest,
		NoMaximizedCard,
		ParentDifferent,
		AnotherModalActive,
		AppToLaunchDoesntExist,
		AppToLaunchIsntReady,
		//AppToLaunchIsNotHeadless,
		AnotherInstanceAlreadyRunning,
		MissingAppDescriptor,
		MissingProcessId,
		InternalError,
		LaunchErrorMax
	};

	enum ModalWinDismissErrorReason {
		DismissUnknown = (LaunchErrorMax+1),
		HomeButtonPressed,
		ServiceDismissedModalCard,
		ParentCardDismissed,
		ActiveCardsSwitched,
		UiMinimized,
	};

	void setModalWindowDismissErrReason(ModalWinDismissErrorReason reason) { if(m_modalWindowDismissErr != reason) m_modalWindowDismissErr = reason; }
	void setModalWindowLaunchErrReason(ModalWinLaunchErrorReason reason) { if(m_modalWindowLaunchErr != reason) m_modalWindowLaunchErr = reason; }

	bool wasModalAddedSuccessfully() {  return(m_modalWindowLaunchErr == NoErr)? true : false; }

	std::string getModalWindowLaunchErrReason();
	std::string getModalWindowDismissErrReason();
	int getModalWindowLaunchErrReasonCode() {return m_modalWindowLaunchErr; }
	int getModalWindowDismissErrReasonCode() {return m_modalWindowDismissErr; }

public:
	void cardWindowAdded();
	void cardWindowTimeout();

public:

	enum LauncherOperations
	{
			LAUNCHEROP_ADDCARD,
			LAUNCHEROP_REORDER,
			LAUNCHEROP_DELETECARD,
			LAUNCHEROP_INVOKERENAMECARD,
			LAUNCHEROP_UNKNOWN						//keep this as the last one
	};

	enum LauncherActions
	{
			LAUNCHERACT_MENUACTIVE,
			LAUNCHERACT_INFOACTIVE,
			LAUNCHERACT_EDITINGCARDTITLE,
			LAUNCHERACT_DBG_ACTIVATE_SCREEN_GRID,
			LAUNCHERACT_DBG_DEACTIVATE_SCREEN_GRID,
	};
	
	void launcherAction(LauncherActions act);
	void launcherMenuOp(LauncherOperations op);
	void launcherChangeCardTitle(int launcherCardId,const std::string& launcherCardLabel);

	void launcherDbgActionScreenGrid(LauncherActions act,int xspan=0,int yspan=0);

	uint32_t getOverlayNotificationPosition() { return m_overlayNotificationPosition; }

	// Signals

Q_SIGNALS:
	void signalCardWindowAdded();
	void signalCardWindowTimeout();

	void signalPositiveSpaceChanged(const QRect& r);
	void signalPositiveSpaceAboutToChange(const QRect& r, bool fullScreen, bool screenResizing);
	void signalPositiveSpaceChangeFinished(const QRect& r);
	void signalNegativeSpaceChanged(const QRect& r);
	void signalNegativeSpaceAboutToChange(const QRect& r, bool fullScreen, bool screenResizing);
	void signalNegativeSpaceChangeFinished(const QRect& r);
	void signalEmergencyMode(bool enable);
	void signalEmergencyModeHomeButtonPressed();

	void signalFocusMaximizedCardWindow(bool enable);
	void signalMaximizedCardWindowChanged(Window* win);
	void signalMaximizeActiveCardWindow();
	void signalMinimizeActiveCardWindow();
	void signalCardWindowMaximized();
	void signalCardWindowMinimized();
	void signalChangeCardWindow(bool next);

	void signalHideMenu();

	void signalLauncherVisible(bool visible, bool fullyVisible);
	void signalLauncherShown(bool shown);
	void signalShowUniversalSearch();
	void signalHideUniversalSearch(bool showLauncher, bool speedDial);
	void signalUniversalSearchFocusChange(bool enabled);

	void signalShowLauncher();
	void signalHideLauncher();
	void signalToggleLauncher();
	void signalLauncherAddCard();
	void signalLauncherEnterReorderCardMode();
	void signalLauncherRenameCard(int,QString);
	void signalLauncherDeleteCard();
	void signalLauncherInvokeRenameCard();

	void signalLauncherActionEditingCardTitle();
	void signalLauncherActionAppInfoWindowActive();
	void signalLauncherActionMenuActive();
	void signalLauncherActionDbgScreenGrid(bool on,int xspan,int yspan);

	void signalLauncherMenuOpEnd();
	void signalLauncherActionEnd();

	void signalShowDock();
	void signalHideDock();
	void signalFadeDock(bool fadeOut);

	void signalOpenDashboard();
	void signalCloseDashboard(bool forceClose);
	void signalCloseAlert();

	void signalDockModeEnable(bool enabled);
	void signalChangeDockModeApp(bool next);
	void signalBackGestureRejectedDockMode();

	void signalBootFinished();

	void signalStatusBarAndNotificationShown(bool shown);

	void signalOverlayNotificationPositionChanged(uint32_t newPosition);
	void signalOverlayNotificationSuppressBannerMessages(bool suppress);

	void signalAlertActivated();
	void signalAlertDeactivated();
	void signalTransientAlertActivated();
	void signalTransientAlertDeactivated();
	void signalBannerActivated();
	void signalBannerDeactivated();
	void signalBannerAboutToUpdate(QRect& scrBounds);

	void signalEditorFocusChanged(bool focus);

	void signalAboutToSendSyncMessage();

	void signalDockModeStatusBarToggle();

	void signalModalWindowAdded();

	void signalModalWindowRemoved();

	void signalUiRotationAboutToStart();
	void signalUiRotationCompleted();

	void signalEmergencyModeWindowFocusChange(bool focus);

private Q_SLOTS:

	void slotEnterBrickMode(bool val);
	void slotExitBrickMode();

	void slotCopy();
	void slotCut();
	void slotPaste();
	void slotSelectAll();

	void slotAnimFinished();

	void slotKeyEventRejected(const SysMgrKeyEvent& event);
private:

	SystemUiController();
	void startPositiveSpaceAnimation(const QRect& start, const QRect& end);
	void stopPositiveSpaceAnimation();
	void animValueChanged(const QVariant& value);
	bool allowSuspend();
	void setSuspended(bool);
	void handleScreenEdgeFlickGesture(QGesture* gesture);

	Window* m_parentOfModalWindow;
	Window* m_activeCardWindow;
	Window* m_maximizedCardWindow;
	Window* m_launcherWindow;
	Window* m_universalSearchWindow;

	bool m_cardWindowAboutToMaximize;
	bool m_cardWindowMaximized;
	bool m_dashboardOpened;
	bool m_dashboardSoftDismissable;
	bool m_dashboardHasContent;
	bool m_alertVisible;
	const bool m_dashboardOwnsNegativeSpace;
	bool m_launcherShown;
	bool m_dockShown;
	bool m_inDockMode;
    bool m_universalSearchShown;

	bool m_emergencyMode;

	QGraphicsItem* m_uiRootItemPtr;

	QRect m_positiveSpace;
	QRect m_negativeSpace;
	int m_minimumPositiveSpaceHeight;
	int m_maximumPositiveSpaceHeight;

	bool m_launcherVisible;
	bool m_dockVisible;
	bool m_menuVisible;
	bool m_deviceLocked;

	bool m_started;
	int m_startCount;

	int m_dockBrightness;
	bool m_statusBarAndNotificationShown;

	int m_requestedNegativeSpaceTop;

	int m_uiWidth, m_uiHeight;

	VariantAnimation<SystemUiController>* m_anim;

	bool m_bootFinished;

	bool m_uiRotating;
	int  m_rotationAngle;

	SuspendBlocker<SystemUiController> m_suspendBlocker;

	void applyWindowProperties (Window* win);
	void removeWindowProperties ();
	bool m_isBlockScreenTimeout;
	bool m_isSubtleLightbar;
	bool m_activeTouchpanel;
	bool m_alsDisabled;
	uint32_t m_overlayNotificationPosition;
	bool m_suppressBannerMessages;
	bool m_suppressGestures;
	bool m_modalCardWindowActive;

    // members used for Focus/DirectRendering control
    struct DirectRenderLayer {
    	bool        requestedDirectRendring;
    	CardWindow* activeWindow;
    };
    
    DirectRenderLayer  m_directRenderLayers[NUMBER_OF_LAYERS];
    int                m_currentlyDirectRenderingLayer;
    CardWindow*        m_currentlyDirectRenderingWindow;

    ModalWinLaunchErrorReason m_modalWindowLaunchErr;
    ModalWinDismissErrorReason m_modalWindowDismissErr;

private:

	SystemUiController(const SystemUiController&);
	SystemUiController& operator=(const SystemUiController&);

	std::string getMenuTitleForMaximizedWindow(Window* win);

	QWeakPointer<QPixmap> m_iconProgressSurf;
	QWeakPointer<QPixmap> m_iconWarningSurf;

	StatusBar*   m_statusBarPtr;
};


#endif /* SYSTEMUICONTROLLER_H */
