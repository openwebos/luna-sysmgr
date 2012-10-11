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




#include "RemoteWindowDataSoftwareOpenGLComposited.h"

#include <stdio.h>
#include <glib.h>
#include <PGLESContext.h>
#include <PGLESPixmap.h>
#include <PIpcBuffer.h>
#include <PIpcChannel.h>
#include <QTime>
#include <WebGLES2Context.h>
#include <GLES2/gl2.h>

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>

#include "Logging.h"
#include "NAppWindow.h"
#include "WebAppManager.h"

namespace Palm {
	class WebGLES2Context;
	class WebView;
}


// -------------------------------------------------------------------------------------

RemoteWindowDataSoftwareOpenGLComposited::RemoteWindowDataSoftwareOpenGLComposited(int width, int height, bool hasAlpha, bool createIpcBuffer)
	: RemoteWindowDataSoftware(width, height, hasAlpha, createIpcBuffer)
	, m_glContext(0)
{
}

RemoteWindowDataSoftwareOpenGLComposited::~RemoteWindowDataSoftwareOpenGLComposited()
{
}

Palm::WebGLES2Context* RemoteWindowDataSoftwareOpenGLComposited::getGLES2Context()
{
	if (!m_glContext) {
		m_glContext = new PGLES2Context(this);
	}
	return m_glContext;
}

bool RemoteWindowDataSoftwareOpenGLComposited::directRenderingAllowed(bool val)
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

    if (m_glContext)
    	m_glContext->enableDirectRendering(val);
	return true;
#else
	return false;
#endif
}

bool RemoteWindowDataSoftwareOpenGLComposited::isDirectRendering()
{
	return m_directRendering;
}

void RemoteWindowDataSoftwareOpenGLComposited::translate(int x, int y)
{
	if (m_glContext)
		m_glContext->translate(x, y);
}

void RemoteWindowDataSoftwareOpenGLComposited::rotate(int degrees) {
	if (m_glContext)
		m_glContext->rotate(degrees);
}

void RemoteWindowDataSoftwareOpenGLComposited::sendCompositedTextureUpdate(int x, int y, int w, int h)
{
	luna_assert(m_channel);
	luna_assert(m_glContext);

	const unsigned int size = width() * height();
	static unsigned *imageBuffer = new unsigned[size];
	m_glContext->getPGLESContext()->GetPixels(PRect(0, 0, w, h), imageBuffer);

	m_ipcBuffer->lock();
	memcpy((char *) m_ipcBuffer->data(), (char*) imageBuffer, w * h * 4);
	m_ipcBuffer->unlock();

	m_channel->sendAsyncMessage(new ViewHost_UpdateWindowRegion(key(), 0, 0, w, h));
}

void RemoteWindowDataSoftwareOpenGLComposited::flip() {
	RemoteWindowDataSoftware::flip();
	if (m_glContext)
		m_glContext->recreateTexture();
}

// -------------------------------------------------------------------------------------

static PGLESContext2D* s_onScreenGLContext  = 0;
static PGLESContext2D* s_offScreenGLContext = 0;
static PGLES2Context* s_currentContext = 0;
static NAppWindow* s_dummyWindow = 0;


PGLES2Context::PGLES2Context(RemoteWindowDataSoftwareOpenGLComposited* data)
	: m_view(0)
	, m_parent(0)
	, m_glPixmap(0)
	, m_nappWindow(0)
	, m_data(data)
	, m_xOffset(0)
	, m_yOffset(0)
	, m_rotationDegrees(0)

{
}

PGLES2Context::~PGLES2Context() {

	destroy();

	if (this == s_currentContext)
		s_currentContext = 0;
}

bool PGLES2Context::initialize(Palm::WebView* view, WebGLES2Context* parent) {

	g_debug("%s called", __PRETTY_FUNCTION__);
	m_view = view;
	(void) context();
	createWindow(m_data->width(), m_data->height());

	bool result = m_data->isDirectRendering() ? renderOnScreen() : renderOffscreen();

#if defined(PALM_DEVICE)
	// Workaround for driver issue on qualcom 8660; to be removed eventually		
#define GL_USE_UNCACHED_VBOS_HINT_QCOM						 0x8FC0	   
	glHint(GL_USE_UNCACHED_VBOS_HINT_QCOM, GL_TRUE);
#endif

	return result;
}


void PGLES2Context::createWindow(int width, int height) {
	if (!m_nappWindow) {
		m_nappWindow = new NAppWindow();
		m_nappWindow->Set(width, height);
	}
}

void PGLES2Context::resizeWindow(int width, int height) {

	makeCurrent();

	NAppWindow* copyNappWindow = m_nappWindow;
	m_nappWindow = 0;

	createWindow(width, height);

	if (m_data->isDirectRendering()) {
		// we technically should not get here while direct rendering,
		// since flip() is called after turning off direct rendering by the host side
		if (!s_onScreenGLContext->RenderToWindow(0)) {
			g_error("RenderToWindow(0) failed in %s (%d)",
					__PRETTY_FUNCTION__, __LINE__);
			return;
		}
	} else {

		if (!s_offScreenGLContext->RenderToWindow(m_nappWindow)) {
			g_error("RenderToWindow(m_nappWindow) failed in %s (%d)",
					__PRETTY_FUNCTION__, __LINE__);
			return;
		}
	}

	makeCurrent();

	delete copyNappWindow;
}

void PGLES2Context::enableDirectRendering(bool val) {

	if (val)
		renderOnScreen();
	else
		renderOffscreen();
}

bool PGLES2Context::makeCurrent()
{
	if (this == s_currentContext)
		return true;

	setToCurrent();

	if (m_data->isDirectRendering()) {
		if (!s_onScreenGLContext->RenderToWindow(0))
			g_error("RenderToWindow(0) failed in %s (%d)",
					__PRETTY_FUNCTION__, __LINE__);
	}
	else {
		if (!s_offScreenGLContext->RenderToWindow(m_nappWindow))
			g_error("RenderToWindow(m_nappWindow) failed in %s (%d)",
					__PRETTY_FUNCTION__, __LINE__);
	}

	return true;
}

bool PGLES2Context::destroy() {

	setToCurrent();
		
	s_offScreenGLContext->RenderToWindow(s_dummyWindow);

	if (m_glPixmap) {
		s_offScreenGLContext->DestroyPixmap(m_glPixmap);
		m_glPixmap = 0;
	}

	if (m_nappWindow) {
		delete m_nappWindow;
		m_nappWindow = 0;
	}

	return true;
}

bool PGLES2Context::swapBuffers() {
	if (m_data->isDirectRendering()) {
		s_onScreenGLContext->Flip();
	} else {
		m_data->sendCompositedTextureUpdate(0, 0, m_data->width(), m_data->height());
	}

	return true;
}

void PGLES2Context::translate(int x, int y) {
	m_xOffset = x;
	m_yOffset = y;
}

void PGLES2Context::getTranslation(int& x, int& y) {
	x = m_xOffset;
	y = m_yOffset;
}

void PGLES2Context::getScreenWidthAndHeight(int& width, int& height) {
	WebAppManager* wam = WebAppManager::instance();
	width = wam->currentUiWidth();
	height = wam->currentUiHeight();
}

void PGLES2Context::rotate(int degrees) {
	m_rotationDegrees = degrees;
}

int PGLES2Context::getRotation() {
	return m_rotationDegrees;
}

bool PGLES2Context::isDirectRendering() {
	return m_data->isDirectRendering();
}

void PGLES2Context::recreateTexture() {
	if (m_glPixmap) {
		resizeWindow(m_data->width(), m_data->height());

		setToCurrent();
		
		if (!s_offScreenGLContext->RenderToTexture(0, false)) {
			g_error("RenderToTexture(0) failed in %s (%d)",
					__PRETTY_FUNCTION__, __LINE__);
		}

		g_debug("%s: resizing glpixmap to %d,%d", __FUNCTION__, m_data->width(), m_data->height());
		s_offScreenGLContext->DestroyPixmap(m_glPixmap);
		m_glPixmap = s_offScreenGLContext->CreatePixmap(m_data->width(), m_data->height(), PFORMAT_8888, 0);

		if (m_data->isDirectRendering()) {
			// we technically should not get here while direct rendering,
			// since flip() is called after turning off direct rendering by the host side
			if (!s_onScreenGLContext->RenderToWindow(0)) {
				g_error("RenderToWindow(0) failed in %s (%d)",
						__PRETTY_FUNCTION__, __LINE__);
			}
		} else {
			if (!s_offScreenGLContext->RenderToTexture(m_glPixmap, false)) {
				g_error("RenderToTexture(m_glPixmap) failed in %s (%d)",
						__PRETTY_FUNCTION__, __LINE__);
			}
		}
	}
}

// Resizes the backing store used for offscreen rendering.
void PGLES2Context::reshape(int width, int height) {
}

// Returns the ID of the texture used for offscreen rendering in the context of the parent.
PGLESPixmap* PGLES2Context::getOffscreenContentParentTextureId() {
	return m_glPixmap;
}

PGLESContext2D* PGLES2Context::getPGLESContext() {
	return context();
}

bool PGLES2Context::setDisplay() {
	return true;
}

bool PGLES2Context::renderOffscreen() {
	g_debug("%s called", __PRETTY_FUNCTION__);

	luna_assert(s_offScreenGLContext);

	setToCurrent();

	if (!m_glPixmap) {
		m_glPixmap = s_offScreenGLContext->CreatePixmap(m_data->width(),
														m_data->height(),
														PFORMAT_8888, 0);
	}
	
	createWindow(m_data->width(), m_data->height());
	
	if (!s_offScreenGLContext->RenderToWindow(m_nappWindow)) {
		g_error("RenderToWindow(m_nappWindow) failed in %s (%d)",
				__PRETTY_FUNCTION__, __LINE__);
		return false;
	}


	if (!s_offScreenGLContext->RenderToTexture(m_glPixmap, false)) {
		g_error("RenderToTexture failed in %s (%d)",
				__PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	
	return true;
}

bool PGLES2Context::renderOnScreen() {

	g_debug("%s called", __PRETTY_FUNCTION__);

	luna_assert(s_onScreenGLContext);

	setToCurrent();
	
	if (!s_onScreenGLContext->RenderToWindow(0)) {
		g_error("RenderToWindow(0) failed in %s (%d)",
				__PRETTY_FUNCTION__, __LINE__);
		return false;
	}

	if (!s_onScreenGLContext->RenderToTexture(0, false)) {
		g_error("RenderToTexture(0) failed in %s (%d)",
				__PRETTY_FUNCTION__, __LINE__);
		return false;
	}

	return true;
}
	
PGLESContext2D* PGLES2Context::context()
{
	if (G_UNLIKELY(s_onScreenGLContext == 0)) {

		s_onScreenGLContext = new PGLESContext2D;
		if (!s_onScreenGLContext->SetDisplay(PPrimary, 1, PFORMAT_8888, 0)) {
			g_error("SetDisplay(PPrimary...) failed in %s (%d)",
					__PRETTY_FUNCTION__, __LINE__);
			delete s_onScreenGLContext;
			s_onScreenGLContext = 0;
			return 0;
		}

		// Magic settings to eliminate stencil buffer in Piranha
		s_onScreenGLContext->SetAttribute(2,1);
	}

	if (G_UNLIKELY(s_offScreenGLContext == 0)) {

		s_offScreenGLContext = new PGLESContext2D;
		if (!s_offScreenGLContext->SetDisplay(PPrimary, 1, PFORMAT_8888, 0)) {
			g_error("SetDisplay(PPrimary...) failed in %s (%d)",
					__PRETTY_FUNCTION__, __LINE__);
			delete s_offScreenGLContext;
			s_offScreenGLContext = 0;
			return 0;
		}

		// Magic settings to eliminate stencil buffer in Piranha
		s_offScreenGLContext->SetAttribute(2,1);
	}

	if (G_UNLIKELY(s_dummyWindow == 0)) {

		WebAppManager* wam = WebAppManager::instance();
		s_dummyWindow = new NAppWindow();
		s_dummyWindow->Set(wam->currentUiWidth(), wam->currentUiHeight());
	}


	if (m_data->isDirectRendering())
		return s_onScreenGLContext;
	else
		return s_offScreenGLContext;
}

void PGLES2Context::setToCurrent()
{
	s_currentContext = this;
}
