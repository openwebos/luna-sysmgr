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

#include "CardWindowManager.h"

#include <QGesture>

#include "ApplicationDescription.h"
#include "AnimationSettings.h"
#include "CardWindow.h"
#include "HostBase.h"
#include "Logging.h"
#include "Settings.h"
#include "SoundPlayerPool.h"
#include "SystemService.h"
#include "SystemUiController.h"
#include "Time.h"
#include "Window.h"
#include "WindowServer.h"
#include "Utils.h"

#include "CardWindowManagerStates.h"
#include "CardGroup.h"
#include "FlickGesture.h"
#include "GhostCard.h"
#include "IMEController.h"

#include <QTapGesture>
#include <QTapAndHoldGesture>
#include <QPropertyAnimation>

static int kGapBetweenGroups = 0;
static qreal kActiveScale = 0.659;
static qreal kNonActiveScale = 0.61;

static const qreal kWindowOriginRatio = 0.40;
static int kWindowOrigin = 0;
static int kWindowOriginMax = 0;

static const qreal kMinimumWindowScale = 0.26;
static int kMinimumHeight = 0;

static const int kFlickToCloseWindowVelocityThreshold = -1100;
static const int kFlickToCloseWindowDistanceThreshold = -50;
static const int kFlickToCloseWindowMinimumVelocity = -500;
static const int kModalWindowAnimationTimeout = 45;

static const int s_marginSlice = 5;

int kAngryCardThreshold = 0;

// -------------------------------------------------------------------------------------------------------------

CardWindowManager::CardWindowManager(int maxWidth, int maxHeight)
	: WindowManagerBase(maxWidth, maxHeight)
	, m_activeGroup(0)
	, m_draggedWin(0)
	, m_penDown(false)
	, m_cardToRestoreToMaximized(0)
	, m_lowResMode(false)
	, m_movement(MovementUnlocked)
	, m_trackWithinGroup(true)
	, m_seenFlickOrTap(false)
    , m_activeGroupPivot(0)
	, m_reorderZone(ReorderZone_Center)
	, m_stateMachine(0)
	, m_minimizeState(0)
	, m_maximizeState(0)
	, m_preparingState(0)
	, m_loadingState(0)
	, m_focusState(0)
	, m_reorderState(0)
	, m_curState(0)
	, m_addingModalWindow(false)
	, m_initModalMaximizing(false)
	, m_parentOfModalCard(0)
	, m_modalDismissInProgress(false)
	, m_modalDimissed(false)
	, m_dismissModalImmediately(-1)
	, m_animateWindowForModalDismisal(true)
	, m_modalWindowState(NoModalWindow)
    , m_playedAngryCardStretchSound(false)
	, m_animationsActive(false)
				  
{
	setObjectName("CardWindowManager");

	SystemUiController* sysui = SystemUiController::instance();

	connect(sysui, SIGNAL(signalPositiveSpaceAboutToChange(const QRect&, bool, bool)),
			SLOT(slotPositiveSpaceAboutToChange(const QRect&, bool, bool)));
	connect(sysui, SIGNAL(signalPositiveSpaceChangeFinished(const QRect&)),
			SLOT(slotPositiveSpaceChangeFinished(const QRect&)));
	connect(sysui, SIGNAL(signalPositiveSpaceChanged(const QRect&)),
			SLOT(slotPositiveSpaceChanged(const QRect&)));
	connect(sysui, SIGNAL(signalLauncherVisible(bool, bool)),
			SLOT(slotLauncherVisible(bool, bool)));
	connect(sysui, SIGNAL(signalLauncherShown(bool)),
			SLOT(slotLauncherShown(bool)));

	connect(sysui, SIGNAL(signalMaximizeActiveCardWindow()),
			SLOT(slotMaximizeActiveCardWindow()));
	connect(sysui, SIGNAL(signalMinimizeActiveCardWindow()),
			SLOT(slotMinimizeActiveCardWindow()));

	connect(sysui, SIGNAL(signalChangeCardWindow(bool)),
			SLOT(slotChangeCardWindow(bool)));

	connect(sysui, SIGNAL(signalFocusMaximizedCardWindow(bool)),
			SLOT(slotFocusMaximizedCardWindow(bool)));

    connect(SystemService::instance(), SIGNAL(signalTouchToShareAppUrlTransfered(const std::string&)),
            SLOT(slotTouchToShareAppUrlTransfered(const std::string&)));

    connect(SystemService::instance(), SIGNAL(signalDismissModalDialog()),
            SLOT(slotDismissActiveModalWindow()));

    connect(SystemService::instance(), SIGNAL(signalStopModalDismissTimer()),
            SLOT(slotDismissModalTimerStopped()));

	connect(&m_anims, SIGNAL(finished()), SLOT(slotAnimationsFinished()));

	grabGesture(Qt::TapGesture);
	grabGesture(Qt::TapAndHoldGesture);
	grabGesture((Qt::GestureType) SysMgrGestureFlick);

}

CardWindowManager::~CardWindowManager()
{
	// doesn't reach here
}

void CardWindowManager::init()
{
	kGapBetweenGroups = Settings::LunaSettings()->gapBetweenCardGroups;

    if (g_file_test(Settings::LunaSettings()->firstCardLaunch.c_str(), G_FILE_TEST_EXISTS)){
        m_dismissedFirstCard=true;
    }
    else{
        m_dismissedFirstCard=false;
    }

	// register CardWindow::Position types for use as Q_PROPERTY's
	qRegisterMetaType<CardWindow::Position>("CardWindow::Position");
	qRegisterAnimationInterpolator<CardWindow::Position>(positionInterpolator);
	// needed in order for CardWindow pointers to be used with queued signal and slot
	// connections, i.e. QStateMachine signal transitions
	qRegisterMetaType<CardWindow*>("CardWindow*");

	m_stateMachine = new QStateMachine(this);

	// setup our states
	m_minimizeState = new MinimizeState(this);
	m_maximizeState = new MaximizeState(this);
	m_preparingState = new PreparingState(this);
	m_loadingState = new LoadingState(this);
	m_focusState = new FocusState(this);
	m_reorderState = new ReorderState(this);

	m_stateMachine->addState(m_minimizeState);
	m_stateMachine->addState(m_maximizeState);
	m_stateMachine->addState(m_preparingState);
	m_stateMachine->addState(m_loadingState);
	m_stateMachine->addState(m_focusState);
	m_stateMachine->addState(m_reorderState);

	// connect allowed state transitions
	m_minimizeState->addTransition(this,
		SIGNAL(signalMaximizeActiveWindow()), m_maximizeState);
	m_minimizeState->addTransition(this,
		SIGNAL(signalPreparingWindow(CardWindow*)), m_preparingState);
	m_minimizeState->addTransition(this,
		SIGNAL(signalFocusWindow(CardWindow*)), m_focusState);
	m_minimizeState->addTransition(this,
		SIGNAL(signalEnterReorder(QPoint, int)), m_reorderState);

	m_maximizeState->addTransition(this,
		SIGNAL(signalMinimizeActiveWindow()), m_minimizeState);
	m_maximizeState->addTransition(this,
		SIGNAL(signalPreparingWindow(CardWindow*)), m_preparingState);
	m_maximizeState->addTransition(new MaximizeToFocusTransition(this, m_focusState));

	m_focusState->addTransition(this,
		SIGNAL(signalMaximizeActiveWindow()), m_maximizeState);
	m_focusState->addTransition(this,
		SIGNAL(signalMinimizeActiveWindow()), m_minimizeState);
	m_focusState->addTransition(this,
		SIGNAL(signalFocusWindow(CardWindow*)), m_focusState);
	m_focusState->addTransition(this,
		SIGNAL(signalPreparingWindow(CardWindow*)), m_preparingState);

	m_preparingState->addTransition(this,
		SIGNAL(signalMinimizeActiveWindow()), m_minimizeState);
	m_preparingState->addTransition(this,
		SIGNAL(signalMaximizeActiveWindow()), m_maximizeState);
	m_preparingState->addTransition(this,
		SIGNAL(signalPreparingWindow(CardWindow*)), m_preparingState);
	m_preparingState->addTransition(this,
		SIGNAL(signalLoadingActiveWindow()), m_loadingState);

	m_loadingState->addTransition(this,
		SIGNAL(signalMinimizeActiveWindow()), m_minimizeState);
	m_loadingState->addTransition(this,
		SIGNAL(signalMaximizeActiveWindow()), m_maximizeState);
	m_loadingState->addTransition(this,
		SIGNAL(signalPreparingWindow(CardWindow*)), m_preparingState);

	m_reorderState->addTransition(this,
		SIGNAL(signalExitReorder(bool)), m_minimizeState);

	// start off minimized
	m_stateMachine->setInitialState(m_minimizeState);

	m_stateMachine->start();

    updateAngryCardThreshold();
}

bool CardWindowManager::handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate)
{
    propogate = false;
    return m_curState->handleKeyNavigation(keyEvent);
}

bool CardWindowManager::okToResize() {
	if(m_anims.state() != QAbstractAnimation::Stopped)
		return false;

	return true;
}

void CardWindowManager::updateAngryCardThreshold()
{
    kAngryCardThreshold = ((boundingRect().height() / 2) * 0.30);
}

void CardWindowManager::resize(int width, int height)
{
	// accept requests for resizing to the current dimensions, in case we are doing a force resize due to
	// previous cancelation of Card Window flip operations.

	WindowManagerBase::resize(width, height);

	m_normalScreenBounds = QRect(0, Settings::LunaSettings()->positiveSpaceTopPadding,
						  	     SystemUiController::instance()->currentUiWidth(),
							     SystemUiController::instance()->currentUiHeight() - Settings::LunaSettings()->positiveSpaceTopPadding);

	kMinimumHeight = (int) (kMinimumWindowScale * m_normalScreenBounds.height());
	kMinimumHeight = (int) ((kMinimumHeight/2) / kWindowOriginRatio);

	SystemUiController::instance()->setMinimumPositiveSpaceHeight(kMinimumHeight);

	m_targetPositiveSpace = SystemUiController::instance()->positiveSpaceBounds();

	kWindowOrigin = boundingRect().y() + ((m_normalScreenBounds.y() + 48) + (int) ((m_normalScreenBounds.height() - 48) * kWindowOriginRatio));

    updateAngryCardThreshold();

	if(m_groups.size() > 0) {
		int index = m_groups.indexOf(m_activeGroup);

		// first resize the active group
		m_groups[index]->resize(width, height, m_normalScreenBounds);
		m_groups[index]->setY(kWindowOrigin);

		// resize the group to the left of the active group
		if(index > 0) {
			m_groups[index-1]->resize(width, height, m_normalScreenBounds);
			m_groups[index-1]->setY(kWindowOrigin);
		}

		// resize the group to the right of the active group
		if(index < m_groups.size()-1) {
			m_groups[index+1]->resize(width, height, m_normalScreenBounds);
			m_groups[index+1]->setY(kWindowOrigin);
		}

		// now resize the other groups, if there are any
		if(index-1 > 0) {
			// left side
			for(int x = index-2; x >= 0; x--) {
				m_groups[x]->resize(width, height, m_normalScreenBounds);
				m_groups[x]->setY(kWindowOrigin);
			}
		}

		if(index+1 < m_groups.size()-1) {
			// right side
			for(int x = index+2; x < m_groups.size(); x++) {
				m_groups[x]->resize(width, height, m_normalScreenBounds);
				m_groups[x]->setY(kWindowOrigin);
			}
		}
	}

	m_curState->relayout(m_boundingRect, false);
}

void CardWindowManager::removeAnimationForWindow(CardWindow* win, bool includeDeletedAnimations)
{
	if ((m_curState != m_maximizeState) && m_penDown && !includeDeletedAnimations)
		win->allowUpdates(false);
	else
		win->allowUpdates(true);

	if (m_cardAnimMap.contains(win)) {
		QPropertyAnimation* a = m_cardAnimMap.value(win);
		m_anims.removeAnimation(a);
		m_cardAnimMap.remove(win);
		delete a;
	}

	if (includeDeletedAnimations) {
		QMap<CardWindow*,QPropertyAnimation*>::iterator it = m_deletedAnimMap.find(win);
		if (it != m_deletedAnimMap.end()) {

			QPropertyAnimation* a = qobject_cast<QPropertyAnimation*>(it.value());
			if (a) {
				m_deletedAnimMap.erase(it);
				delete a;
			}
		}
	}
}

bool CardWindowManager::windowHasAnimation(CardWindow* win) const
{
	return m_cardAnimMap.contains(win) || m_deletedAnimMap.contains(win);
}

void CardWindowManager::setAnimationForWindow(CardWindow* win, QPropertyAnimation* anim)
{
	removeAnimationForWindow(win);

	m_cardAnimMap.insert(win, anim);
	m_anims.addAnimation(anim);
}

void CardWindowManager::setAnimationForGroup(CardGroup* group, QPropertyAnimation* anim)
{
	removeAnimationForGroup(group);

	m_groupAnimMap.insert(group, anim);
	m_anims.addAnimation(anim);
}

void CardWindowManager::removeAnimationForGroup(CardGroup* group)
{
	if (m_groupAnimMap.contains(group)) {
		QPropertyAnimation* a = m_groupAnimMap.value(group);
		m_anims.removeAnimation(a);
		m_groupAnimMap.remove(group);
		delete a;
	}
}

bool CardWindowManager::groupHasAnimation(CardGroup* group) const
{
	return m_groupAnimMap.contains(group);
}

void CardWindowManager::startAnimations()
{
	m_animationsActive = true;
	updateAllowWindowUpdates();

	m_anims.start();
}

void CardWindowManager::clearAnimations()
{
	m_anims.stop();
	m_anims.clear();
	m_cardAnimMap.clear();
	m_groupAnimMap.clear();

	m_animationsActive = false;
	updateAllowWindowUpdates();
}

void CardWindowManager::resetMouseTrackState()
{
	m_draggedWin = 0;
	m_penDown = false;
	updateAllowWindowUpdates();
	
	m_trackWithinGroup = true;
	m_seenFlickOrTap = false;
    m_playedAngryCardStretchSound = false;
	m_movement = MovementUnlocked;
}

int CardWindowManager::proceedToAddModalWindow(CardWindow* win)
{
	Window* maxCardWindow = SystemUiController::instance()->maximizedCardWindow();
	CardWindow* activeWin = activeWindow();

	// Check if we have an active card
	if(!maxCardWindow || !activeWin)
		return SystemUiController::NoMaximizedCard;

	// Check if the active window is the same as the maximized window
	if(activeWin != maxCardWindow)
		return SystemUiController::ParentDifferent;

	// Get the id of the currently active window
	ApplicationDescription* desc = activeWin->appDescription();
	if(desc)
	{
		std::string id = desc->id();
		if(id.length() > 0) {
			// Compare with what the card thinks it's caller is
			if((0 == win->launchingAppId().compare(id) && (0 == win->launchingProcessId().compare(activeWin->processId())))) {
				return SystemUiController::NoErr;
			}
			else {
				return SystemUiController::ParentDifferent;
			}
		}
	}
	else {
		// If it's a PDK app and we have no appDescription, comparing to appId
		if(activeWin->isHost()) {
			if((0 == win->launchingAppId().compare(activeWin->appId()) && (0 == win->launchingProcessId().compare(activeWin->processId())))) {
				return SystemUiController::NoErr;
			}
			else {
				return SystemUiController::ParentDifferent;
			}
		}
	}
	return SystemUiController::LaunchUnknown;
}

void CardWindowManager::prepareAddWindow(Window* win)
{
	CardWindow* card = static_cast<CardWindow*>(win);
	if(!card)
		return;

	if ((card->hostWindowData() != 0) &&
	    !card->isHost() && (card->type() != Window::Type_ModalChildWindowCard) &&
	    (card->getCardFixedOrientation() == Event::Orientation_Invalid)) {
		// safeguard code in case the data card was launched right before we changed orientation, resulting
		// in possibly a landscape card in portrait mode or vice versa
		bool isCardLandscape = card->hostWindowData()->width() >= card->hostWindowData()->height();
		bool isUiLandscape = SystemUiController::instance()->currentUiWidth() >= SystemUiController::instance()->currentUiHeight();

		if(isCardLandscape != isUiLandscape) {
			// we need to resize this card
			card->flipEventSync();
		}
	}


	// Proxy cards don't have to wait
	if (!card->isHost()) {
		// delay adding a new card
		if (card->delayPrepare()) {
			return;
		}
	}

	// If we have a modal card and we cannot add it for whatever reason, just return
	if(Window::Type_ModalChildWindowCard == win->type() && (SystemUiController::NoErr != (m_dismissModalImmediately = proceedToAddModalWindow(card)))) {
		m_modalWindowState = ModalWindowAddInitCheckFail;
		notifySysControllerOfModalStatus(m_dismissModalImmediately, false, ModalLaunch, win);
		return;
	}

	Q_EMIT signalExitReorder();
	card->enableShadow();

	// Do this ONLY if we are not adding a MODAL window
	if(Window::Type_ModalChildWindowCard != card->type()) {

		// If the currently active card is a modal card, make sure we dismiss it as we are going to get a new active card - don't restore the state as the new card will be the active card
		if(activeWindow() && Window::Type_ModalChildWindowCard == activeWindow()->type()) {

			m_modalWindowState = ModalWindowDismissedParentSwitched;
			notifySysControllerOfModalStatus(SystemUiController::ActiveCardsSwitched, true, ModalDismissNoAnimate, activeWindow());

			// Set the fact that for all purposes m_parentOfModalCard is the currently active card
			if(m_parentOfModalCard) {
				// If this card is a modal parent, clear that flag
				if(m_parentOfModalCard->isCardModalParent())
					m_parentOfModalCard->setCardIsModalParent(false);

				// Set the fact that this card no longer has a modal child
				if(NULL != m_parentOfModalCard->getModalChild())
					m_parentOfModalCard->setModalChild(NULL);

				// Set that this card needs to process all input
				m_parentOfModalCard->setModalAcceptInputState(CardWindow::NoModalWindow);
			}
		}

		card->setParentItem(this);
	}
	else {

		m_parentOfModalCard = activeWindow();

		// Set the desired fields for the modal card.
		card->setModalParent(m_parentOfModalCard);

		// Set the desired fields for the parent of the modal card.
		m_parentOfModalCard->setCardIsModalParent(true);
		m_parentOfModalCard->setModalChild(card);
		m_parentOfModalCard->setModalAcceptInputState(CardWindow::ModalLaunchedNotAcceptingInput);

		// Let the modal window compute it's initial position
		card->computeModalWindowPlacementInf(-1);

		// Set the fact that we are adding a modal window
		m_addingModalWindow = true;
        }

	disableCardRestoreToMaximized();
	Q_EMIT signalPreparingWindow(card);
	SystemUiController::instance()->cardWindowAdded();
}

void CardWindowManager::prepareAddWindowSibling(CardWindow* win)
{
	if (m_activeGroup && !win->launchInNewGroup()) {
		CardWindow* activeWin = m_activeGroup->activeCard();
		if(Window::Type_ModalChildWindowCard != win->type()) {
			if ((activeWin->focused() &&
				(win->launchingProcessId() == activeWin->processId() ||
				(win->launchingAppId() == activeWin->appId())))) {
				// add to active group
				m_activeGroup->addToFront(win);
				setActiveGroup(m_activeGroup);
				m_cardToRestoreToMaximized = activeWin;
			}
			else {
				// spawn new group to the right of active group
				CardGroup* newGroup = new CardGroup(kActiveScale, kNonActiveScale);
				newGroup->setPos(QPointF(0, kWindowOrigin));
				newGroup->addToGroup(win);
				m_groups.insert(m_groups.indexOf(m_activeGroup)+1, newGroup);
				setActiveGroup(newGroup);
			}

			queueFocusAction(activeWin, false);

			setActiveCardOffScreen();
			slideAllGroups(false);
			startAnimations();
		}
		else {
			queueFocusAction(activeWin, false);
		}
	}
	else {
		CardGroup* newGroup = new CardGroup(kActiveScale, kNonActiveScale);
		newGroup->setPos(QPointF(0, kWindowOrigin));
		newGroup->addToGroup(win);
		m_groups.append(newGroup);
		setActiveGroup(newGroup);
		setActiveCardOffScreen();
	}

	SystemUiController::instance()->setCardWindowAboutToMaximize();
	SystemUiController::instance()->cardWindowAdded();
}

void CardWindowManager::addWindowTimedOut(Window* win)
{
	CardWindow* card = static_cast<CardWindow*>(win);
	// Host windows shouldn't fire this handler anyways
	if (card->isHost())
		return;

	if(Window::Type_ModalChildWindowCard == win->type() && -1 != m_dismissModalImmediately) {
		m_dismissModalImmediately = -1;
		return;
	}

	Q_EMIT signalExitReorder();

	m_curState->windowTimedOut(card);
	SystemUiController::instance()->cardWindowTimeout();
}

void CardWindowManager::addWindowTimedOutNormal(CardWindow* win)
{
	m_penDown = false;
	updateAllowWindowUpdates();

	Q_ASSERT(m_activeGroup && m_activeGroup->activeCard() == win);

	if(Window::Type_ModalChildWindowCard != win->type()) {
		setActiveCardOffScreen(false);
		slideAllGroups();
	}

	if(Window::Type_ModalChildWindowCard == win->type() && -1 != m_dismissModalImmediately) {
		m_dismissModalImmediately = -1;
		return;
	}

	Q_EMIT signalLoadingActiveWindow();
}

void CardWindowManager::addWindow(Window* win)
{
	CardWindow* card = static_cast<CardWindow*>(win);

	card->setAddedToWindowManager();
	// process addWindow once preparing has finished
	if (!card->isHost() && !card->prepareAddedToWindowManager())
		return;

	Q_EMIT signalExitReorder();

	m_curState->windowAdded(card);
}

void CardWindowManager::removeWindow(Window* win)
{
	if(!win)
		return;

	CardWindow* card = static_cast<CardWindow*>(win);
	if(!card)
		return;

	Q_EMIT signalExitReorder();

	// Either there are no modal window(s) OR we are not deleting the modal parent - default to the plain vanilla delete.
	if((win->type() != Window::Type_ModalChildWindowCard && false == m_addingModalWindow) || (win->type() != Window::Type_ModalChildWindowCard && false == card->isCardModalParent())) {
		removeWindowNoModality(card);
	}
	else
		removeWindowWithModality(card);
}

void CardWindowManager::removeWindowNoModality(CardWindow* win)
{
	if(!win)
		return;

	QPropertyAnimation* anim = NULL;

	if(false == performCommonWindowRemovalTasks(win, true))
		return;

	// slide card off the top of the screen
	CardWindow::Position pos = win->position();
	QRectF r = pos.toTransform().mapRect(win->boundingRect());
	qreal offTop = boundingRect().y() - (win->y() + (r.height()/2));
	anim = new QPropertyAnimation(win, "y");
	anim->setDuration(AS(cardDeleteDuration));
	anim->setEasingCurve(AS_CURVE(cardDeleteCurve));
	anim->setEndValue(offTop);

	QM_CONNECT(anim, SIGNAL(finished()), SLOT(slotDeletedAnimationFinished()));
	m_deletedAnimMap.insert(win, anim);
	anim->start();

	m_curState->windowRemoved(win);
}


void CardWindowManager::removeWindowWithModality(CardWindow* win)
{
	CardWindow* card = NULL, *activeCard = NULL;
	QPropertyAnimation* anim = NULL;
	bool restore = false;

	if(!win)
		return;

	activeCard = activeWindow();
	if(!activeCard)
		return;

	card = win;

	restore = (activeCard == card && Window::Type_ModalChildWindowCard == card->type()) ? true:false;

	// If the modal card was deleted because it's parent was deleted externally, don't run any of these, simply remove the modal and return
	if(Window::Type_ModalChildWindowCard == card->type() && m_modalWindowState == ModalParentDimissedWaitForChildDismissal && NULL == m_parentOfModalCard) {
		handleModalRemovalForDeletedParent(card);
		m_modalWindowState = NoModalWindow;
		return;
	}

	/*
		  This function is called externally by some component when it wants the CardWindow to go away. Also when ::closeWindow's call to win->close will result in
		  a call to this function. For the modal windows, we have 2 cases to consider.

		  1) ::removeWindow is called on a modal window by an external component.
		  2) ::removeWindow is called on a modal window by as a result of a call to ::closeWindow.

		  We also need to consider the case if the modal was even added.
	*/

	// This is not a part of a call to closeWindow and came by externally. This means there is NO need to call close on the window again.
	if(false == m_modalDismissInProgress) {

		SystemUiController::ModalWinDismissErrorReason dismiss = SystemUiController::DismissUnknown;

		// We are removing a modal card externally
		if(Window::Type_ModalChildWindowCard == card->type()) {
			m_modalWindowState = ModalWindowDismissedExternally;
		}
		// check if w is a modal parent
		else if(true == card->isCardModalParent() && (Window::Type_ModalChildWindowCard == activeWindow()->type())) {
			m_modalWindowState = ModalParentDismissed;
			dismiss = SystemUiController::ParentCardDismissed;
		}

		notifySysControllerOfModalStatus(dismiss, restore, ModalDismissNoAnimate);
	}
	else {
		m_modalDismissInProgress = false;
	}

	// Signal that we no longer have an active modal window - Either the modal is getting dismissed/parent is getting dismissed/modal add failed as the parent was different etc
	if(true == restore || m_modalWindowState == ModalParentDismissed || m_modalWindowState == ModalWindowAddInitCheckFail || m_modalWindowState == ModalWindowDismissedParentSwitched)
		SystemUiController::instance()->notifyModalWindowDeactivated();

	// Reset in different ways (if true == restore => modal card was added successfully and is the card being deleted.
	if(true == restore)
		resetModalFlags();
	// If we are deleting the modal card because of a failure during initialization, just reset the flags here.
	else if(m_modalWindowState == ModalWindowAddInitCheckFail)
		resetModalFlags(true);
	else if(m_modalWindowState == ModalParentDismissed) {

		// modal parent is deleted - Do the following to make things smoother.
		// 1 - Make the modal card invisible.
		// 2 - Set that the parent no longer has a modal child. (Don't reset the state)

		activeCard->setVisible(false);

		if(m_parentOfModalCard) {
			m_parentOfModalCard->setCardIsModalParent(false);
			m_parentOfModalCard->setModalChild(NULL);
			m_parentOfModalCard->setModalAcceptInputState(CardWindow::NoModalWindow);
		}
	}
	else if(m_modalWindowState == ModalWindowDismissedParentSwitched) {
		resetModalFlags(true);
	}

	if(false == performCommonWindowRemovalTasks(card, (m_modalWindowState == ModalParentDismissed)?true:false))
		return;

	if(Window::Type_ModalChildWindowCard != card->type()) {
		// slide card off the top of the screen
		CardWindow::Position pos = card->position();
		QRectF r = pos.toTransform().mapRect(card->boundingRect());
		qreal offTop = boundingRect().y() - (card->y() + (r.height()/2));
		anim = new QPropertyAnimation(card, "y");
		anim->setDuration(AS(cardDeleteDuration));
		anim->setEasingCurve(AS_CURVE(cardDeleteCurve));
		anim->setEndValue(offTop);
	}
	else if(true == m_animateWindowForModalDismisal){
		anim = new QPropertyAnimation();
	}

	QM_CONNECT(anim, SIGNAL(finished()), SLOT(slotDeletedAnimationFinished()));
	m_deletedAnimMap.insert(card, anim);

	if(true == m_animateWindowForModalDismisal)
		anim->start();

	m_curState->windowRemoved(card);

	// Finally if we are deleting a modal parent, clean reset the state
	if(m_modalWindowState == ModalParentDismissed) {
		resetModalFlags(true);
		m_modalWindowState = ModalParentDimissedWaitForChildDismissal;
	}
}

void CardWindowManager::handleModalRemovalForDeletedParent(CardWindow* card)
{
	if(NULL == card || Window::Type_ModalChildWindowCard != card->type())
		return;

	// ignore the return value.
	performCommonWindowRemovalTasks(card, false);
}

bool CardWindowManager::performCommonWindowRemovalTasks(CardWindow* card, bool checkCardGroup)
{
	if(!card)
		return false;

	removePendingActionWindow(card);

	// Mark window as removed. Its safe to delete this now
	card->setRemoved();

	// Is it already on the deleted animation list?
	if (m_deletedAnimMap.contains(card)) {

		// if it is animating, let it finish which will delete it
		QPropertyAnimation* a = m_deletedAnimMap.value(card);
		if (a && a->state() != QAbstractAnimation::Running) {
			// nuke the animation
			removeAnimationForWindow(card, true);
			delete card;
		}
		return false;
	}

	if(true == checkCardGroup)
		Q_ASSERT(card->cardGroup() != 0);

	removeAnimationForWindow(card);

	return true;
}

void CardWindowManager::initiateRemovalOfActiveModalWindow()
{
	// Ensure that the last window we added was a modal window
	if(true == m_addingModalWindow)
	{
		CardWindow* activeWin = activeWindow();

		// ERROR. DONT KNOW WHICH CARD WILL BE THE NEW ACTIVE CARD
		if(!activeWin) {
			g_warning("Unable to get active modal window %s", __PRETTY_FUNCTION__);
			return;
		}

		// Techinically this should never happen, but just in case.
		if(!m_parentOfModalCard)
			m_parentOfModalCard = static_cast<CardWindow*>(activeWin->parentItem());

		if(!m_parentOfModalCard) {
			g_warning("Unable to get parent of active modal window %s", __PRETTY_FUNCTION__);
			return;
		}

		// Start an animation for the opacity of the currently active modal window
		QPropertyAnimation* winAnim = new QPropertyAnimation(activeWin, "opacity");
		winAnim->setEndValue(0.0);
		winAnim->setDuration(kModalWindowAnimationTimeout);

		// connect to the slot that gets called when this animation gets done.
		connect(winAnim, SIGNAL(finished()), SLOT(slotOpacityAnimationFinished()));

		// start the animation
		winAnim->start(QAbstractAnimation::DeleteWhenStopped);
	}
}

void CardWindowManager::resetModalFlags(bool forceReset)
{
	m_addingModalWindow = false;
	m_initModalMaximizing = false;
	m_modalDismissInProgress = false;
	m_modalDimissed = false;
	m_dismissModalImmediately = -1;

	if(m_parentOfModalCard) {
		// If this card is a modal parent, clear that flag
		if(m_parentOfModalCard->isCardModalParent())
			m_parentOfModalCard->setCardIsModalParent(false);

		// Set the fact that this card no longer has a modal child
		if(NULL != m_parentOfModalCard->getModalChild())
			m_parentOfModalCard->setModalChild(NULL);

		// Set that this card needs to process all input
		m_parentOfModalCard->setModalAcceptInputState(CardWindow::NoModalWindow);
	}

	if(true == forceReset)
		m_parentOfModalCard = NULL;
}

void CardWindowManager::performPostModalWindowRemovedActions(Window* win, bool restore)
{
	CardWindow* activeWin = (NULL != win)? (static_cast<CardWindow*>(win)) : activeWindow();

	if(!activeWin)
	{
		g_warning("Unable to get active modal window %s", __PRETTY_FUNCTION__);
		// Set the parent to the first card of the active card group.
		if(m_activeGroup)
			m_activeGroup->makeBackCardActive();

		return;
	}

	// call close ONLY if the modal was deleted internally
	if(ModalWindowDismissedExternally != m_modalWindowState || NoModalWindow != m_modalWindowState) {
		closeWindow(activeWin);
	}

	// Make the parent card as the active card. Notify SysUiController of this fact as well. No animations are needed as the parent is already the active card in full screen
	if(true == restore && NULL != m_parentOfModalCard) {
		m_modalDimissed = true;
		// Just set this flag so that the parent doesn't forward events to the modal card anymore
		m_parentOfModalCard->setModalAcceptInputState(CardWindow::NoModalWindow);

		// Set the new maximized/active cards.
		SystemUiController::instance()->setMaximizedCardWindow(m_parentOfModalCard);
		SystemUiController::instance()->setActiveCardWindow(m_parentOfModalCard);

		// PDK apps need both Focus and Maximized events to be sent to them to direct render. The call above will give focus, disable DR here and resetModalFlags will re-enable DR on the parent
		if(m_parentOfModalCard->isHost()) {
			SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::CARD_WINDOW_MANAGER, m_parentOfModalCard, false);
		}

		// If we are restoring the parent state coz of active card being switched, don't start an animation, but do all the actions in sequence
		if(ModalWindowDismissedParentSwitched != m_modalWindowState) {

			// Queue up the fact that we need to give focus back to the parent
			queueFocusAction(m_parentOfModalCard, true);

			// Create an empty animation and add to m_anims.start(). When it completes, it'll call performPendingFocusActions(); to give focus back to the parent.
			QPropertyAnimation* anim = new QPropertyAnimation();
			m_anims.addAnimation(anim);
			m_anims.start();
		}
		else {
			// we have a modal card as the active card and a new card is being added to the system. Perform all the actions here.

			if (m_activeGroup) {
				m_activeGroup->raiseCards();
			}

			m_parentOfModalCard->aboutToFocusEvent(true);
			m_parentOfModalCard->queueFocusAction(true);
			m_parentOfModalCard->performPendingFocusAction();
			m_curState->animationsFinished();

			m_animationsActive = false;
			updateAllowWindowUpdates();

			//CardWindowManagerStates relies on m_addingModalWindow for it's states. Don't call resetModalFlags, just change this flag
			m_addingModalWindow = false;
		}
	}
	else {
		if(!((ModalWindowAddInitCheckFail == m_modalWindowState) || (ModalParentDismissed == m_modalWindowState) || (ModalWindowDismissedParentSwitched == m_modalWindowState))) {
			resetModalFlags();
		}
	}

	// Finally - if the modal was dismissed externally then make it invisible here so that it doesn't linger around. ResetModal() will clear out the flags on the parent
	if((ModalWindowDismissedExternally == m_modalWindowState || ModalWindowDismissedParentSwitched == m_modalWindowState) && (activeWin->type() == Window::Type_ModalChildWindowCard))
		activeWin->setVisible(false);
}

void CardWindowManager::slotOpacityAnimationFinished()
{
	performPostModalWindowRemovedActions(NULL, true);
}

void CardWindowManager::removeCardFromGroup(CardWindow* win, bool adjustLayout)
{
	if(Window::Type_ModalChildWindowCard == win->type())
		return;

	CardGroup* group = win->cardGroup();
	luna_assert(group != 0);

	group->removeFromGroup(win);
	if (group->empty()) {
		// clean up this group
		m_groups.remove(m_groups.indexOf(group));
		removeAnimationForGroup(group);
		delete group;
	}

	// make sure we aren't holding a reference to a soon to be deleted card
	if (m_draggedWin == win) {
		// clear any dragging state
		resetMouseTrackState();
	}

	// If we are removing a modal dialog , we don't need these.
	if(adjustLayout) {
		// select a new active group
		setActiveGroup(groupClosestToCenterHorizontally());

        // make sure everything is positioned properly
    	slideAllGroups();
    }
}

void CardWindowManager::removeCardFromGroupMaximized(CardWindow* win)
{
	IMEController::instance()->removeClient(win);

	if(Window::Type_ModalChildWindowCard == win->type())
		return;

	// Switch out only if we are the active window
	if (activeWindow() == win) {

		Q_EMIT signalMinimizeActiveWindow();

		removeCardFromGroup(win);

		return;
	}else if(NULL != m_parentOfModalCard && m_parentOfModalCard == win && true == m_parentOfModalCard->isCardModalParent()) {
		removeCardFromGroup(win);
		return;
	}

	removeCardFromGroup(win, false);
}

void CardWindowManager::layoutGroups(qreal xDiff)
{
	if (!m_activeGroup || m_groups.empty())
		return;

	int activeGroupPosition = m_groups.indexOf(m_activeGroup);
	removeAnimationForGroup(m_activeGroup);
	m_activeGroup->setX(m_activeGroup->x() + xDiff);
	int centerX = -m_activeGroup->left() - kGapBetweenGroups + m_activeGroup->x();
	for (int i=activeGroupPosition-1; i>=0; i--) {

		centerX += -m_groups[i]->right();
		removeAnimationForGroup(m_groups[i]);
		m_groups[i]->setX(centerX);

		centerX += -kGapBetweenGroups - m_groups[i]->left();
	}

	centerX = m_activeGroup->right() + kGapBetweenGroups + m_activeGroup->x();
	for (int i=activeGroupPosition+1; i<m_groups.size(); i++) {

		centerX += m_groups[i]->left();
		removeAnimationForGroup(m_groups[i]);
		m_groups[i]->setX(centerX);

		centerX += kGapBetweenGroups + m_groups[i]->right();
	}
}

void CardWindowManager::maximizeActiveWindow(bool animate)
{
	if (!m_activeGroup)
		return;

	Q_EMIT signalExitReorder();

	QRect r;

	// If the currently active card window is a modal window, don't do any of these operations except If we are doing this as a part of rotation
	if(activeWindow()->type() != Window::Type_ModalChildWindowCard || (activeWindow()->type() == Window::Type_ModalChildWindowCard && true == SystemUiController::instance()->isUiRotating())) {

		m_activeGroup->raiseCards();

		setActiveGroup(m_activeGroup);

		if(animate)
			slideAllGroups(false);
		else
			layoutAllGroups(false);

		if(activeWindow()->type() != Window::Type_ModalChildWindowCard)
			r = normalOrScreenBounds(m_activeGroup->activeCard());
		else if(NULL != m_parentOfModalCard)
			r = normalOrScreenBounds(m_parentOfModalCard);

		if(animate) {
			QList<QPropertyAnimation*> maxAnims = m_activeGroup->maximizeActiveCard(r.y()/2);

			Q_FOREACH(QPropertyAnimation* anim, maxAnims) {

				setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
			}
			startAnimations();
		} else {
			m_activeGroup->maximizeActiveCardNoAnimation(r.y()/2);
		}
	}
	else {
		// Create an empty animation and add to m_anims.start(). When it completes, it'll call performPendingFocusActions(); to give focus to the active card.
		QPropertyAnimation* anim = new QPropertyAnimation();
		m_anims.addAnimation(anim);

		if(false == m_initModalMaximizing) {
			// Notify SystemUiController that the modal window was added successfully.
			if(true == m_addingModalWindow) {
				// Notify SysUiController that we have setup a modal
				notifySysControllerOfModalStatus(SystemUiController::NoErr, false, ModalLaunch, activeWindow());
			}
			m_initModalMaximizing = true;
		}
		// start the animations
		startAnimations();
	}

	Q_EMIT signalMaximizeActiveWindow();
}

void CardWindowManager::slotMaximizeActiveCardWindow()
{
	if(false == m_addingModalWindow)
		maximizeActiveWindow();
}

void CardWindowManager::minimizeActiveWindow(bool animate)
{
	disableCardRestoreToMaximized();

	Q_EMIT signalExitReorder();

	if (m_activeGroup)
		m_activeGroup->raiseCards();

	// always allow transitions to minimized mode
	if(false == m_addingModalWindow) {

		Q_EMIT signalMinimizeActiveWindow();

		if(animate)
			slideAllGroups();
		else
			layoutAllGroups();
	}
	else {
		m_modalWindowState = ModalWindowDismissedInternally;
		notifySysControllerOfModalStatus(SystemUiController::UiMinimized, true, ModalDismissAnimate);
	}

}

void CardWindowManager::markFirstCardDone()
{
    g_message("[%s]: DEBUG: staring markFirstCardDone.", __PRETTY_FUNCTION__);
    m_dismissedFirstCard=true;
    // For first-use mode, touch a marker file on the filesystem
    //if (Settings::LunaSettings()->uiType == Settings::UI_MINIMAL) {
    g_mkdir_with_parents(Settings::LunaSettings()->lunaPrefsPath.c_str(), 0755);
    FILE* f = fopen(Settings::LunaSettings()->firstCardLaunch.c_str(), "w");
    fclose(f);
    //}
}

void CardWindowManager::firstCardAlert()
{
    if(!m_dismissedFirstCard){
        Q_EMIT signalFirstCardRun();

        markFirstCardDone();
    }
}

void CardWindowManager::handleKeyNavigationMinimized(QKeyEvent* keyEvent)
{
    if (!m_activeGroup || keyEvent->type() != QEvent::KeyPress)
        return;

    switch (keyEvent->key()) {
    case Qt::Key_Left:
        switchToPrevApp();
        break;
    case Qt::Key_Right:
        switchToNextApp();
        break;
    case Qt::Key_Return:
        if (!keyEvent->isAutoRepeat())
            maximizeActiveWindow();
        break;
    case Qt::Key_Backspace:
        if ((keyEvent->modifiers() & Qt::ControlModifier) && !keyEvent->isAutoRepeat())
            closeWindow(activeWindow());
        break;
    default:
        break;
    }
}

void CardWindowManager::slotMinimizeActiveCardWindow()
{
	if(false == m_addingModalWindow)
		minimizeActiveWindow();
	else {
		m_modalWindowState = ModalWindowDismissedInternally;
		notifySysControllerOfModalStatus(SystemUiController::HomeButtonPressed, true, ModalDismissAnimate);
	}
}

void CardWindowManager::setActiveCardOffScreen(bool fullsize)
{
	CardWindow* activeCard = activeWindow();
	if (!activeCard)
		return;

	// safety precaution
	removeAnimationForWindow(activeCard);

	CardWindow::Position pos;
	qreal yOffset = boundingRect().bottom() - activeCard->y();
	pos.trans.setZ(fullsize ? 1.0 : kActiveScale);
	pos.trans.setY(yOffset - activeCard->boundingRect().y() * pos.trans.z());
	activeCard->setPosition(pos);
}

void CardWindowManager::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	// We may get a second pen down. Just ignore it.
	if (m_penDown)
		return;

	resetMouseTrackState();

	if (m_groups.empty() || !m_activeGroup)
		return;

	m_penDown = true;
	updateAllowWindowUpdates();

	m_curState->mousePressEvent(event);
}

void CardWindowManager::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	mousePressEvent(event);
}

void CardWindowManager::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	if (!m_penDown || m_seenFlickOrTap)
		return;

	m_curState->mouseMoveEvent(event);
}

void CardWindowManager::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if (m_penDown)
		m_curState->mouseReleaseEvent(event);

	resetMouseTrackState();
}

bool CardWindowManager::playAngryCardSounds() const
{
    return WindowServer::instance()->getUiOrientation() == OrientationEvent::Orientation_Down;
}

bool CardWindowManager::sceneEvent(QEvent* event)
{
	if (event->type() == QEvent::GestureOverride && m_curState != m_maximizeState) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* g = ge->gesture(Qt::TapGesture);
		if (g) {
			event->accept();
			return true;
		}
		g = ge->gesture(Qt::TapAndHoldGesture);
		if (g) {
			event->accept();
			return true;
		}
		g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
		if (g) {
			event->accept();
			return true;
		}
	}
	else if (event->type() == QEvent::Gesture) {
		QGestureEvent* ge = static_cast<QGestureEvent*>(event);
		QGesture* g = ge->gesture(Qt::TapGesture);
		if (g) {
			QTapGesture* tap = static_cast<QTapGesture*>(g);
			if (tap->state() == Qt::GestureFinished) {
				tapGestureEvent(tap);
			}
			return true;
		}
		g = ge->gesture(Qt::TapAndHoldGesture);
		if (g) {
			QTapAndHoldGesture* hold = static_cast<QTapAndHoldGesture*>(g);
			if (hold->state() == Qt::GestureFinished) {
				tapAndHoldGestureEvent(hold);
			}
			return true;
		}
		g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
		if (g) {
			FlickGesture* flick = static_cast<FlickGesture*>(g);
			if (flick->state() == Qt::GestureFinished) {
				flickGestureEvent(ge);
			}
			return true;
		}
	}
	return QGraphicsObject::sceneEvent(event);
}

void CardWindowManager::tapGestureEvent(QTapGesture* event)
{
	if (!m_penDown)
		return;

	m_seenFlickOrTap = true;

	if (m_groups.empty() || !m_activeGroup)
		return;

	m_curState->tapGestureEvent(event);
}

void CardWindowManager::tapAndHoldGestureEvent(QTapAndHoldGesture* event)
{
	if (!m_penDown)
		return;

	if (m_groups.empty() || !m_activeGroup)
		return;

	m_curState->tapAndHoldGestureEvent(event);
}

void CardWindowManager::handleTapAndHoldGestureMinimized(QTapAndHoldGesture* event)
{
	QPoint pt = mapFromScene(event->position()).toPoint();
	if (m_activeGroup->setActiveCard(event->position())) {
		// start reordering the active card
		Q_EMIT signalEnterReorder(pt, s_marginSlice);
	}
	else if (pt.x() < 0) {
		switchToPrevGroup();
	}
	else {
		switchToNextGroup();
	}
}

void CardWindowManager::flickGestureEvent(QGestureEvent* event)
{
    g_message("%s", __PRETTY_FUNCTION__);
	if (!m_penDown || m_seenFlickOrTap)
		return;

	m_seenFlickOrTap = true;

	if (m_groups.empty() || !m_activeGroup)
		return;

	m_curState->flickGestureEvent(event);
}

void CardWindowManager::handleFlickGestureMinimized(QGestureEvent* event)
{
	QGesture* g = event->gesture((Qt::GestureType) SysMgrGestureFlick);
	if (!g)
		return;

	FlickGesture* flick = static_cast<FlickGesture*>(g);

	if (m_movement == MovementVLocked) {

		if (!m_draggedWin) {
			slideAllGroups();
			return;
		}

		QPointF start = mapFromScene(event->mapToGraphicsScene(flick->hotSpot()));
		QPointF end = mapFromScene(event->mapToGraphicsScene(flick->endPos()));
		int distanceY = end.y() - start.y();

		static const int flickDistanceVelocityMultiplied = kFlickToCloseWindowVelocityThreshold *
														   kFlickToCloseWindowDistanceThreshold;

        QRectF pr = m_draggedWin->mapRectToParent(m_draggedWin->boundingRect());
		if (distanceY < kFlickToCloseWindowDistanceThreshold &&
			flick->velocity().y() < kFlickToCloseWindowMinimumVelocity &&
			flick->velocity().y() < (flickDistanceVelocityMultiplied / distanceY)) {

			closeWindow(m_draggedWin);
		}
		else if (pr.center().y() > boundingRect().bottom()) {
			closeWindow(m_draggedWin, true);
		}
        else if (pr.center().y() < boundingRect().top()) {
            closeWindow(m_draggedWin);
        }
		else {
			slideAllGroups();
		}
	}
	else if (m_movement == MovementHLocked) {

		if (m_trackWithinGroup) {

			// adjust the fanning position within the group based on the users flick velocity
			m_activeGroup->flick(flick->velocity().x());
			setActiveGroup(m_activeGroup);
			slideAllGroups();
		}
		else {
			// advance to the next/previous group if we are Outer Locked or were still unbiased horizontally
			if (flick->velocity().x() > 0)
				switchToPrevGroup();
			else
				switchToNextGroup();
		}
	}
}

void CardWindowManager::handleMousePressMinimized(QGraphicsSceneMouseEvent* event)
{
	// try to capture the card the user first touched
    if (m_activeGroup && m_activeGroup->setActiveCard(event->scenePos()))
        m_draggedWin = m_activeGroup->activeCard();
}

void CardWindowManager::handleMouseMoveMinimized(QGraphicsSceneMouseEvent* event)
{
	if (m_groups.isEmpty() || !m_activeGroup)
		return;

	QPoint delta = (event->pos() - event->buttonDownPos(Qt::LeftButton)).toPoint();
	QPoint diff; // distance move between last and current mouse position

	// lock movement to an axis
	if (m_movement == MovementUnlocked) {

		if ((delta.x() * delta.x() + delta.y() * delta.y()) <
			Settings::LunaSettings()->tapRadiusSquared)
			return;

		if (abs(delta.x()) > 0.866 * abs(delta.y())) {

			m_movement = MovementHLocked;
            m_activeGroupPivot = m_activeGroup->x();
		}
		else {

			m_movement = MovementVLocked;
		}

		diff = delta;
	}
	else {

		diff = (event->pos() - event->lastPos()).toPoint();
	}

	if (m_movement == MovementHLocked) {

		if (m_trackWithinGroup) {

			m_trackWithinGroup = !m_activeGroup->atEdge(diff.x());
			if (m_trackWithinGroup) {

				// shift cards within the active group
				m_activeGroup->adjustHorizontally(diff.x());

				slideAllGroups();
			}
            else {
                m_activeGroupPivot = m_activeGroup->x();
            }
		}
		
		if (!m_trackWithinGroup) {

            m_activeGroupPivot += diff.x();
            slideAllGroupsTo(m_activeGroupPivot);
		}
	}
	else if (m_movement == MovementVLocked) {

		if (!m_draggedWin) {
			if (m_activeGroup->setActiveCard(event->scenePos()))
				m_draggedWin = m_activeGroup->activeCard();
			if (!m_draggedWin)
				return;
		}

		// ignore pen movements outside the vertical pillar around the active window
		QPointF mappedPos = m_draggedWin->mapFromParent(event->pos());
		if (mappedPos.x() < m_draggedWin->boundingRect().x() ||
			mappedPos.x() >= m_draggedWin->boundingRect().right()) {
			return;
		}

		if (delta.y() == 0)
			return;

        if (!m_playedAngryCardStretchSound && (delta.y() > kAngryCardThreshold) && playAngryCardSounds()) {
            SoundPlayerPool::instance()->playFeedback("carddrag");
            m_playedAngryCardStretchSound = true;
        }

		removeAnimationForWindow(m_draggedWin);

		// cards are always offset from the parents origin
		CardWindow::Position pos = m_draggedWin->position();
		pos.trans.setY(delta.y());
		m_draggedWin->setPosition(pos);
	}
}

void CardWindowManager::handleMouseMoveReorder(QGraphicsSceneMouseEvent* event)
{
	CardWindow* activeWin = activeWindow();
	if (!activeWin)
		return;

	// track the active window under the users finger
	QPoint delta = (event->pos() - event->lastPos()).toPoint();
	CardWindow::Position pos;
	pos.trans = QVector3D(activeWin->position().trans.x() + delta.x(), 
						  activeWin->position().trans.y() + delta.y(), 
						  kActiveScale);
	activeWin->setPosition(pos);

	// should we switch zones?
	ReorderZone newZone = getReorderZone(event->pos().toPoint());

	if (newZone == m_reorderZone && newZone == ReorderZone_Center) {

		moveReorderSlotCenter(event->pos());
	}
	else if (newZone != m_reorderZone) {

		if (newZone == ReorderZone_Right) {

			m_reorderZone = newZone;
			moveReorderSlotRight();
		}
		else if (newZone == ReorderZone_Left) {

			m_reorderZone = newZone;
			moveReorderSlotLeft();
		}
		else {

			m_reorderZone = newZone;
		}
	}
}

CardWindowManager::ReorderZone CardWindowManager::getReorderZone(QPoint pt)
{
	qreal section = boundingRect().width() / s_marginSlice;
	if (pt.x() < boundingRect().left() + section) {
		return ReorderZone_Left;
	}
	else if (pt.x() > boundingRect().right() - section) {
		return ReorderZone_Right;
	}
	return ReorderZone_Center;
}

void CardWindowManager::enterReorder(QPoint pt)
{
	CardWindow* activeWin = activeWindow();
	luna_assert(activeWin != 0);
	activeWin->setOpacity(0.8);
	activeWin->disableShadow();

	activeWin->setAttachedToGroup(false);

	// get our initial zone
	m_reorderZone = getReorderZone(pt);
}

void CardWindowManager::cycleReorderSlot()
{
	if (m_reorderZone == ReorderZone_Right)
		moveReorderSlotRight();
	else if (m_reorderZone == ReorderZone_Left)
		moveReorderSlotLeft();
}

void CardWindowManager::moveReorderSlotCenter(QPointF pt)
{
	bool animate = false;
	int duration = 0;
	QEasingCurve::Type curve = QEasingCurve::Linear;

	// NOTE: it is assumed that the active card has already 
	// been repositioned when making this check
	if (m_activeGroup->moveActiveCard()) {

		m_activeGroup->raiseCards();

		duration = AS(cardShuffleReorderDuration);
		curve = AS_CURVE(cardShuffleReorderCurve);
		animate = true;
	}

    setActiveGroup(m_activeGroup);

	if (animate)
		arrangeWindowsAfterReorderChange(duration, curve);
}

void CardWindowManager::moveReorderSlotRight()
{
	bool animate = false;
	int duration = 0;
	QEasingCurve::Type curve = QEasingCurve::Linear;

    CardGroup* newActiveGroup = m_activeGroup;

	// have we reached the top card in the group?
	if (m_activeGroup->moveActiveCard(1)) {
		// no, update the stacking order within the group
		m_activeGroup->raiseCards();

		duration = AS(cardShuffleReorderDuration);
		curve = AS_CURVE(cardShuffleReorderCurve);
		animate = true;
	}
	else if (m_activeGroup != m_groups.last() || m_activeGroup->size() > 1) {

		CardWindow* activeWin = activeWindow();
		int activeIndex = m_groups.indexOf(m_activeGroup);
		// yes, remove from the active group
		m_activeGroup->removeFromGroup(activeWin);
		if (m_activeGroup->empty()) {
			// this was a temporarily created group.
			// delete the temp group.
			m_groups.remove(activeIndex);
			delete m_activeGroup;

            newActiveGroup = m_groups[activeIndex];
		}
		else {
			// this was an existing group.
			// insert a new group to the right of the current active group
			CardGroup* newGroup = new CardGroup(kActiveScale, kNonActiveScale);
			newGroup->setPos(QPointF(0, kWindowOrigin));
			m_groups.insert(activeIndex+1, newGroup);

            newActiveGroup = newGroup;
		}
        newActiveGroup->addToBack(activeWin);
        newActiveGroup->raiseCards();

		duration = AS(cardGroupReorderDuration);
		curve = AS_CURVE(cardGroupReorderCurve);
		animate = true;
	}
			
    setActiveGroup(newActiveGroup);

	if (animate)
		arrangeWindowsAfterReorderChange(duration, curve);
}

void CardWindowManager::moveReorderSlotLeft()
{
	bool animate = false;
	int duration = 0;
	QEasingCurve::Type curve = QEasingCurve::Linear;

    CardGroup* newActiveGroup = m_activeGroup;

	// have we reached the bottom card in the group?
	if (m_activeGroup->moveActiveCard(-1)) {
		// no, update the stacking order within the group
		m_activeGroup->raiseCards();

		duration = AS(cardShuffleReorderDuration);
		curve = AS_CURVE(cardShuffleReorderCurve);
		animate = true;
	}
	else if (m_activeGroup != m_groups.first() || m_activeGroup->size() > 1) {

		CardWindow* activeWin = activeWindow();
		int activeIndex = m_groups.indexOf(m_activeGroup);
		// yes, remove from the active group
		m_activeGroup->removeFromGroup(activeWin);
		if (m_activeGroup->empty()) {
			// this was a temporarily created group.
			// delete the temp group
			m_groups.remove(activeIndex);
			delete m_activeGroup;

			// the previous group is the new active group
			newActiveGroup = m_groups[qMax(0, activeIndex-1)];
		}
		else {
			// this was an existing group.
			// insert a new group to the left of the current active group
			CardGroup* newGroup = new CardGroup(kActiveScale, kNonActiveScale);
			newGroup->setPos(QPointF(0, kWindowOrigin));
			m_groups.insert(activeIndex, newGroup);

            newActiveGroup = newGroup;
		}
        newActiveGroup->addToFront(activeWin);
        newActiveGroup->raiseCards();

		duration = AS(cardGroupReorderDuration);
		curve = AS_CURVE(cardGroupReorderCurve);
		animate = true;
	}
			
    setActiveGroup(newActiveGroup);

	if (animate)
		arrangeWindowsAfterReorderChange(duration, curve);
}

void CardWindowManager::arrangeWindowsAfterReorderChange(int duration, QEasingCurve::Type curve)
{
	if (m_groups.empty() || !m_activeGroup)
		return;

	int activeGrpIndex = m_groups.indexOf(m_activeGroup);

	clearAnimations();

	QPropertyAnimation* anim = new QPropertyAnimation(m_activeGroup, "x");
	anim->setEasingCurve(AS_CURVE(cardShuffleReorderCurve));
	anim->setDuration(AS(cardShuffleReorderDuration));
	anim->setEndValue(0);
	setAnimationForGroup(m_activeGroup, anim);
	QList<QPropertyAnimation*> cardAnims = m_activeGroup->animateOpen(duration, curve, false);
	Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

		setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
	}

	int centerX = -m_activeGroup->left() - kGapBetweenGroups;
	for (int i=activeGrpIndex-1; i>=0;i--) {

		cardAnims = m_groups[i]->animateClose(duration, curve);
		centerX += -m_groups[i]->right();

		anim = new QPropertyAnimation(m_groups[i], "x");
		anim->setEasingCurve(curve);
		anim->setDuration(duration);
		anim->setEndValue(centerX);
		setAnimationForGroup(m_groups[i], anim);
		Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

			setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
		}

		centerX += -kGapBetweenGroups - m_groups[i]->left();
	}

	centerX = m_activeGroup->right() + kGapBetweenGroups;
	for (int i=activeGrpIndex+1; i<m_groups.size(); i++) {

		cardAnims = m_groups[i]->animateClose(duration, curve);
		centerX += m_groups[i]->left();

		anim = new QPropertyAnimation(m_groups[i], "x");
		anim->setEasingCurve(curve);
		anim->setDuration(duration);
		anim->setEndValue(centerX);
		setAnimationForGroup(m_groups[i], anim);
		Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

			setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
		}

		centerX += kGapBetweenGroups + m_groups[i]->right();
	}

	startAnimations();
}

void CardWindowManager::handleMouseReleaseMinimized(QGraphicsSceneMouseEvent* event)
{
	if (m_groups.empty() || m_seenFlickOrTap)
		return;

	if (m_movement == MovementVLocked) {

		// Did we go too close to the top?
		if (m_draggedWin) {
            QRectF pr = m_draggedWin->mapRectToParent(m_draggedWin->boundingRect());
            if ((!event->canceled()) && (pr.center().y() > boundingRect().bottom())) {
                closeWindow(m_draggedWin, true);
            }
            else if ((!event->canceled()) && (pr.center().y() < boundingRect().top())) {
				closeWindow(m_draggedWin);
			}
			else {
				// else just restore all windows back to original position
				slideAllGroups();
			}
		}
	}
	else if (m_movement == MovementHLocked) {

		if(!event->canceled())
			setActiveGroup(groupClosestToCenterHorizontally());
		slideAllGroups();
	}
}

void CardWindowManager::handleMouseReleaseReorder(QGraphicsSceneMouseEvent* event)
{
	Q_UNUSED(event)

	Q_EMIT signalExitReorder(false);

	CardWindow* activeWin = activeWindow();
	if (!activeWin)
		return;

	// TODO: fix the y for the draggedWin
	activeWin->setOpacity(1.0);
	activeWin->enableShadow();
	activeWin->setAttachedToGroup(true);

	slideAllGroups();
}

void CardWindowManager::handleTapGestureMinimized(QTapGesture* event)
{
	if (!m_activeGroup)
		return;

	//Things that must be done here:
	//--Perform a hit test to determine if the current active group was hit
	if (m_activeGroup->testHit(event->position())) {
		//--If it was, then we need to see if the card hit was in a reasonable
		//  range of the active card.  If it was then maximize it.  If it was not
		//  slide the card fan over to make it more visible.
		if (m_activeGroup->shouldMaximizeOrScroll(event->position())) {
			m_activeGroup->setActiveCard(event->position());
			m_activeGroup->moveToActiveCard();
			maximizeActiveWindow();
		}
		else {
			slideAllGroups();
		}
	}
	else {
		// first test to see if the tap is not above/below the active group
		QPointF pt = mapFromScene(event->position());
		if (!m_activeGroup->withinColumn(pt)) {
			if (pt.x() < 0) {
				// tapped to the left of the active group
				switchToPrevGroup();
			}
			else {
				// tapped to the right of the active group
				switchToNextGroup();
			}
		}
		else {

			// poke the groups to make sure they animate to their final positions
			slideAllGroups();
		}
	}
}

CardGroup* CardWindowManager::groupClosestToCenterHorizontally() const
{
	if (m_groups.empty())
		return 0;

	qreal deltaX = FLT_MAX;
	qreal curDeltaX = 0;
	CardGroup* grp = 0;
	Q_FOREACH(CardGroup* cg, m_groups) {
		curDeltaX = qAbs(cg->pos().x());
		if (curDeltaX < deltaX) {
			grp = cg;
			deltaX = curDeltaX;
		}
	}

	return grp;
}

void CardWindowManager::setActiveGroup(CardGroup* group)
{
	m_activeGroup = group;

	SystemUiController::instance()->setActiveCardWindow(m_activeGroup ? m_activeGroup->activeCard() : 0);
}

void CardWindowManager::switchToNextApp()
{
	disableCardRestoreToMaximized();

	if (!m_activeGroup || m_groups.empty())
		return;

	if (!m_activeGroup->makeNextCardActive()) {
		// couldn't move, switch to the next group
		int index = m_groups.indexOf(m_activeGroup);
		if (index < m_groups.size() - 1) {

			m_activeGroup = m_groups[index + 1];
            m_activeGroup->makeBackCardActive();
		}
	}
			
    setActiveGroup(m_activeGroup);

	slideAllGroups();
}

void CardWindowManager::switchToPrevApp()
{
	disableCardRestoreToMaximized();

	if (!m_activeGroup || m_groups.empty())
		return;

	if (!m_activeGroup->makePreviousCardActive()) {

		// couldn't move, switch to the previous group
		int index = m_groups.indexOf(m_activeGroup);
		if (index > 0) {

			m_activeGroup = m_groups[index - 1];
            m_activeGroup->makeFrontCardActive();
		}
	}
			
    setActiveGroup(m_activeGroup);

	slideAllGroups();
}

void CardWindowManager::switchToNextGroup()
{
	disableCardRestoreToMaximized();

	if (!m_activeGroup || m_groups.empty())
		return;

	if (m_activeGroup == m_groups.last()) {
		slideAllGroups();
		return;
	}

	int activeGroupIndex = m_groups.indexOf(m_activeGroup);
	activeGroupIndex++;
	activeGroupIndex = qMin(activeGroupIndex, m_groups.size() - 1);

	setActiveGroup(m_groups[activeGroupIndex]);

	slideAllGroups();
}

void CardWindowManager::switchToPrevGroup()
{
	disableCardRestoreToMaximized();

	if (!m_activeGroup || m_groups.empty())
		return;

	if (m_activeGroup == m_groups.first()) {
		slideAllGroups();
		return;
	}

	int activeGroupIndex = m_groups.indexOf(m_activeGroup);
	activeGroupIndex--;
	activeGroupIndex = qMax(activeGroupIndex, 0);

	setActiveGroup(m_groups[activeGroupIndex]);

	slideAllGroups();
}

void CardWindowManager::switchToNextAppMaximized()
{
	disableCardRestoreToMaximized();

	if (!m_activeGroup || m_groups.empty())
		return;

	CardWindow* oldActiveCard = activeWindow();

	if(!oldActiveCard)
		return;

	// Check if the currently active card is a modal card. If so dismiss it
	if(Window::Type_ModalChildWindowCard == oldActiveCard->type()) {
		// We don't need to run any animations
		m_animateWindowForModalDismisal = false;
		// Set the reason for dismissal
		m_modalWindowState = ModalWindowDismissedParentSwitched;
		// Notify SysUi Controller that we no longer have a modal active
		notifySysControllerOfModalStatus(SystemUiController::ActiveCardsSwitched, false, ModalDismissNoAnimate);

		// Set the fact that for all purposes m_parentOfModalCard is the currently ative card
		if(m_parentOfModalCard) {
			// If this card is a modal parent, clear that flag
			if(m_parentOfModalCard->isCardModalParent())
				m_parentOfModalCard->setCardIsModalParent(false);

			// Set the fact that this card no longer has a modal child
			if(NULL != m_parentOfModalCard->getModalChild())
				m_parentOfModalCard->setModalChild(NULL);

			// Set that this card needs to process all input
			m_parentOfModalCard->setModalAcceptInputState(CardWindow::NoModalWindow);
		}

		// Get the old active window for further use
		oldActiveCard = activeWindow();
	}

	SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::CARD_WINDOW_MANAGER, oldActiveCard, false);

	if (!m_activeGroup->makeNextCardActive()) {

		if (m_activeGroup == m_groups.last()) {

			// shift card off to the side and let it slide back
			CardWindow::Position pos = oldActiveCard->position();
			pos.trans.setX(pos.trans.x() - 40);
			oldActiveCard->setPosition(pos);
		}
		else {

			// switch to the bottom card of the next group
			int index = m_groups.indexOf(m_activeGroup);
			setActiveGroup(m_groups[index+1]);
			m_activeGroup->makeBackCardActive();
		}
	}

	CardWindow* newActiveCard = activeWindow();
	if (oldActiveCard != newActiveCard) {
		QRect r(normalOrScreenBounds(0));
		if (oldActiveCard->allowResizeOnPositiveSpaceChange())
			oldActiveCard->resizeEventSync(r.width(), r.height());
		else
			oldActiveCard->adjustForPositiveSpaceSize(r.width(), r.height());
				
		queueFocusAction(oldActiveCard, false);
		oldActiveCard->setAttachedToGroup(true);

		queueFocusAction(newActiveCard, true);
		newActiveCard->setAttachedToGroup(false);

		QRectF boundingRect = normalOrScreenBounds(0);
		oldActiveCard->setBoundingRect(boundingRect.width(), boundingRect.height());

		boundingRect = normalOrScreenBounds(newActiveCard);
		newActiveCard->setBoundingRect(boundingRect.width(), boundingRect.height());

		SystemUiController::instance()->setMaximizedCardWindow(newActiveCard);
	}

	// maximize the new active window
	maximizeActiveWindow();
}

void CardWindowManager::switchToPrevAppMaximized()
{
	disableCardRestoreToMaximized();

	if (!m_activeGroup || m_groups.empty())
		return;

	CardWindow* oldActiveCard = activeWindow();

	// Check if the currently active card is a modal card. If so dismiss it
	if(Window::Type_ModalChildWindowCard == oldActiveCard->type()) {

		// We don't need to run any animations
		m_animateWindowForModalDismisal = false;
		// Set the reason for dismissal
		m_modalWindowState = ModalWindowDismissedParentSwitched;
		// Notify SysUi Controller that we no longer have a modal active
		notifySysControllerOfModalStatus(SystemUiController::ActiveCardsSwitched, false, ModalDismissNoAnimate);

		// Set the fact that for all purposes m_parentOfModalCard is the currently ative card
		if(m_parentOfModalCard) {
			// If this card is a modal parent, clear that flag
			if(m_parentOfModalCard->isCardModalParent())
				m_parentOfModalCard->setCardIsModalParent(false);

			// Set the fact that this card no longer has a modal child
			if(NULL != m_parentOfModalCard->getModalChild())
				m_parentOfModalCard->setModalChild(NULL);

			// Set that this card needs to process all input
			m_parentOfModalCard->setModalAcceptInputState(CardWindow::NoModalWindow);
		}

		// Get the old active window for further use
		oldActiveCard = activeWindow();
	}

	SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::CARD_WINDOW_MANAGER, oldActiveCard, false);

	if (!m_activeGroup->makePreviousCardActive()) {

		if (m_activeGroup == m_groups.first()) {

			// shift card off to the side and let it slide back
			CardWindow::Position pos = oldActiveCard->position();
			pos.trans.setX(pos.trans.x() + 40);
			oldActiveCard->setPosition(pos);
		}
		else {

			// shift to the bottom card in the next group 
			int index = m_groups.indexOf(m_activeGroup);
			setActiveGroup(m_groups[index-1]);
			m_activeGroup->makeFrontCardActive();
		}
	}

	CardWindow* newActiveCard = activeWindow();
	if (oldActiveCard != newActiveCard) {
		// current maximized card 
		QRect r(normalOrScreenBounds(0));
		if (oldActiveCard->allowResizeOnPositiveSpaceChange())
			oldActiveCard->resizeEventSync(r.width(), r.height());
		else
			oldActiveCard->adjustForPositiveSpaceSize(r.width(), r.height());

		queueFocusAction(oldActiveCard, false);
		oldActiveCard->setAttachedToGroup(true);

		queueFocusAction(newActiveCard, true);
		newActiveCard->setAttachedToGroup(false);
		
		QRectF boundingRect = normalOrScreenBounds(0);
		oldActiveCard->setBoundingRect(boundingRect.width(), boundingRect.height());
		
		boundingRect = normalOrScreenBounds(newActiveCard);
		newActiveCard->setBoundingRect(boundingRect.width(), boundingRect.height());

		SystemUiController::instance()->setMaximizedCardWindow(newActiveCard);
	}

	// maximize the new active window
	maximizeActiveWindow();
}

void CardWindowManager::slideAllGroups(bool includeActiveCard)
{
	if (m_groups.empty() || !m_activeGroup)
		return;

	int activeGrpIndex = m_groups.indexOf(m_activeGroup);

	clearAnimations();

	QPropertyAnimation* anim = new QPropertyAnimation(m_activeGroup, "x");
	anim->setEasingCurve(AS_CURVE(cardSlideCurve));
	anim->setDuration(AS(cardSlideDuration));
	anim->setEndValue(0);
	setAnimationForGroup(m_activeGroup, anim);
	QList<QPropertyAnimation*> cardAnims = m_activeGroup->animateOpen(200, QEasingCurve::OutCubic, includeActiveCard);
	Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

		setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
	}

	int centerX = -m_activeGroup->left() - kGapBetweenGroups;
	for (int i=activeGrpIndex-1; i>=0;i--) {

		cardAnims = m_groups[i]->animateClose(AS(cardSlideDuration), AS_CURVE(cardSlideCurve));
		centerX += -m_groups[i]->right();

		anim = new QPropertyAnimation(m_groups[i], "x");
		anim->setEasingCurve(AS_CURVE(cardSlideCurve));
		anim->setDuration(AS(cardSlideDuration));
		anim->setEndValue(centerX);
		setAnimationForGroup(m_groups[i], anim);
		Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

			setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
		}

		centerX += -kGapBetweenGroups - m_groups[i]->left();
	}

	centerX = m_activeGroup->right() + kGapBetweenGroups;
	for (int i=activeGrpIndex+1; i<m_groups.size(); i++) {

		cardAnims = m_groups[i]->animateClose(AS(cardSlideDuration), AS_CURVE(cardSlideCurve));
		centerX += m_groups[i]->left();

		anim = new QPropertyAnimation(m_groups[i], "x");
		anim->setEasingCurve(AS_CURVE(cardSlideCurve));
		anim->setDuration(AS(cardSlideDuration));
		anim->setEndValue(centerX);
		setAnimationForGroup(m_groups[i], anim);
		Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

			setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
		}

		centerX += kGapBetweenGroups + m_groups[i]->right();
	}

	if (includeActiveCard)
		startAnimations();
}

void CardWindowManager::slideAllGroupsTo(int xOffset)
{
	if (m_groups.empty() || !m_activeGroup)
		return;

	int activeGrpIndex = m_groups.indexOf(m_activeGroup);

	clearAnimations();

	QPropertyAnimation* anim = new QPropertyAnimation(m_activeGroup, "x");
	anim->setEasingCurve(AS_CURVE(cardTrackGroupCurve));
	anim->setDuration(AS(cardTrackGroupDuration));
	anim->setEndValue(xOffset);
	setAnimationForGroup(m_activeGroup, anim);
	QList<QPropertyAnimation*> cardAnims = m_activeGroup->animateCloseWithOffset(AS(cardTrackDuration), 
                                                                                AS_CURVE(cardTrackCurve), 
                                                                                xOffset);
	Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

		setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
	}

	int centerX = -m_activeGroup->left() - kGapBetweenGroups + xOffset;
	for (int i=activeGrpIndex-1; i>=0;i--) {

		centerX += -m_groups[i]->right();
		cardAnims = m_groups[i]->animateCloseWithOffset(AS(cardTrackDuration), AS_CURVE(cardTrackCurve), centerX);

		anim = new QPropertyAnimation(m_groups[i], "x");
		anim->setEasingCurve(AS_CURVE(cardTrackGroupCurve));
		anim->setDuration(AS(cardTrackGroupDuration));
		anim->setEndValue(centerX);
		setAnimationForGroup(m_groups[i], anim);
		Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

			setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
		}

		centerX += -kGapBetweenGroups - m_groups[i]->left();
	}

	centerX = m_activeGroup->right() + kGapBetweenGroups + xOffset;
	for (int i=activeGrpIndex+1; i<m_groups.size(); i++) {

		centerX += m_groups[i]->left();
		cardAnims = m_groups[i]->animateCloseWithOffset(AS(cardTrackDuration), AS_CURVE(cardTrackCurve), centerX);

		anim = new QPropertyAnimation(m_groups[i], "x");
		anim->setEasingCurve(AS_CURVE(cardTrackGroupCurve));
		anim->setDuration(AS(cardTrackGroupDuration));
		anim->setEndValue(centerX);
		setAnimationForGroup(m_groups[i], anim);
		Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

			setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
		}

		centerX += kGapBetweenGroups + m_groups[i]->right();
	}

    startAnimations();
}

void CardWindowManager::layoutAllGroups(bool includeActiveCard)
{
	if (m_groups.empty() || !m_activeGroup)
		return;

	int activeGrpIndex = m_groups.indexOf(m_activeGroup);

	clearAnimations();

	m_activeGroup->layoutCards(true, includeActiveCard);

	int centerX = -m_activeGroup->left() - kGapBetweenGroups;
	for (int i=activeGrpIndex-1; i>=0;i--) {

		m_groups[i]->layoutCards(false, false);
		centerX += -m_groups[i]->right();
		m_groups[i]->setX(centerX);
		centerX += -kGapBetweenGroups - m_groups[i]->left();
	}

	centerX = m_activeGroup->right() + kGapBetweenGroups;
	for (int i=activeGrpIndex+1; i<m_groups.size(); i++) {

		m_groups[i]->layoutCards(false, false);
		centerX += m_groups[i]->left();
		m_groups[i]->setX(centerX);
		centerX += kGapBetweenGroups + m_groups[i]->right();
	}
}

void CardWindowManager::slideToActiveCard()
{
	if (m_groups.empty() || !m_activeGroup)
		return;

	int activeGrpIndex = m_groups.indexOf(m_activeGroup);

	clearAnimations();

	QPropertyAnimation* anim = new QPropertyAnimation(m_activeGroup, "x");
	anim->setEasingCurve(AS_CURVE(cardSlideCurve));
	anim->setDuration(AS(cardSlideDuration));
	anim->setEndValue(0);
	setAnimationForGroup(m_activeGroup, anim);

	QList<QPropertyAnimation*> cardAnims = m_activeGroup->animateOpen(AS(cardSlideDuration), 
																	  AS_CURVE(cardSlideCurve));
	Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

		setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
	}

	int centerX = -m_activeGroup->left() - kGapBetweenGroups;
	for (int i=activeGrpIndex-1; i>=0;i--) {

		cardAnims = m_groups[i]->animateClose(AS(cardSlideDuration), AS_CURVE(cardSlideCurve));
		centerX += -m_groups[i]->right();

		anim = new QPropertyAnimation(m_groups[i], "x");
		anim->setEasingCurve(AS_CURVE(cardSlideCurve));
		anim->setDuration(AS(cardSlideDuration));
		anim->setEndValue(centerX);
		setAnimationForGroup(m_groups[i], anim);
		Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

			setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
		}

		centerX += -kGapBetweenGroups - m_groups[i]->left();
	}

	centerX = m_activeGroup->right() + kGapBetweenGroups;
	for (int i=activeGrpIndex+1; i<m_groups.size(); i++) {

		cardAnims = m_groups[i]->animateClose(AS(cardSlideDuration), AS_CURVE(cardSlideCurve));
		centerX += m_groups[i]->left();

		anim = new QPropertyAnimation(m_groups[i], "x");
		anim->setEasingCurve(AS_CURVE(cardSlideCurve));
		anim->setDuration(AS(cardSlideDuration));
		anim->setEndValue(centerX);
		setAnimationForGroup(m_groups[i], anim);
		Q_FOREACH(QPropertyAnimation* anim, cardAnims) {

			setAnimationForWindow(static_cast<CardWindow*>(anim->targetObject()), anim);
		}

		centerX += kGapBetweenGroups + m_groups[i]->right();
	}

	startAnimations();
}

void CardWindowManager::focusWindow(Window* win)
{
	Q_EMIT signalExitReorder();

	disableCardRestoreToMaximized();

	// make sure this are window has already been group'd
	CardWindow* card = static_cast<CardWindow*>(win);
	if (!m_groups.contains(card->cardGroup()) || !m_activeGroup)
		return;

	// If the active card is a modal window and we are focusing another window, we need to dismiss the modal first.
	if(Window::Type_ModalChildWindowCard == activeWindow()->type() && card != activeWindow())
	{
		// Cehck if we are trying to focus the parent
		m_modalWindowState = ModalWindowDismissedParentSwitched;

		if(m_parentOfModalCard != card) {
			// Some other card is being focussed so no need to restore the state of the parent
			notifySysControllerOfModalStatus(SystemUiController::ActiveCardsSwitched, false, ModalDismissNoAnimate, activeWindow());

			// Set the fact that for all purposes m_parentOfModalCard is the currently active card
			if(m_parentOfModalCard) {
				// If this card is a modal parent, clear that flag
				if(m_parentOfModalCard->isCardModalParent())
					m_parentOfModalCard->setCardIsModalParent(false);
				// Set the fact that this card no longer has a modal child
				if(NULL != m_parentOfModalCard->getModalChild())
					m_parentOfModalCard->setModalChild(NULL);
				// Set that this card needs to process all input
				m_parentOfModalCard->setModalAcceptInputState(CardWindow::NoModalWindow);

				//CardWindowManagerStates relies on m_addingModalWindow for it's states. Don't call resetModalFlags, just change this flag
				m_addingModalWindow = false;
				m_modalDimissed = true;
			}
		}
		else {
			// Someone tried to give focus to the parent.
			notifySysControllerOfModalStatus(SystemUiController::ActiveCardsSwitched, true, ModalDismissNoAnimate, activeWindow());
			return;
		}
	}
	Q_EMIT signalFocusWindow(card);
}

void CardWindowManager::slotPositiveSpaceAboutToChange(const QRect& r, bool fullScreenMode, bool screenResizing)
{
	Q_EMIT signalExitReorder();

	m_targetPositiveSpace = r;

	if (m_curState)
		m_curState->positiveSpaceAboutToChange(r, fullScreenMode);
}

void CardWindowManager::slotPositiveSpaceChangeFinished(const QRect& r)
{
	Q_EMIT signalExitReorder();

	if (m_curState)
		m_curState->positiveSpaceChangeFinished(r);
}

void CardWindowManager::slotPositiveSpaceChanged(const QRect& r)
{
	static bool initialBounds = true;
	static qreal kActiveWindowScale = Settings::LunaSettings()->activeCardWindowRatio;
	static qreal kNonActiveWindowScale = Settings::LunaSettings()->nonActiveCardWindowRatio;

	if (initialBounds) {
		initialBounds = false;
		m_normalScreenBounds = r;

		kMinimumHeight = (int) (kMinimumWindowScale * m_normalScreenBounds.height());
		kMinimumHeight = (int) ((kMinimumHeight/2) / kWindowOriginRatio);

		SystemUiController::instance()->setMinimumPositiveSpaceHeight(kMinimumHeight);

		m_targetPositiveSpace = r;

		// TODO: this is a temporary solution to fake the existence of the search pill
		// 	which happens to be 48 pixels tall
		kActiveScale = ((qreal) (r.height() - 48) * kActiveWindowScale) / (qreal) m_normalScreenBounds.height();
		kActiveScale = qMax(kMinimumWindowScale, kActiveScale);

		kNonActiveScale = ((qreal) (r.height() - 48) * kNonActiveWindowScale) / (qreal) m_normalScreenBounds.height();
		kNonActiveScale = qMax(kMinimumWindowScale, kNonActiveScale);

		// allow groups to shift up to a maximum so the tops of cards don't go off the screen
		kWindowOriginMax = (boundingRect().y() + ((r.y() + 48) + (int) ((r.height() - 48) * kWindowOriginRatio))) -
						   Settings::LunaSettings()->positiveSpaceTopPadding -
						   Settings::LunaSettings()->positiveSpaceBottomPadding;

		kWindowOrigin = boundingRect().y() + ((r.y() + 48) + (int) ((r.height() - 48) * kWindowOriginRatio));
	}

	QRect rect = r;
	if (rect.height() < kMinimumHeight) {
		rect.setHeight(kMinimumHeight);
	}

	Q_EMIT signalExitReorder();

	if (m_curState)
		m_curState->positiveSpaceChanged(rect);
}

void CardWindowManager::disableCardRestoreToMaximized()
{
	m_cardToRestoreToMaximized = 0;
}

void CardWindowManager::restoreCardToMaximized()
{
	if (!m_cardToRestoreToMaximized || !m_activeGroup)
		return;

	if (m_activeGroup->setActiveCard(m_cardToRestoreToMaximized))
		maximizeActiveWindow();

	disableCardRestoreToMaximized();
}

QRect CardWindowManager::normalOrScreenBounds(CardWindow* win) const
{
	if (win && win->fullScreen() && win->type() != Window::Type_ModalChildWindowCard) {
		return QRect(m_targetPositiveSpace.x(), m_targetPositiveSpace.y(),
				     SystemUiController::instance()->currentUiWidth(), SystemUiController::instance()->currentUiHeight());
	}
	return m_normalScreenBounds;
}

void CardWindowManager::cancelReorder(bool dueToPenCancel)
{
	handleMouseReleaseReorder(NULL);
}

void CardWindowManager::closeWindow(CardWindow* win, bool angryCard)
{
	if(!win)
		return;

	QPropertyAnimation* anim = NULL;
	/*// The only case we need to worry about here is if a modal parent called closeWindow() on itself. Then we need to close the child first and then continue
	if(true == win->isCardModalParent() && (Window::Type_ModalChildWindowCard == activeWindow()->type())) {
		m_modalWindowState = ModalParentDismissed;
		notifySysControllerOfModalStatus(SystemUiController::ParentCardDismissed, false, ModalDismissNoAnimate);
		win->setCardIsModalParent(false);
		win->setModalChild(NULL);
		win->setModalAcceptInputState(CardWindow::NoModalWindow);
	}*/

    if (angryCard)
        win->setDisableKeepAlive();

    win->close();

	// remove the window from the current animation list
	removeAnimationForWindow(win, true);

	if(Window::Type_ModalChildWindowCard != win->type()) {
		CardWindow::Position pos = win->position();
		QRectF r = win->mapRectToParent(win->boundingRect());
		qreal offTop = boundingRect().y() - (win->y() + (r.height()/2));
		pos.trans.setY(offTop);

		anim = new QPropertyAnimation(win, "position");
		QVariant end; end.setValue(pos);
		anim->setEasingCurve(AS_CURVE(cardDeleteCurve));
		anim->setDuration(AS(cardDeleteDuration));
		anim->setEndValue(end);
	}
	else {
		anim = new QPropertyAnimation();
	}

	QM_CONNECT(anim, SIGNAL(finished()), SLOT(slotDeletedAnimationFinished()));

	m_deletedAnimMap.insert(win, anim);
	anim->start();

	// Modal cards are not a part of any card group.
	if(Window::Type_ModalChildWindowCard != win->type()) {
		removeCardFromGroup(win);
	}

    if (angryCard && playAngryCardSounds())
        SoundPlayerPool::instance()->playFeedback("birdappclose");
    else if (!Settings::LunaSettings()->lunaSystemSoundAppClose.empty())
        SoundPlayerPool::instance()->playFeedback(Settings::LunaSettings()->lunaSystemSoundAppClose);
}

void CardWindowManager::queueFocusAction(CardWindow* win, bool focused)
{
	if (win->removed())
		return;

	win->aboutToFocusEvent(focused);
	win->queueFocusAction(focused);
	if (!m_pendingActionWinSet.contains(win))
		m_pendingActionWinSet.insert(win);
}

void CardWindowManager::performPendingFocusActions()
{
	Q_FOREACH(CardWindow* card, m_pendingActionWinSet) {
		card->performPendingFocusAction();
	}
	m_pendingActionWinSet.clear();
}

void CardWindowManager::queueTouchToShareAction(CardWindow* win)
{
	if (win->removed())
		return;
	if (!m_pendingTouchToShareWinSet.contains(win))
		m_pendingTouchToShareWinSet.insert(win);
}

void CardWindowManager::performPendingTouchToShareActions()
{
    Q_FOREACH(CardWindow* card, m_pendingTouchToShareWinSet) {

        GhostCard* ghost = card->createGhost();
        if (ghost) {

            // place the ghost on top of the card we're sharing
            ghost->setParentItem(card->parentItem());
            ghost->stackBefore(card);
            card->stackBefore(ghost);

            // anchor the ghost within it's parent's group
            CardGroup* group = card->cardGroup();
            ghost->setPos((group ? group->pos() : QPointF(0,0)));
            ghost->setOpacity(0.5);

            QPropertyAnimation* anim = new QPropertyAnimation(ghost, "position");
            anim->setDuration(AS(cardGhostDuration));
            anim->setEasingCurve(AS_CURVE(cardGhostCurve));

            // animate the ghost off the top of the screen
            CardWindow::Position pos;
            QRectF r = ghost->mapRectToParent(ghost->boundingRect());
            qreal offTop = boundingRect().y() - (ghost->y() + (r.height()/2));
            pos.trans.setY(offTop);
            pos.trans.setZ(Settings::LunaSettings()->ghostCardFinalRatio);
            QVariant end; end.setValue(pos);
            anim->setEndValue(end);
            connect(anim, SIGNAL(finished()), SLOT(slotTouchToShareAnimationFinished()));
            anim->start();
        }
    }
    m_pendingTouchToShareWinSet.clear();
}

void CardWindowManager::removePendingActionWindow(CardWindow* win)
{
	QSet<CardWindow*>::iterator it = m_pendingActionWinSet.find(win);
	if (it != m_pendingActionWinSet.end())
		m_pendingActionWinSet.erase(it);

    it = m_pendingTouchToShareWinSet.find(win);
    if (it != m_pendingTouchToShareWinSet.end())
        m_pendingTouchToShareWinSet.erase(it);
}

CardWindow* CardWindowManager::activeWindow() const
{
	CardWindow* activeCard = NULL;
	CardWindow* w = NULL;

	if((NULL == m_activeGroup) || (NULL == (activeCard = m_activeGroup->activeCard()))) {
		return NULL;
	}
	else {
		w = activeCard->getModalChild();
		return( NULL != w)? w: activeCard;
	}
}

CardGroup* CardWindowManager::activeGroup() const
{
	return m_activeGroup;
}

void CardWindowManager::slotAnimationsFinished()
{
	if(false == m_addingModalWindow) {
		if (m_anims.animationCount() == 0) {
			return;
		}
	}

	m_cardAnimMap.clear();
	m_groupAnimMap.clear();
	m_anims.clear();

	// make sure the active group stays at the top of the stacking order
	if (m_activeGroup) {
		m_activeGroup->raiseCards();
	}

	performPendingFocusActions();
	m_curState->animationsFinished();

	m_animationsActive = false;
	updateAllowWindowUpdates();
}

void CardWindowManager::slotDeletedAnimationFinished()
{
	QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
	Q_ASSERT(anim != 0);

	// find the card whose animation finished and delete it if webkit already told us we can
	QMap<CardWindow*,QPropertyAnimation*>::iterator it = m_deletedAnimMap.begin();
	for (; it != m_deletedAnimMap.end(); ++it) {

		QPropertyAnimation* a = qobject_cast<QPropertyAnimation*>(it.value());
		if (anim == a) {
			CardWindow* w = static_cast<CardWindow*>(it.key());
			if (w->removed()) {
				m_deletedAnimMap.erase(it);
				delete w;
				delete a;
			}
            else {
                // since we don't adjust these when ui orientation changes, make sure they remain
                // invisible until the reaper comes to collect them
                w->setVisible(false);
            }
			break;
		}
	}
}

void CardWindowManager::slotTouchToShareAnimationFinished()
{
    g_debug("%s: deleting TapToShare Ghost", __PRETTY_FUNCTION__);

    QPropertyAnimation* anim = qobject_cast<QPropertyAnimation*>(sender());
    Q_ASSERT(anim != 0);
    GhostCard* target = static_cast<GhostCard*>(anim->targetObject());
    delete anim;
    if (target) {
        delete target;
    }
}

void CardWindowManager::slotLauncherVisible(bool val, bool fullyVisible)
{
	if (fullyVisible && !m_lowResMode) {
		m_lowResMode = true;
		Q_FOREACH(CardGroup* group, m_groups) {
			group->disableShadows();
		}
	}
	else if (!fullyVisible && m_lowResMode) {
		m_lowResMode = false;
		Q_FOREACH(CardGroup* group, m_groups) {
			group->enableShadows();
		}
	}
}

void CardWindowManager::slotLauncherShown(bool val)
{
	if (!val)
		return;

	if (m_curState && m_curState->supportLauncherOverlay())
		return;

	minimizeActiveWindow();
}

void CardWindowManager::slotChangeCardWindow(bool next)
{
	if (m_curState)
		m_curState->changeCardWindow(next);
}

void CardWindowManager::slotFocusMaximizedCardWindow(bool focus)
{
	if (m_curState)
		m_curState->focusMaximizedCardWindow(focus);
}

void CardWindowManager::slotTouchToShareAppUrlTransfered(const std::string& appId)
{
    if (m_curState)
        m_curState->processTouchToShareTransfer(appId);
}

void CardWindowManager::slotDismissActiveModalWindow()
{
	if(Window::Type_ModalChildWindowCard == activeWindow()->type()) {
		m_modalWindowState = ModalWindowDismissedInternally;
		notifySysControllerOfModalStatus(SystemUiController::ServiceDismissedModalCard, true, ModalDismissAnimate);
	}
	else {
		resetModalFlags();
	}
}

void CardWindowManager::slotDismissModalTimerStopped()
{
	CardWindow* activeWin = activeWindow();
	if(activeWin && (Window::Type_ModalChildWindowCard == activeWin->type()) && m_parentOfModalCard) {
		m_parentOfModalCard->setModalAcceptInputState(CardWindow::ModalLaunchedAcceptingInput);
	}
}

void CardWindowManager::setInModeAnimation(bool animating)
{
	if (animating) {
		Q_FOREACH(CardGroup* group, m_groups) {
			group->setCompositionMode(QPainter::CompositionMode_SourceOver);
		}
	} else {
		Q_FOREACH(CardGroup* group, m_groups) {
			group->setCompositionMode(QPainter::CompositionMode_Source);
		}
	}
}

void CardWindowManager::notifySysControllerOfModalStatus(int reason, bool restore, NotifySystemUiControllerAction type, Window* win)
{
	if(type == Invalid)
		return;

	if(type == ModalLaunch) {
		// Notify SystemUiController of the result of the modal launch
		SystemUiController::instance()->setModalWindowLaunchErrReason((SystemUiController::ModalWinLaunchErrorReason)(reason));

		// Signal that we no longer have an active modal window only if the reason is NoErr, else remove the window
		if(SystemUiController::NoErr == ((SystemUiController::ModalWinLaunchErrorReason)(reason))) {
			SystemUiController::instance()->notifyModalWindowActivated(m_parentOfModalCard);
		}
		else {
			// If we are already in the process of deleting a modal and another call comes in to do the same, just ignore it
			if(true == m_modalDismissInProgress) {
				return;
			}
			// we are going to delete a modal.
			m_modalDismissInProgress = true;
			performPostModalWindowRemovedActions(win, restore);
		}

		return;
	}

	// If we are already in the process of deleting a modal and another call comes in to do the same, just ignore it
	if(true == m_modalDismissInProgress) {
		return;
	}

	// we are going to delete a modal.
	m_modalDismissInProgress = true;

	// Notify SystemUiController of the reason why the modal was dismissed
	SystemUiController::instance()->setModalWindowDismissErrReason((SystemUiController::ModalWinDismissErrorReason)(reason));

	if(type == ModalDismissAnimate) {
		// We need to animate the removal of the modal. So call this function, which will call performPostModalWindowRemovedActions(restore) with the correct params
		initiateRemovalOfActiveModalWindow();
	}
	else {
		// directly get rid of the modal card
		performPostModalWindowRemovedActions(NULL, restore);
	}
}

void CardWindowManager::updateAllowWindowUpdates()
{
	bool allow = true;

	if (m_animationsActive)
		allow = false;
	else if ((m_curState != m_maximizeState) && m_penDown)
		allow = false;
	
	Q_FOREACH(CardGroup* cg, m_groups) {
		Q_FOREACH(CardWindow* w, cg->cards())
			w->allowUpdates(allow);
	}
}
