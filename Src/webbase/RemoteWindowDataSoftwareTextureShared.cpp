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

#include <PIpcChannel.h>
#include <napp/NWindowBuffer.h>
#include <GLES2/gl2.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "RemoteWindowDataSoftwareTextureShared.h"
#include "Logging.h"
#include "SharedGlobalProperties.h"
#include "WebAppManager.h"

RemoteWindowDataSoftwareTextureShared::RemoteWindowDataSoftwareTextureShared(int width, int height, bool hasAlpha)
	: RemoteWindowDataSoftwareOpenGLComposited(width, height, hasAlpha, false)
	, m_mustPost(false)
	, m_lockedBuffer(false)
{
	m_pitch = calcPitch(width);
	size_t size = m_pitch * m_height;
	
	m_sharedWinBuffer = NSharedWindowBuffer::Create(size);
	m_ipcBuffer = PIpcBuffer::attach(m_sharedWinBuffer->Handle());

	if (m_hasAlpha)
		memset(m_sharedWinBuffer->Vaddr(), 0x00, size);
	else
		memset(m_sharedWinBuffer->Vaddr(), 0xFF, size);
}

RemoteWindowDataSoftwareTextureShared::~RemoteWindowDataSoftwareTextureShared()
{
	//TODO: change this back to Free once libnapp is updated
	//m_sharedWinBuffer->Free();
	m_sharedWinBuffer->FreeNoDelete();
	delete m_sharedWinBuffer;

	delete m_ipcBuffer;
	m_ipcBuffer = 0;
}

Palm::WebGLES2Context* RemoteWindowDataSoftwareTextureShared::getGLES2Context()
{
	if (!m_glContext) {
		m_glContext = new PGLES2ContextTextureShared(this);
	}
	return m_glContext;
}

int RemoteWindowDataSoftwareTextureShared::key() const
{
	return m_sharedWinBuffer->Handle();
}

void RemoteWindowDataSoftwareTextureShared::lock()
{
	if (supportsDirectRendering()) {

		// Need to lock only if:
		//    * direct rendering is not active
		// or * direct rendering is active and this is the direct rendering window

		SharedGlobalProperties* globalProps = WebAppManager::globalProperties();
		if (G_LIKELY(globalProps)) {
			int currentActiveKey = g_atomic_int_get(&globalProps->directRenderingWindowKey);
			if (currentActiveKey) {
				if (currentActiveKey != key()) {
					return;
				}
			}
		}
	}

	if (!m_sharedWinBuffer->TryLock()) {

		//printf( "*** webappmgr requesting buffer %d\n", m_sharedWinBuffer->Handle());

		m_mustPost = true;
		m_ipcBuffer->lock2();

		m_channel->sendAsyncMessage(new ViewHost_UpdateWindowRequest(key()));
		m_sharedWinBuffer->Lock();
	}

	m_lockedBuffer = true;

	//printf( "*** webappgr got lock %d\n",  m_sharedWinBuffer->Handle());
}

void RemoteWindowDataSoftwareTextureShared::unlock()
{
	if (!m_lockedBuffer)
		return;

	m_lockedBuffer = false;

	//printf( "*** webappgr unlock %d\n",  m_sharedWinBuffer->Handle());
    m_sharedWinBuffer->Unlock();
	if (m_mustPost) {
		//printf( "*** webappgr releasing synchronization lock %d\n",  m_sharedWinBuffer->Handle());
		m_ipcBuffer->unlock2();
		m_mustPost = false;
	}
}

void* RemoteWindowDataSoftwareTextureShared::data()
{
    return m_sharedWinBuffer->Vaddr();
}

void RemoteWindowDataSoftwareTextureShared::resize(int newWidth, int newHeight)
{
	if (m_width == newWidth && m_height == newHeight)
		return;

	if (m_context) {
		m_surface->releaseRef();
		m_context->releaseRef();
		m_surface = 0;
		m_context = 0;
	}
	
	m_sharedWinBuffer->Free();
	delete m_ipcBuffer;
	m_sharedWinBuffer = 0;
	m_ipcBuffer = 0;
	m_displayOpened = false;

	m_pitch = calcPitch(newWidth);
	const int size = m_pitch * newHeight;    

	m_sharedWinBuffer = NSharedWindowBuffer::Create(size);
	m_ipcBuffer = PIpcBuffer::attach(m_sharedWinBuffer->Handle());
	m_width = newWidth;
	m_height = newHeight;

	luna_assert(m_ipcBuffer);

	if (m_hasAlpha)
		memset(m_sharedWinBuffer->Vaddr(), 0x00, size);
	else
		memset(m_sharedWinBuffer->Vaddr(), 0xFF, size);

	m_context = PGContext::create();
	m_surface = m_context->wrapBitmapData(m_width, m_height, true, data(), m_pitch);
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


int RemoteWindowDataSoftwareTextureShared::calcPitch(int width) {
	// Stride has to be a multiple of 32 texels	   
	return sizeof(uint32_t) * ((width + 31) & ~31);
}

void RemoteWindowDataSoftwareTextureShared::flip() {
	RemoteWindowDataSoftware::flip();
	if (m_glContext)
		m_glContext->resizeWindow(m_width, m_height);
}

// -------------------------------------------------------------------------------------


PGLES2ContextTextureShared::PGLES2ContextTextureShared(RemoteWindowDataSoftwareTextureShared* data) :
		PGLES2Context(data) {
}

PGLES2ContextTextureShared::~PGLES2ContextTextureShared() {
}

void PGLES2ContextTextureShared::createWindow(int width, int height) {
	if (!m_nappWindow) {
		m_nappWindow = new NAppWindow();
		m_nappWindow->Set(width, height);

		unsigned int handles[1] = { m_data->key() };
		if (!m_nappWindow->AttachBuffers(1, handles)) {
			g_critical("%s: Failed to attach to buffers", __PRETTY_FUNCTION__);
			return;
		}

		m_nappWindow->SetBuffer(0, static_cast<RemoteWindowDataSoftwareTextureShared*>(m_data)->getSharedWindowBuffer());
	}
}

bool PGLES2ContextTextureShared::swapBuffers() {
	if (m_data->isDirectRendering()) {
		context()->Flip();
	} else {
		glFinish();
	}
	return true;
}

bool PGLES2ContextTextureShared::renderOffscreen() {
	g_debug("%s called.", __PRETTY_FUNCTION__);

	PGLESContext2D* ctxt = context();
	luna_assert(ctxt);

	createWindow(m_data->width(), m_data->height());
	setToCurrent();
	if (!ctxt->RenderToWindow(m_nappWindow)) {
		g_error("RenderToWindow(m_nappWindow) failed in %s", __PRETTY_FUNCTION__);
		return false;
	}

	return true;
}

bool PGLES2ContextTextureShared::renderOnScreen() {
	g_debug("%s called", __PRETTY_FUNCTION__);

	PGLESContext2D* ctxt = context();
	luna_assert(ctxt);

	setToCurrent();
	if (!ctxt->RenderToWindow(0)) {
		g_error("RenderToWindow(0) failed in %s", __PRETTY_FUNCTION__);
		return false;
	}

	return true;
}
