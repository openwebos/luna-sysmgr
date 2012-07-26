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

#include "HostWindowDataOpenGLTextureShared.h"

#include <QGLContext>
#include <QPainter>

#include <napp/NRemoteWindow.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <glib.h>
#include "WebAppMgrProxy.h"

#include <PIpcBuffer.h>

#include "CustomEvents.h"
#include "HostBase.h"
#include "Logging.h"
#include "WindowServer.h"

#ifndef GL_BGRA
#define GL_BGRA	0x80E1
#endif

// --------------------------------------------------------------------------------------------------------------

class TextureSharedCompositingWindow : public NRemoteWindow
{
public:
	
	TextureSharedCompositingWindow(int width, int height, unsigned int handle, bool hasAlpha,
								   HostWindowDataOpenGLTextureShared* parent);
	virtual ~TextureSharedCompositingWindow();

	virtual void Ack();
	int Lock(int bufIndex);

	QPixmap m_pixmap;
	GLuint m_textureId;
	bool m_locked;
	bool m_mustWait;
	HostWindowDataOpenGLTextureShared* m_parent;
};

// --------------------------------------------------------------------------------------------------------------

TextureSharedCompositingWindow::TextureSharedCompositingWindow(int width, int height, unsigned int handle, bool hasAlpha,
															   HostWindowDataOpenGLTextureShared* parent)
	: m_textureId(0)
	, m_locked(false)
	, m_mustWait(false)
	, m_parent(parent)
{		
	Set(width, height);

	unsigned int handles[1] = { handle };
	if (!AttachBuffers(1, handles)) {
		g_critical("%s: Failed to attach to buffers", __PRETTY_FUNCTION__);
		return;
	}

	QGLContext* gc = (QGLContext*) QGLContext::currentContext();
	luna_assert(gc);

	int pitch = sizeof(uint32_t) * ((width + 31) & ~31);
	SetBuffer(0, NPixmap::Attach(handle, width, height, pitch));
 
	m_pixmap = QPixmap(width, height);
	m_pixmap.setHasAlpha(true); // Need to set alpha to true for Card Windows to allow blending to occur

	m_textureId = gc->bindTexture(m_pixmap, GL_TEXTURE_2D,
								  GL_BGRA, QGLContext::PremultipliedAlphaBindOption);

	bool ret = textureCreate(0, m_textureId);
	if (!ret)
		g_critical("%s, Failed to create texture\n", __PRETTY_FUNCTION__);
}

TextureSharedCompositingWindow::~TextureSharedCompositingWindow()
{
	QGLContext* gc = (QGLContext*) QGLContext::currentContext();
	luna_assert(gc);

	gc->deleteTexture(m_textureId);
}

int TextureSharedCompositingWindow::Lock(int)
{
	// do not lock if the WebAppMgr process is suspended (potential deadlock)
	if (m_locked)
		return 0;

	if (G_UNLIKELY(m_mustWait)) {

		//printf("*** host waiting: %d\n", GetBuffer(0)->Handle());
		m_parent->lock2();
		m_parent->unlock2();
		m_mustWait = false;
/*
		static int count = 0;
		char fileName[128];
		sprintf(fileName, "/media/internal/images/%08d.png", count++);

		QImage image((unsigned char*) static_cast<NPixmap*>(GetBuffer(0))->Vaddr(),
					 m_pixmap.width(), m_pixmap.height(), QImage::Format_ARGB32_Premultiplied);
		image.save(fileName);
*/

	}

	//printf("*** host locked: %d\n", GetBuffer(0)->Handle());	
	
	GetBuffer(0)->Lock();
	m_locked = true;
	return 0;
}

void TextureSharedCompositingWindow::Ack()
{
	m_mustWait = true;

	if (!m_locked)
		return;

	//printf("*** host unlocked: %d\n", GetBuffer(0)->Handle());
	
	GetBuffer(0)->Unlock();
	m_locked = false;
}

// --------------------------------------------------------------------------------------------------------------

HostWindowDataOpenGLTextureShared::HostWindowDataOpenGLTextureShared(int key, int metaDataKey, int width, int height, bool hasAlpha)
	: HostWindowDataSoftware(key, metaDataKey, width, height, hasAlpha)
	, m_win(0)
	, m_updatedAllowed(true)
	, m_dirty(false)
{
	if (m_ipcBuffer)
		m_win = new TextureSharedCompositingWindow(m_width, m_height, m_ipcBuffer->key(), hasAlpha, this);
}

HostWindowDataOpenGLTextureShared::~HostWindowDataOpenGLTextureShared()
{
	delete m_win;
}

void HostWindowDataOpenGLTextureShared::flip()
{
	int width = m_width;
	m_width = m_height;
	m_height = width;

	if (m_win) {
		delete m_win;
		m_win = new TextureSharedCompositingWindow(m_width, m_height, m_ipcBuffer->key(), m_hasAlpha, this);
	}
}

void HostWindowDataOpenGLTextureShared::initializePixmap(QPixmap& screenPixmap)
{
	// Nullify the screen pixmap
	screenPixmap = QPixmap();
}

QPixmap* HostWindowDataOpenGLTextureShared::acquirePixmap(QPixmap& screenPixmap)
{
	if (!m_win)
		return &screenPixmap;

	m_win->Lock(0);

	return &m_win->m_pixmap;	
}

void HostWindowDataOpenGLTextureShared::onUpdateRegion(QPixmap& screenPixmap, int x, int y, int w, int h)
{
	if (m_win)
		return;
	
	m_win = new TextureSharedCompositingWindow(m_width, m_height, m_ipcBuffer->key(), m_hasAlpha, this);
}

void HostWindowDataOpenGLTextureShared::onUpdateWindowRequest()
{
	if (!m_ipcBuffer)
		return;

	if (!m_win)
		m_win = new TextureSharedCompositingWindow(m_width, m_height, m_ipcBuffer->key(), m_hasAlpha, this);

	if (m_updatedAllowed) {
		glFinish();
		m_win->Ack();
	}
	else
		m_dirty = true;
}

void HostWindowDataOpenGLTextureShared::lock2()
{
	if (G_LIKELY(m_ipcBuffer))
		m_ipcBuffer->lock2();    
}

void HostWindowDataOpenGLTextureShared::unlock2()
{
	if (G_LIKELY(m_ipcBuffer))
	    m_ipcBuffer->unlock2();
}

void HostWindowDataOpenGLTextureShared::updateFromAppDirectRenderingLayer(int screenX, int screenY, int screenOrientation)
{
	QImage appScreenShot = HostBase::instance()->takeAppDirectRenderingScreenShot();
	if (appScreenShot.isNull() || !m_win)
		return;	

	m_win->Lock(0);

/*	
	QImage image((unsigned char*) static_cast<NPixmap*>(m_win->GetBuffer(0))->Vaddr(),
				 m_width, m_height, QImage::Format_ARGB32_Premultiplied);

	glFinish();

	QPainter painter(&image);
	painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);

	switch (screenOrientation) {
    case OrientationEvent::Orientation_Down: {
		painter.translate(m_width/2, m_height/2);
		painter.rotate(180);
		painter.translate(-m_width/2, -m_height/2);
		painter.drawImage(QPoint(screenX, screenY), appScreenShot,
					QRect(0, 0, m_width-screenX, m_height-screenY));
		break;
	}
    case OrientationEvent::Orientation_Left: {
		painter.translate(m_width/2, m_height/2);
		painter.rotate(-90);
		painter.translate(-m_height/2, -m_width/2);
		painter.drawImage(QPoint(screenY, screenX), appScreenShot,
					QRect(0, 0, m_height-screenY, m_width-screenX));
		break;
	}
    case OrientationEvent::Orientation_Right: {
		painter.translate(m_width/2, m_height/2);
		painter.rotate(90);
		painter.translate(-m_height/2, -m_width/2);
		painter.drawImage(QPoint(0, 0), appScreenShot,
					QRect(screenY, screenX, m_height-screenY, m_width-screenX));
		break;
	}

	default: {
		painter.drawImage(QPoint(0, 0), appScreenShot,
					QRect(screenX, screenY, m_width-screenX, m_height-screenY));
		break;
	}
	}

	painter.end();
*/

	// FIXME: The DestinationOver composition mode broke with Qt4.8.
	// Till that is fixed, we will do a slightly more expensive copy based draw

	QImage image((unsigned char*) static_cast<NPixmap*>(m_win->GetBuffer(0))->Vaddr(),
				   m_width, m_height, QImage::Format_ARGB32_Premultiplied);
	QImage fb0Copy = image;
	fb0Copy.detach();

	glFinish();

	QPainter painter(&image);

	switch (screenOrientation) {
    case OrientationEvent::Orientation_Down: {
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.save();
		painter.translate(m_width/2, m_height/2);
		painter.rotate(180);
		painter.translate(-m_width/2, -m_height/2);
		painter.drawImage(QPoint(screenX, screenY), appScreenShot,
					QRect(0, 0, m_width-screenX, m_height-screenY));
		painter.restore();
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(QPoint(0, 0), fb0Copy);
		break;
	}
    case OrientationEvent::Orientation_Left: {
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.save();
		painter.translate(m_width/2, m_height/2);
		painter.rotate(-90);
		painter.translate(-m_height/2, -m_width/2);
		painter.drawImage(QPoint(screenY, screenX), appScreenShot,
					QRect(0, 0, m_height-screenY, m_width-screenX));
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(QPoint(0, 0), fb0Copy);
		break;
	}
    case OrientationEvent::Orientation_Right: {
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.save();
		painter.translate(m_width/2, m_height/2);
		painter.rotate(90);
		painter.translate(-m_height/2, -m_width/2);
		painter.drawImage(QPoint(0, 0), appScreenShot,
					QRect(screenY, screenX, m_height-screenY, m_width-screenX));
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(QPoint(0, 0), fb0Copy);
		break;
	}

	default: {
		painter.setCompositionMode(QPainter::CompositionMode_Source);
		painter.drawImage(QPoint(0, 0), appScreenShot,
					QRect(screenX, screenY, m_width-screenX, m_height-screenY));
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
		painter.drawImage(QPoint(0, 0), fb0Copy);
		break;
	}
	}

	painter.end();
}

void HostWindowDataOpenGLTextureShared::onAboutToSendSyncMessage()
{
	if (!m_ipcBuffer || !m_win)
		return;

	glFinish();
	m_win->Ack();
}

void HostWindowDataOpenGLTextureShared::allowUpdates(bool allow)
{
	if (m_updatedAllowed == allow)
		return;

	m_updatedAllowed = allow;
	if (m_updatedAllowed) {
		if (m_dirty) {
			glFinish();
			m_win->Ack();
			m_dirty = false;
		}
	}
}
