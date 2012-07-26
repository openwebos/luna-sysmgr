/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
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

#include <PGSurface.h>
#include <PGContext.h>

#include "CardWebApp.h"
#include "PIpcChannel.h"
#include "PIpcBuffer.h"
#include "WindowContentTransitionRunner.h"
#include "SyncTask.h"

#define MESSAGES_INTERNAL_FILE "SysMgrMessagesInternal.h"
#include <PIpcMessageMacros.h>


WindowContentTransitionRunner* WindowContentTransitionRunner::instance()
{
	static WindowContentTransitionRunner* s_instance = 0;
	if (G_UNLIKELY(s_instance == 0))
		s_instance = new WindowContentTransitionRunner;

	return s_instance;
}

WindowContentTransitionRunner::WindowContentTransitionRunner()
	: m_mainLoop(0)
	, m_mainCtxt(0)
	, m_transitionType(Transition_Invalid)
	, m_app(0)
	, m_channel(0)
	, m_ipcBuffer(0)
	, m_dstContext(0)
	, m_fromSceneSurface(0)
	, m_toSceneSurface(0)
	, m_currRotateAngle(0)
	, m_targRotateAngle(0)
{
	m_mainCtxt = g_main_context_new();
	m_mainLoop = g_main_loop_new(m_mainCtxt, TRUE);
}

WindowContentTransitionRunner::~WindowContentTransitionRunner()
{
    // not reached
}

void WindowContentTransitionRunner::runRotateTransition(CardWebApp* app,
														PGSurface* toSurface,
														PGContext* dstContext,
														int currAngle,
														int targAngle)
{
	m_transitionType = Transition_Rotate;
	m_app = app;
	m_toSceneSurface = toSurface;
	m_dstContext = dstContext;
	m_currRotateAngle = currAngle;
	m_targRotateAngle = targAngle;

	// -----------------------------------------------------------------------	

	runLoop();

	// -----------------------------------------------------------------------	
	
	m_transitionType = Transition_Invalid;
	m_app = 0;
	m_toSceneSurface = 0;
	m_dstContext = 0;
	m_currRotateAngle = 0;
	m_targRotateAngle = 0;
}

void WindowContentTransitionRunner::runLoop()
{
	GSource* src = g_timeout_source_new(33);
	g_source_set_callback(src, WindowContentTransitionRunner::sourceCallback, this, NULL);
	g_source_attach(src, m_mainCtxt);
	g_source_unref(src);

	g_main_loop_run(m_mainLoop);    
}

#define SCALE_DELTA(val, numer, denom)									\
	if (val > 0)														\
		val = MAX((val * numer)/denom, 1);								\
	else if (val < 0)													\
		val = MIN((val * numer)/denom, -1);								\

bool WindowContentTransitionRunner::timerTicked()
{
	bool ret = false;
	
	switch (m_transitionType) {
	case Transition_Rotate:
		ret = rotateTick();
		break;
	default:
		return false;
	}

	if(m_channel && m_ipcBuffer)
		m_channel->sendAsyncMessage(new ViewHost_UpdateFullWindow(m_ipcBuffer->key()));	
	
    return ret;
}

gboolean WindowContentTransitionRunner::sourceCallback(gpointer arg)
{
	WindowContentTransitionRunner* runner = (WindowContentTransitionRunner*) arg;
	bool ret = runner->timerTicked();
	if (!ret)
		g_main_loop_quit(runner->m_mainLoop);

	return ret;
}

bool WindowContentTransitionRunner::rotateTick()
{
// not used anymore - see:
// DockWebApp::resizeWindowForOrientation and
// CardWebApp::resizeWindowForOrientation being commented out.
// We can probably reimplement this much cheaper by implementing the animation
// with GraphicsView's view transform.
/*
	static const int danielNumer = 3;
	static const int danielDenom = 5;
	
	if (m_targRotateAngle == m_currRotateAngle) {

		return false;
	}

	int delta = m_targRotateAngle - m_currRotateAngle;
	SCALE_DELTA(delta, danielNumer, danielDenom);

	m_currRotateAngle += delta;
	
	// Painting --------------------------------------------------------------

	m_app->beginPaint();
	
	m_dstContext->push();

	// Fill with black
	m_dstContext->setStrokeColor(PColor32(0x00, 0x00, 0x00, 0x00));
	m_dstContext->setFillColor(PColor32(0x00, 0x00, 0x00, 0xFF));
	m_dstContext->drawRect(0, 0, (int) m_toSceneSurface->width(), (int) m_toSceneSurface->height());

	m_dstContext->translate((int) m_toSceneSurface->width()/2, (int) m_toSceneSurface->height()/2);
	m_dstContext->rotate(m_currRotateAngle);
	m_dstContext->translate(-(int) m_toSceneSurface->width()/2, -(int) m_toSceneSurface->height()/2);
	m_dstContext->bitblt(m_toSceneSurface, 0, 0,
						 (int) m_toSceneSurface->width(),
						 (int) m_toSceneSurface->height());

	m_dstContext->pop();

	m_app->endPaint();
	
	return true;    
*/
}
