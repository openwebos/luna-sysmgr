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




#ifndef DOCKMODEWINDOWMANAGER_H
#define DOCKMODEWINDOWMANAGER_H

#include "Common.h"

#include <set>
#include "WindowManagerBase.h"
#include "PtrArray.h"
#include "VariantAnimation.h"
#include "CustomEvents.h"
#include "SystemMenu.h"
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsSceneMouseEvent>
#include <QDeclarativeComponent>
#include <QGestureEvent>
#include <QTextLayout>
#include <QPointer>
#include <QBitArray>

#define MINIMIZED_SCALE_FACTOR  4.0


class Window;
class StatusBar;
class DockModeWindow;
class LaunchPoint;
class DockModeIcons;
class QTapGesture;
class FlickGesture;
class DockModeLaunchPoint;
class PixmapButton;
class DockModeClock;
class DockModeAppMenuContainer;
class DockModePositionManager;

class DockModeWindowManager : public WindowManagerBase
{
	Q_OBJECT

public:

	DockModeWindowManager(uint32_t maxWidth, uint32_t maxHeight);
	virtual ~DockModeWindowManager();

	virtual void init();
	
	
	virtual void addWindow(Window* win);
	virtual void prepareAddWindow(Window* win);
	virtual void removeWindow(Window* win);
	
	bool appLoadingTimedOut(DockModeLaunchPoint* dlp);

	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void setDockModeState(bool enabled);
	void setInModeAnimation(bool animating);

	void resize(int width, int height);

	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);

	void setDashboardOpened(bool val, bool softDismissable);
	bool isDashboardOpened();
	bool isDashboardSoftDismissable();
	void closeDashboard(bool force);

	bool enableDockModeLaunchPoint(const LaunchPoint* lp);
	bool disableDockModeLaunchPoint(const LaunchPoint* lp); 
	
	Window* getActiveDockModeWindow() const { return m_inDockMode ? m_activeWin : 0; }

	void switchApplication(DockModeLaunchPoint*);

	void addPuckIdAndDefaultApp (const std::string& puckId, const std::string& appId);

    virtual bool handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate);
	
Q_SIGNALS:
	
	void signalDockModeStatusChanged(bool enabled);
	void signalCloseDashboard(bool forceClose);
	void signalDockModeAppChanged (DockModeLaunchPoint* lp);
	
private Q_SLOTS:

	void slotBootFinished();
	void slotWindowChangeAnimationFinished();	
	void slotVisibilityChanged();
	void slotLowMemoryActionsRequested(bool allowExpensive);
	void slotDockModeAppSelected (DockModeLaunchPoint* dlp);
	void slotDockModeLaunchPointEnabled (const LaunchPoint* lp);
	void slotDockModeLaunchPointDisabled (const LaunchPoint* lp);
	void slotDockModeAnimationStarted();
	void slotDockModeAnimationComplete();
	void slotPuckConnected(bool connected);
	void slotPositiveSpaceChanged (const QRect& rect);
	void slotLaunchPointRemoved(const LaunchPoint* lp, QBitArray reasons);
	
private:
	
	bool tapAndHoldEvent(QGestureEvent* tapAndHoldEvent);

	int appIndexForId(const std::string appId);
	DockModeLaunchPoint* launchPointForId(const std::string appId);

	bool launchApp(DockModeLaunchPoint* dockApp, bool maximized=false);
	void closeApp(const std::string appId);	
	void closeWindow(Window* win);
	
	bool enableDockModeLaunchPointInternal(const LaunchPoint* lp, bool isPermanent);
	void updateDockModeLaunchPoint(const LaunchPoint* lp); // $$$ Implement this
	
	void queueFocusAction(Window* win, bool focused);
	void performPendingFocusActions();
	void removePendingFocusActionWindow(Window* win);
	
	void positionLauncherIcons(bool immediate = true);
	void configureAllIconsAndWindows();
	
	int findDefaultDlpIndex (const std::string&);
	void animateWindowChange(Window* win);
	
	QRect m_screenBounds;

	bool m_dashboardOpened;
	bool m_dashboardSoftDismissable;

	QPainter::CompositionMode m_compMode, m_previous;
	QPixmap *m_background;
	
	bool m_inDockMode;
	bool m_inTransitionAnimation;
	int  m_defaultIndex;
	OrientationEvent::Orientation m_orientation, m_newOrientation;

	StatusBar* m_statusBar;
	DockModeClock* m_dockModeClock;
	DockModePositionManager* m_dockModePosMgr;
	DockModeAppMenuContainer* m_appMenuContainer;
	QRect m_positiveSpace;

	PtrArray<DockModeLaunchPoint> m_dockLpArray;

	// Pointers (no instances) to Launch points
	DockModeLaunchPoint* m_sysUiLpInUse;
	DockModeLaunchPoint* m_newlyAddedLp;

	Window* m_activeWin;
	Window* m_lastMaximizedWin;
	
	bool m_inReorderMode;
	DockModeLaunchPoint* m_dragLp;
	
	std::set<Window*> m_pendingActionWinSet;

	// animations for fading LockMode in/out
	QPointer<QPropertyAnimation> m_fadeAnimation;
		
	Window* m_windowInAnimation;
	
	// animations for switching maximized apps
	QPropertyAnimation m_windowAnimationCurrent;
	QPropertyAnimation m_windowAnimationNext;
	QParallelAnimationGroup    m_windowChangeAnimationGroup;
	Window* m_nextActiveWin;

	// animations for the Preferences App
	QPointer<QParallelAnimationGroup>    m_systemAppAnimation;
	
	std::map<std::string, int> m_puckIdToDlpIndex;
	std::string m_currentPuckId;
	bool m_bootFinished;
};


#endif /* DOCKMODEWINDOWMANAGER_H */
