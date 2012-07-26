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




#ifndef PAGEMOVEMENT_H_
#define PAGEMOVEMENT_H_

#include <QStateMachine>
#include <QTimer>

class LauncherObject;
class PageMovementControl : public QStateMachine
{
	Q_OBJECT
public:
	PageMovementControl(LauncherObject * p_launcher);
	bool isLeftPageMoveRestricted() const;
	bool isRightPageMoveRestricted() const;

	friend class LauncherObject;
Q_SIGNALS:

	void signalPageMovementFSM_MovedPageLeft();
	void signalPageMovementFSM_LeftTimeout();
	void signalPageMovementFSM_MovedPageRight();
	void signalPageMovementFSM_RightTimeout();
	void signalPageMovementFSM_Reset();	//moving ended

protected:

	void connectLauncherSignals();
	void setupPageMovementFSM();

protected:

	QState * m_p_pageMovementFSMStateNoRestriction;
	QState * m_p_pageMovementFSMStateRestrictLeft;
	QState * m_p_pageMovementFSMStateRestrictRight;
	QState * m_p_pageMovementFSMStateRestrictLR;

	static const char * PageMovementFSMPropertyName_isPageLeftRestricted;
	static const char * PageMovementFSMPropertyName_isPageRightRestricted;

	QTimer	m_leftTimeout;
	QTimer  m_rightTimeout;

protected Q_SLOTS:

	void slotStopResetTimers();
	void slotRestartLeftTimer();
	void slotRestartRightTimer();

	void dbgSlotPrint();

};

#endif /* PAGEMOVEMENT_H_ */
