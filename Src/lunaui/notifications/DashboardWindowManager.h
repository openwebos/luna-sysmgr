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




#ifndef DASHBOARDWINDOWMANAGER_H
#define DASHBOARDWINDOWMANAGER_H

#include "Common.h"
#include "Timer.h"
#include "Window.h"
#include "WindowManagerBase.h"
#include "CustomEvents.h"
#include "GraphicsItemContainer.h"
#include "Settings.h"

#include <list>
#include <map>
#include <QVector>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

class QGraphicsSceneMouseEvent;
class QStateMachine;
class QState;
class QEvent;

class DWMStateBase;
class DWMStateOpen;
class DWMStateAlertOpen;
class DWMStateClosed;

class AlertWindow;
class BannerWindow;
class DashboardWindow;
class DashboardWindowContainer;
class GraphicsItemContainer;
class NotificationPolicy;
class QDeclarativeComponent;

class DashboardWindowManager : public WindowManagerBase
{
	Q_OBJECT

public:

	DashboardWindowManager(int maxWidth, int maxHeight);
	~DashboardWindowManager();

	void init();
	void hideOrCloseAlertWindow(AlertWindow* win);
	void notifyActiveAlertWindowActivated(AlertWindow* win);
	void notifyActiveAlertWindowDeactivated(AlertWindow* win);
	void notifyTransientAlertWindowActivated(AlertWindow* win);
	void notifyTransientAlertWindowDeactivated(AlertWindow* win);
	void setBannerHasContent(bool val);
	void raiseAlertWindow(AlertWindow* window);
	void raiseDashboardWindowContainer();
	void raiseBackgroundWindow();
	void openDashboard();
	void sendClicktoDashboardWindow(int num, int x, int y, bool whileLocked);
	void focusWindow(Window* win);
	void unfocusWindow(Window* win);
	void hideDashboardWindow();
	void animateAlertWindow();
	void animateTransientAlertWindow(bool in);
	int bannerWindowHeight();
	void resizeAlertWindowContainer(AlertWindow*, bool);
	void setInDockMode(bool dockMode);

	inline bool alertOpen() const {
		return m_stateCurrent == (DWMStateBase*)m_stateAlertOpen;
	}

	inline bool alertVisible() const {
		return m_alertWinContainer->isVisible();
	} // Remove this

	inline const Window* getActiveAlertWin() const {
		return m_alertWinArray.empty() ? 0 : (const Window*) m_alertWinArray[0];
	}

	inline const Window* getActiveTransientAlertWin() const {
		return (const Window*) m_activeTransientAlert;
	}

	inline NotificationPolicy* notificationPolicy() const {
		return m_notificationPolicy;
	}

	inline AlertWindow* topAlertWindow() const {
		if (m_alertWinArray.empty())
			return 0;

		return m_alertWinArray[0];
	}

	inline DashboardWindowContainer* dashboardWindowContainer() const {
		return m_dashboardWinContainer;
	}

	inline QGraphicsObject* dashboardMenu() const {
		return m_menuObject;
	}

	inline void currentStateChanged(DWMStateBase* state) {
		m_stateCurrent = state;
	}
	bool isInDockModeAnimation() const { return m_inDockModeAnimation; }

	
	bool hasDashboardContent() const;
	bool canCloseDashboard() const;
	bool dashboardOpen() const;
	bool isOverlay() const { return m_isOverlay; }

	static int sTabletUiWidth();

	enum AlertWindowFadeOption {
		Invalid = 0,
		FadeInOnly,
		FadeInAndOut,
		FadeOutOnly
	};

	inline AlertWindowFadeOption getWindowFadeOption() const {
		return m_AlertWindowFadeOption;
	}

// private functions.
private:

	int activeAlertWindowHeight();
	void setupStateMachine();
	void resize(int width, int height);
	void resizeAlertWindows(int width);
	void negativeSpaceChanged(const QRect& r);
	void addWindow(Window* win);
	void removeWindow(Window* win);
	void addAlertWindow(AlertWindow* win);
	void addTransientAlertWindow(AlertWindow* win);
	void removeAlertWindow(AlertWindow* win);
	void removeTransientAlertWindow(AlertWindow* win);
	void addAlertWindowBasedOnPriority(AlertWindow* win);
	bool doesMousePressFallInBannerWindow(QGraphicsSceneMouseEvent* event);

	void mousePressEvent(QGraphicsSceneMouseEvent* event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	bool sceneEvent(QEvent* event);

	// Override the signals in DashboardWindowManagerBase
Q_SIGNALS:
	void signalActiveAlertWindowChanged();
	void signalOpen();
	void signalClose(bool forceClose);
	void signalAlertWindowActivated(AlertWindow*);
	void signalAlertWindowDeactivated();

private Q_SLOTS:
	void slotPositiveSpaceChanged(const QRect& r);
	void slotPositiveSpaceChangeFinished(const QRect& r);
	void slotNegativeSpaceChanged(const QRect& r);
	void slotNegativeSpaceChangeFinished(const QRect& r);
	void slotOpenDashboard();
	void slotCloseDashboard(bool forceClose);
	void slotCloseAlert();
	void slotDashboardWindowAdded(DashboardWindow* w);
	void slotDashboardWindowsRemoved(DashboardWindow* w);
	void slotDashboardViewportHeightChanged();
	void slotDeleteAnimationFinished();
	void slotTransientAnimationFinished();
	void slotDashboardAreaRightEdgeOffset(int offset);

	void slotDockModeAnimationStarted();
	void slotDockModeAnimationComplete();

// private data
private:

	DashboardWindowManager();
	DashboardWindowManager(const DashboardWindowManager&);
	const DashboardWindowManager& operator=(const DashboardWindowManager&);
	void positionDashboardContainer(const QRect& posSpace = QRect());
	void positionAlertWindowContainer(const QRect& posSpace = QRect());
	void positionTransientWindowContainer(const QRect& posSpace = QRect());

	QParallelAnimationGroup m_fadeInOutAnimation;
	QPropertyAnimation m_transAlertAnimation;
	AlertWindowFadeOption m_AlertWindowFadeOption;
	QStateMachine* m_stateMachine;
	DWMStateOpen* m_stateDashboardOpen;
	DWMStateAlertOpen* m_stateAlertOpen;
	DWMStateClosed* m_stateClosed;
	DWMStateBase* m_stateCurrent;
	BannerWindow* m_bannerWin;
	GraphicsItemContainer* m_bgItem;
	GraphicsItemContainer* m_alertWinContainer;
	GraphicsItemContainer* m_transientContainer;
	DashboardWindowContainer* m_dashboardWinContainer;
	NotificationPolicy* m_notificationPolicy;
	QVector<AlertWindow*> m_alertWinArray;
	AlertWindow* m_activeTransientAlert;
	AlertWindow* m_deleteWinAfterAnimation;
	bool m_deleteTransientWinAfterAnimation;
	AlertWindow* m_previousActiveAlertWindow;

	bool m_isOverlay;
	bool m_bannerHasContent;
	int m_dashboardRightOffset;
	friend class BannerWindow;

	QDeclarativeComponent* m_qmlNotifMenu;
	// Top Level Menu Object
	QGraphicsObject* m_menuObject;
	int m_notifMenuRightEdgeOffset;
	//bool m_goingUp;
	bool m_inDockModeAnimation;
};

#endif /* DASHBOARDWINDOWMANAGER_H */
