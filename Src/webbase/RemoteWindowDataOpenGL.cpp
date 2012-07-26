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

#include "RemoteWindowDataOpenGL.h"

#include <stdint.h>

#include <PIpcBuffer.h>
#include <PIpcChannel.h>
#include <PGContext.h>
#include <PGSurface.h>
#include <PGThreadGlobalContext.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "WebAppManager.h"
#include "WindowMetaData.h"
#include "Logging.h"

static uint32_t* PrvGetImageBuffer()
{
	// Create a static buffer since GetPixels won't let us copy into an offset
	// for the destination surface.
	static uint32_t* s_imageBuffer = 0;

	if (s_imageBuffer == 0) {
		int width = WebAppManager::instance()->currentUiWidth();
		int height = WebAppManager::instance()->currentUiHeight();
		s_imageBuffer = new uint32_t[width * height];
	}

	return s_imageBuffer;
}

RemoteWindowDataOpenGL::RemoteWindowDataOpenGL(int width, int height, bool hasAlpha, bool createIpcBuffer)
	: m_ipcBuffer(0)
	, m_width(width)
	, m_height(height)
	, m_hasAlpha(hasAlpha)
	, m_context(0)
	, m_surface(0)
	, m_directRendering(false)
	, m_displayOpened(false)
{
	if (createIpcBuffer) {
		const int size = width * height * sizeof(uint32_t);
		m_ipcBuffer = PIpcBuffer::create(size);

		if (m_hasAlpha)
			memset(m_ipcBuffer->data(), 0x00, size);
		else
			memset(m_ipcBuffer->data(), 0xFF, size);
	}
}

RemoteWindowDataOpenGL::~RemoteWindowDataOpenGL()
{
	if (m_context) {
		m_surface->releaseRef();
		m_context->releaseRef();
	}		

	delete m_ipcBuffer;
}

void RemoteWindowDataOpenGL::setWindowMetaDataBuffer(PIpcBuffer* metaDataBuffer)
{
	m_metaDataBuffer = metaDataBuffer;    
}

int RemoteWindowDataOpenGL::key() const
{
	return m_ipcBuffer->key();    
}

void RemoteWindowDataOpenGL::flip()
{
	int width = m_width;
	m_width = m_height;
	m_height = width;

	if (m_surface) {
		m_surface->releaseRef();
		m_surface = m_context->createSurface(m_width, m_height, m_hasAlpha);
		if (!m_directRendering) {
			m_context->setSurface(m_surface);
		}
	}
}

PGContext* RemoteWindowDataOpenGL::renderingContext()
{
	if (m_context)
		return m_context;
	
	static bool s_initialized = false;
	if (!s_initialized) {
		s_initialized = true;
		PGThreadGlobalContext::instance()->graphicsContext()->setDisplay();
	}
	
	m_context = PGContext::create();
	if (!m_displayOpened) {
		m_context->setDisplay(PPrimary, 1, PFORMAT_8888, 3);
		m_displayOpened = true;
	}

	m_surface = m_context->createSurface(m_width, m_height, m_hasAlpha);
	
	if (m_directRendering) {
		m_context->setSurface(0);
	} else {
		m_context->setSurface(m_surface);
	}

	return m_context;
}

void RemoteWindowDataOpenGL::beginPaint()
{
	luna_assert(m_context && m_surface);
	
	if (m_directRendering) {
		if (!m_displayOpened) {
			m_context->setDisplay(PPrimary, 1, PFORMAT_8888, 3);
			m_displayOpened = true;
		}
		else
			m_context->setSurface(0);
	}
	else {
		m_context->setSurface(m_surface);
	}
}

void RemoteWindowDataOpenGL::endPaint(bool preserveOnFlip, const PRect& rect, bool flipBuffers)
{
	if (m_directRendering) {
		if (flipBuffers) {
			m_context->pgContext()->Flip(false);
		}
	}		
}

void RemoteWindowDataOpenGL::sendWindowUpdate(int x, int y, int w, int h)
{
	if (m_directRendering)
		return;
	
	luna_assert(m_channel);
	luna_assert(m_context);

	
	// GetPixels doesn't work for subrects so copy the whole surface
	//m_displayContext->pgContext()->GetPixels(PRect(x, y, x + w, y + h), imageBuffer);
	m_context->setSurface(m_surface);

	uint32_t* imageBuffer = PrvGetImageBuffer();
	m_context->pgContext()->GetPixels(PRect(0, 0, m_width, m_height), imageBuffer);

	// Copy the scanlines from the static buffer to the ipcBuffer
	m_ipcBuffer->lock();

	for (int i = 0; i < h; i++)
		memcpy((char *)m_ipcBuffer->data() + (m_width * (y + i) + x) * 4,
			   (char*) imageBuffer + (m_width * (y + i) + x) * 4,
			   w * 4);

	m_ipcBuffer->unlock();

	m_channel->sendAsyncMessage(new ViewHost_UpdateWindowRegion(key(), x, y, w, h));
}

bool RemoteWindowDataOpenGL::hasDirectRendering() const
{
#if defined(DIRECT_RENDERING)
	return true;
#endif
	return false;	
}

bool RemoteWindowDataOpenGL::directRenderingAllowed(bool val)
{
#if defined(TARGET_DEVICE)
	m_directRendering = val;

    if (m_context) {
        if (m_directRendering) {
			if (!m_displayOpened) {
				m_context->setDisplay(PPrimary, 1, PFORMAT_8888, 3);
				m_displayOpened = true;
			} else {
				m_context->setSurface(0);
			}
        } else {
            m_context->setSurface(m_surface);
        }
    }

	return true;
#else
	return false;
#endif    
}

void RemoteWindowDataOpenGL::resize(int newWidth, int newHeight)
{
    
}
