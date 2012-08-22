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

#include "HostWindowDataOpenGL.h"

#include <QGLContext>
#include <QPainter>

#if defined(TARGET_DESKTOP)
#include <GL/gl.h>
#elif defined(TARGET_DEVICE)
#include <GLES2/gl2.h>
#endif

#include "HostBase.h"
#include "Logging.h"
#include "WebAppMgrProxy.h"

#ifndef GL_BGRA
#define GL_BGRA	0x80E1
#endif

#if defined(TARGET_DESKTOP)
static const int kGLInternalFormat = GL_RGBA;
static const int kGLExternalFormat = GL_BGRA;
#else
static const int kGLInternalFormat = GL_BGRA;
static const int kGLExternalFormat = GL_BGRA;
#endif

HostWindowDataOpenGL::HostWindowDataOpenGL(int key, int metaDataKey, int width, int height, bool hasAlpha)
	: HostWindowDataSoftware(key, metaDataKey, width, height, hasAlpha)
	, m_textureId(0)
{
}

HostWindowDataOpenGL::~HostWindowDataOpenGL()
{
	QGLContext* gc = (QGLContext*) QGLContext::currentContext();
	if (gc) {
		if (m_textureId)
			gc->deleteTexture(m_textureId);
	}
}

void HostWindowDataOpenGL::initializePixmap(QPixmap& screenPixmap)
{
	QGLContext* gc = (QGLContext*) QGLContext::currentContext();
	if (gc) {
		if (m_textureId)
			gc->deleteTexture(m_textureId);

		screenPixmap = QPixmap(m_width, m_height);
        screenPixmap.fill(QColor(255,255,255,0));
		m_textureId = gc->bindTexture(screenPixmap, GL_TEXTURE_2D, kGLInternalFormat,
									  QGLContext::PremultipliedAlphaBindOption);
	}
	m_dirty = true;
}

void HostWindowDataOpenGL::flip()
{
	HostWindowDataSoftware::flip();

	m_dirty = true;
}

void HostWindowDataOpenGL::onUpdateRegion(QPixmap& screenPixmap, int x, int y, int w, int h)
{
	m_dirty = true;
}

QPixmap* HostWindowDataOpenGL::acquirePixmap(QPixmap& screenPixmap)
{
	if (m_dirty) {
		m_dirty = false;
    
		m_ipcBuffer->lock();

		QGLContext* gc = (QGLContext*) QGLContext::currentContext();
		if (gc) {
            QImage data((uchar*) m_ipcBuffer->data(), m_width, m_height, QImage::Format_ARGB32_Premultiplied);
            screenPixmap = QPixmap::fromImage(data);
            // FIXME: is it really necessary to bind it here? Qt will bind it through
            // QGL2PaintEngineEx::drawPixmap -> QGLContextPrivate::bindTexture anyways
            m_textureId = gc->bindTexture(screenPixmap, GL_TEXTURE_2D, kGLInternalFormat,
                                          QGLContext::PremultipliedAlphaBindOption);
		}

		m_ipcBuffer->unlock();
	}

	return &screenPixmap;
}

void HostWindowDataOpenGL::updateFromAppDirectRenderingLayer(int screenX, int screenY,
															 int screenOrientation)
{
	QImage appScreenShot = HostBase::instance()->takeAppDirectRenderingScreenShot();
	if (appScreenShot.isNull())
		return;

	m_ipcBuffer->lock();

	QImage image((const unsigned char*) m_ipcBuffer->data(),
				 m_width, m_height, QImage::Format_ARGB32_Premultiplied);

	QPainter painter(&image);
	painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
	painter.drawImage(QPoint(0, 0), appScreenShot,
					  QRect(screenX, screenY, m_width, m_height));

	m_ipcBuffer->unlock();

	m_dirty = true;
}
