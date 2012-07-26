/**
 *  Copyright (c) 2012 Hewlett-Packard Development Company, L.P.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#include "Common.h"

#include "RemoteWindowDataSoftwareQt.h"
#include "WebAppManager.h"

#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <PIpcBuffer.h>
#include <PIpcChannel.h>
#include <QPainter>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "WindowMetaData.h"
#include "Logging.h"
#include "Settings.h"

RemoteWindowDataSoftwareQt::RemoteWindowDataSoftwareQt(int width, int height, bool hasAlpha, bool createIpcBuffer)
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

RemoteWindowDataSoftwareQt::~RemoteWindowDataSoftwareQt()
{
    delete m_context;
    delete m_surface;
	delete m_ipcBuffer;
}

void RemoteWindowDataSoftwareQt::setWindowMetaDataBuffer(PIpcBuffer* metaDataBuffer)
{
	m_metaDataBuffer = metaDataBuffer;
}

int RemoteWindowDataSoftwareQt::key() const
{
	return m_ipcBuffer->key();
}

void RemoteWindowDataSoftwareQt::flip()
{
	int width = m_width;
	m_width = m_height;
	m_height = width;
	m_pitch = calcPitch(m_width);

	if (m_surface) {
        delete m_surface;
		m_surface = new QImage(reinterpret_cast<uchar*>(data()), m_width, m_height, QImage::Format_ARGB32_Premultiplied);
		if (!m_directRendering) {
            if (m_context)
                delete m_context;
            m_context = new QPainter;
		}
	}
}

QPainter* RemoteWindowDataSoftwareQt::qtRenderingContext()
{
	if (m_context) {
		return m_context;
	}

	m_surface = new QImage(reinterpret_cast<uchar*>(data()), m_width, m_height, QImage::Format_ARGB32_Premultiplied);
	m_context = new QPainter;

	return m_context;
}

void RemoteWindowDataSoftwareQt::beginPaint()
{
	luna_assert(m_context && m_surface);

	if (m_directRendering) {
	}
	else {
		lock();
		m_context->begin(m_surface);
	}
}

void RemoteWindowDataSoftwareQt::endPaint(bool preserveOnFlip, const QRect& rect, bool flipBuffers)
{
	if (m_directRendering) {
	} else {
        m_context->end();
		unlock();
	}
}

void RemoteWindowDataSoftwareQt::sendWindowUpdate(int x, int y, int w, int h)
{
	luna_assert(m_channel);
	if (!m_directRendering) {
		m_channel->sendAsyncMessage(new ViewHost_UpdateWindowRegion(key(), x, y, w, h));
	}
}

bool RemoteWindowDataSoftwareQt::hasDirectRendering() const
{
	return false;
}

bool RemoteWindowDataSoftwareQt::directRenderingAllowed(bool val)
{
	return false;
}

void RemoteWindowDataSoftwareQt::lock()
{
	luna_assert(m_ipcBuffer);
	m_ipcBuffer->lock();
}

void RemoteWindowDataSoftwareQt::unlock()
{
	luna_assert(m_ipcBuffer);
	m_ipcBuffer->unlock();
}

void* RemoteWindowDataSoftwareQt::data()
{
	luna_assert(m_ipcBuffer);
	return m_ipcBuffer->data();
}

void RemoteWindowDataSoftwareQt::resize(int newWidth, int newHeight)
{
	if (m_width == newWidth && m_height == newHeight)
		return;

	luna_assert(m_ipcBuffer);

	if (m_context) {
        delete m_context;
        delete m_surface;
        m_context = 0;
        m_surface = 0;
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

	m_context = new QPainter;
	m_surface = new QImage(reinterpret_cast<uchar*>(data()), m_width, m_height, QImage::Format_ARGB32_Premultiplied);
}

int RemoteWindowDataSoftwareQt::calcPitch(int width) {
	return sizeof(uint32_t) * width;
}

void RemoteWindowDataSoftwareQt::clear() {
	lock();
	memset(data(), 0, m_pitch * m_height);
	unlock();
	bool oldDirectRendering = m_directRendering;
	m_directRendering = false;
	sendWindowUpdate(0, 0, m_width, m_height);
	m_directRendering = oldDirectRendering;
}
