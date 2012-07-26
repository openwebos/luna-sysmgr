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

#include <glib.h>
#include <string>

#include "HostBase.h"
#include "Settings.h"
#include "AnimationSettings.h"
#include "TopLevelWindowManager.h"
#include "Window.h"
#include "LockWindow.h"
#include "WindowServer.h"
#include "SystemService.h"
#include "SystemUiController.h"
#include "Logging.h"
#include "WebAppMgrProxy.h"

TopLevelWindowManager::TopLevelWindowManager(uint32_t maxWidth, uint32_t maxHeight)
	: WindowManagerBase(maxWidth, maxHeight)
	, m_brickWindow(0)
	, m_brickSurf(0)
	, m_suspendedWebkitProcess(false)
	, m_inBrickMode(false)
	, m_lockedWindow(0)
{
	setObjectName("TopLevelWindowManager");

	connect(SystemService::instance(), SIGNAL(signalEnterBrickMode(bool)), this, SLOT(slotEnterBrickMode(bool)));
	connect(SystemService::instance(), SIGNAL(signalExitBrickMode()), this, SLOT(slotExitBrickMode()));
}

TopLevelWindowManager::~TopLevelWindowManager()
{
    
}

void TopLevelWindowManager::init()
{
	m_lockedWindow = new LockWindow(boundingRect().width(), boundingRect().height());
	if (m_lockedWindow) {
		m_lockedWindow->init();
		m_lockedWindow->setPos(0,0);
		connect(m_lockedWindow, SIGNAL(signalScreenLocked(int)), this, SLOT(slotScreenLocked(int)));
		m_lockedWindow->setParentItem(this);
	}

	m_brickSurfAnimation.setPropertyName("opacity");
	m_brickSurfAnimation.setDuration(AS(brickDuration)); 
	m_brickSurfAnimation.setEasingCurve(AS_CURVE(brickCurve));
	connect(&m_brickSurfAnimation, SIGNAL(finished()), SLOT(slotBrickSurfAnimationFinished()));
}

bool TopLevelWindowManager::handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate)
{
    propogate = (!m_inBrickMode && !m_lockedWindow->isLocked());
    return false;
}

bool TopLevelWindowManager::okToResize()
{
	if(m_lockedWindow && !m_lockedWindow->okToResize())
		return false;

	if(m_brickSurfAnimation.state() == QAbstractAnimation::Running)
		return false;

	// Webkit Process is suspended, so we cannot resize
	if(m_suspendedWebkitProcess)
		return false;

	return true;
}

void TopLevelWindowManager::resize(int width, int height)
{
	// accept requests for resizing to the current dimensions, in case we are doing a force resize

	WindowManagerBase::resize(width, height);

	if(m_lockedWindow) {
		m_lockedWindow->resize(width, height);
	}

	if(m_brickWindow) {
		// regenerate the brick mode window for the correct size
		createBrickModeWindow();
	}
}


void TopLevelWindowManager::addWindow(Window* win)
{
	// phone app is the only acceptable creator of the Lock Screen Phone window
	if (win->type() != Window::Type_PIN)
	    return;

	if (win->appId() != "com.palm.app.phone") {
		g_critical("Rogue appl (%s) is trying to subvert the PIN lock entry app!", win->appId().c_str());
		WebAppMgrProxy::instance()->closeWindow(win);
		return;
	}
		
	if (m_lockedWindow) {
		m_lockedWindow->addPhoneAppWindow(win);
	}
}

void TopLevelWindowManager::removeWindow(Window* win)
{
	// we only own PIN type windows
	if (win->type() != Window::Type_PIN)
		return;

//	g_error("%s: PIN app requested to be removed", __PRETTY_FUNCTION__);
	WindowManagerBase::removeWindow(win);
}

bool TopLevelWindowManager::handleEvent(QEvent* event)
{
	// Disabling this for now in Dartfish, as we for now are using Emergency Mode for the Full Screen FLASH player
	if (/*SystemUiController::instance()->isInEmergencyMode() || */!m_lockedWindow)
		return false;
	return m_lockedWindow->handleFilteredSceneEvent(event);
}

void TopLevelWindowManager::delayDockModeLocking()
{
	if(m_lockedWindow)
		m_lockedWindow->delayDockModeLocking();
}

void TopLevelWindowManager::performDelayedLock()
{
	if(m_lockedWindow)
		m_lockedWindow->performDelayedLock();
}

void TopLevelWindowManager::createBrickModeWindow()
{
	qreal opacity = 0.0;

	if(m_brickWindow) {
		if (m_brickSurfAnimation.targetObject())
			m_brickSurfAnimation.stop();

		m_brickSurfAnimation.setTargetObject(NULL);

		opacity = m_brickWindow->opacity();

		m_brickWindow->setParentItem(0);
		delete m_brickWindow;
		m_brickWindow = 0;

		if( m_brickSurf ) {
			delete m_brickSurf;
			m_brickSurf = 0;
		}
	}

	// create scratch context
	QPainter* painter = new QPainter();
	m_brickSurf = new QPixmap(boundingRect().width(), boundingRect().height());

	// fill with black
	m_brickSurf->fill(QColor(0x00, 0x00, 0x00, 0xFF));

	// blit brick image centered
	painter->begin(m_brickSurf);

    std::string filePath =  Settings::LunaSettings()->lunaSystemResourcesPath;
    std::string bgPath = filePath + "/normal-bg.png";
    std::string fgPath = filePath + "/normal-usb.png";

	QPixmap image(bgPath.c_str());
	if (!image.isNull()) {
	    painter->drawPixmap((m_brickSurf->width() - image.width())/2,
		    				(m_brickSurf->height() - image.height())/2, image.width(), image.height(),
			    			image);
    }
    else {
        g_warning("Failed to load brick image: %s", bgPath.c_str());
    }

    image.load(fgPath.c_str());
    if (!image.isNull()) {
        painter->drawPixmap((m_brickSurf->width() - image.width())/2,
		    				(m_brickSurf->height() - image.height())/2, image.width(), image.height(),
			    			image);
    }
    else {
        g_warning("Failed to load brick image: %s", fgPath.c_str());
    }

	painter->end();
	
	// clean up
	delete painter;

	m_brickWindow = new Window(Window::Type_Invalid, *m_brickSurf);
	m_brickWindow->setOpacity(opacity);
	m_brickWindow->setParentItem(this);
}

void TopLevelWindowManager::slotEnterBrickMode(bool mediaSyncMode)
{
	// Media Sync mode is no longer valid, so teh mediaSyncMode parameter will be ignored here
	if (m_brickWindow || m_inBrickMode)
		return;

	if (m_brickSurfAnimation.targetObject())
		m_brickSurfAnimation.stop();

	createBrickModeWindow();

	if(!m_brickWindow)
		return;

	if(SystemUiController::instance()->isInDockMode()) {
		// in Dock mode, so immediately put the brick mode image on top
		m_brickWindow->setOpacity(1.0);
		m_brickWindow->setVisible(true);
		
		SystemUiController::instance()->enterOrExitDockModeUi(false);
	} else {
		// Not in Dock mode, so animate the screen in
		m_brickSurfAnimation.setTargetObject(NULL);
		m_brickSurfAnimation.setTargetObject(m_brickWindow);
	
		m_brickSurfAnimation.setStartValue(m_brickWindow->opacity());
		m_brickSurfAnimation.setEndValue(1.0);
		m_brickSurfAnimation.start();
	}

	m_inBrickMode = true;

	msmEntryComplete();

	g_message("Showing brick");
}

void TopLevelWindowManager::slotExitBrickMode()
{
	m_inBrickMode = false;

	if (m_brickWindow) {
		// fade out the brick window
        m_brickSurfAnimation.stop();
		m_brickSurfAnimation.setTargetObject(NULL);
		m_brickSurfAnimation.setTargetObject(m_brickWindow);
		m_brickSurfAnimation.setStartValue(m_brickWindow->opacity());
		m_brickSurfAnimation.setEndValue(0.0);
		m_brickSurfAnimation.start();
	}
	
	g_warning("Resuming webkit thread....");

	// thaw the webkit thread
	WebAppMgrProxy::instance()->resumeWebKitProcess();
	m_suspendedWebkitProcess = false;
}

void TopLevelWindowManager::slotBrickSurfAnimationFinished()
{
	if (m_brickWindow && qFuzzyCompare((double)(m_brickWindow->opacity()), (double)(0.0f))) {
		// done fading out

		m_brickSurfAnimation.setTargetObject(NULL);

		m_brickWindow->setParentItem(0);
		delete m_brickWindow;
		m_brickWindow = 0;

		if (m_brickSurf) {
			delete m_brickSurf;
			m_brickSurf = 0;
		}
	}
}

void TopLevelWindowManager::msmEntryComplete()
{
	if (!m_inBrickMode) {
		// got msm entry complete when we are not in brick mode
		return;
	}		
	
	if (!m_brickWindow)
		return;

	// already suspended thread
	if (m_suspendedWebkitProcess)
		return;

	g_warning("Suspending webkit thread....");	
	
	WebAppMgrProxy::instance()->suspendWebKitProcess();
	m_suspendedWebkitProcess = true;
}

void TopLevelWindowManager::slotScreenLocked(int state)
{
	switch (state) {
	case LockWindow::ScreenUnlocked: {
		// notify the WindowServer that the other layers should be visible again
		Q_EMIT signalScreenLockStatusChanged(false);
		unlockScreen();
	}
	break;
	
	case LockWindow::ScreenLocking: {
		lockScreen();
	}
	break;
	
	case LockWindow::ScreenLocked: {
		// notify the WindowServer that all other layers should be made invisible
		Q_EMIT signalScreenLockStatusChanged(true);
	}
	break;
	
	}
}

void TopLevelWindowManager::lockScreen()
{
	if (m_lockedWindow->isLocked())
		return;

	m_lockedWindow->fadeWindow(true);

	SystemUiController::instance()->setDeviceLocked(true);
}

void TopLevelWindowManager::unlockScreen()
{
	if (!m_lockedWindow->isLocked())
		return;

	m_lockedWindow->fadeWindow(false);
	
	SystemUiController::instance()->setDeviceLocked(false);
}

