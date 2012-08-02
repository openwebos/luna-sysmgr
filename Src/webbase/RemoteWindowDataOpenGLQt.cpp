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

#include "RemoteWindowDataOpenGLQt.h"

#include <stdint.h>
#include <QtOpenGL>
#include <PIpcBuffer.h>
#include <PIpcChannel.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "WebAppManager.h"
#include "WindowMetaData.h"
#include "Logging.h"

static QGLWidget* sGLWidget = 0;
static QGLFormat fmt;

RemoteWindowDataOpenGLQt::RemoteWindowDataOpenGLQt(int width, int height, bool hasAlpha, bool createIpcBuffer)
    : m_ipcBuffer(0)
    , m_width(width)
    , m_height(height)
    , m_hasAlpha(hasAlpha)
    , m_context(0)
    , m_surface(0)
    , m_directRendering(false)
    , m_displayOpened(false)
    , m_pitch(0)
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

RemoteWindowDataOpenGLQt::~RemoteWindowDataOpenGLQt()
{
    delete m_context;
    delete m_surface;
    delete m_ipcBuffer;
}

void RemoteWindowDataOpenGLQt::setWindowMetaDataBuffer(PIpcBuffer* metaDataBuffer)
{
    m_metaDataBuffer = metaDataBuffer;
}

int RemoteWindowDataOpenGLQt::key() const
{
    return m_ipcBuffer->key();
}

void RemoteWindowDataOpenGLQt::flip()
{
    int width = m_width;
    m_width = m_height;
    m_height = width;
    m_pitch = calcPitch(m_width);

    qDebug() << __PRETTY_FUNCTION__ << "width: " << m_width << "height : " << m_height;

    if (m_surface) {
        delete m_surface;
        m_surface = new QImage(reinterpret_cast<uchar*>(m_ipcBuffer->data()), m_width, m_height, QImage::Format_ARGB32_Premultiplied);

        if (!m_directRendering) {
            if (m_context)
                delete m_context;

            m_context = new QPainter;
        }
    }
}

QPainter* RemoteWindowDataOpenGLQt::qtRenderingContext()
{
    if (m_context)
        return m_context;

    if (!sGLWidget)
    {
        fmt.setAlpha(true);
        fmt.setAlphaBufferSize(8);
        fmt.setRedBufferSize(8);
        fmt.setGreenBufferSize(8);
        fmt.setBlueBufferSize(8);
        fmt.setDirectRendering(true);
        fmt.setDoubleBuffer(true);

        sGLWidget = new QGLWidget(fmt);
        sGLWidget->setAttribute(Qt::WA_PaintOnScreen, true);
        sGLWidget->setAttribute(Qt::WA_OpaquePaintEvent, true);
        sGLWidget->setAttribute(Qt::WA_NoSystemBackground, true);
    }

    sGLWidget->setAutoFillBackground(false);

    m_context = new QPainter;
    m_surface = new QImage(reinterpret_cast<uchar*>(m_ipcBuffer->data()), m_width, m_height, QImage::Format_ARGB32_Premultiplied);

    return m_context;
}

QGLWidget* RemoteWindowDataOpenGLQt::getWidget()
{
    return sGLWidget;
}

void RemoteWindowDataOpenGLQt::beginPaint()
{
    luna_assert(m_context);

    if (m_directRendering)
    {
        sGLWidget->makeCurrent();
        m_context->begin(sGLWidget);
    }
    else
    {
        lock();
        m_context->begin(m_surface);
    }
}

void RemoteWindowDataOpenGLQt::endPaint(bool preserveOnFlip, const QRect& rect, bool flipBuffers)
{
    m_context->end();

    if (m_directRendering)
        sGLWidget->doneCurrent();
    else
        unlock();
}

void RemoteWindowDataOpenGLQt::sendWindowUpdate(int x, int y, int w, int h)
{
    if (m_directRendering)
        return;

    m_channel->sendAsyncMessage(new ViewHost_UpdateWindowRegion(key(), x, y, w, h));
}

bool RemoteWindowDataOpenGLQt::hasDirectRendering() const
{
#if defined(TARGET_DEVICE)
    return true;
#else
    return false;
#endif
}

bool RemoteWindowDataOpenGLQt::directRenderingAllowed(bool val)
{
    m_directRendering = val;
    return true;
}

void RemoteWindowDataOpenGLQt::resize(int newWidth, int newHeight)
{
    qDebug() << __PRETTY_FUNCTION__;

    if (m_directRendering) return;

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
    m_surface = new QImage(reinterpret_cast<uchar*>(m_ipcBuffer->data()), m_width, m_height, QImage::Format_ARGB32_Premultiplied);
}

void RemoteWindowDataOpenGLQt::clear()
{
    lock();

    memset(m_ipcBuffer->data(), 0, m_pitch * m_height);

    unlock();
    bool oldDirectRendering = m_directRendering;
    m_directRendering = false;
    sendWindowUpdate(0, 0, m_width, m_height);
    m_directRendering = oldDirectRendering;
}

void RemoteWindowDataOpenGLQt::lock()
{
    luna_assert(m_ipcBuffer);
    m_ipcBuffer->lock();
}

void RemoteWindowDataOpenGLQt::unlock()
{
    luna_assert(m_ipcBuffer);
    m_ipcBuffer->unlock();
}

int RemoteWindowDataOpenGLQt::calcPitch(int width) {
    return sizeof(uint32_t) * width;
}
