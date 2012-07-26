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




#ifndef TOPLEVELWINDOWMANAGER_H
#define TOPLEVELWINDOWMANAGER_H

#include "Common.h"

#include "WindowManagerBase.h"
#include "Timer.h"
#include <QPropertyAnimation>


class Window;
class LockWindow;

class TopLevelWindowManager : public WindowManagerBase
{
	Q_OBJECT

public:

	TopLevelWindowManager(uint32_t maxWidth, uint32_t maxHeight);
	virtual ~TopLevelWindowManager();

	virtual void init();

	virtual void addWindow(Window* win);
	virtual void removeWindow(Window* win);

	virtual bool handleEvent(QEvent* event);

	bool okToResize();
	void resize(int width, int height);
	void delayDockModeLocking();
	void performDelayedLock();

    virtual bool handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate);
	
Q_SIGNALS:

	void signalScreenLockStatusChanged(bool locked);
	
private Q_SLOTS:

	void slotEnterBrickMode(bool mediaSyncMode);
	void slotExitBrickMode();
	void slotBrickSurfAnimationFinished();
	
	void slotScreenLocked(int state);
	
private:

	void lockScreen();
	void unlockScreen();

	void createBrickModeWindow();

    void msmEntryComplete();

	Window* m_brickWindow;
	QPixmap* m_brickSurf;
	QPropertyAnimation m_brickSurfAnimation;
	bool m_suspendedWebkitProcess;
	bool m_inBrickMode;

	LockWindow* m_lockedWindow;
};


#endif /* TOPLEVELWINDOWMANAGER_H */
