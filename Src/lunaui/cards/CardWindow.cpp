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

#include "CardWindow.h"

#include <PIpcBuffer.h>
#include <PIpcChannel.h>

#if defined(HAVE_OPENGL) && defined(TARGET_DEVICE)
#define USE_ROUNDEDCORNER_SHADER 1
#else
#undef USE_ROUNDEDCORNER_SHADER
#endif

#if defined(USE_ROUNDEDCORNER_SHADER)
#include <QtOpenGL/qglcustomshaderstage_p.h>
#endif

#include "AppDirectRenderingArbitrator.h"
#include "AlertWindow.h"
#include "AnimationSettings.h"
#include "EventThrottler.h"
#include "FlickGesture.h"
#include "IMEController.h"
#include "Logging.h"
#include "MenuWindowManager.h"
#include "Settings.h"
#include "SharedGlobalProperties.h"
#include "SystemUiController.h"
#include "WebAppMgrProxy.h"
#include "WindowServer.h"
#include "WindowServerLuna.h"
#include "WindowManagerBase.h"
#include "ApplicationDescription.h"
#include "Time.h"
#include "CardLoading.h"
#include "IpcClientHost.h"
#include "CardDropShadowEffect.h"
#include "CardGroup.h"
#include "DisplayManager.h"
#include "HostBase.h"
#include "WindowMetaData.h"
#include "WindowServer.h"
#include "GhostCard.h"
#include "QtUtils.h"

#include <SysMgrDefs.h>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QMatrix4x4>
#include <QDebug>
#include <QApplication>
#include "SingleClickGesture.h"
#include "IMEController.h"

int CardWindow::sStartSpaceChangeValue = -1;
int CardWindow::sLastKnownPositiveSpace = 0;
float CardWindow::sBoundingRectYBeforeResize = 0;
QPointF CardWindow::sModalWindowYBeforeResize;

static const int sModalCardAnimationTimeout = 500;

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

QDebug operator<<(QDebug dbg, const CardWindow::Position &p)
{
	dbg.nospace() << "Position(" << p.trans << "(rotation" << p.zRot << "))";
	return dbg;
}

QVariant positionInterpolator(const CardWindow::Position &start, const CardWindow::Position &end, qreal progress)
{
	return qVariantFromValue(CardWindow::Position(start + (end - start) * progress));
}

CardWindow::CardWindow(Window::Type type, HostWindowData* data, IpcClientHost* clientHost)
	: HostWindow(type, data, clientHost)
	, m_prepareAddedToWm(false)
	, m_addedToWm(false)
	, m_capturedEvents(CapturedNone)
	, m_fullScreenEnabled(false)
	, m_fullScreenModeSuppressed(false)
	, m_maximized(false)
	, m_forceFocus(true)
	, m_focused(false)
	, m_touchEventsEnabled(false)
	, m_pendingFocus(PendingFocusNone)
	, m_loadingAnim(0)
	, m_loadingTimerId(0)
    , m_splashBackgroundName()
	, m_compMode(QPainter::CompositionMode_Source)
	, m_group(0)
	, m_attachedToGroup(true)
	, m_launchInNewGroup(false)
	, m_isResizing(false)
	, m_pendingDirectRenderingEnable(false)
	, m_flipsQueuedUp(0)
	, m_pendingRotationRequests(0)
	, m_ignorePendingRotationRequests(0)
	, m_roundedCornerShaderStage(0)
    , m_activeAnimation(0)
    , m_dimming(1.0f)
	, m_appFixedOrientation(Event::Orientation_Invalid)
	, m_appPendingFixedOrientation(Event::Orientation_Invalid)
	, m_adjustmentAngle(0)
	, m_focusPendingRotation(false)
	, m_keyboardShownMessageSent(false)
	, m_isCardModalParent(false)
	, m_modalChild(NULL)
	, m_modalParent(NULL)
	, m_modalAcceptInputState(NoModalWindow)
	, m_maxEndingPositionForOrientation(-1)
{
    init();
}

CardWindow::CardWindow(Window::Type type, const QPixmap& pixmap)
	: HostWindow(type, pixmap.width(), pixmap.height(), false)
	, m_prepareAddedToWm(false)
	, m_addedToWm(false)
	, m_capturedEvents(CapturedNone)
	, m_fullScreenEnabled(false)
	, m_fullScreenModeSuppressed(false)
	, m_maximized (false)
	, m_forceFocus(true)
	, m_focused(false)
	, m_touchEventsEnabled(false)
	, m_pendingFocus(PendingFocusNone)
	, m_loadingAnim(0)
	, m_loadingTimerId(0)
	, m_splashBackgroundName()
	, m_compMode(QPainter::CompositionMode_Source)
	, m_group(0)
	, m_attachedToGroup(true)
    , m_launchInNewGroup(false)
	, m_isResizing(false)
	, m_pendingDirectRenderingEnable(false)
	, m_flipsQueuedUp(0)
	, m_pendingRotationRequests(0)
	, m_ignorePendingRotationRequests(0)
	, m_roundedCornerShaderStage(0)
    , m_activeAnimation(0)
    , m_dimming(1.0f)
	, m_appFixedOrientation(Event::Orientation_Invalid)
	, m_adjustmentAngle(0)
	, m_focusPendingRotation(false)
	, m_keyboardShownMessageSent(false)
	, m_isCardModalParent(false)
	, m_modalChild(NULL)
	, m_modalParent(NULL)
	, m_modalAcceptInputState(NoModalWindow)
	, m_maxEndingPositionForOrientation(-1)
{
    init();
}

void CardWindow::init()
{
	setFlags(QGraphicsItem::ItemIsFocusable);
	grabGesture((Qt::GestureType) SysMgrGestureFlick);
	grabGesture((Qt::GestureType) SysMgrGestureSingleClick);
	grabGesture(Qt::PinchGesture);

	connect(DisplayManager::instance(), SIGNAL(signalDisplayStateChange(int)),
										   SLOT(slotDisplayStateChanged(int)));

	connect(IMEController::instance(), SIGNAL(signalShowIME()), SLOT(slotShowIME()));
	connect(IMEController::instance(), SIGNAL(signalHideIME()), SLOT(slotHideIME()));

	if (Window::Type_ModalChildWindowCard == type()) {
		m_ModalWindowEndY = 0;
		m_modalWindowShrinkHeight = 0;
		m_fRecomputeInitPositionsValues = true;
		m_ModalPositionAnimation = new QPropertyAnimation(this, "y");
		m_posSpChangeNotificationState = Unknown;
	}

	if (Settings::LunaSettings()->uiType == Settings::UI_LUNA) {
		WindowServerLuna* ws = static_cast<WindowServerLuna*>(WindowServer::instance());
		connect(ws, SIGNAL(signalUiRotated()), SLOT(slotUiRotated()));
	}

    m_activeAnimation = new QPropertyAnimation(this, "dimming");
    m_activeAnimation->setEasingCurve(AS_CURVE(cardDimmingCurve));
    m_activeAnimation->setDuration(AS(cardDimmingDuration));
}

CardWindow::~CardWindow()
{
	AppDirectRenderingArbitrator::setLayerEnabled(this, false);

	stopLoadingTimer();

	delete m_loadingAnim;

	if(m_maximized && Window::Type_ModalChildWindowCard != type()) {
		// disable the direct rendering request for this window with SystemUiController
		SystemUiController::instance()->setDirectRenderingForWindow(SystemUiController::CARD_WINDOW_MANAGER, this, false);
	}

	ungrabGesture((Qt::GestureType) SysMgrGestureSingleClick);
	ungrabGesture((Qt::GestureType) SysMgrGestureFlick);
	ungrabGesture(Qt::PinchGesture);

#if defined(USE_ROUNDEDCORNER_SHADER)
	delete m_roundedCornerShaderStage;
#endif

    delete m_activeAnimation;
}

void CardWindow::setDimm(bool dimm)
{
    if (!m_activeAnimation) {
        g_critical("%s: animation object not present", __PRETTY_FUNCTION__);
        return;
    }

    m_activeAnimation->stop();
    m_activeAnimation->setStartValue(m_dimming);
    m_activeAnimation->setEndValue(dimm ? Settings::LunaSettings()->cardDimmPercentage : 1.0f);
    m_activeAnimation->start();
}

void CardWindow::onMessageReceived(const PIpcMessage& msg)
{
	bool msgIsOk;

	IPC_BEGIN_MESSAGE_MAP(CardWindow, msg, msgIsOk)
		IPC_MESSAGE_HANDLER(ViewHost_Card_AppLaunchOptionsEvent, setAppLaunchOptions)
		IPC_MESSAGE_HANDLER(ViewHost_Card_SetAppOrientation, onSetAppOrientation)
		IPC_MESSAGE_HANDLER(ViewHost_Card_SetFreeOrientation, onSetAppFreeOrientation)
		IPC_MESSAGE_HANDLER(ViewHost_Card_SetAppFixedOrientation, onSetAppFixedOrientation)
		// not handled here, forward it to the base class
		IPC_MESSAGE_UNHANDLED( HostWindow::onMessageReceived(msg); )
	IPC_END_MESSAGE_MAP()
}

void CardWindow::setAppLaunchOptions(const AppLaunchOptionsEventWrapper& options) {
	if (!options.event->splashBackground.empty()) {
        m_splashBackgroundName = options.event->splashBackground;
	}

	m_launchInNewGroup = options.event->launchInNewGroup;

}

int CardWindow::forwardToModal()
{
	if(NULL != m_modalChild) {
		if(ModalLaunchedAcceptingInput == m_modalAcceptInputState)
			return ForwardEventToChild;
		else
			return WaitForChildToAcceptEvents;
	}

	return ParentHandleEvent;
}

void CardWindow::customEvent ( QEvent * event ) {

	switch(forwardToModal()) {
		case ParentHandleEvent:
			break;
		case WaitForChildToAcceptEvents:
			event->accept();
			return;
		case ForwardEventToChild:
			m_modalChild->customEvent(event);
			return;
	}

	if (!m_focused || m_pendingFocus == PendingFocusFalse) {
		event->ignore();
		return;
	}
}
bool CardWindow::sceneEvent(QEvent* event)
{
	switch(forwardToModal()) {
		case ParentHandleEvent:
			break;
		case WaitForChildToAcceptEvents:
			event->accept();
			return true;
		case ForwardEventToChild:
			return m_modalChild->sceneEvent(event);
	}

	if (m_focused && m_pendingFocus != PendingFocusFalse) {

		if (event->type() == QEvent::GestureOverride) {
			QGestureEvent* ge = static_cast<QGestureEvent*>(event);
			QGesture* g = ge->gesture(Qt::PinchGesture);
			if (g) {
				event->accept();
				return true;
			}

			g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
			if (g) {
				event->accept();
				return true;
			}
			g = ge->gesture((Qt::GestureType) SysMgrGestureSingleClick);
			if (g) {
				event->accept();
				return true;
			}
		}
		else if (event->type() == QEvent::Gesture) {
			QGestureEvent* ge = static_cast<QGestureEvent*>(event);
			QGesture* g = ge->gesture((Qt::GestureType) SysMgrGestureFlick);
			if (g && g->state() == Qt::GestureFinished) {
				if (mouseFlickEvent(ge)) {
					return true;
				}
			}

			g = ge->gesture(Qt::PinchGesture);
			if (g) {
				if (pinchEvent(ge)) {
					return true;
				}
			}
			g = ge->gesture((Qt::GestureType) SysMgrGestureSingleClick);
			if (g && g->state() == Qt::GestureFinished) {
			    if (mouseSingleClickEvent (ge)) {
					return true;
				}
			}
		}
		else if (event->type() == QEvent::TouchBegin
			|| event->type() == QEvent::TouchUpdate
			|| event->type() == QEvent::TouchEnd) 
		{
		    QTouchEvent* te = static_cast<QTouchEvent*>(event);
		    if (touchEvent (te)) {
				return true;
			}
		}
        else if (event->type() == QEvent::KeyPress) {
            // Tab is normally treated as focus shifting in qgraphicsitems
            if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Tab) {
                keyPressEvent(static_cast<QKeyEvent*>(event));
                return true;
            }
        }
	}

	return QGraphicsObject::sceneEvent(event);
}

void CardWindow::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	switch(forwardToModal()) {
		case ParentHandleEvent:
			break;
		case WaitForChildToAcceptEvents:
			event->accept();
			return;
		case ForwardEventToChild:
			m_modalChild->mousePressEvent(event);
			return;
	}

	if (!m_focused || m_pendingFocus == PendingFocusFalse) {
		event->ignore();
		return;
	}

	event->accept();

	QRectF br = boundingRect();

	Event ev;
	ev.type = Event::PenDown;
	ev.setMainFinger(true);
	qreal x = event->pos().x();
	qreal y = event->pos().y();
	mapCoordinates(x,y);
	ev.x = x;
	ev.y = y;
	ev.clickCount = 1;
	ev.modifiers = Event::modifiersFromQt(event->modifiers());
	ev.time = Time::curSysTimeMs();

	inputEvent(&ev);
}

void CardWindow::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	switch(forwardToModal()) {
		case ParentHandleEvent:
			break;
		case WaitForChildToAcceptEvents:
			return;
		case ForwardEventToChild:
			m_modalChild->mouseDoubleClickEvent(event);
			return;
	}

	QRectF br = boundingRect();

	Event ev;
	ev.type = Event::PenDown;
	ev.setMainFinger(true);
	qreal x = event->pos().x();
	qreal y = event->pos().y();
	mapCoordinates(x,y);
	ev.x = x;
	ev.y = y;
	ev.clickCount = 2;
	ev.modifiers = Event::modifiersFromQt(event->modifiers());
	ev.time = Time::curSysTimeMs();

	inputEvent(&ev);
}

void CardWindow::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	switch(forwardToModal()) {
		case ParentHandleEvent:
			break;
		case WaitForChildToAcceptEvents:
			return;
		case ForwardEventToChild:
			m_modalChild->mouseMoveEvent(event);
			return;
	}

	QRectF br = boundingRect();

	Event ev;
	ev.type = Event::PenMove;
	ev.setMainFinger(true);
	qreal x = event->pos().x();
	qreal y = event->pos().y();
	mapCoordinates(x,y);
	ev.x = x;
	ev.y = y;
	ev.modifiers = Event::modifiersFromQt(event->modifiers());
	ev.time = Time::curSysTimeMs();

	inputEvent(&ev);
}

void CardWindow::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	switch(forwardToModal()) {
		case ParentHandleEvent:
			break;
		case WaitForChildToAcceptEvents:
			return;
		case ForwardEventToChild:
			m_modalChild->mouseReleaseEvent(event);
			return;
	}

	QRectF br = boundingRect();

	Event ev;
	if (event->canceled())
		ev.type = Event::PenCancel;
	else 
		ev.type = Event::PenUp;
	ev.setMainFinger(true);
	qreal x = event->pos().x();
	qreal y = event->pos().y();
	mapCoordinates(x,y);
	ev.x = x;
	ev.y = y;
	ev.clickCount = 0;
	ev.modifiers = Event::modifiersFromQt(event->modifiers());
	ev.time = Time::curSysTimeMs();

	inputEvent(&ev);
}

bool CardWindow::mouseFlickEvent(QGestureEvent* event)
{
	switch(forwardToModal()) {
		case ParentHandleEvent:
			break;
		case WaitForChildToAcceptEvents:
			return true;
		case ForwardEventToChild:
			return m_modalChild->mouseFlickEvent(event);
	}

	QGesture* g = event->gesture((Qt::GestureType) SysMgrGestureFlick);
	FlickGesture* flick = static_cast<FlickGesture*>(g);

	QPointF pos = mapFromScene(event->mapToGraphicsScene(flick->hotSpot()));
	QRectF br = boundingRect();

	Event ev;
	ev.type = Event::PenFlick;
	ev.setMainFinger(true);
	qreal x = pos.x();
	qreal y = pos.y();
	mapCoordinates(x,y);
	ev.x = x;
	ev.y = y;
	qreal xvel = flick->velocity().x();
	qreal yvel = flick->velocity().y();
	mapFlickVelocities(xvel,yvel);
	ev.flickXVel = xvel;
	ev.flickYVel = yvel;

	inputEvent(&ev);
	return true;
}

bool CardWindow::mouseSingleClickEvent(QGestureEvent* singleClickEvent)
{
	switch(forwardToModal()) {
		case ParentHandleEvent:
			break;
		case WaitForChildToAcceptEvents:
			return true;
		case ForwardEventToChild:
			return m_modalChild->mouseSingleClickEvent(singleClickEvent);
	}

	QGesture* g = singleClickEvent->gesture((Qt::GestureType) SysMgrGestureSingleClick);
	SingleClickGesture* singleClick = static_cast<SingleClickGesture*>(g);

	QPointF pos = mapFromScene(singleClickEvent->mapToGraphicsScene(singleClick->hotSpot()));
	QRectF br = boundingRect();

	Event ev;
	ev.type = Event::PenSingleTap;
	ev.setMainFinger(true);
	qreal x = pos.x();
	qreal y = pos.y();
	mapCoordinates(x,y);
	ev.x = x;
	ev.y = y;
	ev.modifiers = Event::modifiersFromQt (singleClick->modifiers());

	inputEvent(&ev);
	return true;
}


bool CardWindow::pinchEvent(QGestureEvent* event)
{
	switch(forwardToModal()) {
		case ParentHandleEvent:
			break;
		case WaitForChildToAcceptEvents:
			return true;
		case ForwardEventToChild:
			return m_modalChild->pinchEvent(event);
	}

	Event ev;
	ev.time = Time::curTimeMs();

	QPinchGesture* pinch = static_cast<QPinchGesture*>(event->gesture(Qt::PinchGesture));

	switch (pinch->state()) {
	case Qt::GestureStarted:
		ev.type = Event::GestureStart;
		break;
	case Qt::GestureUpdated:
		ev.type = Event::GestureChange;
		break;
	case Qt::GestureFinished:
		ev.type = Event::GestureEnd;
		break;
	case Qt::GestureCanceled:
		ev.type = Event::GestureCancel;
		break;
	case Qt::NoGesture:
		// FIXME -- I'm not sure if this is what was intended
		return false;
	}

	QRectF br = boundingRect();
	
	ev.gestureScale = pinch->totalScaleFactor();
	ev.gestureRotate = pinch->totalRotationAngle();
	QPointF centerPt = mapFromScene(event->mapToGraphicsScene(pinch->centerPoint()));

	qreal x = centerPt.x();
	qreal y = centerPt.y();
	mapCoordinates(x,y);
	ev.gestureCenterX = x;
	ev.gestureCenterY = y;
	
	inputEvent(&ev);

	return true;
}

void CardWindow::inputEvent(Event* e)
{
	switch(forwardToModal()) {
		case ParentHandleEvent:
			break;
		case WaitForChildToAcceptEvents:
			return;
		case ForwardEventToChild:
			m_modalChild->inputEvent(e);
			return;
	}

	if (Event::isPenEvent(e) && !e->mainFinger())
		return;

	switch (e->type) {
	case Event::PenDown:
		m_capturedEvents |= CapturedPen;
		break;
	case Event::PenUp:
	case Event::PenCancel:
		m_capturedEvents &= ~CapturedPen;
		break;
	case Event::GestureStart:
		m_capturedEvents |= CapturedGesture;
		break;
	case Event::GestureEnd:
	case Event::GestureCancel:
		m_capturedEvents &= ~CapturedGesture;
		break;
	default:
		break;
	}

	if (isHost()) {
		if (m_channel)
			m_channel->sendAsyncMessage(new View_InputEvent(routingId(), SysMgrEventWrapper(e)));
	}
	else if (isIpcWindow()) {
		WebAppMgrProxy::instance()->inputEvent(this, e);
	}
}

void CardWindow::resizeEvent(int w, int h)
{
//	if(SystemUiController::instance()->isUiRotating() && (m_appFixedOrientation == Event::Orientation_Invalid))
//		return;

	if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90){
		if (m_channel)
			m_channel->sendAsyncMessage(new View_Resize(routingId(), h, w, false));
	} else {
		if (m_channel)
			m_channel->sendAsyncMessage(new View_Resize(routingId(), w, h, false));
	}
}

void CardWindow::resizeEventSync(int w, int h)
{
	// Ignore this check if we are modal window.

	if(Window::Type_ModalChildWindowCard != type()) {
		if(SystemUiController::instance()->isUiRotating())
			return;
	}

	if (m_channel) {

		SystemUiController::instance()->aboutToSendSyncMessage();

		int dummy = 0;

		if(Window::Type_ModalChildWindowCard != type()) {
			if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90){
				if (m_channel)
					m_channel->sendSyncMessage(new View_SyncResize(routingId(), h, w, false, &dummy));
			} else {
				if (m_channel)
					m_channel->sendSyncMessage(new View_SyncResize(routingId(), w, h, false, &dummy));
			}
		}
		else {
			if (m_channel) {
				m_channel->sendSyncMessage(new View_SyncResize(routingId(), w, h, false, &dummy));
			}
		}
	}
}

void CardWindow::resizeWindowBufferEvent(int w, int h, QRect windowScreenBounds, bool forceSync, bool ignoreFixedOrient)
{
	g_message("ROTATION: [%s]: id: %s, ptr = 0x%x, w = %d, h = %d, sync = %d, resizing = %d, inQueue = %d, SB = (%d, %d, %d, %d)", __PRETTY_FUNCTION__,
			   m_appId.c_str(), (int)this, w, h, forceSync, m_isResizing, m_flipsQueuedUp, windowScreenBounds.x(), windowScreenBounds.y(), windowScreenBounds.width(), windowScreenBounds.height());

	bool obscured = false;
	bool directRendering = m_maximized;
	bool visible = isVisible() && this->sceneBoundingRect().intersects(WindowServer::instance()->sceneRect());

	if(false == SystemUiController::instance()->isModalWindowActive())
		obscured = SystemUiController::instance()->maximizedCardWindow() ? (SystemUiController::instance()->maximizedCardWindow() != this) : false;
	else
		obscured = SystemUiController::instance()->getModalParent() ? (SystemUiController::instance()->getModalParent() != this) : false;

	bool synch = forceSync || m_maximized || !m_addedToWm ||
			     (!SystemUiController::instance()->isInEmergencyMode() && ((visible  && !obscured) || (this == SystemUiController::instance()->maximizedCardWindow())));

	if(m_appPendingFixedOrientation != Event::Orientation_Invalid) {
		m_appFixedOrientation = Event::Orientation_Invalid;
	}

	if(Settings::LunaSettings()->displayUiRotates) {
		if(m_appFixedOrientation == Event::Orientation_Invalid || ignoreFixedOrient) {
			if(!m_isResizing && !m_flipsQueuedUp && (w == (int)m_bufWidth) && (h == (int)m_bufHeight)) {
				// already the right size, so do not resize buffer
				if(m_appPendingFixedOrientation != Event::Orientation_Invalid) {
					m_appFixedOrientation = m_appPendingFixedOrientation;
					m_appPendingFixedOrientation = Event::Orientation_Invalid;
					refreshAdjustmentAngle();
				}
				setBoundingRect(windowScreenBounds.width(), windowScreenBounds.height());
				resizeEvent(windowScreenBounds.width(), windowScreenBounds.height());
				g_message("ROTATION: [%s]: Resize Aborted (1), window already in the right size. id: %s, ptr = 0x%x, m_isResizing = %d, m_flipsQueuedUp = %d",
						   __PRETTY_FUNCTION__, m_appId.c_str(), (int)this, m_isResizing, m_flipsQueuedUp);
				return;
			}

			if((m_flipsQueuedUp%2 != 0) && (w == (int)m_bufWidth) && (h == (int)m_bufHeight)) {
				// already the right size, so do not resize buffer and cancel queued up flip request
				WindowServer::instance()->removeWindowFromFlipQueue(this);
				m_flipsQueuedUp = 0;
				queuedFlipCanceled(windowScreenBounds);

				setBoundingRect(windowScreenBounds.width(), windowScreenBounds.height());
				resizeEvent(windowScreenBounds.width(), windowScreenBounds.height());
				g_message("ROTATION: [%s]: Resize Aborted (2), window already in the right size. id: %s, ptr = 0x%x, m_isResizing = %d, m_flipsQueuedUp = %d",
						   __PRETTY_FUNCTION__, m_appId.c_str(), (int)this, m_isResizing, m_flipsQueuedUp);
				return;
			}

			if((m_flipsQueuedUp%2 != 0) && (h == (int)m_bufWidth) && (w == (int)m_bufHeight)) {
				// request to flip the buffer, but we already have a similar request queued up, so cancel the one in the queue before proceding
				WindowServer::instance()->removeWindowFromFlipQueue(this);
				g_message("ROTATION: [%s]: Canceling similar flip request in Queue. id: %s, ptr = 0x%x, m_isResizing = %d, m_flipsQueuedUp = %d",
						   __PRETTY_FUNCTION__, m_appId.c_str(), (int)this, m_isResizing, m_flipsQueuedUp);
				m_flipsQueuedUp = 0;
				queuedFlipCanceled(windowScreenBounds);
			}

			if(directRendering)
				setMaximized(false); // disable direct rendering for the resize event

			if(((m_flipsQueuedUp%2 == 0) && (w == (int)m_bufHeight) && (h == (int)m_bufWidth)) ||
			   ((m_flipsQueuedUp%2 != 0) && (w == (int)m_bufWidth) && (h == (int)m_bufHeight))) {
				// don't resize, just flip
				if(synch) {
					flipEventSync();
				} else {
					// queue this window up for rotation after the animation is done
					queueUpWindowForFlip(windowScreenBounds, false);
				}
			} else {
				g_warning("ROTATION: [%s]: Warning, Default to Window resize, not fLIP! id: %s, ptr = 0x%x, m_isResizing = %d, m_flipsQueuedUp = %d, m_bufWidth = %d,m_bufHeight = %d",
						   __PRETTY_FUNCTION__, m_appId.c_str(), (int)this, m_isResizing, m_flipsQueuedUp, m_bufWidth, m_bufHeight);
				HostWindow::resizeEventSync(w,h);
			}

			if(synch) {
				setBoundingRect(windowScreenBounds.width(), windowScreenBounds.height());

				// reconstruct shadow
				CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
				if (shadow) {
					bool enable = shadow->isEnabled();
					setGraphicsEffect(0);
					shadow = new CardDropShadowEffect(this);
					setGraphicsEffect(shadow);
					shadow->setEnabled(enable);
				}

				initializeRoundedCornerStage();

				if(directRendering)
					setMaximized(true); // re-enable direct rendering
			}
		} else {
			if(directRendering)
				setMaximized(false); // disable direct rendering for the resize event

			refreshAdjustmentAngle();

			setBoundingRect(w, h);
			resizeEvent(w, h);

			// reconstruct shadow
			CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
			if (shadow) {
				bool enable = shadow->isEnabled();
				setGraphicsEffect(0);
				shadow = new CardDropShadowEffect(this);
				setGraphicsEffect(shadow);
				shadow->setEnabled(enable);
			}

			initializeRoundedCornerStage();

			if(directRendering)
				setMaximized(true); // re-enable direct rendering
		}

        if(m_appPendingFixedOrientation != Event::Orientation_Invalid) {
                  m_appFixedOrientation = m_appPendingFixedOrientation;
                  m_appPendingFixedOrientation = Event::Orientation_Invalid;
                  refreshAdjustmentAngle();
          }

	}
}

void CardWindow::flipEventSync(bool fromQueue)
{
	g_message("ROTATION: [%s]: fromQueue = %d, id: %s, ptr = 0x%x, m_isResizing = %d, m_flipsQueuedUp = %d, m_pendingRotationRequests = %d",
			   __PRETTY_FUNCTION__, fromQueue, m_appId.c_str(), (int)this, m_isResizing , m_flipsQueuedUp, m_pendingRotationRequests);

	if(fromQueue)
		m_flipsQueuedUp--;

	while(m_pendingRotationRequests > 0) {
		m_ignorePendingRotationRequests += 1; // Sysnc flip requested, but there are still pending Async ones, so flag them to be ignored when completed
		m_pendingRotationRequests--;
		m_data->flip();

		// calls prepareGeometryChange
		resize(m_data->width(), m_data->height());

		m_data->initializePixmap(m_screenPixmap);

		if (m_screenPixmap.isNull()) {
			bool hasAlpha = m_screenPixmap.hasAlpha();
			m_screenPixmap = QPixmap(m_data->width(), m_data->height());
			QColor fillColor(255,255,255,hasAlpha?0:255);
			m_screenPixmap.fill(fillColor);
		}
	}

	if(!m_flipsQueuedUp)
		m_tempRotatedBrush = QBrush(); // clear this

	HostWindow::flipEventSync();

	m_isResizing = (m_flipsQueuedUp != 0);
}

void CardWindow::queueUpWindowForFlip(QRect windowScreenBounds, bool sync)
{
	g_message("ROTATION: [%s]: id: %s, ptr = 0x%x, m_isResizing = %d, m_flipsQueuedUp = %d",
			   __PRETTY_FUNCTION__, m_appId.c_str(), (int)this, m_isResizing, m_flipsQueuedUp);
	if((m_flipsQueuedUp > 0) && (m_flipsQueuedUp%2 != 0)) {
		// cancel a previously queued up request, instead of creating a new one
		WindowServer::instance()->removeWindowFromFlipQueue(this);
		m_flipsQueuedUp = 0;
		queuedFlipCanceled(windowScreenBounds);
		return;
	}

	// save this in case the operation gets canceled
	m_preFlipBoundingRect = m_boundingRect;

	// queue up this card for resizing (flip event) after the rotation animation is complete
	WindowServer::instance()->enqueueWindowForFlip(this, windowScreenBounds, sync);
	// adjust the bounding rect immediately, so the WM can position the card correctly
	m_boundingRect.setRect(-windowScreenBounds.width()/2, -windowScreenBounds.height()/2, windowScreenBounds.width(), windowScreenBounds.height());

	m_isResizing = true;
	m_flipsQueuedUp++;

	// create a temporary brush to paint the window while we wait for the resize
	const QPixmap* pix = acquireScreenPixmap();
	if(pix) {
		if(m_tempRotatedBrush.style() == Qt::NoBrush)
			m_tempRotatedBrush = QBrush(*pix);
		QTransform trans = m_tempRotatedBrush.transform();
		// compensate for the UI rotation to make the card look right while we resize
		trans.rotate( - SystemUiController::instance()->getRotationAngle());
		m_tempRotatedBrush.setTransform(trans);

		m_paintPath = QPainterPath();

		if (m_adjustmentAngle != 90 && m_adjustmentAngle != -90){
			m_paintPath.addRoundedRect(boundingRect(), 25, 25);
		} else {
			m_paintPath.addRoundedRect(QRectF(m_boundingRect.y(), m_boundingRect.x(), m_boundingRect.height(), m_boundingRect.width()), 25, 25);
		}
	}

	// reconstruct shadow
	CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
	if (shadow) {
		bool enable = shadow->isEnabled();
		setGraphicsEffect(0);
		shadow = new CardDropShadowEffect(this);
		setGraphicsEffect(shadow);
		shadow->setEnabled(enable);
	}

	initializeRoundedCornerStage();
}

void CardWindow::queuedFlipCanceled(QRect windowScreenBounds)
{
	g_message("ROTATION: [%s]: id: %s, ptr = 0x%x, m_isResizing = %d, m_flipsQueuedUp = %d",
			   __PRETTY_FUNCTION__, m_appId.c_str(), (int)this, m_isResizing, m_flipsQueuedUp);
	// this window was on a queue to be resized (flip) after the rotation animation, but the request
	// was dropped due to a new rotation start, so we must put things back to normal before that

	// this was a flip (w=h, h=w), so restore the visible dimensions to what they were based on previous data
	m_boundingRect = m_preFlipBoundingRect;

	//restore the paint path to what it was, in case it got changed
	m_paintPath = QPainterPath();
	m_paintPath.addRoundedRect(boundingRect(), 25, 25);

	m_isResizing = false;
	m_flipsQueuedUp--;

	m_tempRotatedBrush = QBrush();

	// reconstruct shadow
	CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
	if (shadow) {
		bool enable = shadow->isEnabled();
		setGraphicsEffect(0);
		shadow = new CardDropShadowEffect(this);
		setGraphicsEffect(shadow);
		shadow->setEnabled(enable);
	}

	if(m_pendingDirectRenderingEnable && !m_isResizing && !m_flipsQueuedUp) {
		// re-enable pending Direct Rendering request
		g_message("ROTATION: [%s]: id: %s, ptr = 0x%x, Enabling Direct Rendering due to previous request.",
				   __PRETTY_FUNCTION__, m_appId.c_str(), (int)this);
		setMaximized(true);
	}

	initializeRoundedCornerStage();
}

void CardWindow::flipEventAsync(QRect windowScreenBounds, bool fromQueue)
{
	g_message("ROTATION: [%s]: fromQueue = %d, id: %s, ptr = 0x%x, m_isResizing = %d, m_flipsQueuedUp = %d",
			   __PRETTY_FUNCTION__, fromQueue, m_appId.c_str(), (int)this, m_isResizing, m_flipsQueuedUp);
	if (!m_channel || !m_data) {
		return;
	}

	if(fromQueue)
		m_flipsQueuedUp--;

	int newWidth = m_bufHeight;
	int newHeight = m_bufWidth;

	m_isResizing = true;
	m_pendingRotationRequests++;

	m_channel->sendAsyncMessage(new View_AsyncFlip(routingId(), newWidth, newHeight, windowScreenBounds.width(), windowScreenBounds.height()));

	m_bufWidth = newWidth;
	m_bufHeight = newHeight;

	if(!fromQueue) {
		m_boundingRect.setRect(-windowScreenBounds.width()/2, -windowScreenBounds.height()/2, windowScreenBounds.width(), windowScreenBounds.height());
		setVisibleDimensions(windowScreenBounds.width(), windowScreenBounds.height());
	}

	// reconstruct shadow
	CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
	if (shadow) {
		bool enable = shadow->isEnabled();
		setGraphicsEffect(0);
		shadow = new CardDropShadowEffect(this);
		setGraphicsEffect(shadow);
		shadow->setEnabled(enable);
	}

	initializeRoundedCornerStage();
}

void CardWindow::asynchFlipCompleted(int newWidth, int newHeight, int newScreenWidth, int newScreenHeight)
{
	g_message("ROTATION: [%s]: id: %s, ptr = 0x%x, w = %d, h = %d, screenBounds = (%d, %d), m_isResizing = %d, m_flipsQueuedUp = %d, m_pendingRotationRequests = %d, m_ignorePendingRotationRequests = %d",
			   __PRETTY_FUNCTION__, m_appId.c_str(), (int)this, newWidth, newHeight, newScreenWidth, newScreenHeight,
			   m_isResizing, m_flipsQueuedUp, m_pendingRotationRequests, m_ignorePendingRotationRequests);

	if(m_ignorePendingRotationRequests > 0) {
		g_message("ROTATION: m_ignorePendingRotationRequests = %d. RETURNING\r\n", m_ignorePendingRotationRequests);
		m_ignorePendingRotationRequests--;
		return;
	}

	m_pendingRotationRequests--;
	if(m_pendingRotationRequests < 0)
		m_pendingRotationRequests = 0;

	m_isResizing = ((m_pendingRotationRequests != 0) || (m_flipsQueuedUp > 0));
	if(!m_isResizing)
		m_tempRotatedBrush = QBrush();

	m_data->flip();

	if(!m_isResizing) {
		// calls prepareGeometryChange
		resize(newWidth, newHeight);
	}

	m_data->initializePixmap(m_screenPixmap);

	if (m_screenPixmap.isNull()) {
		bool hasAlpha = m_screenPixmap.hasAlpha();
		m_screenPixmap = QPixmap(m_data->width(), m_data->height());
		QColor fillColor(255,255,255,hasAlpha?0:255);
		m_screenPixmap.fill(fillColor);
	}

	if(m_pendingDirectRenderingEnable && !m_isResizing && !m_flipsQueuedUp) {
		// re-enable pending Direct Rendering request
		g_message("ROTATION: [%s]: id: %s, ptr = 0x%x, Enabling Direct Rendering due to previous request.",
				   __PRETTY_FUNCTION__, m_appId.c_str(), (int)this);
		setMaximized(true);
	}

	onUpdateFullWindow();
}

void CardWindow::queueFocusAction(bool focused)
{
	if (focused) {
		m_pendingFocus = PendingFocusTrue;
	}
	else {
		m_pendingFocus = PendingFocusFalse;
		IMEController::instance()->removeClient(this);
	}
}

void CardWindow::performPendingFocusAction()
{
	if (m_pendingFocus == PendingFocusNone)
		return;

	if (m_pendingFocus == PendingFocusTrue) {
		if(!isInValidOrientation()) {
			m_focusPendingRotation = true;
			return;
		}

		if (!m_focused || m_forceFocus) {
			focusEvent(true);
		}
        else if (IMEController::instance()->client() != this) {
            // make sure that we are the active input client.
            // this can happen if an app gets rapid pending defocus/focus changes
            IMEController::instance()->setClient(this);
        }

		if (!hasFocus()) {
			setFocus();
		}
	}
	else {
		if (m_focused || m_forceFocus) {
			focusEvent(false);
		}

		if (hasFocus()) {
			clearFocus();
		}
	}

	m_pendingFocus = PendingFocusNone;
}

void CardWindow::focusEvent(bool enable)
{
	g_warning("Sending focus Event to app: %s: %d",
			  appId().c_str(), enable);

	setAcceptTouchEvents(enable && m_touchEventsEnabled);	

	if (!enable && (m_capturedEvents != CapturedNone)) {

		if (m_capturedEvents & CapturedPen) {

			Event e;
			e.type = Event::PenCancel;
			e.setMainFinger(true);

			inputEvent(&e);
		}

		if (m_capturedEvents & CapturedGesture) {

			Event e;
			e.type = Event::GestureCancel;

			inputEvent(&e);
		}

		m_capturedEvents = CapturedNone;
	}

	if(enable && !isInValidOrientation()) {// only enable if the card is in its required orientation.
		queueFocusAction(true);
		m_focusPendingRotation = true;
		return;
	}

	if (m_channel)
		m_channel->sendAsyncMessage(new View_Focus(routingId(), enable));

	m_focused = enable;
	if (enable) {
		setFocus();
	    IMEController::instance()->setClient(this);
	}
	else {
		clearFocus();
	    IMEController::instance()->removeClient(this);
	}

	m_forceFocus = false;
}

void CardWindow::enableShadow()
{
	CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
	if (shadow == 0) {
		shadow = new CardDropShadowEffect(this);
		setGraphicsEffect(shadow);
	}
	else {
		shadow->setEnabled(true);
	}
}

void CardWindow::disableShadow()
{
	CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
	if (shadow)
		shadow->setEnabled(false);
}

void CardWindow::setWindowProperties(const WindowProperties& props)
{
	m_winProps.merge(props);
}

bool CardWindow::rotationLockMaximized() const
{
	return m_winProps.rotationLockMaximized;
}

bool CardWindow::allowResizeOnPositiveSpaceChange() const
{
	return m_winProps.allowResizeOnPositiveSpaceChange;
}

void CardWindow::adjustForPositiveSpaceSize(int width, int height) const
{
    if (m_channel)
		m_channel->sendAsyncMessage(new View_AdjustForPositiveSpace(routingId(), width, height));
}

bool CardWindow::coversScreenFully() const
{
	return sceneBoundingRect() == scene()->sceneRect();
}

void CardWindow::enableFullScreen()
{
	// Modal Cards cannot be full screen - ever.
	if (Window::Type_ModalChildWindowCard == type()) {
		m_fullScreenEnabled = true;
		return;
	}

	// always toggle the rounded corners when we enable full screen since this
	// refers to CardWindow maximized now
	if (Settings::LunaSettings()->uiType == Settings::UI_LUNA) {
		WindowServerLuna* ws = static_cast<WindowServerLuna*>(WindowServer::instance());
		static_cast<MenuWindowManager*>(ws->menuWindowManager())->showOrHideRoundedCorners(!coversScreenFully());
	}

	if (m_fullScreenModeSuppressed) {
		g_message("Full screen mode suppressed. Not enabling full screen");
		return;
	}

	if (m_fullScreenEnabled)
		return;

	m_fullScreenEnabled = true;

	fullScreenEnabled(true);
}

void CardWindow::disableFullScreen()
{
	if (!m_fullScreenEnabled)
		return;

	m_fullScreenEnabled = false;

	if (Settings::LunaSettings()->uiType == Settings::UI_LUNA) {
		WindowServerLuna* ws = static_cast<WindowServerLuna*>(WindowServer::instance());
		static_cast<MenuWindowManager*>(ws->menuWindowManager())->showOrHideRoundedCorners(true);
	}

	fullScreenEnabled(false);
}

void CardWindow::fullScreenEnabled(bool val)
{
	if (!m_channel || !m_data)
		return;

	PIpcBuffer* metaDataBuffer = m_data->metaDataBuffer();
	if (!metaDataBuffer)
		return;

	WindowMetaData* metaData = (WindowMetaData*) metaDataBuffer->data();

    if (val) {
		g_message("Allowing direct rendering");
		QPoint pt = mapToScene(QPointF(m_visibleBounds.x(), m_visibleBounds.y())).toPoint();

		int uiOrientation = WindowServer::instance()->getUiOrientation();
		const HostInfo& info = HostBase::instance()->getInfo();

		switch (uiOrientation) {
        case OrientationEvent::Orientation_Down:
			pt = QPoint(info.displayWidth - pt.x(), info.displayHeight - pt.y());
			break;
        case OrientationEvent::Orientation_Left:
			pt = QPoint(pt.y(), info.displayWidth - pt.x());
			break;
        case OrientationEvent::Orientation_Right:
			pt = QPoint(info.displayHeight - pt.y(), pt.x());
			break;
		default:
			break;
		}

		metaDataBuffer->lock();
		metaData->allowDirectRendering = true;
		metaData->directRenderingScreenX = pt.x();
		metaData->directRenderingScreenY = pt.y();
		metaData->directRenderingOrientation = uiOrientation;
		metaDataBuffer->unlock();

		AppDirectRenderingArbitrator::setLayerEnabled(this, true);

		SharedGlobalProperties* globalProps = WindowServer::globalProperties();
		g_atomic_int_set(&globalProps->directRenderingWindowKey, routingId());

		m_channel->sendAsyncMessage(new View_DirectRenderingChanged(routingId()));
	}
	else {
		g_message("Disallowing direct rendering");

		int currDirectRenderingScreenX;
		int currDirectRenderingScreenY;
		int currDirectRenderingOrientation;

		metaDataBuffer->lock();
		metaData->allowDirectRendering = false;
		currDirectRenderingScreenX = metaData->directRenderingScreenX;
		currDirectRenderingScreenY = metaData->directRenderingScreenY;
		currDirectRenderingOrientation = metaData->directRenderingOrientation;
		metaData->directRenderingScreenX = 0;
		metaData->directRenderingScreenY = 0;
        metaData->directRenderingOrientation = OrientationEvent::Orientation_Up;
		metaDataBuffer->unlock();

		if (m_data)
			m_data->updateFromAppDirectRenderingLayer(currDirectRenderingScreenX,
													  currDirectRenderingScreenY,
													  currDirectRenderingOrientation);

		AppDirectRenderingArbitrator::setLayerEnabled(this, false);

		SharedGlobalProperties* globalProps = WindowServer::globalProperties();
		g_atomic_int_set(&globalProps->directRenderingWindowKey, 0);

		// FIXME: We should update the offscreen directly from Framebuffer1
		m_channel->sendAsyncMessage(new View_DirectRenderingChanged(routingId()));
	}
}

bool CardWindow::delayPrepare()
{
	if (!m_prepareAddedToWm && !hasLoadingTimer()) {
		startLoadingTimer(AS(cardPrepareAddDuration));
	}
	return !m_prepareAddedToWm;
}

void CardWindow::startLoadingOverlay()
{
	if (!m_loadingAnim && type() == Window::Type_Card) {

		m_loadingAnim = new CardLoading(this);
		connect(m_loadingAnim, SIGNAL(signalLoadingFinished()),
				this, SLOT(slotLoadingFinished()));
	}
}

void CardWindow::stopLoadingOverlay()
{
	if (m_loadingAnim) {

		WindowManagerBase* wm = WindowServer::instance()->windowManagerForWindow(this);
		bool transition = wm ? wm->boundingRect().intersects(wm->mapRectFromItem(this, boundingRect())) : false;

		if (transition) {
			m_loadingAnim->finish();
		}
		else {
			delete m_loadingAnim;
			m_loadingAnim = 0;
		}
	}
}

void CardWindow::slotLoadingFinished()
{
	if (m_loadingAnim) {
		delete m_loadingAnim;
		m_loadingAnim = 0;
	}
}

void CardWindow::slotDisplayStateChanged(int state)
{
	if (state == DISPLAY_SIGNAL_OFF) {
		displayOff();
	}
	else if (state == DISPLAY_SIGNAL_DIM) {
		displayDimmed();
	}
}

void CardWindow::startLoadingTimer(int duration)
{
	stopLoadingTimer();

	m_loadingTimerId = g_timeout_add_full(G_PRIORITY_HIGH, duration,
							&CardWindow::loadingTimeout, this, NULL);
}

gboolean CardWindow::loadingTimeout(gpointer data)
{
	CardWindow* win = static_cast<CardWindow*>(data);
	WindowManagerBase* wm = WindowServer::instance()->windowManagerForWindow(win);
	luna_assert(wm != 0);

	if (win->removed()) {
		win->stopLoadingTimer();
		return FALSE;
	}

	if (!win->prepareAddedToWindowManager()) {

		// add it and reset our timer
		win->setPrepareAddedToWindowManager();

		win->stopLoadingTimer();

		wm->prepareAddWindow(win);

		if (win->addedToWindowManager()) {
			// application loaded before our timeout, have a cookie!
			win->stopLoadingOverlay();

			wm->addWindow(win);
		}
		else {

			// still waiting for the window to get added
			if(Window::Type_ModalChildWindowCard != win->type())
				win->startLoadingTimer(AS(cardAddMaxDuration));
			else
				win->startLoadingTimer(AS(modalCardAddMaxDuration));

			// only need to kick off the loading animation if we need to wait
			// longer than our initial timeout
			win->startLoadingOverlay();
		}
	}
	else {
		win->stopLoadingTimer();

		if (!win->addedToWindowManager()) {

			// timed out waiting for the addWindow
			wm->addWindowTimedOut(win);
		}
	}
	return FALSE;
}

void CardWindow::stopLoadingTimer()
{
	if (m_loadingTimerId > 0) {
		g_source_remove(m_loadingTimerId);
		m_loadingTimerId = 0;
	}
}

void CardWindow::setVisibleDimensions(int width, int height)
{
	if(!m_isResizing && m_flipsQueuedUp) {
		if (m_data && !isHost() &&
			(type() != Window::Type_ModalChildWindowCard) && (m_appFixedOrientation == Event::Orientation_Invalid)) {
			// safeguard code in case the data buffer and the bounding rect dimensions get out of sync
			bool isDataLandscape = m_data->width() >= m_data->height();
			bool newDimLandscape = width >= height;
			bool isBoundingRectLandscape = m_boundingRect.width() >= m_boundingRect.height();

			if(isDataLandscape != newDimLandscape) {
				// problem here!! something got out os sync, so try to remediate using the buffer data sizes
				width = m_data->width();
				height = m_data->height() - (fullScreen() ? 0 : Settings::LunaSettings()->positiveSpaceTopPadding);
			}

			if(isDataLandscape != isBoundingRectLandscape) {
				// problem here!! something got out os sync, so try to remediate using the buffer data sizes
				m_boundingRect.setRect(-width/2, -height/2, width, height);
				CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
				if (shadow)
					shadow->cacheDrawingData();
			}
		}
	}

	Window::setVisibleDimensions(width, height);

	if (G_UNLIKELY(m_boundingRect.isNull())) {
		m_boundingRect = m_visibleBounds;
	}

	// NOTE: it would be preferable to use QGraphicsEffect::sourceChanged(flags)
	// but after experimenting, it seems that it is incorrectly notifying us that the
	// source bounding rect has actually changed.
	CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
	if (shadow)
		shadow->cacheDrawingData();

	m_paintPath = QPainterPath();

	if ((m_adjustmentAngle != 90 && m_adjustmentAngle != -90) || this->type() == Window::Type_ModalChildWindowCard) {
                m_paintPath.addRoundedRect(boundingRect(), 8, 6); //where you alter the loading rect stuff
	} else {
		m_paintPath.addRoundedRect(QRectF(m_boundingRect.y(), m_boundingRect.x(), m_boundingRect.height(), m_boundingRect.width()), 25, 25);
	}

	initializeRoundedCornerStage();
}

void CardWindow::setPaintCompositionMode(QPainter::CompositionMode mode)
{
	m_compMode = mode;
}

void CardWindow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
	QPainter::CompositionMode previous = painter->compositionMode();
	if (opacity() == 1.0)
		painter->setCompositionMode(m_compMode);

        if (G_UNLIKELY(m_loadingAnim)) {
		if (m_loadingAnim->finishing()) {
			paintBase(painter, m_maximized);
		}
		if(m_adjustmentAngle)
			painter->rotate(m_adjustmentAngle);
		// temporarily enable blending
                m_loadingAnim->paint(painter, m_maximized||m_maxAndLoading);
		if(m_adjustmentAngle)
			painter->rotate(-m_adjustmentAngle);
	} else {
		paintBase(painter, m_maximized);
	}

	paintOverlay(painter, m_maximized);

	// If this card is a modal parent, draw black background with 60% opacity.
	if(m_isCardModalParent) {
		qreal curOpacity = painter->opacity();
		painter->setOpacity(0.60);
		painter->fillRect(boundingRect(), QColor(0x0f,0x0f,0x0f,0xff));
		painter->setOpacity(curOpacity);
	}

	painter->setCompositionMode(previous);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
}

void CardWindow::paintBase(QPainter* painter, bool maximized)
{
    if (maximized) {

        // faster, rectangular blit
        painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

        if(m_adjustmentAngle)
            painter->rotate(m_adjustmentAngle);

        Window::paint(painter, 0, 0);

        if(m_adjustmentAngle)
            painter->rotate(-m_adjustmentAngle);

        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    }
    else {
        // draw with rounded corners
        const QPixmap* pix = acquireScreenPixmap();
        if (pix) {
            QRectF brect = boundingRect();
#if defined(USE_ROUNDEDCORNER_SHADER)
            if(G_LIKELY(!m_isResizing)) {

                initializeRoundedCornerStage();

                // We don't need this for the modal card
                if(Window::Type_ModalChildWindowCard != type())
                    m_roundedCornerShaderStage->setOnPainter(painter);

                painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

                if(m_adjustmentAngle) {
                    painter->rotate(m_adjustmentAngle);
                }

                // If we are modal card and have resized, then our position is not (-w/2, -h/2)
                if((Window::Type_ModalChildWindowCard != type()) || (Window::Type_ModalChildWindowCard == type() && boundingRect().height() == Settings::LunaSettings()->modalWindowHeight)) {
                    if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90) {
                        painter->drawPixmap(-brect.height()/2, -brect.width()/2, *pix);
                    } else {
                        painter->drawPixmap(-brect.width()/2, -brect.height()/2, *pix);
                    }
                }
                else {
                    if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90) {
                        painter->drawPixmap((CardWindow::sBoundingRectYBeforeResize + ((pos().y() - CardWindow::sModalWindowYBeforeResize.y()))), -brect.width()/2, *pix);
                    } else {
                        painter->drawPixmap(-brect.width()/2, (CardWindow::sBoundingRectYBeforeResize + ((pos().y() - CardWindow::sModalWindowYBeforeResize.y()))), *pix);
                        CardWindow::sModalWindowYBeforeResize.setY(pos().y());
                    }
                }

                if(m_adjustmentAngle) {
                    painter->rotate(-m_adjustmentAngle);
                }

                // We don't need this for the modal card
                if(Window::Type_ModalChildWindowCard != type())
                    m_roundedCornerShaderStage->removeFromPainter(painter);

            } else {
#endif
                if(m_adjustmentAngle) {

                    painter->rotate(m_adjustmentAngle);
                    if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90){
                        if (fullScreen())
                            painter->setBrushOrigin(brect.y(), brect.x() - (pix->height() - brect.height()) / 2);
                        else
                            painter->setBrushOrigin(brect.y(), brect.x());
                    } else {
                        if (fullScreen())
                            painter->setBrushOrigin(brect.x(), brect.y() - (pix->height() - brect.height()) / 2);
                        else
                            painter->setBrushOrigin(brect.x(), brect.y());
                    }
                } else {
                    if (fullScreen())
                        painter->setBrushOrigin(brect.x(), brect.y() - (pix->height() - brect.height()) / 2);
                    else
                        painter->setBrushOrigin(brect.x(), brect.y());
                }

                if(!m_isResizing) {
                    painter->fillPath(m_paintPath, *pix);
                } else {
                    painter->fillPath(m_paintPath, m_tempRotatedBrush);
                }

                painter->setBrushOrigin(0, 0);
                if(m_adjustmentAngle)
                    painter->rotate(-m_adjustmentAngle);
#if defined(USE_ROUNDEDCORNER_SHADER)
            }
#endif
        }
    }
}

void CardWindow::keyPressEvent(QKeyEvent* event)
{
	if (m_channel)
		m_channel->sendAsyncMessage(new View_KeyEvent(routingId(), event));
}

void CardWindow::keyReleaseEvent(QKeyEvent* event)
{
	if (m_channel)
		m_channel->sendAsyncMessage(new View_KeyEvent(routingId(), event));
}

bool CardWindow::touchEvent(QTouchEvent* event)
{
	if (!m_focused || m_pendingFocus == PendingFocusFalse) {
		return false;
	}

	if (EventThrottler::instance()->shouldDropEvent(event)) {
		return true;
	}

    if (m_channel) {
		QRectF br = boundingRect();
		m_channel->sendAsyncMessage(new View_TouchEvent(routingId(), SysMgrTouchEvent(event, -br.x(), -br.y())));
    }
	
    return true;
}

void CardWindow::setPosition(const CardWindow::Position& pos)
{
	m_position = pos;
    setTransform(m_position.toTransform());
#if defined(USE_ROUNDEDCORNER_SHADER)
    if (m_roundedCornerShaderStage) {
        m_roundedCornerShaderStage->setScale(pos.trans.z());
    }
    if(m_loadingAnim) {
         m_loadingAnim->setScale(pos.trans.z());
    }
#endif
}

void CardWindow::setMaximized(bool enable)
{
	if(enable && !isInValidOrientation()) {
		// only enable if the card is in its required orientation.
		SystemUiController::instance()->setRequestedSystemOrientation(m_appFixedOrientation, true, SystemUiController::instance()->isInEmergencyMode());
		return;
	}

	if(enable && (m_isResizing || m_flipsQueuedUp)) {
		// defer this until we are done rotating/resizing
		m_pendingDirectRenderingEnable = true;
		g_message("ROTATION: [%s]: Deferred Direct Rendering Enable due to pending rotation. id: %s, ptr = 0x%x, m_isResizing = %d, m_flipsQueuedUp = %d",
				   __PRETTY_FUNCTION__, m_appId.c_str(), (int)this, m_isResizing , m_flipsQueuedUp);

		return;
	}

	m_pendingDirectRenderingEnable = false;

	m_maximized = enable;
	if (m_maximized && !m_fullScreenEnabled) {
		enableFullScreen();
	}
	else if (!m_maximized && m_fullScreenEnabled) {
		disableFullScreen();
	}

	if(m_maximized && m_focusPendingRotation) {
		m_focusPendingRotation = false;
		performPendingFocusAction();
	}
}

void CardWindow::setMaxAndLoading(bool enable)
{
    m_maxAndLoading=enable;

}

bool CardWindow::isInValidOrientation()
{
	if(m_appFixedOrientation == Event::Orientation_Invalid)
		return true;

    OrientationEvent::Orientation sysOrient = WindowServer::instance()->getUiOrientation();

	if(m_appFixedOrientation == Event::Orientation_Up){
            return (sysOrient == OrientationEvent::Orientation_Up);
	}

	if(m_appFixedOrientation == Event::Orientation_Down){
            return (sysOrient == OrientationEvent::Orientation_Down);
	}

	if(m_appFixedOrientation == Event::Orientation_Left){
            return (sysOrient == OrientationEvent::Orientation_Left);
	}

	if(m_appFixedOrientation == Event::Orientation_Right){
            return (sysOrient == OrientationEvent::Orientation_Right);
	}

	if(m_appFixedOrientation == Event::Orientation_Landscape){
		if(WindowServer::instance()->deviceIsPortraitType()) {
            return (sysOrient == OrientationEvent::Orientation_Left || sysOrient == OrientationEvent::Orientation_Right);
		} else {
            return (sysOrient == OrientationEvent::Orientation_Up || sysOrient == OrientationEvent::Orientation_Down);
		}
	}

	if(m_appFixedOrientation == Event::Orientation_Portrait){
		if(WindowServer::instance()->deviceIsPortraitType()) {
            return (sysOrient == OrientationEvent::Orientation_Up || sysOrient == OrientationEvent::Orientation_Down);
		} else {
            return (sysOrient == OrientationEvent::Orientation_Left || sysOrient == OrientationEvent::Orientation_Right);
		}
	}

	return false;
}

void CardWindow::setModalChild(CardWindow* w)
{
	m_modalChild = w;

	// reset all position info about the modal if the modal was dismissed
	if(NULL == m_modalChild)
		resetModalWindowPositionInfo();
}

void CardWindow::setModalParent(CardWindow* parent)
{
	if (Window::Type_ModalChildWindowCard != type() || NULL == parent)
		return;

	// Set the parent item for the modal card.
	this->setParentItem(parent);

	// Save off a copy for easy access
	m_modalParent = parent;
}

QPointF CardWindow::positionModalWindowWrpParent(int spaceAvailableAbove, int windowHeight)
{
	if(!m_modalParent || Window::Type_ModalChildWindowCard != type())
		return QPointF(0,0);

	// This assumes that we have the entire +ve space and we position the modal accordingly
	if(-1 == spaceAvailableAbove && -1 == windowHeight) {
		return QPointF(m_modalParent->pos().x(), (m_modalParent->boundingRect().y() + (boundingRect().height()/2 + m_initMWindPlacementInf.m_positiveSpaceThreshold_Above)));
	}

	return QPointF(m_modalParent->pos().x(), (m_modalParent->boundingRect().y() + (windowHeight/2 + spaceAvailableAbove)));
}

// This assumes that the height of the modal window is or will be 480 (Settings::LunaSettings()->modalWindowHeight)
void CardWindow::centerModal(int availablePositiveSpace)
{
	// set the flag that we need to center the modal
	m_initMWindPlacementInf.m_centerModalWindow = true;

	m_initMWindPlacementInf.m_positiveSpaceThreshold_Above = (availablePositiveSpace - Settings::LunaSettings()->modalWindowHeight)/2;
	m_initMWindPlacementInf.m_positiveSpaceThreshold_Below = m_initMWindPlacementInf.m_positiveSpaceThreshold_Above;

	// Center is (0,0) So the modal is moving by pos().y() units.
	m_initMWindPlacementInf.setCardMoveDelta((-1) * pos().y());

	// could've just said 0,0, but lets just make sure
	m_ModalWindowEndY = positionModalWindowWrpParent(((availablePositiveSpace - Settings::LunaSettings()->modalWindowHeight)/2), Settings::LunaSettings()->modalWindowHeight).y();
}

void CardWindow::positionModalForLessPositiveSpace(int availablePosSpace, int newPositiveSpace)
{
	// What we need to do depends on whether the card was resized
	if(Settings::LunaSettings()->modalWindowHeight == boundingRect().height()) {

		// This is the final ending position of the card
		int endY = (m_initMWindPlacementInf.m_positiveSpaceThreshold_Below - m_initMWindPlacementInf.m_negativeSpaceConsumed);
		int unsignedEndY = (endY < 0) ? (-1 * endY) : endY;

		// Check if we need to move the card at all.
		if((m_initMWindPlacementInf.m_negativeSpaceConsumed <= m_initMWindPlacementInf.m_positiveSpaceThreshold_Below) || (unsignedEndY <= m_initMWindPlacementInf.m_positiveSpaceThreshold_Above)) {

			// We know the new positive space and the height of the card. The space above will be (newPosSpace - height/2). Negative space is all the remaining space.
			m_initMWindPlacementInf.m_positiveSpaceThreshold_Above = (newPositiveSpace - boundingRect().height())/2;
			m_initMWindPlacementInf.m_positiveSpaceThreshold_Below = (availablePosSpace - (boundingRect().height() + m_initMWindPlacementInf.m_positiveSpaceThreshold_Above));

			// Get the final resting position of the card
			m_ModalWindowEndY = positionModalWindowWrpParent(m_initMWindPlacementInf.m_positiveSpaceThreshold_Above, boundingRect().height()).y();

			// Save the space that the modal window has moved from it's current position.
			m_initMWindPlacementInf.setCardMoveDelta(pos().y() + m_ModalWindowEndY);
		}
		else if (m_initMWindPlacementInf.m_negativeSpaceConsumed > m_initMWindPlacementInf.m_positiveSpaceThreshold_Below) {
			if (unsignedEndY > m_initMWindPlacementInf.m_positiveSpaceThreshold_Above) {
				// Set the flag that the height of the modal needs to be reduced.
				m_initMWindPlacementInf.m_ModalCardResizeInf = ReduceCardHeight;
				// Calculate the height we need to reduce the height of the modal.  [m_modalWindowShrinkHeight = m_initMWindPlacementInf.m_originalY - m_ModalWindowEndY;]
				m_modalWindowShrinkHeight = m_initMWindPlacementInf.m_negativeSpaceConsumed - (m_initMWindPlacementInf.m_positiveSpaceThreshold_Above + m_initMWindPlacementInf.m_positiveSpaceThreshold_Below);
				// Save the space that the modal window has moved from it's current position.
				m_initMWindPlacementInf.setCardMoveDelta(m_initMWindPlacementInf.m_positiveSpaceThreshold_Above);

				// In this case, we would have moved the card up by m_initMWindPlacementInf.m_positiveSpaceThreshold_Above and then resized the card. At the end, we'll have 0 space above.
				m_ModalWindowEndY = pos().y() - m_initMWindPlacementInf.m_positiveSpaceThreshold_Above;
				m_initMWindPlacementInf.m_positiveSpaceThreshold_Above = 0;
				m_initMWindPlacementInf.m_positiveSpaceThreshold_Below = (availablePosSpace - (Settings::LunaSettings()->modalWindowHeight - m_modalWindowShrinkHeight));
			}
		}
	}
	else {
		// We never increase the height past Settings::LunaSettings()->modalWindowHeight, so the card height is already reduced and we have less positive space than before
		m_initMWindPlacementInf.m_ModalCardResizeInf = ReduceCardHeight;
		decreaseHeightAndPositionModalCard(availablePosSpace, newPositiveSpace);
	}

	// Finally check if you can place the window @ m_ModalWindowEndY and still keep the card in the screen
	int correctYLoc = 0;
	if(false == canPositionModalAtY(m_ModalWindowEndY, false, correctYLoc)) {
		// The card doesn't have to move as previously computed
		m_initMWindPlacementInf.m_cardMoveDelta += (m_ModalWindowEndY - correctYLoc);
		m_ModalWindowEndY = correctYLoc;
	}
}

void CardWindow::positionModalForMorePositiveSpace(int availablePosSpace, int newPositiveSpace)
{
	// What we need to do depends on whether the modal was resized.

	int netPosSpaceGained = (newPositiveSpace - availablePosSpace);
	int unsignedPosSpaceGained = (netPosSpaceGained < 0)? (-1 * netPosSpaceGained) : netPosSpaceGained;

	// CASE 1 - Modal was NOT resized
	if(boundingRect().height() == Settings::LunaSettings()->modalWindowHeight) {

		// We've gotten back all of the +ve space - just center the modal window again.
		if(0 == unsignedPosSpaceGained) {
			centerModal(availablePosSpace);
		}
		else {
			// We know the new positive space and the height of the card. The space above will be (newPosSpace - height/2). Negative space is all the remaining space.
			m_initMWindPlacementInf.m_positiveSpaceThreshold_Above = (newPositiveSpace - boundingRect().height())/2;
			m_initMWindPlacementInf.m_positiveSpaceThreshold_Below = (availablePosSpace - (boundingRect().height() + m_initMWindPlacementInf.m_positiveSpaceThreshold_Above));

			// Get the final resting position of the card
			m_ModalWindowEndY = positionModalWindowWrpParent(m_initMWindPlacementInf.m_positiveSpaceThreshold_Above, boundingRect().height()).y();

			// Save the space that the modal window has moved from it's current position.
			m_initMWindPlacementInf.setCardMoveDelta(pos().y() + m_ModalWindowEndY);
		}
	}
	else {

		m_initMWindPlacementInf.m_ModalCardResizeInf = IncreaseCardHeight;

		// SPL case, if we have all the positive space back
		if(0 == unsignedPosSpaceGained) {
			centerModal(availablePosSpace);
			// Get the amount by which we can increase the height
			m_modalWindowShrinkHeight = Settings::LunaSettings()->modalWindowHeight - boundingRect().height();
		}
		else {
			increaseHeightAndPositionModalCard(availablePosSpace, newPositiveSpace);
		}
	}

	// Finally check if you can place the window @ m_ModalWindowEndY and still keep the card centered
	int correctYLoc = 0;
	if(false == canPositionModalAtY(m_ModalWindowEndY, true, correctYLoc)) {
		// The card doesn't have to move as previously computed
		m_initMWindPlacementInf.m_cardMoveDelta += (m_ModalWindowEndY - correctYLoc);
		m_ModalWindowEndY = correctYLoc;
	}
}

void CardWindow::decreaseHeightAndPositionModalCard(int availablePosSpace, int newPositiveSpace)
{
	m_initMWindPlacementInf.m_ModalCardResizeInf = ReduceCardHeight;

	// We need to decrease the height of the card by this extent
	m_modalWindowShrinkHeight = boundingRect().height() - newPositiveSpace;

	// Positive space increased - make sure m_modalWindowShrinkHeight is atleast 0
	if(m_modalWindowShrinkHeight < 0)
		m_modalWindowShrinkHeight = 0;

	// Recompute the space above and below the card based on the new space/card height - at this point we need them to be the same
	int newCardHeight = boundingRect().height() - m_modalWindowShrinkHeight;

	int spaceAboveOld = m_initMWindPlacementInf.m_positiveSpaceThreshold_Above;
	int spaceBelowOld = m_initMWindPlacementInf.m_positiveSpaceThreshold_Below;

	// We know the total space, the new positive space/card height. Space below is (total height - (card height + space above))
	m_initMWindPlacementInf.m_positiveSpaceThreshold_Above = (newPositiveSpace - newCardHeight)/2;
	m_initMWindPlacementInf.m_positiveSpaceThreshold_Below = (availablePosSpace - (newCardHeight + m_initMWindPlacementInf.m_positiveSpaceThreshold_Above));

	// Compute the ideal end position based on the new values
	m_ModalWindowEndY = positionModalWindowWrpParent(m_initMWindPlacementInf.m_positiveSpaceThreshold_Above, newCardHeight).y();

	// Check if we can position the modal at this location with the reduced height - Check the space above before rotation and after rotation. If we had more space before the size decrease, then we may have to move the card down
	if(true == SystemUiController::instance()->isUiRotating() && spaceAboveOld >= m_initMWindPlacementInf.m_positiveSpaceThreshold_Above) {
		m_ModalWindowEndY += m_modalWindowShrinkHeight/2;
	}
	else if(false == SystemUiController::instance()->isUiRotating()) {
		if(m_initMWindPlacementInf.m_positiveSpaceThreshold_Above > m_modalWindowShrinkHeight/2)
			m_ModalWindowEndY -= m_modalWindowShrinkHeight/2;
		else if(m_initMWindPlacementInf.m_positiveSpaceThreshold_Above > 0 && m_initMWindPlacementInf.m_positiveSpaceThreshold_Above < m_modalWindowShrinkHeight/2)
			m_ModalWindowEndY -= m_initMWindPlacementInf.m_positiveSpaceThreshold_Above;
		else
			m_ModalWindowEndY = pos().y();
	}

	// Set how much the card would have moved
	m_initMWindPlacementInf.setCardMoveDelta((-1) * (pos().y() - m_ModalWindowEndY));
}

void CardWindow::increaseHeightAndPositionModalCard(int availablePosSpace, int newPositiveSpace)
{
	m_initMWindPlacementInf.m_ModalCardResizeInf = IncreaseCardHeight;

	// We can increase the card height by this value.
	if(newPositiveSpace > Settings::LunaSettings()->modalWindowHeight) {
		m_modalWindowShrinkHeight = Settings::LunaSettings()->modalWindowHeight - boundingRect().height();
	}
	// If newPositiveSpace is less than boundingRect().height() and this function gets called, there is an error somewhere else :(
	else if(newPositiveSpace > boundingRect().height()) {
		m_modalWindowShrinkHeight = newPositiveSpace - boundingRect().height();
	}

	// Positive space increased - make sure m_modalWindowShrinkHeight is atleast 0
	if(m_modalWindowShrinkHeight < 0)
		m_modalWindowShrinkHeight = 0;

	// Recompute the space above and below the card based on the new space/card height - at this point we need them to be the same
	int newCardHeight = boundingRect().height() + m_modalWindowShrinkHeight;

	int spaceAboveOld = m_initMWindPlacementInf.m_positiveSpaceThreshold_Above;
	int spaceBelowOld = m_initMWindPlacementInf.m_positiveSpaceThreshold_Below;

	m_initMWindPlacementInf.m_positiveSpaceThreshold_Above = (newPositiveSpace - newCardHeight)/2;

	// We know the total space, the new positive space/card height. Space below is (total height - (card height + space above))
	m_initMWindPlacementInf.m_positiveSpaceThreshold_Below = (availablePosSpace - (newCardHeight + m_initMWindPlacementInf.m_positiveSpaceThreshold_Above));

	m_ModalWindowEndY = positionModalWindowWrpParent(m_initMWindPlacementInf.m_positiveSpaceThreshold_Above, newCardHeight).y();

	// Based on the above computations, ideally m_ModalWindowEndY is the new end position. Check if we can place the card here with the increased height
	if((newCardHeight + m_initMWindPlacementInf.m_positiveSpaceThreshold_Above) >= newPositiveSpace) {
		m_ModalWindowEndY = pos().y();
	}

	// Set how much the card would have moved
	m_initMWindPlacementInf.setCardMoveDelta((-1) * (pos().y() - m_ModalWindowEndY));
}

bool CardWindow::canPositionModalAtY(int yLoc, bool increasePositiveSpace, int& correctYLoc)
{
	correctYLoc = 0;

	// we have more positive space - i.e the card is moving down. the max it can be at is 0
	if(increasePositiveSpace) {
		if(yLoc > 0) {
			correctYLoc = 0;
			return false;
		}
		return true;
	}
	else {
		// Positive space decreased - we are moving the card up. m_maxEndingPositionForOrientation will have the max you can position the card and still be within the screen.
		if(yLoc < m_maxEndingPositionForOrientation) {
			correctYLoc = m_maxEndingPositionForOrientation;
			return false;
		}
		return true;
	}
	return false;
}

void CardWindow::computeModalWindowPlacementInf(int newPosSpace)
{
	if (Window::Type_ModalChildWindowCard != type())
		return;

	SystemUiController* sysUi = SystemUiController::instance();

	if(!sysUi)
		return;

	// Determine available positive space - depends on whether the status bar is visible/not visible.
	int availablePosSpace = (true == sysUi->statusBarAndNotificationAreaShown())? (sysUi->currentUiHeight() - Settings::LunaSettings()->positiveSpaceTopPadding) : sysUi->currentUiHeight();

	bool positiveSpaceIncreased = true;

	// These are the initial position(s) and the threshold values when the card is launched or when the card is rotated.
	if (true == m_fRecomputeInitPositionsValues) {

		// reset this flag
		m_fRecomputeInitPositionsValues = false;

		// Check what is the maximum we can position the modal window for that orientation - used ONLY when we are animating the window up. Subtract available space from modalWindowHeight as we are dealing with negative numbers
		m_maxEndingPositionForOrientation = (Settings::LunaSettings()->modalWindowHeight - availablePosSpace)/2;

		// If we are just placing the card for the first time OR positioning the card in the center after rotation just set these values and return
		if(((-1 == m_initMWindPlacementInf.m_positiveSpaceThreshold_Below) && (-1 == m_initMWindPlacementInf.m_positiveSpaceThreshold_Above) && (Invalid == m_initMWindPlacementInf.m_ModalCardResizeInf))
				|| (Settings::LunaSettings()->modalWindowHeight == boundingRect().height() && true == SystemUiController::instance()->isUiRotating() && (0 == pos().x() && 0 == pos().y())))
		{

			// Initially space above the card is (Positive Space - Height of the card)/2. The same is for the space below the card.
			m_initMWindPlacementInf.m_positiveSpaceThreshold_Below = (availablePosSpace - boundingRect().height()) / 2;
			m_initMWindPlacementInf.m_positiveSpaceThreshold_Above = m_initMWindPlacementInf.m_positiveSpaceThreshold_Below;

			// get the initial Y position of the modal window
			m_initMWindPlacementInf.m_originalY = positionModalWindowWrpParent(-1, -1).y();

			// Set that we have centered the card
			m_initMWindPlacementInf.m_ModalCardResizeInf = HeightAndPositionUnchanged;

			// Save off the last known positive space
			CardWindow::sLastKnownPositiveSpace = availablePosSpace;
			return;
		}
		else {

			// Determine if we have more/less positive space than what we cached before
			if(newPosSpace < CardWindow::sLastKnownPositiveSpace) {
				positiveSpaceIncreased = false;
			}
		}
	}

	// ERROR - should never happen, just for sake of completeness
	if(newPosSpace > availablePosSpace)
		return;

	// Get the negative space consumed
	m_initMWindPlacementInf.m_negativeSpaceConsumed = availablePosSpace	- newPosSpace;

	if(false == SystemUiController::instance()->isUiRotating()) {
		// Animating the window up.
		if (newPosSpace < CardWindow::sLastKnownPositiveSpace) {
			positionModalForLessPositiveSpace(availablePosSpace, newPosSpace);
		}
		else {
			// Animating the window down
			positionModalForMorePositiveSpace(availablePosSpace, newPosSpace);
		}
	}
	else {
		// The modal card's height was unchanged before this
		if(Settings::LunaSettings()->modalWindowHeight == boundingRect().height()) {
			if(true == positiveSpaceIncreased && newPosSpace >= Settings::LunaSettings()->modalWindowHeight)
				positionModalForMorePositiveSpace(availablePosSpace, newPosSpace);
			else if(false == positiveSpaceIncreased && newPosSpace >= Settings::LunaSettings()->modalWindowHeight)
				positionModalForLessPositiveSpace(availablePosSpace, newPosSpace);
			else if(false == positiveSpaceIncreased && newPosSpace < Settings::LunaSettings()->modalWindowHeight)
				decreaseHeightAndPositionModalCard(availablePosSpace, newPosSpace);
		}
		else if(boundingRect().height() < Settings::LunaSettings()->modalWindowHeight) {
			if(true == positiveSpaceIncreased && newPosSpace > boundingRect().height())
				increaseHeightAndPositionModalCard(availablePosSpace, newPosSpace);
			if(false == positiveSpaceIncreased && newPosSpace < Settings::LunaSettings()->modalWindowHeight)
				decreaseHeightAndPositionModalCard(availablePosSpace, newPosSpace);
		}
	}

	// Save off the last known positive space for the next iteration
	CardWindow::sLastKnownPositiveSpace = newPosSpace;
}

void CardWindow::resetModalWindowPositionInfo()
{
	m_initMWindPlacementInf.reset();

	m_ModalWindowEndY = 0;
	m_modalWindowShrinkHeight = 0;
	m_maxEndingPositionForOrientation = -1;
	m_fRecomputeInitPositionsValues = true;

	CardWindow::sStartSpaceChangeValue = -1;
	CardWindow::sLastKnownPositiveSpace = 0;
	CardWindow::sModalWindowYBeforeResize = QPointF(0, 0);
	CardWindow::sBoundingRectYBeforeResize = 0;
}

void CardWindow::startModalAnimation()
{
	if(pos().y() == m_ModalWindowEndY) {
		return;
	}

	// If an animation is running, stop it
	if(m_ModalPositionAnimation && QAbstractAnimation::Running == m_ModalPositionAnimation->state()) {
		m_ModalPositionAnimation->stop();
	}

	// indicate that we have started the animation
	m_initMWindPlacementInf.m_IsAnimating = true;

	// Prepare the animation
	m_ModalPositionAnimation->setDuration(sModalCardAnimationTimeout);
	m_ModalPositionAnimation->setEasingCurve(AS_CURVE(cardDeleteCurve));
	m_ModalPositionAnimation->setEndValue(m_ModalWindowEndY);

	// start the animation
	m_ModalPositionAnimation->start();
}

void CardWindow::resizeModalCard()
{
	// Check if we need to resize the window
	if((CardHeightNotChanged != m_initMWindPlacementInf.m_ModalCardResizeInf) || (Invalid != m_initMWindPlacementInf.m_ModalCardResizeInf)) {
		// Save off the current modal window position and Y position of the bounding rect.
		CardWindow::sModalWindowYBeforeResize = pos();
		CardWindow::sBoundingRectYBeforeResize = boundingRect().y();

		switch(m_initMWindPlacementInf.m_ModalCardResizeInf) {
		case ReduceCardHeight:
			resizeEventSync(Settings::LunaSettings()->modalWindowWidth, (boundingRect().height() - m_modalWindowShrinkHeight));
			setBoundingRect(Settings::LunaSettings()->modalWindowWidth, (boundingRect().height() - m_modalWindowShrinkHeight));
			break;
		case IncreaseCardHeight:
			resizeEventSync(Settings::LunaSettings()->modalWindowWidth, (boundingRect().height() + m_modalWindowShrinkHeight));
			setBoundingRect(Settings::LunaSettings()->modalWindowWidth, (boundingRect().height() + m_modalWindowShrinkHeight));
			break;
		default:
			break;
		}
	}
}

void CardWindow::positiveSpaceAboutToChange(const QRect& r, bool fullScreen) {
	if (Window::Type_ModalChildWindowCard != this->type())
		updateDirectRenderingPosition();
	else {
		m_posSpChangeNotificationState = GotPositiveSpaceAboutToChangeNotification;
		// Ui is rotating - we need to recompute the new bounds.
		if(true == SystemUiController::instance()->isUiRotating() && false == m_fRecomputeInitPositionsValues)
			m_fRecomputeInitPositionsValues = true;

		computeModalWindowPlacementInf(r.height());
	}
}

void CardWindow::positiveSpaceChanged(const QRect& r) {
	if (Window::Type_ModalChildWindowCard != this->type())
		updateDirectRenderingPosition();
	else {

		// During rotation (coz of a call in CardWindowManager::resize() this API will get called before a call to CardWindow::positiveSpaceAboutToChange. To prevent anything from happening at that call, protect by checking
		if(m_posSpChangeNotificationState < GotPositiveSpaceAboutToChangeNotification)
			return;

		m_posSpChangeNotificationState = GotPositiveSpaceChangeNotification;

		if(false == m_initMWindPlacementInf.m_IsAnimating) {

			bool animateWindow = true;

			// If we are shrinking the modal height during rotation then we will not animate the window - but just position it.
			if(true == SystemUiController::instance()->isUiRotating() && (ReduceCardHeight == m_initMWindPlacementInf.m_ModalCardResizeInf)) {
				this->setPos(pos().x(), m_ModalWindowEndY);
				animateWindow = false;
			}

			// resize the modal card
			resizeModalCard();

			// Start the animation to position the modal to the correct position
			if(true == animateWindow)
				startModalAnimation();

			// Reset this flag
			m_modalWindowShrinkHeight = 0;

			// Reset this flag.
			m_initMWindPlacementInf.m_ModalCardResizeInf = Invalid;
		}
	}
}

void CardWindow::positiveSpaceChangeFinished(const QRect& r)
{
	if (Window::Type_ModalChildWindowCard != this->type())
		updateDirectRenderingPosition();
	else {

		m_posSpChangeNotificationState = Unknown;

		// indicate that we are done animating
		if (true == m_initMWindPlacementInf.m_IsAnimating) {
			m_initMWindPlacementInf.m_IsAnimating = false;
		}
	}

	if (Settings::LunaSettings()->uiType == Settings::UI_LUNA) {
		WindowServerLuna* ws = static_cast<WindowServerLuna*> (WindowServer::instance());
		static_cast<MenuWindowManager*> (ws->menuWindowManager())->showOrHideRoundedCorners(!coversScreenFully());
	}
}

void CardWindow::updateDirectRenderingPosition()
{
	if (!m_maximized || !m_fullScreenEnabled)
		return;
    
	if (!m_channel || !m_data)
		return;

	PIpcBuffer* metaDataBuffer = m_data->metaDataBuffer();
	if (!metaDataBuffer)
		return;

	WindowMetaData* metaData = (WindowMetaData*) metaDataBuffer->data();	

	QPoint pt = mapToScene(QPointF(m_visibleBounds.x(), m_visibleBounds.y())).toPoint();

	int uiOrientation = WindowServer::instance()->getUiOrientation();
	const HostInfo& info = HostBase::instance()->getInfo();

	switch (uiOrientation) {
    case OrientationEvent::Orientation_Down:
		pt = QPoint(info.displayWidth - pt.x(), info.displayHeight - pt.y());
		break;
    case OrientationEvent::Orientation_Left:
		pt = QPoint(pt.y(), info.displayWidth - pt.x());
		break;
    case OrientationEvent::Orientation_Right:
		pt = QPoint(info.displayHeight - pt.y(), pt.x());
		break;
	default:
		break;
	}
	
	metaDataBuffer->lock();

	if (metaData->directRenderingScreenX == pt.x() &&
		metaData->directRenderingScreenY == pt.y() &&
		metaData->directRenderingOrientation == uiOrientation) {
		metaDataBuffer->unlock();
		return;
	}
	
	metaData->directRenderingScreenX = pt.x();
	metaData->directRenderingScreenY = pt.y();
	metaData->directRenderingOrientation = uiOrientation;
	metaDataBuffer->unlock();

	//printf("Updating direct rendering: %d, %d, %d\n", pt.x(), pt.y(),
	//	   WindowServer::instance()->getUiOrientation());
	
	m_channel->sendAsyncMessage(new View_DirectRenderingChanged(routingId()));
}

CardGroup* CardWindow::cardGroup() const
{
	return m_group.isNull() ? 0 : m_group.data();
}

void CardWindow::setCardGroup(CardGroup* group)
{
	m_group = group;
}

bool CardWindow::attachedToGroup() const
{
	return m_attachedToGroup;
}

void CardWindow::setAttachedToGroup(bool attached)
{
	m_attachedToGroup = attached;

	if (!m_group.isNull() && attached) {

		// adjust the card so that it sits at the same place within it's group	
		QPointF gpos = m_group->pos();
		QPointF newPos = pos() - gpos;
		m_position.trans.setX(m_position.trans.x()+newPos.x());
		m_position.trans.setY(m_position.trans.y()+newPos.y());
		setPos(gpos);
	}
}

void CardWindow::mapCoordinates(qreal& x, qreal& y) {
	QRectF br = boundingRect();
	x = x - br.x();
	y = y - br.y();
}

void CardWindow::mapFlickVelocities(qreal& x, qreal& y) {
	//Do nothing
}

QRectF CardWindow::transitionBoundingRect() {
	return boundingRect();
}


CardWindow::Position::Position()
	: zRot(0)
{
	// initial scale is unscaled
	trans.setZ(1.0);
}

bool CardWindow::Position::operator ==(const CardWindow::Position& other)
{
	return trans == other.trans &&
		   zRot == other.zRot;
}

CardWindow::Position CardWindow::Position::operator +(const CardWindow::Position& right) const
{
	Position p(*this);
	p.trans += right.trans;
	p.zRot += right.zRot;
	return p;
}

CardWindow::Position CardWindow::Position::operator -(const CardWindow::Position& right) const
{
	Position p(*this);
	p.trans -= right.trans;
	p.zRot -= right.zRot;
	return p;
}

CardWindow::Position CardWindow::Position::operator *(qreal right) const
{
	Position p(*this);
	p.trans *= right;
	p.zRot *= right;
	return p;
}

QTransform CardWindow::Position::toTransform() const
{
	static QMatrix4x4 m; m.setToIdentity();
	m.translate(trans.x(), trans.y());
	m.scale(trans.z(), trans.z());
	m.rotate(zRot, 0,0,1);
	return m.toTransform(1024);
}

void CardWindow::onEnableTouchEvents(bool enableTouchEvents)
{
    if (!Settings::LunaSettings()->enableTouchEventsForWebApps)
        return;

	if (m_touchEventsEnabled == enableTouchEvents)
		return;

	m_touchEventsEnabled = enableTouchEvents;

	if (enableTouchEvents) {
		if (m_focused)	
			setAcceptTouchEvents(true);
	}
	else
		setAcceptTouchEvents(false);
}

GhostCard* CardWindow::createGhost()
{
    // construct a clone/snapshot of this CardWindow.
    const QPixmap* pixmap = acquireScreenPixmap();
    GhostCard* ghost = 0;

    if (pixmap && !pixmap->isNull()) {
        ghost = new GhostCard(*pixmap, m_paintPath, m_position);
    }
    return ghost;
}

void CardWindow::initializeRoundedCornerStage()
{
#if defined(USE_ROUNDEDCORNER_SHADER)
	if (!m_roundedCornerShaderStage)
		m_roundedCornerShaderStage = new CardRoundedCornerShaderStage();

	int width, height;
	width = m_boundingRect.width();
	height = m_boundingRect.height();

	if (!m_isResizing && !m_flipsQueuedUp && m_data && !isHost() &&
		(type() != Window::Type_ModalChildWindowCard) && (m_appFixedOrientation == Event::Orientation_Invalid)) {
		// safeguard code in case the data buffer and the bounding rect dimensions get out of sync
		bool isDataLandscape = m_data->width() >= m_data->height();
		bool isBoundingRectLandscape = m_boundingRect.width() >= m_boundingRect.height();

		if(isDataLandscape != isBoundingRectLandscape) {
			// problem here!! something got out of sync, so try to remediate using the buffer data sizes
			width = m_data->width();
			height = m_data->height() - (fullScreen() ? 0 : Settings::LunaSettings()->positiveSpaceTopPadding);
			m_boundingRect.setRect(-width/2, -height/2, width, height);

			CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
			if (shadow)
				shadow->cacheDrawingData();
		}
	}

	if (m_adjustmentAngle != 90 && m_adjustmentAngle != -90){
		if (m_data) {
			m_roundedCornerShaderStage->setParameters(m_data->width(),
													  m_data->height(),
													  width,
													  height,
													  40,
                                                      m_dimming);
		}
	} else {
		if (m_data) {
			m_roundedCornerShaderStage->setParameters(m_data->width(),
													  m_data->height(),
													  height,
													  width,
													  40,
                                                      m_dimming);
		}
	}

#endif
}

void CardWindow::onSetAppFixedOrientation(int orientation, bool isPortrait) {
	bool uiIsInPortraitMode;
	bool currBuffPortrait;
	bool requiresBufferResize = false;

	Event::Orientation oldOrient = m_appFixedOrientation;

	uiIsInPortraitMode = SystemUiController::instance()->isUiInPortraitMode();

	currBuffPortrait = m_data->width() < m_data->height();


	if(orientation != Event::Orientation_Invalid) {
		requiresBufferResize = (isPortrait != currBuffPortrait) && !isHost();
	} else {
		if(m_appFixedOrientation!= Event::Orientation_Invalid) {
			requiresBufferResize = uiIsInPortraitMode != currBuffPortrait && !isHost();
		}
	}


	if(requiresBufferResize) {
		if(!m_maximized) {
			m_appFixedOrientation = (Event::Orientation)orientation;
			resizeWindowBufferEvent(m_bufHeight, m_bufWidth, QRect(0, 0, m_boundingRect.width(), m_boundingRect.height()), true, true);
		} else {
			m_appFixedOrientation = Event::Orientation_Invalid;
		}
	} else {
		m_appFixedOrientation = (Event::Orientation)orientation;
		refreshAdjustmentAngle();
	}

	if(m_maximized) {
        if(oldOrient != (Event::Orientation)orientation) {
                if(!WindowServer::instance()->okToResizeUi()) {
                        m_appPendingFixedOrientation = (Event::Orientation)orientation;
                }
                SystemUiController::instance()->setRequestedSystemOrientation((Event::Orientation)orientation);
        }
	}

	if(requiresBufferResize) {
		m_appFixedOrientation = (Event::Orientation)orientation;
		resizeEvent(m_bufWidth, m_bufHeight - (fullScreen() ? 0 : Settings::LunaSettings()->positiveSpaceTopPadding));
        refreshAdjustmentAngle();
	}
}


void CardWindow::refreshAdjustmentAngle() {
	int uiOrientation = WindowServer::instance()->getUiOrientation();
	switch (uiOrientation) {
        case OrientationEvent::Orientation_Up:
		{
			switch (m_appFixedOrientation) {
				case Event::Orientation_Up: {m_adjustmentAngle = 0; break;}
				case Event::Orientation_Left: {m_adjustmentAngle = 90; break;}
				case Event::Orientation_Right: {m_adjustmentAngle = -90; break;}
				case Event::Orientation_Down: {m_adjustmentAngle = 180; break;}
				case Event::Orientation_Landscape: {m_adjustmentAngle = SystemUiController::instance()->isUiInPortraitMode() ? 90 : 0; break;}
				case Event::Orientation_Portrait: {m_adjustmentAngle = SystemUiController::instance()->isUiInPortraitMode() ? 0 : 90; break;}
				default:{m_adjustmentAngle = 0; break;}
			}
			break;
		}
        case OrientationEvent::Orientation_Left:
		{
			switch (m_appFixedOrientation) {
				case Event::Orientation_Up: {m_adjustmentAngle = -90; break;}
				case Event::Orientation_Left: {m_adjustmentAngle = 0; break;}
				case Event::Orientation_Right: {m_adjustmentAngle = 180; break;}
				case Event::Orientation_Down: {m_adjustmentAngle = 90; break;}
				case Event::Orientation_Landscape: {m_adjustmentAngle = SystemUiController::instance()->isUiInPortraitMode() ? 90 : 0; break;}
				case Event::Orientation_Portrait: {m_adjustmentAngle = SystemUiController::instance()->isUiInPortraitMode() ? 0 : 90; break;}
				default:{m_adjustmentAngle = 0; break;}
			}
			break;
		}
        case OrientationEvent::Orientation_Right:
		{
			switch (m_appFixedOrientation) {
				case Event::Orientation_Up: {m_adjustmentAngle = 90; break;}
				case Event::Orientation_Left: {m_adjustmentAngle = 180; break;}
				case Event::Orientation_Right: {m_adjustmentAngle = 0; break;}
				case Event::Orientation_Down: {m_adjustmentAngle = -90; break;}
				case Event::Orientation_Landscape: {m_adjustmentAngle = SystemUiController::instance()->isUiInPortraitMode() ? 90 : 0; break;}
				case Event::Orientation_Portrait: {m_adjustmentAngle = SystemUiController::instance()->isUiInPortraitMode() ? 0 : 90; break;}
				default:{m_adjustmentAngle = 0; break;}
			}
			break;
		}
        case OrientationEvent::Orientation_Down:
		{
			switch (m_appFixedOrientation) {
				case Event::Orientation_Up: {m_adjustmentAngle = 180; break;}
				case Event::Orientation_Left: {m_adjustmentAngle = -90; break;}
				case Event::Orientation_Right: {m_adjustmentAngle = 90; break;}
				case Event::Orientation_Down: {m_adjustmentAngle = 0; break;}
				case Event::Orientation_Landscape: {m_adjustmentAngle = SystemUiController::instance()->isUiInPortraitMode() ? 90 : 0; break;}
				case Event::Orientation_Portrait: {m_adjustmentAngle = SystemUiController::instance()->isUiInPortraitMode() ? 0 : 90; break;}
				default:{m_adjustmentAngle = 0; break;}
			}
			break;
		}
	}
}

void CardWindow::slotUiRotated() {
	if(m_appFixedOrientation != Event::Orientation_Invalid)
		refreshAdjustmentAngle();
}

void CardWindow::allowUpdates(bool allow)
{
	if (!m_data)
		return;

	m_data->allowUpdates(allow);
}

void CardWindow::slotShowIME()
{
	// Use active win here instead of maximized to handle cases where keyboard
	// is bring brought up when an app is being maximized
	Window* activeWin = SystemUiController::instance()->activeWindow();
	if (activeWin != this)
		return;

	m_keyboardShownMessageSent = true;

	if (m_channel)
		m_channel->sendAsyncMessage(new View_KeyboardShown(routingId(), true));
}

void CardWindow::slotHideIME()
{
	// Use active win here instead of maximized to handle cases where keyboard
	// is bring brought up when an app is being maximized
	// If we previously brought up the keyboard on this app, make sure to send
	// the complimentay hide message even if this app is not the current maximized one
	Window* activeWin = SystemUiController::instance()->activeWindow();
	if ((activeWin != this) && !m_keyboardShownMessageSent)
		return;

	m_keyboardShownMessageSent = false;

	if (m_channel)
		m_channel->sendAsyncMessage(new View_KeyboardShown(routingId(), false));
}
