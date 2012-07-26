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

#include "CardWindowManagerStates.h"
#include "CardWindowManager.h"
#include "CardWindow.h"
#include "CardGroup.h"
#include "SystemUiController.h"
#include "SystemService.h"
#include "Settings.h"
#include "SoundPlayerPool.h"
#include "Time.h"

#include <QGestureEvent>
#include <QTapGesture>
#include <QTapAndHoldGesture>

CardWindowManagerState::CardWindowManagerState(CardWindowManager* wm)
	: m_wm(wm)
{
}

void CardWindowManagerState::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	event->accept();
}

void CardWindowManagerState::windowAdded(CardWindow* win)
{
	win->stopLoadingOverlay();
}

void CardWindowManagerState::windowRemoved(CardWindow* win)
{
	if(Window::Type_ModalChildWindowCard == win->type())
		return;

	m_wm->removeCardFromGroup(win);
}

void CardWindowManagerState::focusMaximizedCardWindow(bool focus)
{
    if (!focus)
	    m_wm->minimizeActiveWindow();
}

bool CardWindowManagerState::supportLauncherOverlay() const
{
	return true;
}

void CardWindowManagerState::onEntry(QEvent* event)
{
	//qDebug() << "@@@@@ Entering State" << objectName();
	m_wm->setCurrentState(this);
}

bool CardWindowManagerState::lastWindowAddedType() const
{
	return m_wm->isLastWindowAddedModal();
}

void CardWindowManagerState::resizeWindow(CardWindow* w, int width, int height)
{
	if (w->allowResizeOnPositiveSpaceChange()) {
		if(w->type() == Window::Type_ModalChildWindowCard) {
			w->resizeEvent(Settings::LunaSettings()->modalWindowWidth, Settings::LunaSettings()->modalWindowHeight);
		}
		else {
			w->resizeEvent(width, height);
		}
	}
	else {
		w->adjustForPositiveSpaceSize(width, height);
	}
}

// --------------------------------------------------------------------------------------------------

void MinimizeState::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    CardWindowManagerState::mousePressEvent(event);

    m_wm->handleMousePressMinimized(event);
}

void MinimizeState::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	m_wm->handleMouseMoveMinimized(event);
}

void MinimizeState::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	m_wm->handleMouseReleaseMinimized(event);
}

void MinimizeState::flickGestureEvent(QGestureEvent* event)
{
	m_wm->handleFlickGestureMinimized(event);
}

void MinimizeState::tapGestureEvent(QTapGesture* event)
{
	m_wm->handleTapGestureMinimized(event);
}

void MinimizeState::tapAndHoldGestureEvent(QTapAndHoldGesture* event)
{
	m_wm->handleTapAndHoldGestureMinimized(event);
}

bool MinimizeState::handleKeyNavigation(QKeyEvent* keyEvent)
{
    m_wm->handleKeyNavigationMinimized(keyEvent);
    return true;
}

void MinimizeState::animationsFinished()
{
    // attempt to process any tap-to-share actions
    m_wm->performPendingTouchToShareActions();

	// attempt to auto restore a card to maximized
	m_wm->restoreCardToMaximized();
}

void MinimizeState::relayout(const QRectF& r, bool animate)
{
	Q_UNUSED(r);
	m_wm->minimizeActiveWindow(animate);
}

void MinimizeState::changeCardWindow(bool next)
{
	if (next)
		m_wm->switchToPrevApp();
	else
		m_wm->switchToNextApp();
}

void MinimizeState::onEntry(QEvent* event)
{
	CardWindowManagerState::onEntry(event);

	CardWindow* activeWin = m_wm->activeWindow();
	if (activeWin) {
		m_wm->queueFocusAction(activeWin, false);
		QRectF boundingRect = m_wm->normalOrScreenBounds(0);
		activeWin->setBoundingRect(boundingRect.width(), boundingRect.height());
		m_wm->firstCardAlert();
	}
	SystemUiController::instance()->setCardWindowMaximized(false);
	SystemUiController::instance()->setMaximizedCardWindow(0);
}

// --------------------------------------------------------------------------------------------------

MaximizeState::MaximizeState(CardWindowManager* wm)
	: CardWindowManagerState(wm)
	, m_exiting(false)
	, m_disableDirectRendering(0)
{
	setObjectName("Maximize");

	// react to incoming phone calls while maximized
	QSignalTransition* trans = new QSignalTransition(SystemService::instance(), 
		SIGNAL(signalIncomingPhoneCall()));
	addTransition(trans);
	connect(trans, SIGNAL(triggered()), SLOT(slotIncomingPhoneCall()));
}

void MaximizeState::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	// CardWindow should always accept but just in case
	event->ignore();
}

void MaximizeState::windowAdded(CardWindow* win)
{
	CardWindowManagerState::windowAdded(win);
	// if the active window is already sitting/maximized, complete maximization setup
	if (win == m_wm->activeWindow() && Window::Type_ModalChildWindowCard != win->type() && !m_wm->windowHasAnimation(win))
		finishMaximizingActiveWindow();
}

void MaximizeState::windowRemoved(CardWindow* win)
{
	if(Window::Type_ModalChildWindowCard == win->type())
		return;

	m_wm->removeCardFromGroupMaximized(win);
}

void MaximizeState::positiveSpaceAboutToChange(const QRect& r, bool fullScreen)
{
	CardWindow* parent = NULL;
	CardWindow* activeWin = m_wm->activeWindow();
	if (activeWin) {
		const QRect& positiveSpace = SystemUiController::instance()->positiveSpaceBounds();
		bool expanding = positiveSpace.height() < r.height();

		if(Window::Type_ModalChildWindowCard == activeWin->type()) {
			// Pass on the info to the parent as well.
			parent = m_wm->modalParent();
			if(parent) {
				parent->positiveSpaceAboutToChange(r, fullScreen);
			}

			// let the child know that +ve space change is about to change.
			activeWin->positiveSpaceAboutToChange(r, fullScreen);
		}
		else {
			activeWin->positiveSpaceAboutToChange(r, fullScreen);
		}

		if (!m_exiting) {

        	// Send this out ONLY to the parent, but not the modal card.
    		if (expanding) {
    			if(Window::Type_ModalChildWindowCard == activeWin->type() && NULL != parent) {
    				parent->resizeEventSync(r.width(), r.height());
    			}
    			else {
    				activeWin->resizeEventSync(r.width(), r.height());
    			}
	    	}

			// adjust the maximized window if the positive space top is
    		// moving and it isn't due to exiting this state
	    	if (positiveSpace.y() != r.y()) {
		    	m_wm->maximizeActiveWindow();
    		}
        }
	}
}

void MaximizeState::positiveSpaceChangeFinished(const QRect& r)
{
	CardWindow* activeWin = m_wm->activeWindow();
	if (activeWin) {
		if(Window::Type_ModalChildWindowCard == activeWin->type()) {
			// Pass on the info to the parent as well.
			CardWindow* parent = m_wm->modalParent();
			if(parent) {
				parent->positiveSpaceChangeFinished(r);
				resizeWindow(parent, r.width(), r.height());
			}

			// let the child know that +ve space change is finished.
			activeWin->positiveSpaceChangeFinished(r);
		}
		else {
			activeWin->positiveSpaceChangeFinished(r);
			resizeWindow(activeWin, r.width(), r.height());
		}
	}
}

void MaximizeState::positiveSpaceChanged(const QRect& r)
{
	CardWindow* activeWin = m_wm->activeWindow();
	if (activeWin) {
		if(Window::Type_ModalChildWindowCard == activeWin->type()) {

			// Pass on the info to the parent as well.
			CardWindow* parent = m_wm->modalParent();
			if(parent) {
				parent->positiveSpaceChanged(r);
			}

			// let the child know that +ve space change is changed.
			activeWin->positiveSpaceChanged(r);
		}
		else {
			activeWin->positiveSpaceChanged(r);
		}
	}    
}

void MaximizeState::animationsFinished()
{
	finishMaximizingActiveWindow();
	QRectF boundingRect;

	CardWindow* activeWin = m_wm->activeWindow();
	if (activeWin) {
		if(Window::Type_ModalChildWindowCard != activeWin->type()) {
			boundingRect = m_wm->normalOrScreenBounds(activeWin);
			activeWin->setBoundingRect(boundingRect.width(), boundingRect.height());
		}
		else {
			activeWin->setBoundingRect(Settings::LunaSettings()->modalWindowWidth, Settings::LunaSettings()->modalWindowHeight);

			// Notify the parent.
			CardWindow* parent = m_wm->modalParent();
			if(parent) {
				boundingRect = m_wm->normalOrScreenBounds(parent);
				parent->setBoundingRect(boundingRect.width(), boundingRect.height());
			}
		}
	}
}

void MaximizeState::finishMaximizingActiveWindow()
{
	// set the active windows' size to the positive space

	CardWindow* activeWin = m_wm->activeWindow();

        if(activeWin){
            activeWin->setMaxAndLoading(1);
        }
	if (activeWin && activeWin->addedToWindowManager()) {
                // allow direct rendering if no one has requested it to be disabled - Do this only if the window is NOT a modal window
		if(Window::Type_ModalChildWindowCard != activeWin->type()) {
			const QRect& r = m_wm->targetPositiveSpace();
            resizeWindow(activeWin, r.width(), r.height());

			if (m_disableDirectRendering == 0) {
				if(false == m_wm->isModalDismissed()) {
					SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::CARD_WINDOW_MANAGER, activeWin, true);
				}
				else {
					m_wm->setModalDismissed(false);
				}
			}
		}

		if (G_UNLIKELY(Settings::LunaSettings()->perfTesting)) {
			// NOV-97107, hook for automated performance testing
			g_message("SYSMGR PERF: STATE MAXIMIZED appid: %s, processid: %s, type: %s, time: %d",
				activeWin->appId().c_str(),
				activeWin->processId().c_str(),
				activeWin->isHost() ? "host" : "card",
				Time::curTimeMs());
		}
	}
}

void MaximizeState::focusMaximizedCardWindow(bool focus)
{
	CardWindow* activeWin = m_wm->activeWindow();

	m_disableDirectRendering = qMax((focus ? m_disableDirectRendering-1 : m_disableDirectRendering+1), 0);

   // take focus away?
    if (!focus && m_disableDirectRendering == 1) {

        // disable direct rendering so the emergency windows can render. [This is applicable ONLY if the active window is not a modal window]
    	if(activeWin->type() != Window::Type_ModalChildWindowCard) {
    		SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::CARD_WINDOW_MANAGER, activeWin, false);
    	}

    	// clear focus
        activeWin->aboutToFocusEvent(false);
        activeWin->queueFocusAction(false);
        activeWin->performPendingFocusAction();
    }
    else if (focus && m_disableDirectRendering == 0) {

        // make sure the active window is sized properly and re-enables
        // direct-rendering
       if (!m_wm->windowHasAnimation(activeWin)) {

            // re-enable direct rendering or just wait for any animations to finish [This is applicable ONLY if the active window is not a modal window]
        	if(activeWin->type() != Window::Type_ModalChildWindowCard) {
        		SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::CARD_WINDOW_MANAGER, activeWin, true);
        	}

            // refocus window
            activeWin->aboutToFocusEvent(true);
            activeWin->queueFocusAction(true);
            activeWin->performPendingFocusAction();
        } else {
            // queue refocus event for the window
        	m_wm->queueFocusAction(activeWin, true);
        }
    }
}

void MaximizeState::changeCardWindow(bool next)
{
	if (next)
		m_wm->switchToPrevAppMaximized();
	else
		m_wm->switchToNextAppMaximized();
}

void MaximizeState::relayout(const QRectF& r, bool animate)
{
	Q_UNUSED(r);
	m_wm->maximizeActiveWindow(animate);
	QRectF boundingRect;

	CardWindow* activeWin = m_wm->activeWindow();
	if(activeWin) {
		if(Window::Type_ModalChildWindowCard != activeWin->type()) {
			boundingRect = m_wm->normalOrScreenBounds(activeWin);
			activeWin->setBoundingRect(boundingRect.width(), boundingRect.height());
		}
		else {
			// forward the call to the child.
			activeWin->setBoundingRect(Settings::LunaSettings()->modalWindowWidth, activeWin->boundingRect().height());

			// Notify the parent as well.
			CardWindow* parent = m_wm->modalParent();
			if(parent) {
				boundingRect = m_wm->normalOrScreenBounds(parent);
				parent->setBoundingRect(boundingRect.width(), boundingRect.height());
			}
		}
	}
}

void MaximizeState::slotIncomingPhoneCall()
{
	CardWindow* activeWin = m_wm->activeWindow();
	if (activeWin && activeWin->supportsPause()) {
		activeWin->pause();
	}
}

void MaximizeState::processTouchToShareTransfer(const std::string& appId)
{
    CardWindow* activeWindow = m_wm->activeWindow();
    if (activeWindow && activeWindow->appId() == appId) {
        // looks like a match, queue up an animation and minimize the app
        m_wm->queueTouchToShareAction(activeWindow);
        m_wm->minimizeActiveWindow();
    }
}

void MaximizeState::onEntry(QEvent* event)
{
	CardWindowManagerState::onEntry(event);
	CardWindow* activeWin = m_wm->activeWindow();
	Q_ASSERT(activeWin != 0);

	SystemUiController::instance()->setCardWindowMaximized(true);
	SystemUiController::instance()->setMaximizedCardWindow(activeWin);

	m_wm->queueFocusAction(activeWin, true);
	activeWin->setAttachedToGroup(false);

	if(Window::Type_ModalChildWindowCard != activeWin->type()) {
		QRectF boundingRect = m_wm->normalOrScreenBounds(activeWin);
		activeWin->setBoundingRect(boundingRect.width(), boundingRect.height());
	}
	else {
		activeWin->setBoundingRect(Settings::LunaSettings()->modalWindowWidth, Settings::LunaSettings()->modalWindowHeight);
		//activeWin->setMaximized(true);
	}
}

void MaximizeState::onExit(QEvent* event)
{
	CardWindowManagerState::onExit(event);
	CardWindow* activeWin = m_wm->activeWindow();
	m_exiting = true;

	if(false == m_wm->isLastWindowAddedModal()) {
		if (activeWin) {
                    activeWin->setMaxAndLoading(0);
			QRect r = m_wm->normalOrScreenBounds(0);
			resizeWindow(activeWin, r.width(), r.height());

			if (!activeWin->removed())
				m_wm->queueFocusAction(activeWin, false);

			SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::CARD_WINDOW_MANAGER, activeWin, false);
			activeWin->setAttachedToGroup(true);
		}

		// notify the system that we are no longer maximized
		SystemUiController::instance()->setCardWindowMaximized(false);
		SystemUiController::instance()->setMaximizedCardWindow(0);

		m_exiting = false;
		m_disableDirectRendering = 0;
	}
}


// --------------------------------------------------------------------------------------------------

void FocusState::animationsFinished()
{
	m_wm->maximizeActiveWindow();
}

void FocusState::onEntry(QEvent* event)
{
	CardWindowManagerState::onEntry(event);

	CardWindow* win = 0;
	if (event->type() == QEvent::StateMachineSignal) {
		QStateMachine::SignalEvent* se = static_cast<QStateMachine::SignalEvent*>(event);
		win = se->arguments().at(0).value<CardWindow*>();
	}

	CardGroup* activeGroup = m_wm->activeGroup();
	Q_ASSERT(activeGroup != 0);
	if (activeGroup == win->cardGroup()) {
		// update the active card within the active group and maximize it
		activeGroup->setActiveCard(win);
		m_wm->maximizeActiveWindow();
	}
	else {
		// make this card the active card in its group and focus the group
		win->cardGroup()->setActiveCard(win);
		m_wm->setActiveGroup(win->cardGroup());
		m_wm->slideAllGroups();
	}
}

MaximizeToFocusTransition::MaximizeToFocusTransition(CardWindowManager* wm, QState* target)
	: QSignalTransition(wm, SIGNAL(signalFocusWindow(CardWindow*)))
{
	setTargetState(target);
}

bool MaximizeToFocusTransition::eventTest(QEvent* event)
{
	if (!QSignalTransition::eventTest(event))
		return false;

	QStateMachine::SignalEvent* se = static_cast<QStateMachine::SignalEvent*>(event);
	CardWindow* win = se->arguments().at(0).value<CardWindow*>();
	CardWindowManager* wm = static_cast<CardWindowManager*>(senderObject());
	if (win == wm->activeWindow()) {
		// window is already focused
		SystemUiController::instance()->setCardWindowMaximized(true);
		return false;
	}

	return true;
}

// --------------------------------------------------------------------------------------------------
void PreparingState::onEntry(QEvent* event)
{
	CardWindowManagerState::onEntry(event);

	// unpack the window which we need to prepare
	CardWindow* win = 0;
	if (event->type() == QEvent::StateMachineSignal) {
		QStateMachine::SignalEvent* se = static_cast<QStateMachine::SignalEvent*>(event);
		win = se->arguments().at(0).value<CardWindow*>();

		if (win) {
			QRectF boundingRect = m_wm->normalOrScreenBounds(0);
			if(Window::Type_ModalChildWindowCard != win->type())
				win->setBoundingRect(boundingRect.width(), boundingRect.height());
			else {
				win->setBoundingRect(Settings::LunaSettings()->modalWindowWidth, Settings::LunaSettings()->modalWindowHeight);
			}
		}
	}

	m_wm->prepareAddWindowSibling(win);
}

void PreparingState::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	event->accept();

	m_wm->minimizeActiveWindow();
}

void PreparingState::windowAdded(CardWindow* win)
{
	CardWindow* activeWin = m_wm->activeWindow();
	if (win != activeWin) {
		CardWindowManagerState::windowAdded(win);
		return;
	}

	activeWin->stopLoadingOverlay();
	m_wm->maximizeActiveWindow(!lastWindowAddedType());

	if (!Settings::LunaSettings()->lunaSystemSoundAppOpen.empty())
		SoundPlayerPool::instance()->playFeedback(Settings::LunaSettings()->lunaSystemSoundAppOpen);
}

void PreparingState::windowTimedOut(CardWindow* win)
{
	CardWindow* activeWin = m_wm->activeWindow();
	if (win == activeWin) {
		m_wm->addWindowTimedOutNormal(win);
	}
}

bool PreparingState::supportLauncherOverlay() const
{
	return false;
}

// --------------------------------------------------------------------------------------------------

void LoadingState::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	event->accept();

	m_wm->minimizeActiveWindow();
}

void LoadingState::windowAdded(CardWindow* win)
{
	CardWindow* activeWin = m_wm->activeWindow();
	if (activeWin != win) {
		CardWindowManagerState::windowAdded(win);
		return;
	}

	if (!m_wm->windowHasAnimation(win)) {
		m_wm->maximizeActiveWindow(!lastWindowAddedType());
	}
}

void LoadingState::animationsFinished()
{
	CardWindow* activeWin = m_wm->activeWindow();
	if (activeWin && activeWin->addedToWindowManager()) {
		m_wm->maximizeActiveWindow(!lastWindowAddedType());
	}
}

bool LoadingState::supportLauncherOverlay() const
{
	return false;
}

void LoadingState::relayout(const QRect& r, bool animate)
{
    Q_UNUSED(r);
	m_wm->layoutAllGroups(false);
}

void LoadingState::onExit(QEvent* event)
{
    CardWindowManagerState::onExit(event);

	// in case we exit the Loading state during the maximizing animation
	CardWindow* activeWin = m_wm->activeWindow();
	if (activeWin && activeWin->addedToWindowManager()) {

		activeWin->stopLoadingOverlay();
	}
}

// --------------------------------------------------------------------------------------------------

ReorderGrid::ReorderGrid(QGraphicsItem* parent, int slice)
	: QGraphicsItem(parent)
{
	m_boundingRect = parent->boundingRect();
	m_slice = slice;
}

void ReorderGrid::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	QPen oldPen = painter->pen();
	QPen pen(Qt::magenta);

	painter->setPen(pen);
	
	qreal section = m_boundingRect.width() / m_slice;

	painter->drawLine(m_boundingRect.left() + section, m_boundingRect.top(),
					  m_boundingRect.left() + section, m_boundingRect.bottom());
	painter->drawLine(m_boundingRect.right() - section, m_boundingRect.top(),
					  m_boundingRect.right() - section, m_boundingRect.bottom());

	painter->setPen(oldPen);
}

void ReorderState::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	m_wm->handleMouseMoveReorder(event);
}

void ReorderState::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	m_wm->handleMouseReleaseReorder(event);
}

void ReorderState::animationsFinished()
{
	// attempt to cycle the card we are reordering to the left or right slot
	m_wm->cycleReorderSlot();
}

void ReorderState::onExit(QEvent* event)
{
	CardWindowManagerState::onExit(event);

	QStateMachine::SignalEvent* se = static_cast<QStateMachine::SignalEvent*>(event);
	if (se && !se->arguments().isEmpty()) {
		bool canceled = se->arguments().at(0).toBool();
		if (canceled)
			m_wm->cancelReorder();
	}

	//delete m_grid;
	SystemUiController::instance()->enterOrExitCardReorder(false);
}

void ReorderState::onEntry(QEvent* event)
{
	CardWindowManagerState::onEntry(event);

	QStateMachine::SignalEvent* se = static_cast<QStateMachine::SignalEvent*>(event);
	if (se && !se->arguments().isEmpty()) {
		QPoint pt = se->arguments().at(0).toPoint();		
		m_wm->enterReorder(pt);
		//int slice = se->arguments().at(1).toInt();
		//m_grid = new ReorderGrid(m_wm, slice);
	}

	SystemUiController::instance()->enterOrExitCardReorder(true);
}
