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

#include "DashboardWindowManagerStates.h"

#include <QStateMachine>
#include "AlertWindow.h"
#include "DashboardWindowManager.h"
#include "DashboardWindowContainer.h"
#include "GraphicsItemContainer.h"
#include "HostBase.h"
#include "Logging.h"
#include "Settings.h"
#include "SystemUiController.h"
#include "WindowServerLuna.h"

static SystemUiController* s_suc = 0;
static int s_closedTopPosition = 0; // relative to bottom edge of screen

DWMStateBase::DWMStateBase(DashboardWindowManager* wm, QState* parent)
	: QState(parent)
	, m_wm(wm)
{
	if (G_UNLIKELY(s_suc == 0)) {
		s_suc = SystemUiController::instance();
		s_closedTopPosition = Settings::LunaSettings()->positiveSpaceBottomPadding;
	}
}

void DWMStateBase::onEntry(QEvent* e)
{
	g_message("State: %s\n", qPrintable(objectName()));
	m_wm->currentStateChanged(this);
	QState::onEntry(e);    
}

void DWMStateBase::negativeSpaceAnimationFinished()
{
	Q_EMIT signalNegativeSpaceAnimationFinished();
}

// --------------------------------------------------------------------


DWMStateAlertOpen::DWMStateAlertOpen(DashboardWindowManager* wm, QState* parent)
	: DWMStateBase(wm, parent)
{
	setObjectName("DWMStateAlertOpen");
}

DWMStateAlertOpen::~DWMStateAlertOpen()
{
    
}

void DWMStateAlertOpen::onEntry(QEvent* e)
{
	DWMStateBase::onEntry(e);

	AlertWindow* win = m_wm->topAlertWindow();
	luna_assert(win != 0);
	
	QRectF bounds = win->boundingRect();
	int width = (true == m_wm->isOverlay())?DashboardWindowManager::sTabletUiWidth():SystemUiController::instance()->currentUiWidth();

	if (bounds.width() != width ||
		bounds.height() != win->initialHeight()) {
		if(!m_wm->isOverlay() ||(m_wm->isOverlay() && (m_wm->getWindowFadeOption() != DashboardWindowManager::FadeInAndOut)))
			win->resizeEventSync(width, win->initialHeight());
	}

	if(!m_wm->isOverlay()) {
		win->show();
		win->activate();
	}
	else {
		m_wm->animateAlertWindow();
		s_suc->setAlertVisible(true);
	}

	m_wm->raiseAlertWindow(win);

	if (!m_wm->isOverlay()) {
		int targetTop = SystemUiController::instance()->currentUiHeight() - bounds.height();
		s_suc->changeNegativeSpace(targetTop, true);
		s_suc->setDashboardOpened(true, false);
	}

	m_wm->notifyActiveAlertWindowActivated(win);
}

void DWMStateAlertOpen::onExit(QEvent* e)
{
	if(m_wm->isOverlay()) {
		s_suc->setAlertVisible(false);
	}

	AlertWindow* win = m_wm->topAlertWindow();
	if (!win)
		return;

	if (e->type() != QEvent::StateMachineSignal)
		return;
		
	QStateMachine::SignalEvent *se = static_cast<QStateMachine::SignalEvent*>(e);
	QList<QVariant> args = se->arguments();
	if (args.empty())
		return;

	if (args.at(0).toBool() == true) {
		// This was force closed.
		m_wm->hideOrCloseAlertWindow(win);
	}
}

// --------------------------------------------------------------------

DWMStateClosed::DWMStateClosed(DashboardWindowManager* wm, QState* parent)
	: DWMStateBase(wm, parent)
{
	setObjectName("DWMStateClosed");
}

DWMStateClosed::~DWMStateClosed()
{    
}

void DWMStateClosed::dashboardContentAdded()
{
    adjustNegativeSpace();    
}

void DWMStateClosed::dashboardContentRemoved()
{
    adjustNegativeSpace();
}

void DWMStateClosed::onEntry(QEvent* e)
{
	DWMStateBase::onEntry(e);

	m_wm->raiseBackgroundWindow();
	adjustNegativeSpace();
}

void DWMStateClosed::adjustNegativeSpace()
{
	if (!m_wm->isOverlay()) {

		int targetTop = SystemUiController::instance()->currentUiHeight();
		if (m_wm->hasDashboardContent()) {
			g_message("%s: dashboard has content. Window Container empty: %d",
					  __PRETTY_FUNCTION__, m_wm->dashboardWindowContainer()->empty());
			targetTop -= s_closedTopPosition;
		}

		s_suc->changeNegativeSpace(targetTop, false);
	}
	s_suc->setDashboardOpened(false,true);
}

// --------------------------------------------------------------------

DWMStateOpen::DWMStateOpen(DashboardWindowManager* wm, QState* parent )
	: DWMStateBase(wm, parent)
{
	setObjectName("DWMStateOpen");    
}

DWMStateOpen::~DWMStateOpen()
{
    
}

void DWMStateOpen::onEntry(QEvent* e)
{
	DWMStateBase::onEntry(e);

	if(!m_wm->isOverlay())
		m_wm->dashboardWindowContainer()->setScrollBottom(0);
	else
		m_wm->dashboardWindowContainer()->layoutAllWindowsInMenu();

	m_wm->raiseDashboardWindowContainer();

	if (!m_wm->isOverlay()) {
		int targetTop = SystemUiController::instance()->currentUiHeight() - m_wm->dashboardWindowContainer()->viewportHeight();
		s_suc->changeNegativeSpace(targetTop, true);
	}
	s_suc->setDashboardOpened(true, true);

	m_wm->dashboardWindowContainer()->focusAllWindows(true);
}

void DWMStateOpen::onExit(QEvent* e)
{
	m_wm->dashboardWindowContainer()->focusAllWindows(false);
}

void DWMStateOpen::dashboardViewportHeightChanged()
{
	if (m_wm->isOverlay())
		return;

	int targetTop = SystemUiController::instance()->currentUiHeight() - m_wm->dashboardWindowContainer()->viewportHeight();
	s_suc->changeNegativeSpace(targetTop, true);
}

void DWMStateOpen::handleUiResizeEvent()
{
	if (!m_wm->isOverlay()) {

		int targetTop = SystemUiController::instance()->currentUiHeight() - m_wm->dashboardWindowContainer()->viewportHeight();
		s_suc->changeNegativeSpace(targetTop, true);
	}

	if(!m_wm->isOverlay())
		m_wm->dashboardWindowContainer()->setScrollBottom(0);
	else
		m_wm->dashboardWindowContainer()->layoutAllWindowsInMenu();
}

// --------------------------------------------------------------------------------------

DWMTransitionClosedToAlertOpen::DWMTransitionClosedToAlertOpen(DashboardWindowManager* wm,
															   QState* target, QObject* sender,
															   const char* signal)
	: QSignalTransition(sender, signal)
	, m_wm(wm)
{
	setTargetState(target);
}

bool DWMTransitionClosedToAlertOpen::eventTest(QEvent *e)
{
	if (!QSignalTransition::eventTest(e))
		return false;

	AlertWindow* win = m_wm->topAlertWindow();
	if (win)
		return true;

	return false;    
}
