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

#include "RemoteWindowDataSoftware.h"
#include "WebAppManager.h"

#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <PIpcBuffer.h>
#include <PIpcChannel.h>
#include <PGContext.h>
#include <PGSurface.h>
#include <PGThreadGlobalContext.h>
#include <WebGLES2Context.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "WindowMetaData.h"
#include "Logging.h"
#include "Settings.h"

RemoteWindowDataSoftware::RemoteWindowDataSoftware(int width, int height, bool hasAlpha, bool createIpcBuffer)
	: m_ipcBuffer(0)
	, m_width(width)
	, m_height(height)
	, m_pitch(0)
	, m_hasAlpha(hasAlpha)
	, m_context(0)
	, m_surface(0)
	, m_directRendering(false)
	, m_displayOpened(false)
{
	m_pitch = calcPitch(m_width);
    
	if (createIpcBuffer) {
		const int size = m_pitch * m_height;
		m_ipcBuffer = PIpcBuffer::create(size);

		if (m_hasAlpha)
			memset(m_ipcBuffer->data(), 0x00, size);
		else
			memset(m_ipcBuffer->data(), 0xFF, size);
	}
}

RemoteWindowDataSoftware::~RemoteWindowDataSoftware()
{
	if (m_context) {
		m_surface->releaseRef();
		m_context->releaseRef();
	}

	delete m_ipcBuffer;
}

void RemoteWindowDataSoftware::setWindowMetaDataBuffer(PIpcBuffer* metaDataBuffer)
{
	m_metaDataBuffer = metaDataBuffer;
}

int RemoteWindowDataSoftware::key() const
{
	return m_ipcBuffer->key();
}

void RemoteWindowDataSoftware::flip()
{
	int width = m_width;
	m_width = m_height;
	m_height = width;
	m_pitch = calcPitch(m_width);

	if (m_surface) {
		m_surface->releaseRef();
		m_surface = m_context->wrapBitmapData(m_width, m_height, true, data(), m_pitch);
		if (!m_directRendering) {
			m_context->setSurface(m_surface);
		}
	}
}

PGContext* RemoteWindowDataSoftware::renderingContext()
{
	if (m_context) {
		return m_context;
	}

	m_context = PGContext::create();
	m_surface = m_context->wrapBitmapData(m_width, m_height, true, data());

	if (m_directRendering) {
		if (!m_displayOpened) {
			m_context->setDisplay(PPrimary, 1, PFORMAT_8888, 3);
			m_displayOpened = true;
		}
		else
			m_context->setSurface(0);
	} else {
		m_context->setSurface(m_surface);
	}

	return m_context;
}

void RemoteWindowDataSoftware::beginPaint()
{
	luna_assert(m_context && m_surface);

	if (m_directRendering) {
	}
	else {
		lock();

		m_context->setSurface(m_surface);
	}
}

void RemoteWindowDataSoftware::endPaint(bool preserveOnFlip, const PRect& rect, bool flipBuffers)
{
	if (m_directRendering) {
		if (flipBuffers) {
			if (preserveOnFlip) {
				m_context->pgContext()->Flip(true, &rect);
			} else {
				m_context->pgContext()->Flip(false);
			}
		}
	} else {
		unlock();
	}
}

void RemoteWindowDataSoftware::sendWindowUpdate(int x, int y, int w, int h)
{
	luna_assert(m_channel);
	if (!m_directRendering) {
		m_channel->sendAsyncMessage(new ViewHost_UpdateWindowRegion(key(), x, y, w, h));
	}
}

bool RemoteWindowDataSoftware::hasDirectRendering() const
{
#if defined(DIRECT_RENDERING)
	return true;
#endif
	return false;
}

bool RemoteWindowDataSoftware::directRenderingAllowed(bool val)
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

void RemoteWindowDataSoftware::lock()
{
	luna_assert(m_ipcBuffer);
	m_ipcBuffer->lock();
}

void RemoteWindowDataSoftware::unlock()
{
	luna_assert(m_ipcBuffer);
	m_ipcBuffer->unlock();
}

void* RemoteWindowDataSoftware::data()
{
	luna_assert(m_ipcBuffer);
	return m_ipcBuffer->data();
}

void RemoteWindowDataSoftware::resize(int newWidth, int newHeight)
{
	if (m_width == newWidth && m_height == newHeight)
		return;

	luna_assert(m_ipcBuffer);

	if (m_context) {
		m_surface->releaseRef();
		m_context->releaseRef();
		m_surface = 0;
		m_context = 0;
	}

	delete m_ipcBuffer;
	m_ipcBuffer = 0;
	m_displayOpened = false;

	const int size = newWidth * newHeight * sizeof(uint32_t);
	m_ipcBuffer = PIpcBuffer::create(size);
	m_width = newWidth;
	m_height = newHeight;

	luna_assert(m_ipcBuffer);

	if (m_hasAlpha)
		memset(m_ipcBuffer->data(), 0x00, size);
	else
		memset(m_ipcBuffer->data(), 0xFF, size);

	m_context = PGContext::create();
	m_surface = m_context->wrapBitmapData(m_width, m_height, true,
										  data());

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

int RemoteWindowDataSoftware::calcPitch(int width) {
	return sizeof(uint32_t) * width;
}

void RemoteWindowDataSoftware::clear() {
	lock();
	memset(data(), 0, m_pitch * m_height);
	unlock();
	bool oldDirectRendering = m_directRendering;
	m_directRendering = false;
	sendWindowUpdate(0, 0, m_width, m_height);
	m_directRendering = oldDirectRendering;
}
