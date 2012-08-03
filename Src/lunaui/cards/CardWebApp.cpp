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

#include "CardWebApp.h"
#include "EventThrottler.h"
#include "Logging.h"
#include "RemoteWindowData.h"
#include "RoundedCorners.h"
#include "Settings.h"
#include "Utils.h"
#include "WebAppCache.h"
#include "WebAppDeferredUpdateHandler.h"
#include "WebAppManager.h"
#include "WebAppFactory.h"
#include "SysMgrWebBridge.h"
#include "Window.h"
#include "WindowMetaData.h"
#include "Time.h"
#include "EventReporter.h"
#include "ApplicationDescription.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <glib.h>
#include <string>
#include <PIpcBuffer.h>
#include <PIpcChannel.h>
#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsView>

#if defined(HAVE_OPENGL)
#include <QGLWidget>
#include "RemoteWindowDataOpenGLQt.h"
#endif

#ifdef GFX_DEBUGGING
#include <QElapsedTimer>
#endif


void CardWebApp::paintEvent(QPaintEvent* event)
{
#ifdef GFX_DEBUGGING
    qDebug() << "   +=================================================================+";
    qDebug() << "   +          paintEvent                                             +";
    qDebug() << "   +=================================================================+";
    qDebug() << "direct:" << m_directRendering;
    qDebug() << "rect:" << m_paintRect;
    qDebug() << "own  geometry" << geometry();
    qDebug() << "viewport  geometry" << viewport()->geometry();
    qDebug() << m_webview->geometry()<<"->"<<mapFromScene(m_webview->geometry()).boundingRect();
#endif

    if (m_directRendering) {
        QGraphicsView::paintEvent(event);
        // clear IPC Buffer if we switched from software to direct rendering
        // but only after we finished painting!
        if (m_lastPaintIPCBuffer) {
#if defined (HAVE_OPENGL)
            glFlush();
#endif
            m_data->clear();
        }
        m_lastPaintIPCBuffer = false;
    } else {
        m_lastPaintIPCBuffer = true;
        if (m_renderingSuspended)
            return;
        if (!appLoaded())
            return;

        if (m_childWebApp) {
            m_paintRect = QRect();
            return;
        }

        luna_assert(!m_beingDeleted);

        if (m_beingDeleted)
            g_critical("FATAL ERROR: Being painted when deleted\n");

        if (m_paintingDisabled)
            return;

        if (!m_data->supportsPartialUpdates())
            m_paintRect = QRect(0, 0, m_appBufWidth, m_appBufHeight);

        QPainter* paintContext = m_data->qtRenderingContext();
        m_data->beginPaint();
#ifdef GFX_DEBUGGING
        QElapsedTimer paintTime;
        paintTime.start();
#endif
        setTransform(QTransform()); // identity transform
        // watch out: this is different from m_CardOrientation! For IPC buffer painting we
        // have to use this or we get wrong results
        rotate(angleForOrientation(m_orientation));
        render(paintContext, m_paintRect, m_paintRect);
        m_data->endPaint(false, QRect());
#ifdef GFX_DEBUGGING
        qDebug() << page()->appId() << "manual paint took" << paintTime.elapsed() << "ms";
#endif
        // notify WindowServer about paint update across IPC
        m_data->sendWindowUpdate(m_paintRect.x(), m_paintRect.y(), m_paintRect.width(), m_paintRect.height());

        // clear to indicate we handled the dirty regions
        m_paintRect.setRect(0, 0, 0, 0);
    }
#ifdef GFX_DEBUGGING
    qDebug() << "   +~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+";
#endif
};

#ifdef GFX_DEBUGGING
bool CardWebApp::viewportEvent(QEvent* event)
{
    qDebug() << __PRETTY_FUNCTION__ << event;
    return QGraphicsView::viewportEvent(event);
}

bool CardWebApp::event(QEvent* event)
{
    qDebug() << __PRETTY_FUNCTION__ << event;
    return QGraphicsView::event(event);
}
#endif


CardWebApp::CardWebApp(Window::Type winType, PIpcChannel *channel, ApplicationDescription* desc)
	: WindowedWebApp(0, 0, winType, channel)
	, m_parentWebApp(0)
	, m_childWebApp(0)
    , m_CardOrientation(Event::Orientation_Invalid)
    , m_orientation(Event::Orientation_Up)
	, m_fixedOrientation(Event::Orientation_Invalid)
	, m_allowsOrientationChange(false)
	, m_pendingResizeWidth(-1)
	, m_pendingResizeHeight(-1)
	, m_pendingOrientation(Event::Orientation_Invalid)
	, m_pendingFullScreenMode(-1)
	, m_enableFullScreen(false)
	, m_doPageUpDownInLandscape(false)
	, m_directRendering(false)
	, m_renderOffsetX(0)
	, m_renderOffsetY(0)
	, m_renderOrientation(Event::Orientation_Up)
	, m_paintingDisabled(false)
	, m_renderingSuspended(false)
    , m_glw(0)
    , m_lastPaintIPCBuffer(false)
{
	if(desc != 0) {
		std::string request = desc->requestedWindowOrientation();


		if(request.length() > 0) {
			if (strcasecmp(request.c_str(), "free") == 0) {
				m_fixedOrientation = Event::Orientation_Invalid;
				m_allowsOrientationChange = true;
			}
			else {
				Event::Orientation appOrient = Event::Orientation_Invalid;
				if (strcasecmp(request.c_str(), "up") == 0)
					appOrient = Event::Orientation_Up;
				else if (strcasecmp(request.c_str(), "down") == 0)
					appOrient = Event::Orientation_Down;
				else if (strcasecmp(request.c_str(), "left") == 0)
					appOrient = Event::Orientation_Left;
				else if (strcasecmp(request.c_str(), "right") == 0)
					appOrient = Event::Orientation_Right;
				else if (strcasecmp(request.c_str(), "landscape") == 0)
					appOrient = Event::Orientation_Landscape;
				else if (strcasecmp(request.c_str(), "portrait") == 0)
					appOrient = Event::Orientation_Portrait;

				m_fixedOrientation = orientationForThisCard(appOrient);

			}
		}
	}

	int widthAdj = 0;
	int heightAdj = Settings::LunaSettings()->positiveSpaceTopPadding;

    if(winType == Window::Type_ModalChildWindowCard) {
        heightAdj = 0;
        m_width = Settings::LunaSettings()->modalWindowWidth;
        m_height = Settings::LunaSettings()->modalWindowHeight;
    }
    else if(!Settings::LunaSettings()->displayUiRotates || m_fixedOrientation == Event::Orientation_Invalid) {
        m_width = WebAppManager::instance()->currentUiWidth();
        m_height = WebAppManager::instance()->currentUiHeight();
    } else {
        getFixedOrientationDimensions(m_width, m_height, widthAdj, heightAdj);
    }

	m_appBufWidth = m_width;
	m_appBufHeight = m_height;

	m_windowWidth = m_width - widthAdj;
	m_windowHeight = m_height - heightAdj;
	m_setWindowWidth = m_windowWidth;
	m_setWindowHeight = m_windowHeight;

	// --------------------------------------------------------------------------

	if (winType == Window::Type_Card || winType == Window::Type_PIN || 
        winType == Window::Type_Emergency || winType == Window::Type_ModalChildWindowCard) {
		
		init();

		setOrientation(WebAppManager::instance()->orientation());

	}

	// for child windows the setup will be done in the attach page call
	if (winType == Window::Type_Card || winType == Window::Type_ChildCard)
		WebAppDeferredUpdateHandler::registerApp(this);

    QGraphicsScene* scene = new QGraphicsScene(0, 0, m_width, m_height);
    setScene(scene);
    setGeometry(0, 0, m_width, m_height);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // these are important to prevent weird one pixel offsets due to
    // invisible frames
    setContentsMargins(0, 0, 0, 0);
    setFrameShape(QFrame::NoFrame);

    setAlignment(Qt::AlignTop | Qt::AlignLeft);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setTransform(QTransform()); // identity transform
    rotate(angleForOrientation(m_CardOrientation));

    m_webview = new QGraphicsWebView();
    m_webview->setGeometry(QRectF(0, 0, m_windowWidth, m_windowHeight));
    m_webview->setResizesToContents(false);
    scene->addItem(m_webview);
}

CardWebApp::~CardWebApp()
{
    if (page()) {
		// We do not want to report this if it was parked; it was already reported WHEN it was parked.
		EventReporter::instance()->report( "close", static_cast<ProcessBase*>(page())->appId().toUtf8().constData() );
	}
    if (m_webview)
        delete m_webview;
    if (scene()) {
        delete scene();
        setScene(0);
    }

	if (m_childWebApp) {
		m_childWebApp->setParentCardWebApp(0);
        m_childWebApp->WebAppBase::close();
    }
	
	if (m_parentWebApp)
		m_parentWebApp->removeChildCardWebApp(this);

	if (m_winType == Window::Type_ChildCard) {
		m_data = 0;
		m_metaDataBuffer = 0;
	}

	if (m_winType == Window::Type_Card || m_winType == Window::Type_ChildCard)
		WebAppDeferredUpdateHandler::unregisterApp(this);
}

void CardWebApp::inputEvent(sptr<Event> e)
{
	if (m_childWebApp) {
		m_childWebApp->inputEvent(e);
		return;
	}
	
	switch (e->type) {
	case (Event::KeyDown):
	case (Event::KeyUp): {

		bool inLandScapeMode = false;
		if (m_doPageUpDownInLandscape &&
			(m_orientation == Event::Orientation_Left ||
			 m_orientation == Event::Orientation_Right)) {
			inLandScapeMode = true;
		}

		if (inLandScapeMode) {
			if (e->key == Event::Key_CoreNavi_Back || e->key == Event::Key_CoreNavi_Previous) {
				e->key = (m_orientation == Event::Orientation_Right) ? Event::Key_PageDown : Event::Key_PageUp;
				e->setGestureKey(false);
			}
			else if (e->key == Event::Key_CoreNavi_Menu || e->key == Event::Key_CoreNavi_Next) {
				e->key = (m_orientation == Event::Orientation_Right) ? Event::Key_PageUp : Event::Key_PageDown;
				e->setGestureKey(false);
			}
		}

		if (e->key >= Event::Key_Up && e->key <= Event::Key_Right) {

			// Reject cursor events if we are in the middle of a gesture
			// (indicated by m_blockPenEvents)
			if (m_blockPenEvents)
				return;

			switch (m_orientation) {
			case (Event::Orientation_Down): {

				switch (e->key) {
				case (Event::Key_Left):
					e->key = Event::Key_Right;
					break;
				case (Event::Key_Right):
					e->key = Event::Key_Left;
					break;
				case (Event::Key_Up):
					e->key = Event::Key_Down;
					break;
				case (Event::Key_Down):
					e->key = Event::Key_Up;
					break;
				default:
					break;
				}

				break;
			}
			case (Event::Orientation_Left): {

				switch (e->key) {
				case (Event::Key_Left):
					e->key = Event::Key_Down;
					break;
				case (Event::Key_Right):
					e->key = Event::Key_Up;
					break;
				case (Event::Key_Up):
					e->key = Event::Key_Left;
					break;
				case (Event::Key_Down):
					e->key = Event::Key_Right;
					break;
				default:
					break;
				}

				
				break;
			}
			case (Event::Orientation_Right): {

				switch (e->key) {
				case (Event::Key_Left):
					e->key = Event::Key_Up;
					break;
				case (Event::Key_Right):
					e->key = Event::Key_Down;
					break;
				case (Event::Key_Up):
					e->key = Event::Key_Right;
					break;
				case (Event::Key_Down):
					e->key = Event::Key_Left;
					break;
				default:
					break;
				}

				break;
			}
			default:
				break;
			}

		}

		break;
	}
	case (Event::PenDown):
	case (Event::PenUp):
	case (Event::PenMove):
	case (Event::PenFlick):
	case (Event::PenPressAndHold):
	case (Event::PenCancel):
	case (Event::PenCancelAll):
	case (Event::PenSingleTap): {
	
		switch (m_orientation) {
		case (Event::Orientation_Right): {

			int tmp = e->x;
			e->x = m_windowWidth - e->y;
			e->y = tmp;

			tmp = e->flickXVel;
			e->flickXVel = - e->flickYVel;
			e->flickYVel = tmp;			
			break;
		}
		case (Event::Orientation_Left): {

			int tmp = e->x;
			e->x = e->y;
			e->y = m_windowHeight - tmp;

			tmp = e->flickXVel;
			e->flickXVel = e->flickYVel;
			e->flickYVel = - tmp;						
			break;
		}
		case (Event::Orientation_Down): {
			e->x = m_windowWidth - e->x;
			e->y = m_windowHeight - e->y;
			e->flickXVel = - e->flickXVel;
			e->flickYVel = - e->flickYVel;
			break;
		}
		default:
			break;
		}

		break;
	}
	case (Event::GestureStart):
	case (Event::GestureChange):
	case (Event::GestureEnd): {

		switch (m_orientation) {
		case (Event::Orientation_Right): {
			int tmp = e->gestureCenterX;
			e->gestureCenterX = m_windowWidth - e->gestureCenterY;
			e->gestureCenterY = tmp;			
			break;
		}
		case (Event::Orientation_Left): {
			int tmp = e->gestureCenterX;
			e->gestureCenterX = e->gestureCenterY;
			e->gestureCenterY = m_windowHeight - tmp;
			break;
		}
		case (Event::Orientation_Down): {
			e->gestureCenterX = m_windowWidth - e->gestureCenterX;
			e->gestureCenterY = m_windowHeight - e->gestureCenterY;		
			break;
		}
		default:
			break;
		}

		e->gestureCenterX = CLAMP(e->gestureCenterX, 0, (int) m_windowWidth - 1);
		e->gestureCenterY = CLAMP(e->gestureCenterY, 0, (int) m_windowHeight - 1);

	}
	default:
		break;
	}
			
	WindowedWebApp::inputEvent(e);	
}

void CardWebApp::keyEvent(QKeyEvent* e)
{
	if (m_childWebApp) {
		m_childWebApp->keyEvent(e);
		return;
	}	
	
	bool inLandScapeMode = false;
	if (m_doPageUpDownInLandscape &&
		(m_orientation == Event::Orientation_Left ||
		 m_orientation == Event::Orientation_Right)) {
		inLandScapeMode = true;
	}

	if (inLandScapeMode) {
		if (e->key() == Qt::Key_CoreNavi_Back ||
			e->key() == Qt::Key_CoreNavi_Previous) {
			int ev_key = (m_orientation == Event::Orientation_Right) ? Qt::Key_PageDown : Qt::Key_PageUp;
			QKeyEvent ev(e->type(), ev_key, e->modifiers());
			WindowedWebApp::keyEvent(&ev);
			return;
		}
		else if (e->key() == Qt::Key_CoreNavi_Menu ||
				 e->key() == Qt::Key_CoreNavi_Next) {
			int ev_key = (m_orientation == Event::Orientation_Right) ? Qt::Key_PageUp : Qt::Key_PageDown;
			QKeyEvent ev(e->type(), ev_key, e->modifiers());
			WindowedWebApp::keyEvent(&ev);
			return;
		}
	}

	if (e->key() >= Qt::Key_Left && e->key() <= Qt::Key_Down) {

		// Reject cursor events if we are in the middle of a gesture
		// (indicated by m_blockPenEvents)
		if (m_blockPenEvents)
			return;

		int orientedKey = e->key();

		switch (m_orientation) {
		case (Event::Orientation_Down): {

			switch (e->key()) {
			case (Qt::Key_Left):
				orientedKey = Qt::Key_Right;
				break;
			case (Qt::Key_Right):
				orientedKey = Qt::Key_Left;
				break;
			case (Qt::Key_Up):
				orientedKey = Qt::Key_Down;
				break;
			case (Qt::Key_Down):
				orientedKey = Qt::Key_Up;
				break;
			default:
				break;
			}

			break;
		}
		case (Event::Orientation_Left): {

			switch (e->key()) {
			case (Qt::Key_Left):
				orientedKey = Qt::Key_Down;
				break;
			case (Qt::Key_Right):
				orientedKey = Qt::Key_Up;
				break;
			case (Qt::Key_Up):
				orientedKey = Qt::Key_Left;
				break;
			case (Qt::Key_Down):
				orientedKey = Qt::Key_Right;
				break;
			default:
				break;
			}

				
			break;
		}
		case (Event::Orientation_Right): {

			switch (e->key()) {
			case (Qt::Key_Left):
				orientedKey = Qt::Key_Up;
				break;
			case (Qt::Key_Right):
				orientedKey = Qt::Key_Down;
				break;
			case (Qt::Key_Up):
				orientedKey = Qt::Key_Right;
				break;
			case (Qt::Key_Down):
				orientedKey = Qt::Key_Left;
				break;
			default:
				break;
			}

			break;
		}
		default:
			break;
		}

		QKeyEvent ev(e->type(), orientedKey, e->modifiers());
		WindowedWebApp::keyEvent(&ev);
		return;
	}

	
	WindowedWebApp::keyEvent(e);
}

void CardWebApp::paint()
{
    if (m_directRendering) {
        // direct rendering - let Qt handle the paints through QGLWidget
        scene()->update();
        repaint();
    } else {
        // indirect painting - we manually render the graphicsview into our IPC buffer
        // TODO: have widgets backed by the ipc buffer be the viewport, then we can use
        // the path above
        forcePaint();
    }
}

void CardWebApp::focusedEvent(bool focused)
{
	if (m_childWebApp)
		m_childWebApp->focusedEvent(focused);

	WindowedWebApp::focusedEvent(focused);

	if (focused)
		focusActivity();
	else
		blurActivity();
}

/**
  * overloaded from WebAppBase::resizeWebPage
  */
void CardWebApp::resizeWebPage(uint32_t width, uint32_t height)
{
    m_webview->resize(width, height);
}

int CardWebApp::resizeEvent(int newWidth, int newHeight, bool resizeBuffer)
{
    resize(newWidth, newHeight);
    m_webview->resize(newWidth, newHeight);
	if (m_setWindowWidth == newWidth && m_setWindowHeight == newHeight) {
		return -1;
	}

	m_setWindowWidth = newWidth;
	m_setWindowHeight = newHeight;

	if (m_childWebApp) {
		m_childWebApp->resizeEvent(newWidth, newHeight, resizeBuffer);
		m_pendingResizeWidth = newWidth;
		m_pendingResizeHeight = newHeight;
		return -1;
	}
	
    if (!page() || !page()->page())
		goto Done;

	// We allow window resize only in UP orientation mode (Except for Dock Mode Windows)
	if ((m_orientation != Event::Orientation_Up) && (m_winType != Window::Type_DockModeWindow))
		goto Done;

	if ((int) m_windowWidth == newWidth &&
		(int) m_windowHeight == newHeight)
	{
		if((m_pendingResizeWidth != -1) && (m_pendingResizeHeight != -1))
		{
			// if there are pending changes, make sure they are updated to the new values
			m_pendingResizeWidth = newWidth;
			m_pendingResizeHeight = newHeight;
		}
		goto Done;
	}

	if(resizeBuffer) {
		int oldKey = m_data->key();
		m_data->resize(newWidth, newHeight);

		int newKey = m_data->key();

		m_appBufWidth = newWidth;
		m_appBufHeight = newHeight;

		m_windowWidth = newWidth;

		m_windowHeight = newHeight - (m_enableFullScreen ? 0 : Settings::LunaSettings()->positiveSpaceTopPadding);

		m_width = newWidth;
		m_height = newHeight;

        resizeWebPage(newWidth, newHeight);

		m_paintRect.setRect(0, 0, m_appBufWidth, m_appBufHeight);

        forcePaint();

		WebAppManager::instance()->windowedAppKeyChanged(this, oldKey);

		setVisibleDimensions(m_width, m_height - (m_enableFullScreen ? 0 : Settings::LunaSettings()->positiveSpaceTopPadding));
		return newKey;
	} else {
		m_windowHeight = newHeight;
		if(m_fixedOrientation == Event::Orientation_Invalid) {
			m_windowWidth = newWidth;
		} else {
			m_windowWidth = m_width;
		}

        if (Window::Type_ModalChildWindowCard == WindowedWebApp::windowType())
            resizeWebPage(m_width, newHeight);
        else
            resizeWebPage(m_windowWidth, m_windowHeight);

        if (Window::Type_ModalChildWindowCard != WindowedWebApp::windowType())
            setVisibleDimensions(m_width, m_height - (m_enableFullScreen ? 0 : Settings::LunaSettings()->positiveSpaceTopPadding));
		else
			setVisibleDimensions(m_width, newHeight);

		// Force a full app repaint (not clipped to window dimensions)
		m_paintRect.setRect(0, 0, m_appBufWidth, m_appBufHeight);

        forcePaint();
    }

Done:
	return -1;
}

/**
  * This event happens when the device was rotated 90 degrees and the long and short edge
  * have flipped. It is similar to rotate and resize events. Maybe this was intended as
  * an optimization because you don't have to reallocate the IPC draw buffers as their
  * effective byte size doesn't change when rotating by 90 degrees
  */
void CardWebApp::flipEvent(int newWidth, int newHeight)
{
	if (m_childWebApp) {
		m_childWebApp->flipEvent(newWidth, newHeight);
		return;
	}

	m_data->flip();

	int tempWidth = m_width;
	m_width = m_height;
	m_height = tempWidth;

	m_appBufWidth = m_width;
	m_appBufHeight = m_height;

	m_pendingResizeWidth = -1;
	m_pendingResizeHeight = -1;

	m_windowWidth = m_width;
	if(m_winType == Window::Type_ModalChildWindowCard) {
		m_windowHeight = newHeight;
	}
	else {
		m_windowHeight = m_height - (m_enableFullScreen ? 0 : Settings::LunaSettings()->positiveSpaceTopPadding);
	}

	m_setWindowWidth = m_windowWidth;
	m_setWindowHeight = m_windowHeight;
    resizeWebPage(m_windowWidth, m_windowHeight);

	m_paintRect.setRect(0, 0, m_appBufWidth, m_appBufHeight);

    resize(newWidth, newHeight);

    // we have to manually resize the viewport independently, probably because the
    // GraphicsView widget is set to be hidden in card view and doesn't propagate resize
    // events when invisible.
    viewport()->resize(newWidth, newHeight);

    m_webview->resize(m_windowWidth, m_windowHeight);
    update();
    forcePaint();

	setVisibleDimensions(m_windowWidth, m_windowHeight);
}

void CardWebApp::asyncFlipEvent(int newWidth, int newHeight, int newScreenWidth, int newScreenHeight)
{
	if (m_childWebApp) {
		m_childWebApp->asyncFlipEvent(newWidth, newHeight, newScreenWidth, newScreenHeight);
		return;
	}

	if (m_data)
		m_data->flip();

	int tempWidth = m_width;
	m_width = m_height;
	m_height = tempWidth;

	m_pendingResizeWidth = -1;
	m_pendingResizeHeight = -1;

	m_appBufWidth = m_width;
	m_appBufHeight = m_height;

	m_windowWidth = m_width;
	if (m_winType == Window::Window::Type_ModalChildWindowCard ){
		m_windowHeight = newHeight;
	}
	else {
		m_windowHeight = m_height - (m_enableFullScreen ? 0 : Settings::LunaSettings()->positiveSpaceTopPadding);
	}

    resizeWebPage(m_windowWidth, m_windowHeight);

	m_paintRect.setRect(0, 0, m_windowWidth, m_windowHeight);
    forcePaint();

	// notify Host that this window is done resizing
	m_channel->sendAsyncMessage(new ViewHost_AsyncFlipCompleted(routingId(), newWidth, newHeight, newScreenWidth, newScreenHeight));

	setVisibleDimensions(m_windowWidth, m_windowHeight);

}

void CardWebApp::setOrientation(Event::Orientation orient)
{
	switch (orient) {
	case Event::Orientation_Up:
		break;
	case Event::Orientation_Left:
		break;
	case Event::Orientation_Down:
		break;
	case Event::Orientation_Right:
		break;
	default:
		break;
	}
	if (m_childWebApp) {
		m_childWebApp->setOrientation(orient);
		return;
	}
	
	switch (orient) {
	case (Event::Orientation_Up):    
	case (Event::Orientation_Down):  
	case (Event::Orientation_Left):  
	case (Event::Orientation_Right):
		break;
	default:
		return;
	}
	
    // Keep this variable always up-to-date
    m_CardOrientation = orient;

	if(Settings::LunaSettings()->displayUiRotates) {
		return;
	} else {
		orient = orientationForThisCard(orient);

		resizeWindowForOrientation(orient);

        //callMojoScreenOrientationChange();
	}
}

Event::Orientation CardWebApp::orientation() const
{
    if (Event::Orientation_Invalid != m_fixedOrientation)
    {
        return m_fixedOrientation;
    }
    else
    {
        return m_CardOrientation;
    }
}

void CardWebApp::resizeWindowForOrientation(Event::Orientation orient)
{
	if (!m_allowsOrientationChange) {
		return;
	}

	if (orient == m_orientation)
		return;

	Event::Orientation oldOrientation = m_orientation;
	m_orientation = orient;

	switch (orient) {
	case (Event::Orientation_Left):
	case (Event::Orientation_Right): {
		// Full screen in this mode
		m_windowWidth = m_height;
		m_windowHeight = m_width;
		m_appBufWidth = m_height;
		m_appBufHeight = m_width;
		break;
	}
	case (Event::Orientation_Down): {
		// Full screen in this mode
		m_windowWidth = m_width;
		m_windowHeight = m_height;
		m_appBufWidth = m_width;
		m_appBufHeight = m_height;
		break;
	}
	case (Event::Orientation_Up):
	default: {
		m_windowWidth = m_width;
		m_windowHeight = m_height - (m_enableFullScreen ? 0 :
									 Settings::LunaSettings()->positiveSpaceTopPadding);
		m_appBufWidth = m_width;
		m_appBufHeight = m_height;
		break;
	}
	}

	int savedWindowWidth = m_windowWidth;
	int savedWindowHeight = m_windowHeight;
	m_windowWidth = m_appBufWidth;
	m_windowHeight = m_appBufHeight;

    WebAppBase::resizeWebPage(m_windowWidth, m_windowHeight);

	if (m_orientation == Event::Orientation_Up && !m_enableFullScreen) {
		setVisibleDimensions(m_windowWidth, m_windowHeight);
	}
	else {
		setVisibleDimensions(m_width, m_height);
	}	
	
	// Force a full paint onto a temporary surface
    QImage toSceneSurface(m_width, m_height, QImage::Format_ARGB32_Premultiplied);
    QPainter toSceneContext;

    forcePaint();
    // Now we can kick off the animation
	int currAngleForAnim = angleForOrientation(oldOrientation) - angleForOrientation(m_orientation);
	if (currAngleForAnim > 180)
		currAngleForAnim = -90;
	else if (currAngleForAnim < -180)
		currAngleForAnim = 90;
	int targAngleForAnim = 0;

	updateWindowProperties();

    QPainter* gc = m_data->qtRenderingContext();

//	WindowContentTransitionRunner::instance()->runRotateTransition(this,
//																   toSceneSurface,
//																   gc,
//																   currAngleForAnim,
//																   targAngleForAnim);

//	toSceneSurface->releaseRef();
//	toSceneContext->releaseRef();

	if (savedWindowWidth != (int) m_windowWidth ||
		savedWindowHeight != (int) m_windowHeight) {
		m_windowWidth = savedWindowWidth;
		m_windowHeight = savedWindowHeight;
        WebAppBase::resizeWebPage(m_windowWidth, m_windowHeight);
		//The visible dimensions were artificially expanded above.  After resizing webkit, we need
		//to also resize the visible dimensions

		if (m_orientation == Event::Orientation_Up && !m_enableFullScreen) {
			setVisibleDimensions(m_windowWidth, m_windowHeight);
		}
		else {
			setVisibleDimensions(m_width, m_height);
		}
	}

	m_channel->sendAsyncMessage(new ViewHost_Card_SetAppOrientation(routingId(), orient));
	
    //animationFinished();
}

void CardWebApp::resizeWindowForFixedOrientation(Event::Orientation orient)
{
	if (!Settings::LunaSettings()->displayUiRotates || !m_allowsOrientationChange)
		return;

	if (orient == Event::Orientation_Invalid)
		return;

	// already there?
//	if (orient == m_fixedOrientation)
//		return;

	// sanity check (when the UI rotates the app orientation should always be UP)
	if (Event::Orientation_Up != m_orientation)
		return;

	m_fixedOrientation = orient;

	int widthAdj = Settings::LunaSettings()->positiveSpaceTopPadding;
	int heightAdj = 0;

	getFixedOrientationDimensions(m_width, m_height, widthAdj, heightAdj);

	m_windowWidth = m_width - (m_enableFullScreen ? 0 : widthAdj);
	m_windowHeight = m_height - (m_enableFullScreen ? 0 : heightAdj);

	m_appBufWidth = m_width;
	m_appBufHeight = m_height;

	m_setWindowWidth = m_windowWidth;
	m_setWindowHeight = m_windowHeight;

    resizeWebPage(m_windowWidth, m_windowHeight);

	setVisibleDimensions(m_windowWidth, m_windowHeight);
}

void CardWebApp::getFixedOrientationDimensions(int& width, int& height, int& wAdjust, int& hAdjust)
{
	bool appRequestedPortrait = false;
	bool uiIsInPortraitMode = false;

	if(WebAppManager::instance()->currentUiWidth() >= WebAppManager::instance()->currentUiHeight()) {
		uiIsInPortraitMode = false;
	} else {
		uiIsInPortraitMode = true;
	}

	appRequestedPortrait = isOrientationPortrait(m_fixedOrientation);

	if(appRequestedPortrait == uiIsInPortraitMode) {
		width = WebAppManager::instance()->currentUiWidth();
		height = WebAppManager::instance()->currentUiHeight();

		wAdjust = 0;
		hAdjust = Settings::LunaSettings()->positiveSpaceTopPadding;
	} else {
		width = WebAppManager::instance()->currentUiHeight();
		height = WebAppManager::instance()->currentUiWidth();

		wAdjust = Settings::LunaSettings()->positiveSpaceTopPadding;
		hAdjust = 0;
	}
}

bool CardWebApp::isOrientationPortrait(Event::Orientation orient)
{
	bool isPortrait = false;

	switch (orient) {
		case (Event::Orientation_Left):
		case (Event::Orientation_Right):
		{
			if(Settings::LunaSettings()->homeButtonOrientationAngle == 0 || Settings::LunaSettings()->homeButtonOrientationAngle == 180)
				isPortrait = !WebAppManager::instance()->isDevicePortraitType();
			else
				isPortrait = WebAppManager::instance()->isDevicePortraitType();
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
				isPortrait = WebAppManager::instance()->isDevicePortraitType();
			else
				isPortrait = !WebAppManager::instance()->isDevicePortraitType();
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

void CardWebApp::setFixedOrientation(Event::Orientation orient)
{

	if(!Settings::LunaSettings()->displayUiRotates) {
		m_fixedOrientation = orientationForThisCard(orient);
		setOrientation(m_fixedOrientation);
	} else {

		m_fixedOrientation = orientationForThisCard(orient);

		m_channel->sendAsyncMessage(new ViewHost_Card_SetAppFixedOrientation(routingId(), m_fixedOrientation, isOrientationPortrait(m_fixedOrientation)));

	}
}

bool CardWebApp::allowsOrientationChange() const
{
	return m_allowsOrientationChange;
}

int CardWebApp::angleForOrientation(Event::Orientation orient) const
{
    switch (orient) {
	case (Event::Orientation_Down):
		return 180;
	case (Event::Orientation_Left):
		return 90;
    case (Event::Orientation_Right):
        return 270;
	default:
		break;
	}

	return 0;
}

void CardWebApp::enableFullScreenMode(bool enable)
{
	if (m_enableFullScreen == enable) {
		return;
	}

	m_enableFullScreen = enable;

	if (m_orientation == Event::Orientation_Up) {

		// We allow window resizes only in UP orientation mode

		int newWidth = m_width;
		int newHeight = m_height;
		
		if (!enable)
			newHeight -= Settings::LunaSettings()->positiveSpaceTopPadding;

		if ((int) m_windowWidth == newWidth &&
			(int) m_windowHeight == newHeight)
			goto Done;

		m_windowWidth = newWidth;
		m_windowHeight = newHeight;
        resizeWebPage(m_windowWidth, m_windowHeight);

		setVisibleDimensions(newWidth, newHeight);
	}
	else {

		// in other orientation modes, full screen is always true
		m_enableFullScreen = true;
	}

Done:		
	updateWindowProperties();

}

void CardWebApp::invalContents(int x, int y, int width, int height)
{
    // this callback is called through QtWebKits onRepaintRequested mechanism.
    // we use it to handle our ipc buffer (card view) paints. In the future we
    // should try to have a window surface that wraps around the ipc buffer
    // and just pass a widget with that surface as the viewport.
    invalidateScene(QRectF(x, y, width, height));
    // paint timer will call our overloaded ::paint from WindowedWebApp
    startPaintTimer();
}

void CardWebApp::invalidate()
{
    invalContents(0, 0, m_appBufWidth, m_appBufHeight);
}

void CardWebApp::addChildCardWebApp(CardWebApp* app)
{
	if (m_childWebApp) {
		g_critical("%s: Trying to pile on multiple cross-app scenes", __PRETTY_FUNCTION__);
		return;
	}
	
    m_childWebApp = app;
}

void CardWebApp::removeChildCardWebApp(CardWebApp* app)
{
    m_childWebApp = 0;
}

void CardWebApp::setParentCardWebApp(CardWebApp* app)
{
	m_parentWebApp = app; 
}

void CardWebApp::attach(SysMgrWebBridge* page)
{
	if (m_winType == Window::Type_ChildCard) {

		// this is a child card

		m_parentWebApp = static_cast<CardWebApp*>(WebAppManager::instance()->findApp(page->launchingProcessId()));
		m_parentWebApp->addChildCardWebApp(this);

		m_data = m_parentWebApp->m_data;
		m_metaDataBuffer = m_parentWebApp->m_metaDataBuffer;
		m_directRendering = m_parentWebApp->m_directRendering;
		m_renderOffsetX = m_parentWebApp->m_renderOffsetX;
		m_renderOffsetY = m_parentWebApp->m_renderOffsetY;

		if (Settings::LunaSettings()->displayUiRotates) {
			setOrientation(WebAppManager::instance()->orientation());
		}
	} 

	//Perform the attach before trying to send any messages to it or else there
	//won't be a window to receive them.

	WindowedWebApp::attach(page);

	if(m_fixedOrientation != Event::Orientation_Invalid) {
		if(!Settings::LunaSettings()->displayUiRotates) {
			m_allowsOrientationChange = true;
			setOrientation(m_fixedOrientation);
		} else {
			m_allowsOrientationChange = false;
			m_channel->sendAsyncMessage(new ViewHost_Card_SetAppFixedOrientation(routingId(), m_fixedOrientation, isOrientationPortrait(m_fixedOrientation)));
		    resizeWindowForFixedOrientation(m_fixedOrientation);
		}
	}

	if (m_winType == Window::Type_Card || m_winType == Window::Type_ModalChildWindowCard) {

		AppLaunchOptionsEvent ops_ev;

		ops_ev.splashBackground = std::string();
		ops_ev.launchInNewGroup = false;

		// Cards have the option to specify a custom splash background
		const StringVariantMap& stageArgs = page->stageArguments();
		if(stageArgs.size() != 0) {
		    StringVariantMap::const_iterator it = stageArgs.find("splashbackgroundname");
		    if (it == stageArgs.end())
			it = stageArgs.find("splashbackground");
		    if (it != stageArgs.end()) {
			ops_ev.splashBackground = it.value().toString().toStdString();
		    }

		    it = stageArgs.find("launchinnewgroup");
		    if (it != stageArgs.end()) {
			ops_ev.launchInNewGroup = it.value().toBool();
		    }

		    it = stageArgs.find("statusbarcolor");
		    if (it != stageArgs.end()) {
			unsigned int color = (unsigned int)(it.value().toInt());
			WindowProperties prop;
			prop.setStatusBarColor(color);
			setWindowProperties(prop);
		    }

		    if (!ops_ev.splashBackground.empty() || ops_ev.launchInNewGroup != false) {
			m_channel->sendAsyncMessage(new ViewHost_Card_AppLaunchOptionsEvent(routingId(), AppLaunchOptionsEventWrapper(&ops_ev)));
		    }
		}
	}
	if(m_winType != Window::Type_ModalChildWindowCard)
		setVisibleDimensions(m_width, m_height - Settings::LunaSettings()->positiveSpaceTopPadding);
	else
		setVisibleDimensions(m_width, m_height);

    m_webview->setPage(page->page());
    page->page()->settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled, false);
}

SysMgrWebBridge* CardWebApp::detach()
{
	return WindowedWebApp::detach();
}

bool CardWebApp::isWindowed() const
{
    return m_winType != Window::Type_ChildCard;
}

bool CardWebApp::isChildApp() const
{
	return m_winType == Window::Type_ChildCard;
}

bool CardWebApp::isLeafApp() const
{
    return m_childWebApp == 0;
}

void CardWebApp::loadFinished()
{
	if (m_winType != Window::Type_Card) {
		WindowedWebApp::loadFinished();
	}

	// fake this
	if (m_winType == Window::Type_ChildCard) {
		m_addedToWindowMgr = true;
	}
}


void CardWebApp::handlePendingChanges()
{
	if (m_pendingOrientation == Event::Orientation_Invalid &&
		m_pendingFullScreenMode == -1 &&
		m_pendingResizeWidth == -1 &&
		m_pendingResizeHeight == -1) {

		// nothing to do
		return;
	}

	if (m_pendingOrientation != Event::Orientation_Invalid &&
		Settings::LunaSettings()->displayUiRotates)  {
		if (m_fixedOrientation != Event::Orientation_Invalid) {
			//NOTE: m_fixedOrientation is in system orientation, but orientationForThisCard()
			//can be called to map app-to-system and system-to-app.
			m_orientation = orientationForThisCard(m_fixedOrientation);
		}
		else if (m_allowsOrientationChange) {
			m_orientation = orientationForThisCard(m_pendingOrientation);
		}
	}

    if (m_pendingFullScreenMode != -1 && WindowedWebApp::windowType() != Window::Type_DockModeWindow) {
        if (m_enableFullScreen == m_pendingFullScreenMode) {
			//Requesting we switch to a state we're already in.  This means
			//there's nothing to do and we should suppress the processing
			//of the request.
			m_pendingFullScreenMode = -1;
		} else {
			m_enableFullScreen = m_pendingFullScreenMode;
		}
	}

	switch (m_orientation) {
	case (Event::Orientation_Left):
	case (Event::Orientation_Right): {
		// Full screen in this mode
		m_windowWidth = m_height;
		m_windowHeight = m_width;
		m_appBufWidth = m_height;
		m_appBufHeight = m_width;
		break;
	}
	case (Event::Orientation_Down): {
		// Full screen in this mode
		m_windowWidth = m_width;
		m_windowHeight = m_height;
		m_appBufWidth = m_width;
		m_appBufHeight = m_height;
		break;
	}
	case (Event::Orientation_Up):
	default: {
		m_appBufWidth = m_width;
		m_appBufHeight = m_height;

		if (m_enableFullScreen) {
			m_windowWidth = m_width;
			m_windowHeight = m_height;
		}
		else if (m_pendingResizeWidth != -1 &&
				 m_pendingResizeHeight != -1) {
			m_windowWidth = m_pendingResizeWidth;
			m_windowHeight = m_pendingResizeHeight;
		}
		else {
			m_windowWidth = m_setWindowWidth;
			m_windowHeight = m_setWindowHeight;
			if (m_pendingFullScreenMode != -1 && !m_enableFullScreen) {
				//If leaving full screen mode, adjust the window down
				//for the height of the status bar.
				m_windowHeight = m_windowHeight - Settings::LunaSettings()->positiveSpaceTopPadding;
			}
		}

		break;
	}
	}

	// only resize the window if something was pending that could trigger it
	if (m_pendingResizeWidth != -1 || m_pendingResizeHeight != -1 ||
		m_pendingFullScreenMode != -1 ||
		m_pendingOrientation != Event::Orientation_Invalid)	{
        resizeWebPage(m_windowWidth, m_windowHeight);
	}

	if (m_orientation == Event::Orientation_Up && !m_enableFullScreen) {
		setVisibleDimensions(m_width, m_height - Settings::LunaSettings()->positiveSpaceTopPadding);
	}
	else {
		setVisibleDimensions(m_width, m_height);
	}	

	m_setWindowWidth = m_windowWidth;
	m_setWindowHeight = m_windowHeight;
	
    setTransform(QTransform()); // identity transform
    rotate(angleForOrientation(m_CardOrientation));
	updateWindowProperties();

	Event::Orientation newOrient = m_pendingOrientation;
	
	m_pendingResizeWidth = -1;
	m_pendingResizeHeight = -1;
	m_pendingOrientation = Event::Orientation_Invalid;
	m_pendingFullScreenMode = -1;

	if (newOrient != Event::Orientation_Invalid) {
		callMojoScreenOrientationChange();
	}
}

void CardWebApp::receivePageUpDownInLandscape(bool val)
{
	m_doPageUpDownInLandscape = val;
}

void CardWebApp::updateWindowProperties()
{
	WindowProperties winProps;
	
	if (m_enableFullScreen)
		winProps.setFullScreen(true);
	else
		winProps.setFullScreen(!Settings::LunaSettings()->displayUiRotates && (m_orientation != Event::Orientation_Up));

	switch (m_orientation) {
	case (Event::Orientation_Down):
		winProps.setOverlayNotificationsPosition(WindowProperties::OverlayNotificationsTop);
		break;
	case (Event::Orientation_Left):
		winProps.setOverlayNotificationsPosition(WindowProperties::OverlayNotificationsLeft);
		break;
	case (Event::Orientation_Right):
		winProps.setOverlayNotificationsPosition(WindowProperties::OverlayNotificationsRight);
		break;
	case (Event::Orientation_Up):
	default:
		winProps.setOverlayNotificationsPosition(WindowProperties::OverlayNotificationsBottom);
		break;
	}

	WindowedWebApp::setWindowProperties(winProps);
}

void CardWebApp::callMojoScreenOrientationChange()
{
    Event::Orientation orient = orientationForThisCard(WebAppManager::instance()->orientation());

	callMojoScreenOrientationChange(orient);
}

void CardWebApp::callMojoScreenOrientationChange(Event::Orientation orient)
{
/*
	// FIXME: We should call the callback directly without using a script
	static const char* scriptUp    = "Mojo.screenOrientationChanged(\"up\")";
	static const char* scriptDown  = "Mojo.screenOrientationChanged(\"down\")";
	static const char* scriptLeft  = "Mojo.screenOrientationChanged(\"left\")";
	static const char* scriptRight = "Mojo.screenOrientationChanged(\"right\")";

	const char* script = 0;
	
	switch (orient) {
	case (Event::Orientation_Up):    script = scriptUp; break;
	case (Event::Orientation_Down):  script = scriptDown; break;
	case (Event::Orientation_Left):  script = scriptLeft; break;
	case (Event::Orientation_Right): script = scriptRight; break;
	default: return;
	}

	if (script && m_page && m_page->webkitPage()) {

		// Recursion protector
		static bool s_alreadyHere = false;
		if (!s_alreadyHere) {
			s_alreadyHere = true;
			m_page->webkitPage()->evaluateScript(script);
			s_alreadyHere = false;
		}
	}
*/
}

void CardWebApp::onSetComposingText(const std::string& text)
{
	if (m_childWebApp) {
		m_childWebApp->onSetComposingText(text);
		return;
	}
	WindowedWebApp::onSetComposingText(text);
}

void CardWebApp::onCommitComposingText()
{
	if (m_childWebApp) {
		m_childWebApp->onCommitComposingText();
		return;
	}
	WindowedWebApp::onCommitComposingText();
}

void CardWebApp::onCommitText(const std::string& text)
{
	if (m_childWebApp) {
		m_childWebApp->onCommitText(text);
		return;
	}
	WindowedWebApp::onCommitText(text);

}

void CardWebApp::onPerformEditorAction(int action)
{
	if (m_childWebApp) {
		m_childWebApp->onPerformEditorAction(action);
		return;
	}
	WindowedWebApp::onPerformEditorAction(action);
}

void CardWebApp::onRemoveInputFocus()
{
	if (m_childWebApp) {
		m_childWebApp->onRemoveInputFocus();
		return;
	}
	WindowedWebApp::onRemoveInputFocus();
}

void CardWebApp::onInputEvent(const SysMgrEventWrapper& wrapper)
{
/*
    if (Settings::LunaSettings()->enableTouchEventsForWebApps && 
        page()->isTouchEventsNeeded()) {
        // filter out mouse related events
        switch (wrapper.event->type) {
        case Event::PenDown:
        case Event::PenMove:
        case Event::PenUp:
        case Event::PenCancel:
        case Event::PenCancelAll:
            return;
        default:
            break;
        }
    }
*/
    WindowedWebApp::onInputEvent(wrapper);
}

void CardWebApp::setVisibleDimensions(int width, int height)
{
	m_channel->sendAsyncMessage(new ViewHost_SetVisibleDimensions(routingId(), width, height));
}

void CardWebApp::onDirectRenderingChanged()
{
	if (!m_data->hasDirectRendering())
		return;

	bool directRendering = false;
	int renderOffsetX = 0;
	int renderOffsetY = 0;
    SysMgrEvent::Orientation renderOrientation = Event::Orientation_Invalid;

	WindowMetaData* metaData = (WindowMetaData*) m_metaDataBuffer->data();
	m_metaDataBuffer->lock();
	renderOffsetX = metaData->directRenderingScreenX;
	renderOffsetY = metaData->directRenderingScreenY;
    m_webview->setPos(renderOffsetX, renderOffsetY);
    renderOrientation = static_cast<SysMgrEvent::Orientation>(metaData->directRenderingOrientation);
	directRendering = metaData->allowDirectRendering;
	m_metaDataBuffer->unlock();

	directRenderingChanged(directRendering, renderOffsetX, renderOffsetY, renderOrientation);
}

void CardWebApp::directRenderingChanged(bool directRendering, int renderOffsetX, int renderOffsetY, SysMgrEvent::Orientation renderOrientation)
{
    qDebug() << "directRendering appid: " << (page() ? page()->appId() : QString("unknown"))
             << ", processid: " << (page() ? page()->processId() : QString("unknown"))
             << ", enabled: " << directRendering
             << ", offset: " << renderOffsetX << "," << renderOffsetY
             << ", orientation: " << renderOrientation;

	m_renderOffsetX = renderOffsetX;
	m_renderOffsetY = renderOffsetY;
	m_renderOrientation = renderOrientation;

	if (m_childWebApp) {
		m_childWebApp->directRenderingChanged(directRendering, renderOffsetX, renderOffsetY, renderOrientation);
		m_directRendering = directRendering;
		return;
	}

    m_webview->setGeometry(QRectF(m_renderOffsetX, m_renderOffsetY, m_windowWidth, m_windowHeight));
	if (directRendering)
		directRenderingAllowed();
    else
		directRenderingDisallowed();
}

void CardWebApp::directRenderingAllowed()
{
	WebAppDeferredUpdateHandler::directRenderingActive(this);

	if (!m_data->hasDirectRendering())
		return;

    qDebug() << (page() ? page()->appId() : QString("unknown")) << ":" 
             << (page() ? page()->processId() : QString("unknown")) << " doing direct rendering";

    #if defined(HAVE_OPENGL)
    // lazy instantiation of our QGLWidget viewport when direct rendering becomes available.
    if (!m_glw) {
        RemoteWindowDataOpenGLQt* rd = static_cast<RemoteWindowDataOpenGLQt*>(m_data);
        QGLContext* ctx = new QGLContext(rd->getWidget()->format());
        ctx->create(rd->getWidget()->context()); // do we need to make sure the contexts are shared here?
        m_glw = new QGLWidget(ctx);
        setViewport(m_glw);
    }
    #endif

    setTransform(QTransform()); // identity transform
    rotate(angleForOrientation(m_renderOrientation));

    page()->page()->settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled, true);

    m_directRendering = true;
    m_data->directRenderingAllowed(m_directRendering);
    show();
}

void CardWebApp::directRenderingDisallowed()
{
	WebAppDeferredUpdateHandler::directRenderingInactive(this);

	if (!m_data->hasDirectRendering())
		return;

    qDebug() << (page() ? page()->appId() : QString("unknown")) << ":"
             << (page() ? page()->processId() : QString("unknown"))
             << " doing offscreen rendering";

	m_directRendering = false;
	m_renderOffsetX = 0;
	m_renderOffsetY = 0;
	m_renderOrientation = Event::Orientation_Invalid;
	m_data->directRenderingAllowed(m_directRendering);
    page()->page()-> settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled, false);
    hide();
}

void CardWebApp::displayOn()
{
	if (!m_paintingDisabled)
		return;
	
    qDebug() << (page() ? page()->appId() : QString("unknown")) << ":"
             << (page() ? page()->processId() : QString("unknown"))
             << " resuming paints";

	m_paintingDisabled = false;
    forcePaint();
}

void CardWebApp::displayOff()
{
	if (m_paintingDisabled)
		return;

    qDebug() << (page() ? page()->appId() : QString("unknown")) << ":"
             << (page() ? page()->processId() : QString("unknown"))
             << " suspending paints";

	m_paintingDisabled = true;
}

Event::Orientation CardWebApp::orientationForThisCard(Event::Orientation orient)
{
		int angle = Settings::LunaSettings()->homeButtonOrientationAngle;

		// translate the orientation requested by the card to an orientation that matches the physical layout of the device

		switch (angle) {
            // Adding case for handling orientation for Opal devices
            // As our orientations are reversed in topaz, we'll be
            // Continuing with the same values.
            case 0: // For Opal
            {
                switch(orient)
                {
                case (Event::Orientation_Left):
                     return Event::Orientation_Right;
                case (Event::Orientation_Right):
                    return Event::Orientation_Left;
                default:
                    return orient;
                }
                break;
            }

			case 90:
			{
				switch (orient) {
				case (Event::Orientation_Up):
					return Event::Orientation_Left; //rotation by 270 deg
					break;
				case (Event::Orientation_Down):
					return Event::Orientation_Right; //rotation by 270 deg
					break;
				case (Event::Orientation_Left):
					return Event::Orientation_Up; //rotation by 90 deg
					break;
				case (Event::Orientation_Right):
					return Event::Orientation_Down; //rotation by 90 deg
					break;
				case (Event::Orientation_Landscape):
					return Event::Orientation_Landscape;
					break;
				case (Event::Orientation_Portrait):
					return Event::Orientation_Portrait;
					break;
				default:
					return Event::Orientation_Invalid;
				}
			}
			break;

			case -180:
			case 180:
			{
				switch (orient) {
				case (Event::Orientation_Up):
						return Event::Orientation_Down;
						break;
				case (Event::Orientation_Down):
						return Event::Orientation_Up;
						break;
				case (Event::Orientation_Left):
					return Event::Orientation_Right;
					break;
				case (Event::Orientation_Right):
					return Event::Orientation_Left;
					break;
				case (Event::Orientation_Landscape):
					return Event::Orientation_Landscape;
					break;
				case (Event::Orientation_Portrait):
					return Event::Orientation_Portrait;
					break;
				default:
					return Event::Orientation_Invalid;
				}
			}
			break;
			case -90:
            case 270://For Topaz
			{
				switch (orient) {
				case (Event::Orientation_Up):
					return Event::Orientation_Right; //rotation by 90 deg
					break;
				case (Event::Orientation_Down):
					return Event::Orientation_Left; //rotation by 90 deg
					break;
				case (Event::Orientation_Left):
                        return Event::Orientation_Down;
					break;
				case (Event::Orientation_Right):
                        return Event::Orientation_Up;
					break;
				case (Event::Orientation_Landscape):
					return Event::Orientation_Landscape;
					break;
				case (Event::Orientation_Portrait):
					return Event::Orientation_Portrait;
					break;
			  default:
			      return Event::Orientation_Invalid;
			  }
			}
			break;

			default:
				return orient;
		}

}

void CardWebApp::allowResizeOnPositiveSpaceChange(bool allowResize)
{
	WindowProperties props;
	props.setAllowResizeOnPositiveSpaceChange(allowResize);

	WindowedWebApp::setWindowProperties(props);
}

void CardWebApp::thawFromCache()
{

	luna_assert(page());

	if (!inCache()) {
		focus();
		return;
	}

	if (m_winType == Window::Type_Card || m_winType == Window::Type_ChildCard)
		WebAppDeferredUpdateHandler::registerApp(this);

	// Resume all timers in the app before we do the rest of the thawing work
	// which may indirectly rely on timers working.
//	m_page->webkitPage()->throttle(100, 0);

	WebAppCache::remove(this);
	page()->page()->mainFrame()->evaluateJavaScript("if (window.Mojo && Mojo.show) Mojo.show()");

    qDebug("THAWING app %s", page()->appId().toStdString().c_str());

	if (m_fixedOrientation == Event::Orientation_Invalid) {
		WebAppManager* wam = WebAppManager::instance();
		if (m_width != wam->currentUiWidth() ||
			m_height != wam->currentUiHeight())
			flipEvent(wam->currentUiWidth(), wam->currentUiHeight());
	}
	
	m_channel->sendAsyncMessage(new ViewHost_PrepareAddWindowWithMetaData(routingId(), metadataId(),
																		  m_winType, m_width, m_height));		
	m_channel->sendAsyncMessage(new ViewHost_SetAppId(routingId(), page()->appId().toStdString().c_str()));
	m_channel->sendAsyncMessage(new ViewHost_SetProcessId(routingId(), page()->processId().toStdString().c_str()));
	m_channel->sendAsyncMessage(new ViewHost_SetLaunchingAppId(routingId(), page()->launchingAppId().toStdString().c_str()));
	m_channel->sendAsyncMessage(new ViewHost_SetLaunchingProcessId(routingId(), page()->launchingProcessId().toStdString().c_str()));
	m_channel->sendAsyncMessage(new ViewHost_SetName(routingId(), page()->name().toStdString().c_str()));

	
	stagePreparing();
	invalidate();
	stageReady();    

	EventReporter::instance()->report("launch", page()->appId().toStdString().c_str());

	// Send our window properties to the sysmgr side
	setWindowProperties(m_winProps);

	markInCache(false);

}

void CardWebApp::freezeInCache()
{
    luna_assert(page());

    qDebug("CACHING app %s", page()->appId().toStdString().c_str());

	if (m_winType == Window::Type_Card || m_winType == Window::Type_ChildCard)
		WebAppDeferredUpdateHandler::unregisterApp(this);

	if (m_data)
		m_channel->sendAsyncMessage(new ViewHost_RemoveWindow(routingId()));

//	m_page->webkitView()->setSupportsAcceleratedCompositing(false);
//	m_page->webkitView()->unmapCompositingTextures();

	page()->page()->mainFrame()->evaluateJavaScript("if (window.Mojo && Mojo.hide) Mojo.hide()");
	WebAppCache::put(this);

	m_stagePreparing = false;
	m_stageReady = false;
	m_addedToWindowMgr = false;

	EventReporter::instance()->report("close", page()->appId().toStdString().c_str());

	// Suspend all timers in the app after we finish the freezing work.
//	m_page->webkitPage()->throttle(0, 0);

	markInCache(true);

}

void CardWebApp::focus()
{
	if (inCache()) {
		thawFromCache();
		return;
	}

	WindowedWebApp::focus();
}

void CardWebApp::suspendAppRendering()
{
	m_renderingSuspended = true;
	stopPaintTimer();
}

void CardWebApp::resumeAppRendering()
{
	m_renderingSuspended = false;
}

void CardWebApp::screenSize(int& width, int& height) {
	if(m_winType == Window::Type_ModalChildWindowCard){
		width = Settings::LunaSettings()->modalWindowWidth;
		height = Settings::LunaSettings()->modalWindowHeight;
	}
	else {
		width = WebAppManager::instance()->currentUiWidth();
		height = WebAppManager::instance()->currentUiHeight();
	}
}

void CardWebApp::forcePaint()
{
	bool oldRenderingSuspend = m_renderingSuspended;
	m_renderingSuspended = false;
    QPaintEvent p(QRect(0, 0, m_width, m_height));
    paintEvent(&p);
	m_renderingSuspended = oldRenderingSuspend;
}

CardWebApp* CardWebApp::parentWebApp() const
{
    return m_parentWebApp;
}

