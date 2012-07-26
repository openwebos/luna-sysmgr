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




#ifndef HOSTWINDOW_H
#define HOSTWINDOW_H

#include "Common.h"

#include "Window.h"
#include "HostWindow.h"
#include "HostWindowData.h"
#include "InputClient.h"
#include <PIpcBuffer.h>
#include <PIpcChannelListener.h>

class PIpcChannel;
class IpcClientHost;

class HostWindow : public Window
				 , public PIpcChannelListener
                 , public InputClient
{
	Q_OBJECT
	
public:

	// for non-IPC windows
	HostWindow(Type type, int width, int height, bool hasAlpha);
	HostWindow(Type type, HostWindowData* data, IpcClientHost* clientHost=0);	
	virtual ~HostWindow();

	int routingId() const;

	bool isIpcWindow() const { return m_isIpcWindow; }
	void channelRemoved();
	void setClientHost(IpcClientHost* clientHost);

	virtual void close();
	
	virtual void setVisibleDimensions(int width, int height) { }
	virtual void resizeEventSync(int w, int h, bool forceSync=false);
	virtual void flipEventSync(bool fromQueue = false);
	virtual void flipEventAsync(QRect windowScreenBounds, bool fromQueue = false);
	virtual void asynchFlipCompleted(int newWidth, int newHeight, int newScreenWidth, int newScreenHeight);
	virtual void queuedFlipCanceled(QRect windowScreenBounds);
	virtual void onMessageReceived(const PIpcMessage& msg);
	virtual void onDisconnected();

    virtual void onUpdateWindowRegion(int x, int y, int w, int h);
    virtual void onUpdateFullWindow();
	virtual void onUpdateWindowRequest();
	void onAsynchFlipCompleted(int newWidth, int newHeight, int newScreenWidth, int newScreenHeight);

	virtual const QPixmap* acquireScreenPixmap();

	virtual void setComposingText(const std::string& text);
	virtual void commitComposingText();

	virtual void commitText(const std::string& text);

	virtual void performEditorAction(PalmIME::FieldAction action);

	virtual void removeInputFocus();

	virtual const HostWindowData* hostWindowData() const { return m_data; }

private Q_SLOTS:

	void slotAboutToSendSyncMessage();

protected:

	virtual void onEditorFocusChanged(bool focus, const PalmIME::EditorState& state);
    void onAutoCapChanged(bool enabled);
	virtual void onEnableTouchEvents(bool) {}

protected:

	HostWindowData* m_data;
	bool m_isIpcWindow;	
	IpcClientHost* m_clientHost;
};

#endif /* HOSTWINDOW_H */
