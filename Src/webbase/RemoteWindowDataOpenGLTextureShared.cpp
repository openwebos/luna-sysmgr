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

#include "RemoteWindowDataOpenGLTextureShared.h"

#include <napp/NWindow.h>
#include <PGContext.h>
#include <PIpcChannel.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "Logging.h"

// -------------------------------------------------------------------------------------

class TextureSharedRenderingWindow : public NWindow
{
public:

	TextureSharedRenderingWindow(RemoteWindowDataOpenGLTextureShared* winData)
		: m_winData(winData) {

		Set(m_winData->width(), m_winData->height());
	}

	~TextureSharedRenderingWindow() {
	}
	
    virtual void Post(int Buffer) {

		//printf("Remote Unlocked Handle: %d\n", GetBuffer(Buffer)->Handle());
		GetBuffer(Buffer)->Unlock();

		/*
		m_winData->m_channel->sendAsyncMessage(new ViewHost_UpdateDoubleBufferedWindow(
												   m_winData->key(),
												   GetBuffer(0)->Handle(),
												   GetBuffer(1)->Handle(),
												   Buffer));
		*/
    }

    virtual void Wait(int* Buffer) {

		//printf("Remote Locked Handle: %d\n", GetBuffer(*Buffer)->Handle());
		GetBuffer(*Buffer)->Lock();
    }

    virtual void Ack() {

		luna_assert(false);
    }

private:

	RemoteWindowDataOpenGLTextureShared* m_winData;
};

// -------------------------------------------------------------------------------------

RemoteWindowDataOpenGLTextureShared::RemoteWindowDataOpenGLTextureShared(int width, int height, bool hasAlpha)
	: m_width(width)
	, m_height(height)
	, m_hasAlpha(hasAlpha)
	, m_context(0)
	, m_win(0)
	, m_directRendering(false)
{
	static int s_index = 1;
	m_key = s_index++;

	if (s_index <= 0) {
		s_index = 1;
	}    
}

RemoteWindowDataOpenGLTextureShared::~RemoteWindowDataOpenGLTextureShared()
{
	delete m_win;

	if (m_context)
		m_context->releaseRef();
}

void RemoteWindowDataOpenGLTextureShared::flip()
{
	int width = m_width;
	m_width = m_height;
	m_height = width;

	if (m_win) {
		delete m_win;
		m_win = new TextureSharedRenderingWindow(this);
	}
}

PGContext* RemoteWindowDataOpenGLTextureShared::renderingContext()
{
	if (m_context) {
		m_context->setWindow(m_directRendering ? 0 : m_win);
		return m_context;
	}
	
	m_context = PGContext::create();
	m_context->setDisplay();

	m_win = new TextureSharedRenderingWindow(this);
	m_context->setWindow(m_directRendering ? 0 : m_win);

	return m_context;
    
}

void RemoteWindowDataOpenGLTextureShared::beginPaint()
{
	luna_assert(m_context);
	m_context->setWindow(m_directRendering ? 0 : m_win);
}

void RemoteWindowDataOpenGLTextureShared::endPaint(bool preserveOnFlip, const PRect& rect, bool flipBuffers)
{
    luna_assert(m_context);
	m_context->flip();
}

void RemoteWindowDataOpenGLTextureShared::sendWindowUpdate(int x, int y, int w, int h)
{
	// NO-OP    
}

bool RemoteWindowDataOpenGLTextureShared::directRenderingAllowed(bool val)
{
	m_directRendering = val;
	return true;
}
	
bool RemoteWindowDataOpenGLTextureShared::needsClear() const
{
	return m_directRendering == false;
}
