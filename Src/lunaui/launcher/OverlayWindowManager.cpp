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

#include "OverlayWindowManager.h"
#include "OverlayWindowManager_p.h"

#include "CardWindowManager.h"

#include <string>
#include <algorithm>
#include <SysMgrDefs.h>

#include <QPropertyAnimation>
#include <QEvent>
#include <QGesture>
#include <QGestureEvent>
#include <QtDebug>
#include <QSequentialAnimationGroup>
#include <QGraphicsObject>
#include <QStateMachine>
#include <QSignalTransition>
#include <QEventTransition>
#include <QHistoryState>
#include <QKeyEventTransition>
#include <QApplication>
#include <QMouseEvent>

#include "AnimationSettings.h"
#include "ApplicationManager.h"
#include "ApplicationDescription.h"
#include "FlickGesture.h"
#include "LaunchPoint.h"
#include "WebAppMgrProxy.h"
#include "Settings.h"
#include "Window.h"
#include "WindowServer.h"
#include "HostBase.h"
#include "SystemUiController.h"
#include "HostWindow.h"
#include "Logging.h"
#include "Time.h"
#include "Localization.h"
#include "HostWindow.h"

#include "cjson/json.h"

#include "DisplayManager.h"
#include "IMEController.h"

#include "gfxsettings.h"
#include "layoutsettings.h"

#include <glib.h>

static const int kSlopFactorForClicks = 4;

static const double kDragAnimFactorNumer = 1.0;
static const double kDragAnimFactorDenom = 1.8;
static const double kDragScaleFactor = 1.2;
static const int kMaxDragSide = 64; // clamp the drag image to 128x128

static const char *kOverlayState = "overlayViewState";

#define TEST_DISABLE_LAUNCHER3	1

#define SEARCHPILL_BACKGROUND_FILEPATH QString("search-field-bg-launcher.png")
#define SEARCHPILL_ICON_FILEPATH QString("search-button-launcher.png")

// Z_ORDER Values for the different items in the OverlayWindowManager
#define Z_LAUNCHER_WIN    0
#define Z_SEARCH_WIN	  10
#define Z_DOCK_WIN        20
#define Z_FEEDBACK_WIN    40

class LauncherFeedbackWindow : public QGraphicsPixmapItem, public QObject
{
	Q_PROPERTY(qreal pos READ pos WRITE setPos)
public:

	LauncherFeedbackWindow();
	virtual ~LauncherFeedbackWindow();

	void show();
	void setCenter(int cx, int cy);
	void hide();

	bool timeout();
private:
	Timer<LauncherFeedbackWindow> m_timer;
};

class KeyboardTransition : public QKeyEventTransition
{
public:
	enum KeyTrigger {
		KeyPress,
		KeyRelease,
	};

	Q_DECLARE_FLAGS(KeyTriggers, KeyTrigger)

	KeyboardTransition(QObject *object, Qt::Key key, KeyTriggers triggers, QState *sourceState = NULL)
		: QKeyEventTransition(sourceState), m_object(object), m_triggers(triggers)
	{
		m_keys << key;
	}

	KeyboardTransition(QObject *object, QSet<Qt::Key> key, KeyTriggers triggers, QState *sourceState = NULL)
		: QKeyEventTransition(sourceState), m_object(object), m_keys(key), m_triggers(triggers)
	{
	}

	bool eventTest(QEvent *event)
	{
		QKeyEvent *keyEvent;

		if (event->type() != QEvent::StateMachineWrapped)
			return false;

		QStateMachine::WrappedEvent *we = static_cast<QStateMachine::WrappedEvent*>(event);
		if (we->object() != m_object)
			return false;

		event = we->event();
		if (event->type() == QEvent::KeyPress && !m_triggers.testFlag(KeyPress))
			return false;

		if (event->type() == QEvent::KeyRelease && !m_triggers.testFlag(KeyRelease))
			return false;

		keyEvent = static_cast<QKeyEvent *>(event);
		return m_keys.contains(static_cast<Qt::Key>(keyEvent->key()));
	}
private:
	QObject *m_object;
	QSet<Qt::Key> m_keys;
	KeyTriggers m_triggers;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KeyboardTransition::KeyTriggers)


OverlayWindowManager::OverlayWindowManager(int maxWidth, int maxHeight)
	: WindowManagerBase(maxWidth, maxHeight)
	, m_lastPenX(0)
	, m_lastPenY(0)
	, m_dockWasShownBeforeDrag(false)
	, m_dockWin(0)
	, m_dockShown(false)
	, m_dockHasMetFinger(false)
	, m_universalSearchWin(0)
	, m_universalSearchShown(false)
	, m_penDownState(PenDownInvalid)
	, m_searchPill(0)
	, m_dragWin(0)
	, m_dockStates(NULL)
	, m_launcherStates(NULL)
	, m_dockState(StateDockNormal)
	, m_launcherState(StateNoLauncher)
	, m_qp_launcherPosAnimation(NULL)
	, m_dockOpacityAnimation(NULL)
    , m_dockPosAnimation(NULL)
	, m_animateLauncherOpacity(NULL)
    , m_dockBgOpacityAnimation(NULL)
	, m_qp_animateUniversalSearchOpacity(NULL)
	, m_qp_animateSearchPillOpacity(NULL)
	, m_inDrag(false)
	, m_dockOffsetFromBottom(0)
	, m_oldDockOffsetFromBottom(0)
	, m_ignoreKeyDueToSpeedDialEvent(false)
	, m_keyDown(0)
	, m_pendingLauncherRecreate(false)
{
	setObjectName("OverlayWindowManager");

	connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceAboutToChange(const QRect&, bool, bool)),
			this, SLOT(slotPositiveSpaceAboutToChange(const QRect&, bool, bool)));
	connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceChangeFinished(const QRect&)),
			this, SLOT(slotPositiveSpaceChangeFinished(const QRect&)));
	connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceChanged(const QRect&)),
			this, SLOT(slotPositiveSpaceChanged(const QRect&)));
	connect(SystemUiController::instance(), SIGNAL(signalEmergencyMode(bool)),
			SLOT(slotEmergencyModeChanged(bool)));

	connect(SystemUiController::instance(), SIGNAL(signalShowDock()), this, SLOT(slotShowDock()));
	connect(SystemUiController::instance(), SIGNAL(signalHideDock()), this, SLOT(slotHideDock()));

	connect(SystemUiController::instance(), SIGNAL(signalShowUniversalSearch()), this, SLOT(slotShowUniversalSearch()));
	connect(SystemUiController::instance(), SIGNAL(signalHideUniversalSearch(bool, bool)), this, SLOT(slotHideUniversalSearch(bool, bool)));

	connect(SystemUiController::instance(), SIGNAL(signalFadeDock(bool)), this, SLOT(slotFadeDock(bool)));

	connect(SystemUiController::instance(), SIGNAL(signalCardWindowMaximized()), this, SLOT(slotCardWindowMaximized()));
	connect(SystemUiController::instance(), SIGNAL(signalCardWindowMinimized()), this, SLOT(slotCardWindowMinimized()));

	connect(SystemUiController::instance(), SIGNAL(signalUniversalSearchFocusChange(bool)), this, SLOT(slotUniversalSearchFocusChange(bool)));
	connect(DisplayManager::instance(), SIGNAL(signalDisplayStateChange(int)), SLOT(slotDisplayStateChange(int)));

	////////// LAUNCHER CONTROL SIGNALS
	connect(SystemUiController::instance(),SIGNAL(signalToggleLauncher()),
			this,SLOT(slotSystemAPIToggleLauncher()));

	grabGesture((Qt::GestureType) SysMgrGestureFlick);
}

OverlayWindowManager::~OverlayWindowManager()
{
}

bool OverlayWindowManager::handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate)
{
    propogate = (!universalSearchShown() && !launcherVisible());
    return false;
}

void OverlayWindowManager::dockStateTransition()
{
	g_message("%s: transitioned to state %d",__FUNCTION__,dockState());
}

void OverlayWindowManager::launcherStateTransition()
{
	//printf("\r\n[@#@#@#@] LAUNCHER SWITCHED TO STATE: %d\r\n\r\n", launcherState());
}

QState* OverlayWindowManager::createState(QString name, QState *parent)
{
	QState *result = new QState(parent);
	result->setObjectName(name);
	return result;
}

void OverlayWindowManager::setupLauncherAnimations()
{
	m_animateLauncherOpacity = new QPropertyAnimation(DimensionsUI::primaryInstance(), "opacity", this);
	m_animateLauncherOpacity->setDuration(AS(universalSearchCrossFadeDuration));
	m_animateLauncherOpacity->setEasingCurve(AS_CURVE(universalSearchCrossFadeCurve));
	m_animateLauncherOpacity->setObjectName("launcher fade");

	m_qp_launcherPosAnimation = new QPropertyAnimation(DimensionsUI::primaryInstance(), "pos", this);
	m_qp_launcherPosAnimation->setDuration(AS(launcherDuration));
	m_qp_launcherPosAnimation->setEasingCurve(AS_CURVE(launcherCurve));
	m_qp_launcherPosAnimation->setObjectName("launcher pos");
	connect(m_qp_launcherPosAnimation, SIGNAL(finished()), SLOT(launcherAnimationFinished()));
}

void OverlayWindowManager::setupDockAnimations()
{
	m_dockPosAnimation = new QPropertyAnimation(m_dockWin, "pos", this);
	m_dockPosAnimation->setDuration(AS(quickLaunchDuration));
	m_dockPosAnimation->setEasingCurve(AS_CURVE(quickLaunchCurve));
	m_dockPosAnimation->setObjectName("dock pos");
	connect(m_dockPosAnimation, SIGNAL(finished()), SLOT(dockAnimationFinished()));

	m_dockOpacityAnimation = new QPropertyAnimation(m_dockWin, "opacity", this);
	m_dockOpacityAnimation->setDuration(AS(quickLaunchFadeDuration));
	m_dockOpacityAnimation->setEasingCurve(AS_CURVE(quickLaunchFadeCurve));
	m_dockOpacityAnimation->setObjectName("dock state machine fade");

    m_dockBgOpacityAnimation = new QPropertyAnimation(m_dockWin, "backgroundOpacity", this);
    m_dockBgOpacityAnimation->setDuration(AS(launcherDuration));
    m_dockBgOpacityAnimation->setEasingCurve(AS_CURVE(launcherCurve));
}

void OverlayWindowManager::setupSearchPillAnimations()
{
    Q_ASSERT(m_searchPill != NULL);
	m_qp_animateSearchPillOpacity = new QPropertyAnimation(m_searchPill, "opacity", this);
	m_qp_animateSearchPillOpacity->setDuration(AS(quickLaunchFadeDuration));
	m_qp_animateSearchPillOpacity->setEasingCurve(AS_CURVE(quickLaunchCurve));
	m_qp_animateSearchPillOpacity->setObjectName("search pill fade");
}

void OverlayWindowManager::setupUniversalSearchAnimations()
{
	m_qp_animateUniversalSearchOpacity = new QPropertyAnimation(m_universalSearchWin, "opacity", this);
	m_qp_animateUniversalSearchOpacity->setDuration(AS(universalSearchCrossFadeDuration));
	m_qp_animateUniversalSearchOpacity->setEasingCurve(AS_CURVE(universalSearchCrossFadeCurve));
	m_qp_animateUniversalSearchOpacity->setObjectName("universal search fade");
	connect(m_qp_animateUniversalSearchOpacity, SIGNAL(finished()), SLOT(universalSearchOpacityAnimationFinished()));
}

void OverlayWindowManager::setupDockStateMachine()
{
	QSignalTransition *transition;

	Q_ASSERT(m_dockStateMachine == NULL);
	Q_ASSERT(m_dockWin != NULL);
    // this assert may serve no use, regardless of build environment
    if (!m_searchPill) qWarning("%s: m_searchPill is nul", __PRETTY_FUNCTION__);
    // Q_ASSERT(m_searchPill != NULL);


	SystemUiController *uiController = SystemUiController::instance();

	m_dockStateMachine = new QStateMachine(this);
	m_dockStateMachine->setObjectName("Dock Window state machine");
	m_dockStates = new DockStateManager;

	// create the main Dock States here
	m_dockStates->noDock           = createState("no dock");
	m_dockStates->dockNormal       = createState("normal dock");
	m_dockStates->dockReorder      = createState("reorder dock");

	// ------------  STATE: StateNoDock --------------
	// StateNoDock state PROPERTIES
	m_dockStates->noDock->assignProperty(this, "dockState", StateNoDock);
	m_dockStates->noDock->assignProperty(this, "dockShown", false);
	connect(m_dockStates->noDock,SIGNAL(entered()), SLOT(slotAnimateHideDock())); // aimate the dock into hiding
	connect(m_dockStates->noDock, SIGNAL(propertiesAssigned()), SLOT(dockStateTransition())); // $$$ Debug - remove

	// StateNoDock state TRANSITIONS

	// noDock ---> NormalDock [Trigger: signalFSMShowDock]
	transition = m_dockStates->noDock->addTransition(this, SIGNAL(signalFSMShowDock()), m_dockStates->dockNormal);

	// noDock ---> NormalDock [Trigger: signalFSMShowLauncher]
	transition = m_dockStates->noDock->addTransition(this, SIGNAL(signalFSMShowLauncher()), m_dockStates->dockNormal);

	// noDock ---> DockReorder  [Trigger: signalFSMDragStarted]
	transition = m_dockStates->noDock->addTransition(this, SIGNAL(signalFSMDragStarted()), m_dockStates->dockReorder);
	
	// ------------  STATE: StateDockNormal --------------
	// StateDockNormal state PROPERTIES
	m_dockStates->dockNormal->assignProperty(this, "dockState", StateDockNormal);
	m_dockStates->dockNormal->assignProperty(this, "dockShown", true);
	m_dockStates->dockNormal->assignProperty(m_dockWin, "visible", true);
	connect(m_dockStates->dockNormal,SIGNAL(entered()), SLOT(slotAnimateShowDock())); // aimate the dock in
	connect(m_dockStates->dockNormal, SIGNAL(propertiesAssigned()), SLOT(dockStateTransition())); // $$$ Debug - remove

	// StateDockNormal state TRANSITIONS

	// NormalDock ---> noDock  [Trigger: signalHideDock]
	transition = m_dockStates->dockNormal->addTransition(this, SIGNAL(signalFSMHideDock()), m_dockStates->noDock);

	// NormalDock ---> noDock  [Trigger: signalCardWindowAdded]
	transition = m_dockStates->dockNormal->addTransition(uiController, SIGNAL(signalCardWindowAdded()), m_dockStates->noDock);

	// NormalDock ---> noDock  [Trigger: signalCardWindowMaximized]
	transition = m_dockStates->dockNormal->addTransition(uiController, SIGNAL(signalCardWindowMaximized()), m_dockStates->noDock);

	// NormalDock ---> noDock  [Trigger: signalFSMShowUniversalSearch]
	transition = m_dockStates->dockNormal->addTransition(this, SIGNAL(signalFSMShowUniversalSearch()), m_dockStates->noDock);

	// ------------  STATE: StateDockReorder --------------
	// StateDockReorder state PROPERTIES
	m_dockStates->dockReorder->assignProperty(this, "dockState", StateDockReorder);
	m_dockStates->dockReorder->assignProperty(this, "dockShown", true);
	connect(m_dockStates->dockReorder,SIGNAL(entered()), SLOT(slotAnimateShowDock())); // aimate the dock in
	connect(m_dockStates->dockReorder, SIGNAL(propertiesAssigned()), SLOT(dockStateTransition())); // $$$ Debug - remove

	// StateDockReorder state TRANSITIONS
	// DockReorder ---> noDock [Trigger: signalFSMDragEnded]
	transition = m_dockStates->dockReorder->addTransition(this, SIGNAL(signalFSMDragEnded()), m_dockStates->noDock);
	
	// DockReorder ---> NormalDock [Trigger: signalFSMShowDock]
	transition = m_dockStates->dockReorder->addTransition(this, SIGNAL(signalFSMShowDock()), m_dockStates->dockNormal);
	
	// DockReorder ---> noDock  [Trigger: signalFSMShowUniversalSearch]
	transition = m_dockStates->dockReorder->addTransition(this, SIGNAL(signalFSMShowUniversalSearch()), m_dockStates->noDock);

	// Add the top level states to the State Machine
	m_dockStateMachine->addState(m_dockStates->noDock);
	m_dockStateMachine->addState(m_dockStates->dockNormal);
	m_dockStateMachine->addState(m_dockStates->dockReorder);

	// start the Dock State Machine
	m_dockStateMachine->setInitialState(m_dockStates->dockNormal);
	m_dockStateMachine->start();
}

void OverlayWindowManager::setupLauncherStateMachine()
{
	QSignalTransition *transition;

	Q_ASSERT(m_launcherStateMachine == NULL);

	SystemUiController *uiController = SystemUiController::instance();

	m_launcherStateMachine = new QStateMachine(this);
	m_launcherStateMachine->setObjectName("Launcher state machine");
	m_launcherStates = new LauncherStateManager;

	// create the main Launcher States here
	m_launcherStates->noLauncher        = createState("no launcher");
	m_launcherStates->launcherRegular   = createState("regular launcher");
	m_launcherStates->launcherReorder   = createState("reorder launcher");

	// ------------  STATE: StateNoLauncher --------------
	// StateNoLauncher state PROPERTIES
	m_launcherStates->noLauncher->assignProperty(this, "launcherState", StateNoLauncher);
	connect(m_launcherStates->noLauncher,SIGNAL(entered()), SIGNAL(signalLauncherClosed()));
	connect(m_launcherStates->noLauncher,SIGNAL(entered()), SLOT(slotStartHideLauncherSequence())); // aimate the launcher into hiding
	connect(m_launcherStates->noLauncher,SIGNAL(entered()), SLOT(slotLauncherClosedState()));
	connect(m_launcherStates->noLauncher, SIGNAL(propertiesAssigned()), SLOT(launcherStateTransition())); // $$$ Debug - remove

	// StateNoLauncher state TRANSITIONS
	// noLauncher ---> RegularLauncher [Trigger: signalFSMShowLauncher]
	transition = m_launcherStates->noLauncher->addTransition(this, SIGNAL(signalFSMShowLauncher()), m_launcherStates->launcherRegular);

	// ------------  STATE: StateLauncherRegular --------------
	// StateLauncherRegular state PROPERTIES
	m_launcherStates->launcherRegular->assignProperty(this, "launcherState", StateLauncherRegular);
	m_launcherStates->launcherRegular->assignProperty(DimensionsUI::primaryInstance(), "visible", true);
	m_launcherStates->launcherRegular->assignProperty(DimensionsUI::primaryInstance(), "opacity", 1.0);
	connect(m_launcherStates->launcherRegular,SIGNAL(entered()), SIGNAL(signalLauncherOpened()));
	connect(m_launcherStates->launcherRegular,SIGNAL(entered()), SLOT(slotStartShowLauncherSequence())); // animate the launcher in
	connect(m_launcherStates->launcherRegular,SIGNAL(entered()), SLOT(slotLauncherOpenState()));
	connect(m_launcherStates->launcherRegular, SIGNAL(propertiesAssigned()), SLOT(launcherStateTransition())); // $$$ Debug - remove

	// StateLauncherRegular state TRANSITIONS

	// RegularLauncher ---> noLauncher  [Trigger: signalCardWindowAdded]
	transition = m_launcherStates->launcherRegular->addTransition(uiController, SIGNAL(signalCardWindowAdded()), m_launcherStates->noLauncher);

	// RegularLauncher ---> noLauncher  [Trigger: signalCardWindowMaximized]
	transition = m_launcherStates->launcherRegular->addTransition(uiController, SIGNAL(signalCardWindowMaximized()), m_launcherStates->noLauncher);

	// RegularLauncher ---> noLauncher  [Trigger: signalMaximizeActiveCardWindow]
	transition = m_launcherStates->launcherRegular->addTransition(uiController, SIGNAL(signalMaximizeActiveCardWindow()), m_launcherStates->noLauncher);

	// RegularLauncher ---> noLauncher  [Trigger: signalEmergencyMode]
	transition = m_launcherStates->launcherRegular->addTransition(uiController, SIGNAL(signalEmergencyMode(bool)), m_launcherStates->noLauncher);

	// RegularLauncher ---> noLauncher  [Trigger: signalFSMHideLauncher]
	transition = m_launcherStates->launcherRegular->addTransition(this, SIGNAL(signalFSMHideLauncher()), m_launcherStates->noLauncher);

	// RegularLauncher ---> LauncherReorder  [Trigger: signalFSMDragStarted]
	transition = m_launcherStates->launcherRegular->addTransition(this, SIGNAL(signalFSMDragStarted()), m_launcherStates->launcherReorder);


	// ------------  STATE: StateLauncherReorder -------------------------------------------------
	// StateLauncherReorder state PROPERTIES
	m_launcherStates->launcherReorder->assignProperty(this, "launcherState", StateLauncherReorder);
	m_launcherStates->launcherReorder->assignProperty(DimensionsUI::primaryInstance(), "visible", true);
	m_launcherStates->launcherReorder->assignProperty(DimensionsUI::primaryInstance(), "opacity", 1.0);
	connect(m_launcherStates->launcherReorder, SIGNAL(propertiesAssigned()), SLOT(launcherStateTransition())); // $$$ Debug - remove
	// -------------------------------------------------------------------------------------------

	// StateLauncherReorder state TRANSITIONS
	// LauncherReorder ---> RegularLauncher  [Trigger: signalFSMDragEnded]
	transition = m_launcherStates->launcherReorder->addTransition(this, SIGNAL(signalFSMDragEnded()), m_launcherStates->launcherRegular);

	// StateLauncherReorder state TRANSITIONS
	// LauncherReorder ---> noLauncher  [Trigger: signalFSMShowUniversalSearch]
	transition = m_launcherStates->launcherReorder->addTransition(this, SIGNAL(signalFSMShowUniversalSearch()), m_launcherStates->noLauncher);

	// Add the top level states to the State Machine
	m_launcherStateMachine->addState(m_launcherStates->noLauncher);
	m_launcherStateMachine->addState(m_launcherStates->launcherRegular);
	m_launcherStateMachine->addState(m_launcherStates->launcherReorder);

	// start the Launcher State Machine
	m_launcherStateMachine->setInitialState(m_launcherStates->noLauncher);
	m_launcherStateMachine->start();

}

void OverlayWindowManager::setupSearchPillStateMachine()
{
	Q_ASSERT(m_searchPillStateMachine == NULL);

	QSignalTransition *transition = 0;
	SystemUiController *uiController = SystemUiController::instance();

	m_searchPillStateMachine = new QStateMachine(this);
	m_searchPillStateMachine->setObjectName("Search pill state machine");
	m_searchPillStates = new SearchPillStateManager;

	// create the main search pill states here
	m_searchPillStates->pillHidden        = createState("pill hidden");
	m_searchPillStates->pillVisible   = createState("pill visible");

	// Add the top level states to the State Machine
	m_searchPillStateMachine->addState(m_searchPillStates->pillHidden);
	m_searchPillStateMachine->addState(m_searchPillStates->pillVisible);

	//Define the states ++++++++++++++++++++++++

	// ------------  STATE: pillHidden (StateSearchPillHidden)--------------
	// 			StateSearchPillHidden state PROPERTIES
	m_searchPillStates->pillHidden->assignProperty(this, "searchPillState", StateSearchPillHidden);
	connect(m_searchPillStates->pillHidden,SIGNAL(entered()), SLOT(slotAnimateFadeOutSearchPill()));

	// 	+++		StateSearchPillHidden state TRANSITIONS

	// StateSearchPillHidden ---> StateSearchPillVisible [Trigger: signalFSMShowSearchPill]
	transition = m_searchPillStates->pillHidden->addTransition(this, SIGNAL(signalFSMShowSearchPill()), m_searchPillStates->pillVisible);

	// StateSearchPillHidden ---> StateSearchPillVisible [Trigger: signalFSMHideLauncher]
	transition = m_searchPillStates->pillHidden->addTransition(this, SIGNAL(signalFSMHideLauncher()), m_searchPillStates->pillVisible);

	// StateSearchPillHidden ---> StateSearchPillVisible  [Trigger: signalCardWindowMaximized]
	transition = m_searchPillStates->pillHidden->addTransition(uiController, SIGNAL(signalCardWindowMinimized()), m_searchPillStates->pillVisible);

	// ------------ STATE: pillVisible (StateSearchPillVisible)
	// 			StateSearchPillVisible state PROPERTIES
	m_searchPillStates->pillVisible->assignProperty(this, "searchPillState", StateSearchPillVisible);
	connect(m_searchPillStates->pillVisible,SIGNAL(entered()), SLOT(slotAnimateFadeInSearchPill()));

	// 	+++		StateSearchPillVisible state TRANSITIONS

	// StateSearchPillVisible ---> StateSearchPillHidden [Trigger: signalFSMHideSearchPill]
	transition = m_searchPillStates->pillVisible->addTransition(this, SIGNAL(signalFSMHideSearchPill()), m_searchPillStates->pillHidden);

	// StateSearchPillVisible ---> StateSearchPillHidden [Trigger: signalFSMShowLauncher]
	transition = m_searchPillStates->pillVisible->addTransition(this, SIGNAL(signalFSMShowLauncher()), m_searchPillStates->pillHidden);

	// StateSearchPillVisible ---> StateSearchPillHidden  [Trigger: signalFSMShowUniversalSearch]
	transition = m_searchPillStates->pillVisible->addTransition(this, SIGNAL(signalFSMShowUniversalSearch()), m_searchPillStates->pillHidden);

	// StateSearchPillVisible ---> StateSearchPillHidden  [Trigger: signalCardWindowAdded]
	transition = m_searchPillStates->pillVisible->addTransition(uiController, SIGNAL(signalCardWindowAdded()), m_searchPillStates->pillHidden);

	// StateSearchPillVisible ---> StateSearchPillHidden  [Trigger: signalCardWindowMaximized]
	transition = m_searchPillStates->pillVisible->addTransition(uiController, SIGNAL(signalCardWindowMaximized()), m_searchPillStates->pillHidden);

	// StateSearchPillVisible ---> StateSearchPillHidden  [Trigger: signalMaximizeActiveCardWindow]
	transition = m_searchPillStates->pillVisible->addTransition(uiController, SIGNAL(signalMaximizeActiveCardWindow()), m_searchPillStates->pillHidden);

	// StateSearchPillVisible ---> StateSearchPillHidden  [Trigger: signalEmergencyMode]
	transition = m_searchPillStates->pillVisible->addTransition(uiController, SIGNAL(signalEmergencyMode(bool)), m_searchPillStates->pillHidden);

	// start the Search Pill State Machine
	m_searchPillStateMachine->setInitialState(m_searchPillStates->pillVisible);
	m_searchPillStateMachine->start();

}

void OverlayWindowManager::setupUniversalSearchStateMachine()
{
	Q_ASSERT(m_universalSearchStateMachine == NULL);

	QSignalTransition *transition = 0;
	SystemUiController *uiController = SystemUiController::instance();

	m_universalSearchStateMachine = new QStateMachine(this);
	m_universalSearchStateMachine->setObjectName("universal search state machine");
	m_uSearchStates = new UniversalSearchStateManager;

	// create the main search states here
	m_uSearchStates->uSearchHidden        = createState("uSearch hidden");
	m_uSearchStates->uSearchVisible   = createState("uSearch visible");

	// Add the top level states to the State Machine
	m_universalSearchStateMachine->addState(m_uSearchStates->uSearchHidden);
	m_universalSearchStateMachine->addState(m_uSearchStates->uSearchVisible);

	//Define the states ++++++++++++++++++++++++

	// ------------  STATE: uSearchHidden (StateUSearchHidden)--------------
	// 			StateUSearchHidden state PROPERTIES
	m_uSearchStates->uSearchHidden->assignProperty(this, "universalSearchState", StateUSearchHidden);
	m_uSearchStates->uSearchHidden->assignProperty(this,"universalSearchShown",false);
	connect(m_uSearchStates->uSearchHidden,SIGNAL(entered()), SLOT(slotAnimateHideUniversalSearch()));
	connect(m_uSearchStates->uSearchHidden,SIGNAL(entered()), SLOT(slotFSMUniversalSearchHidden()));
	connect(m_uSearchStates->uSearchHidden,SIGNAL(entered()), SIGNAL(signalUniversalSearchClosed()));

	// 	+++		StateUSearchHidden state TRANSITIONS
	// StateUSearchHidden ---> StateUSearchVisible [Trigger: signalFSMShowUniversalSearch]
	transition = m_uSearchStates->uSearchHidden->addTransition(this, SIGNAL(signalFSMShowUniversalSearch()), m_uSearchStates->uSearchVisible);


	// ------------ STATE: uSearchVisible (StateUSearchVisible)
	// 			StateUSearchVisible state PROPERTIES
	m_uSearchStates->uSearchVisible->assignProperty(this, "universalSearchState", StateUSearchVisible);
	m_uSearchStates->uSearchVisible->assignProperty(this,"universalSearchShown",true);
	m_uSearchStates->uSearchVisible->assignProperty(m_universalSearchWin,"visible",true);
	connect(m_uSearchStates->uSearchVisible,SIGNAL(entered()), SLOT(slotAnimateShowUniversalSearch()));
	connect(m_uSearchStates->uSearchVisible,SIGNAL(entered()), SIGNAL(signalUniversalSearchOpened()));

	// 	+++		StateUSearchVisible state TRANSITIONS
	// StateUSearchVisible ---> StateUSearchHidden [Trigger: signalFSMHideUniversalSearch]
	transition = m_uSearchStates->uSearchVisible->addTransition(this, SIGNAL(signalFSMHideUniversalSearch()), m_uSearchStates->uSearchHidden);

	// StateUSearchVisible ---> StateSearchPillHidden  [Trigger: signalCardWindowAdded]
	transition = m_uSearchStates->uSearchVisible->addTransition(uiController, SIGNAL(signalCardWindowAdded()), m_uSearchStates->uSearchHidden);

	// StateUSearchVisible ---> StateSearchPillHidden  [Trigger: signalCardWindowMaximized]
	transition = m_uSearchStates->uSearchVisible->addTransition(uiController, SIGNAL(signalCardWindowMaximized()), m_uSearchStates->uSearchHidden);

	// StateUSearchVisible ---> StateSearchPillHidden  [Trigger: signalMaximizeActiveCardWindow]
	transition = m_uSearchStates->uSearchVisible->addTransition(uiController, SIGNAL(signalMaximizeActiveCardWindow()), m_uSearchStates->uSearchHidden);

	// StateUSearchVisible ---> StateUSearchHidden  [Trigger: signalEmergencyMode]
	transition = m_uSearchStates->uSearchVisible->addTransition(uiController, SIGNAL(signalEmergencyMode(bool)), m_uSearchStates->uSearchHidden);

	// start the Universal Search State Machine
	m_universalSearchStateMachine->setInitialState(m_uSearchStates->uSearchHidden);
	m_universalSearchStateMachine->start();
}

void OverlayWindowManager::slotFSMUniversalSearchVisible()
{

}

void OverlayWindowManager::slotFSMUniversalSearchHidden()
{
	if (SystemUiController::instance()->isCardWindowMaximized())
		return;
	
	//decide if the search pill and dock window (quicklaunch should come back)
	if (m_launcherState == StateNoLauncher)
	{
		//the pill should come back
		Q_EMIT signalFSMShowSearchPill();
	}
	//and the dock always
	Q_EMIT signalFSMShowDock();
}

//static
OverlayWindowManager * OverlayWindowManager::systemActiveInstance()
{
	//get the OverlayWindowManager that is currently being used by the system...i.e. the one that WindowServer(whatever)
	// is using

	return qobject_cast<OverlayWindowManager *>(WindowServer::instance()->getWindowManagerByClassName("OverlayWindowManager"));

}

void OverlayWindowManager::init()
{
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	setFocus();
	g_message("%s: overlaywm getting focus",__FUNCTION__);

	setupSearchPill();
	setupDock();
	setupLauncher();
	setupFeedbackWindow();
}

QRectF OverlayWindowManager::quickLaunchVisibleArea() const
{
	if (m_dockWin)
	{
		return m_dockWin->geometry().translated(m_dockShownPos);
	}
	return QRectF();
}

QPointF OverlayWindowManager::quickLaunchVisiblePosition() const
{
	return m_dockShownPos;
}

bool OverlayWindowManager::okToResize()
{
	if(m_animateLauncherOpacity && m_animateLauncherOpacity->state() != QAbstractAnimation::Stopped) {
		return false;
	}
	if(m_qp_animateUniversalSearchOpacity && m_qp_animateUniversalSearchOpacity->state() != QAbstractAnimation::Stopped){
		return false;
	}
	if(m_qp_animateSearchPillOpacity && m_qp_animateSearchPillOpacity->state() != QAbstractAnimation::Stopped) {
		return false;
	}
	if(m_dockOpacityAnimation && m_dockOpacityAnimation->state() != QAbstractAnimation::Stopped) {
		return false;
	}
	if(m_dockPosAnimation && m_dockPosAnimation->state() != QAbstractAnimation::Stopped){
		return false;
	}
	if(m_qp_launcherPosAnimation && m_qp_launcherPosAnimation->state() != QAbstractAnimation::Stopped){
		return false;
	}

	return true;
}


void OverlayWindowManager::resize(int width, int height)
{
	// accept requests for resizing to the current dimensions, in case we are doing a force resize

	WindowManagerBase::resize(width, height);

	if (m_dockWin)
	{
		m_dockWin->resize(width,height);
		int quicklaunchHeight = m_dockWin->geometry().height();
		m_dockShownPos  = QPoint(0, (boundingRect().toRect().height() - quicklaunchHeight) / 2);
		m_dockHiddenPos = QPoint(m_dockShownPos.x(), m_dockShownPos.y() + quicklaunchHeight);
		if(m_dockShown)
			m_dockWin->setPos(m_dockShownPos);
		else
			m_dockWin->setPos(m_dockHiddenPos);
	}
	//-LAUNCHER3-QLSWAP
	if(m_searchPill) {
		m_searchPill->resize(width, m_searchPill->boundingRect().height());
		m_searchPill->setPos(0, boundingRect().y() +
				Settings::LunaSettings()->positiveSpaceTopPadding +
				(qreal)LayoutSettings::settings()->searchPillTopOffsetFromStatusBar +
				m_searchPill->geometry().height()/2);
	}

	//DO THE LAUNCHER RESIZE **AFTER** THE QUICKLAUNCH RESIZE!!! ("shown position" of the QL MUST be set for launcher to
	// set its page position correctly)
	QPointF pos = QPointF();
	if (DimensionsUI::primaryInstance())
	{
		DimensionsUI::primaryInstance()->resize(width,height);
		//reposition to where it's supposed to be now that the screen changed
		dimensionsLauncherAdjustPositionOnResize();
	}

	if (m_universalSearchWin) {
		const qreal positivePadAmount = Settings::LunaSettings()->positiveSpaceTopPadding;
		QPointF pos = QPoint(0, positivePadAmount);
		m_universalSearchWin->setPos(QPointF(0.0,positivePadAmount));
		((HostWindow*)m_universalSearchWin)->resizeEventSync((qint32)(boundingRect().width()),(qint32)(boundingRect().height()));
	}
}


bool OverlayWindowManager::sceneEvent(QEvent* event)
{
	if (event->type() == QEvent::GestureOverride) {
		QGesture* g = static_cast<QGestureEvent*>(event)->gesture((Qt::GestureType) SysMgrGestureFlick);
		if (g && m_universalSearchShown) {
			event->accept();
			return true;
		}
	} else if (event->type() == QEvent::Gesture) {
		QGesture* g = static_cast<QGestureEvent*>(event)->gesture((Qt::GestureType) SysMgrGestureFlick);
		if (g && g->state() == Qt::GestureFinished && m_universalSearchShown) {
			if (mouseFlickEvent(static_cast<FlickGesture*>(g)))
				return true;
		}
	}
	return QGraphicsObject::sceneEvent(event);
}


void OverlayWindowManager::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	Event ev;
	ev.type = Event::PenDown;
	ev.setMainFinger(true);
	ev.x = event->pos().x();
	ev.y = event->pos().y();
	ev.modifiers = Event::modifiersFromQt(event->modifiers());
	ev.clickCount = 1;
	ev.time = Time::curSysTimeMs();

	if (handlePenDownEvent(&ev))
		event->accept();
	else
		event->ignore();
}

void OverlayWindowManager::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	Event ev;
	ev.type = Event::PenDown;
	ev.setMainFinger(true);
	ev.x = event->pos().x();
	ev.y = event->pos().y();
	ev.modifiers = Event::modifiersFromQt(event->modifiers());
	ev.clickCount = 2;
	ev.time = Time::curSysTimeMs();

	if (handlePenDownEvent(&ev))
		event->accept();
	else
		event->ignore();
}

void OverlayWindowManager::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	Event ev;
	ev.type = Event::PenMove;
	ev.setMainFinger(true);
	ev.x = event->pos().x();
	ev.y = event->pos().y();
	ev.modifiers = Event::modifiersFromQt(event->modifiers());
	ev.time = Time::curSysTimeMs();

	handlePenMoveEvent(&ev);
}

void OverlayWindowManager::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	Event ev;
	ev.setMainFinger(true);
	ev.x = event->pos().x();
	ev.y = event->pos().y();
	ev.modifiers = Event::modifiersFromQt(event->modifiers());
	ev.clickCount = 0;
	ev.time = Time::curSysTimeMs();

	QPointF diff = event->pos() - event->buttonDownPos(Qt::LeftButton);
	ev.setClicked((diff.manhattanLength() <= kSlopFactorForClicks) && !event->canceled());

	if(!event->canceled()){
		ev.type = Event::PenUp;
		handlePenUpEvent(&ev);
	} else {
		ev.type = Event::PenCancel;
		handlePenCancelEvent(&ev);
	}
}

bool OverlayWindowManager::mouseFlickEvent(FlickGesture* flick)
{
	if (m_universalSearchWin && m_universalSearchShown && m_penDownState == PenDownInSearch) {
		Event ev;
		ev.type = Event::PenFlick;
		ev.x = flick->hotSpot().x();
		ev.y = flick->hotSpot().y();
		ev.flickXVel = flick->velocity().x();
		ev.flickYVel = flick->velocity().y();
		ev.setMainFinger(true);
		ev.time = Time::curSysTimeMs();

		mapCoordToWindow(m_universalSearchWin, ev.x, ev.y);
		WebAppMgrProxy::instance()->inputEvent(m_universalSearchWin, &ev);
		return true;
	}
	// eat these events even if the app doesn't handle it
	return m_universalSearchShown;
}

void OverlayWindowManager::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_CoreNavi_QuickLaunch)
		return;
	
	if (m_universalSearchShown) {
		if(!m_ignoreKeyDueToSpeedDialEvent || (m_keyDown != event->key())) {
			WebAppMgrProxy::instance()->inputQKeyEvent(m_universalSearchWin, event);
		}
	}
	else if (!SystemUiController::instance()->isCardWindowMaximized()) {
		// Launch global search -- forward the text to the Universal Search app
		if (event->key() == Qt::Key_Shift || 
			event->key() == Qt::Key_Alt ||
			((event->key() >= Qt::Key_Space && event->key() <= Qt::Key_ydiaeresis)))
		{
			if(!m_ignoreKeyDueToSpeedDialEvent || (m_keyDown != event->key())) {
                slotShowUniversalSearch();

				// allows modifiers + the range of valid visible characters
				WebAppMgrProxy::instance()->inputQKeyEvent(m_universalSearchWin, event);
			}
		}
	}
	m_keyDown = event->key();
}

void OverlayWindowManager::keyReleaseEvent(QKeyEvent* event)
{
	m_keyDown = 0;
	m_ignoreKeyDueToSpeedDialEvent = false;

	switch (event->key()) {
	case (Qt::Key_CoreNavi_QuickLaunch): {
		m_dockHasMetFinger = false;
		m_dockWasShownBeforeDrag = m_dockShown;
		
		grabMouse();
		break;
	}
	default: {
		if (m_universalSearchShown) {
			WebAppMgrProxy::instance()->inputQKeyEvent(m_universalSearchWin, event);
		}
		else if (!SystemUiController::instance()->isCardWindowMaximized()) {
			// support sticky state for first key in global search
			if (event->key() == Event::Key_Shift || event->key() == Event::Key_Alt) {
				WebAppMgrProxy::instance()->inputQKeyEvent(m_universalSearchWin, event);
			}
		}
	}
	}
}

bool OverlayWindowManager::handlePenDownEvent(Event* event)
{
	bool handled = false;
	
	if (m_penDownState != PenDownInvalid || event->rejected())
		return false;

	if (launcherVisible())
	{
		//BLOCK from going below
		handled = true;
	}

	if (m_universalSearchWin && m_universalSearchShown) {
		// in the launcher?
		m_penDownState = PenDownInSearch;

		mapCoordToWindow(m_universalSearchWin, event->x, event->y);
		WebAppMgrProxy::instance()->inputEvent(m_universalSearchWin, event);

		handled = true;
	}

	m_lastPenX = event->x;
	m_lastPenY = event->y;

	return handled;
}

bool OverlayWindowManager::handlePenMoveEvent(Event* event)
{
	bool handled = handlePenMoveEventStateNormal(event);

	if (launcherVisible())
	{
		//BLOCK from going below
		handled = true;
	}

	m_lastPenX = event->x;
	m_lastPenY = event->y;
	return handled;
}

bool OverlayWindowManager::handlePenMoveEventStateNormal(Event* event)
{
	bool handled = false;

	if (PenDownInSearch == m_penDownState) {
		mapCoordToWindow(m_universalSearchWin, event->x, event->y);
		WebAppMgrProxy::instance()->inputEvent(m_universalSearchWin, event);

		handled = true;
	}

	return handled;
}

bool OverlayWindowManager::handlePenUpEvent(Event* event)
{
	bool handled = handlePenUpEventStateNormal(event);

	if (launcherVisible())
	{
		//BLOCK from going below
		handled = true;
	}

	m_penDownState = PenDownInvalid;
	return handled;
}

bool OverlayWindowManager::handlePenUpEventStateNormal(Event* event)
{
	bool handled = false;

	// launch the highlighted app assuming the user hasn't drifted too far away
	if(PenDownInSearch == m_penDownState) {
		if (m_universalSearchShown) {
			mapCoordToWindow(m_universalSearchWin, event->x, event->y);
			WebAppMgrProxy::instance()->inputEvent(m_universalSearchWin, event);

			handled = true;
		}
	} else if(m_inDrag){
		handled = true;
	}
	
	m_penDownState = PenDownInvalid;
	return handled;
}

bool OverlayWindowManager::handlePenFlickEvent(Event* event)
{
	if (m_universalSearchWin && m_universalSearchShown && !event->rejected() && m_penDownState == PenDownInSearch) {
		mapCoordToWindow(m_universalSearchWin, event->x, event->y);
		WebAppMgrProxy::instance()->inputEvent(m_universalSearchWin, event);

		return true;
	}
	return false;
}

bool OverlayWindowManager::handlePenCancelEvent(Event* event)
{
	bool handled = false;

	if (m_penDownState == PenDownInSearch) {
		if (m_universalSearchShown) {
			mapCoordToWindow(m_universalSearchWin, event->x, event->y);
			WebAppMgrProxy::instance()->inputEvent(m_universalSearchWin, event);

			handled = true;
		}
	} else {
		handled = true;
	}
	handled = (m_penDownState != PenDownInvalid);


	m_penDownState = PenDownInvalid;
	return handled;
}

void OverlayWindowManager::setupSearchPill()
{

	PixmapObject * pNormalBgPmo = (PixmapObject *)PixmapObjectLoader::instance()->quickLoadThreeHorizTiled(
			QString(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + SEARCHPILL_BACKGROUND_FILEPATH),
			40,40
	);

	if (pNormalBgPmo)
	{
		PixmapObject * pIconPmo = PixmapObjectLoader::instance()->quickLoad(
				QString(GraphicsSettings::DiUiGraphicsSettings()->graphicsAssetBaseDirectory + SEARCHPILL_ICON_FILEPATH)
		);

		quint32 width = qMax(LayoutSettings::settings()->searchPillWidth,(quint32)(pIconPmo ? pIconPmo->width() : 0));
		quint32 height = pNormalBgPmo->height();
		QRectF pillGeom = DimensionsGlobal::realRectAroundRealPoint(QSize(width - (width %2),height - (height %2)));
		m_searchPill = new SearchPill(pNormalBgPmo, pIconPmo, pillGeom,this);
		m_searchPill->setZValue(Z_SEARCH_WIN);
		m_searchPill->setPos(0, boundingRect().y() +
				Settings::LunaSettings()->positiveSpaceTopPadding +
				(qreal)LayoutSettings::settings()->searchPillTopOffsetFromStatusBar +
				pillGeom.height()/2);

		connect(m_searchPill, SIGNAL(signalTapped()), SLOT(slotShowUniversalSearch()));
		setupSearchPillStateMachine();
		setupSearchPillAnimations();
	}
}

void OverlayWindowManager::setupFeedbackWindow()
{
	m_feedbackWindow = new LauncherFeedbackWindow();
}

void OverlayWindowManager::setupDock()
{
	m_dockWin = new Quicklauncher(boundingRect().size().toSize().width(),boundingRect().size().toSize().height());

	QSize dockWinSize = m_dockWin->geometry().size().toSize();
	m_dockShownPos  = QPoint(0, (boundingRect().toRect().height() - dockWinSize.height()) / 2);
	m_dockHiddenPos = QPoint(m_dockShownPos.x(), m_dockShownPos.y() + dockWinSize.height());

	SystemUiController::instance()->setDockShown(true);

	WindowServer::instance()->addWindow(m_dockWin);

	m_dockWin->setPos(m_dockShownPos);
	m_dockWin->setZValue(Z_DOCK_WIN);

	setupDockAnimations();
	setupDockStateMachine();

	if(m_dockWin->quickLaunchBar())
		connect(m_dockWin->quickLaunchBar(),SIGNAL(signalToggleLauncher()),this,SLOT(slotSystemAPIToggleLauncher()));

	return;
}

void OverlayWindowManager::slotLauncherRequestShowDimensionsLauncher(DimensionsTypes::ShowCause::Enum cause)
{


}

void OverlayWindowManager::slotLauncherRequestHideDimensionsLauncher(DimensionsTypes::HideCause::Enum cause)
{

}

void OverlayWindowManager::slotSystemAPIToggleLauncher()
{
	if (m_launcherState == StateNoLauncher)
	{
		//it's down...signal that I want it up
		Q_EMIT signalFSMShowLauncher();
	}
	else
	{
		//it's up, signal that I want it down
		Q_EMIT signalFSMHideLauncher();
	}
}

void OverlayWindowManager::slotSystemAPICloseLauncher()
{
	Q_EMIT signalFSMHideLauncher();
}

void OverlayWindowManager::slotSystemAPIOpenLauncher()
{
	Q_EMIT signalFSMShowLauncher();
}

void OverlayWindowManager::slotSystemAPIRecreateLauncher()
{
	//if the launcher needs to be taken down first, do so
	m_pendingLauncherRecreate = true;
	Q_EMIT signalFSMHideLauncher();
}

void OverlayWindowManager::slotLauncherOpenState()
{
	SystemUiController::instance()->setLauncherShown(true);
}

void OverlayWindowManager::slotLauncherClosedState()
{
	SystemUiController::instance()->setLauncherShown(false);
}

void OverlayWindowManager::slotLauncherReady()
{
	//place the window at the right position
	// ...initially it is hidden in the hidden position, but setVisible(true)
	//TODO: this is not the best thing; mark it invisible when offscreen
	if (DimensionsUI::primaryInstance())
	{
		//boundary rects that have non-0,0 origins unravel the moral fibers of society! >=(
		QPointF hiddenPos = QPoint(0, SystemUiController::instance()->currentUiHeight());

		DimensionsUI::primaryInstance()->setPos(hiddenPos);
		DimensionsUI::primaryInstance()->setVisible(true);
	}
}

void OverlayWindowManager::slotLauncherNotReady()
{
	//TODO: IMPLEMENT
}

void OverlayWindowManager::slotQuickLaunchReady()
{
//	//display the window at the right position
//	if (m_qp_diuiQlaunch)
//	{
//		//boundary rects that have non-0,0 origins unravel the moral fibers of society! >=(
//		QPointF shownPos = QPointF(0,
//				(qreal)(SystemUiController::instance()->positiveSpaceBounds().height()/2)
//				+(qreal)(SystemUiController::instance()->positiveSpaceBounds().top())
//				-m_qp_diuiQlaunch->geometry().bottom());
//		connect(m_qp_diuiQlaunch->quickLaunchBarObject(),SIGNAL(signalFlickAction(QPointF)),
//				this,SLOT(slotQuickLaunchFlickAction(QPointF)));
//
//		m_qp_diuiQlaunch->setPos(shownPos);
//		m_qp_diuiQlaunch->setVisible(true);
//	}
}

void OverlayWindowManager::slotQuickLaunchNotReady()
{
	//TODO: IMPLEMENT
}

void OverlayWindowManager::slotPositiveSpaceAboutToChange(const QRect& r, bool fullScreen, bool screenResizing)
{
	m_dockOffsetFromBottom = boundingRect().height() - (r.y() + r.height());

	// adjust the dock bottom offset
	if (m_dockWin) {
		if (m_dockOffsetFromBottom <= Settings::LunaSettings()->positiveSpaceBottomPadding) {
			// track its new position but only update it when the dock is visible and not being dragged around
			QPoint newShownPos = m_dockShownPos;
			QPoint oldShownPos = m_dockShownPos;
			newShownPos.setY((boundingRect().height() - m_dockWin->normalHeight()) / 2 - m_dockOffsetFromBottom);
			m_dockShownPos = newShownPos;			
			
			if(m_dockState == StateDockNormal) { // only update in NORMAL state
				QPoint dist = m_dockWin->pos().toPoint() - m_dockShownPos;
				if (screenResizing || (m_dockPosAnimation == NULL)) {
					// No animation, just set the position directly
					m_dockWin->setPos(m_dockShownPos);
				} else if(m_dockPosAnimation->state() != QAbstractAnimation::Running) {
					// animation not running, so animate the dock to its final destination
					m_dockPosAnimation->setStartValue(m_dockWin->pos());
					m_dockPosAnimation->setEndValue(m_dockShownPos);
					m_dockPosAnimation->start();					
				} else if(m_dockPosAnimation != NULL) {
					// animation already in progress, so just update the final target
					if (m_dockPosAnimation->endValue() != newShownPos) {
						m_dockPosAnimation->pause();
						m_dockPosAnimation->setEndValue(m_dockShownPos);
						m_dockPosAnimation->resume();
					}
				}
			}
		}
	}

	if (m_universalSearchWin && !SystemUiController::instance()->isUiRotating()) {
		WebAppMgrProxy::instance()->resizeEvent(m_universalSearchWin, r.width(), r.height());
	}
}

void OverlayWindowManager::slotPositiveSpaceChangeFinished(const QRect& r)
{
}

void OverlayWindowManager::slotPositiveSpaceChanged(const QRect& r) 
{
	if (DimensionsUI::primaryInstance())
	{
		DimensionsUI::primaryInstance()->resize(r.width(),r.height());
	}
}

void OverlayWindowManager::mapCoordToWindow(Window* win, int& x, int& y) const
{
	if (!win)
		return;

	QPointF pt = win->mapFromItem(this, x, y);
	QRectF br = win->boundingRect();
	x = pt.x() - br.x();
	y = pt.y() - br.y();
}

void OverlayWindowManager::slotLauncherOpened()
{
	Q_EMIT signalFSMHideDock();

	if(!hasFocus()) {
		SystemUiController::instance()->focusMaximizedCardWindow(false);
		setFlag(QGraphicsItem::ItemIsFocusable, true);
		setFocus();
		g_message("%s: [NOV-115353] overlaywm getting focus",__FUNCTION__);
		m_ignoreKeyDueToSpeedDialEvent = false;
	}
}

void OverlayWindowManager::slotLauncherClosed()
{
	if(SystemUiController::instance()->isCardWindowMaximized()) {
		SystemUiController::instance()->focusMaximizedCardWindow(true);
		if(hasFocus()) {
			clearFocus();
			setFlag(QGraphicsItem::ItemIsFocusable, false);
			m_ignoreKeyDueToSpeedDialEvent = false;
		}
	} else {
		Q_EMIT signalFSMShowDock();
	}
}

void OverlayWindowManager::slotShowDock()
{

	//TODO: make safer....this is currently only bound to SystemUiController's signalShowDock. So it is safe to just add a gate here to not blindly show the quicklaunch (dock)
	// and instead make it conditional. However, the name and convention of this function/slot suggests that it will unconditionally show the dock, so refactoring this may be desired


	//show the dock BUT NOT if universal search is already up
	if (m_uSearchState == StateUSearchVisible)
	{
		return;
	}
	Q_EMIT signalFSMShowDock();
}

void OverlayWindowManager::slotHideDock()
{
	Q_EMIT signalFSMHideDock();
}

void OverlayWindowManager::slotStartShowLauncherSequence()
{
	qDebug() << __PRETTY_FUNCTION__;
	animateShowLauncher();
}

void OverlayWindowManager::slotStartHideLauncherSequence()
{
	qDebug() << __PRETTY_FUNCTION__;
	//before the animation starts, tell the CardWindowManager to become visible again
	CardWindowManager * pCardWinMgr =
			qobject_cast<CardWindowManager *>(WindowServer::instance()->getWindowManagerByClassName("CardWindowManager"));
	if (pCardWinMgr)
	{
		pCardWinMgr->setVisible(true);
	}
	animateHideLauncher();
}

void OverlayWindowManager::animateShowLauncher()
{
	//in-place mod the launcher's position animation to do a show
	dimensionsLauncherShowUpAnimation(m_qp_launcherPosAnimation);
	if (m_qp_launcherPosAnimation)
	{
		m_qp_launcherPosAnimation->start(QAbstractAnimation::KeepWhenStopped);
	}

    m_dockBgOpacityAnimation->stop();
    m_dockBgOpacityAnimation->setStartValue(m_dockWin->backgroundOpacity());
    m_dockBgOpacityAnimation->setEndValue(1.0);
    m_dockBgOpacityAnimation->start();
}

void OverlayWindowManager::animateHideLauncher()
{
	//in-place mod the launcher's position animation to do a hide
	dimensionsLauncherHideDownAnimation(m_qp_launcherPosAnimation);
	if (m_qp_launcherPosAnimation)
	{
		m_qp_launcherPosAnimation->start(QAbstractAnimation::KeepWhenStopped);
	}

    m_dockBgOpacityAnimation->stop();
    m_dockBgOpacityAnimation->setStartValue(m_dockWin->backgroundOpacity());
    m_dockBgOpacityAnimation->setEndValue(0.0);
    m_dockBgOpacityAnimation->start();
}

void OverlayWindowManager::slotAnimateShowDock()
{
	if(!m_dockWin) {
		return;
	}

	if((scene() != NULL) && (scene()->mouseGrabberItem() == this))
		ungrabMouse();

	if((m_dockWin->pos() == m_dockShownPos) && ((!m_dockPosAnimation) || (m_dockPosAnimation->state() == QAbstractAnimation::Stopped))) {
		// already there
		return;
	}
	
	m_dockWin->setVisible(true);

	if(!m_dockPosAnimation) {
		m_dockWin->setPos(m_dockShownPos);
		return;
	}
	
	m_dockOpacityAnimation->stop();
	if(m_dockWin->opacity() < 1.0) {
		m_dockOpacityAnimation->setStartValue(m_dockWin->opacity());
		m_dockOpacityAnimation->setEndValue(1.0);
		m_dockOpacityAnimation->start();
	}

	m_dockPosAnimation->stop();
	m_dockPosAnimation->setStartValue(m_dockWin->pos());
	m_dockPosAnimation->setEndValue(m_dockShownPos);
	m_dockPosAnimation->start();
}

void OverlayWindowManager::slotAnimateHideDock()
{
	if(!m_dockWin) {
		return;
	}
		
	if((scene() != NULL) && (scene()->mouseGrabberItem() == this))
		ungrabMouse();

	if((m_dockWin->pos() == m_dockHiddenPos) && ((!m_dockPosAnimation) || (m_dockPosAnimation->state() == QAbstractAnimation::Stopped))) {
		// already there
		return;
	}
	
	m_dockWin->setVisible(true);
	
	if(!m_dockPosAnimation) {
		m_dockWin->setPos(m_dockHiddenPos);
		return;
	}
	
	m_dockOpacityAnimation->stop();
	if(m_dockWin->opacity() > 0.0) {
		m_dockOpacityAnimation->setStartValue(m_dockWin->opacity());
		m_dockOpacityAnimation->setEndValue(0.0);
		m_dockOpacityAnimation->start();
	}
	
    m_dockBgOpacityAnimation->stop();
    m_dockBgOpacityAnimation->setStartValue(m_dockWin->backgroundOpacity());
    m_dockBgOpacityAnimation->setEndValue(m_launcherState == StateNoLauncher ? 0.0 : 1.0);
    m_dockBgOpacityAnimation->start();

	m_dockPosAnimation->stop();
	m_dockPosAnimation->setDuration(AS(quickLaunchDuration));
	m_dockPosAnimation->setEasingCurve(AS_CURVE(quickLaunchCurve));
	m_dockPosAnimation->setStartValue(m_dockWin->pos());
	m_dockPosAnimation->setEndValue(m_dockHiddenPos);
	m_dockPosAnimation->start();
}

void OverlayWindowManager::slotAnimateFadeOutSearchPill()
{
	fadeOutSearchPillAnimation(m_qp_animateSearchPillOpacity);
	if (m_qp_animateSearchPillOpacity)
	{
		m_qp_animateSearchPillOpacity->start(QAbstractAnimation::KeepWhenStopped);
	}
}

void OverlayWindowManager::slotAnimateFadeInSearchPill()
{
	fadeInSearchPillAnimation(m_qp_animateSearchPillOpacity);
	if (m_qp_animateSearchPillOpacity)
	{
		m_qp_animateSearchPillOpacity->start(QAbstractAnimation::KeepWhenStopped);
	}
}

void OverlayWindowManager::slotAnimateHideUniversalSearch()
{
	fadeOutUniversalSearchAnimation(m_qp_animateUniversalSearchOpacity);
	if (m_qp_animateUniversalSearchOpacity)
	{
		m_qp_animateUniversalSearchOpacity->start(QAbstractAnimation::KeepWhenStopped);
	}
    if (m_feedbackWindow)
    {
        m_feedbackWindow->hide();
    }
}

void OverlayWindowManager::slotAnimateShowUniversalSearch()
{
	fadeInUniversalSearchAnimation(m_qp_animateUniversalSearchOpacity);
	if (m_qp_animateUniversalSearchOpacity)
	{
		m_qp_animateUniversalSearchOpacity->start(QAbstractAnimation::KeepWhenStopped);
	}
}

void OverlayWindowManager::slotUniversalSearchFocusChange (bool enabled)
{
	QSize univSearchSize = ( DimensionsUI::primaryInstance() ? DimensionsUI::primaryInstance()->geometry().toRect().size() : m_geom.toRect().size());
	WebAppMgrProxy::instance()->resizeEvent(m_universalSearchWin,univSearchSize.width(),univSearchSize.height());
	WebAppMgrProxy::instance()->focusEvent(m_universalSearchWin, enabled);
    if (enabled)
    	IMEController::instance()->setClient(static_cast<HostWindow*>(m_universalSearchWin));
    else
        IMEController::instance()->removeClient(static_cast<HostWindow*>(m_universalSearchWin));
}

void OverlayWindowManager::dockAnimationFinished()
{
	m_dockWin->removeHighlight();
	
	if(!m_dockShown)
	{
		//tell the launcher that the dock is fully down
		if (DimensionsUI::primaryInstance())
		{
			DimensionsUI::primaryInstance()->slotQuicklaunchFullyClosed();
		}
		m_dockWin->setVisible(false);
	}
	else
	{
		//tell the launcher that the dock is fully up
		if (DimensionsUI::primaryInstance())
		{
			DimensionsUI::primaryInstance()->slotQuicklaunchFullyOpen();
		}
	}
}

void OverlayWindowManager::universalSearchOpacityAnimationFinished()
{
	if(m_universalSearchWin->opacity() == 0.0)
		m_universalSearchWin->setVisible(false);
	else
		m_universalSearchWin->setVisible(true);
}

bool OverlayWindowManager::dockInAnimation() const
{
	return m_dockPosAnimation != NULL && m_dockPosAnimation->state() == QAbstractAnimation::Running;
}

bool OverlayWindowManager::dockShown() const
{
	return m_dockShown;
}

void OverlayWindowManager::setDockShown(bool shown)
{
	m_dockShown = shown;
	SystemUiController::instance()->setDockShown(shown);
}

bool OverlayWindowManager::launcherVisible() const
{
	return (m_launcherState != OverlayWindowManager::StateNoLauncher);
}

void OverlayWindowManager::launcherAnimationFinished()
{
	if (m_dockWin) {
        m_dockWin->removeHighlight();
	}
	if (DimensionsUI::primaryInstance())
	{
		CardWindowManager * pCardWinMgr =
							qobject_cast<CardWindowManager *>(WindowServer::instance()->getWindowManagerByClassName("CardWindowManager"));
		if (launcherVisible())
		{
			DimensionsUI::primaryInstance()->slotLauncherFullyOpen();
			//and tell the cardwindomanager to go invisible (for PERF reasons)
			if (pCardWinMgr)
			{
				pCardWinMgr->setVisible(false);
			}
		}
		else
		{
			DimensionsUI::primaryInstance()->slotLauncherFullyClosed(m_pendingLauncherRecreate);
			m_pendingLauncherRecreate = false;
		}
	}
}

bool OverlayWindowManager::universalSearchShown() const
{
	return m_universalSearchShown;
}

void OverlayWindowManager::setUniversalSearchShown(bool shown)
{
    if (shown == m_universalSearchShown)
        return;

    // fire a pen cancel to possibly clean up any state and 
    // guarantee that universal search gets a complete event sequence.
    Event ev;
    ev.type = Event::PenCancel;
    ev.setMainFinger(true);
    ev.x = m_lastPenX;
    ev.y = m_lastPenY;
    ev.clickCount = 0;
    ev.time = Time::curSysTimeMs();
    ev.setClicked(false);
    
    handlePenCancelEvent(&ev);

    // focus was not being set when showing universal search, it was just assumed.
    // this brings back support for universal search being shown on top of a maximized app
    if (shown) {
        if (SystemUiController::instance()->isCardWindowMaximized())
            SystemUiController::instance()->focusMaximizedCardWindow(false);
        if (!hasFocus()) {
            setFlag(QGraphicsItem::ItemIsFocusable, true);
            setFocus();
        }
    }
    else {
        if (SystemUiController::instance()->isCardWindowMaximized())
            SystemUiController::instance()->focusMaximizedCardWindow(true);
    }

	m_universalSearchShown = shown;
	SystemUiController::instance()->setUniversalSearchShown(m_universalSearchShown);
	QSize univSearchSize = ( DimensionsUI::primaryInstance() ? DimensionsUI::primaryInstance()->geometry().toRect().size() : m_geom.toRect().size());
	if (m_universalSearchWin) {
		WebAppMgrProxy::instance()->resizeEvent(m_universalSearchWin,univSearchSize.width(),univSearchSize.height());
		
		WebAppMgrProxy::instance()->focusEvent(m_universalSearchWin, shown);
        if (shown) {
		// explicitly set inputFocus() of justType to avoid race condition
                static_cast<HostWindow*>(m_universalSearchWin)->setInputFocus(true);
                IMEController::instance()->setClient(static_cast<HostWindow*>(m_universalSearchWin));
        }
        else
            IMEController::instance()->removeClient(static_cast<HostWindow*>(m_universalSearchWin));
	}
}

bool OverlayWindowManager::setupLauncher()
{
	//side-effect of this "new" is that the primaryInstance() os DimensionsUI will have the new object
	DimensionsUI * pLauncher = new DimensionsUI(0,0);		//0,0 is the init-only flavor; it will be hidden

	/// main launcher UI Window -> OWM
	connect(pLauncher,SIGNAL(signalReady()),
				this,SLOT(slotLauncherReady()));
	connect(pLauncher,SIGNAL(signalNotReady()),
				this,SLOT(slotLauncherNotReady()));

	connect(pLauncher,SIGNAL(signalShowMe(DimensionsTypes::ShowCause::Enum)),
			this, SLOT(slotLauncherRequestShowDimensionsLauncher(DimensionsTypes::ShowCause::Enum)));
	connect(pLauncher,SIGNAL(signalHideMe(DimensionsTypes::HideCause::Enum)),
			this, SLOT(slotLauncherRequestHideDimensionsLauncher(DimensionsTypes::HideCause::Enum)));

	/// OWM -> main launcher UI Window
	connect(this,SIGNAL(signalLauncherClosed()),
			pLauncher,SIGNAL(signalRelayOWMHidingLauncher()));
	connect(this,SIGNAL(signalLauncherOpened()),
			pLauncher,SIGNAL(signalRelayOWMShowingLauncher()));

	connect(this,SIGNAL(signalUniversalSearchClosed()),
			pLauncher,SIGNAL(signalRelayOWMHidingUniversalSearch()));
	connect(this,SIGNAL(signalUniversalSearchOpened()),
			pLauncher,SIGNAL(signalRelayOWMShowingUniversalSearch()));

	setupLauncherStateMachine();
	setupLauncherAnimations();
	addWindow(pLauncher);
	pLauncher->setVisible(false);

	return true;
}

void OverlayWindowManager::slotShowUniversalSearch(bool hideLauncher)
{
	Q_EMIT signalFSMShowUniversalSearch();
}

void OverlayWindowManager::slotHideUniversalSearch(bool displayLauncher, bool speedDial)
{
	if (speedDial)
	{
		m_ignoreKeyDueToSpeedDialEvent = true;
	}

	Q_EMIT signalFSMHideUniversalSearch();
	
}

void OverlayWindowManager::slotFadeDock(bool fadeOut)
{
	if (!m_dockWin)
		return;

	m_dockOpacityAnimation->stop();
	m_dockOpacityAnimation->setStartValue(m_dockWin->opacity());
	m_dockOpacityAnimation->setEndValue(fadeOut ? 0.0 : 1.0);
	m_dockOpacityAnimation->start();

    if (!m_searchPill)
        return;

    m_qp_animateSearchPillOpacity->stop();
    m_qp_animateSearchPillOpacity->setStartValue(m_searchPill->opacity());
    m_qp_animateSearchPillOpacity->setEndValue(fadeOut ? 0.0 : 1.0);
    m_qp_animateSearchPillOpacity->start();
}

void OverlayWindowManager::slotDisplayStateChange(int newState)
{
	if (newState == DISPLAY_SIGNAL_ON)
	{
		//display turned on...is there an app maximized or is the lock screen on, or in dock mode ?
		//if not, then take focus.
		if (!SystemUiController::instance()->isCardWindowMaximized() &&
			(!SystemUiController::instance()->isScreenLocked())      &&
			(!SystemUiController::instance()->isInDockMode())         )
		{
			setFlag(QGraphicsItem::ItemIsFocusable, true);
			setFocus();
			m_ignoreKeyDueToSpeedDialEvent = false;
			g_message("%s: [NOV-115353] overlaywm getting focus",__FUNCTION__);
		}
	}
}

void OverlayWindowManager::slotCardWindowMaximized()
{
	if(hasFocus()) {
		clearFocus();
		setFlag(QGraphicsItem::ItemIsFocusable, false);
		m_ignoreKeyDueToSpeedDialEvent = false;
	}
	m_dockWin->removeHighlight();
}

void OverlayWindowManager::slotCardWindowMinimized()
{
	if(!hasFocus()) {
		m_ignoreKeyDueToSpeedDialEvent = false;
		setFlag(QGraphicsItem::ItemIsFocusable, true);
		setFocus();
		g_message("%s: overlaywm getting focus",__FUNCTION__);
	}
	else
		g_message("%s: overlaywm NOT getting focus - it thought it had focus already",__FUNCTION__);
}

void OverlayWindowManager::slotEmergencyModeChanged(bool enabled)
{
	if(enabled && hasFocus()) {
		clearFocus();
		setFlag(QGraphicsItem::ItemIsFocusable, false);
	} else if (!SystemUiController::instance()->isCardWindowMaximized() &&
			   !SystemUiController::instance()->isScreenLocked()        &&
		  	   !SystemUiController::instance()->isInDockMode()          &&
		  	   !hasFocus()) {
		setFlag(QGraphicsItem::ItemIsFocusable, true);
		setFocus();
		g_message("%s: overlaywm getting focus",__FUNCTION__);
	}  
	m_ignoreKeyDueToSpeedDialEvent = false;
}

void OverlayWindowManager::addWindow(Window* win)
{
	WindowManagerBase::addWindow(win);

	if( win->type() == Window::Type_Launcher )
	{
		m_universalSearchWin = win;

		const qreal positivePadAmount = Settings::LunaSettings()->positiveSpaceTopPadding;
		((HostWindow*)m_universalSearchWin)->resizeEventSync((qint32)(boundingRect().width()),(qint32)(boundingRect().height()), true);
		m_universalSearchWin->setParentItem(this);
		m_universalSearchWin->setPos(QPointF(0.0,positivePadAmount));
		m_universalSearchWin->setVisible(universalSearchShown());
		setUniversalSearchShown(universalSearchShown());

		win->setZValue(Z_LAUNCHER_WIN);

		if (m_feedbackWindow) {
			m_feedbackWindow->setParentItem(m_universalSearchWin);
			m_feedbackWindow->setZValue(Z_FEEDBACK_WIN);
			m_feedbackOffset =
			    (m_feedbackWindow->boundingRect().size() +
			    m_universalSearchWin->boundingRect().size()).toSize() / 2;
		}

		setupUniversalSearchAnimations();
		setupUniversalSearchStateMachine();

        // IME Support: Set universal search window so SysUi can track it...
		SystemUiController::instance()->setUniversalSearchWindow(m_universalSearchWin);
	}
	else if ( win->type() == Window::Type_QtNativePaintWindow )
	{
		win->setZValue(Z_LAUNCHER_WIN);
		win->setPos(0,0);
	}
}

// ----------------------------------------------------------------------------------------------------

void OverlayWindowManager::applyLaunchFeedback(int centerX, int centerY)
{
	if(m_feedbackWindow) {
		centerX -= m_feedbackOffset.width();
		centerY -= m_feedbackOffset.height();

		m_feedbackWindow->setCenter(centerX, centerY);
		m_feedbackWindow->show();
	}
}

////////////////// -- Launcher3 ADDED /////

QPropertyAnimation * OverlayWindowManager::dimensionsLauncherShowUpAnimation()
{
	QPointF shownPos = QPoint(0, Settings::LunaSettings()->positiveSpaceTopPadding / 2);
	QPropertyAnimation * pAnim = new QPropertyAnimation(DimensionsUI::primaryInstance(),"pos");
	pAnim->setEndValue(shownPos);
	return pAnim;
}

void OverlayWindowManager::dimensionsLauncherShowUpAnimation(QPropertyAnimation * p_anim)
{
	if (p_anim == NULL)
	{
		return;
	}
	p_anim->stop();
	QPointF shownPos = QPoint(0, Settings::LunaSettings()->positiveSpaceTopPadding / 2);
	p_anim->setEndValue(shownPos);
}

QPropertyAnimation * OverlayWindowManager::dimensionsLauncherHideDownAnimation()
{
	QPointF hiddenPos = QPoint(0, SystemUiController::instance()->currentUiHeight());
	QPropertyAnimation * pAnim = new QPropertyAnimation(DimensionsUI::primaryInstance(),"pos");
	pAnim->setEndValue(hiddenPos);
	return pAnim;
}

void OverlayWindowManager::dimensionsLauncherHideDownAnimation(QPropertyAnimation * p_anim)
{
	if (p_anim == NULL)
	{
		return;
	}
	p_anim->stop();
	QPointF hiddenPos = QPoint(0, SystemUiController::instance()->currentUiHeight());
	p_anim->setEndValue(hiddenPos);
}

void OverlayWindowManager::fadeOutSearchPillAnimation(QPropertyAnimation * p_anim)
{
	if (p_anim == NULL)
	{
		return;
	}
	p_anim->stop();
	p_anim->setEndValue(0.0);
}

void OverlayWindowManager::fadeInSearchPillAnimation(QPropertyAnimation * p_anim)
{
	if (p_anim == NULL)
	{
		return;
	}
	p_anim->stop();
	p_anim->setEndValue(1.0);
}

void OverlayWindowManager::fadeOutUniversalSearchAnimation(QPropertyAnimation * p_anim)
{
	if (p_anim == NULL)
	{
		return;
	}
	p_anim->stop();
	p_anim->setEndValue(0.0);
}

void OverlayWindowManager::fadeInUniversalSearchAnimation(QPropertyAnimation * p_anim)
{
	if (p_anim == NULL)
	{
		return;
	}
	p_anim->stop();
	p_anim->setEndValue(1.0);
}

void OverlayWindowManager::dimensionsLauncherAdjustPositionOnResize()
{
	if (m_launcherState == StateNoLauncher)
	{
		QPointF hiddenPos = QPoint(0, SystemUiController::instance()->currentUiHeight());
		DimensionsUI::primaryInstance()->setPos(hiddenPos);
	}
	else
	{
		QPointF shownPos = QPoint(0, Settings::LunaSettings()->positiveSpaceTopPadding / 2);
		DimensionsUI::primaryInstance()->setPos(shownPos);
	}
}

// -------------------------------------------------------------------------

LauncherFeedbackWindow::LauncherFeedbackWindow()
	: m_timer(HostBase::instance()->masterTimer(), this, &LauncherFeedbackWindow::timeout)
{
	setVisible(false);

	std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath + "/launcher-touch-feedback.png";
	setPixmap(QPixmap(qFromUtf8Stl(filePath)));
	setPos(-pixmap().width() / 2, -pixmap().height() / 2);
}

LauncherFeedbackWindow::~LauncherFeedbackWindow()
{
}

void LauncherFeedbackWindow::show()
{
	setVisible(true);
	m_timer.start(5000, true);
}

void LauncherFeedbackWindow::hide()
{
	setVisible(false);
	m_timer.stop();
}

void LauncherFeedbackWindow::setCenter(int cx, int cy)
{
	QGraphicsPixmapItem::setPos(cx,cy);
}

bool LauncherFeedbackWindow::timeout()
{
	hide();
	return false;
}
