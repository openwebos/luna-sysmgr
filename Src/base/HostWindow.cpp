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




#include "Common.h"

#include "HostWindow.h"

#include <PIpcChannel.h>
#include "HostWindowData.h"
#include "IpcClientHost.h"
#include "SystemUiController.h"
#include "WindowServer.h"
#include "IMEController.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

HostWindow::HostWindow(Type type, int width, int height, bool hasAlpha)
		: Window(type, width, height, hasAlpha)
		, m_data(0)
		, m_isIpcWindow(false)
		, m_clientHost(0)
{
}

HostWindow::HostWindow(Type type, HostWindowData* data, IpcClientHost* clientHost)
	: Window(type, data->width(), data->height(), data->hasAlpha())
	, m_data(data)
	, m_isIpcWindow(false)
	, m_clientHost(clientHost)
{
    if (m_clientHost) {
		m_isIpcWindow = true;
		m_channel = clientHost->channel();
	}

	data->initializePixmap(m_screenPixmap);

	connect(SystemUiController::instance(), SIGNAL(signalAboutToSendSyncMessage()),
			SLOT(slotAboutToSendSyncMessage()));
}

HostWindow::~HostWindow()
{
	if(m_clientHost)
		m_clientHost->windowDeleted(this);

	delete m_data;
}

int HostWindow::routingId() const
{
	if (!m_data)
		return 0;
	
	return m_data->key();
}

void HostWindow::channelRemoved()
{
	m_channel = 0;    
}

void HostWindow::setClientHost(IpcClientHost* clientHost)
{
	m_clientHost = clientHost;    
}

void HostWindow::close()
{
	if (m_clientHost)
		m_clientHost->closeWindow(this);
}


void HostWindow::onMessageReceived(const PIpcMessage& msg)
{
	bool msgIsOk;
	
	IPC_BEGIN_MESSAGE_MAP(HostWindow, msg, msgIsOk)
		IPC_MESSAGE_HANDLER(ViewHost_UpdateWindowRegion, onUpdateWindowRegion)
		IPC_MESSAGE_HANDLER(ViewHost_UpdateFullWindow, onUpdateFullWindow)
		IPC_MESSAGE_HANDLER(ViewHost_UpdateWindowRequest, onUpdateWindowRequest)
		IPC_MESSAGE_HANDLER(ViewHost_SetVisibleDimensions, setVisibleDimensions)
		IPC_MESSAGE_HANDLER(ViewHost_SetAppId, setAppId)
		IPC_MESSAGE_HANDLER(ViewHost_SetProcessId, setProcessId)
		IPC_MESSAGE_HANDLER(ViewHost_SetLaunchingAppId, setLaunchingAppId)
		IPC_MESSAGE_HANDLER(ViewHost_SetLaunchingProcessId, setLaunchingProcessId)
		IPC_MESSAGE_HANDLER(ViewHost_SetName, setName)
		IPC_MESSAGE_HANDLER(ViewHost_EditorFocusChanged, onEditorFocusChanged)
		IPC_MESSAGE_HANDLER(ViewHost_EnableTouchEvents, onEnableTouchEvents)
        IPC_MESSAGE_HANDLER(ViewHost_AutoCapChanged, onAutoCapChanged)
		IPC_MESSAGE_HANDLER(ViewHost_AsyncFlipCompleted, onAsynchFlipCompleted)
	IPC_END_MESSAGE_MAP()
}

void HostWindow::onDisconnected()
{
	// NO OP
}

void HostWindow::onUpdateWindowRegion(int x, int y, int w, int h)
{
	if (m_data)
		m_data->onUpdateRegion(m_screenPixmap, x, y, w, h);

	// translate update from top/left to Window's item coordinate system
	QRectF bounds = boundingRect();
	update(x + bounds.x(), y + bounds.y(), w, h);
}

void HostWindow::onUpdateFullWindow()
{
	QRectF r = boundingRect();
	onUpdateWindowRegion(0, 0, r.width(), r.height());
}

void HostWindow::onUpdateWindowRequest()
{
	if (m_data)
		m_data->onUpdateWindowRequest();    
}

void HostWindow::onAsynchFlipCompleted(int newWidth, int newHeight, int newScreenWidth, int newScreenHeight)
{
	asynchFlipCompleted(newWidth, newHeight, newScreenWidth, newScreenHeight);
}

void HostWindow::resizeEventSync(int w, int h, bool forceSync)
{
	bool visible = isVisible() && this->sceneBoundingRect().intersects(WindowServer::instance()->sceneRect());

	if (!m_channel || !m_data)
		return;

	if((w == (int)m_bufWidth) && (h == (int)m_bufHeight)) {
		// already the right size, so do nothing
		return;
	}

	if((w == (int)m_bufHeight) && (h == (int)m_bufWidth)) {
		// no need to resize the buffer in this case, just flip it
		if(forceSync || visible) {
			flipEventSync();
		} else {
			// queue up this card for resizing (flip event) after the rotation animation is complete
			QRect screenRect(0, 0, w, h);
			WindowServer::instance()->enqueueWindowForFlip(this, screenRect, false);
			// adjust the bounding rect (visible bounds) immediately, so the WM can position the card correctly
			setVisibleDimensions(w, h);
		}
	} else {
		// buffer resize required
		int newKey = -1;
		SystemUiController::instance()->aboutToSendSyncMessage();
		m_channel->sendSyncMessage(new View_SyncResize(routingId(), w, h, true, &newKey), -1);
		if (newKey < 0)
			return;
	
		bool hasAlpha = m_data->hasAlpha();
		int oldKey = m_data->key();
		int metaDataKey = m_data->metaDataBuffer()->key();
		delete m_data;
		m_data = HostWindowDataFactory::generate(newKey, metaDataKey, w, h, hasAlpha);
		if (!m_data || !m_data->isValid()) {
			g_critical("%s (%d): Failed to generate HostWindowData for key: %d\n",
					   __PRETTY_FUNCTION__, __LINE__, newKey);
			delete m_data;
			m_data = 0;
			return;
		}

		// calls prepareGeometryChange
		resize(w, h);
		m_clientHost->replaceWindowKey(this, oldKey, newKey);

		m_data->initializePixmap(m_screenPixmap);

		onUpdateFullWindow();
	}
}

void HostWindow::queuedFlipCanceled(QRect windowScreenBounds)
{
	// this window was on a queue to be resized (flip) after the rotation animation, but the request
	// was dropped due to a new rotation start, so we must put things back to normal before that

	// this was a flip (w=h, h=w), so restore the visible dimensions to what they were based on the data from the flip request
	setVisibleDimensions(windowScreenBounds.height(), windowScreenBounds.width());
}

void HostWindow::flipEventSync(bool fromQueue)
{
	if (!m_channel || !m_data)
		return;

	int newWidth = m_bufHeight, newHeight = m_bufWidth;

	SystemUiController::instance()->aboutToSendSyncMessage();

	m_channel->sendSyncMessage(new View_Flip(routingId(), newWidth, newHeight), -1);

	m_data->flip();

	// calls prepareGeometryChange
	resize(newWidth, newHeight);

	m_data->initializePixmap(m_screenPixmap);

	if (m_screenPixmap.isNull()) {
		bool hasAlpha = m_screenPixmap.hasAlpha();
		m_screenPixmap = QPixmap(m_data->width(), m_data->height());
		QColor fillColor(255,255,255,hasAlpha?0:255);
		m_screenPixmap.fill(fillColor);
	}

	onUpdateFullWindow();
}

void HostWindow::flipEventAsync(QRect windowScreenBounds, bool fromQueue)
{
	if (!m_channel || !m_data)
		return;

	int newWidth = m_bufHeight;
	int newHeight = m_bufWidth;

	m_channel->sendAsyncMessage(new View_AsyncFlip(routingId(), newWidth, newHeight, windowScreenBounds.width(), windowScreenBounds.height()));

	m_bufWidth = newWidth;
	m_bufHeight = newHeight;
}

void HostWindow::asynchFlipCompleted(int newWidth, int newHeight, int newScreenWidth, int newScreenHeight)
{
	m_data->flip();

	// calls prepareGeometryChange
	resize(newWidth, newHeight);

	m_data->initializePixmap(m_screenPixmap);

	if (m_screenPixmap.isNull()) {
		bool hasAlpha = m_screenPixmap.hasAlpha();
		m_screenPixmap = QPixmap(m_data->width(), m_data->height());
		QColor fillColor(255,255,255,hasAlpha?0:255);
		m_screenPixmap.fill(fillColor);
	}

	onUpdateFullWindow();
}



const QPixmap* HostWindow::acquireScreenPixmap()
{
	return m_data ? m_data->acquirePixmap(m_screenPixmap) : Window::acquireScreenPixmap();
}

void HostWindow::slotAboutToSendSyncMessage()
{
	if (m_data)
		m_data->onAboutToSendSyncMessage();
}

void HostWindow::onEditorFocusChanged(bool focused, const PalmIME::EditorState& state)
{
	g_debug("IME: Window got focus change in sysmgr %s focused: %d, fieldtype: %d, fieldactions: 0x%02x", 
			 appId().c_str(), focused, state.type, state.actions);
	// cache input focus state for this window
	setInputFocus(focused);
	setInputState(state);

	IMEController::instance()->notifyInputFocusChange(this, focused);
}

void HostWindow::onAutoCapChanged(bool enabled)
{
//	g_debug("IME: Window got autocap change in sysmgr %s enabled: %d", appId().c_str(), enabled);

    IMEController::instance()->notifyAutoCapChanged(this, enabled);
}

void HostWindow::setComposingText(const std::string& text)
{
	if (!inputFocus())
		return;

	if (m_channel)
		m_channel->sendAsyncMessage(new View_SetComposingText(routingId(), text));
}

void HostWindow::commitComposingText()
{
	if (m_channel)
		m_channel->sendAsyncMessage(new View_CommitComposingText(routingId()));
}

void HostWindow::commitText(const std::string& text)
{
	if (!inputFocus())
			return;

	if (m_channel)
		m_channel->sendAsyncMessage(new View_CommitText(routingId(), text));
}

void HostWindow::performEditorAction(PalmIME::FieldAction action)
{
	if (!inputFocus())
		return;

	if (!(inputState().actions & action)) {

		g_warning("%s: failed to send action not accepted by the current editor (requested: 0x%02x, supported: 0x%02x)", 
				  __PRETTY_FUNCTION__, action, inputState().actions);
		return;
	}

	if (m_channel)
		m_channel->sendAsyncMessage(new View_PerformEditorAction(routingId(), static_cast<int>(action)));
}

void HostWindow::removeInputFocus()
{
	if (!inputFocus())
		return;

	if (m_channel)
		m_channel->sendAsyncMessage(new View_RemoveInputFocus(routingId()));
}

