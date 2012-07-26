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

#include "CardHostWindow.h"

#include <PIpcBuffer.h>
#include <PIpcChannel.h>

#include "AnimationSettings.h"
#include "BannerMessageHandler.h"
#include "CardTransition.h"
#include "HostBase.h"
#include "IpcClientHost.h"
#include "Settings.h"
#include "SystemUiController.h"
#include "Time.h"
#include "CardDropShadowEffect.h"
#include "WindowServer.h"
#include "WindowServerLuna.h"
#include "AppDirectRenderingArbitrator.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

static QPixmap* s_playButtonPixmap = 0;
static QPixmap* s_scrimPixmap = 0;

static const int kSyncCallTimeOutMs = 500;


CardHostWindow::CardHostWindow(Window::Type type, HostWindowData* data, IpcClientHost* clientHost)
	: CardWindow(type, data, clientHost)
	, m_paused(false)
	, m_pausedDueToDisplayOff(false)
	, m_penDownInPlayButton(false)
	, m_penInPlayButton(false)
	, m_inRotateAnim(false)
	, m_rotateAngleStart(0)
	, m_rotateAngleTarget(0)
	, m_rotateScaleStart(1.0f)
	, m_rotateScaleTarget(1.0f)
	, m_rotateTimerStart(0)
	, m_rotateAnimTimer(HostBase::instance()->masterTimer(), this, &CardHostWindow::rotateTimerTicked)
{
	static QPixmap playButtonPixmap;
	static QPixmap scrimPixmap;

	m_prepareAddedToWm = true; // host windows don't get prepared
	m_touchEventsEnabled = true;
	setAcceptTouchEvents(true);

	int animationStrength = AS(cardTransitionCurve);
	m_rotateEquation =  AS_EASEOUT(animationStrength);

	// FIXME: This should be an API
	m_winProps.setSuppressGestures(true);

	if (G_UNLIKELY(s_playButtonPixmap == 0)) {
		std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath +
							   "/fullscreen-play-button.png";
		playButtonPixmap = QPixmap(filePath.c_str());
		s_playButtonPixmap = &playButtonPixmap;
		if (s_playButtonPixmap->isNull()) {
			g_critical("Failed to load image file: %s", filePath.c_str());
		}
	}

	if (G_UNLIKELY(s_scrimPixmap == 0)) {
		std::string filePath = Settings::LunaSettings()->lunaSystemResourcesPath +
							   "/scrim.png";
		scrimPixmap = QPixmap(filePath.c_str());
		s_scrimPixmap = &scrimPixmap;
		if (s_scrimPixmap->isNull()) {
			g_critical("Failed to load image file: %s", filePath.c_str());
		}
	}

	//Also call onSetAppFixedOrientation() to initialize the rotation locking
	//It doesn't matter what the arguments are since we don't really use them.
	onSetAppFixedOrientation(Event::Orientation_Up, false);

	setVisibleDimensions(data->width(), data->height());
}

CardHostWindow::~CardHostWindow()
{
	AppDirectRenderingArbitrator::setLayerEnabled(this, false);
}

void CardHostWindow::resizeEvent(int w, int h)
{
    if (type() == Window::Type_Emergency) {
        if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90){
            setBoundingRect(h,w);
        } else {
            setBoundingRect(w,h);
        }
    }
    else {
        CardWindow::resizeEvent(w, h);
/*
        int height = h;
        if(fullScreen()){
            height = h - (isMaximized() ? 0 : Settings::LunaSettings()->positiveSpaceTopPadding);
        }
        m_boundingRect.setRect(-w/2, -height/2, w, height);
        CardWindow::resizeEvent(w, height);
*/
    }
}

void CardHostWindow::resizeEventSync(int w, int h)
{
    if (type() == Window::Type_Emergency) {
        if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90){
            setBoundingRect(h,w);
        } else {
            setBoundingRect(w,h);
        }
    }
    else {
        CardWindow::resizeEventSync(w, h);
    }
}

void CardHostWindow::resizeWindowBufferEvent(int w, int h, QRect windowScreenBounds, bool forceSync)
{
	//Don't rotate.  Not now.  Not ever.
	bool directRendering = m_maximized;

	if(Settings::LunaSettings()->displayUiRotates) {
		if(directRendering)
			setMaximized(false); // disable direct rendering for the resize event
		setBoundingRect(windowScreenBounds.width(), windowScreenBounds.height());

		m_paintPath = QPainterPath();
		m_paintPath.addRoundedRect(m_boundingRect, 25, 25);

		// reconstruct shadow
		CardDropShadowEffect* shadow = static_cast<CardDropShadowEffect*>(graphicsEffect());
		if (shadow) {
			bool enable = shadow->isEnabled();
			setGraphicsEffect(0);
			shadow = new CardDropShadowEffect(this);
			setGraphicsEffect(shadow);
			shadow->setEnabled(enable);
		}

		if(directRendering)
			setMaximized(true); // re-enable direct rendering
	}
}


void CardHostWindow::aboutToFocusEvent(bool enable)
{
	// If losing focus, remove game scenario mode immediately
	if (!enable) {
		if (m_fullScreenEnabled) {
			disableFullScreen();
		}
	}

	bool runRotateAnimation = false;
	QSize dims = getVisibleDimensions();

	switch (m_winProps.overlayNotificationsPosition) {
	case (WindowProperties::OverlayNotificationsTop): {
		runRotateAnimation = true;
		if (enable) {
			m_rotateAngleStart  = 180;
			m_rotateAngleTarget = 0;
			m_rotateScaleStart = 1.0f;
			m_rotateScaleTarget = 1.0f;
		}
		else {
			m_rotateAngleStart  = 0;
			m_rotateAngleTarget = 180;
			m_rotateScaleStart = 1.0f;
			m_rotateScaleTarget = 1.0f;
		}
		break;
	}
	case (WindowProperties::OverlayNotificationsLeft): {
		runRotateAnimation = true;
		if (enable) {
			m_rotateAngleStart  = 90;
			m_rotateAngleTarget = 0;
			m_rotateScaleStart = (dims.height() * 1.0f) / dims.width();
			m_rotateScaleTarget = 1.0f;
		}
		else {
			m_rotateAngleStart  = 0;
			m_rotateAngleTarget = 90;
			m_rotateScaleStart = 1.0f;
			m_rotateScaleTarget = (dims.height() * 1.0f) / dims.width();
		}
		break;
	}
	case (WindowProperties::OverlayNotificationsRight):
		runRotateAnimation = true;
		if (enable) {
			m_rotateAngleStart  = -90;
			m_rotateAngleTarget =  0;
			m_rotateScaleStart = (dims.height() * 1.0f) / dims.width();
			m_rotateScaleTarget = 1.0f;
		}
		else {
			m_rotateAngleStart  = 0;
			m_rotateAngleTarget = -90;
			m_rotateScaleStart = 1.0f;
			m_rotateScaleTarget = (dims.height() * 1.0f) / dims.width();
		}
		break;
	}

	if (runRotateAnimation) {
		m_inRotateAnim = true;
		m_rotateTimerStart = 0;
		m_rotateAnimTimer.start((int) 1000/AS(slowFPS), false);
	}
}

void CardHostWindow::focusEvent(bool enable)
{
	if (m_focused == enable)
		return;

	if (!enable && !m_paused) {
		pauseCard();
	}
	else if (enable && m_paused && m_winProps.hasPauseUi) {
		resumeCard();
	}

	CardWindow::focusEvent(enable);
}

void CardHostWindow::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	//painter->rotate(m_adjustmentAngle);
	CardWindow::paint(painter, option, widget);

	// Resume if needed on first draw after a display off.
	// this works in the scenario of pin lock/screen lock
	if (G_UNLIKELY(m_paused && m_pausedDueToDisplayOff && m_winProps.hasPauseUi)) {
		resumeCard();
	}
	//painter->rotate(-m_adjustmentAngle);
}

void CardHostWindow::paintOverlay(QPainter* painter, bool maximized)
{
	if (G_LIKELY(!m_paused))
		return;

	if (m_winProps.hasPauseUi)
		return;

	if (G_UNLIKELY(s_playButtonPixmap == 0 || s_scrimPixmap == 0))
		return;

	painter->setBrushOrigin(m_visibleBounds.x(), m_visibleBounds.y());
	painter->fillPath(m_paintPath, *s_scrimPixmap);
	painter->setBrushOrigin(0, 0);

	if (m_fullScreenEnabled) {

		int pw = s_playButtonPixmap->width();
		int ph = s_playButtonPixmap->height() / 2;
		int yOffset = m_penInPlayButton ? ph : 0;

		QPainter::CompositionMode oldMode = painter->compositionMode();
		painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter->drawPixmap(-pw/2, -ph/2, *s_playButtonPixmap, 0, yOffset, pw, ph);
		painter->setCompositionMode(oldMode);
	}
}

void CardHostWindow::fullScreenEnabled(bool enabled)
{
	g_message("Full screen %s", enabled ? "enabled" : "disabled");
	HostBase::instance()->turboModeSubscription(enabled);

	if(!m_isIpcWindow || !m_channel)
		return;

	if (enabled) {
		AppDirectRenderingArbitrator::setLayerEnabled(this, true);
        
		m_channel->sendAsyncMessage(new View_FullScreenEnabled(m_data->key()));
	}
	else {

		SystemUiController::instance()->aboutToSendSyncMessage();
		
		if(!m_channel->sendSyncMessage(new View_FullScreenDisabled(m_data->key()), kSyncCallTimeOutMs)){
			g_critical("%s (%d): Synchronous call failed.", __PRETTY_FUNCTION__, __LINE__);
		}
		
		AppDirectRenderingArbitrator::setLayerEnabled(this, false);
	}

	if (enabled) {
		if (m_paused && m_winProps.hasPauseUi) {
			resumeCard();
		}
	}
	else {
		if (!m_paused) {
			pauseCard();
		}
	}
}

void CardHostWindow::displayOff()
{
	if (m_paused)
		return;

	pauseCard();
	m_pausedDueToDisplayOff = true;
}

void CardHostWindow::displayDimmed()
{
	displayOff();
	update();
}

void CardHostWindow::pause()
{
	if (m_paused)
		return;

	pauseCard();

	if (m_winProps.hasPauseUi) {
		resumeCard();
	}
}

void CardHostWindow::pauseCard()
{
	if (m_paused)
		return;

	g_message("%s: pausing", __PRETTY_FUNCTION__);

	if (m_channel)
		m_channel->sendAsyncMessage(new View_Pause(m_data->key()));

	m_paused = true;
}

void CardHostWindow::resumeCard()
{
    if (!m_paused)
		return;

	g_message("%s: resuming", __PRETTY_FUNCTION__);

	if (m_channel)
		m_channel->sendAsyncMessage(new View_Resume(m_data->key()));

	m_paused = false;
	m_pausedDueToDisplayOff = false;
}

void CardHostWindow::initializeRoundedCornerStage()
{
#if defined(USE_ROUNDEDCORNER_SHADER)
    if (!m_roundedCornerShaderStage)
        m_roundedCornerShaderStage = new CardRoundedCornerShaderStage();

    int width, height;
    width = m_boundingRect.width();
    height = m_boundingRect.height();

    if (m_adjustmentAngle != 90 && m_adjustmentAngle != -90){
        if (m_data) {
            m_roundedCornerShaderStage->setParameters(m_data->width(),
                                                      m_data->height(),
                                                      width,
                                                      height,
                                                      55,
                                                      m_dimming);
        }
    } else {
        if (m_data) {
            m_roundedCornerShaderStage->setParameters(m_data->width(),
                                                      m_data->height(),
                                                      height,
                                                      width,
                                                      46,
                                                      m_dimming);
        }
    }
#endif
}

void CardHostWindow::paintBase(QPainter* painter, bool maximized)
{
    if (maximized) {
        // faster, rectangular blit
        painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter->rotate(m_adjustmentAngle);

        Window::paint(painter, 0, 0);
        painter->rotate(-m_adjustmentAngle);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    } else {
        // draw with rounded corners
        const QPixmap* pix = acquireScreenPixmap();
        if (pix) {
            QRectF brect = boundingRect();
            QPainterPath paintPath;

                            initializeRoundedCornerStage();

                            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

            if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90) {
                QRectF rotRect = QRectF(brect.y(), brect.x(), brect.height(), brect.width());
                paintPath.addRoundedRect(rotRect, 25, 25);
            } else {
                paintPath.addRoundedRect(m_boundingRect, 25, 25);
            }

            int originX = brect.x();
            int originY = brect.y();

            if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90) {
                originX = brect.y();
                originY = brect.x();
            }

            if (fullScreen()) {
                if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90) {
                    painter->setBrushOrigin(originX - (pix->width()-brect.height())/2, originY);
                } else {
                    painter->setBrushOrigin(originX, originY - (pix->height() - brect.height()) / 2);
                }
            }
            else
                painter->setBrushOrigin(originX, originY);
                            painter->rotate(m_adjustmentAngle);
#if defined(USE_ROUNDEDCORNER_SHADER)
                            m_roundedCornerShaderStage->setOnPainter(painter);
                            if (m_adjustmentAngle == 90 || m_adjustmentAngle == -90) {
                                painter->drawPixmap(-brect.height()/2, -brect.width()/2, *pix);
                            }
                            else {
                                painter->drawPixmap(-brect.width()/2, -brect.height()/2, *pix);
                            }
                            m_roundedCornerShaderStage->removeFromPainter(painter);
#else
                            painter->fillPath(paintPath, *pix);
#endif
                            painter->rotate(-m_adjustmentAngle);
            painter->setBrushOrigin(0, 0);
        }
    }
}

bool CardHostWindow::rotateTimerTicked()
{
	if (G_UNLIKELY(m_rotateTimerStart == 0)) {
		m_rotateTimerStart = Time::curTimeMs();
	}

	update();

	int durationMs = AS(cardTransitionDuration);
	uint32_t currTime = Time::curTimeMs();
	if (currTime > (m_rotateTimerStart + durationMs)) {
		m_inRotateAnim = false;
		return false;
	}

	return true;
}

bool CardHostWindow::touchEvent(QTouchEvent* event)
{
	if (!m_focused || m_pendingFocus == PendingFocusFalse) {
		return false;
	}

	if (m_paused)
		return true;

    if (m_channel) {

		typedef QList<QTouchEvent::TouchPoint> TouchPoints;
		TouchPoints touchPoints = event->touchPoints();

		QPointF topLeft = boundingRect().topLeft();

		for (TouchPoints::const_iterator it = touchPoints.begin();
			 it != touchPoints.end(); ++it) {

//			printf("TouchPoint Id: %d, State: %d, pos: %g, %g\n",
//				   (*it).id(), (*it).state(), (*it).pos().x(), (*it).pos().y());

			qreal adjustmentAngle = m_adjustmentAngle;


			int displayWidth = Settings::LunaSettings()->displayWidth;
			int displayHeight = Settings::LunaSettings()->displayHeight;
			QPointF pt = (*it).pos();
			pt -= topLeft;
			int x = pt.x();
			int y = pt.y();

			//		adjustmentAngle, x, y, displayWidth, displayHeight);
			if (adjustmentAngle == 90) { //0,0 in upper right
				int tmp = y;
				y = x;
				x = tmp;

				y = displayHeight-y;
			}
			else if (adjustmentAngle == -90) { //0,0 in lower left

				int tmp = y;
				y = x;
				x = tmp;

				x = displayWidth-x;
			} else if (adjustmentAngle == 0) { //0,0 in upper left
				//Do nothing
			}
			else if (adjustmentAngle == 180) { //0,0 in lower right / screen dimensions flip
				x = displayWidth-x;
				y = displayHeight-y;
			}


			switch ((*it).state()) {
			case Qt::TouchPointPressed: {

				Event evt;
				evt.type = Event::PenDown;
				evt.id = (*it).id();
				evt.x = x;
				evt.y = y;
				evt.z = 0;
				evt.key = Event::Key_Null;
				evt.button = Event::Left;
				evt.modifiers = Event::modifiersFromQt(event->modifiers());
				evt.time = Time::curTimeMs();
				evt.clickCount = 1;

				m_channel->sendAsyncMessage (new View_InputEvent(routingId(),
																 SysMgrEventWrapper(&evt)));

				break;				
			}
			case Qt::TouchPointMoved: {

				Event evt;
				evt.type = Event::PenMove;
				evt.id = (*it).id();
				evt.x = x;
				evt.y = y;
				evt.z = 0;
				evt.key = Event::Key_Null;
				evt.button = Event::Left;
				evt.modifiers = Event::modifiersFromQt(event->modifiers());
				evt.time = Time::curTimeMs();
				evt.clickCount = 0;

				m_channel->sendAsyncMessage (new View_InputEvent(routingId(),
																 SysMgrEventWrapper(&evt)));

				break;
			}
			case Qt::TouchPointReleased: {

				Event evt;
				evt.type = Event::PenUp;
				evt.id = (*it).id();
				evt.x = x;
				evt.y = y;
				evt.z = 0;
				evt.key = Event::Key_Null;
				evt.button = Event::Left;
				evt.modifiers = Event::modifiersFromQt(event->modifiers());
				evt.time = Time::curTimeMs();
				evt.clickCount = 0;

				m_channel->sendAsyncMessage (new View_InputEvent(routingId(),
																 SysMgrEventWrapper(&evt)));

				break;
			}
			default:
				continue;
			}								
		}
    }

	return true;
}

void CardHostWindow::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if (!m_focused || m_pendingFocus == PendingFocusFalse) {
		event->ignore();
		return;
	}

	event->accept();

	if (G_UNLIKELY(s_playButtonPixmap == 0))
		return;

	int pw = s_playButtonPixmap->width();
	int ph = s_playButtonPixmap->height() / 2;

	QRect rect(-pw/2, -ph/2, pw, ph);
	QPoint pos = event->pos().toPoint();

	if (rect.contains(pos)) {
		m_penDownInPlayButton = true;
		m_penInPlayButton = true;
	}
	
	update();
}

void CardHostWindow::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	event->accept();
}

void CardHostWindow::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	event->accept();

	if (!m_penInPlayButton)
		return;

	if (!m_penDownInPlayButton)
		return;

	if (G_UNLIKELY(s_playButtonPixmap == 0))
		return;

	int pw = s_playButtonPixmap->width();
	int ph = s_playButtonPixmap->height() / 2;

	QRect rect(-pw/2, -ph/2, pw, ph);
	QPoint pos = event->pos().toPoint();

	bool initial = m_penInPlayButton;
	
	m_penInPlayButton = rect.contains(pos);

	if (m_penInPlayButton != initial)
		update();
}

void CardHostWindow::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	event->accept();

	if (!m_penInPlayButton)
		return;

	if (G_UNLIKELY(s_playButtonPixmap == 0))
		return;

	int pw = s_playButtonPixmap->width();
	int ph = s_playButtonPixmap->height() / 2;

	QRect rect(-pw/2, -ph/2, pw, ph);
	QPoint pos = event->pos().toPoint();

	bool initial = m_penInPlayButton;
	
	m_penInPlayButton = (rect.contains(pos) && !event->canceled());
	if (m_penInPlayButton)
		resumeCard();

	m_penDownInPlayButton = false;
	m_penInPlayButton = false;
}

void CardHostWindow::refreshAdjustmentAngle() {
	int uiOrientation = WindowServer::instance()->getUiOrientation();
	switch (uiOrientation) {
        case OrientationEvent::Orientation_Up:
		{
			m_adjustmentAngle = 0;
			break;
		}
        case OrientationEvent::Orientation_Left:
		{
			m_adjustmentAngle = -90;
			break;
		}
        case OrientationEvent::Orientation_Right:
		{
			m_adjustmentAngle = 90;
			break;
		}
        case OrientationEvent::Orientation_Down:
		{
			m_adjustmentAngle = 180;
			break;
		}
	}
}

void CardHostWindow::onSetAppFixedOrientation(int orientation, bool isPortrait) {
	CardWindow::onSetAppFixedOrientation(orientation, isOrientationPortrait((Event::Orientation)orientation));
}

bool CardHostWindow::isOrientationPortrait(Event::Orientation orient)
{
	bool isPortrait = false;
	bool deviceIsPortraitType = true;

	int displayWidth = Settings::LunaSettings()->displayWidth;
	int displayHeight = Settings::LunaSettings()->displayHeight;


	if((Settings::LunaSettings()->homeButtonOrientationAngle == 0) || (Settings::LunaSettings()->homeButtonOrientationAngle == 180)) {
		if(displayWidth > displayHeight) {
			deviceIsPortraitType = false;
		} else {
			deviceIsPortraitType = true;
		}
	} else {
		if(displayWidth > displayHeight) {
			deviceIsPortraitType = true;
		} else {
			deviceIsPortraitType = false;
		}
	}


	switch (orient) {
		case (Event::Orientation_Left):
		case (Event::Orientation_Right):
		{
			if(Settings::LunaSettings()->homeButtonOrientationAngle == 0 || Settings::LunaSettings()->homeButtonOrientationAngle == 180)
				isPortrait = !deviceIsPortraitType;
			else
				isPortrait = deviceIsPortraitType;
			break;
		}

		case (Event::Orientation_Landscape): {
			isPortrait = false;
			break;
		}

		case (Event::Orientation_Up):
		case (Event::Orientation_Down):
		{
			if(Settings::LunaSettings()->homeButtonOrientationAngle == 0 || Settings::LunaSettings()->homeButtonOrientationAngle == 180)
				isPortrait = deviceIsPortraitType;
			else
				isPortrait = !deviceIsPortraitType;
			break;
		}

		case (Event::Orientation_Portrait): {
			isPortrait = true;
			break;
		}

		default: {
			isPortrait = false;
		}
	}
	return isPortrait;
}

