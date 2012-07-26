/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "EmergencyWindowManager.h"

#include "CardWindow.h"
#include "Logging.h"
#include "RoundedCorners.h"
#include "Settings.h"
#include "SystemUiController.h"
#include "SystemService.h"
#include "WebAppMgrProxy.h"
#include "WindowServer.h"
#include "Utils.h"
#include "HostBase.h"
#include "FlickGesture.h"
#include "IMEController.h"


static const char* s_logChannel = "EmergencyWindowManager";

static const int kTopLeftWindowIndex     = 0;
static const int kTopRightWindowIndex    = 1;
static const int kBottomLeftWindowIndex  = 2;
static const int kBottomRightWindowIndex = 3;

static const int kFadeAnimDuration       = 350;
static const qreal kFadedOpacity         = 0.001;

EmergencyWindowManager::EmergencyWindowManager(int maxWidth, int maxHeight)
	: WindowManagerBase(maxWidth, maxHeight)
	, m_emergencyWindow(0)
    , m_cornerContainer(0)
{
    memset(m_corners, 0, sizeof(m_corners));
	setObjectName("EmergencyWindowManager");

    ::memset(m_corners, 0, 4 * sizeof(QGraphicsPixmapItem*));

	connect(SystemUiController::instance(), SIGNAL(signalPositiveSpaceChanged(const QRect&)),
			this, SLOT(slotPositiveSpaceChanged(const QRect&)));
	connect(SystemService::instance(), SIGNAL(signalIncomingPhoneCall()),
			this, SLOT(slotIncomingPhoneCall()));
	connect(SystemUiController::instance(), SIGNAL(signalEmergencyModeHomeButtonPressed()),
			this, SLOT(slotHomeButtonPressed()));
	connect(SystemUiController::instance(), SIGNAL(signalEmergencyModeWindowFocusChange(bool)),
			this, SLOT(slotEmergencyModeWindowFocusChange(bool)));
	connect(SystemUiController::instance(), SIGNAL(signalUiRotationCompleted()),
			this, SLOT(slotUiRotationCompleted()));

	m_winRect.setRect(0, 0, maxWidth, maxHeight);

	grabGesture(Qt::TapGesture);
	grabGesture(Qt::TapAndHoldGesture);
	grabGesture(Qt::PinchGesture);
	grabGesture((Qt::GestureType) SysMgrGestureFlick);
	grabGesture((Qt::GestureType) SysMgrGestureSingleClick);

	setVisible(false);
}

EmergencyWindowManager::~EmergencyWindowManager()
{

}

void EmergencyWindowManager::init()
{
    if (!Settings::LunaSettings()->tabletUi) {
        m_cornerContainer = new GraphicsItemContainer(boundingRect().width(),
                                                      boundingRect().height());

        QSize dims = RoundedCorners::topLeft().size();

        m_corners[kTopLeftWindowIndex] = new QGraphicsPixmapItem(RoundedCorners::topLeft(), m_cornerContainer);
        m_corners[kTopRightWindowIndex] = new QGraphicsPixmapItem(RoundedCorners::topRight(), m_cornerContainer);
        m_corners[kBottomLeftWindowIndex] = new QGraphicsPixmapItem(RoundedCorners::bottomLeft(), m_cornerContainer);
        m_corners[kBottomRightWindowIndex] = new QGraphicsPixmapItem(RoundedCorners::bottomRight(), m_cornerContainer);

        for (int i=kTopLeftWindowIndex; i <= kBottomRightWindowIndex; i++) {
            m_corners[i]->setOffset(-dims.width()/2, -dims.height()/2);
        }

        hideCornerWindows();

        m_cornerContainer->setParentItem(this);
    }
}

bool EmergencyWindowManager::handleNavigationEvent(QKeyEvent* keyEvent, bool& propogate)
{
    propogate = (!m_emergencyWindow || !(static_cast<CardWindow*>(m_emergencyWindow)->isHost()));
    return false;
}

bool EmergencyWindowManager::okToResize()
{
	if(!m_opacityAnimPtr.isNull() && m_opacityAnimPtr->state() != QAbstractAnimation::Stopped)
		return false;

	return true;
}

void EmergencyWindowManager::resize(int width, int height)
{
	// accept requests for resizing to the current dimensions, in case we are doing a force resize

	WindowManagerBase::resize(width, height);

    if (m_cornerContainer)
    	m_cornerContainer->resize(width, height);

	if(m_emergencyWindow) {
	    QRect screenBounds;

        if (static_cast<CardWindow*>(m_emergencyWindow)->isHost() ||
            static_cast<CardWindow*>(m_emergencyWindow)->fullScreen()) {
            screenBounds = QRect(0, 0, width, height);
        }
        else {
            screenBounds = QRect(0, Settings::LunaSettings()->positiveSpaceTopPadding,
		    				  	   width,
			    				   height - Settings::LunaSettings()->positiveSpaceTopPadding);
        }
		static_cast<CardWindow*>(m_emergencyWindow)->resizeWindowBufferEvent(width, height, screenBounds);
    }
}

void EmergencyWindowManager::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	event->setAccepted(m_emergencyWindow != 0);
}

void EmergencyWindowManager::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	event->setAccepted(m_emergencyWindow != 0);
}

bool EmergencyWindowManager::sceneEvent(QEvent* event)
{
	if (event->type() == QEvent::GestureOverride) {

		event->setAccepted(m_emergencyWindow != 0);
	}
	return QGraphicsObject::sceneEvent(event);
}

void EmergencyWindowManager::addWindow(Window* win)
{
	if (win->type() != Window::Type_Emergency)
		return;

	if (m_emergencyWindow) {

		luna_critical(s_logChannel, "Only one emergency window is allowed at a time");
		WebAppMgrProxy::instance()->closeWindow(win);
		return;
	}
	setVisible(true);
	
	m_emergencyWindow = win;
	static_cast<CardWindow*>(win)->focusEvent(true);
	IMEController::instance()->setClient(static_cast<HostWindow*>(win));

	m_emergencyWindow->setParentItem(this);
	m_emergencyWindow->stackBefore(m_cornerContainer);
	m_emergencyWindow->setOpacity(kFadedOpacity);
	m_emergencyWindow->setVisible(true);
	static_cast<CardWindow*>(m_emergencyWindow)->setPaintCompositionMode(QPainter::CompositionMode_SourceOver);

	static_cast<CardWindow*>(m_emergencyWindow)->setBoundingRect(m_winRect.width(), m_winRect.height());

	setPosTopLeft(win, m_winRect.x(), m_winRect.y());

	positionCornerWindows();
	showCornerWindows();

	SystemUiController::instance()->setEmergencyMode(true);

	if (static_cast<CardWindow*>(m_emergencyWindow)->fullScreen())
		SystemUiController::instance()->hideStatusBarAndNotificationArea();
	else {
		SystemUiController::instance()->showStatusBarAndNotificationArea();
	}
	
	m_opacityAnimPtr = new QPropertyAnimation();
	m_opacityAnimPtr->setPropertyName("opacity");
	m_opacityAnimPtr->setEasingCurve(QEasingCurve::Linear);
	m_opacityAnimPtr->setTargetObject(m_emergencyWindow);
	m_opacityAnimPtr->setDuration(kFadeAnimDuration);
	m_opacityAnimPtr->setEndValue(1.0);
	connect(m_opacityAnimPtr, SIGNAL(finished()), SLOT(fadeAnimationFinished()));
	m_opacityAnimPtr->start(QAbstractAnimation::DeleteWhenStopped);
	
	update();
}

void EmergencyWindowManager::removeWindow(Window* win)
{
	if (!win)
		return;

	if (win == m_emergencyWindow) {

		m_opacityAnimPtr = new QPropertyAnimation();
		m_opacityAnimPtr->setPropertyName("opacity");
		m_opacityAnimPtr->setEasingCurve(QEasingCurve::Linear);
		m_opacityAnimPtr->setTargetObject(m_emergencyWindow);
		m_opacityAnimPtr->setDuration(kFadeAnimDuration); 
		m_opacityAnimPtr->setEndValue(kFadedOpacity);
		connect(m_opacityAnimPtr, SIGNAL(finished()), SLOT(fadeAnimationFinished()));
		m_opacityAnimPtr->start(QAbstractAnimation::DeleteWhenStopped);

		IMEController::instance()->removeClient(static_cast<HostWindow*>(win));
		SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::EMERGENCY_MODE_WINDOW_MANAGER, static_cast<CardWindow*>(win), false);
		static_cast<CardWindow*>(m_emergencyWindow)->setPaintCompositionMode(QPainter::CompositionMode_SourceOver);

		return;
	}
	else {

		// this happens if window is closed quickly before it was added
		delete win;
	}

	// FIXME: Need to handle any popup alerts we have
}

void EmergencyWindowManager::fadeAnimationFinished()
{
	if(m_emergencyWindow && (m_emergencyWindow->opacity() == 1.0)) {
		// Finished fading IN
		static_cast<CardWindow*>(m_emergencyWindow)->setPaintCompositionMode(QPainter::CompositionMode_Source);

		// check to see if the window requires a specific orientation
		if(Settings::LunaSettings()->displayUiRotates) {
			CardWindow* win = static_cast<CardWindow*>(m_emergencyWindow);
			SystemUiController::instance()->setRequestedSystemOrientation(win->getCardFixedOrientation(), true, true);
		}

		SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::EMERGENCY_MODE_WINDOW_MANAGER, static_cast<CardWindow*>(m_emergencyWindow), true);
		update();
	} else if(m_emergencyWindow && (m_emergencyWindow->opacity() <= kFadedOpacity)) {

		// Finished fading OUT
		m_emergencyWindow->setParentItem(0);

		hideCornerWindows();

		update();
		SystemUiController::instance()->setEmergencyMode(false);

		if(Settings::LunaSettings()->displayUiRotates) {
			// restore fixed orientation properties for the maximized card window
			CardWindow* win = static_cast<CardWindow*>(SystemUiController::instance()->maximizedCardWindow());
			if(win)
				SystemUiController::instance()->setRequestedSystemOrientation(win->getCardFixedOrientation(), true, false);
			else
				WindowServer::instance()->setUiRotationMode(WindowServer::RotationMode_FreeRotation);
		}
		// If we had enabled full screen mode, restore normal mode again
		if (static_cast<CardWindow*>(m_emergencyWindow)->fullScreen()) {
			SystemUiController::instance()->showStatusBarAndNotificationArea();
		}

		delete m_emergencyWindow;
		m_emergencyWindow = 0;

		setVisible(false);
	}
}

void EmergencyWindowManager::focusWindow(Window* win)
{
	
}

void EmergencyWindowManager::slotEmergencyModeWindowFocusChange(bool enable)
{
	if(m_emergencyWindow) {
		static_cast<CardWindow*>(m_emergencyWindow)->focusEvent(enable);
        if (enable) {
            if (!emergencyWindowBeingDeleted()) {
                SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::EMERGENCY_MODE_WINDOW_MANAGER,
                                                                            static_cast<CardWindow*>(m_emergencyWindow), true);
            }
        }
        else {
            SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::EMERGENCY_MODE_WINDOW_MANAGER,
                                                                        static_cast<CardWindow*>(m_emergencyWindow), false);
        }
	}
}

void EmergencyWindowManager::slotUiRotationCompleted()
{
	if(SystemUiController::instance()->isInEmergencyMode() && m_emergencyWindow && !static_cast<CardWindow*>(m_emergencyWindow)->isMaximized()) {
		// make sure the active window has direct rendering after rotation
        if (!emergencyWindowBeingDeleted()) {
            SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::EMERGENCY_MODE_WINDOW_MANAGER,
                                                                        static_cast<CardWindow*>(m_emergencyWindow), true, true);
        }
	}
}

void EmergencyWindowManager::slotPositiveSpaceChanged(const QRect& r)
{
	// The emergency window manager always covers the whole display but limits 
	// the size of the active emergency window to be either full screen or
	// screen height - top positive space

	int positiveSpaceTopPadding = Settings::LunaSettings()->positiveSpaceTopPadding;

	int y = qMin(r.y(), positiveSpaceTopPadding);
	int w = SystemUiController::instance()->currentUiWidth() - r.x();
	int h = SystemUiController::instance()->currentUiHeight() - y;

	m_winRect.setX(qMin(0, r.x()));
	m_winRect.setY(y);
	m_winRect.setHeight(h);
	m_winRect.setWidth(w);

	positionCornerWindows();

	if (m_emergencyWindow) {
		setPosTopLeft(m_emergencyWindow, m_winRect.x(), m_winRect.y());
		static_cast<CardWindow*>(m_emergencyWindow)->resizeEvent(w, h);
	}
}

void EmergencyWindowManager::slotIncomingPhoneCall()
{
	if (!m_emergencyWindow)
		return;

	// if you are on an emergency call, don't exit
	if (m_emergencyWindow->appId() == "com.palm.app.phone")
		return;

	static_cast<CardWindow*>(m_emergencyWindow)->close();
}

void EmergencyWindowManager::positionCornerWindows()
{
    if (Settings::LunaSettings()->tabletUi)
        return;

    int i = kTopLeftWindowIndex;
    int trueBottom = m_winRect.y() + m_winRect.height();
    int trueRight = m_winRect.x() + m_winRect.width();
    Q_ASSERT(m_corners[i]);
    QRectF rect = m_corners[i]->boundingRect();

    setPosTopLeft(m_corners[i], m_winRect.x(), m_winRect.y());

    i = kTopRightWindowIndex;
    Q_ASSERT(m_corners[i]);
    setPosTopLeft(m_corners[i], trueRight - rect.width(), m_winRect.y());

    i = kBottomLeftWindowIndex;
    Q_ASSERT(m_corners[i]);
    setPosTopLeft(m_corners[i], m_winRect.x(), trueBottom - rect.height());

    i = kBottomRightWindowIndex;
    Q_ASSERT(m_corners[i]);
    setPosTopLeft(m_corners[i], trueRight - rect.width(), trueBottom - rect.height());
}

void EmergencyWindowManager::showCornerWindows()
{
    if (m_cornerContainer)
    	m_cornerContainer->setVisible(true);
}

void EmergencyWindowManager::hideCornerWindows()
{
    if (m_cornerContainer)
    	m_cornerContainer->setVisible(false);
}

void EmergencyWindowManager::slotHomeButtonPressed()
{
	if (!m_emergencyWindow)
		return;

	if (!static_cast<CardWindow*>(m_emergencyWindow)->isHost())
		return;

	static_cast<CardWindow*>(m_emergencyWindow)->close();    
}

bool EmergencyWindowManager::emergencyWindowBeingDeleted() const
{
	if (m_emergencyWindow &&
        !m_opacityAnimPtr.isNull() &&
        m_opacityAnimPtr->state() != QAbstractAnimation::Stopped)
		return true;

    return false;
}
