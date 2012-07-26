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




#ifndef DASHBOARDWINDOWMANAGERSTATES_H
#define DASHBOARDWINDOWMANAGERSTATES_H

#include "Common.h"

#include <QSignalTransition>
#include <QState>

class DashboardWindowManager;
class SystemUiController;

class DWMStateBase : public QState
{
	Q_OBJECT
	
public:

	DWMStateBase(DashboardWindowManager* wm, QState* parent=0);
	virtual ~DWMStateBase() {}

	virtual void dashboardContentAdded() {}
	virtual void dashboardContentRemoved() {}
	virtual void dashboardViewportHeightChanged() {}
	virtual void negativeSpaceAnimationFinished();
	virtual bool allowsSoftClose() const { return true; }
	virtual void handleUiResizeEvent() {}

Q_SIGNALS:

	void signalNegativeSpaceAnimationFinished();

protected:

	virtual void onEntry(QEvent* e);

	DashboardWindowManager* m_wm;
};

// --------------------------------------------------------------------

class DWMStateClosed : public DWMStateBase
{
public:

	DWMStateClosed(DashboardWindowManager* wm, QState* parent=0);
	virtual ~DWMStateClosed();

	virtual void dashboardContentAdded();
	virtual void dashboardContentRemoved();

protected:

	virtual void onEntry(QEvent* e);

private:

	void adjustNegativeSpace();
};

// --------------------------------------------------------------------

class DWMStateAlertOpen : public DWMStateBase
{
public:

	DWMStateAlertOpen(DashboardWindowManager* wm, QState* parent=0);
	virtual ~DWMStateAlertOpen();

	virtual bool allowsSoftClose() const { return false; }
	
protected:

	virtual void onEntry(QEvent* e);
	virtual void onExit(QEvent* e);
};

// --------------------------------------------------------------------

class DWMStateOpen : public DWMStateBase
{
public:

	DWMStateOpen(DashboardWindowManager* wm, QState* parent=0);
	virtual ~DWMStateOpen();

	virtual void dashboardViewportHeightChanged();
	virtual void handleUiResizeEvent();
		
protected:

	virtual void onEntry(QEvent* e);
	virtual void onExit(QEvent* e);
};

// --------------------------------------------------------------------

class DWMTransitionClosedToAlertOpen : public QSignalTransition
{
public:

	DWMTransitionClosedToAlertOpen(DashboardWindowManager* wm,
								   QState* target, QObject* sender,
								   const char* signal);
		  

protected:

	virtual bool eventTest(QEvent *e);

private:

	DashboardWindowManager* m_wm;
};

#endif /* DASHBOARDWINDOWMANAGERSTATES_H */
