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




#ifndef OVERLAYWINDOWMANAGER_H
#define OVERLAYWINDOWMANAGER_H

#include "Common.h"

#include "Timer.h"
#include "sptr.h"
#include "Event.h"
#include "WindowManagerBase.h"
#include "dimensionsmain.h"
#include "dimensionstypes.h"
#include "pixmaploader.h"
#include "pixmapobject.h"

#include <vector>
#include <string>

#include <QGraphicsSceneMouseEvent>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QTapAndHoldGesture>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QGestureEvent;
class QGraphicsObject;
class QStateMachine;
class QState;
QT_END_NAMESPACE

class DragWindow;
class LauncherFeedbackWindow;
class FlickGesture;
class LaunchPoint;
class SearchPill;

class OverlayWindowManager : public WindowManagerBase
{
	Q_OBJECT
	Q_ENUMS(LauncherState)
	Q_ENUMS(DockState)
	Q_ENUMS(SearchPillState)
	Q_ENUMS(UniversalSearchState)
	Q_PROPERTY(DockState dockState READ dockState WRITE setDockState)
	Q_PROPERTY(LauncherState launcherState READ launcherState WRITE setLauncherState)
	Q_PROPERTY(SearchPillState searchPillState READ searchPillState WRITE setSearchPillState)
	Q_PROPERTY(UniversalSearchState universalSearchState READ universalSearchState WRITE setUniversalSearchState)
	Q_PROPERTY(bool dockShown READ dockShown WRITE setDockShown)
	Q_PROPERTY(bool universalSearchShown READ universalSearchShown WRITE setUniversalSearchShown)
	Q_PROPERTY(QRectF quickLaunchVisibleArea READ quickLaunchVisibleArea);
	Q_PROPERTY(QPointF quickLaunchVisiblePosition READ quickLaunchVisiblePosition);

public:

	enum LauncherState {
		StateNoLauncher = 1,
		StateLauncherRegular,
		StateLauncherReorder,
	};

	enum DockState {
		StateNoDock = 1,
		StateDockNormal,
		StateDockReorder,
	};

	enum SearchPillState {
		StateSearchPillHidden = 1,
		StateSearchPillVisible
	};

	enum UniversalSearchState {
		StateUSearchHidden = 1,
		StateUSearchVisible
	};

	OverlayWindowManager(int maxWidth, int maxHeight);
	~OverlayWindowManager();

	static OverlayWindowManager * systemActiveInstance();

	void init();

	// property 'quickLaunchVisibleArea' read function
	QRectF quickLaunchVisibleArea() const;

	// property 'quickLaunchVisiblePosition' read function
	QPointF quickLaunchVisiblePosition() const;

	// property 'universalSearchState' access functions
	UniversalSearchState universalSearchState() const { return m_uSearchState; }
	void setUniversalSearchState(UniversalSearchState s) { m_uSearchState = s; }

	// property 'searchPillState' access functions
	SearchPillState searchPillState() const { return m_searchPillState; }
	void setSearchPillState(SearchPillState s) { m_searchPillState = s; }

	// property 'dockState' access functions	
	DockState dockState() const { return m_dockState; }
	void setDockState(DockState s) { m_dockState = s; }

	// property 'launcherState' access functions
	LauncherState launcherState() const { return m_launcherState; }
	void setLauncherState(LauncherState s) { m_launcherState = s; }

	// property 'dockShown' access functions
	bool dockShown() const;
	void setDockShown(bool shown);

	// property 'universalSearchShown' access functions
	bool universalSearchShown() const;
	void setUniversalSearchShown(bool shown);

	void addWindow(Window* win);

	bool launcherVisible() const;

	// launch feedback for the Universal Search window
	void applyLaunchFeedback(int centerX, int centerY);

	int lastPenX() const { return m_lastPenX; }
	int lastPenY() const { return m_lastPenY; }

	bool inDrag() const { return m_inDrag; }
	bool canRedirectDragToQuicklaunch(const QPointF& dragPosition);

	bool okToResize();
	void resize(int width, int height);

    // REMOVE ME: this is a workaround to make the quick launch toggle the new launcher
    void toggleLauncher() { slotSystemAPIToggleLauncher(); }

    Quicklauncher * quicklaunchBar() const { return m_dockWin; }

    virtual bool handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate);

protected:

	bool sceneEvent(QEvent* event);
	void mousePressEvent(QGraphicsSceneMouseEvent* event);
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	void keyPressEvent(QKeyEvent* event);
	void keyReleaseEvent(QKeyEvent* event);

	bool mouseFlickEvent(FlickGesture* flick);

private:

	bool dockInAnimation() const;
	void mapCoordToWindow(Window* win, int& x, int& y) const;

	void setupSearchPill();
	void setupDock();
	bool setupLauncher();
	void setupDragWindow();
	void setupFeedbackWindow();

	void setupLauncherAnimations();
	void setupDockAnimations();
	void setupSearchPillAnimations();
	void setupUniversalSearchAnimations();

	void setupDockStateMachine();
	void setupLauncherStateMachine();
	void setupSearchPillStateMachine();
	void setupUniversalSearchStateMachine();
	
	QState* createState(QString name, QState *parent = NULL);

	bool handlePenDownEvent(Event* event);
	bool handlePenMoveEvent(Event* event);
	bool handlePenUpEvent(Event* event);
	bool handlePenFlickEvent(Event* event);
	bool handlePenCancelEvent(Event* event);
	bool handlePressAndHoldEvent(Event* event);

	bool handlePenMoveEventStateNormal(Event* event);
	void handlePenMoveEventStateStache(Event* event);

	bool handlePenUpEventStateNormal(Event* event);
	bool handlePenUpEventStateStache(Event* event);
	
	void handlePenCancelEventStateStache(Event* event);

	void cancelReorder();

//THESE ARE INTERNAL STATE MACHINE TRIGGERS ONLY!!! DO NOT HOOK UP TO EXTERNAL SLOTS
Q_SIGNALS:

	void signalFSMShowSearchPill();
	void signalFSMHideSearchPill();

	void signalFSMShowLauncher();
	void signalFSMHideLauncher();

	void signalFSMShowUniversalSearch();
	void signalFSMHideUniversalSearch();

	void signalFSMShowDock();                          // show dock (quicklaunch)
	void signalFSMHideDock();                          // hide dock (quicklaunch)

	void signalFSMDragStarted();
	void signalFSMDragEnded();
/////
//THESE ARE INTERNAL STATE MACHINE TRIGGERS ONLY!!! DO NOT HOOK UP TO EXTERNAL SIGNALS
private Q_SLOTS:

	void slotFSMUniversalSearchVisible();
	void slotFSMUniversalSearchHidden();
	void slotUniversalSearchFocusChange (bool enabled);
/////

Q_SIGNALS:
	void signalPenUpEvent();                        // release event
	void signalPenCancelEvent();                    // cancel event
	void signalLauncherOpened();                    // launcher was opened
	void signalLauncherClosed();                    // launcher was closed 
	void signalUniversalSearchOpened();				//  (obvious)
	void signalUniversalSearchClosed();

private Q_SLOTS:

	//
	void slotSystemAPIToggleLauncher();
	void slotSystemAPICloseLauncher();
	void slotSystemAPIOpenLauncher();
	void slotSystemAPIRecreateLauncher();

	//////////// ------------------

	void slotLauncherOpenState();
	void slotLauncherClosedState();

	void slotPositiveSpaceAboutToChange(const QRect& r, bool fullScreen, bool screenResizing);
	void slotPositiveSpaceChangeFinished(const QRect& r);
	void slotPositiveSpaceChanged(const QRect& r);
	void slotCardWindowMaximized();
	void slotCardWindowMinimized();
	void slotEmergencyModeChanged(bool enabled);

	// These are used by the Dimensions Launcher itself to ask for things to happen. Don't call/trigger from anywhere else.
	// (use the other, external slot interfaces for general launcher control from outside OWM)
	void slotLauncherRequestShowDimensionsLauncher(DimensionsTypes::ShowCause::Enum cause = DimensionsTypes::ShowCause::None);
	void slotLauncherRequestHideDimensionsLauncher(DimensionsTypes::HideCause::Enum cause = DimensionsTypes::HideCause::None);

	void slotLauncherReady();
	void slotLauncherNotReady();

	void slotQuickLaunchReady();
	void slotQuickLaunchNotReady();

	void slotShowDock();
	void slotHideDock();

	void slotStartShowLauncherSequence();
	void slotStartHideLauncherSequence();

	void slotAnimateShowDock();
	void slotAnimateHideDock();

	void slotAnimateFadeOutSearchPill();
	void slotAnimateFadeInSearchPill();

	void slotAnimateHideUniversalSearch();
	void slotAnimateShowUniversalSearch();

	void slotFadeDock(bool fadeOut);
	void slotLauncherOpened();
	void slotLauncherClosed();

	void slotShowUniversalSearch(bool hideLauncher=true);
	void slotHideUniversalSearch(bool showLauncher, bool speedDial);

	void launcherAnimationFinished();
	void dockAnimationFinished();
	void universalSearchOpacityAnimationFinished();

	// $$$ DEBUG - remove
	void dockStateTransition();
	void launcherStateTransition();

	void slotDisplayStateChange(int);

private:

	QPropertyAnimation * dimensionsLauncherShowUpAnimation();
	void dimensionsLauncherShowUpAnimation(QPropertyAnimation * p_anim);
	QPropertyAnimation * dimensionsLauncherHideDownAnimation();
	void dimensionsLauncherHideDownAnimation(QPropertyAnimation * p_anim);

	void fadeOutSearchPillAnimation(QPropertyAnimation * p_anim);
	void fadeInSearchPillAnimation(QPropertyAnimation * p_anim);

	void fadeOutUniversalSearchAnimation(QPropertyAnimation * p_anim);
	void fadeInUniversalSearchAnimation(QPropertyAnimation * p_anim);

	//call this after a resize of OWM of any kind, so that it can reposition; there is no animation here, just a setPos
	void	dimensionsLauncherAdjustPositionOnResize();

	void animateShowLauncher();
	void animateHideLauncher();

private:

	enum DragIconState {
		DragInvalid,
		DragQuicklaunch,
		DragLauncher,
	};

	enum PenDownState {
		PenDownInvalid,
		PenDownInSearch
	};
	
	QRectF		m_geom;
	QSize		m_screenSize;		//precomputed from m_geom, aligned to px coords
	Quicklauncher * m_dockWin;
	bool        m_dockShown;
	QPoint      m_dockShownPos;
	QPoint		m_dockHiddenPos;
	
	int m_dockOffsetFromBottom;
	int m_oldDockOffsetFromBottom;

	Window* m_universalSearchWin;
	bool m_universalSearchShown;
	bool m_ignoreKeyDueToSpeedDialEvent;
	int  m_keyDown;

	SearchPill* m_searchPill;
	DragWindow* m_dragWin;

	int m_lastPenX, m_lastPenY;
	bool m_dockWasShownBeforeDrag;

	bool m_inDrag;

	PenDownState m_penDownState;

	bool m_dockHasMetFinger;

	LauncherFeedbackWindow* m_feedbackWindow;
	QSize m_feedbackOffset;
	
	QPointer<QStateMachine> m_searchPillStateMachine;
	QPointer<QStateMachine> m_dockStateMachine;
	QPointer<QStateMachine> m_launcherStateMachine;
	QPointer<QStateMachine> m_universalSearchStateMachine;

	struct SearchPillStateManager {
		QState * pillHidden;
		QState * pillVisible;
	};
	
	struct DockStateManager {
		QState *noDock;
		QState *dockNormal;
		QState *dockReorder;
	};

	struct LauncherStateManager {
		QState *noLauncher;
		QState *launcherRegular;
		QState *launcherReorder;
	};

	struct UniversalSearchStateManager {
		QState *uSearchHidden;
		QState *uSearchVisible;
	};

	SearchPillStateManager *m_searchPillStates;
	DockStateManager     *m_dockStates;
	LauncherStateManager *m_launcherStates;
	UniversalSearchStateManager *m_uSearchStates;

	SearchPillState m_searchPillState;
	DockState     m_dockState;
	LauncherState m_launcherState;
	DragIconState m_dragSource;
	UniversalSearchState m_uSearchState;

	// these animatios are used by the State Machines ONLY
	QPropertyAnimation *m_animateLauncherOpacity;
	QPointer<QPropertyAnimation> m_qp_animateUniversalSearchOpacity;
	QPointer<QPropertyAnimation> m_qp_animateSearchPillOpacity;

	// these are the animations that we control directly (not via the state machine)
	QPropertyAnimation *m_dockOpacityAnimation;
	QPropertyAnimation *m_dockPosAnimation;	
    QPropertyAnimation *m_dockBgOpacityAnimation;

	QPointer<QPropertyAnimation> m_qp_launcherPosAnimation;
	
	bool m_pendingLauncherRecreate;

	friend class WindowServerLuna;

};

Q_DECLARE_METATYPE(OverlayWindowManager::DockState)
Q_DECLARE_METATYPE(OverlayWindowManager::LauncherState)

#endif /* OVERLAYWINDOWMANAGER_H */
