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

#include <unistd.h>
#include <cjson/json.h>

#include <PIpcChannel.h>

#include "Debug.h"
#include "EventThrottler.h"
#include "EventReporter.h"
#include "Logging.h"
#include "Preferences.h"
#include "WebAppFactory.h"
#include "WindowedWebApp.h"
#include "SysMgrWebBridge.h"
#include "RemoteWindowData.h"
#include "Settings.h"
#include "Time.h"
#include "Utils.h"
#include "WebAppManager.h"
#include "WebKitKeyMap.h"
#include "WindowMetaData.h"
#include "DeviceInfo.h"

#include <QDebug>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

static const int kShowWindowTimeoutMs = 3000;
static const int kMinAllowedIntervalGestureEndToSingleTap = 500;

static const int kNumRecordedGestures = 5;
static const int s_recordedGestureAvgWeights[] = { 1, 2, 4, 8, 16 };

//#define DEBUG_WEBAPP_INPUT_EVENTS 1

WindowedWebApp::WindowedWebApp(int width, int height, Window::Type type, PIpcChannel *channel)
	: m_data(0)
	, m_metaDataBuffer(0)
	, m_metaData(0)
	, m_winType(type)
	, m_width(width)
	, m_height(height)
	, m_beingDeleted(false)
	, m_stagePreparing(true)
	, m_stageReady(false)
	, m_addedToWindowMgr(false)
	, m_windowWidth(-1)
	, m_windowHeight(-1)
	, m_blockCount(0)
	, m_blockPenEvents(false)
	, m_lastGestureEndTime(0)
	, m_showPageStats(false)
	, m_focused(false)
	, m_pendingFocus(PendingFocusNone)
	, m_showWindowTimer(WebAppManager::instance()->masterTimer(),
						this, &WindowedWebApp::showWindowTimeout)
    , m_generateMouseClick(false)
    , m_paintTimer(0)
{
	setChannel(channel);

	m_windowWidth = width;
	m_windowHeight = height;

	if (type == Window::Type_Launcher) {
		m_windowHeight -= Settings::LunaSettings()->positiveSpaceTopPadding;
	}
	
	if (width != 0 && height != 0)
		init();
}

WindowedWebApp::~WindowedWebApp()
{
	m_beingDeleted = true;

	stopPaintTimer();

	if (m_winType != Window::Type_ChildCard) {
		if(m_data) {
			m_channel->sendAsyncMessage(new ViewHost_RemoveWindow(routingId()));
			WebAppManager::instance()->windowedAppRemoved(this);
		}
	}
	// delete the page before m_data since webkit owns the gl context in gl compositing and needs to make it current when the layers get deleted
	cleanResources();
	delete m_data;
	delete m_metaDataBuffer;
}

void WindowedWebApp::init()
{
	if (m_data)  { 
		return;
	}

	m_data = RemoteWindowDataFactory::generate(m_width, m_height, isTransparent());
	m_data->setChannel(m_channel);

	m_metaDataBuffer = PIpcBuffer::create(sizeof(WindowMetaData));
	m_metaData = (WindowMetaData*) m_metaDataBuffer->data();
	m_metaData->init();

	m_data->setWindowMetaDataBuffer(m_metaDataBuffer);

	m_data->setSupportsDirectRendering(m_winType == Window::Type_Card);
}

void WindowedWebApp::closeWindowRequest() 
{
    WebAppManager::instance()->closePageRequest(page());
}

void WindowedWebApp::attach(SysMgrWebBridge* page)
{
	// Call into super
	WebAppBase::attach(page);

	if (isTransparent()) {
        // In order to have Webkit paint the background transparent 
        // we need to set the Base palette correctly.
        // Fixes part2 of OWEBOS-1522
        QPalette palette = page->page()->palette();
        palette.setBrush(QPalette::Base, Qt::transparent);
        page->page()->setPalette(palette);
    }
/*
    // for devices without a built-in physical keyboard, disable app side sticky state behavior
    m_page->webkitView()->setSupportsSingleAndLockStickyStates(DeviceInfo::instance()->keyboardAvailable());
*/


    connect(page, SIGNAL(signalInvalidateRect(const QRect&)), SLOT(slotInvalidateRect(const QRect&)));
    connect(page, SIGNAL(signalResizedContents(const QSize&)), SLOT(slotResizeContent(const QSize&)));
    connect(page, SIGNAL(signalGeometryChanged(const QRect&)), SLOT(slotGeometryChanged(const QRect&)));

    connect(page->page(), SIGNAL(windowCloseRequested()), this, SLOT(closeWindowRequest()));

    page->page()->setViewportSize(QSize(m_windowWidth, m_windowHeight));

	if (m_winType != Window::Type_ChildCard) {

		// wait until progress = 100 before actually adding the window.
		// We do the prepare add window here since we don't have the appid
		// info from earlier.
		m_channel->sendAsyncMessage(new ViewHost_PrepareAddWindowWithMetaData(routingId(), metadataId(),
																			  m_winType, m_width, m_height));		
		m_channel->sendAsyncMessage(new ViewHost_SetAppId(routingId(), this->page()->appId().toStdString()));
		m_channel->sendAsyncMessage(new ViewHost_SetProcessId(routingId(), this->page()->processId().toStdString()));
		m_channel->sendAsyncMessage(new ViewHost_SetLaunchingAppId(routingId(), this->page()->launchingAppId().toStdString()));
		m_channel->sendAsyncMessage(new ViewHost_SetLaunchingProcessId(routingId(), this->page()->launchingProcessId().toStdString()));
		m_channel->sendAsyncMessage(new ViewHost_SetName(routingId(), this->page()->name().toStdString()));

		g_debug("%s:%d,  page->stageReadyPending() = %d", __PRETTY_FUNCTION__, __LINE__, page->stageReadyPending());
		if (page->stageReadyPending()) {
			stageReady();
		}
	}
	
	if( m_winType == Window::Type_ChildCard ||
		m_winType == Window::Type_Card) {
		EventReporter::instance()->report( "launch", static_cast<ProcessBase*>(page)->appId().toUtf8().constData() );
	}

}

void WindowedWebApp::onMessageReceived(const PIpcMessage& msg)
{
	bool msgIsOk;
	
	IPC_BEGIN_MESSAGE_MAP(WindowedWebApp, msg, msgIsOk)
		IPC_MESSAGE_HANDLER(View_Focus, focusedEvent)
		IPC_MESSAGE_HANDLER(View_Resize, onResize)
		IPC_MESSAGE_HANDLER(View_SyncResize, onSyncResize)
		IPC_MESSAGE_HANDLER(View_InputEvent, onInputEvent)
		IPC_MESSAGE_HANDLER(View_KeyEvent, onKeyEvent)
		IPC_MESSAGE_HANDLER(View_TouchEvent, onTouchEvent)
		IPC_MESSAGE_HANDLER(View_Close, onClose)
		IPC_MESSAGE_HANDLER(View_DirectRenderingChanged, onDirectRenderingChanged)
		IPC_MESSAGE_HANDLER(View_ClipboardEvent_Cut, onClipboardEvent_Cut)
		IPC_MESSAGE_HANDLER(View_ClipboardEvent_Copy, onClipboardEvent_Copy)
		IPC_MESSAGE_HANDLER(View_ClipboardEvent_Paste, onClipboardEvent_Paste)
        IPC_MESSAGE_HANDLER(View_SelectAll, onSelectAll)
		IPC_MESSAGE_HANDLER(View_Flip, onFlip)
		IPC_MESSAGE_HANDLER(View_AsyncFlip, onAsyncFlip)
		IPC_MESSAGE_HANDLER(View_AdjustForPositiveSpace, onAdjustForPositiveSpace)
		IPC_MESSAGE_HANDLER(View_KeyboardShown, onKeyboardShow)
		IPC_MESSAGE_HANDLER(View_SetComposingText, onSetComposingText)
		IPC_MESSAGE_HANDLER(View_CommitComposingText, onCommitComposingText)
		IPC_MESSAGE_HANDLER(View_CommitText, onCommitText)
		IPC_MESSAGE_HANDLER(View_PerformEditorAction, onPerformEditorAction)
		IPC_MESSAGE_HANDLER(View_RemoveInputFocus, onRemoveInputFocus)
	IPC_END_MESSAGE_MAP()
}

int WindowedWebApp::getKey() const
{
	return m_data->key();
}

int WindowedWebApp::routingId() const
{
    return m_data->key();
}

int WindowedWebApp::metadataId() const
{
	return m_metaDataBuffer->key();    
}

void WindowedWebApp::onDisconnected()
{
	// NO OP
}

void WindowedWebApp::onResize(int width, int height, bool resizeBuffer)
{
	resizeEvent(width, height, resizeBuffer);
}

void WindowedWebApp::onFlip(int newWidth, int newHeight)
{
	flipEvent(newWidth, newHeight);
}

void WindowedWebApp::onAsyncFlip(int newWidth, int newHeight, int newScreenWidth, int newScreenHeight)
{
	asyncFlipEvent(newWidth, newHeight, newScreenWidth, newScreenHeight);
}

void WindowedWebApp::onSyncResize(int width, int height, bool resizeBuffer, int* newKey)
{
	*newKey = resizeEvent(width, height, resizeBuffer);
}

void WindowedWebApp::onAdjustForPositiveSpace(int width, int height)
{
/*
	if (!m_page || !m_page->webkitPage())
		return;

	gchar* script = g_strdup_printf("if (window.Mojo && window.Mojo.positiveSpaceChanged) {window.Mojo.positiveSpaceChanged(%d, %d);}",
									width, height);
	g_debug("WindowedWebApp::onAdjustForPositiveSpace %s", script);
	m_page->webkitPage()->evaluateScript((const char*) script);
	g_free(script);
*/
}

void WindowedWebApp::onKeyboardShow(bool val)
{
/*
	if (!m_page || !m_page->webkitPage())
		return;

	gchar* script = g_strdup_printf("if (window.Mojo && window.Mojo.keyboardShown) {window.Mojo.keyboardShown(%s);}",
									val ? "true" : "false");
	g_debug("WindowedWebApp::onKeyboardShown %s", script);
	m_page->webkitPage()->evaluateScript((const char*) script);
	g_free(script);
*/
}


void WindowedWebApp::onClose(bool disableKeepAlive)
{
    // force an app to be deleted 
    if (disableKeepAlive)
       setKeepAlive(false); 
	WebAppManager::instance()->closeAppInternal(this);
}

void WindowedWebApp::onDirectRenderingChanged()
{
	// NO OP
}

void WindowedWebApp::onClipboardEvent_Cut()
{
	page()->cut();
}

void WindowedWebApp::onClipboardEvent_Copy()
{
	page()->copy();
}

void WindowedWebApp::onClipboardEvent_Paste()
{
	page()->paste();
}

void WindowedWebApp::onSelectAll()
{
	page()->selectAll();
}

void WindowedWebApp::onSetComposingText(const std::string& text)
{
	page()->setComposingText(text.c_str());
}

void WindowedWebApp::onCommitComposingText()
{
	page()->commitComposingText();
}

void WindowedWebApp::onCommitText(const std::string& text)
{
	page()->commitText(text.c_str());
}

void WindowedWebApp::onPerformEditorAction(int action)
{
	page()->performEditorAction(static_cast<PalmIME::FieldAction>(action));
}

void WindowedWebApp::onRemoveInputFocus()
{
	page()->removeInputFocus();
}

void WindowedWebApp::windowSize(int& width, int& height)
{
	width = m_windowWidth;
	height = m_windowHeight;
}

void WindowedWebApp::screenSize(int& width, int& height)
{
	width = WebAppManager::instance()->currentUiWidth();
	height = WebAppManager::instance()->currentUiHeight();
}

void WindowedWebApp::paint()
{
    stopPaintTimer();

    if (!appLoaded())
        return;

    if (m_paintRect.isEmpty())
        return;

    QPainter* ctxt = m_data->qtRenderingContext();
    m_data->beginPaint();

    int px = m_paintRect.x();
    int py = m_paintRect.y();
    int pw = m_paintRect.width();
    int ph = m_paintRect.height();

    ctxt->setClipRect(px, py, pw, ph);

    page()->page()->mainFrame()->render(ctxt, QWebFrame::ContentsLayer, m_paintRect);

    m_paintRect.setRect(0, 0, 0, 0);

    m_data->endPaint(false, QRect());

    m_data->sendWindowUpdate(px, py, pw, ph);

    if (!m_paintRect.isEmpty())
        startPaintTimer();
}

void WindowedWebApp::onInputEvent(const SysMgrEventWrapper& wrapper)
{
	Event* evt = new Event;
	memcpy(&(evt->type), &(wrapper.event->type), sizeof(SysMgrEvent));

	sptr<Event> e = evt;
	inputEvent(e);
}

void WindowedWebApp::inputEvent(sptr<Event> e)
{
    SysMgrWebBridge* bridge = page();
    if (!bridge || !bridge->page())
        return;

    Event* evt = e.get();

    if(Event::isPenEvent(evt)) {
        if (evt->type == Event::PenDown) {
            QMouseEvent* qtEvent = new QMouseEvent(QEvent::MouseButtonPress, QPoint(evt->x, evt->y), Qt::LeftButton, Qt::LeftButton, 0);
            bridge->page()->event(qtEvent);
        } else if (evt->type == Event::PenUp) {
            QMouseEvent* qtEvent = new QMouseEvent(QEvent::MouseButtonRelease, QPoint(evt->x, evt->y), Qt::LeftButton, Qt::LeftButton, 0);
            bridge->page()->event(qtEvent);
            QWebHitTestResult hitTest = bridge->page()->mainFrame()->hitTestContent(QPoint(evt->x, evt->y));
            if (hitTest.isContentEditable()) {
                QWebElement element = hitTest.element();
                QString palmType = element.attribute("x-palm-input-type");
                QString attrType = element.attribute("type");
                QString inputType = !palmType.isEmpty() ? palmType : attrType;
                PalmIME::EditorState state;
                if(inputType.isEmpty() || !(inputType.compare("text", Qt::CaseInsensitive)))
                    state.type = PalmIME::FieldType_Text;
                else if(!(inputType.compare("password", Qt::CaseInsensitive))) 
                    state.type = PalmIME::FieldType_Password;
                else if(!(inputType.compare("search", Qt::CaseInsensitive))) 
                    state.type = PalmIME::FieldType_Search;
                else if(!(inputType.compare("range", Qt::CaseInsensitive))) 
                    state.type = PalmIME::FieldType_Range;
                else if(!(inputType.compare("email", Qt::CaseInsensitive))) 
                    state.type = PalmIME::FieldType_Email;
                else if(!(inputType.compare("number", Qt::CaseInsensitive))) 
                    state.type = PalmIME::FieldType_Number;
                else if(!(inputType.compare("itel", Qt::CaseInsensitive))) 
                    state.type = PalmIME::FieldType_Phone;
                else if(!(inputType.compare("url", Qt::CaseInsensitive))) 
                    state.type = PalmIME::FieldType_URL;
                else if(!(inputType.compare("color", Qt::CaseInsensitive))) 
                    state.type = PalmIME::FieldType_Color;
                else
                    state.type = PalmIME::FieldType_Text;

                editorFocusChanged(true, state);
            } else {
                editorFocusChanged(false, PalmIME::EditorState());
            }
        } else if (evt->type == Event::PenMove) {
            QMouseEvent* qtEvent = new QMouseEvent(QEvent::MouseMove, QPoint(evt->x, evt->y), Qt::NoButton, Qt::NoButton, 0);
            bridge->page()->event(qtEvent);
        } else if (evt->type == Event::PenFlick) {
            QString script = QString().sprintf("if (window.Mojo && window.Mojo.handleGesture) {window.Mojo.handleGesture('flick', {x: %d, y: %d, timeStamp: %u, xVel: %d, yVel: %d})}", evt->x, evt->y, evt->time, evt->flickXVel, evt->flickYVel);
            page()->page()->mainFrame()->evaluateJavaScript(script);
        }
    }
/*
	if (!m_page || !m_page->webkitPage()) {
        g_warning("%s: No page", __PRETTY_FUNCTION__);
		return;
    }
*/

/* To allow finger rejection to work together with multifinger gestures, we check if a gesture is handled,
 * and if it is, we disable regular pen events until the gesture stops.
 *
 * Since the finger rejection mechanism will cancel fingers and report Down/Up events only based of the most recent finger
 * We can end up in a situation where we get the last pen up, before we receive the GestureEnd.
 * We use m_blockCount to count PenDown events that we have blocked, and will keep blocking pen events until GestureEnd + m_blockCount is zero.
 * In addition, we need to deal with the case where the gestureStart did not get handled
 * (user taps down on, say, a luna button in the webbrowser, and then moves into the pinchable area.
 * We deal with this by also checking if the GestureChange got handled, and start blocking at that time.
 */
/*
	switch (e->type) {
	case (Event::PenDown): {
#ifdef DEBUG_WEBAPP_INPUT_EVENTS	
		g_debug("WindowedWebApp PenDown: x: %d, y: %d, click: %d, modifiers: 0x%08x, simulated: %d", e->x, e->y, e->clickCount, e->modifiers, e->simulated);
#endif
		if (m_blockPenEvents) {
			m_blockCount++;
#ifdef DEBUG_WEBAPP_INPUT_EVENTS
			g_debug("WindowedWebApp PenDown: m_blockCount: %d",m_blockCount);
#endif
			break;
		}

        WebAppManager::instance()->setInSimulatedMouseEvent(e->simulated);

        m_page->webkitView()->mouseEvent(Palm::MouseDown, e->x, e->y, e->clickCount,
			e->modifiers & Event::Shift, e->modifiers & Event::Control,
			e->modifiers & Event::Alt, e->modifiers & Event::Meta, e->simulated);

		WebAppManager::instance()->setInSimulatedMouseEvent(false);
		
		break;
	}
	case (Event::PenMove): {
		if (m_blockPenEvents)
			break;

#ifdef DEBUG_WEBAPP_INPUT_EVENTS	
		g_debug("WindowedWebApp PenMove: x: %d, y: %d, click: %d, modifiers: 0x%08x", e->x, e->y, e->clickCount, e->modifiers);
#endif

		WebAppManager::instance()->setInSimulatedMouseEvent(e->simulated);

		m_page->webkitView()->mouseEvent(Palm::MouseMove, e->x, e->y, 0,
										 e->modifiers & Event::Shift, e->modifiers & Event::Control,
										 e->modifiers & Event::Alt, e->modifiers & Event::Meta, e->simulated);
		
		WebAppManager::instance()->setInSimulatedMouseEvent(false);

		break;
	}
	case (Event::PenUp): {
#ifdef DEBUG_WEBAPP_INPUT_EVENTS	
		g_debug("WindowedWebApp PenUp: x: %d, y: %d, click: %d, modifiers: 0x%08x", e->x, e->y, e->clickCount, e->modifiers);
#endif
		if (m_blockPenEvents && m_blockCount > 0) {
			m_blockCount--;
#ifdef DEBUG_WEBAPP_INPUT_EVENTS
			g_debug("WindowedWebApp PenUp: m_blockCount: %d",m_blockCount);
#endif
			break;
		}
	
		WebAppManager::instance()->setInSimulatedMouseEvent(e->simulated);

        //printf(" WindowedWebApp::inputEvent()  e->modifiers=%08x\n", e->modifiers );
		m_page->webkitView()->mouseEvent(Palm::MouseUp, e->x, e->y, 0,
										 e->modifiers & Event::Shift, e->modifiers & Event::Control,
										 e->modifiers & Event::Alt, e->modifiers & Event::Meta, e->simulated);

		WebAppManager::instance()->setInSimulatedMouseEvent(false);

		break;
	}
    case (Event::PenCancel):
    case (Event::PenCancelAll): {
#ifdef DEBUG_WEBAPP_INPUT_EVENTS	
		g_debug("WindowedWebApp PenCancel: x: %d, y: %d", e->x, e->y);
#endif
		if (m_blockPenEvents && m_blockCount > 0) {
			m_blockCount--;
#ifdef DEBUG_WEBAPP_INPUT_EVENTS
			g_debug("WindowedWebApp PenCancel: m_blockCount: %d",m_blockCount);
#endif
			break;
		}
	
		m_page->webkitView()->mouseEvent(Palm::MouseUp, -10000, -10000, 0, e->simulated);
        break;
    }
	case (Event::GestureStart): {
		bool handled = m_page->webkitView()->gestureEvent(Palm::GestureStart,
														  e->gestureCenterX,e->gestureCenterY,
														  e->gestureRotate,e->gestureScale,
														  e->gestureCenterX,e->gestureCenterY);
#ifdef DEBUG_WEBAPP_INPUT_EVENTS
		g_debug("WindowedWebApp Gesture START: rotate: %f, scale: %f, center (%d,%d), handled: %d",
			   e->gestureRotate, e->gestureScale,e->gestureCenterX,e->gestureCenterY, handled);
#endif
		if (handled) {
			m_page->webkitView()->mouseEvent(Palm::MouseUp, -10000, -10000, 0, e->simulated);		
			m_blockPenEvents = true;
		}			
		break;
	}
	case (Event::GestureEnd): {
#ifdef DEBUG_WEBAPP_INPUT_EVENTS	
		g_debug("WindowedWebApp Gesture END: rotate: %f, scale: %f, center (%d,%d)",
			   e->gestureRotate, e->gestureScale, e->gestureCenterX,e->gestureCenterY);
#endif

		m_blockPenEvents = false;
		m_lastGestureEndTime = e->time;		

		m_page->webkitView()->gestureEvent(Palm::GestureEnd,
										  e->gestureCenterX,e->gestureCenterY,
										  e->gestureRotate,e->gestureScale,
										  e->gestureCenterX,e->gestureCenterY);

		break;
	}
	case (Event::GestureChange): {

		bool handled = m_page->webkitView()->gestureEvent(Palm::GestureChange,
														  e->gestureCenterX,e->gestureCenterY,
														  e->gestureRotate,e->gestureScale,
														  e->gestureCenterX,e->gestureCenterY);
		
#ifdef DEBUG_WEBAPP_INPUT_EVENTS
		g_debug("WindowedWebApp Gesture CHANGE: rotate: %f, scale: %f, center (%d,%d), handled: %d",
			   e->gestureRotate, e->gestureScale,e->gestureCenterX,e->gestureCenterY, handled);
#endif
		if (handled && !m_blockPenEvents) {
			m_page->webkitView()->mouseEvent(Palm::MouseUp, -10000, -10000, 0, e->simulated);
			m_blockPenEvents = true;
		}
		break;
	}

	case (Event::GestureCancel): {
#ifdef DEBUG_WEBAPP_INPUT_EVENTS
		g_debug("WindowedWebApp Gesture CANCEL\n");
#endif

		m_blockPenEvents = false;
		m_lastGestureEndTime = e->time;		

		RecordedGestureEntry avgGesture = getAveragedGesture();
		
		m_page->webkitView()->gestureEvent(Palm::GestureEnd,
										   avgGesture.centerX, avgGesture.centerY,
										   avgGesture.rotate, avgGesture.scale,
										   avgGesture.centerX, avgGesture.centerY);
		break;
	}

	case (Event::PenFlick): {
		// FIXME: The Time::curSysTimeMs() overflows
		if (m_blockPenEvents)
			break;

#ifdef DEBUG_WEBAPP_INPUT_EVENTS	
		g_debug("WindowedWebApp FLICK: x: %d, y: %d, time: %d, xVel: %d, yVel: %d", e->x, e->y, e->time, e->flickXVel, e->flickYVel);
#endif
		// FIXME: Remove soon
		gchar* script = g_strdup_printf("if (window.Mojo && window.Mojo.handleGesture) {window.Mojo.handleGesture('flick', {x: %d, y: %d, timeStamp: %u, xVel: %d, yVel: %d})}",
                                                e->x, e->y, e->time, e->flickXVel, e->flickYVel);
        Palm::WebFrame* target = m_page->webkitView()->targetFrame();
        if (m_page->isValidFrame(target))
            target->evaluateScript((const char*) script);
		g_free(script);
		
		break;		
	}
    case (Event::PenSingleTap): {

		if (m_blockPenEvents)
			break;
		
		gchar* script = g_strdup_printf("if (window.Mojo && Mojo.handleSingleTap) {Mojo.handleSingleTap({x: %d, y: %d, shiftKey: %d, ctrlKey: %d, altKey: %d, metaKey: %d, timestamp: %u})}",
                                                e->x, e->y,(e->modifiers & Event::Shift) ? 1 : 0, (e->modifiers & Event::Control) ? 1 : 0, (e->modifiers & Event::Alt) ? 1 : 0,
                                                (e->modifiers & Event::Meta) ? 1 : 0, e->time);

#ifdef DEBUG_WEBAPP_INPUT_EVENTS
		g_debug("WindowedWebApp PenSingleTap: x: %d, y: %d, click: %d, modifiers: 0x%08x time: %u", e->x, e->y, e->clickCount, e->modifiers, e->time);
#endif

        //Don't care about return, either someone handles it or not...
		m_page->webkitPage()->evaluateScript((const char*) script);
		g_free(script);

		m_page->webkitView()->gestureEvent(Palm::GestureSingleTap, e->x, e->y, 0.0, 0.0,
										   e->x, e->y,
										   (e->modifiers & Event::Shift),
										   (e->modifiers & Event::Control),
										   (e->modifiers & Event::Alt),
										   (e->modifiers & Event::Meta));
		break;
    }

    default:
		break;
    }
*/
}

void WindowedWebApp::onKeyEvent(const SysMgrKeyEvent& e)
{
    QKeyEvent ev = e.qtEvent();
    if (ev.key() == Qt::Key_Enter || ev.key() == Qt::Key_Return) {
        // Trap both key Enter and Return and make sure that we pass the key code as Qt::Key_Enter
        // This is because of a bug in WebCore/platform/qt/PlatformKeyboardEventQt.cpp.
        //
        // Also have the 'text' be an NULL QString, i.e. QString.isNull() == true.
        // When SysMgr process sends the QKeyEvent the luna-sysmgr-ipc-messages will
        // turn the isNull() value from true to false, i.e. replace it with an empty string.
        // See NAYA-118 for more details
        ev = QKeyEvent(ev.type(), Qt::Key_Enter, ev.modifiers(), QString());
    }
    keyEvent(&ev);
}

void WindowedWebApp::onTouchEvent(const SysMgrTouchEvent& e)
{
/*
	// sending this directly to webkit
	if (!m_page || !m_page->webkitPage()) {
	    return;
	}

	if (!m_page->isTouchEventsNeeded()) {
	    return;
	}

	Palm::TouchEventType  type;
	switch (e.type) {
    case QTouchEvent::TouchBegin:	
        type = Palm::TouchStart; 
        m_generateMouseClick = (e.numTouchPoints == 1); 
        break;
    case QTouchEvent::TouchUpdate:	
        type = Palm::TouchMove;
        // events are throttled on the sysmgr side which means that if we get an update,
        // it's because the user moved their finger out side the threshold of a tap.
        m_generateMouseClick = false;
        break;
    case QTouchEvent::TouchEnd:
        type = Palm::TouchEnd; 
        break;
    default: g_warning ("%s: unknown type %d\n", __PRETTY_FUNCTION__, e.type); return;
	}

	Palm::TouchPointPalm wkTouches[e.numTouchPoints];

	for (uint32_t i = 0; i < e.numTouchPoints; ++i) {

		SysMgrTouchPoint& touch = e.touchPoints[i];
	    switch (touch.state) {
		case Qt::TouchPointPressed:	wkTouches[i].state = Palm::TouchPointPalm::TouchPressed; break;
		case Qt::TouchPointMoved:	wkTouches[i].state = Palm::TouchPointPalm::TouchMoved; break;
		case Qt::TouchPointStationary:	wkTouches[i].state = Palm::TouchPointPalm::TouchMoved; break;
		case Qt::TouchPointReleased:	wkTouches[i].state = Palm::TouchPointPalm::TouchReleased; break;
		default: g_warning ("%s: unknown type of touchpoint state %d\n", __PRETTY_FUNCTION__, touch.state); continue;
	    }
	    
        wkTouches[i].id = touch.id;
		wkTouches[i].x = touch.x;
		wkTouches[i].y = touch.y;
	}

	//printf ("Sending webkit touch event of type %d number of touches %d\n", type, e.numTouchPoints);
	bool defaultPrevented = m_page->webkitView()->touchEvent(type, wkTouches, e.numTouchPoints,
									 (e.modifiers & Qt::ControlModifier) ?  true : false,
									 (e.modifiers & Qt::AltModifier) ?  true : false,
									 (e.modifiers & Qt::ShiftModifier) ?  true : false,
									 (e.modifiers & Qt::MetaModifier) ?  true : false);

    // don't send mouse events for recognized taps that the page has prevented a previous touch
    if (type == Palm::TouchStart && defaultPrevented)
        m_generateMouseClick = false;

//    if (type == Palm::TouchEnd && m_generateMouseClick) {
//        m_generateMouseClick = false;

        // only generate a mouse sequence if an item under the touch is considered clickable
//        if (m_page->webkitView()->isClickableAtPoint(wkTouches[0].x, wkTouches[0].y)) {

//            m_page->webkitView()->mouseEvent(Palm::MouseDown, wkTouches[0].x, wkTouches[0].y, 1,
//                e.modifiers & Qt::ShiftModifier, e.modifiers & Qt::ControlModifier,
//                e.modifiers & Qt::AltModifier, e.modifiers & Qt::MetaModifier, false);
//            m_page->webkitView()->mouseEvent(Palm::MouseUp, wkTouches[0].x, wkTouches[0].y, 0,
//                e.modifiers & Qt::ShiftModifier, e.modifiers & Qt::ControlModifier,
//                e.modifiers & Qt::AltModifier, e.modifiers & Qt::MetaModifier, false);
//        }
    }
*/
}

void WindowedWebApp::keyEvent(QKeyEvent* e)
{
    SysMgrWebBridge* bridge = page();
    if (!bridge || !bridge->page())
        return;

    bridge->page()->event(e);

/*
    // Toggle display of the memory stats.
    if( e->type() == QEvent::KeyPress && 
		e->key() == Qt::Key_Percent && 
		e->modifiers() == (Qt::ControlModifier|Qt::AltModifier) &&
		!e->isAutoRepeat() ) {
		m_showPageStats = !m_showPageStats;
		invalidate();
    }

    if ((e->key() >= Qt::Key_Escape && e->key() <= Qt::Key_ScrollLock) // This will need to be reviewed
	    ||
		(e->key() >= Qt::Key_Space && e->key() < 0xE000)	// allow all standard unicode values
	    ||
	    e->key() == Qt::Key_CoreNavi_Back) {

		unsigned short key = WebKitKeyMap::instance()->translateKey (e->key());

		// This is were we "un-shift" plain letters that arrive here using capitalized
		if (key >= Key_A && key <=Key_Z && !(e->modifiers() & Qt::ShiftModifier))
			key += Key_a - Key_A;

		// key events should abort any composing state
		page()->commitComposingText();

		unsigned short mod = 0;

		if(e->modifiers() & Qt::ShiftModifier) {
			g_debug ("setting shift modifier before sending to webkit");
			mod |= 0x80; // this is how we communicate it to WebCore (KeyEventPalm.cpp)
		}

        if (key == Key_Null) {
            return;
        }

		// In the pre-Qt sysmgr, when the Orange key was pressed, luna-sysmgr used luna-keymaps to find the
		// corresponding key and sent that to webkit without any modifiers.
		// In the post-Qt sysmgr, the Orange key is sent as a key and subsequent keys are sent with the 
		// non-modified key code and the modifier set if the key is held down (to be compatible with Qt on the 
		// desktop). However, this confused webkit.
		// So we are taking out the modifier. Sending the non-modified key without any modifier will allow webkit to 
		// find the right modified key since it maintains state of the Orange key. 
		// This applies only to the orange key. Shift and meta are to be handled as before.
		if( e->modifiers() & Qt::ControlModifier) 
			mod |= 0x40; // this is how we communicate it to WebCore (KeyEventPalm.cpp)
		if( e->modifiers() & Qt::MetaModifier) {
			g_message("%s: MetaModifier is set", __PRETTY_FUNCTION__);
			mod |= 0x20; // this is how we communicate it to WebCore (KeyEventPalm.cpp)
		}

#if 0 // to help debug key events
		g_warning("Sending webkit key event of value %02X '%s' mod %d press %d", key, QString(key).toUtf8().data(), mod, e->type() == QEvent::KeyPress);
#endif
		bool ret = m_page->webkitView()->keyEvent(key, mod,  e->type() == QEvent::KeyPress);

		if (!ret) {
			// Key was not handled. return it to the Window Server event queue

			// Temporary code to allow rejecting the Back Gesture key. This should be removed once Webkit converts to Qt Key events.
			if(e->key() == Qt::Key_CoreNavi_Back) {		
				QKeyEvent rejectedEvent(e->type(), e->key(), e->modifiers(), "");

				WebAppManager::instance()->sendAsyncMessage(new View_Host_ReturnedKeyEvent(&rejectedEvent));
			}
		}
    }
    else if (e->type() == QEvent::KeyRelease) {

		keyGesture(e);
    }
*/
}

void WindowedWebApp::focusedEvent(bool focused)
{
/*
	if (!m_page || !m_page->webkitPage())
		return;

	if (!appLoaded()) {
		m_pendingFocus = focused ? PendingFocusTrue : PendingFocusFalse;
		return;
	}

	// Some apps may rely on focused event being always sent in startup
	// if (m_focused == focused)
	// 	return;			

	g_message("Sending stage %s to app: %s", focused ? "activation" : "deactivation",
			  appId().c_str());
*/
    m_focused = focused;
    // The script will tie into the EnyoFW so that the javascritp apps get their 
    // windowActivated and windowDeActivated signals respectively
    QString script;
    if (focused) {
        WebAppManager::instance()->setActiveAppId(page()->getIdentifier());
        script = QString("if (window.Mojo && Mojo.stageActivated) {Mojo.stageActivated();}");
    } else
        script = QString("if (window.Mojo && Mojo.stageDeactivated) {Mojo.stageDeactivated();}");
    page()->page()->mainFrame()->evaluateJavaScript(script);

    QFocusEvent fEvent(QEvent::FocusIn);
    page()->event(&fEvent);

/*
	m_page->webkitView()->setViewIsActive(focused);

	// initialize input field focused state
	if (focused && isLeafApp()) {
		g_debug("%s (%s): isEditing: %d", __PRETTY_FUNCTION__, m_page->appId().c_str(), m_page->isEditing());
		editorFocusChanged(m_page->isEditing(), m_page->editorState());
		autoCapEnabled(m_page->lastAutoCap());
	}

*/
}

void WindowedWebApp::focus()
{
	m_channel->sendAsyncMessage(new ViewHost_FocusWindow(routingId()));
}

void WindowedWebApp::unfocus()
{
	m_channel->sendAsyncMessage(new ViewHost_UnfocusWindow(routingId()));
}

int WindowedWebApp::resizeEvent(int newWidth, int newHeight, bool resizeBuffer)
{
    if (!page() || !page()->page() || !m_data)
		return -1;

	if (m_windowWidth == (uint32_t) newWidth &&
		m_windowHeight == (uint32_t) newHeight)
		return -1;

	// Just resize the dimensions
	if (!resizeBuffer) {
		m_windowWidth = newWidth;
		m_windowHeight = newHeight;
        resizeWebPage(newWidth, newHeight);
		return -1;
	}

	int oldKey = m_data->key();
	m_data->resize(newWidth, newHeight);
	int newKey = m_data->key();

	m_windowWidth = newWidth;
	m_windowHeight = newHeight;
	m_width = newWidth;
	m_height = newHeight;

    resizeWebPage(newWidth, newHeight);

	m_paintRect.setRect(0, 0, newWidth, newHeight);
    paint();

	WebAppManager::instance()->windowedAppKeyChanged(this, oldKey);

	return newKey;
}

void WindowedWebApp::flipEvent(int newWidth, int newHeight)
{
	if (m_data)
		m_data->flip();

	int temp = m_width;

	m_width  = m_height;
	m_height = temp;

	m_windowWidth  = m_width;
	m_windowHeight = m_height;

    WebAppBase::resizeWebPage(m_windowWidth, m_windowHeight);

	m_paintRect.setRect(0, 0, m_windowWidth, m_windowHeight);
    paint();
}

void WindowedWebApp::asyncFlipEvent(int newWidth, int newHeight, int newScreenWidth, int newScreenHeight)
{
	if (m_data)
		m_data->flip();

	int tempWidth = m_width;
	m_width = m_height;
	m_height = tempWidth;

	m_windowWidth = m_width;
	m_windowHeight = m_height;

    WebAppBase::resizeWebPage(m_windowWidth, m_windowHeight);

	m_paintRect.setRect(0, 0, m_windowWidth, m_windowHeight);
    paint();

	// notify Host that this window is done resizing
	m_channel->sendAsyncMessage(new ViewHost_AsyncFlipCompleted(routingId(), newWidth, newHeight, newScreenWidth, newScreenHeight));
}


void WindowedWebApp::invalContents(int x, int y, int width, int height)
{
    QRect interSection(x, y, width, height);
    interSection &= QRect(0, 0, m_windowWidth, m_windowHeight);
    if (interSection.isEmpty())
        return;

    // Union (Combine) the rectanage to create a final rect
    m_paintRect |= interSection;

    startPaintTimer();
}

void WindowedWebApp::slotInvalidateRect(const QRect& rect)
{
    invalContents(rect.x(), rect.y(), rect.width(), rect.height());
}

void WindowedWebApp::slotResizeContent(const QSize& rect)
{
    qDebug() << __PRETTY_FUNCTION__ << rect;
}

void WindowedWebApp::slotGeometryChanged(const QRect& rect)
{
    qDebug() << __PRETTY_FUNCTION__ << rect;
}

void WindowedWebApp::scrollContents(int newContentsX, int newContentsY)
{
    invalidate();
}

void WindowedWebApp::startPaintTimer()
{
    // will call us after we are deleted
    if (m_beingDeleted)
        return;

    if (!m_paintTimer)
    {
        m_paintTimer = new QTimer(this);
        connect(m_paintTimer, SIGNAL(timeout()), this, SLOT(PrvCbPaintTimeout()));
    }

    m_paintTimer->setSingleShot(true);
    m_paintTimer->start(0);
}

void WindowedWebApp::stopPaintTimer()
{
    if (m_paintTimer) {
        m_paintTimer->stop();
    }
}

void WindowedWebApp::invalidate()
{
    slotInvalidateRect(QRect(0,0,m_windowWidth, m_windowHeight));
}

void WindowedWebApp::PrvCbPaintTimeout()
{
    paint();
}

void WindowedWebApp::loadFinished()
{
/*	
	if (m_page) {
		bool hasMojo = true;
		hasMojo = m_page->webkitPage()->evaluateScript("var sysmgrCheck = window.Mojo;");
		if (!hasMojo) {
			g_message("%s: No mojo found", __PRETTY_FUNCTION__);
			m_stagePreparing = false;
		}
	}
*/
	
	// if the framework called us with an explicit stagePreparing call we
	// will wait for the call to stageReady to come in
	if (m_stagePreparing && !m_stageReady) {

		if (!m_addedToWindowMgr && !m_showWindowTimer.running())
			m_showWindowTimer.start(kShowWindowTimeoutMs);
		return;
	}

	if (m_pendingFocus != PendingFocusNone) {
		focusedEvent(m_pendingFocus == PendingFocusTrue);
		m_pendingFocus = PendingFocusNone;
	}
	
	if (!m_addedToWindowMgr && m_winType != Window::Type_ChildCard) {
		qDebug() << __PRETTY_FUNCTION__ << ":" << __LINE__ << "Adding to windowManager: " << (page() ? page()->url() : QUrl());
        paint();

		m_channel->sendAsyncMessage(new ViewHost_AddWindow(routingId()));
		m_addedToWindowMgr=true;
	}
}

void WindowedWebApp::renderPageStatistics(int offsetX, int offsetY)
{
/*
	Palm::WebPageStatistics s;
	m_page->webkitPage()->getStatistics(s);
	
	static PGFont* debugFont = 0;

	if (!debugFont) {
		debugFont = NativeGraphicsFont::createFromFile( "/usr/share/fonts/Prelude-Bold.ttf", 18 );
	}

	PGContext* pg = (m_data ? m_data->renderingContext() : 0);
	
	if (debugFont && pg) {
		int x,y;
		const int len = 128;
		char buffer[len];
		
		const int kOffsetColors[3][5] = {
			{ 0,  0, 0, 0 },
			{ 2,  0, 0, 0 },
			{ 1,  0xff, 0xff, 0xff },
		};
		
		for( int i=0; i<3; i++ ) {
			x = 10-kOffsetColors[i][0] + offsetX;
			y = 24-kOffsetColors[i][0] + offsetY;
			const int yInc = 30;
		
			pg->setStrokeColor(PColor32(kOffsetColors[i][1], kOffsetColors[i][2], kOffsetColors[i][3], 0xff));
			
			snprintf(buffer, len-1, "%ld image(s), %ld k total", s.numImages, s.totalImagesBytes / 1024 );
			pg->drawTextA( buffer, strlen(buffer), x, y, debugFont );
			y += yInc;
	
			snprintf(buffer, len-1, "%ld DOM Nodes", s.numDOMNodes );
			pg->drawTextA( buffer, strlen(buffer), x, y, debugFont );
			y += yInc;
	
			snprintf(buffer, len-1, "%ld Render Nodes", s.numRenderTreeNodes );
			pg->drawTextA( buffer, strlen(buffer), x, y, debugFont );
			y += yInc;
	
			snprintf(buffer, len-1, "%ld Render Layer(s)", s.numRenderLayers );
			pg->drawTextA( buffer, strlen(buffer), x, y, debugFont );
		}
	}
	
	g_debug("\n\n---- STATS ----" );
	g_debug(" %s ", m_page->url() ); 
	g_debug(" %ld image(s), %ld k total (unique, encoded and decoded)", s.numImages, s.totalImagesBytes / 1024 );
	g_debug(" %ld DOM Nodes", s.numDOMNodes );
	g_debug(" %ld Render Nodes", s.numRenderTreeNodes );
	g_debug(" %ld Render Layer(s)", s.numRenderLayers );
*/
}

void WindowedWebApp::stagePreparing()
{
	qDebug() << __PRETTY_FUNCTION__ << ":" << __LINE__ << "," << (page() ? page()->url() : QUrl());
	m_stagePreparing = true;

	if (G_UNLIKELY(Settings::LunaSettings()->perfTesting)) {
        qDebug() << "SYSMGR PERF: APP PREPARE appid: " << (page() ? (page()->appId()) : QString("unknown"))
                 << ", processid: " << (page() ? (page()->processId()) : QString("unknown"))
                 << ", type: " << WebAppFactory::nameForWindowType(m_winType)
                 << ", time: " << Time::curTimeMs();
	}
}

void WindowedWebApp::stageReady()
{
    qDebug() << __PRETTY_FUNCTION__ << ":" << __LINE__ << (page() ? page()->url() : QUrl());
	m_stagePreparing = false;
	m_stageReady = true;
	page()->setStageReadyPending(false);
	
	if (!m_addedToWindowMgr && m_winType != Window::Type_ChildCard) {

        qDebug() << __PRETTY_FUNCTION__ << ":" << __LINE__ << "Adding to windowManager:" << (page() ? (page()->url()) : QUrl());
        paint();

	 	m_channel->sendAsyncMessage(new ViewHost_AddWindow(routingId()));
	 	m_addedToWindowMgr=true;
	}

	m_showWindowTimer.stop();

	if (m_pendingFocus != PendingFocusNone) {
		focusedEvent(m_pendingFocus == PendingFocusTrue);
		m_pendingFocus = PendingFocusNone;
	}		

	if (G_UNLIKELY(Settings::LunaSettings()->perfTesting)) {
        qDebug() << "SYSMGR PERF: APP READY appid: " << (page() ? (page()->appId()) : QString("unknown"))
                 << ", processid: " << (page() ? (page()->processId()) : QString("unknown"))
                 << ", type: " << WebAppFactory::nameForWindowType(m_winType)
                 << ", time: " << Time::curTimeMs();
	}
}

void WindowedWebApp::editorFocusChanged(bool focused, const PalmIME::EditorState& state)
{
	// ignore requests from dashboard's, alert's or anything that isn't a card & windowed
	if (isDashboardApp() || isAlertApp() || (!isCardApp() && !isWindowed())) {
		g_debug("%s: Invalid app type requested editor focus changed (win type %d)",
				__PRETTY_FUNCTION__, windowType());
		return;
	}

	qDebug() << __PRETTY_FUNCTION__ << "(" << (page() ? (page()->appId()) : QString()) << "): focused:" << focused;

        if (focused) {
            QFocusEvent fEvent(QEvent::FocusIn);
            page()->event(&fEvent);
        } else {
            QFocusEvent fEvent(QEvent::FocusOut);
            page()->event(&fEvent);
        }
	
    if (m_channel)
        m_channel->sendAsyncMessage(new ViewHost_EditorFocusChanged(routingId(), focused, state));
}

void WindowedWebApp::autoCapEnabled(bool enabled)
{
	// ignore requests from dashboard's, alert's or anything that isn't a card & windowed
	if (isDashboardApp() || isAlertApp() || (!isCardApp() && !isWindowed())) {
		g_debug("%s: Invalid app type notifying auto cap state (win type %d)", __PRETTY_FUNCTION__, windowType());
		return;
	}

    if (m_channel)
        m_channel->sendAsyncMessage(new ViewHost_AutoCapChanged(routingId(), enabled));
}

void WindowedWebApp::needTouchEvents(bool needTouchEvents)
{
	if (!m_channel)
		return;

    if (isDashboardApp() || isAlertApp() || (!isCardApp() && !isWindowed())) {
        g_debug("%s: Ignoring touch event request for window type %d", __PRETTY_FUNCTION__, windowType());
        return;
    }

	m_channel->sendAsyncMessage(new ViewHost_EnableTouchEvents(routingId(), needTouchEvents));
}

bool WindowedWebApp::showWindowTimeout()
{
	qWarning() << __PRETTY_FUNCTION__ << ": timed out in waiting for stageReady: " << (page() ? (page()->url()) : QUrl());
	if (!m_addedToWindowMgr && m_winType != Window::Type_ChildCard) {
        paint();

	 	m_channel->sendAsyncMessage(new ViewHost_AddWindow(routingId()));
	 	m_addedToWindowMgr=true;
	}

	return false;
}

void WindowedWebApp::recordGesture(float s, float r, int cX, int cY)
{
	m_recordedGestures.push_back(RecordedGestureEntry(s, r, cX, cY));
	if (m_recordedGestures.size() > (size_t) kNumRecordedGestures)
		m_recordedGestures.pop_front();
}

WindowedWebApp::RecordedGestureEntry WindowedWebApp::getAveragedGesture() const
{
	RecordedGestureEntry avg(0, 0, 0, 0);

	if (m_recordedGestures.empty()) {
		avg.scale = 1.0;
		return avg;
	}

	int index = 0;
	int denom = 0;
	int weight = 0;
	for (std::list<RecordedGestureEntry>::const_iterator it = m_recordedGestures.begin();
		 it != m_recordedGestures.end(); ++it, ++index) {

		const RecordedGestureEntry& e = (*it);
		weight = s_recordedGestureAvgWeights[index];

		avg.scale   += e.scale   * weight;
		avg.rotate  += e.rotate  * weight;
		avg.centerX += e.centerX * weight;
		avg.centerY += e.centerY * weight;
		
		denom += weight;
	}

	avg.scale   /= denom;
	avg.rotate  /= denom;
	avg.centerX /= denom;
	avg.centerY /= denom;

	return avg;
}

bool WindowedWebApp::appLoaded() const
{
	return page() && ((page()->progress() >= 100 && !m_stagePreparing) || m_stageReady);
}

void WindowedWebApp::getWindowPropertiesString(WindowProperties &winProp, std::string &propString) const
{
	// Compose json string from the parameters  -------------------------------

	json_object* json = json_object_new_object();
	const int len = 10;
	char tmp[len];

	if (winProp.flags & WindowProperties::isSetBlockScreenTimeout)
		json_object_object_add(json, (char*) "blockScreenTimeout", json_object_new_boolean(winProp.isBlockScreenTimeout));
	
	if (winProp.flags & WindowProperties::isSetSubtleLightbar)
		json_object_object_add(json, (char*) "subtleLightbar", json_object_new_boolean(winProp.isSubtleLightbar));
	
	if (winProp.flags & WindowProperties::isSetFullScreen)
		json_object_object_add(json, (char*) "fullScreen", json_object_new_boolean(winProp.fullScreen));

	if (winProp.flags & WindowProperties::isSetActiveTouchpanel)
		json_object_object_add(json, (char*) "activeTouchpanel", json_object_new_boolean(winProp.activeTouchpanel));

	if (winProp.flags & WindowProperties::isSetAlsDisabled)
		json_object_object_add(json, (char*) "alsDisabled", json_object_new_boolean(winProp.alsDisabled));

	if (winProp.flags & WindowProperties::isSetEnableCompassEvents)
			json_object_object_add(json, (char*) "enableCompass", json_object_new_boolean(winProp.compassEnabled));

	if (winProp.flags & WindowProperties::isSetGyro)
			json_object_object_add(json, (char*) "enableGyro", json_object_new_boolean(winProp.gyroEnabled));

	if (winProp.flags & WindowProperties::isSetOverlayNotifications) {
		if(WindowProperties::OverlayNotificationsLeft == winProp.overlayNotificationsPosition) {
			snprintf(tmp, len-1, "left");
		} else if(WindowProperties::OverlayNotificationsRight == winProp.overlayNotificationsPosition) {
			snprintf(tmp, len-1, "right");
		} else if(WindowProperties::OverlayNotificationsTop == winProp.overlayNotificationsPosition) {
			snprintf(tmp, len-1, "top");
		} else {
			snprintf(tmp, len-1, "bottom");		
		}	
		json_object_object_add(json, (char*) "overlayNotificationsPosition", json_object_new_string (tmp));
	}
	
	if (winProp.flags & WindowProperties::isSetSuppressBannerMessages)
		json_object_object_add(json, (char*) "suppressBannerMessages", json_object_new_boolean(winProp.suppressBannerMessages));
	
	if (winProp.flags & WindowProperties::isSetHasPauseUi)
		json_object_object_add(json, (char*) "hasPauseUi", json_object_new_boolean(winProp.hasPauseUi));
	
	if (winProp.flags & WindowProperties::isSetSuppressGestures)
		json_object_object_add(json, (char*) "suppressGestures", json_object_new_boolean(winProp.suppressGestures));

	if (winProp.flags & WindowProperties::isSetDashboardManualDragMode)
		json_object_object_add(json, (char*) "webosDragMode", json_object_new_boolean(winProp.dashboardManualDrag));

	if (winProp.flags & WindowProperties::isSetStatusBarColor)
		json_object_object_add(json, (char*) "statusBarColor", json_object_new_int(winProp.statusBarColor));

	if (winProp.flags & WindowProperties::isSetRotationLockMaximized)
		json_object_object_add(json, (char*) "rotationLockMaximized", json_object_new_boolean(winProp.rotationLockMaximized));

	if (winProp.flags & WindowProperties::isSetAllowResizeOnPositiveSpaceChange)
		json_object_object_add(json, (char*) "allowResizeOnPositiveSpaceChange",
							   json_object_new_boolean(winProp.allowResizeOnPositiveSpaceChange));

	propString = json_object_to_json_string(json);

	json_object_put(json);

}

void WindowedWebApp::applyLaunchFeedback(int cx, int cy)
{
	WebAppManager::instance()->sendAsyncMessage(new ViewHost_ApplyLaunchFeedback(cx, cy));
}

void WindowedWebApp::setWindowProperties(WindowProperties &winProp)
{
	std::string propString;

	if(!getKey() || !m_channel)
		return;

	m_winProps.merge(winProp);

	getWindowPropertiesString(winProp, propString);
	m_channel->sendAsyncMessage(new ViewHost_SetWindowProperties(routingId(), propString));
}

void WindowedWebApp::keyGesture(QKeyEvent* e)
{
/*
	static const char* fwdText  = "forward";
	static const char* upText   = "up";
	static const char* downText = "down";
	const char* text = 0;

	switch (e->key()) {
	case (Qt::Key_CoreNavi_Menu):
		text = fwdText;
		break;
	case (Qt::Key_CoreNavi_Launcher):
		text = upText;
		break;
	case (Qt::Key_CoreNavi_SwipeDown):
		text = downText;
		break;
	default:
		break;
	}

	if (text) {

		gchar* script = g_strdup_printf("if (window.Mojo && Mojo.handleGesture) { "
										"    Mojo.handleGesture('%s', { timeStamp: %u}) }",
										text, Time::curTimeMs());
		m_page->webkitPage()->evaluateScript((const char*) script);
		g_free(script);
	}
    */
}

bool WindowedWebApp::isTransparent() const
{
	return ((m_winType == Window::Type_Menu) ||
			(m_winType == Window::Type_Dashboard) ||
            (m_winType == Window::Type_PopupAlert));
}
