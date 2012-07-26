/* @@@LICENSE
*
*      Copyright (c) 2011-2012 Hewlett-Packard Development Company, L.P.
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




#include "pagemovement.h"
#include "dimensionsglobal.h"
#include "dimensionslauncher.h"
#include "dynamicssettings.h"

#include <QDebug>

const char * PageMovementControl::PageMovementFSMPropertyName_isPageLeftRestricted =  "isPageLeftRestricted";
const char * PageMovementControl::PageMovementFSMPropertyName_isPageRightRestricted = "isPageRightRestricted";

PageMovementControl::PageMovementControl(LauncherObject * p_launcher)
: QStateMachine((QObject *)p_launcher)
{
	m_leftTimeout.setSingleShot(true);
	m_rightTimeout.setSingleShot(true);

	connect(&m_leftTimeout,SIGNAL(timeout()),
			this,SIGNAL(signalPageMovementFSM_LeftTimeout()));
	connect(&m_rightTimeout,SIGNAL(timeout()),
			this,SIGNAL(signalPageMovementFSM_RightTimeout()));
	connectLauncherSignals();
	setupPageMovementFSM();
	start();
}

void PageMovementControl::setupPageMovementFSM()
{
	QSignalTransition * pTransition = 0;
	this->setObjectName("pagemovecontrolfsm");
	m_p_pageMovementFSMStateNoRestriction = DimensionsGlobal::createFSMState("no_restriction",this);
	m_p_pageMovementFSMStateRestrictLeft = DimensionsGlobal::createFSMState("restrict_left",this);
	m_p_pageMovementFSMStateRestrictRight = DimensionsGlobal::createFSMState("restrict_right",this);
	m_p_pageMovementFSMStateRestrictLR = DimensionsGlobal::createFSMState("restrict_lr",this);

	// ------------------- STATE: no_restriction -----------------------------------
	//	no_restriction PROPERTIES
	m_p_pageMovementFSMStateNoRestriction->assignProperty(this,PageMovementFSMPropertyName_isPageLeftRestricted, false);
	m_p_pageMovementFSMStateNoRestriction->assignProperty(this,PageMovementFSMPropertyName_isPageRightRestricted, false);

	m_p_pageMovementFSMStateNoRestriction->addTransition(this,SIGNAL(signalPageMovementFSM_MovedPageLeft()),m_p_pageMovementFSMStateRestrictLeft);
	m_p_pageMovementFSMStateNoRestriction->addTransition(this,SIGNAL(signalPageMovementFSM_MovedPageRight()),m_p_pageMovementFSMStateRestrictRight);

	//  no_restriction SIDE-EFFECTS
	connect(m_p_pageMovementFSMStateNoRestriction, SIGNAL(propertiesAssigned()), SLOT(slotStopResetTimers()));
//	connect(m_p_pageMovementFSMStateNoRestriction, SIGNAL(propertiesAssigned()), SLOT(dbgSlotPrint()));

	// ------------------- STATE: restrict_left -----------------------------------
	//	restrict_left PROPERTIES
	m_p_pageMovementFSMStateRestrictLeft->assignProperty(this,PageMovementFSMPropertyName_isPageLeftRestricted, true);
	m_p_pageMovementFSMStateRestrictLeft->assignProperty(this,PageMovementFSMPropertyName_isPageRightRestricted, false);
	//  restrict_left TRANSITIONS
	pTransition = m_p_pageMovementFSMStateRestrictLeft->addTransition(this,SIGNAL(signalPageMovementFSM_MovedPageRight()),m_p_pageMovementFSMStateRestrictRight);
	pTransition = m_p_pageMovementFSMStateRestrictLeft->addTransition(this,SIGNAL(signalPageMovementFSM_Reset()),m_p_pageMovementFSMStateNoRestriction);
	pTransition = m_p_pageMovementFSMStateRestrictLeft->addTransition(this,SIGNAL(signalPageMovementFSM_LeftTimeout()),m_p_pageMovementFSMStateNoRestriction);
	//  restrict_left SIDE-EFFECTS
	connect(m_p_pageMovementFSMStateRestrictLeft, SIGNAL(propertiesAssigned()), SLOT(slotRestartLeftTimer()));
//	connect(m_p_pageMovementFSMStateRestrictLeft, SIGNAL(propertiesAssigned()), SLOT(dbgSlotPrint()));

	// ------------------- STATE: restrict_right -----------------------------------
	//	restrict_right PROPERTIES
	m_p_pageMovementFSMStateRestrictRight->assignProperty(this,PageMovementFSMPropertyName_isPageLeftRestricted, false);
	m_p_pageMovementFSMStateRestrictRight->assignProperty(this,PageMovementFSMPropertyName_isPageRightRestricted, true);
	//  restrict_right TRANSITIONS
	pTransition = m_p_pageMovementFSMStateRestrictRight->addTransition(this,SIGNAL(signalPageMovementFSM_MovedPageLeft()),m_p_pageMovementFSMStateRestrictLeft);
	pTransition = m_p_pageMovementFSMStateRestrictRight->addTransition(this,SIGNAL(signalPageMovementFSM_Reset()),m_p_pageMovementFSMStateNoRestriction);
	pTransition = m_p_pageMovementFSMStateRestrictRight->addTransition(this,SIGNAL(signalPageMovementFSM_RightTimeout()),m_p_pageMovementFSMStateNoRestriction);
	//  restrict_right SIDE-EFFECTS
	connect(m_p_pageMovementFSMStateRestrictRight, SIGNAL(propertiesAssigned()), SLOT(slotRestartRightTimer()));
//	connect(m_p_pageMovementFSMStateRestrictRight, SIGNAL(propertiesAssigned()), SLOT(dbgSlotPrint()));

	// ------------------- STATE: restrict_lr -----------------------------------
	//	restrict_lr PROPERTIES
	m_p_pageMovementFSMStateRestrictLR->assignProperty(this,PageMovementFSMPropertyName_isPageLeftRestricted, true);
	m_p_pageMovementFSMStateRestrictLR->assignProperty(this,PageMovementFSMPropertyName_isPageRightRestricted, true);
	//  restrict_lr TRANSITIONS
	pTransition = m_p_pageMovementFSMStateRestrictLR->addTransition(this,SIGNAL(signalPageMovementFSM_Reset()),m_p_pageMovementFSMStateNoRestriction);
	//  restrict_lr SIDE-EFFECTS

//	connect(m_p_pageMovementFSMStateRestrictLR, SIGNAL(propertiesAssigned()), SLOT(dbgSlotPrint()));

	this->setInitialState(m_p_pageMovementFSMStateNoRestriction);
}

void PageMovementControl::connectLauncherSignals()
{
	LauncherObject * pLauncherOwner = qobject_cast<LauncherObject *>(parent());
	if (!pLauncherOwner)
	{
		return;
	}

	connect(pLauncherOwner,SIGNAL(signalPagePanLeft()),
			this,SIGNAL(signalPageMovementFSM_MovedPageLeft()),
			Qt::UniqueConnection
			);
	connect(pLauncherOwner,SIGNAL(signalPagePanRight()),
			this,SIGNAL(signalPageMovementFSM_MovedPageRight()),
			Qt::UniqueConnection
	);
	connect(pLauncherOwner,SIGNAL(signalPageMovementEnd()),
			this,SIGNAL(signalPageMovementFSM_Reset()),
			Qt::UniqueConnection
	);
}

bool PageMovementControl::isLeftPageMoveRestricted() const
{
	return (property(PageMovementFSMPropertyName_isPageLeftRestricted).toBool());
}

bool PageMovementControl::isRightPageMoveRestricted() const
{
	return (property(PageMovementFSMPropertyName_isPageRightRestricted).toBool());
}

void PageMovementControl::slotStopResetTimers()
{
	m_leftTimeout.stop();
	m_rightTimeout.stop();
}

void PageMovementControl::slotRestartLeftTimer()
{
	m_leftTimeout.stop();
	m_leftTimeout.start(DynamicsSettings::settings()->pagePanForIconMoveDelayMs);
}

void PageMovementControl::slotRestartRightTimer()
{
	m_rightTimeout.stop();
	m_rightTimeout.start(DynamicsSettings::settings()->pagePanForIconMoveDelayMs);
}

void PageMovementControl::dbgSlotPrint()
{
//	qDebug() << __FUNCTION__ << "++++++++++++++++++++++++++++++++++++++++ : s = " << sender()->objectName();
}
